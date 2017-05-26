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
 * $Id: cr_atomic.h,v 1.10 2007/02/16 20:23:24 phargrov Exp $
 */

#ifndef _CR_ATOMIC_H
#define _CR_ATOMIC_H	1

// Define cri_atomic_t and five required operations:
//   read, write, inc, dec-and-test, compare-and-swap

typedef volatile unsigned int cri_atomic_t;

// Single-word reads are naturally atomic on Pentium-family
CR_INLINE unsigned int
cri_atomic_read(cri_atomic_t *p)
{
    __asm__ __volatile__("": : :"memory");
    return *p;
}

// Single-word writes are naturally atomic on Pentium-family
CR_INLINE void
cri_atomic_write(cri_atomic_t *p, unsigned int val)
{
    *p = val;
    __asm__ __volatile__("": : :"memory");
}

// Increment requires lock prefix for SMPs
CR_INLINE void
cri_atomic_inc(cri_atomic_t *p)
{
    __asm__ __volatile__ ("lock; incl %0"
			  : "=m" (*p)
			  : "m" (*p)
			  : "cc", "memory");
}

// Dec-and-test requires lock prefix for SMPs
// Returns non-zero if value reaches zero
CR_INLINE int
cri_atomic_dec_and_test(cri_atomic_t *p)
{
    register unsigned char ret;
    
    __asm__ __volatile__ ("lock; decl %0; sete %1"
			  : "=m" (*p), "=qm" (ret)
			  : "m" (*p)
			  : "cc", "memory");
    return ret;
}

// cri_cmp_swap()
//
// Atomic compare and exchange (swap).
// Atomic equivalent of:
//	if (*p == oldval) {
//	    *p = newval;
//	    return NONZERO;
//	} else {
//	    return 0;
//	}
//
// Based on glibc source (LGPL)
// XXX: this will fail on an old i386
CR_INLINE unsigned int
cri_cmp_swap(cri_atomic_t *p, unsigned int oldval, unsigned int newval)
{
    register unsigned char ret;
    register unsigned int readval;

    __asm__ __volatile__ ("lock; cmpxchgl %3, %1; sete %0"
			  : "=q" (ret), "=m" (*p), "=a" (readval)
			  : "r" (newval), "m" (*p), "a" (oldval)
			  : "cc", "memory");
    return ret;
}

#endif
