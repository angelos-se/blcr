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
 * $Id: overlap.c,v 1.8 2008/08/29 21:36:53 phargrov Exp $
 *
 * crut-based test to ensure that checkpoint requests are excluded while
 * checkpoints and restarts are running.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <linux/limits.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "libcr.h"
#include "crut.h"

static char *cmd = NULL;

static void do_it(const char *where) {
    int status = system(cmd);

    if (!WIFEXITED(status)) {
	CRUT_FAIL("Abnormal failure calling '%s' at %s time", cmd, where);
	exit(1);
    } else if (WEXITSTATUS(status) != EBUSY) {
	CRUT_FAIL("Unexpected exit code %d calling '%s' at %s time", WEXITSTATUS(status), cmd, where);
	exit(1);
    } else {
	CRUT_DEBUG("Correct result at %s time", where);
    }
}


static int my_callback(void* arg)
{
    int rc;

    do_it("CHECKPOINT");
    rc = cr_checkpoint(0);
    if (rc > 0) {
	do_it("RESTART");
    } else if (!rc) {
	do_it("CONTINUE");
    }

    return 0;
}

static int
overlap_setup(void **testdata)
{

    CRUT_DEBUG("Initializing overlap.");

    (void)cr_init();
    (void)cr_register_callback(my_callback, NULL, CR_THREAD_CONTEXT);

    *testdata = NULL;
    return 0;
}

static int
overlap_precheckpoint(void *p)
{
    return 0;
}

static int
overlap_continue(void *p)
{
    CRUT_DEBUG("Continuing after checkpoint.");
    return 0;
}

static int
overlap_restart(void *p)
{

    CRUT_DEBUG("Restarting from checkpoint.");
    return 0;
}

static int
overlap_teardown(void *p)
{
    return 0;
}

int
main(int argc, char * const argv[])
{
    int ret;
    struct crut_operations overlap_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"overlap_chkpt",
        test_description:"test for checkpoint request overlapping checkpoint or restart.",
	test_setup:overlap_setup,
	test_precheckpoint:overlap_precheckpoint,
	test_continue:overlap_continue,
	test_restart:overlap_restart,
	test_teardown:overlap_teardown,
    };
    int pid = getpid();
    
    cmd = crut_aprintf("exec %s -f context.%d %d 2>/dev/null",
			crut_find_cmd(argv[0], "cr_checkpoint"), pid, pid);

    /* add the basic tests */
    crut_add_test(&overlap_ops);

    ret = crut_main(argc, argv);

    return ret;
}
