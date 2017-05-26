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
 * $Id: hooks.c,v 1.6 2008/11/30 04:36:56 phargrov Exp $
 *
 * Simple example for using crut
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include "libcr.h"
#include "crut.h"
#include "crut_util.h"

volatile int my_pid;

static int my_barrier_before;
static int my_barrier_after;

typedef enum {
    TEST_WHEN_CONT,
    TEST_WHEN_RSTRT
} test_when_t;

struct testdata {
  cr_hook_event_t	event;
  test_when_t		when;
  pthread_t		thread;
};

static void *
do_thread_stuff(void *p)
{
    int *thread_ret;

    my_pid = getpid();

    thread_ret = malloc(sizeof(int));
    if (thread_ret == NULL) {
        goto out_nomem;
    }

    CRUT_DEBUG("thread_ret = %p", thread_ret);

    crut_barrier(&my_barrier_before);
    crut_wait(CRUT_EVENT_CONTINUE); /* Also wakes on _RESTART */
    crut_barrier(&my_barrier_after);

    *thread_ret = (uintptr_t)p;

    return thread_ret;

out_nomem:
    return NULL;
}

static int my_callback(void *arg)
{
    return 0;
}

static pthread_mutex_t my_lock = PTHREAD_MUTEX_INITIALIZER;
static int my_event = -1;

static void my_hook(cr_hook_event_t event)
{
    cr_hook_event_t old;

    if ((getpid() != my_pid) &&
        ((event == CR_HOOK_CONT_NO_CALLBACKS) ||
	 (event == CR_HOOK_RSTRT_NO_CALLBACKS))) {
	 CRUT_DEBUG("Ignoring event from \"extra\" thread (e.g. LinuxTheads manager)");
	 return;
    }

    pthread_mutex_lock(&my_lock);
    old = my_event;
    my_event = event;
    pthread_mutex_unlock(&my_lock);

    if (old != -1) {
        CRUT_FAIL("More than one event seen");
	exit(1);
    }
}

static int check_event(cr_hook_event_t want)
{
    cr_hook_event_t got;
    int retval = 0;

    pthread_mutex_lock(&my_lock);
    got = my_event;
    pthread_mutex_unlock(&my_lock);

    if (got != want) {
        CRUT_FAIL("Expected my_event==%d, but got %d", want, got);
	retval = -1;
    } else {
        CRUT_DEBUG("Got expected my_event==%d", want);
    }

    return retval;
}

static struct testdata *
hooks_setup_common(cr_hook_event_t event, test_when_t when, int threads)
{
    struct testdata *td;

    td = malloc(sizeof(*td));
    if (td) {
	cr_register_hook(event, my_hook);
	td->event = event;
	td->when = when;
    }
    if (threads) {
	my_barrier_before = 1+threads;
    }
    my_barrier_after = 1+threads;
    my_pid = getpid();
    return td;
}

static int
hooks_setup_nocb(void **testdata, cr_hook_event_t event, test_when_t when)
{
    struct testdata *td;
    int retval;
    pthread_t *test_thread = malloc(sizeof(pthread_t));

    td = hooks_setup_common(event, when, 1);
    if (!td) {
	retval = -1;
	goto out_nomem;
    }

    retval = crut_pthread_create(&td->thread, NULL, do_thread_stuff, NULL);
    if (retval) {
        perror("pthread_create"); 
	retval = -1;
	goto out_nothread;
    }
    crut_barrier(&my_barrier_before);

    *testdata = td;
    return 0;

    if (pthread_cancel(*test_thread)) {
        perror("pthread_cancel");
    }
out_nothread:
    free(test_thread);
out_nomem:
    *testdata = NULL;
    return retval;
}

static int
hooks_teardown_nocb(void *p)
{
    struct testdata *td = p;
    int retval;
    int join_ret;
    void *pthread_ret;
    int *thread_ret;

    retval = 0;
    join_ret = pthread_join(td->thread, &pthread_ret);
    if (join_ret) {
	/* what else can we do? */
        perror("pthread_join");
	retval = join_ret;
    }

    thread_ret = (int *) pthread_ret;

    /* check return value from thread */
    if (thread_ret != NULL) {
        CRUT_DEBUG("Thread return *(%p)=%d.", thread_ret, *thread_ret);
	if (*thread_ret < 0) {
            CRUT_FAIL("Thread failed (return %d)", *thread_ret);
	}
	free(pthread_ret);
    } else {
        CRUT_FAIL("Thread failed (returned NULL)");
    }

    free(p);

    return retval;
}

static int
hooks_setup_cont_nocb(void **testdata)
{
    return hooks_setup_nocb(testdata, CR_HOOK_CONT_NO_CALLBACKS, TEST_WHEN_CONT);
}

static int
hooks_setup_rstrt_nocb(void **testdata)
{
    return hooks_setup_nocb(testdata, CR_HOOK_RSTRT_NO_CALLBACKS, TEST_WHEN_RSTRT);
}

