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
 * $Id: crut.h,v 1.10 2008/12/12 02:19:34 phargrov Exp $
 *
 * Header for BLCR unit test functions
 */

#include "blcr_common.h" /* For internal bits */
#include "crut_util.h"

/*
 * limits
 */
#define CRUT_MAX_TESTS 255
#define CRUT_TESTNAME_MAX 80
#define CRUT_TESTDESC_MAX 255

/*
 * constants for crut_wait()
 */
#define CRUT_EVENT_SETUP 10
#define CRUT_EVENT_PRECHECKPOINT 20
#define CRUT_EVENT_CONTINUE 30
#define CRUT_EVENT_RESTART 40
#define CRUT_EVENT_TEARDOWN 50
#define CRUT_EVENT_NEVER 1000

__BEGIN_DECLS

typedef int crut_event_t;

struct crut_operations
{
    cr_scope_t test_scope;
    char test_name[CRUT_TESTNAME_MAX+1];
    char test_description[CRUT_TESTDESC_MAX+1];
    int (*test_setup)(void **);
    int (*test_teardown)(void *);
    int (*test_precheckpoint)(void *);
    int (*test_continue)(void *);
    int (*test_restart)(void *);
};

extern int  crut_main(int argc, char * const *argv);
extern void crut_add_test(struct crut_operations *test_ops);
extern int  crut_wait(crut_event_t event);
extern int  crut_poll(crut_event_t event);

__END_DECLS
