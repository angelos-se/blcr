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
 * $Id: pthread_misc.c,v 1.13 2008/08/27 21:11:42 phargrov Exp $
 */

#define _LARGEFILE64_SOURCE 1   /* For O_LARGEFILE */

#include <time.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "libcr.h"


/* Take a blocking checkpoint of my own process */
int checkpoint_self(void)
{
    int ret;
    cr_checkpoint_args_t my_args;
    cr_checkpoint_handle_t my_handle;
    char filename[64];

    /* build the filename */
    snprintf(filename, sizeof(filename), "context.%d", (int)getpid());

    /* remove existing context file, if any */
    (void)unlink(filename);

    /* open the context file */
    ret = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE, 0600);
    if (ret < 0) {
        perror("open");
        goto out;
    }

    cr_initialize_checkpoint_args_t(&my_args);
    my_args.cr_fd = ret; /* still holds the return from open() */
    my_args.cr_scope = CR_SCOPE_PROC;

    /* issue the request */
    ret = cr_request_checkpoint(&my_args, &my_handle);
    if (ret < 0) {
        perror("cr_request_checkpoint");
        goto out;
    }

    /* wait for the request to complete */
    do {
        ret = cr_poll_checkpoint(&my_handle, NULL);
        if (ret < 0) {
            if ((ret == CR_POLL_CHKPT_ERR_POST) && (errno == CR_ERESTARTED)) {
                /* restarting -- not an error */
                ret = 1; /* Signal RESTART to caller */
            } else if (errno == EINTR) {
                /* poll was interrupted by a signal -- retry */
            } else {
                perror("cr_poll_checkpoint");
                goto out;
            }
        } else if (ret == 0) {
            fprintf(stderr, "cr_poll_checkpoint returned unexpected 0\n");
            ret = -1;
            goto out;
        } else {
            ret = 0; /* Signal CONTINUE to caller */
	}
    } while (ret < 0);

    close(my_args.cr_fd);
out:
    return ret;
}

#define THREADS 2

static int count = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static pthread_t main_tid = (pthread_t)-1;

#define DATELEN 80
static char date[DATELEN];

static char * now(void)
{
    struct timeb when;

    ftime(&when);
    return ctime(&when.time);
}

static int my_callback(void* arg)
{
    int * rc_ptr = (int *)arg;
    int rc;

    if (pthread_self() == main_tid) {
	strncpy(date, now(), DATELEN);
    }

    rc = cr_checkpoint(0);

    if (rc_ptr != NULL) {
	*rc_ptr = rc;
    }

    return 0;
}

static void advance(void)
{
    pthread_mutex_lock(&lock);
    count++;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&lock);
}

static void wait_for(int want)
{
    pthread_mutex_lock(&lock);
    while (count < want) {
	pthread_cond_wait(&cond, &lock);
    }
    pthread_mutex_unlock(&lock);
}

static void * thread(void * arg)
{
    int tid = (int)pthread_self();
    int rc;

    printf("STARTING thread w/ tid %d\n", tid);
    rc = (int)cr_init();
    if (rc < 0) {
	exit(-1);
    }

    (void)cr_register_callback(my_callback, &rc, CR_SIGNAL_CONTEXT);
    advance();
    wait_for(THREADS + 1);
    if (rc) {
	printf("RESTARTING tid %d, from checkpoint taken %s", tid, date);
    } else {
	printf("CONTINUING tid %d, after taking checkpoint\n", tid);
    }
    printf("EXITING tid %d at %s", tid, now());

    return NULL;
}

int main(void)
{
    pthread_t th[THREADS];
    int rc;
    int i;
	
    printf("Starting main process w/ pid %d\n", (int)getpid());
    main_tid = pthread_self();

    rc = (int)cr_init();
    if (rc < 0) {
	exit(-1);
    }

    (void)cr_register_callback(my_callback, &rc, CR_SIGNAL_CONTEXT);

    // Start one or more threads
    for (i = 0; i < THREADS; ++i) {
	pthread_create(&th[i], NULL, thread, NULL);
    }

    // Wait for them to initialize
    wait_for(THREADS);
    
    rc = checkpoint_self(); // should cause a checkpoint NOW
    if (rc < 0) {
	exit(-1);
    }

    if (rc) {
	printf("RESTARTING main thread, from checkpoint taken %s", date);
    } else {
	printf("CONTINUING main thread, after taking checkpoint\n");
    }

    advance();

    for (i = 0; i < THREADS; ++i) {
	rc = pthread_join(th[i], NULL);
	if (rc < 0) {
	    perror("pthread_join()");
	    exit(-1);
	}
    }

    printf("EXITING main thread (pid %d) at %s", (int)getpid(), now());

    return 0;
}
