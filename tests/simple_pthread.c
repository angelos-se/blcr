/*
 * Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
 * 2003, The Regents of the University of California, through Lawrence
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
 * $Id: simple_pthread.c,v 1.11 2008/11/30 04:36:56 phargrov Exp $
 *
 * Simple example for using crut
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include <pthread.h>

#include "crut.h"
#include "crut_util.h"

#define MSG "Hello world!\n"
#define NUM_PTHREADS 5

static int my_barrier;

static void *
do_thread_stuff(void *p)
{
    int retval;
    void *thread_ret;

    thread_ret = malloc(sizeof(int));
    if (thread_ret == NULL) {
        goto out_nomem;
    }

    CRUT_DEBUG("thread_ret = %p", thread_ret);

    /* check that the poll routines are working correctly. */
    retval = crut_poll(CRUT_EVENT_SETUP);
    if (retval < 0) {
        goto out;
    } else if (retval == 0) {
        CRUT_FAIL("crut_poll(CRUT_EVENT_SETUP) returned wrong answer!");
	goto out;
    }

    /* check that the poll routines are working correctly. */
    retval = crut_poll(CRUT_EVENT_NEVER);
    if (retval < 0) {
        goto out;
    } else if (retval == 1) {
        CRUT_FAIL("crut_poll(CRUT_EVENT_NEVER) returned wrong answer!");
	goto out;
    }

    CRUT_DEBUG("crut_wait(setup)");
    retval = crut_wait(CRUT_EVENT_SETUP);
    if (retval < 0) {
        goto out;
    }

    CRUT_DEBUG("crut_wait(precheckpoint)");
    retval = crut_wait(CRUT_EVENT_PRECHECKPOINT);
    if (retval < 0) {
        goto out;
    }

    CRUT_DEBUG("crut_wait(continue)");
    retval = crut_wait(CRUT_EVENT_CONTINUE);
    if (retval < 0) {
        goto out;
    }

    crut_barrier(&my_barrier);

    CRUT_DEBUG("crut_wait(restart)");
    retval = crut_wait(CRUT_EVENT_RESTART);
    if (retval < 0) {
        goto out;
    }

    CRUT_DEBUG("crut_wait(teardown)");
    retval = crut_wait(CRUT_EVENT_TEARDOWN);
    if (retval < 0) {
        goto out;
    }

    *(int *) thread_ret = retval;

out:
    return thread_ret;

out_nomem:
    return NULL;
}

static int
simple_pthread_setup(void **testdata)
{
    int retval;
    pthread_t *test_thread;
    int num_threads = NUM_PTHREADS;
    int i;

    test_thread = malloc(sizeof(pthread_t)*num_threads);
    if (test_thread == NULL) {
        perror("malloc");
	goto out;
    }

    my_barrier = NUM_PTHREADS+1;

    for (i=0; i<num_threads; ++i) {
        retval = crut_pthread_create(&test_thread[i], NULL, do_thread_stuff, NULL);
        if (retval) {
            perror("pthread_create"); 
	    retval = -1;
	    goto out_cancel;
        }
    }

out:
    *testdata = test_thread;
    return 0;

out_cancel:
    for ( ; i>=0; --i) {
        if (pthread_cancel(test_thread[i])) {
            perror("pthread_cancel");
        }
    }
    free(test_thread);
    *testdata = NULL;
    return retval;
}

static int
simple_pthread_precheckpoint(void *p)
{
    CRUT_DEBUG("Preparing to checkpoint.");

    return 0;
}

static int
simple_pthread_continue(void *p)
{
    CRUT_DEBUG("Continuing after checkpoint.");
    crut_barrier(&my_barrier);

    return 0;
}

static int
simple_pthread_restart(void *p)
{
    CRUT_DEBUG("Restarting from checkpoint.");
    crut_barrier(&my_barrier);

    return 0;
}

static int
simple_pthread_teardown(void *p)
{
    int retval;
    int join_ret;
    void *pthread_ret;
    int *thread_ret;
    pthread_t *threads = (pthread_t *) p;
    int num_threads = NUM_PTHREADS;
    int i;

    retval = 0;
    for (i=0; i<num_threads; ++i) {
	join_ret = pthread_join(threads[i], &pthread_ret);
	if (join_ret) {
	    /* what else can we do? */
            perror("pthread_join");
	    retval = join_ret;
	}

	thread_ret = (int *) pthread_ret;

	/* check return value from thread */
	if (thread_ret != NULL) {
            CRUT_DEBUG("Thread %d.  return *(%p)=%d.", i, thread_ret, 
		    *thread_ret);
	    if (*thread_ret < 0) {
                CRUT_FAIL("Thread %d failed (return %d)", i, *thread_ret);
	    }
	    free(pthread_ret);
	} else {
            CRUT_FAIL("Thread %d failed (returned NULL)", i);
	}
    }

    free(threads);

    return retval;
}

int
main(int argc, char *argv[])
{
    int ret;
    struct crut_operations simple_pthread_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"simple_pthread_basic",
        test_description:"Simple example test using pthreads.",
	test_setup:simple_pthread_setup,
	test_precheckpoint:simple_pthread_precheckpoint,
	test_continue:simple_pthread_continue,
	test_restart:simple_pthread_restart,
	test_teardown:simple_pthread_teardown,
    };

    /* add the basic tests */
    crut_add_test(&simple_pthread_test_ops);

    ret = crut_main(argc, argv);

    return ret;
}
