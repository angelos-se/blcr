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
 * $Id: cr_signal.c,v 1.6 2008/08/29 21:36:53 phargrov Exp $
 *
 * Check for interference w/ out signal handler
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

#include "crut.h"

// This test is disabled.
// We don't currently do the right thing.
#define TEST_SIGNAL_BLOCKED 0

#define MSG "Hello world!\n"

struct testdata_t {
	char *msg;
	void *handler;
};

static int
common_setup(void **testdata, void *handler)
{
    struct testdata_t *data;
    struct sigaction sa;

    data = malloc(sizeof(*data));
    if (!data) { return -1; }
    data->msg = strdup(MSG);
    if (!data->msg) { return -1; }

    data->handler = handler;
    sa.sa_handler = handler;
    sa.sa_flags = 0;
    (void)sigaction(CR_SIGNUM, &sa, NULL); /* Ignore any failure */

    *testdata = data;
    return 0;
}

volatile int flag = -1;
static void catcher(int signo) {
    if (flag != 0) {
	CRUT_FAIL("Catcher ran unexpectedly");
	exit(-1);
    }
    flag = 1;
    return;
}

static void empty_handler(int signo) {
    CRUT_DEBUG("Running empty handler");
}

static int
test1_setup(void **testdata) {
    /* Test1: catch the signal */
    return common_setup(testdata, &catcher);
}

static int
test2_setup(void **testdata) {
    /* Test2: ignore the signal */
    return common_setup(testdata, SIG_IGN);
}

static int
test3_setup(void **testdata) {
    /* Test3: reset to default */
    return common_setup(testdata, SIG_DFL);
}

#if TEST_SIGNAL_BLOCKED
static int
test4_setup(void **testdata) {
    /* Test4: block it */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, CR_SIGNUM);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    return common_setup(testdata, empty_handler);
}
#endif

static int
test5_setup(void **testdata) {
    /* Test5: corrupt the sa_restorer!! */
    struct testdata_t *data;
    struct sigaction sa;

    data = malloc(sizeof(*data));
    if (!data) { return -1; }
    data->msg = strdup(MSG);
    if (!data->msg) { return -1; }

    (void)sigaction(CR_SIGNUM, NULL, &sa); /* Ignore any failure */
    data->handler = sa.sa_sigaction;
    if (sa.sa_restorer) {
	sa.sa_restorer = (void *)&abort;
	(void)sigaction(CR_SIGNUM, &sa, NULL); /* Ignore any failure */
    } else {
	/* Don't mess w/ NULL */
    }

    *testdata = data;
    return 0;
}

static int
check_it(struct testdata_t *data)
{
    sigset_t mask, old_mask;
    /* The sigismember() checks that follow avoid false-alarms when glibc thinks
     * that CR_SIGNUM is out of bounds, by thinking -1 is OK.
     */
    if (data->handler == &catcher) { /* TEST1 */
	flag = 0;
	CRUT_DEBUG("Raise signal to test");
	raise(CR_SIGNUM);
	if (flag != 1) {
	    CRUT_FAIL("Signal handler failed to run");
	    return -1;
	}
    } else if (data->handler == SIG_IGN) { /* TEST2 */
	/* Check that signal actually *is* ignored (as opposed to blocked) */
	CRUT_DEBUG("Raise signal to test");
	raise(CR_SIGNUM);
	sigemptyset(&mask);
	sigpending(&mask);
	if (sigismember(&mask,CR_SIGNUM) == 1) {
	    CRUT_FAIL("Failed to ignore the signal");
	    return -1;
	}
    } else if (data->handler == SIG_DFL) { /* TEST3 */
	/* Not safe to raise */
    } else if (data->handler == empty_handler) { /* TEST4 */
	/* Check that signal actually *is* blocked (as opposed to ignored) */
	CRUT_DEBUG("Checking signal mask");
	sigemptyset(&mask);
	sigprocmask(SIG_BLOCK, &mask, &old_mask);
	if (sigismember(&old_mask,CR_SIGNUM) == 0) {
	    CRUT_FAIL("Failed to mask the signal");
	    return -1;
	}
	CRUT_DEBUG("Raise signal to test");
	raise(CR_SIGNUM);
	sigemptyset(&mask);
	sigpending(&mask);
	if (sigismember(&mask,CR_SIGNUM) == 0) {
	    CRUT_FAIL("Failed to block the signal");
	    return -1;
	}
	/* Absorb the pending signal */
	sigemptyset(&mask);
	sigaddset(&mask, CR_SIGNUM);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
	sigprocmask(SIG_BLOCK, &mask, NULL);
    } else { /* TEST5 */
	/* Do nothing here */
    }
    return strncmp(data->msg, MSG, sizeof(MSG)) ? -1 : 0;
}

static int
common_precheckpoint(void *p)
{
    CRUT_DEBUG("Testing sanity before we checkpoint");
    return check_it(p);
}

static int
common_continue(void *p)
{
    CRUT_DEBUG("Continuing after checkpoint.");
    return check_it(p);
}

static int
common_restart(void *p)
{
    CRUT_DEBUG("Restarting from checkpoint.");
    return check_it(p);
}

static int
common_teardown(void *p)
{
    struct testdata_t *data = (struct testdata_t *)p;
    free(data->msg);

    return 0;
}

int
main(int argc, char *argv[])
{
    int ret;
    struct crut_operations test1_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"cr_signal_catch",
        test_description:"Signal catch test.",
	test_setup:test1_setup,
	test_precheckpoint:common_precheckpoint,
	test_continue:common_continue,
	test_restart:common_restart,
	test_teardown:common_teardown,
    };

    struct crut_operations test2_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"cr_signal_ignore",
        test_description:"SIG_IGN test.",
	test_setup:test2_setup,
	test_precheckpoint:common_precheckpoint,
	test_continue:common_continue,
	test_restart:common_restart,
	test_teardown:common_teardown,
    };

    struct crut_operations test3_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"cr_signal_default",
        test_description:"SIG_DFL test.",
	test_setup:test3_setup,
	test_precheckpoint:common_precheckpoint,
	test_continue:common_continue,
	test_restart:common_restart,
	test_teardown:common_teardown,
    };

#if TEST_SIGNAL_BLOCKED
    struct crut_operations test4_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"cr_signal_block",
        test_description:"Signal blocking test.",
	test_setup:test4_setup,
	test_precheckpoint:common_precheckpoint,
	test_continue:common_continue,
	test_restart:common_restart,
	test_teardown:common_teardown,
    };
#endif

    struct crut_operations test5_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"cr_signal_restorer",
        test_description:"Signal restorer test.",
	test_setup:test5_setup,
	test_precheckpoint:common_precheckpoint,
	test_continue:common_continue,
	test_restart:common_restart,
	test_teardown:common_teardown,
    };


    /* add the tests */
    crut_add_test(&test1_ops);
    crut_add_test(&test2_ops);
    crut_add_test(&test3_ops);
#if TEST_SIGNAL_BLOCKED
    crut_add_test(&test4_ops);
#endif
    crut_add_test(&test5_ops);

    ret = crut_main(argc, argv);

    return ret;
}
