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
 * $Id: cwd.c,v 1.7 2008/08/29 21:36:53 phargrov Exp $
 *
 * Simple example for using crut
 */

#ifndef _GNU_SOURCE
  #define _GNU_SOURCE // for get_current_dir_name() in unistd
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include <linux/limits.h>

#include "crut.h"

#define TMP_DIR_NAME "tstdir"
#define TMP_DIR_MODE 0755

char checkpoint_cwd[PATH_MAX+1];

static int
cwd_setup(void **testdata)
{
    int retval;

    (void)rmdir(TMP_DIR_NAME);

    retval = mkdir(TMP_DIR_NAME, TMP_DIR_MODE);
    if (retval < 0) {
	perror("mkdir");
        goto out;
    }

    retval = chdir(TMP_DIR_NAME);
    if (retval < 0) {
	perror("mkdir");
        goto out;
    }

    if (getcwd(checkpoint_cwd, sizeof(checkpoint_cwd)) == NULL) {
	perror("getcwd");
        goto out_unlink;
    }
    retval = 0;
    *testdata = NULL;

    return retval;

out_unlink:
    if (unlink(TMP_DIR_NAME) < 0) {
	perror("unlink");
	retval = -1;
    }
out:
    return retval;
}

static int
cwd_precheckpoint(void *p)
{
    return 0;
}

static int
cwd_continue(void *p)
{
    return 0;
}

static int
cwd_restart(void *p)
{
    int retval = -1;
    char new_cwd[PATH_MAX+1];

    if (getcwd(new_cwd, PATH_MAX) == NULL) {
	perror("getcwd");
        goto out;
    }

    if (strncmp(checkpoint_cwd, new_cwd, sizeof(checkpoint_cwd))) {
	CRUT_FAIL("Did not restore cwd %s", checkpoint_cwd);
	retval = -1;
    } else {
	retval = 0;
    }

out:
    return retval;
}

static int
cwd_teardown(void *p)
{
    int retval;

    /* go back to where we started */
    retval = chdir("..");
    CRUT_DEBUG("cwd = %s", get_current_dir_name());
    if (retval < 0) {
        perror("chdir");
	goto out;
    }

    /* now kill the test directory */
retry:
    retval = rmdir(TMP_DIR_NAME);
    if (retval < 0) {
        CRUT_FAIL("rmdir(%s): %s", TMP_DIR_NAME, strerror(errno));
	// XXX
        sync();
	goto retry;
	goto out;
    }

    retval = 0;

out:
    return retval;
}

int
main(int argc, char *argv[])
{
    int ret;
    struct crut_operations cwd_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"cwd_rw",
        test_description:"Simple example test.",
	test_setup:cwd_setup,
	test_precheckpoint:cwd_precheckpoint,
	test_continue:cwd_continue,
	test_restart:cwd_restart,
	test_teardown:cwd_teardown,
    };

    /* add the basic tests */
    crut_add_test(&cwd_test_ops);

    ret = crut_main(argc, argv);

    return ret;
}
