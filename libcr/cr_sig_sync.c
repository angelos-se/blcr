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
 * $Id: cr_sig_sync.c,v 1.10 2004/12/10 20:55:05 phargrov Exp $
 *
 *
 * cr_sig_sync.c: routines for signal-safe synchronization
 */

#include "cr_private.h"

#define CRI_SL_DEBUG	0

#define CRI_SL_LOCKED	0x27182818U	/* decimal digits of e */
#define CRI_SL_UNLOCKED	CR_SPINLOCK_INITIALIZER

void cr_spinlock_init(cr_spinlock_t *x)
{
    cri_atomic_t *p = (cri_atomic_t *)x;

    cri_atomic_write(p, CRI_SL_UNLOCKED);
}

void cr_spinlock_lock(cr_spinlock_t *x)
{
    cri_atomic_t *p = (cri_atomic_t *)x;

    if (!cri_cmp_swap(p, CRI_SL_UNLOCKED, CRI_SL_LOCKED)) {
#if CRI_SL_DEBUG
	cri_atomic_t tmp = cri_atomic_read(p);

	if ((tmp != CRI_SL_LOCKED) && (tmp != CRI_SL_UNLOCKED)) {
	    CRI_ABORT("Spinlock %p has invalid state %x", p, tmp);
	}
#endif

	CRI_DO_YIELD_WHILE_COND(!cri_cmp_swap(p, CRI_SL_UNLOCKED, CRI_SL_LOCKED));
    }
}

void cr_spinlock_unlock(cr_spinlock_t *x)
{
    cri_atomic_t *p = (cri_atomic_t *)x;

#if CRI_SL_DEBUG
    if (!cri_cmp_swap(p, CRI_SL_LOCKED, CRI_SL_UNLOCKED)) {
	cri_atomic_t tmp = cri_atomic_read(p);

	if (tmp == CRI_SL_UNLOCKED) {
	    CRI_ABORT("Spinlock %p is not locked", p);
	} else {
	    CRI_ABORT("Spinlock %p has invalid state %x", p, tmp);
	}
    }
#else
    cri_atomic_write(p, CRI_SL_UNLOCKED);
#endif
}

int cr_spinlock_trylock(cr_spinlock_t *x)
{
    cri_atomic_t *p = (cri_atomic_t *)x;
    int retval;

    retval = cri_cmp_swap(p, CRI_SL_UNLOCKED, CRI_SL_LOCKED);

#if CRI_SL_DEBUG
    if (!retval) {
	cri_atomic_t tmp = cri_atomic_read(p);

	if ((tmp != CRI_SL_LOCKED) && (tmp != CRI_SL_UNLOCKED)) {
	    CRI_ABORT("Spinlock %p has invalid state %x", p, tmp);
	}
    }
#endif

    return retval;
}

int cri_barrier_enter(cri_atomic_t *p)
{
    int retval;

    retval = cri_atomic_dec_and_test(p);
    if (!retval) {
	CRI_DO_YIELD_WHILE_COND(cri_atomic_read(p));
    }

    return retval;
}
