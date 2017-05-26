/*
 * Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
 * 2006, The Regents of the University of California, through Lawrence
 * Berkeley National Laboratory (subject to receipt of any required
 * approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * Portions may be copyrighted by others, as may be noted in specific
 * copyright notices within specific files.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: crut.c,v 1.32 2008/11/30 22:56:41 phargrov Exp $
 */
#define _GNU_SOURCE	/* To get prototype for getpgid() */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <linux/limits.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "crut.h"
#include "libcr.h"


static char *context_filename;
static int crut_initialized = 0;
static struct crut_operations crut_tests[CRUT_MAX_TESTS];
static char crut_usage_message[] = {
"\n"
"  -h,-?  --help    Print this help message.\n"
"  -l     --list    List tests.\n"
"  -v     --verbose Show checkpoint steps.\n"
"  -d     --debug   Show debugging messages.\n"
"\n"
};

enum crut_state {
    crut_error=0,
    crut_continue,
    crut_restart,
};

static int crut_callback_counter = 0;
static enum crut_state crut_checkpoint_status = crut_error;
static int crut_saved_error = 0;

/*
 *  * crut_wait and crut_signal use this to keep track of where we are in the
 *   * checkpoint progression
 *    */
static int crut_event_state = 0;
/* lock for crut_event_state */
static pthread_mutex_t crut_event_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t crut_event_condition = PTHREAD_COND_INITIALIZER;

int optind;

/* Extended perror() while preserving errno */
static void my_perror(const char *msg)
{
    int saved_errno = errno;
    fprintf(stderr, "%s: %s\n", msg, cr_strerror(errno));
    errno = saved_errno;
}

/*
 * We use a callback to find out whether we're continuing or restarting.
 */
static int
crut_callback(void *arg)
{
    int ret;

    ++crut_callback_counter;
    ret = cr_checkpoint(0);
    if (ret > 0) {
        crut_checkpoint_status = crut_restart;
    } else if (ret == 0) {
        crut_checkpoint_status = crut_continue;
    } else {
        crut_checkpoint_status = crut_error;
        crut_saved_error = ret;
    }

    return 0;
}

static void
crut_list_tests(FILE *stream)
{
    int i=0;

    while (crut_tests[i].test_name[0] != '\0' && i < CRUT_MAX_TESTS) {
        fprintf(stream, "%s\n", crut_tests[i].test_name);
	i++;
    } 
}

static void
crut_usage(FILE *stream, char *name)
{
    fprintf(stream, "Usage: %s [OPTIONS] test_name\n", name);
}

static void
crut_help(FILE *stream)
{
    int i;

    fprintf(stream, "%s", crut_usage_message);
    fprintf(stream, "Tests available:\n");
    for (i = 0; i < CRUT_MAX_TESTS && crut_tests[i].test_name[0] != '\0'; ++i) {
	fprintf(stream, "  %s: %s\n", crut_tests[i].test_name, crut_tests[i].test_description);
    }
    fprintf(stream, "\nSubmit bugs to http://mantis.lbl.gov/bugzilla\n");
}

static struct crut_operations *
crut_find_test(const char *test_name)
{
    int i;

    for (i=0; i<CRUT_MAX_TESTS; ++i) {
        if (!strncmp(crut_tests[i].test_name, test_name, CRUT_TESTNAME_MAX)) {
            return &crut_tests[i];
	}
    }

    return NULL;
}

static char *
init_context_filename(void)
{
    int pid;
    char cwd[PATH_MAX+1];
    char *p;

    pid = getpid();
    if (pid < 0) {
        my_perror("getpid");
	return NULL;
    }

    p = cwd;
    p = getcwd(cwd, sizeof(cwd)); 
    if (p == NULL || p != cwd) {
	my_perror("getcwd");
        return NULL;
    }

    context_filename = crut_aprintf("%s/context.%d", cwd, pid);

    return context_filename;
}

static void
initialize_crut(void)
{
    memset(&crut_tests, 0, sizeof(crut_tests));
    /*
     * XXX:  Is this correct?
     */
    // crut_event_mutex = PTHREAD_MUTEX_INITIALIZER;
    // crut_event_condition = PTHREAD_COND_INITIALIZER;
    crut_event_state = 0;

    if (init_context_filename() == NULL) {
	goto abort;
    }

    crut_initialized = 1;

    return;

abort:
    exit(-1);
}

