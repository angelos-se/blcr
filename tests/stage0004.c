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
 * $Id: stage0004.c,v 1.1.34.1 2009/02/14 02:55:39 phargrov Exp $
 */

const char description[] =
"Description of stage0004:\n"
"\n"
"This test tries several calls to libcr functions without their prerequisites\n";

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "libcr.h"

int my_callback(void *arg) { return 0; }

int main(void)
{
    pid_t my_pid;
    cr_callback_id_t cb_id;
    cr_client_id_t my_id;
    void *ptr;
    int rc;

    setlinebuf(stdout);

    my_pid = getpid();
    printf("000 Process started with pid %d\n", my_pid);

    /* First several calls that are invalid w/o a call to cr_init() */
    my_id = 0; /* FAKE IT */

    rc = cr_status();
    if ((rc < 0) && (errno == CR_ENOINIT)) {
	printf("001 cr_status() failed w/ errno=CR_ENOINIT as expected\n");
    } else if (rc >= 0) {
	printf("XXX cr_status() unexpectedly suceeded\n");
	exit(-1);
    } else {
	printf("XXX cr_status() failed w/ errno=%d(%s)\n", errno, cr_strerror(errno));
	exit(-1);
    }

    rc = cr_enter_cs(my_id);
    if ((rc < 0) && (errno == CR_ENOINIT)) {
	printf("002 cr_enter_cs() failed w/ errno=CR_ENOINIT as expected\n");
    } else if (rc >= 0) {
	printf("XXX cr_enter_cs() unexpectedly suceeded\n");
	exit(-1);
    } else {
	printf("XXX cr_enter_cs() failed w/ errno=%d(%s)\n", errno, cr_strerror(errno));
	exit(-1);
    }

    rc = cr_leave_cs(my_id);
    if ((rc < 0) && (errno == CR_ENOINIT)) {
	printf("003 cr_leave_cs() failed w/ errno=CR_ENOINIT as expected\n");
    } else if (rc >= 0) {
	printf("XXX cr_leave_cs() unexpectedly suceeded\n");
	exit(-1);
    } else {
	printf("XXX cr_leave_cs() failed w/ errno=%d(%s)\n", errno, cr_strerror(errno));
	exit(-1);
    }

    rc = cr_tryenter_cs(my_id);
    if ((rc < 0) && (errno == CR_ENOINIT)) {
	printf("004 cr_tryenter_cs() failed w/ errno=CR_ENOINIT as expected\n");
    } else if (rc >= 0) {
	printf("XXX cr_tryenter_cs() unexpectedly suceeded\n");
	exit(-1);
    } else {
	printf("XXX cr_tryenter_cs() failed w/ errno=%d(%s)\n", errno, cr_strerror(errno));
	exit(-1);
    }

    cb_id = cr_register_callback(my_callback, NULL, CR_SIGNAL_CONTEXT);
    if ((rc < 0) && (errno == CR_ENOINIT)) {
	printf("005 cr_register_callback() failed w/ errno=CR_ENOINIT as expected\n");
    } else if (rc >= 0) {
	printf("XXX cr_register_callback() unexpectedly suceeded\n");
	exit(-1);
    } else {
	printf("XXX cr_register_callback() failed w/ errno=%d(%s)\n", errno, cr_strerror(errno));
	exit(-1);
    }

    rc = cr_replace_callback(0 /*?*/, my_callback, NULL, CR_SIGNAL_CONTEXT);
    if ((rc < 0) && (errno == CR_ENOINIT)) {
	printf("006 cr_replace_callback() failed w/ errno=CR_ENOINIT as expected\n");
    } else if (rc >= 0) {
	printf("XXX cr_replace_callback() unexpectedly suceeded\n");
	exit(-1);
    } else {
	printf("XXX cr_replace_callback() failed w/ errno=%d(%s)\n", errno, cr_strerror(errno));
	exit(-1);
    }

    rc = cr_inc_persist();
    if ((rc < 0) && (errno == CR_ENOINIT)) {
	printf("007 cr_inc_persist() failed w/ errno=CR_ENOINIT as expected\n");
    } else if (rc >= 0) {
	printf("XXX cr_inc_persist() unexpectedly suceeded\n");
	exit(-1);
    } else {
	printf("XXX cr_inc_persist() failed w/ errno=%d(%s)\n", errno, cr_strerror(errno));
	exit(-1);
    }

    rc = cr_dec_persist();
    if ((rc < 0) && (errno == CR_ENOINIT)) {
	printf("008 cr_dec_persist() failed w/ errno=CR_ENOINIT as expected\n");
    } else if (rc >= 0) {
	printf("XXX cr_dec_persist() unexpectedly suceeded\n");
	exit(-1);
    } else {
	printf("XXX cr_dec_persist() failed w/ errno=%d(%s)\n", errno, cr_strerror(errno));
	exit(-1);
    }

    ptr = (void*)cr_register_hook(0, NULL);
    if ((ptr == (void*)CR_HOOK_FN_ERROR) && (errno == CR_ENOINIT)) {
	printf("009 cr_register_hook() failed w/ errno=CR_ENOINIT as expected\n");
    } else if (ptr != (void*)CR_HOOK_FN_ERROR) {
	printf("XXX cr_register_hook() unexpectedly suceeded\n");
	exit(-1);
    } else {
	printf("XXX cr_register_hook() failed w/ errno=%d(%s)\n", errno, cr_strerror(errno));
	exit(-1);
    }

    my_id = cr_init();
    if (my_id >= 0) {
	printf("010 cr_init() succeeded\n");
    } else {
	printf("XXX cr_init() failed, returning %d\n", my_id);
	exit(-1);
    }

    /* Now some that are invalid outside of callbacks */

    rc = cr_checkpoint(0);
    if (rc == -CR_ENOTCB) {
	printf("011 cr_checkpoint() failed returned -CR_ENOTCB as expected\n");
    } else {
	printf("XXX cr_checkpoint() unexpectedly returned %d\n", rc);
	exit(-1);
    }

    rc = cr_replace_self(my_callback, NULL, CR_SIGNAL_CONTEXT);
    if ((rc < 0) && (errno == CR_ENOTCB)) {
	printf("012 cr_replace_self() failed returned -CR_ENOTCB as expected\n");
    } else if (rc >= 0) {
	printf("XXX cr_replace_self() unexpectedly suceeded\n");
	exit(-1);
    } else {
	printf("XXX cr_replace_self() unexpectedly returned %d\n", rc);
	exit(-1);
    }

    ptr = (void *)cr_get_checkpoint_info();
    if (ptr == NULL) {
	printf("013 cr_get_checkpoint_info() failed returned -CR_ENOTCB as expected\n");
    } else {
	printf("XXX cr_get_checkpoint_info() unexpectedly returned non-NULL\n");
	exit(-1);
    }

    ptr = (void *)cr_get_restart_info();
    if (ptr == NULL) {
	printf("014 cr_get_restart_info() failed returned -CR_ENOTCB as expected\n");
    } else {
	printf("XXX cr_get_restart_info() unexpectedly returned non-NULL\n");
	exit(-1);
    }

    rc = cr_forward_checkpoint(CR_SCOPE_PROC, my_pid);
    if ((rc < 0) && (errno == CR_ENOTCB)) {
	printf("015 cr_forward_checkpoint() failed returned -CR_ENOTCB as expected\n");
    } else if (rc >= 0) {
	printf("XXX cr_forward_checkpoint() unexpectedly suceeded\n");
	exit(-1);
    } else {
	printf("XXX cr_forward_checkpoint() unexpectedly returned %d\n", rc);
	exit(-1);
    }

    /* Argument checks */

    rc = cr_dec_persist();
    if ((rc < 0) && (errno == ERANGE)) {
	printf("016 cr_dec_persist() failed w/ errno=ERANGE as expected\n");
    } else if (rc >= 0) {
	printf("XXX cr_dec_persist() unexpectedly suceeded\n");
	exit(-1);
    } else {
	printf("XXX cr_dec_persist() failed w/ errno=%d(%s)\n", errno, cr_strerror(errno));
	exit(-1);
    }

    /* NOTE: testing cr_inc_persists() is impractical */

    ptr = (void *)cr_register_hook(0, CR_HOOK_FN_ERROR);
    if ((ptr == (void*)CR_HOOK_FN_ERROR) && (errno == EINVAL)) {
	printf("017 cr_register_hook(0,CR_HOOK_FN_ERROR) failed w/ errno=EINVAL as expected\n");
    } else if (ptr != (void*)CR_HOOK_FN_ERROR) {
	printf("XXX cr_register_hook(0,CR_HOOK_FN_ERROR) unexpectedly suceeded\n");
	exit(-1);
    } else {
	printf("XXX cr_register_hook(0,CR_HOOK_FN_ERROR) failed w/ errno=%d(%s)\n", errno, cr_strerror(errno));
	exit(-1);
    }

    ptr = (void *)cr_register_hook(-1, NULL);
    if ((ptr == (void*)CR_HOOK_FN_ERROR) && (errno == EINVAL)) {
	printf("018 cr_register_hook(-1,NULL) failed w/ errno=EINVAL as expected\n");
    } else if (ptr != (void*)CR_HOOK_FN_ERROR) {
	printf("XXX cr_register_hook(-1,NULL) unexpectedly suceeded\n");
	exit(-1);
    } else {
	printf("XXX cr_register_hook(-1,NULL) failed w/ errno=%d(%s)\n", errno, cr_strerror(errno));
	exit(-1);
    }

    ptr = (void *)cr_register_hook(CR_NUM_HOOKS, NULL);
    if ((ptr == (void*)CR_HOOK_FN_ERROR) && (errno == EINVAL)) {
	printf("019 cr_register_hook(CR_NUM_HOOKS,-1) failed w/ errno=EINVAL as expected\n");
    } else if (ptr != (void*)CR_HOOK_FN_ERROR) {
	printf("XXX cr_register_hook(CR_NUM_HOOKS,-1) unexpectedly suceeded\n");
	exit(-1);
    } else {
	printf("XXX cr_register_hook(CR_NUM_HOOKS,-1) failed w/ errno=%d(%s)\n", errno, cr_strerror(errno));
	exit(-1);
    }

    printf("020 DONE\n");

    return 0;
}
