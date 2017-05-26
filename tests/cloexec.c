/*
 * Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
 * 2004, The Regents of the University of California, through Lawrence
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
 * $Id: cloexec.c,v 1.7 2008/08/29 21:36:53 phargrov Exp $
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

#include "crut.h"

#define TEST_FILE "tstfile.0"

typedef struct test_data_t_ {
    int fd0;	/* file w/ cloexec = 0 */
    int fd1;	/* file w/ cloexec = 1 */
} test_data_t;

static int
cloexec_setup(void **testdata)
{
    int rc;
    test_data_t *p;

    CRUT_DEBUG("Initializing cloexec.");
    p = malloc(sizeof(*p));
    if (p == NULL) { return -1; }

    CRUT_DEBUG("Open()ing and dup()ing '" TEST_FILE "'.");
    (void)unlink(TEST_FILE);
    p->fd0 = open(TEST_FILE, O_RDWR | O_CREAT | O_EXCL, 0600);
    p->fd1 = dup(p->fd0);
    if ((p->fd0 < 0) || (p->fd1 < 0)) { return -1; }
    
    CRUT_DEBUG("Setting and clearing CLOEXEC flags.");
    rc = fcntl(p->fd0, F_GETFD);
    if (rc < 0) { return -1; }
    rc = fcntl(p->fd0, F_SETFD, rc & ~FD_CLOEXEC);
    if (rc < 0) { return -1; }
    rc = fcntl(p->fd1, F_GETFD);
    if (rc < 0) { return -1; }
    rc = fcntl(p->fd1, F_SETFD, rc | FD_CLOEXEC);
    if (rc < 0) { return -1; }

    *testdata = p;

    return 0;
}

static int
checkit(test_data_t *t)
{
    int rc;

    rc = fcntl(t->fd0, F_GETFD);
    if (rc & FD_CLOEXEC) { return -1; }
    rc = fcntl(t->fd1, F_GETFD);
    if (!(rc & FD_CLOEXEC)) { return -1; }

    return 0;
}

static int
cloexec_precheckpoint(void *p)
{
    CRUT_DEBUG("Testing sanity before we checkpoint");
    return checkit((test_data_t *)p);
}

static int
cloexec_continue(void *p)
{
    CRUT_DEBUG("Continuing after checkpoint.");
    return checkit((test_data_t *)p);
}

static int
cloexec_restart(void *p)
{
    CRUT_DEBUG("Restarting from checkpoint.");
    return checkit((test_data_t *)p);
}

static int
cloexec_teardown(void *p)
{
    test_data_t *t = (test_data_t *)p;

    (void)unlink(TEST_FILE);
    (void)close(t->fd0);
    (void)close(t->fd1);
    free(p);

    return 0;
}

int
main(int argc, char *argv[])
{
    int ret;
    struct crut_operations cloexec_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"cloexec",
        test_description:"Check for restore of close-on-exec flag.",
	test_setup:cloexec_setup,
	test_precheckpoint:cloexec_precheckpoint,
	test_continue:cloexec_continue,
	test_restart:cloexec_restart,
	test_teardown:cloexec_teardown,
    };

    /* add the test(s) */
    crut_add_test(&cloexec_test_ops);

    ret = crut_main(argc, argv);

    return ret;
}
