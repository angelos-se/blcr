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
 * $Id: dpipe.c,v 1.3 2008/08/29 21:36:53 phargrov Exp $
 *
 * Test of NON-empty dangling (external) pipes
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

enum {
	MSG_CHILD_READY = 18,
	MSG_PARENT_READY,
	MSG_CHILD_DONE
};


static void sigpipe(int signo) {
    fprintf(stderr, "%d exiting on SIGPIPE\n", getpid());
    exit(-1);
}

static int
dpipe_setup(void **p)
{
    const char msg1[4] = { MSG_CHILD_READY,  'a', 'b', 'c' };
    const char msg2[4] = { MSG_PARENT_READY, 'A', 'B', 'C' };
    struct crut_pipes *pipes = malloc(sizeof(struct crut_pipes));
    int retval;

    signal(SIGPIPE, &sigpipe);	

    CRUT_DEBUG("Creating pipes and forking.");
    retval = crut_pipes_fork(pipes);
    if (!retval) {
	/* In the child */
        crut_pipes_expect(pipes, MSG_PARENT_READY);
        crut_pipes_write(pipes, msg1, sizeof(msg1));
	exit(MSG_CHILD_DONE);
    }

    crut_pipes_write(pipes, msg2, sizeof(msg2));
    crut_pipes_expect(pipes, MSG_CHILD_READY);
    crut_waitpid_expect(pipes->child, MSG_CHILD_DONE);

    *p = pipes;

    return retval;
}

static int
dpipe_precheckpoint(void *p)
{
    CRUT_DEBUG("Getting ready to checkpoint");
    return 0;
}

static int
dpipe_continue(void *p)
{
    CRUT_DEBUG("Continuing after checkpoint.");
    return 0;
}

static int
dpipe_restart(void *p)
{
    CRUT_DEBUG("Restarting from checkpoint.");
    return 0;
}

int
main(int argc, char *argv[])
{
    int ret;
    struct crut_operations dpipe_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"dpipe",
        test_description:"Tests checkpoint of non-empty dangling (external) pipes.",
	test_setup:dpipe_setup,
	test_precheckpoint:dpipe_precheckpoint,
	test_continue:dpipe_continue,
	test_restart:dpipe_restart,
    };

    /* add the tests */
    crut_add_test(&dpipe_test_ops);

    ret = crut_main(argc, argv);

    return ret;
}
