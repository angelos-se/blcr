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
 * $Id: stage0001.c,v 1.8.12.1 2012/12/18 18:32:08 phargrov Exp $
 */


const char description[] = 
"Description of tests/Stage0001/cr_test:\n"
"\n"
"This test verifies the basic 'Stage I' functionality of both libcr and the\n"
"blcr.o kernel module.  This basic functionality includes the following:\n"
" + cr_init()\n"
"	Sets up libcr\n"
" + cr_register_callback()\n"
"	Registers a signal context callback.\n"
" + cr_enter_cs()\n"
"	Enters a critical section which excludes checkpoints.\n"
" + cr_leave_cs()\n"
"	Leaves a critical section, invoking a checkpoint if one is pending.\n"
" + cr_status()\n"
"	Indicates if checkpoint is idle, pending or active.\n"
" + cr_request_checkpoint()\n"
"	Can requests a checkpoint be taken of the invoking process.\n"
" + cr_poll_checkpoint()\n"
"	Reaps a checkpoint request.\n"
" + cr_checkpoint()\n"
"	Invoked by callback(s) when the checkpoint may be taken.\n"
"\n"
"To test the functionality this test does the following:\n"
" - Calls cr_init().\n"
" - Register a callback.\n"
" - Enter a critical section.\n"
" - Request a checkpoint (it will be defered).\n"
" - Verify that a checkpoint is pending.\n"
" - Leave the critical section (checkpoint will run now).\n"
" - Verify that callback is run.\n"
" - Verify that cr_checkpoint() returns the CONTINUE case.\n"
" - Request a second checkpoint which runs immediately.\n"
" - Verify that the callback is run again.\n"
" - Verify that cr_checkpoint() returns the CONTINUE case.\n"
" - Verify that a 'context.<pid>' file exists.\n"
" - Verify that 'context.<pid>' is non-empty.\n"
" - Request a third checkpoint which aborts with TEMPFAIL.\n"
"\n"
"In a successful run, expect lines of output in SORTED order.\n"
;


#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "crut_util.h"

static int my_callback(void* arg)
{
    static int count = 0;
    int rc;

    switch(count++) {
    case 0:
	printf("005 enter callback\n");
	rc = cr_checkpoint(0);
	if (rc) {
	    printf("XXX resuming unexpectedly in callback. (rc=%d)\n", rc);
	    exit(-1);
	} else {
	    printf("006 continuing correctly in callback\n");
	}
	break;

    case 1:
	printf("008 enter callback again\n");
	rc = cr_checkpoint(0);
	if (rc) {
	    printf("XXX resuming unexpectedly in callback\n");
	    exit(-1);
	} else {
	    printf("009 continuing correctly in callback again\n");
	}
	break;
    case 2:
	printf("012 enter callback again: testing cancellation\n");
	rc = cr_checkpoint(CR_CHECKPOINT_TEMP_FAILURE);
	if (rc >= 0) {
	    printf("XXX resuming unexpectedly in callback\n");
	    exit(-1);
	} else if (rc != -CR_ETEMPFAIL) {
	    printf("XXX checkpoint TEMP cancellation failed: %d (%s)\n",
			    			rc, cr_strerror(-rc));
	    exit(-1);
	} else {
	    printf("013 checkpoint cancelled successfully with TEMP fail\n");
	}
	break;
    default:
	printf("XXX entered callback unexpectedly: count=%d\n", count);
    }

    return 0;
}

