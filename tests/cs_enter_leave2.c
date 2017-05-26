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
 * $Id: cs_enter_leave2.c,v 1.12 2008/12/26 10:50:35 phargrov Exp $
 */

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

#include "crut_util.h"

#define LIMIT 30

static volatile int dummy = 0;
static volatile int counter = 0;
static volatile int done = 0;

static int my_callback(void *arg)
{
    printf("%d enter callback\n", 3*counter+1);
    (void)cr_checkpoint(CR_CHECKPOINT_READY);
    printf("%d leave callback\n", 3*counter+2);

    return 0;
}

static void *thread_main(void *arg) 
{
    cr_client_id_t my_id;
    cr_callback_id_t cb_id;

    my_id = cr_init();
    if (my_id < 0) {
	printf("XXX cr_init() failed\n");
	exit(-1);
    }

    cb_id = cr_register_callback(my_callback, NULL, CR_SIGNAL_CONTEXT);
    if (cb_id < 0) {
	printf("XXX cr_register_callback() unexpectedly returned %d\n", cb_id);
	exit(-1);
    }

    fprintf(stderr, "cs_enter_leave2:");
    crut_progress_start(time(NULL), LIMIT, 10);
    do {
	int prev = dummy;
	printf("%d issue request %d\n", 3*counter, counter);
	crut_checkpoint_block(NULL);
	++counter;
	while (dummy == prev) sched_yield(); // Avoid overflowing stack with nested signal frames
    } while (crut_progress_step(time(NULL)));

    done = 1 | dummy;
    return NULL;
}


int main(void)
{
    pthread_t th;
    int join_ret;
    void *join_val;
    cr_client_id_t my_id;

    setlinebuf(stdout);
    printf("#ST_ALARM:%d\n", 2*LIMIT);

    my_id = cr_init();
    if (my_id < 0) {
	printf("XXX cr_init() failed\n");
	exit(-1);
    }
	
    if (crut_pthread_create(&th, NULL, &thread_main, NULL) != 0) {
	printf("XXX pthread_create() unexpectedly failed\n");
	exit(-1);
    }

    do {
	cr_enter_cs(my_id);
	dummy = dummy ^ counter;
        cr_leave_cs(my_id);
    } while (!done);

    join_ret = pthread_join(th, &join_val);
    if (join_ret || join_val) {
	printf("XXX pthread_join() unexpectedly failed\n");
	exit(-1);
    }

    printf("%d DONE\n", 3*counter);

    return 0;
}
