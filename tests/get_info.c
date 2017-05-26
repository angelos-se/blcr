/*
 * Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
 * 2007, The Regents of the University of California, through Lawrence
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
 * $Id: get_info.c,v 1.9 2008/08/29 21:36:53 phargrov Exp $
 *
 * crut-based test for cr_get_{checkpoint,restart}_info() functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "libcr.h"
#include "crut.h"

static char *filename = NULL;


/* Callback to verify checkpoint info */
static int my_callback_chkpt(void* arg)
{
    const struct cr_checkpoint_info *chkpt_info;
    int status, rc;

    chkpt_info = cr_get_checkpoint_info();

    status = CR_CHECKPOINT_READY;
    if (!chkpt_info) {
	fprintf(stderr, "NULL checkpoint info\n");
        status = CR_CHECKPOINT_PERM_FAILURE;
    }
    if (strcmp(chkpt_info->dest, filename)) {
	fprintf(stderr, "Destination filename mismatch\nWant '%s'\nGot '%s'\n", filename, chkpt_info->dest);
        status = CR_CHECKPOINT_PERM_FAILURE;
    }
    if (chkpt_info->signal != 0) {
	fprintf(stderr, "Unexpected non-zero signal\n");
        status = CR_CHECKPOINT_PERM_FAILURE;
    }

    rc = cr_checkpoint(status);

    return 0;
}
/* Callback to verify restart info */
static int my_callback_rstrt(void* arg)
{
    const struct cr_restart_info *rstrt_info;
    int status, rc;

    rc = cr_checkpoint(CR_CHECKPOINT_READY);

    status = CR_CHECKPOINT_READY;
    if (rc > 0) { /* Restart */
        rstrt_info = cr_get_restart_info();
        if (!rstrt_info) {
	    fprintf(stderr, "NULL checkpoint info\n");
            status = CR_CHECKPOINT_PERM_FAILURE;
        }
        if (strcmp(rstrt_info->src, filename)) {
	    fprintf(stderr, "Source filename mismatch\nWant '%s'\nGot '%s'\n", filename, rstrt_info->src);
            status = CR_CHECKPOINT_PERM_FAILURE;
        }
	/* The kill(pid,0) will check for an existing process w/ same uid */
        if ((rstrt_info->requester <= 0) || kill(rstrt_info->requester, 0)) {
	    fprintf(stderr, "Invalid requester %d\n", rstrt_info->requester);
            status = CR_CHECKPOINT_PERM_FAILURE;
        }
    }

    return status;
}

static int
get_info_setup_generic(void **testdata, cr_callback_t cb)
{
    size_t alloc_size= PATH_MAX+1;
    int rc, pid;

    CRUT_DEBUG("Initializing get_info.");
    filename = malloc(alloc_size);
    
    pid = getpid();
    if (pid < 0) {
        perror("getpid");
        return -1;
    }

    filename = getcwd(filename, alloc_size);
    if ((filename == NULL) || (*filename == '\0')) {
        perror("getcwd");
        return -1;
    }

    filename = crut_sappendf(filename, "/context.%d", pid);

    rc = (int)cr_init();
    rc = (int)cr_register_callback(cb, NULL, CR_THREAD_CONTEXT);

    *testdata = NULL;
    return 0;
}

static int get_info_setup_chkpt(void **testdata)
{ return get_info_setup_generic(testdata, my_callback_chkpt); }
	
static int get_info_setup_rstrt(void **testdata)
{ return get_info_setup_generic(testdata, my_callback_rstrt); }
	
static int
get_info_precheckpoint(void *p)
{
    return 0;
}

static int
get_info_continue(void *p)
{
    CRUT_DEBUG("Continuing after checkpoint.");
    return 0;
}

static int
get_info_restart(void *p)
{

    CRUT_DEBUG("Restarting from checkpoint.");
    return 0;
}

static int
get_info_teardown(void *p)
{
    free(filename);
    return 0;
}

int
main(int argc, char *argv[])
{
    int ret;
    struct crut_operations chkpt_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"cr_get_checkpoint_info",
        test_description:"cr_get_checkpoint_info() validation test.",
	test_setup:get_info_setup_chkpt,
	test_precheckpoint:get_info_precheckpoint,
	test_continue:get_info_continue,
	test_restart:get_info_restart,
	test_teardown:get_info_teardown,
    };
    struct crut_operations rstrt_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"cr_get_restart_info",
        test_description:"cr_get_restart_info() validation test.",
	test_setup:get_info_setup_rstrt,
	test_precheckpoint:get_info_precheckpoint,
	test_continue:get_info_continue,
	test_restart:get_info_restart,
	test_teardown:get_info_teardown,
    };

    /* add the basic tests */
    crut_add_test(&chkpt_test_ops);
    crut_add_test(&rstrt_test_ops);

    ret = crut_main(argc, argv);

    return ret;
}
