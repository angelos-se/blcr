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
 * $Id: readdir.c,v 1.6 2008/12/10 01:32:47 phargrov Exp $
 *
 * Test directory open across a checkpoint
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include "crut.h"

#define MYNAME "readdir"

static char *test_dir = NULL;

static int
opendir_setup(void **testdata)
{
    DIR *dp;

    CRUT_DEBUG("Initializing opendir.");

    dp = opendir(test_dir);
    if (dp == NULL) { return -1; }
    *testdata = dp;

    return 0;
}

static int scan(DIR *dp, int check_ptr)
{
    static off_t prev_pos;
    struct dirent *ent;
    int found = 0;
    int retval = 0;
	    
    if (check_ptr) {
	if (prev_pos != telldir(dp)) return -1;
	CRUT_DEBUG("Verified position %d", (int)prev_pos);
    }

    rewinddir(dp);

    while (!found && ((ent = readdir(dp)) != NULL)) {
	CRUT_DEBUG("Entry '%s'", ent->d_name);
	if (!strcmp(ent->d_name, MYNAME)) {
	    CRUT_DEBUG("Found '" MYNAME "'");
	    found = 1;
       	}
    }

    if (!found) {
	retval = -1;
    } else if (!check_ptr) {
	prev_pos = telldir(dp);
	if ((int)prev_pos == -1) retval = -1;
	CRUT_DEBUG("Saved position %d", (int)prev_pos);
    }

    return retval;
}

static int
opendir_precheckpoint(void *p)
{
    DIR *dp = (DIR*)p;
    int retval;

    CRUT_DEBUG("Testing sanity before we checkpoint");
    retval = scan(dp, 0);

    return retval;
}

static int
opendir_continue(void *p)
{
    DIR *dp = (DIR*)p;
    int retval;

    CRUT_DEBUG("Continuing after checkpoint.");
    retval = scan(dp, 1);

    return retval;
}

static int
opendir_restart(void *p)
{
    DIR *dp = (DIR*)p;
    int retval;

    CRUT_DEBUG("Restarting from checkpoint.");
    retval = scan(dp, 1);

    return retval;
}

static int
opendir_teardown(void *p)
{
    DIR *dp = (DIR*)p;

    return closedir(dp);
}

int
main(int argc, char *argv[])
{
    int ret;
    struct crut_operations opendir_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"opendir",
        test_description:"opendir() test.",
	test_setup:opendir_setup,
	test_precheckpoint:opendir_precheckpoint,
	test_continue:opendir_continue,
	test_restart:opendir_restart,
	test_teardown:opendir_teardown,
    };

    /* add the basic tests */
    crut_add_test(&opendir_test_ops);

    test_dir = crut_find_testsdir(argv[0]);

    ret = crut_main(argc, argv);

    return ret;
}
