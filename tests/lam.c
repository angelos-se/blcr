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
 * $Id: lam.c,v 1.2 2008/12/05 23:15:21 phargrov Exp $
 *
 * Test exec from restart portion of signal callback, in presence of a
 * thread callback.  This is the "pattern" of LAM/MPI's mpirun.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "crut.h"

static int thread_callback(void *arg)
{
    (void)cr_checkpoint(CR_CHECKPOINT_READY);
    return 0;
}

static int signal_callback(void *arg)
{
    int rc = cr_checkpoint(CR_CHECKPOINT_READY);
    if (rc > 0) {
        execl("/bin/true", "true", NULL);
    }
    return 0;
}

static int
lam_setup(void **testdata)
{
    CRUT_DEBUG("Initializing lam.");
    *testdata = NULL;

    cr_register_callback(signal_callback, NULL, CR_SIGNAL_CONTEXT);
    cr_register_callback(thread_callback, NULL, CR_THREAD_CONTEXT);

    return 0;
}

static int
lam_precheckpoint(void *p)
{
    return 0;
}

static int
lam_continue(void *p)
{
    return 0;
}

static int
lam_restart(void *p)
{
    return 0;
}

int
main(int argc, char *argv[])
{
    int ret;
    struct crut_operations lam_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"lam",
        test_description:"test to mimic behavior of lam's mpirun.",
	test_setup:lam_setup,
	test_precheckpoint:lam_precheckpoint,
	test_continue:lam_continue,
	test_restart:lam_restart,
	test_teardown:NULL,
    };

    crut_add_test(&lam_test_ops);

    ret = crut_main(argc, argv);

    return ret;
}
