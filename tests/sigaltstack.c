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
 * $Id: sigaltstack.c,v 1.1 2008/11/26 07:11:48 phargrov Exp $
 *
 * CRUT test of sigaltstack()
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>

#include "crut.h"

static int
checkit(const char *where, stack_t *st)
{
    stack_t tmp;
    int rc;

    rc = sigaltstack(NULL, &tmp);
    if (rc < 0) {
	CRUT_FAIL("%s: sigaltstack(get) failed.", where);
	return -1;
    }

    rc = 0;
    if (st->ss_size != tmp.ss_size) {
	CRUT_FAIL("%s: ss_size changed.", where);
	rc = -1;
    }
    if (st->ss_sp != tmp.ss_sp) {
	CRUT_FAIL("%s: ss_sp changed.", where);
	rc = -1;
    }
    if (st->ss_flags != tmp.ss_flags) {
	CRUT_FAIL("%s: ss_flags changed.", where);
	rc = -1;
    }

    return rc;
}

static int my_callback(void *stack2) {
    stack_t st;
    int rc;

    st.ss_flags = 0;
    st.ss_size = SIGSTKSZ;
    st.ss_sp = stack2;

    rc = sigaltstack(&st, NULL);
    if (rc < 0) {
	CRUT_FAIL("sigaltstack(set2) failed.");
	return -1;
    }

    cr_checkpoint(CR_CHECKPOINT_READY);

    return checkit("callback", &st);
}

static int
sigaltstack_setup(void **testdata)
{
    stack_t *st;
    void *stack1, *stack2;
    int rc;

    CRUT_DEBUG("Initializing sigaltstack.");


    stack1 = mmap (NULL, SIGSTKSZ, PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (stack1 == MAP_FAILED) {
	CRUT_FAIL("mmap(stack1) failed.");
	return -1;
    }

    stack2 = mmap (NULL, SIGSTKSZ, PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (stack2 == MAP_FAILED) {
	CRUT_FAIL("mmap(stack2) failed.");
	return -1;
    }

    cr_register_callback(my_callback, stack2, CR_THREAD_CONTEXT);

    st = malloc(sizeof(stack_t));
    if (st == NULL) {
	CRUT_FAIL("malloc() failed.");
	return -1;
    }
    st->ss_flags = 0;
    st->ss_size = SIGSTKSZ;
    st->ss_sp = stack1;

    rc = sigaltstack(st, NULL);
    if (rc < 0) {
	CRUT_FAIL("sigaltstack(set) failed.");
	return -1;
    }

    *testdata = st;

    return 0;
}

static int
sigaltstack_precheckpoint(void *p)
{
    CRUT_DEBUG("Testing sanity before we checkpoint");
    return checkit("precheckpoint", (stack_t *)p);
}

static int
sigaltstack_continue(void *p)
{
    CRUT_DEBUG("Continuing after checkpoint.");
    return checkit("continue", (stack_t *)p);
}

static int
sigaltstack_restart(void *p)
{
    CRUT_DEBUG("Restarting from checkpoint.");
    return checkit("restart", (stack_t *)p);
}

int
main(int argc, char *argv[])
{
    int ret;
    struct crut_operations sigaltstack_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"sigaltstack",
        test_description:"Test of sigaltstack.",
	test_setup:sigaltstack_setup,
	test_precheckpoint:sigaltstack_precheckpoint,
	test_continue:sigaltstack_continue,
	test_restart:sigaltstack_restart,
	test_teardown:NULL,
    };

    /* add the basic tests */
    crut_add_test(&sigaltstack_test_ops);

    ret = crut_main(argc, argv);

    return ret;
}
