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
 * $Id: crut_util_pth.c,v 1.2 2008/11/30 04:29:10 phargrov Exp $
 *
 * Utility functions for BLCR tests - thread portion
 */

#include <sys/types.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>

#include "blcr_config.h"
#include "crut_util.h"

/*
 * Wait for other threads by watching a counter
 */
void crut_barrier(int *counter)
{
    static pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t my_cond = PTHREAD_COND_INITIALIZER;

    pthread_mutex_lock(&my_mutex);
    --(*counter);
    if (*counter <= 0) {
        if (*counter < 0) {
	    CRUT_FAIL("Barrier underflow");
	}
	pthread_cond_broadcast(&my_cond);
    } else {
        while (*counter > 0) {
	    pthread_cond_wait(&my_cond, &my_mutex);
        }
    }
    pthread_mutex_unlock(&my_mutex);
}

/*
 * Create a pthread w/ a small stack (avoiding issues like shown in bug 2232)
 */
int crut_pthread_create(pthread_t *thread, pthread_attr_t *attr,
                        void *(*start_routine)(void *), void *arg)
{
#if HAVE_PTHREAD_ATTR_SETSTACKSIZE
    pthread_attr_t my_attr, *attr_p;
    size_t size;
    int rc;

    if (attr) {
	attr_p = attr;
    } else {
        rc = pthread_attr_init(&my_attr);
        if (rc != 0) {
	    CRUT_FAIL("Error calling pthread_attr_init()");
        }
	attr_p = &my_attr;
    }

    /* MAX(4MB, PTHREAD_STACK_MIN) */
    size = 4 * 1024 * 1024;
    if (size < PTHREAD_STACK_MIN) size = PTHREAD_STACK_MIN;
    rc = pthread_attr_setstacksize(attr_p, size);
    if (rc != 0) {
	CRUT_FAIL("Error calling pthread_attr_setstacksize()");
    }

    rc = pthread_create(thread, attr_p, start_routine, arg);

    if (!attr) {
	(void)pthread_attr_destroy(&my_attr);
    }

    return rc;
#else
    return pthread_create(thread, attr, start_routine, arg);
#endif
}

static void *crut_is_linuxthreads_aux(void *arg) {
   return ((int)getpid() == *(int *)arg) ? NULL : (void *)1UL;
}

int crut_is_linuxthreads(void) {
    int mypid = (int)getpid();
    pthread_t th;
    void *join_val;

    if (0 != crut_pthread_create(&th, NULL, &crut_is_linuxthreads_aux, (void *)(&mypid))) {
	CRUT_FAIL("Error calling pthread_create()");
	exit(-1);
    }
    if (0 != pthread_join(th, &join_val)) {
	CRUT_FAIL("Error calling pthread_join()");
	exit(-1);
    }
    return (join_val != NULL);
}
