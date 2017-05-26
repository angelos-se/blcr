/*
 * Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
 * 2008, The Regents of the University of California, through Lawrence
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
 * $Id: atomics_stress.c,v 1.17 2008/12/15 04:54:52 phargrov Exp $
 *
 * Simple serial tests of the atomic operations used inside libcr
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>

#include "blcr_config.h"
#include "cr_atomic.h"
#include "crut_util.h"

#define CHECK(cond) \
  do { if (!(cond)) { ++failed; printf("Test (" #cond ") FAILED at line %d\n", __LINE__); } } while (0)

static int failed = 0;
static volatile char in_signal = 'N';
static sigset_t alarm_mask, vtalarm_mask;
static unsigned long iters = 2;
static int nthreads = 4;
static uint64_t duration = 10000000; /* 10s expressed in us */
static cri_atomic_t X = 0;
static int child = 0;
static const struct itimerval vtalarm_timer = {
 	it_interval:        { tv_sec: 0, tv_usec: 0},
	it_value:           { tv_sec: 0, tv_usec: 1}, /* As fast as possible */
};
static const struct itimerval alarm_timer = {
 	it_interval:        { tv_sec: 0, tv_usec: 100000},
	it_value:           { tv_sec: 0, tv_usec: 100000}, /* 10Hz */
};
static const struct itimerval zero_timer = {
 	it_interval:        { tv_sec: 0, tv_usec: 0},
	it_value:           { tv_sec: 0, tv_usec: 0},
};

/* (potentially) microsecond resolution timer */
static uint64_t
tick(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((uint64_t)tv.tv_sec) * 1000000 + (uint64_t)tv.tv_usec;
}

/* SIGCHLD is fatal for us, as it indicates a timeout */
static void
grim(int signo) { /* a reaper */
    int rc;
    while ((rc = waitpid(child, NULL, WNOHANG)) != 0) {
	if (rc == child) {
	    static char msg[] = " TIMEOUT (in_signal=?)\n";
	    msg[20] = in_signal;
	    write(STDERR_FILENO, msg, sizeof(msg) - 1);
	    _exit(1);
	} else if (rc < 0) {
	    perror("waitpid()"); /* Not strictly safe in a signal handler */
	}
    }
}

/*
 * Print progress
 */
static uint64_t progress = 1;
static void
alarmist(int signo) {
    /* Show life occasionally, but stop when 100% is reached */
    progress = progress && crut_progress_step(tick());
}

/*
 * We perform an atomic no-op to test signal safety.
 */
static void
vtalarmist(int signo) {
    unsigned int tmp;

    in_signal = 'Y';
    do {
	tmp = cri_atomic_read(&X);
    } while (!cri_cmp_swap(&X, tmp, tmp));
    in_signal = 'N';

    (void)setitimer(ITIMER_VIRTUAL, &vtalarm_timer, NULL);
}

/* Each thread increments *arg ITERS times */
static void
*test_inc(void) {
    unsigned long i;

    for (i = 0; i < iters; ++i) {
	cri_atomic_inc(&X);
    }

    return NULL;
}

/* Each thread decrements *arg ITERS times, returns final result */
static void
*test_dec(void) {
    unsigned long i;
    int result = 0;

    for (i = 0; i < iters; ++i) {
	result = cri_atomic_dec_and_test(&X);
    }

    return (void *)(uintptr_t)result;
}

static void *wrap(void *arg)
{
    void *retval;

    pthread_sigmask(SIG_UNBLOCK, &vtalarm_mask, NULL);
    (void)setitimer(ITIMER_VIRTUAL, &vtalarm_timer, NULL);

    retval = ((void *(*)(void))arg)();

    pthread_sigmask(SIG_BLOCK, &vtalarm_mask, NULL);
    (void)setitimer(ITIMER_VIRTUAL, &zero_timer, NULL);

    return retval;
}

