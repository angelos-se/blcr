/*
 * Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
 * 2009, The Regents of the University of California, through Lawrence
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
 * $Id: nscd.c,v 1.3.6.2 2011/08/03 20:18:18 eroman Exp $
 *
 * Test to trigger NCSD problems if they exists
 */

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <netdb.h>

#include "crut.h"

static int
nscd_check(void *p)
{
    int rc = 0;

    /* Series of calls to trigger use of NSCD if possible */

    (void) gethostbyname("www.google.com");

    if (! getpwuid(0)) {
	CRUT_FAIL("Lookup of uid 0 failed");
	rc = -1;
    }

    if (! getgrgid(0)) {
	CRUT_FAIL("Lookup of gid 0 failed");
	rc = -1;
    }

    return rc;
}
     
static int
nscd_setup(void **testdata)
{
    *testdata = NULL;
    return nscd_check(NULL);
}

int
main(int argc, char *argv[])
{
    struct crut_operations nscd_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"nscd",
        test_description:"Test to trigger nscd problems if any.",
	test_setup:nscd_setup,
	test_precheckpoint:nscd_check,
	test_continue:nscd_check,
	test_restart:nscd_check,
	test_teardown:NULL,
    };

    /* add the basic tests */
    crut_add_test(&nscd_test_ops);

    return crut_main(argc, argv);

}
