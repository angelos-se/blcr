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
 * $Id: replace_cb.c,v 1.9 2008/07/26 06:38:15 phargrov Exp $
 */


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "crut_util.h"

static int cb0(void* arg);
static int cb1(void* arg);
static int cb2(void* arg);
static int cb3(void* arg);
static int cb4(void* arg);
static int cb5(void* arg);

int count = 0;

static void out(const char* s)
{
    write(1, s, strlen(s));
}

static int cb0(void* arg)
{
    int ret;
    out(arg); out(" (in cb0)\n");
    ret = cr_replace_self(cb2, "003 self/THREAD from CHECKPOINT section", CR_THREAD_CONTEXT);
    if (ret) {
        printf("XXX cr_replace_self() in cb0 returned %d w/ errno=%d(%s)\n", ret, errno, cr_strerror(errno));
    }
    ret = cr_replace_self(cb2, "XXX wrong flags from cb0", CR_SIGNAL_CONTEXT);
    if (!ret) {
        printf("XXX cr_replace_self() in cb0 suceeded unexpectedly\n");
    } else if (errno != EINVAL) {
        printf("XXX cr_replace_self() in cb0 returned %d w/ errno=%d(%s)\n", ret, errno, cr_strerror(errno));
    }
    return 0;
}

static int cb1(void* arg)
{
    int ret;
    out(arg); out(" (in cb1)\n");
    ret = cr_replace_self(cb3, "004 self/SIGNAL from CHECKPOINT section", CR_SIGNAL_CONTEXT);
    if (ret) {
        printf("XXX cr_replace_self() in cb1 returned %d w/ errno=%d(%s)\n", ret, errno, cr_strerror(errno));
    }
    ret = cr_replace_self(cb3, "XXX wrong flags from cb2", CR_THREAD_CONTEXT);
    if (!ret) {
        printf("XXX cr_replace_self() in cb1 suceeded unexpectedly\n");
    } else if (errno != EINVAL) {
        printf("XXX cr_replace_self() in cb1 returned %d w/ errno=%d(%s)\n", ret, errno, cr_strerror(errno));
    }
    return 0;
}

static int cb2(void* arg)
{
    int ret;
    out(arg); out(" (in cb2)\n");
    ret = cr_replace_self(cb4, "XXX self/THREAD not overwritten by CONTINUE", CR_THREAD_CONTEXT);
    if (ret) {
        printf("XXX cr_replace_self() cb2/pre returned %d w/ errno=%d(%s)\n", ret, errno, cr_strerror(errno));
    }
    ret = cr_checkpoint(0);
    if (ret) {
        printf("XXX cr_checkpoint() unexpectedy returned %d in cb2\n", ret);
    }
    ret = cr_replace_self(cb4, "005 self/THREAD from CONTINUE section", CR_THREAD_CONTEXT);
    if (ret) {
        printf("XXX cr_replace_self() cb2/post returned %d w/ errno=%d(%s)\n", ret, errno, cr_strerror(errno));
    }
    return 0;
}

static int cb3(void* arg)
{
    int ret;
    out(arg); out(" (in cb3)\n");
    ret = cr_replace_self(cb5, "XXX self/SIGNAL not overwritten by CONTINUE", CR_SIGNAL_CONTEXT);
    if (ret) {
        printf("XXX cr_replace_self() cb3/pre returned %d w/ errno=%d(%s)\n", ret, errno, cr_strerror(errno));
    }
    cr_checkpoint(0);
    if (ret) {
        printf("XXX cr_checkpoint() unexpectedy returned %d in cb3\n", ret);
    }
    ret = cr_replace_self(cb5, "006 self/SIGNAL from CONTINUE section", CR_SIGNAL_CONTEXT);
    if (ret) {
        printf("XXX cr_replace_self() cb3/post returned %d w/ errno=%d(%s)\n", ret, errno, cr_strerror(errno));
    }
    return 0;
}

static int cb4(void* arg)
{
    out(arg); out(" (in cb4)\n");
    return 0;
}

static int cb5(void* arg)
{
    out(arg); out(" (in cb5)\n");
    return 0;
}