static void
spawn_and_join(pthread_t *threads, void **join_vals, void * (*fn)(void))
{
    int i;

    pthread_sigmask(SIG_UNBLOCK, &alarm_mask, NULL);
    (void)setitimer(ITIMER_REAL, &alarm_timer, NULL);

    for (i = 0; i < nthreads; ++i) {
	int rc = crut_pthread_create(threads + i, NULL, wrap, (void *)fn);
	if (rc) {
	    perror("pthread_create");
	    exit(1);
	}
    }

    for (i = 0; i < nthreads; ++i) {
	int rc = pthread_join(threads[i], join_vals + i);
	if (rc) {
	    perror("pthread_join");
	    exit(1);
	}
    }

    (void)setitimer(ITIMER_REAL, &zero_timer, NULL);
    pthread_sigmask(SIG_BLOCK, &alarm_mask, NULL);
}

/* Repeat until at least 'duration' microseconds of aggregate running time is reached */
static void
run_test(pthread_t *threads, void **join_vals, void * (*fn)(void), int alpha, int beta)
{
    const int64_t epsilon = 250000; /* 0.25s = Smallest trustworthy interval to extrapolate from */
    uint64_t t_final = duration + tick();
    long next = 1000;

    do {
	uint64_t t1, t2;
	int64_t curr, remain;

	iters = (next < 1000000) ? next : 1000000;
	cri_atomic_write(&X, (unsigned int)(iters * alpha));

	t1 = tick();
	spawn_and_join(threads, join_vals, fn);
	t2 = tick();

	if (cri_atomic_read(&X) != (unsigned int)(iters * beta)) {
	  ++failed;
	  printf("Test (cri_atomic_read(&X) == (unsigned int)(iters * %d)) failed\n", beta);
	  break;
	}

	curr = t2 - t1;
	remain = t_final - t2;
	next = (curr < epsilon) ? (next * 2) : (remain * (double)iters / (double)curr);
    } while (next > 0);  /* Stop on either convergence or on overflow */
}

extern int
main(int argc, char *argv[])
{
    struct sigaction sa;
    pthread_t *threads;
    void **join_vals;

    sync(); /* Try to push out I/O to get a "smoother" test run */

    threads = calloc(nthreads, sizeof(pthread_t));
    join_vals = calloc(nthreads, sizeof(void *));
    if (!threads || !join_vals) {
	    fputs("calloc failed\n", stderr);
	    exit(1);
    }

    /* Setup a child to SIGCHLD us after 1 minute, in case we are hung */
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = &grim;
    (void)sigaction(SIGCHLD, &sa, NULL); /* Ignore any failure */
    child = fork();
    if (child < 0) {
	perror("fork()");
	exit(1);
    } else if (!child) {
	/* I am the child - I exist to SIGCHLD my parent after 1 minute has elapsed */
	sleep(60);
	_exit(0);
    }

    /* Setup progress markers */
    fputs("atomics_stress:", stderr);
    crut_progress_start(tick(), 2 * duration, 10);

    /* Setup for a periodic timer to interrupt the tests threads, but not the main thread.
     *
     * We don't use an auto-renewing itimer, because if we get stuck in the
     * signal handler, we don't want any additional signals generated.
     */
    sigemptyset(&vtalarm_mask);
    sigaddset(&vtalarm_mask, SIGVTALRM);
    sigprocmask(SIG_BLOCK, &vtalarm_mask, NULL);
    sa.sa_flags = 0;
    sa.sa_handler = &vtalarmist;
    (void)sigaction(SIGVTALRM, &sa, NULL); /* Ignore any failure */

    /* Setup for a periodic timer to interrupt only the main thread */
    sigemptyset(&alarm_mask);
    sigaddset(&alarm_mask, SIGALRM);
    sigprocmask(SIG_BLOCK, &alarm_mask, NULL);
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = &alarmist;
    (void)sigaction(SIGALRM, &sa, NULL); /* Ignore any failure */

    /* Is the initial value 0? */
    CHECK(cri_atomic_read(&X) == 0);

    /* Increment test */
    run_test(threads, join_vals, &test_inc, 0, nthreads);

    fputc(',', stderr);

    /* Decrement test */
    run_test(threads, join_vals, &test_dec, nthreads, 0);

    /* Exactly one thread should have a non-zero join_val */
    {
	int sum = 0;
	int i;
	for (i = 0; i < nthreads; ++i) {
	    sum += (int)(uintptr_t)join_vals[i];
	}
	CHECK(sum == 1);
    }

    if (progress) write(STDERR_FILENO, "\n", 1);

    return !!failed;
}
