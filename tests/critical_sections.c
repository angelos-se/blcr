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
 * $Id: critical_sections.c,v 1.5 2008/07/26 06:38:15 phargrov Exp $
 */


const char description[] = 
"Description of tests/CriticalSections/cr_test:\n"
"\n"
"This test verifies the behavior of critical sections in libcr:\n"
" + cr_enter_cs()\n"
"	+ Enters a critical section which excludes checkpoints, inclusive\n"
"	  of thread-context callbacks.\n"
" + cr_leave_cs()\n"
"	Leaves a critical section, invoking a checkpoint if one is pending.\n"
" + cr_status()\n"
"	Must reflect the status of the thread-context callback as well.\n"
"\n"
"In a successful run, expect lines of output in SORTED order.\n"
;


#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <pthread.h>

#include "crut_util.h"

static pthread_mutex_t my_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t my_cond = PTHREAD_COND_INITIALIZER;
static volatile int thread_ran = 0;

static int my_callback(void* arg)
{
    int rc;

    printf("004 enter callback\n");
    rc = cr_checkpoint(CR_CHECKPOINT_READY);
    if (rc) {
	printf("XXX resuming unexpectedly in callback\n");
	exit(-1);
    } else {
	printf("005 continuing correctly in callback\n");
    }

    pthread_mutex_lock(&my_lock);
    thread_ran = 1;
    pthread_cond_signal(&my_cond);
    pthread_mutex_unlock(&my_lock);

    return 0;
}

int main(void)
{
    pid_t my_pid;
    cr_callback_id_t cb_id;
    cr_client_id_t my_id;
    cr_checkpoint_handle_t my_handle;
    char *filename = NULL;
    struct stat s;
    int rc, fd;
	
    setlinebuf(stdout);

    my_pid = getpid();
    filename = crut_aprintf("context.%d", my_pid);
    printf("000 Process started with pid %d\n", my_pid);
    printf("#ST_ALARM:120\n");

    my_id = cr_init();
    if (my_id < 0) {
	printf("XXX cr_init() failed\n");
	exit(-1);
    } else {
	printf("001 cr_init() succeeded\n");
    }

    cb_id = cr_register_callback(my_callback, NULL, CR_THREAD_CONTEXT);
    if (cb_id < 0) {
	printf("XXX cr_register_callback() unexpectedly returned %d\n", cb_id);
	exit(-1);
    }

    cr_enter_cs(my_id);
    rc = cr_status();
    if (rc != CR_STATE_IDLE) {
	printf("XXX cr_status() unexpectedly returned %d\n", rc);
	exit(-1);
    } else {
	printf("002 cr_status() correctly returned IDLE\n");
    }

    fd = crut_checkpoint_request(&my_handle, filename);
    if (fd < 0) {
	printf("XXX crut_checkpoint_request() unexpectedly returned %d\n", rc);
	exit(-1);
    } else {
	printf("003 crut_checkpoint_request() correctly returned 0\n");
    }

    sleep(1);
    if (thread_ran) {
	printf("XXX thread ran unexpectedly in critical section\n");
	exit(-1);
    }

    /* Wait for request to be pending */ 
    while (cr_status() != CR_STATE_PENDING) {
	sched_yield();
    }

    cr_leave_cs(my_id);	// should allow checkpoint to start

    pthread_mutex_lock(&my_lock);
    while (!thread_ran) pthread_cond_wait(&my_cond, &my_lock);
    pthread_mutex_unlock(&my_lock);

    rc = crut_checkpoint_wait(&my_handle, fd);
    if (rc < 0) {
	printf("XXX crut_checkpoint_wait() unexpectedly returned %d errno=%d(%s)\n", rc, errno, cr_strerror(errno));
	exit(-1);
    }

    rc = stat(filename, &s);
    if (rc) {
	printf("XXX stat() unexpectedly returned %d\n", rc);
	exit(-1);
    } else {
	printf("006 stat(context.%d) correctly returned 0\n", my_pid);
    }

    if (s.st_size == 0) {
	printf("XXX context file unexpectedly empty\n");
	exit(-1);
    } else {
	printf("007 context.%d is non-empty\n", my_pid);
    }

    (void)unlink(filename); // might fail silently

    printf("008 DONE\n");

    return 0;
}
