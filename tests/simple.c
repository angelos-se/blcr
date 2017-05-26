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
 * $Id: simple.c,v 1.9 2008/08/29 21:36:53 phargrov Exp $
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

#define MSG "Hello world!\n"

static int my_pid;

static int
simple_setup(void **testdata)
{
    char msg[] = MSG;
    void *p;

    CRUT_DEBUG("Initializing simple.");
    CRUT_DEBUG("sizeof(msg) = %d", (int)sizeof(msg)+1);
    p = malloc(sizeof(msg)+1);
    if (p == NULL) { return -1; }
    memcpy(p, msg, sizeof(msg)+1);
    *testdata = p;

    return 0;
}

static int
simple_precheckpoint(void *p)
{
    int retval;

    CRUT_DEBUG("Testing sanity before we checkpoint");
    retval = strncmp((char *)p, MSG, sizeof(MSG)) ? -1 : 0;
    my_pid = (int)getpid();

    return retval;
}

static int
simple_continue(void *p)
{
    int retval;

    CRUT_DEBUG("Continuing after checkpoint.");
    retval = strncmp((char *)p, MSG, sizeof(MSG)) ? -1 : 0;
    if (my_pid != (int)getpid()) {
	retval = -1;
    }

    return retval;
}

static int
simple_restart(void *p)
{
    int retval;

    CRUT_DEBUG("Restarting from checkpoint.");
    retval = strncmp((char *)p, MSG, sizeof(MSG)) ? -1 : 0;
    if (my_pid != (int)getpid()) {
	retval = -1;
    }

    return retval;
}

static int
simple_teardown(void *p)
{
    free(p);

    return 0;
}

int
main(int argc, char *argv[])
{
    int ret;
    struct crut_operations simple_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"simple_rw",
        test_description:"Simple example test.",
	test_setup:simple_setup,
	test_precheckpoint:simple_precheckpoint,
	test_continue:simple_continue,
	test_restart:simple_restart,
	test_teardown:simple_teardown,
    };

    /* add the basic tests */
    crut_add_test(&simple_test_ops);

    ret = crut_main(argc, argv);

    return ret;
}