int main(void)
{
    pid_t my_pid;
    cr_callback_id_t id0;
    cr_callback_id_t id1;
    cr_client_id_t my_id;
    char *filename;
    int ret;
	
    setlinebuf(stdout);

    my_pid = getpid();
    filename = crut_aprintf("context.%d", my_pid);
    printf("000 Process started with pid %d\n", my_pid);
    printf("#ST_ALARM:120\n");
    fflush(stdout);

    my_id = cr_init();

    id0 = cr_register_callback(cb0, "001 register THREAD from main", CR_THREAD_CONTEXT);
    id1 = cr_register_callback(cb1, "002 register SIGNAL from main", CR_SIGNAL_CONTEXT);

    ret = crut_checkpoint_block(filename);
    if (ret < 0) {
        printf("XXX crut_checkpoint_block() #1 returned %d w/ errno=%d(%s)\n", ret, errno, cr_strerror(errno));
    }
    
    ret = crut_checkpoint_block(filename);
    if (ret < 0) {
        printf("XXX crut_checkpoint_block() #2 returned %d w/ errno=%d(%s)\n", ret, errno, cr_strerror(errno));
    }

    ret = crut_checkpoint_block(filename);
    if (ret < 0) {
        printf("XXX crut_checkpoint_block() #3 returned %d w/ errno=%d(%s)\n", ret, errno, cr_strerror(errno));
    }

    cr_enter_cs(my_id);
    // These two should succeed
    ret = cr_replace_callback(id0, cb4, "007 replace THREAD from main", CR_THREAD_CONTEXT);
    if (ret) {
        printf("XXX cr_replace_callback() #2 returned %d w/ errno=%d(%s)\n", ret, errno, cr_strerror(errno));
    }
    ret = cr_replace_callback(id1, cb5, "008 replace SIGNAL from main", CR_SIGNAL_CONTEXT);
    if (ret) {
        printf("XXX cr_replace_callback() #1 returned %d w/ errno=%d(%s)\n", ret, errno, cr_strerror(errno));
    }
    // Next two should fail due to wrong context in flags
    ret = cr_replace_callback(id0, cb4, "XXX THREAD->SIGNAL from main", CR_SIGNAL_CONTEXT);
    if (!ret) {
        printf("XXX cr_replace_callback() #3 suceeded unexpectedly\n");
    } else if (errno != EINVAL) {
        printf("XXX cr_replace_callback() #3 returned %d w/ errno=%d(%s)\n", ret, errno, cr_strerror(errno));
    }
    ret = cr_replace_callback(id1, cb5, "XXX SIGNAL->THREAD from main", CR_THREAD_CONTEXT);
    if (!ret) {
        printf("XXX cr_replace_callback() #4 suceeded unexpectedly\n");
    } else if (errno != EINVAL) {
        printf("XXX cr_replace_callback() #4 returned %d w/ errno=%d(%s)\n", ret, errno, cr_strerror(errno));
    }
    // Next two should fail due to invalid id
    // -1 can't be valid since that is the error return from register
    ret = cr_replace_callback(-1, cb4, NULL, CR_SIGNAL_CONTEXT);
    if (!ret) {
        printf("XXX cr_replace_callback() #5 suceeded unexpectedly\n");
    } else if (errno != EINVAL) {
        printf("XXX cr_replace_callback() #5 returned %d w/ errno=%d(%s)\n", ret, errno, cr_strerror(errno));
    }
    ret = cr_replace_callback(-1, cb5, NULL, CR_THREAD_CONTEXT);
    if (!ret) {
        printf("XXX cr_replace_callback() #6 suceeded unexpectedly\n");
    } else if (errno != EINVAL) {
        printf("XXX cr_replace_callback() #6 returned %d w/ errno=%d(%s)\n", ret, errno, cr_strerror(errno));
    }
    //
    cr_leave_cs(my_id);

    ret = crut_checkpoint_block(filename);
    if (ret < 0) {
        printf("XXX crut_checkpoint_block() #4 returned %d w/ errno=%d(%s)\n", ret, errno, cr_strerror(errno));
    }

    (void)unlink(filename);	// may fail silently

    out("009 DONE\n");

    return 0;
}
