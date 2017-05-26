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
 * $Id: dev_null.c,v 1.6 2008/08/29 21:36:53 phargrov Exp $
 *
 * Test of /dev/{null,zero,full}
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "crut.h"

struct testdata {
    int null;
    int zero;
    int full;
};

static int
dev_null_setup(void **testdata)
{
    struct testdata *td;
    int fd;

    CRUT_DEBUG("Initializing dev_null.");
    td = malloc(sizeof(struct testdata));
    if (!td) return -1;

    td->null = fd = open("/dev/null", O_RDWR);
    if (fd < 0) {
	CRUT_FAIL("open(/dev/null): %s", strerror(errno));
	return -1;
    }
    td->zero = fd = open("/dev/zero", O_RDWR);
    if (fd < 0) {
	CRUT_FAIL("open(/dev/zero): %s", strerror(errno));
	return -1;
    }
    td->full = fd = open("/dev/full", O_WRONLY);
    if (fd < 0) {
	CRUT_FAIL("open(/dev/full): %s", strerror(errno));
	return -1;
    }

    *testdata = td;

    return 0;
}

static int check_it(struct testdata *td) {
    int retval = 0;
    int i;
    int rc;

    
    /* /dev/null: reads get EOF, writes succeed */
    i = 0x12345678;
    rc = read(td->null, &i, sizeof(i));
    if (rc < 0) {
	CRUT_FAIL("read(/dev/null) failed: %s", strerror(errno));
	retval = -1;
    } else if (rc != 0) {
	CRUT_FAIL("read(/dev/null) returned %d (expect 0)", rc);
	retval = -1;
    }
    rc = write(td->null, &i, sizeof(i));
    if (rc < 0) {
	CRUT_FAIL("write(/dev/null) failed: %s", strerror(errno));
	retval = -1;
    } else if (rc != sizeof(i)) {
	CRUT_FAIL("write(/dev/null) returned %d (expect %d)", rc, (int)sizeof(i));
	retval = -1;
    }

    /* /dev/zero: reads get zeroed bytes, writes succeed */
    i = 0x12345678;
    rc = read(td->zero, &i, sizeof(i));
    if (rc < 0) {
	CRUT_FAIL("read(/dev/zero) failed: %s", strerror(errno));
	retval = -1;
    } else if (rc != sizeof(i)) {
	CRUT_FAIL("read(/dev/zero) returned %d (expect %d)", rc, (int)sizeof(i));
	retval = -1;
    } else if (i != 0) {
	CRUT_FAIL("read(/dev/zero) read value %d (expect 0)", i);
	retval = -1;
    }
    rc = write(td->zero, &i, sizeof(i));
    if (rc < 0) {
	CRUT_FAIL("write(/dev/zero) failed: %s", strerror(errno));
	retval = -1;
    } else if (rc != sizeof(i)) {
	CRUT_FAIL("write(/dev/zero) returned %d (expect %d)", rc, (int)sizeof(i));
	retval = -1;
    }

    /* /dev/full: writes fail w/ ENOSPC */
    i = 0x12345678;
    rc = write(td->full, &i, sizeof(i));
    if (rc >= 0) {
	CRUT_FAIL("write(/dev/zero) returned %d (expect < 0)", rc);
	retval = -1;
    } else if (errno != ENOSPC) {
	CRUT_FAIL("write(/dev/full) failed: %s", strerror(errno));
	retval = -1;
    }

    return retval;
}

static int
dev_null_precheckpoint(void *p)
{
    CRUT_DEBUG("Testing sanity before we checkpoint");
    return check_it((struct testdata *)p);
}

static int
dev_null_continue(void *p)
{
    CRUT_DEBUG("Continuing after checkpoint.");
    return check_it((struct testdata *)p);
}

static int
dev_null_restart(void *p)
{
    CRUT_DEBUG("Restarting from checkpoint.");
    return check_it((struct testdata *)p);
}

static int
dev_null_teardown(void *p)
{
    struct testdata *td = (struct testdata *)p;

    close(td->null);
    close(td->zero);
    close(td->full);
    free(td);

    return 0;
}

int
main(int argc, char *argv[])
{
    int ret;
    struct crut_operations dev_null_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"/dev/null",
        test_description:"Test of /dev/{null,zero,full}",
	test_setup:dev_null_setup,
	test_precheckpoint:dev_null_precheckpoint,
	test_continue:dev_null_continue,
	test_restart:dev_null_restart,
	test_teardown:dev_null_teardown,
    };

    /* add the basic tests */
    crut_add_test(&dev_null_test_ops);

    ret = crut_main(argc, argv);

    return ret;
}
