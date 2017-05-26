/* 
 * Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
 * 2003, The Regents of the University of California, through Lawrence
 * Berkeley National Laboratory (subject to receipt of any required
 * approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * Portions may be copyrighted by others, as may be noted in specific
 * copyright notices within specific files.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: cr_rb_lock.h,v 1.9 2007/12/17 22:33:06 phargrov Exp $
 *
 *
 * Implement red/black locks
 *
 * This is like a reader-writer lock allowing multiple writers.
 * Red is the readers and Black is the writers.
 * 
 * State == 0	Nobody holds the lock
 * State == +n  Lock is held by n Reds
 * State == -n  Lock is held by n Blacks
 */

#ifndef _CR_RB_LOCK_H
#define _CR_RB_LOCK_H 1

#include "cr_atomic.h"	// for cri_atomic_t

// XXX:
// If contention is common then look for a way to block rather than spinning
// on the value of x.  This would be much easier if we stopped allowing
// an application/library to enter/leave critical sections from signal context.

typedef cri_atomic_t cri_rb_lock_t;

#define	CRI_RB_LOCK_INITIALIZER	{0,}

// cri_rb_init()
CR_INLINE void cri_rb_init(cri_rb_lock_t 	*x)
{
    cri_atomic_write(x, 0);
}

// cri_red_lock()
//
// Acquires the lock by incrementing x iff (x >= 0)
//
CR_INLINE void cri_red_lock(cri_rb_lock_t	*x)
{
    int old;

    do {
	CRI_WHILE_COND_YIELD((old = cri_atomic_read(x)) < 0);
    } while (!cri_cmp_swap(x, old, old + 1));
}

// cri_red_trylock()
//
// Acquires the lock by incrementing x iff (x >= 0)
// Returns 0 on success
CR_INLINE int cri_red_trylock(cri_rb_lock_t	*x)
{
    int old;

    do {
	if ((old = cri_atomic_read(x)) < 0) return 1;
    } while (!cri_cmp_swap(x, old, old + 1));
    return 0;
}

// cri_red_unlock()
//
// Releases the lock by decrementing state
CR_INLINE void cri_red_unlock(cri_rb_lock_t	*x)
{
    (void)cri_atomic_dec_and_test(x);
}

// cri_black_lock()
//
// Acquires the lock by decrementing state iff (state <= 0).
CR_INLINE void cri_black_lock(cri_rb_lock_t	*x)
{
    int old;

    do {
	CRI_WHILE_COND_YIELD((old = cri_atomic_read(x)) > 0);
    } while (!cri_cmp_swap(x, old, old - 1));
}

// cri_red_trylock()
//
// Acquires the lock by decrementing x iff (x <= 0)
// Returns 0 on success
CR_INLINE int cri_black_trylock(cri_rb_lock_t	*x)
{
    int old;

    do {
	if ((old = cri_atomic_read(x)) > 0) return 1;
    } while (!cri_cmp_swap(x, old, old - 1));
    return 0;
}

// cri_black_unlock()
//
// Releases the lock by incrementing state
CR_INLINE void cri_black_unlock(cri_rb_lock_t	*x)
{
    cri_atomic_inc(x);
}

#endif /* _CR_RB_LOCK_H */
