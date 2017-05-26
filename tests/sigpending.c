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
 * $Id: sigpending.c,v 1.3 2008/08/29 21:36:53 phargrov Exp $
 *
 * Check for possible loss of pending signals across a restart.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>

#include "crut.h"

#define MY_SIGNAL SIGQUIT

volatile int my_flag = -1;
volatile int my_pid = -1;

static int
common_setup(void **testdata, int flags, void *handler)
{
    struct sigaction sa;
    sigset_t mask, old_mask;

    my_pid = getpid();

    sigemptyset(&mask);
    sigaddset(&mask, MY_SIGNAL);
    sigprocmask(SIG_BLOCK, &mask, &old_mask);

    if (flags & SA_SIGINFO) {
	sa.sa_handler = handler;
    } else {
	sa.sa_sigaction = handler;
    }
    sa.sa_flags = flags;
    (void)sigaction(MY_SIGNAL, &sa, NULL); /* Ignore any failure */

    *testdata = (void *)(unsigned long)flags;
    return 0;
}

static void catcher_raise(int signo) {
    CRUT_DEBUG("In catcher");
    if ((my_flag != 0) || (signo != MY_SIGNAL)) {
	CRUT_FAIL("Catcher ran unexpectedly (sig=%d flag=%d)", signo, my_flag);
	exit(-1);
    }
    my_flag = 1;
    return;
}

static int
raise_setup(void **testdata)
{
    int result = common_setup(testdata, 0, catcher_raise);
    raise(MY_SIGNAL);
    return result;
}

static void catcher_sigqueue(int signo, siginfo_t *siginfo, void *context) {
    CRUT_DEBUG("In catcher");
    if ((my_flag != 0) || (signo != MY_SIGNAL)) {
	CRUT_FAIL("Catcher ran unexpectedly (sig=%d flag=%d)", signo, my_flag);
	exit(-1);
    }
    if (siginfo->si_value.sival_int != my_pid) {
	CRUT_FAIL("Failed to preserve queued siginfo (%d != %d)",
			siginfo->si_value.sival_int, my_pid);
	exit(-1);
    }
    my_flag = 1;
    return;
}

static int
sigqueue_setup(void **testdata)
{
    int result = common_setup(testdata, SA_SIGINFO, catcher_sigqueue);
    union sigval v;
    int rc;

    v.sival_int = my_pid;
    rc = sigqueue(my_pid, MY_SIGNAL, v);
    if (rc < 0) {
        CRUT_FAIL("sigqueue failed w/ errno=%d", errno);
	result = rc;
    }

    return result;
}


static int
check_it(void *p, int unblock)
{
    sigset_t mask, old_mask;
    //int flags = (int)(unsigned long)p;

    /* Check that signal actually *is* blocked (as opposed to ignored) */
    CRUT_DEBUG("Checking signal mask");
    sigemptyset(&mask);
    sigprocmask(SIG_BLOCK, &mask, &old_mask);
    if (sigismember(&old_mask,MY_SIGNAL) == 0) {
        CRUT_FAIL("Failed to mask the signal");
        return -1;
    }

    /* Check that signal is (still) pending */
    CRUT_DEBUG("Checking pending signals");
    sigemptyset(&mask);
    sigpending(&mask);
    if (sigismember(&mask,MY_SIGNAL) == 0) {
	CRUT_FAIL("Failed to preserve the pending signal");
	return -1;
    }

    /* Check on delivery */
    if (unblock) {
        if (my_flag == 1) {
	    CRUT_FAIL("Signal handler ran early");
	    return -1;
        }
	my_flag = 0;
	sched_yield();
	CRUT_DEBUG("Unblock signal to test");
	sigemptyset(&mask);
	sigaddset(&mask, MY_SIGNAL);
	sigprocmask(SIG_UNBLOCK, &mask, &old_mask);
	sched_yield();
        if (my_flag != 1) {
            CRUT_FAIL("Signal handler failed or failed to run");
            return -1;
        }
	my_flag = -1;
    }

    return 0;
}

static int
common_precheckpoint(void *p)
{
    CRUT_DEBUG("Testing sanity before we checkpoint");
    return check_it(p, 0);
}

static int
common_continue(void *p)
{
    CRUT_DEBUG("Continuing after checkpoint.");
    return check_it(p, 1);
}

static int
common_restart(void *p)
{
    CRUT_DEBUG("Restarting from checkpoint.");
    return check_it(p, 1);
}

int
main(int argc, char *argv[])
{
    int ret;
    struct crut_operations test1_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"raise",
        test_description:"Ensure a raised signal is preserved across restart.",
	test_setup:raise_setup,
	test_precheckpoint:common_precheckpoint,
	test_continue:common_continue,
	test_restart:common_restart,
	test_teardown:NULL,
    };

    struct crut_operations test2_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"sigqueue",
        test_description:"Ensure a queued signal is preserved across restart.",
	test_setup:sigqueue_setup,
	test_precheckpoint:common_precheckpoint,
	test_continue:common_continue,
	test_restart:common_restart,
	test_teardown:NULL,
    };

    /* add the tests */
    crut_add_test(&test1_ops);
    crut_add_test(&test2_ops);

    ret = crut_main(argc, argv);

    return ret;
}
