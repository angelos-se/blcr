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
 * $Id: atomics.c,v 1.3 2008/09/04 01:52:49 phargrov Exp $
 *
 * Simple serial tests of the atomic operations used inside libcr
 */

#include <stdio.h>

#include "blcr_config.h"
#include "cr_atomic.h"

#define CHECK(cond) \
  do { if (!(cond)) { ++failed; printf("Test (" #cond ") FAILED at line %d\n", __LINE__); } } while (0)


int main(int argc, char *argv[])
{
    cri_atomic_t X = 0;
    int failed = 0;

    /* Is the initial value 0? */
    CHECK(cri_atomic_read(&X) == 0);

    /* Write 1 */
    cri_atomic_write(&X, 1);
    CHECK(cri_atomic_read(&X) == 1);

    /* Increment 1->2 */
    cri_atomic_inc(&X);
    CHECK(cri_atomic_read(&X) == 2);

    /* Decrement 2->1, expecting "false" result */
    CHECK(cri_atomic_dec_and_test(&X) == 0);
    CHECK(cri_atomic_read(&X) == 1);

    /* Decrement 1->0, expecting "true" result */
    CHECK(cri_atomic_dec_and_test(&X) != 0);
    CHECK(cri_atomic_read(&X) == 0);

    /* Compare-and-swap 0->4, expecting "true" result */
    CHECK(cri_cmp_swap(&X, 0, 4) != 0);
    CHECK(cri_atomic_read(&X) == 4);

    /* Try compare-and-swap 0->5 when value is actually 4, expecting "false" result */
    CHECK(cri_cmp_swap(&X, 0, 5) == 0);
    CHECK(cri_atomic_read(&X) == 4); /* Should *not* have changed */

    /* Compare-and-swap 4->5, expecting "true" result */
    CHECK(cri_cmp_swap(&X, 4, 5) != 0);
    CHECK(cri_atomic_read(&X) == 5);

    /* Compare-and-swap 5->5, expecting "true" result */
    CHECK(cri_cmp_swap(&X, 5, 5) != 0);
    CHECK(cri_atomic_read(&X) == 5);

    return !!failed;
}
