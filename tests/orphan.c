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
 * $Id: orphan.c,v 1.5 2008/08/29 21:36:53 phargrov Exp $
 *
 * Test of pipes w/ orphaned grandchild (a child of init).
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "crut.h"

enum {
	MSG_PARENT_READY = 1,
	MSG_ORPHAN_GOOD,
	MSG_ORPHAN_BAD,
	MSG_CHILD_DONE
};

static void sigpipe(int signo) {
    fprintf(stderr, "%d exiting on SIGPIPE\n", getpid());
    exit(-1);
}

static int
orphan_setup(void **p)
{
    struct crut_pipes *pipes = malloc(sizeof(*pipes));
    int retval;

    signal(SIGPIPE, &sigpipe);	
    
    retval = crut_pipes_fork(pipes);
    if (!retval) {
	/* In the child */
        int pid = fork();
	if (pid < 0) {
	    exit (-1);
	} else if (!pid) {
	    /* In the grandchild */
	    while (1) {
	        CRUT_DEBUG("Orphan entering read()");
	        crut_pipes_expect(pipes, MSG_PARENT_READY);
	        CRUT_DEBUG("Orphan sees ppid %d", getppid());
		crut_pipes_putchar(pipes, (getppid() == 1) ? MSG_ORPHAN_GOOD : MSG_ORPHAN_BAD);
	    }
	    exit(-1); // unreached
	} else {
	    CRUT_DEBUG("Forked grandchild %d", pid);
	}
	exit(MSG_CHILD_DONE);
    }
    crut_waitpid_expect(pipes->child, MSG_CHILD_DONE);

    crut_pipes_putchar(pipes, MSG_PARENT_READY);
    crut_pipes_expect(pipes, MSG_ORPHAN_GOOD);

    *p = pipes;

    return retval;
}

static int
orphan_precheckpoint(void *p)
{
    struct crut_pipes *pipes = p;
    int retval = 0;

    CRUT_DEBUG("Getting ready to checkpoint");

    crut_pipes_putchar(pipes, MSG_PARENT_READY);
    crut_pipes_expect(pipes, MSG_ORPHAN_GOOD);

    return retval;
}

static int
orphan_continue(void *p)
{
    struct crut_pipes *pipes = p;
    int retval = 0;

    CRUT_DEBUG("Continuing after checkpoint.");

    crut_pipes_putchar(pipes, MSG_PARENT_READY);
    crut_pipes_expect(pipes, MSG_ORPHAN_GOOD);

    return retval;
}

static int
orphan_restart(void *p)
{
    struct crut_pipes *pipes = p;
    int retval = 0;

    CRUT_DEBUG("Restarting from checkpoint.");

    crut_pipes_putchar(pipes, MSG_PARENT_READY);
    crut_pipes_expect(pipes, MSG_ORPHAN_GOOD);

    return retval;
}

int
main(int argc, char *argv[])
{
    int ret;
    struct crut_operations orphan_test_ops = {
	test_scope:CR_SCOPE_PGRP,
	test_name:"orphan",
        test_description:"Orphan tester.  Tests BLCR recovery of children of init.",
	test_setup:orphan_setup,
	test_precheckpoint:orphan_precheckpoint,
	test_continue:orphan_continue,
	test_restart:orphan_restart,
	test_teardown:NULL,
    };

    /* add the test */
    crut_add_test(&orphan_test_ops);

    ret = crut_main(argc, argv);

    return ret;
}
