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
 * $Id: pthread_counting.c,v 1.5 2007/05/07 19:41:46 phargrov Exp $
 */


#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <inttypes.h>

#define THREADS 3

static int limit = 120; /* default */
static int count = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static void delay(void)
{
    struct timespec ts = {tv_sec: 1, tv_nsec: 0};
    int rc;

    do {
	rc = nanosleep(&ts, &ts);
    } while ((rc <0) && (errno == EINTR) && (ts.tv_sec || ts.tv_nsec));
}

static void advance(void)
{
    pthread_mutex_lock(&lock);
    count++;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&lock);
}

static void wait_for(int i)
{
    pthread_mutex_lock(&lock);
    while ((count % THREADS) != i) {
	pthread_cond_wait(&cond, &lock);
    }
    pthread_mutex_unlock(&lock);
}

static void * thread(void * arg)
{
    int id = (int)(uintptr_t)arg;
    int local;

    do {
	wait_for(id);
        local = count;
	printf("Thread %d says count = %d\n", id, local);
	delay();
	advance();
    } while (local < limit);

    return NULL;  /* Not reached */
}

int main(int argc, char *argv[])
{
    pthread_t th[THREADS];
    int i;
	
    printf("Starting main process w/ pid %d\n", (int)getpid());

    if (argc > 1) {
	limit = (i = atoi(argv[1])) > 0 ? i : limit;
    }

    // Start one or more threads
    for (i = 1; i < THREADS; ++i) {
	pthread_create(&th[i], NULL, thread, (void *)(uintptr_t)i);
    }
    thread((void *)0);

    return 0;
}
