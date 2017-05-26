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
 * $Id: failed_cb2.c,v 1.7.12.1 2009/02/19 02:17:08 phargrov Exp $
 */


const char description[] = 
"Description of failed_cb2:\n"
"\n"
" Verifies that when a callback invokes cr_checkpoint() with a non-zero\n"
" argument, additional callbacks will be skipped.\n"
" Differs from failed_cb by using THREAD_CONTEXT instead of SIGNAL, and\n"
" by changes required by that difference.\n"
"\n"
"In a successful run, expect lines of output in SORTED order.\n"
;


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <pthread.h>

#include "crut_util.h"

static char *filename = NULL;

/* As requester of a checkpoint completed IN ERROR, we can see
 * completion before the thread callbacks finish.
 * This is our interlock.
 */
static pthread_mutex_t my_lock = PTHREAD_MUTEX_INITIALIZER;
static int thread_ran = 0;

static int my_checkpoint(void) {
    int retval;

    thread_ran = 0;
    retval = crut_checkpoint_block(filename);

    /* There is a small race between end of the callbacks
     * and the return to the IDLE state.
     */
    while (cr_status() == CR_STATE_PENDING) {
	pthread_mutex_lock(&my_lock);
	sched_yield();
	pthread_mutex_unlock(&my_lock);
    }

    return retval;
}

static int callback0(void* arg)
{
    printf("XXX enter 'other' callback unexpectedly\n");
    return 0;
}

static int callback1(void* arg)
{
    static int count = 0;
    int rc;

    switch (count) {
    case 0:
        printf("007 enter callback1 (TEMP)\n");
        rc = cr_checkpoint(CR_CHECKPOINT_TEMP_FAILURE);
        if (rc >= 0) {
	    printf("XXX resuming unexpectedly in callback1\n");
	    exit(-1);
        } else if (rc != -CR_ETEMPFAIL) {
	    printf("XXX callback1: checkpoint TEMP cancellation failed: %d (%s)\n", rc, cr_strerror(-rc));
	    exit(-1);
        } else {
	    printf("008 checkpoint cancelled successfully with TEMP fail\n");
        }
	break;

    case 1:
        printf("013 enter callback1 (OMIT)\n");
        rc = cr_checkpoint(CR_CHECKPOINT_OMIT);
        if (rc >= 0) {
	    printf("XXX resuming unexpectedly in callback_TEMP (rc=%d)\n", rc);
	    exit(-1);
        } else if (rc != -CR_EOMITTED) {
	    printf("XXX callback1: checkpoint OMIT cancellation failed: %d (%s)\n", rc, cr_strerror(-rc));
	    exit(-1);
        } else {
	    printf("014 checkpoint cancelled successfully with OMIT\n");
        }
	break;

    case 2:
        printf("019 enter callback1 (TEMP_CODE)\n");
        rc = cr_checkpoint(CR_CHECKPOINT_TEMP_FAILURE_CODE(911));
        if (rc >= 0) {
	    printf("XXX resuming unexpectedly in callback_TEMP_CODE (rc=%d)\n", rc);
	    exit(-1);
        } else if (rc != -911) {
	    printf("XXX callback1: checkpoint TEMP_CODE cancellation failed: %d (%s)\n", rc, cr_strerror(-rc));
	    exit(-1);
        } else {
	    printf("020 checkpoint cancelled successfully with TEMP_CODE\n");
        }
	break;

    default:
        (void)unlink(filename); /* ICK */
        printf("025 enter callback1 (PERM)\n");
        printf("026 DONE\n");
        (void)cr_checkpoint(CR_CHECKPOINT_PERM_FAILURE);
        printf("XXX cr_checkpoint(CR_CHECKPOINT_PERM_FAILURE) returned unexpectedly\n");
        exit(-1);
    }
    ++count;

    return 0;
}

static int callback2(void* arg)
{
    static int count = 0;
    int rc;

    pthread_mutex_lock(&my_lock);

    switch (count) {
    case 0:
        printf("006 in callback2 (TEMP)\n");
        rc = cr_checkpoint(0);
        if (rc >= 0) {
	    printf("XXX resuming unexpectedly in callback2\n");
	    exit(-1);
        } else if (rc != -CR_ETEMPFAIL) {
	    printf("XXX callback2: checkpoint TEMP cancellation failed: %d (%s)\n", rc, cr_strerror(-rc));
	    exit(-1);
        } else {
	    printf("009 checkpoint cancelled successfully with TEMP fail\n");
        }
	break;

    case 1:
        printf("012 in callback2 (OMIT)\n");
        rc = cr_checkpoint(0);
        if (rc >= 0) {
	    printf("XXX resuming unexpectedly in callback2\n");
	    exit(-1);
        } else if (rc != -CR_EOMITTED) {
	    printf("XXX callback2: checkpoint OMIT %d (%s)\n", rc, cr_strerror(-rc));
	    exit(-1);
        } else {
	    printf("015 checkpoint cancelled successfully with OMIT\n");
        }
	break;

    case 2:
        printf("018 in callback2 (TEMP_CODE)\n");
        rc = cr_checkpoint(0);
        if (rc >= 0) {
	    printf("XXX resuming unexpectedly in callback2\n");
	    exit(-1);
        } else if (rc != -911) {
	    printf("XXX callback2: checkpoint TEMP_CODE %d (%s)\n", rc, cr_strerror(-rc));
	    exit(-1);
        } else {
	    printf("021 checkpoint cancelled successfully with TEMP_CODE\n");
        }
	break;

    default:
        printf("024 in callback0 (PERM)\n");
        (void)cr_checkpoint(0);
        printf("XXX unexpected return from final checkpoint\n");
    }
    ++count;

    thread_ran = 1;
    pthread_mutex_unlock(&my_lock);

    return 0;
}