/*
 * crut_wait - blocks until the test method will be called.
 *
 * Note: The thread is signalled just before the test method is invoked.  e.g.
 * crut_wait(CRUT_EVENT_SETUP) will wake up slightly before TEST_SETUP is
 * called.
 *
 * Note: If the event has been raised already, this returns immediately.
 * i.e.  if you wait for CRUT_EVENT_SETUP after setup has completed, then
 * this returns immediately.
 *
 * If you want to wait for CRUT_EVENT_CONTINUE or CRUT_EVENT_RESTART,
 * it is sufficient to wait only for CRUT_EVENT_CONTINUE.  i.e.
 * crut_wait(CRUT_EVENT_CONTINUE) will return after a restart (or teardown) 
 * also.
 */
int
crut_wait(crut_event_t event)
{
   int retval;

   retval = pthread_mutex_lock(&crut_event_mutex);
   if (retval < 0) {
       goto out;
   }
   while ((crut_event_state < event) && !retval) {
       retval = pthread_cond_wait(&crut_event_condition, &crut_event_mutex);
       if (retval) {
           /* pthread_cond_wait can be interrupted */
           my_perror("pthread_cond_wait(crut_event_condition)");
           goto out_unlock;
       }
   }
   retval = pthread_mutex_unlock(&crut_event_mutex);
   if (retval < 0) {
       my_perror("pthread_mutex_unlock");
       goto out;
   }

out:
   return retval;

out_unlock:
   if(pthread_mutex_unlock(&crut_event_mutex)) {
       /* ignore return code of unlock on error */
       my_perror("pthread_mutex_unlock");
   }
   return retval;
}

/*
 * crut_poll
 *
 * Check to see whether an event has occured.
 *
 * Returns 0 if the event has not occurred, 1 if it has.
 *      < 0 on error 
 */
int
crut_poll(crut_event_t event)
{
    int retval;

    retval = pthread_mutex_lock(&crut_event_mutex);
    if (retval < 0) {
        my_perror("pthread_mutex_lock");
        goto out;
    }

    if (event <= crut_event_state) {
        retval = 1;
    } else {
        retval = 0;
    }

    /* check second, in case they passed in a negative #.  
     * but should really just validate input to ensure event > 0.
     */
    if (crut_event_state <= 0) {
        fprintf(stderr, "crut_poll:  Never initialized!\n");
        retval = -1;
    }

    if (pthread_mutex_unlock(&crut_event_mutex) < 0) {
        my_perror("WARNING: pthread_mutex_unlock");
        goto out;
    }

out:
    return retval;

#if 0 // unused path
out_unlock:
    if (pthread_mutex_unlock(&crut_event_mutex)) {
        /* ignore return code of unlock on error */
        my_perror("pthread_mutex_unlock");
    }
    return retval;
#endif
}

/*
 * wake up all processes waiting for this event or any preceding events
 */
static int
crut_signal(crut_event_t event)
{
    int retval;
    retval = pthread_mutex_lock(&crut_event_mutex);
    if (retval) {
        my_perror("pthread_mutex_lock");
        goto out;
    }

    crut_event_state = event;

    retval = pthread_cond_broadcast(&crut_event_condition);
    if (retval) {
        my_perror("pthread_cond_broadcast");
        goto out_unlock;
    }

    retval = pthread_mutex_unlock(&crut_event_mutex);
    if (retval) {
        my_perror("pthread_mutex_unlock");
        goto out;
    }

out:
    return retval;

out_unlock:
    if(pthread_mutex_unlock(&crut_event_mutex)) {
        /* ignore return code of unlock on error */
        my_perror("pthread_mutex_unlock");
    }
    return retval;
}

void 
crut_exit(int exitcode)
{
    exit(exitcode);
}


void
crut_add_test(struct crut_operations *test_ops)
{
    int i;
    const char noname[] = "(unnamed)";

    /* We fix their input as a side effect... */
    test_ops->test_name[CRUT_TESTNAME_MAX] = '\0';

    if (!crut_initialized) {
	initialize_crut();
    }

    for (i=0; i<CRUT_MAX_TESTS; ++i) {
        if (!strncmp(crut_tests[i].test_name, test_ops->test_name, strlen(test_ops->test_name))) {
            fprintf(stderr, "Duplicate test (%s).\n", crut_tests[i].test_name);
	    exit(-1);
	} else if (crut_tests[i].test_name[0] == '\0') {
            memcpy(&crut_tests[i], test_ops, sizeof(*test_ops));
            if (crut_tests[i].test_name[0] == '\0') {
                strncpy(crut_tests[i].test_name, noname, sizeof(noname));
            }
	    goto out;
	}
    }

    switch (test_ops->test_scope) {
    case CR_SCOPE_PROC:
    case CR_SCOPE_TREE:
    case CR_SCOPE_PGRP:
	break;

    case CR_SCOPE_SESS:
	/* Not automated, since parent must exit for setsid() to work and
	 * that prevents returning an exit code.
	 * XXX: This could be implemented, if really needed, by sending
	 * the return value through a named fifo or something similar.
	 */ 
    default:
        CRUT_FAIL("Invalid scope %d specified", test_ops->test_scope);
	exit(-1);
    }

out:
    return;
}

