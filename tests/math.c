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
 * $Id: math.c,v 1.3 2008/12/01 01:11:50 phargrov Exp $
 *
 * Simple example for using crut
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

#include "crut.h"

/* Peform some simple math in callback to ensure used_math() will
 * be non-zero when the checkpoint is taken on an i386 or x86_64.
 */
static double dummy = 1.23;
static int math_callback(void *arg)
{
    dummy *= (unsigned long)arg;
#if defined(__i386__) || defined(__x86_64__)
    {
	double r, q = dummy;
	__asm__ __volatile__ ("fldl %0" : : "m"(dummy));
	(void)cr_checkpoint(CR_CHECKPOINT_READY);
	__asm__ __volatile__ ("fstpl %0" : "=m"(r));
	if (r != q) {
	    CRUT_FAIL("FP restore failure %g != %g\n", r, q);
	    return -1;
	}
    }
#endif
    return 0;
}

static int
math_setup(void **testdata)
{
    CRUT_DEBUG("Initializing math.");
    *testdata = NULL;

    cr_register_callback(math_callback, (void *)(unsigned long)getppid(), CR_SIGNAL_CONTEXT);
    cr_register_callback(math_callback, (void *)(unsigned long)getpid(), CR_THREAD_CONTEXT);

    return 0;
}

static int
math_precheckpoint(void *p)
{
    CRUT_DEBUG("Testing sanity before we checkpoint");

    return (dummy > 0.0);
}

static int
math_continue(void *p)
{
    CRUT_DEBUG("Continuing after checkpoint.");

    return (dummy > 0.0);
}

static int
math_restart(void *p)
{
    CRUT_DEBUG("Restarting from checkpoint.");

    return (dummy > 0.0);
}

int
main(int argc, char *argv[])
{
    int ret;
    struct crut_operations math_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"math",
        test_description:"Simple test using the FPU.",
	test_setup:math_setup,
	test_precheckpoint:math_precheckpoint,
	test_continue:math_continue,
	test_restart:math_restart,
	test_teardown:NULL,
    };

    /* add the basic tests */
    crut_add_test(&math_test_ops);

    ret = crut_main(argc, argv);

    return ret;
}