int main(void)
{
    pid_t my_pid;
    cr_callback_id_t cb_id;
    cr_client_id_t my_id;
    cr_checkpoint_handle_t my_handle;
    char *filename;
    int rc, fd;
    struct stat s;

    setlinebuf(stdout);

    my_pid = getpid();
    filename = crut_aprintf("context.%d", my_pid);
    printf("000 Process started with pid %d\n", my_pid);
    printf("#ST_ALARM:120\n");

    my_id = cr_init();
    if (my_id < 0) {
	printf("XXX cr_init() failed, returning %d\n", my_id);
	exit(-1);
    } else {
	printf("001 cr_init() succeeded\n");
    }

    cb_id = cr_register_callback(my_callback, NULL, CR_SIGNAL_CONTEXT);
    if (cb_id < 0) {
	printf("XXX cr_register_callback() unexpectedly returned %d\n", cb_id);
	exit(-1);
    } else {
	printf("002 cr_register_callback() correctly returned %d\n", cb_id);
    }

    cr_enter_cs(my_id);
    rc = cr_status();
    if (rc != CR_STATE_IDLE) {
	printf("XXX cr_status() unexpectedly returned %d\n", rc);
	exit(-1);
    } else {
	printf("003 cr_status() correctly returned 0x%x\n", rc);
    }

    /* Request a checkpoint of ourself - should be blocked by critical section */
    fd = crut_checkpoint_request(&my_handle, filename);
    if (fd < 0) {
	printf("XXX crut_checkpoint_request() unexpectedly returned 0x%x\n", fd);
	exit(-1);
    }

    rc = cr_status();
    if (rc != CR_STATE_PENDING) { /* NOTE: only work because there are not threads */
	printf("XXX cr_status() unexpectedly returned 0x%x\n", rc);
	exit(-1);
    } else {
	printf("004 cr_status() correctly returned 0x%x\n", rc);
    }

    cr_leave_cs(my_id);	// should cause checkpoint NOW

    rc = cr_status();
    if (rc != CR_STATE_IDLE) {
	printf("XXX cr_status() unexpectedly returned 0x%x\n", rc);
	exit(-1);
    } else {
	printf("007 cr_status() correctly returned 0x%x\n", rc);
    }

    /* Reap the checkpoint request */
    rc = crut_checkpoint_wait(&my_handle, fd);
    if (rc < 0) {
	printf("XXX crut_checkpoint_wait() #1 unexpectedly returned 0x%x\n", rc);
	exit(-1);
    }

    /* Request a checkpoint of ourself - should happen NOW */
    fd = crut_checkpoint_request(&my_handle, filename);
    if (fd < 0) {
	printf("XXX crut_checkpoint_request() unexpectedly returned 0x%x\n", fd);
	exit(-1);
    }

    rc = stat(filename, &s);
    if (rc) {
	printf("XXX stat() unexpectedly returned %d\n", rc);
	exit(-1);
    } else {
	printf("010 stat(context.%d) correctly returned 0\n", my_pid);
    }

    if (s.st_size == 0) {
	printf("XXX context file unexpectedly empty\n");
	exit(-1);
    } else {
	printf("011 context.%d is non-empty\n", my_pid);
    }

    /* Reap the checkpoint request */
    rc = crut_checkpoint_wait(&my_handle, fd);
    if (rc < 0) {
	printf("XXX crut_checkpoint_wait() #2 unexpectedly returned 0x%x\n", rc);
	exit(-1);
    }

    (void)unlink(filename); // might fail silently

    /* 
     * Do another checkpoint, but this time abort it with TEMPFAIL
     */
    fd = crut_checkpoint_request(&my_handle, filename);
    if (fd < 0) {
	printf("XXX crut_checkpoint_request() unexpectedly returned 0x%x\n", fd);
	exit(-1);
    }
    rc = crut_checkpoint_wait(&my_handle, fd);
    if (rc != CR_POLL_CHKPT_ERR_POST) {
	printf("XXX crut_checkpoint_wait() #3 unexpectedly returned 0x%x\n", rc);
	exit(-1);
    }
    if (errno != CR_ETEMPFAIL) {
	printf("XXX crut_checkpoint_wait() #3 unexpectedly got errno=%d(%s)\n", errno, cr_strerror(errno));
	exit(-1);
    }

    printf("014 DONE\n");

    (void)unlink(filename); // might fail silently

    return 0;
}