/* XXX: global so dup test can read it */
cr_checkpoint_handle_t crut_cr_handle;

int crut_checkpoint_to_file(const char *filename, int scope)
{
    int ret;
    cr_checkpoint_args_t cr_args;

    /* remove existing context file, if any */
    (void)unlink(filename);

    /* open the context file */
    CRUT_VERBOSE("opening the context file: %s", filename);
    ret = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE, 0600);
    if (ret < 0) {
        my_perror("open");
        goto out;
    }

    cr_initialize_checkpoint_args_t(&cr_args);
    cr_args.cr_fd = ret;
    cr_args.cr_scope = scope;

    /* issue the request */
    ret = cr_request_checkpoint(&cr_args, &crut_cr_handle);
    if (ret < 0) {
        (void)close(cr_args.cr_fd);
        (void)unlink(filename);
        my_perror("cr_request_checkpoint");
        goto out;
    }

    /* wait for the request to complete */
    do {
	char *kmsgs = NULL;
	ret = cr_poll_checkpoint_msg(&crut_cr_handle, NULL, &kmsgs);
	if (ret < 0) {
	    if ((ret == CR_POLL_CHKPT_ERR_POST) && (errno == CR_ERESTARTED)) {
		/* restarting -- not an error */
                ret = 0;
	    } else if (errno == EINTR) {
                /* retry */
                ;
            } else {
		int saved_errno = errno;
		fprintf(stderr, "cr_poll_checkpoint returned %d: %s\n", ret, cr_strerror(errno));
		if (kmsgs) {
			fputs(kmsgs, stderr);
		}
		errno = saved_errno;
		goto out;
	    }
        } else if (ret == 0) {
            fprintf(stderr, "cr_poll_checkpoint returned unexpected 0\n");
	    exit(1);
        }
    } while (ret < 0);

    close(cr_args.cr_fd);
out:
    return ret;
}

