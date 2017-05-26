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
 * $Id: child.c,v 1.8 2008/11/30 04:36:56 phargrov Exp $
 *
 * Test of pipes w/ child and grandchild captured via CR_SCOPE_TREE
 *  and of parent w/ a zombie child
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "crut.h"
#include "crut_util.h"

enum {
	MSG_CHILD_GOOD = 18,
	MSG_CHILD_BAD,
	MSG_PARENT_READY,
	MSG_PARENT_DONE,
	MSG_CHILD_DONE
};


static void sigpipe(int signo) {
    fprintf(stderr, "%d exiting on SIGPIPE\n", getpid());
    exit(-1);
}

static void *thread_main(void *arg) 
{
    while(1) { pthread_testcancel(); sleep(1); }
    return NULL;
}

static int
child_setup(void **p)
{
    struct crut_pipes *pipes = malloc(sizeof(struct crut_pipes));
    int retval;

    signal(SIGPIPE, &sigpipe);	

    CRUT_DEBUG("Creating pipes and forking.");
    retval = crut_pipes_fork(pipes);
    if (!retval) {
	/* In the child */
	int parent = pipes->child;
	int child = fork();
        if (child < 0) {
	    CRUT_FAIL("Error forking grandchild.");
	} else if (!child) {
	    /* In the grandchild */
	    pthread_t th;
	    if (crut_pthread_create(&th, NULL, &thread_main, NULL) != 0) {
		exit(1);
	    }
	    while (1) {
	        CRUT_DEBUG("Grandchild entering read()");
	        crut_pipes_expect(pipes, MSG_PARENT_READY);
	        CRUT_DEBUG("Grandchild sees ppid %d", getppid());
	        crut_pipes_putchar(pipes, getppid() == parent ? MSG_CHILD_GOOD : MSG_CHILD_BAD);
	    }
	    exit(-1); // unreached
	} else {
            crut_waitpid_expect(child, MSG_PARENT_DONE);
            exit(MSG_CHILD_DONE);
	}
	exit(-1); // unreached
    }

    crut_pipes_putchar(pipes, MSG_PARENT_READY);
    crut_pipes_expect(pipes, MSG_CHILD_GOOD);

    *p = pipes;

    return retval;
}

static int
child_precheckpoint(void *p)
{
    int retval = 0;

    CRUT_DEBUG("Getting ready to checkpoint");

    crut_pipes_putchar(p, MSG_PARENT_READY);
    crut_pipes_expect(p, MSG_CHILD_GOOD);

    return retval;
}

static int
child_continue(void *p)
{
    struct crut_pipes *pipes = p;
    int retval = 0;

    CRUT_DEBUG("Continuing after checkpoint.");

    crut_pipes_putchar(pipes, MSG_PARENT_READY);
    crut_pipes_expect(pipes, MSG_CHILD_GOOD);
    crut_pipes_putchar(pipes, MSG_PARENT_DONE);
    crut_waitpid_expect(pipes->child, MSG_CHILD_DONE);

    return retval;
}

static int
child_restart(void *p)
{
    int retval = 0;

    CRUT_DEBUG("Restarting from checkpoint.");

    crut_pipes_putchar(p, MSG_PARENT_READY);
    crut_pipes_expect(p, MSG_CHILD_GOOD);

    return retval;
}

static int
child_teardown(void *p)
{
    struct crut_pipes *pipes = p;
    int retval = 0;

    CRUT_DEBUG("Entering child_teardown.");

    crut_pipes_putchar(pipes, MSG_PARENT_DONE);
    crut_waitpid_expect(pipes->child, MSG_CHILD_DONE);

    return retval;
}

static int
zombie_setup(void **p) {
    struct crut_pipes *pipes = malloc(sizeof(struct crut_pipes));
    int retval;

    retval = crut_pipes_fork(pipes);
    if (!retval) {
	/* Am child */
	exit(0);
	return -1;
    } else {
	int rc = crut_pipes_getchar(pipes);
	if (rc != -1) {
	    CRUT_FAIL("Read returned %d when expected EOF", rc);
	    return -1;
	}
	rc = crut_pipes_close(pipes);
	if (rc < 0) {
	    CRUT_FAIL("Close failed");
	    return -1;
	}
    }

    *p = pipes;

    return 0;
}

static int zombie_teardown(void *p) {
    return 0;
}

static int zombie_precheckpoint(void *p) {
    return 0;
}

static int zombie_continue(void *p) {
    struct crut_pipes *pipes = p;

    crut_waitpid_expect(pipes->child, 0);

    return 0;
}

static int zombie_restart(void *p) {
    return 0;
}

int
main(int argc, char *argv[])
{
    int ret;
    struct crut_operations child_test_ops = {
	test_scope:CR_SCOPE_TREE,
	test_name:"child",
        test_description:"Child tester.  Tests checkpoint of children via CR_SCOPE_TREE.",
	test_setup:child_setup,
	test_precheckpoint:child_precheckpoint,
	test_continue:child_continue,
	test_restart:child_restart,
	test_teardown:child_teardown
    };
    struct crut_operations zombie_test_ops = {
	test_scope:CR_SCOPE_TREE,
	test_name:"zombie",
        test_description:"Zombie child tester.  Tests checkpoint with dead/unreaped children via CR_SCOPE_TREE.",
	test_setup:zombie_setup,
	test_precheckpoint:zombie_precheckpoint,
	test_continue:zombie_continue,
	test_restart:zombie_restart,
	test_teardown:zombie_teardown
    };

    /* add the tests */
    crut_add_test(&child_test_ops);
    crut_add_test(&zombie_test_ops);

    ret = crut_main(argc, argv);

    return ret;
}