static int
hooks_setup_cont_sig(void **testdata)
{
    *testdata = hooks_setup_common(CR_HOOK_CONT_SIGNAL_CONTEXT, TEST_WHEN_CONT, 0);
    return testdata ? 0 : -1;
}

static int
hooks_setup_rstrt_sig(void **testdata)
{
    *testdata = hooks_setup_common(CR_HOOK_RSTRT_SIGNAL_CONTEXT, TEST_WHEN_RSTRT, 0);
    return testdata ? 0 : -1;
}

static int
hooks_setup_cont_thr(void **testdata)
{
    cr_register_callback(my_callback, NULL, CR_THREAD_CONTEXT);
    *testdata = hooks_setup_common(CR_HOOK_CONT_THREAD_CONTEXT, TEST_WHEN_CONT, 0);
    return testdata ? 0 : -1;
}

static int
hooks_setup_rstrt_thr(void **testdata)
{
    cr_register_callback(my_callback, NULL, CR_THREAD_CONTEXT);
    *testdata = hooks_setup_common(CR_HOOK_RSTRT_THREAD_CONTEXT, TEST_WHEN_RSTRT, 0);
    return testdata ? 0 : -1;
}

static int
hooks_precheckpoint(void *p)
{
    CRUT_DEBUG("Preparing to checkpoint.");

    return check_event(-1);
}

static int
hooks_continue(void *p)
{
    struct testdata *td = p;

    CRUT_DEBUG("Continuing after checkpoint.");
    crut_barrier(&my_barrier_after);

    return check_event((td->when == TEST_WHEN_CONT) ? td->event : -1);
}

static int
hooks_restart(void *p)
{
    struct testdata *td = p;

    CRUT_DEBUG("Restarting from checkpoint.");
    crut_barrier(&my_barrier_after);

    return check_event((td->when == TEST_WHEN_RSTRT) ? td->event : -1);
}


int
main(int argc, char *argv[])
{
    int ret;

    struct crut_operations cont_nocb_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"hooks_cont_nocb",
        test_description:"Simple test of cr_regsiter_hook(CR_HOOK_CONT_NO_CALLBACKS).",
	test_setup:hooks_setup_cont_nocb,
	test_precheckpoint:hooks_precheckpoint,
	test_continue:hooks_continue,
	test_restart:hooks_restart,
	test_teardown:hooks_teardown_nocb,
    };

    struct crut_operations rstrt_nocb_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"hooks_rstrt_nocb",
        test_description:"Simple test of cr_regsiter_hook(CR_HOOK_RSTRT_NO_CALLBACKS).",
	test_setup:hooks_setup_rstrt_nocb,
	test_precheckpoint:hooks_precheckpoint,
	test_continue:hooks_continue,
	test_restart:hooks_restart,
	test_teardown:hooks_teardown_nocb,
    };

    struct crut_operations cont_sig_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"hooks_cont_sig",
        test_description:"Simple test of cr_regsiter_hook(CR_HOOK_CONT_SIGNAL_CONTEXT).",
	test_setup:hooks_setup_cont_sig,
	test_precheckpoint:hooks_precheckpoint,
	test_continue:hooks_continue,
	test_restart:hooks_restart,
	test_teardown:NULL,
    };

    struct crut_operations rstrt_sig_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"hooks_rstrt_sig",
        test_description:"Simple test of cr_regsiter_hook(CR_HOOK_RSTRT_SIGNAL_CONTEXT).",
	test_setup:hooks_setup_rstrt_sig,
	test_precheckpoint:hooks_precheckpoint,
	test_continue:hooks_continue,
	test_restart:hooks_restart,
	test_teardown:NULL,
    };

    struct crut_operations cont_thr_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"hooks_cont_thr",
        test_description:"Simple test of cr_regsiter_hook(CR_HOOK_CONT_THREAD_CONTEXT).",
	test_setup:hooks_setup_cont_thr,
	test_precheckpoint:hooks_precheckpoint,
	test_continue:hooks_continue,
	test_restart:hooks_restart,
	test_teardown:NULL,
    };

    struct crut_operations rstrt_thr_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"hooks_rstrt_thr",
        test_description:"Simple test of cr_regsiter_hook(CR_HOOK_RSTRT_THREAD_CONTEXT).",
	test_setup:hooks_setup_rstrt_thr,
	test_precheckpoint:hooks_precheckpoint,
	test_continue:hooks_continue,
	test_restart:hooks_restart,
	test_teardown:NULL,
    };

    /* add the tests */
    crut_add_test(&cont_sig_test_ops);
    crut_add_test(&cont_thr_test_ops);
    crut_add_test(&cont_nocb_test_ops);
    crut_add_test(&rstrt_sig_test_ops);
    crut_add_test(&rstrt_thr_test_ops);
    crut_add_test(&rstrt_nocb_test_ops);

    ret = crut_main(argc, argv);

    return ret;
}