int
crut_main(int argc, char * const *argv)
{
    int ret;
    struct crut_operations *test_ops;
    char *test_name;
    void *testdata = NULL;
    cr_client_id_t client_id = 0;

    int opt_fake_cr = 0;

    char *shortflags = "Flh?vd";	/* 1 colon == requires argument */
    struct option longflags[] = {
	/* misc options: */
	{"fake-cr", no_argument, 0, 'F'},
	{"list", no_argument, 0, 'l'},
	{"help", no_argument, 0, '?'},
	{"debug", no_argument, 0, 'd'},
	{"verbose", no_argument, 0, 'v'},
	{0, 0, 0, 0}
    };

    while (1) {
	int longindex = -1;
	int opt = getopt_long(argc, argv, shortflags, longflags, &longindex);

	if (opt == -1) {
	    ret = optind;
	    break;		       /* reached last option */
	}

	switch (opt) {
	case 'F':
	    opt_fake_cr = 1;
	    break;
	case 'l':
	    crut_list_tests(stdout);
	    exit(0);
	case '?':
	case 'h':
	    crut_usage(stdout, argv[0]);
	    crut_help(stdout);
	    exit(0);
	case 'v':
	    crut_trace_mask |= CRUT_TRACETYPE_VERBOSE;
	    break;
	case 'd':
	    crut_trace_mask |= (CRUT_TRACETYPE_DEBUG | CRUT_TRACETYPE_VERBOSE);
	    break;
	default:
	    crut_usage(stderr, argv[0]);
            fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
	    exit(-1);
	}

    }

    crut_program_name = argv[0];

    if (!argv[ret] || ret > argc) {
        crut_usage(stderr, crut_program_name);
        fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
        exit(-1);
    }

    test_name = argv[ret];
    test_ops = crut_find_test(test_name);
    if (!test_ops) {
        CRUT_FAIL("No such test %s", test_name);
	exit(-1);
    }

    CRUT_VERBOSE("PID %d", getpid());

    if (opt_fake_cr) {
	CRUT_VERBOSE("Skipping cr_init and cr_register_callback.");
    } else {
        CRUT_VERBOSE("cr_init()");
        client_id = cr_init();
        if (client_id < 0) {
            CRUT_FAIL("cr_init() failed.  ret=%d.  errno=%d", client_id, errno);
            my_perror("crut");
            crut_exit(client_id);
        }
    
        CRUT_VERBOSE("cr_register_callback()");
        ret = cr_register_callback(crut_callback, NULL, CR_SIGNAL_CONTEXT);
        if (ret < 0) {
            CRUT_FAIL("cr_register_callback() failed.  ret=%d", ret);
            crut_exit(ret);
        }
    }

    if (test_ops->test_setup) {
	CRUT_VERBOSE("test_setup()");
        ret = crut_signal(CRUT_EVENT_SETUP);
        if (ret < 0) {
	    CRUT_FAIL("crut_signal(CRUT_EVENT_SETUP) failed.  ret=%d", ret);
	    crut_exit(ret);
	}
	ret = test_ops->test_setup(&testdata);
	if (ret < 0) {
	    CRUT_FAIL("test_setup() failed.  ret=%d", ret);
	    crut_exit(ret);
	}
    }

    CRUT_VERBOSE("test_precheckpoint()");
    ret = crut_signal(CRUT_EVENT_PRECHECKPOINT);
    if (ret < 0) {
        CRUT_FAIL("crut_signal(CRUT_EVENT_PRECHECKPOINT) failed.  ret=%d", ret);
        crut_exit(ret);
    }
    ret = test_ops->test_precheckpoint(testdata);
    if (ret < 0) {
	CRUT_FAIL("test_precheckpoint() failed.  ret=%d", ret);
	crut_exit(ret);
    }

    if (opt_fake_cr) {
	CRUT_VERBOSE("Skipping cr_request_checkpoint() and faking restart.");
	crut_checkpoint_status = crut_restart;
    } else {
        CRUT_VERBOSE("crut_checkpoint_to_file()");
        ret = crut_checkpoint_to_file(context_filename, test_ops->test_scope);
        if (ret < 0) {
            CRUT_FAIL("crut_checkpoint_to_file(%s, %d) failed ret=%d errno=%s", context_filename, test_ops->test_scope, ret, cr_strerror(errno));
            crut_exit(ret);
        }
        ret = cr_status();
        if (ret != CR_STATE_IDLE) {
	    CRUT_FAIL("cr_status() unexpectedly returned %d", ret);
	    crut_exit(ret);
        }
    }

    switch (crut_checkpoint_status) {
    case crut_continue:
	CRUT_VERBOSE("test_continue()");
        ret = crut_signal(CRUT_EVENT_CONTINUE);
        if (ret < 0) {
            CRUT_FAIL("crut_signal(CRUT_EVENT_CONTINUE) failed.  ret=%d", 
		    ret);
            crut_exit(ret);
        }
	ret = test_ops->test_continue(testdata);
	if (ret < 0) {
	    CRUT_FAIL("test_continue() unexpectedly returned %d", ret);
	    crut_exit(ret);
	}
	CRUT_VERBOSE("test_continue() complete");
	break;
    case crut_restart:
	CRUT_VERBOSE("test_restart()");
        ret = crut_signal(CRUT_EVENT_RESTART);
        if (ret < 0) {
            CRUT_FAIL("crut_signal(CRUT_EVENT_RESTART) failed.  ret=%d",
		    ret);
            crut_exit(ret);
        }
	ret = test_ops->test_restart(testdata);
	if (ret < 0) {
	    CRUT_FAIL("test_restart() unexpectedly returned %d", ret);
	    crut_exit(ret);
	}

        if (test_ops->test_teardown) {
	    CRUT_VERBOSE("test_teardown()");
            ret = crut_signal(CRUT_EVENT_TEARDOWN);
            if (ret < 0) {
                CRUT_FAIL("crut_signal(CRUT_EVENT_TEARDOWN) failed.  ret=%d",
		       	ret);
                crut_exit(ret);
            }
	    ret = test_ops->test_teardown(testdata);
	    if (ret < 0) {
	        CRUT_FAIL("test_teardown() failed.  ret=%d", ret);
	        crut_exit(ret);
	    }
        }
	break;
    case crut_error:
    default:
	CRUT_FAIL("Error during checkpoint.  crut_checkpoint_status = %d, saved error = %d",
		  crut_checkpoint_status, crut_saved_error);
	crut_exit(-1);
    }

    crut_initialized = 0;

    CRUT_DEBUG("Exiting successfully.");
    crut_exit(0);

    /* NOT REACHED */
    return 0; /* avoid compiler warning */
}
