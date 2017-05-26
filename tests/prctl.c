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
 * $Id: prctl.c,v 1.6.8.4 2011/10/04 21:07:21 phargrov Exp $
 *
 * Simple tests of prctl()
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "crut.h"
#include "crut_util.h"
#include "blcr_config.h"

#ifndef HAVE_PRCTL
int main(void) {
    printf("No prctl() support was found at configure time - test skipped\n");
    return 77;
}
#else

#include <sys/prctl.h>

enum {
	my_type_int_ptr,	// 2nd arg is int for set, but addr for get
	my_type_int,		// 2nd arg is int for set, and get is returned
	my_type_bool,		// like type_int except set must be 0 or 1
	my_type_comm		// 2nd arg is buffer for string in and out
};

struct test_elem {
    const char *name;
    int type;	// Type for 2nd arg
    int read;	// Value of 1st argument to read a value
    int write;	// Value of 1st argument to write a value
    int arg2a;	// First choice for 2nd argument
    int arg2b;	// Second choice for 2nd argument
    union {	// Actual value for 2nd argument
    	int i;
	char *s;
    } value;
};

char my_comm[32] = "testing\0";

struct test_elem test_table[] = {
#if defined(PR_GET_PDEATHSIG)
	/* Early 2.6.x kernels could DIE HARD if we set this one non-zero */
	{"PR_GET_PDEATHSIG", my_type_int_ptr, PR_GET_PDEATHSIG, PR_SET_PDEATHSIG, 0, 0, },
#endif
#if defined(PR_GET_DUMPABLE)
	/* Can't write anything but 1, or checkpoint will fail */
	{"PR_GET_DUMPABLE", my_type_bool, PR_GET_DUMPABLE, PR_SET_DUMPABLE, 1, 1, },
#endif
#if defined(PR_GET_UNALIGN)
	{"PR_GET_UNALIGN", my_type_int_ptr, PR_GET_UNALIGN, PR_SET_UNALIGN, 0, PR_UNALIGN_NOPRINT, },
#endif
#if defined(PR_GET_KEEPCAPS)
	{"PR_GET_KEEPCAPS", my_type_bool, PR_GET_KEEPCAPS, PR_SET_KEEPCAPS, 0, 1, },
#endif
#if defined(PR_GET_FPEMU)
	{"PR_GET_FPEMU", my_type_int_ptr, PR_GET_FPEMU, PR_SET_FPEMU, 0, 1, },
#endif
#if defined(PR_GET_FPEXC)
	{"PR_GET_FPEXC", my_type_int_ptr, PR_GET_FPEXC, PR_SET_FPEXC, PR_FP_EXC_DISABLED, PR_FP_EXC_PRECISE, },
#endif
#if defined(PR_GET_TIMING)
	{"PR_GET_TIMING", my_type_int, PR_GET_TIMING, PR_SET_TIMING, PR_TIMING_STATISTICAL, PR_TIMING_TIMESTAMP, },
#endif
#if defined(PR_GET_NAME)
	{"PR_GET_NAME", my_type_comm, PR_GET_NAME, PR_SET_NAME, },
#endif
#if defined(PR_GET_ENDIAN) && 0
	/* Not safe to change this one */
	{"PR_GET_ENDIAN", my_type_int_ptr, PR_GET_ENDIAN, PR_SET_ENDIAN, X, X, },
#endif
#if defined(PR_GET_SECCOMP) && 0
	/* Not safe to change this one */
	{"PR_GET_SECCOMP", my_type_int, PR_GET_SECCOMP, PR_SET_SECCOMP, X, X, },
#endif
#if defined(PR_GET_TSC)
	{"PR_GET_TSC", my_type_int_ptr, PR_GET_TSC, PR_SET_TSC, 1, 0, },
#endif
#if defined(PR_GET_TIMERSLACK)
	/* NOTE: don't use 0, as it is translated to the default value */
	{"PR_GET_TIMERSLACK", my_type_int, PR_GET_TIMERSLACK, PR_SET_TIMERSLACK, 50000, 100000, },
#endif
};

