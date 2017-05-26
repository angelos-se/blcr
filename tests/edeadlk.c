/*
 * Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
 * 2007, The Regents of the University of California, through Lawrence
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
 * $Id: edeadlk.c,v 1.3 2008/07/26 06:38:15 phargrov Exp $
 */

/* Tests implementation of CR_CHKPT_PROHIBIT_SELF flag */

#define _LARGEFILE64_SOURCE 1
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <libcr.h>
#include "crut_util.h"

#ifndef O_LARGEFILE
  #define O_LARGEFILE 0
#endif

enum {
	MSG_CHILD_READY,
	MSG_REGISTER_REQUEST,
	MSG_REGISTER_REPLY,
	MSG_RESULT_QUERY,
	MSG_PARENT_DONE,
	BAD_EXIT_VAL,
};

static volatile int cb_result = 0;

static int my_callback(void *arg)
{
    struct crut_pipes *pipes = arg;
    int rc;
    
    rc = cr_forward_checkpoint(CR_SCOPE_PROC, pipes->parent);
    cb_result = (rc < 0) ? errno : rc;

    return 0;
}

static void child_main(struct crut_pipes *pipes)
{
    cr_callback_id_t cb_id;
    cr_client_id_t my_id;
    int mypid = getpid();

    my_id = cr_init();
    if (my_id < 0) {
	printf("XXX child %d cr_init() failed, returning %d\n", mypid, my_id);
	exit(-1);
    }

    crut_pipes_putchar(pipes, MSG_CHILD_READY);

    crut_pipes_expect(pipes, MSG_REGISTER_REQUEST);
    cb_id = cr_register_callback(my_callback, pipes, CR_SIGNAL_CONTEXT);
    if (cb_id < 0) {
	printf("XXX cr_register_callback() unexpectedly returned %d\n", cb_id);
	exit(-1);
    }
    crut_pipes_putchar(pipes, MSG_REGISTER_REPLY);

    while (1) {
	crut_pipes_expect(pipes, MSG_RESULT_QUERY);
	crut_pipes_putchar(pipes, cb_result);
    }

    exit (BAD_EXIT_VAL);
}

static int checkpoint_to_file(const char *filename, pid_t target, int flags)
{
    int ret;
    cr_checkpoint_handle_t cr_handle;
    cr_checkpoint_args_t cr_args;

    /* open the context file */
    ret = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE, 0600);
    if (ret < 0) {
        perror("open");
        goto out;
    }

    cr_initialize_checkpoint_args_t(&cr_args);
    cr_args.cr_fd = ret;
    cr_args.cr_scope = CR_SCOPE_PROC;
    cr_args.cr_flags = flags;
    cr_args.cr_target = target;

    /* issue the request */
    ret = cr_request_checkpoint(&cr_args, &cr_handle);
    if (ret < 0) {
        ret = errno;
        goto out;
    }

    /* wait for the request to complete */
    do {
	ret = cr_poll_checkpoint(&cr_handle, NULL);
	if (ret < 0) {
	    if (ret == CR_POLL_CHKPT_ERR_POST && errno == CR_ERESTARTED) {
		/* restarting -- not an error */
                ret = 0;
	    } else if (errno == EINTR) {
                /* retry */
                ;
            } else {
		perror("cr_poll_checkpoint");
		goto out;
	    }
        } else if (ret == 0) {
            fprintf(stderr, "cr_poll_checkpoint returned unexpected 0\n");
	    exit(1);
        }
    } while (ret < 0);
    ret = 0;

    close(cr_args.cr_fd);

out:
    return ret;
}

int main(void) {
    int rc, child;
    struct crut_pipes pipes;
    int self = getpid();
    char *filename = crut_aprintf("context.%d", self);

    setlinebuf(stdout);
    printf("#ST_ALARM:60\n");
    printf("000 test started w/ pid %d\n", self);

    child = crut_pipes_fork(&pipes);
    if (!child) {
	child_main(&pipes);
	exit(1);
    }

    crut_pipes_expect(&pipes, MSG_CHILD_READY);
    printf("001 child started w/ pid %d\n", child);

    /* First basic tests of normal checkpoints */
    rc = checkpoint_to_file(filename, self, 0);
    if (rc == 0) {
	printf("002 basic self checkpoint passed\n");
    } else {
	printf("XXX basic self checkpoint FAILED errno=%d (%s)\n", rc, cr_strerror(rc));
    }
    rc = checkpoint_to_file(filename, child, 0);
    if (rc == 0) {
	printf("003 basic child checkpoint passed\n");
    } else {
	printf("XXX basic child checkpoint FAILED errno=%d (%s)\n", rc, cr_strerror(rc));
    }

    /* Next try w/ CR_CHKPT_PROHIBIT_SELF */
    rc = checkpoint_to_file(filename, self, CR_CHKPT_PROHIBIT_SELF);
    if (rc == EDEADLK) {
	printf("004 PROHIBIT_SELF self checkpoint failed in the expected way\n");
    } else {
	printf("XXX PROHIBIT_SELF self checkpoint did not fail in the expected way, errno=%d (%s)\n", rc, cr_strerror(rc));
    }
    rc = checkpoint_to_file(filename, child, CR_CHKPT_PROHIBIT_SELF);
    if (rc == 0) {
	printf("005 PROHIBIT_SELF child checkpoint passed\n");
    } else {
	printf("XXX PROHIBIT_SELF child checkpoint FAILED errno=%d (%s)\n", rc, cr_strerror(rc));
    }

    /* Ask child to register a callback that forwards back to us */
    crut_pipes_putchar(&pipes, MSG_REGISTER_REQUEST);
    crut_pipes_expect(&pipes, MSG_REGISTER_REPLY);
    printf("006 child has registered forwarding callback\n");

    /* Now try w/ and w/o CR_CHKPT_PROHIBIT_SELF */
    rc = checkpoint_to_file(filename, child, 0);
    if (rc == 0) {
	printf("007 child+fwd checkpoint passed\n");
    } else {
	printf("XXX child+fwd checkpoint FAILED errno=%d (%s)\n", rc, cr_strerror(rc));
    }
    crut_pipes_putchar(&pipes, MSG_RESULT_QUERY);
    rc = crut_pipes_getchar(&pipes);
    if (rc == 0) {
	printf("008 child+fwd cr_forward_checkpoint(parent) passed\n");
    } else {
	printf("XXX child+fwd cr_forward_checkpoint(parent) FAILED errno=%d (mod 128)\n", rc);
    }
    rc = checkpoint_to_file(filename, child, CR_CHKPT_PROHIBIT_SELF);
    if (rc == 0) {
	printf("009 PROHIBIT_SELF child+fwd checkpoint passed\n");
    } else {
	printf("XXX PROHIBIT_SELF child+fwd checkpoint FAILED errno=%d (%s)\n", rc, cr_strerror(rc));
    }
    crut_pipes_putchar(&pipes, MSG_RESULT_QUERY);
    rc = crut_pipes_getchar(&pipes);
    if ((char)rc == (char)EDEADLK) {
	printf("010 PROHIBIT_SELF child+fwd cr_forward_checkpoint(parent) failed in the expected way\n");
    } else {
	printf("XXX child+fwd cr_forward_checkpoint(parent) did not fail in the expected way, errno=%d (mod 128)\n", rc);
    }

    crut_pipes_putchar(&pipes, MSG_PARENT_DONE);
    crut_waitpid_expect(pipes.child, MSG_PARENT_DONE);

    printf("011 DONE\n");
    (void)unlink(filename);

    return 0;
}