int main(void)
{
    pid_t my_pid;
    cr_callback_id_t cb_id;
    cr_client_id_t my_id;
    int rc;

    setlinebuf(stdout);
	
    my_pid = getpid();
    filename = crut_aprintf("context.%d", my_pid);
    printf("000 Process started with pid %d\n", my_pid);

    if (crut_is_linuxthreads()) {
	fprintf(stderr, "Skipping threaded 'failed_cb2' test due to LinuxThreads issues\n");
        printf("001 DONE\n");
	return 77;
    }

    printf("#ST_ALARM:120\n");
    printf("#ST_SIGNAL:9\n");

    my_id = cr_init();
    if (my_id < 0) {
	printf("XXX cr_init() failed, returning %d\n", my_id);
	exit(-1);
    } else {
	printf("001 cr_init() succeeded\n");
    }

    cb_id = cr_register_callback(callback0, NULL, CR_THREAD_CONTEXT);
    if (cb_id < 0) {
	printf("XXX cr_register_callback() unexpectedly returned %d\n", cb_id);
	exit(-1);
    } else {
	printf("002 cr_register_callback() correctly returned %x\n", cb_id);
    }

    cb_id = cr_register_callback(callback1, NULL, CR_THREAD_CONTEXT);
    if (cb_id < 0) {
	printf("XXX cr_register_callback() unexpectedly returned %d\n", cb_id);
	exit(-1);
    } else {
	printf("003 cr_register_callback() correctly returned %x\n", cb_id);
    }

    cb_id = cr_register_callback(callback2, NULL, CR_THREAD_CONTEXT);
    if (cb_id < 0) {
	printf("XXX cr_register_callback() unexpectedly returned %d\n", cb_id);
	exit(-1);
    } else {
	printf("004 cr_register_callback() correctly returned %x\n", cb_id);
    }

    rc = cr_status();
    if (rc != CR_STATE_IDLE) {
	printf("XXX cr_status() unexpectedly returned %d\n", rc);
	exit(-1);
    } else {
	printf("005 cr_status() correctly returned 0x%x\n", rc);
    }

    /* First pass... TEMP */
    rc = my_checkpoint();
    if (rc != CR_POLL_CHKPT_ERR_POST) {
	printf("XXX crut_checkpoint_block() unexpectedly returned %d\n", rc);
	exit(-1);
    }
    if (errno != CR_ETEMPFAIL) {
	printf("XXX crut_checkpoint_block() returned unexpected errno=%d(%s)\n", errno, cr_strerror(errno));
	exit(-1);
    }
    printf("010 cr_poll_checkpoint() correctly returned w/ errno=CR_ETEMPFAIL\n");

    rc = cr_status();
    if (rc != CR_STATE_IDLE) {
	printf("XXX cr_status() unexpectedly returned 0x%x\n", rc);
	exit(-1);
    } else {
	printf("011 cr_status() correctly returned 0x%x\n", rc);
    }

    /* Second pass... OMIT */
    rc = my_checkpoint();
    if (rc != CR_POLL_CHKPT_ERR_POST) {
	printf("XXX crut_checkpoint_block() unexpectedly returned %d\n", rc);
	exit(-1);
    }
    if (errno != ESRCH) {
	printf("XXX crut_checkpoint_block() returned unexpected errno=%d(%s)\n", errno, cr_strerror(errno));
	exit(-1);
    }
    printf("016 cr_poll_checkpoint() correctly returned w/ errno=ESRCH\n");

    rc = cr_status();
    if (rc != CR_STATE_IDLE) {
	printf("XXX cr_status() unexpectedly returned 0x%x\n", rc);
	exit(-1);
    } else {
	printf("017 cr_status() correctly returned 0x%x\n", rc);
    }

    /* Third pass... TEMP_CODE */
    rc = my_checkpoint();
    if (rc != CR_POLL_CHKPT_ERR_POST) {
	printf("XXX crut_checkpoint_block() unexpectedly returned %d\n", rc);
	exit(-1);
    }
    if (errno != 911) {
	printf("XXX crut_checkpoint_block() returned unexpected errno=%d(%s)\n", errno, cr_strerror(errno));
	exit(-1);
    }
    printf("022 cr_poll_checkpoint() correctly returned user-specified errno=911\n");

    rc = cr_status();
    if (rc != CR_STATE_IDLE) {
	printf("XXX cr_status() unexpectedly returned 0x%x\n", rc);
	exit(-1);
    } else {
	printf("023 cr_status() correctly returned 0x%x\n", rc);
    }

    /* Final/fatal pass... PERM */
    (void)my_checkpoint();
    printf("XXX crut_checkpoint_block() returned unexpectedly\n");
    return -1;
}