void prctl_init(void) {
  int i;

  for (i=0; i < sizeof(test_table)/sizeof(struct test_elem); ++i) {
    struct test_elem *elem = test_table + i;
    int result;
  
    switch (elem->type) {
    case my_type_int_ptr: {
      int tmp, val;
      elem->value.i = -1;
      result = prctl(elem->read, (unsigned long)&tmp);
      if (result < 0) break;
      CRUT_DEBUG("Read %s=%d", elem->name, tmp);
      val = (tmp == elem->arg2a) ? elem->arg2b : elem->arg2a;
      result = prctl(elem->write, val);
      if (result < 0) break;
      result = prctl(elem->read, (unsigned long)&val); /* Read back */
      if (result < 0) break;
      CRUT_DEBUG("Wrote %s=%d", elem->name, val);
      elem->value.i = val;
      break;
    }
  
    case my_type_bool:
    case my_type_int: {
      int val;
      elem->value.i = -1;
      result = prctl(elem->read, 0UL);
      if (result < 0) break;
      if ((elem->type == my_type_bool) && (result > 0)) result = 1;
      CRUT_DEBUG("Read %s=%d", elem->name, result);
      val = (result == elem->arg2a) ? elem->arg2b : elem->arg2a;
      result = prctl(elem->write, val);
      if (result < 0) break;
      val = prctl(elem->read, 0UL); /* Read back again */
      if (val < 0) break;
      if ((elem->type == my_type_bool) && (val > 0)) val = 1;
      CRUT_DEBUG("Wrote %s=%d", elem->name, val);
      elem->value.i = val;
      break;
    }
  
    case my_type_comm: {
      char tmp[32];
      elem->value.s = NULL;
      result = prctl(elem->read, tmp);
      if (result < 0) break;
      CRUT_DEBUG("Read %s='%s'", elem->name, tmp);
      result = prctl(elem->write, my_comm);
      if (result < 0) break;
      CRUT_DEBUG("Wrote %s='%s'", elem->name, my_comm);
      elem->value.s = my_comm;
      break;
    }
  
    default:
      result = -EINVAL;
      CRUT_FAIL("Bad type in test_table entry %s", elem->name);
    }
  }
}

int prctl_check(void) {
  int fail = 0;
  int i;

  for (i=0; i < sizeof(test_table)/sizeof(struct test_elem); ++i) {
    struct test_elem *elem = test_table + i;
    int result = 0;

    switch (elem->type) {
    case my_type_int_ptr: {
      int tmp;
      if (elem->value.i == -1) break;
      result = prctl(elem->read, (unsigned long)&tmp);
      if (result < 0) break;
      if (tmp != elem->value.i) {
	  CRUT_FAIL("Param %s changed from %d to %d\n",
		    elem->name, elem->value.i, tmp);
	  result = -1;
      }
      break;
    }

    case my_type_bool:
    case my_type_int: {
      if (elem->value.i == -1) break;
      result = prctl(elem->read, 0UL);
      if ((elem->type == my_type_bool) && (result > 0)) result = 1;
      if (result != elem->value.i) {
	  CRUT_FAIL("Param %s changed from %d to %d\n",
		    elem->name, elem->value.i, result);
	  result = -1;
      }
      break;
    }

    case my_type_comm: {
      char tmp[32];
      if (elem->value.s == NULL) break;
      result = prctl(elem->read, tmp);
      if (result < 0) break;
      if (strcmp(tmp, elem->value.s)) {
	  CRUT_FAIL("Param %s changed from '%s' to '%s'\n",
		    elem->name, elem->value.s, tmp);
	  result = -1;
      }
      break;
    }

    default:
      result = -EINVAL;
      CRUT_FAIL("Bad type in test_table entry %s", elem->name);
      exit(-1);
    }

    if (result < 0) fail++;
  }

  return fail ? -1 : 0;
}

static int
prctl_setup(void **testdata)
{
    CRUT_DEBUG("Initializing prctl.");

    prctl_init();
    *testdata = NULL;

    return 0;
}

static int
prctl_precheckpoint(void *p)
{
    int retval;

    CRUT_DEBUG("Testing sanity before we checkpoint");
    retval = prctl_check();

    return retval;
}

static int
prctl_continue(void *p)
{
    int retval;

    CRUT_DEBUG("Continuing after checkpoint.");
    retval = prctl_check();

    return retval;
}

static int
prctl_restart(void *p)
{
    int retval;

    CRUT_DEBUG("Restarting from checkpoint.");
    retval = prctl_check();

    return retval;
}

int
main(int argc, char *argv[])
{
    int ret;
    struct crut_operations prctl_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"prctl_rw",
        test_description:"Simple test or prctl.",
	test_setup:prctl_setup,
	test_precheckpoint:prctl_precheckpoint,
	test_continue:prctl_continue,
	test_restart:prctl_restart,
	test_teardown:NULL,
    };

    /* add the basic tests */
    crut_add_test(&prctl_test_ops);

    ret = crut_main(argc, argv);

    return ret;
}
#endif
