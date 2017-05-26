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
 * $Id: cr_atomic.h,v 1.2 2007/05/04 01:03:36 eroman Exp $
 */

#ifndef _CR_ATOMIC_H
#define _CR_ATOMIC_H	1

// Define cri_atomic_t and five required operations:
//   read, write, inc, dec-and-test, compare-and-swap

typedef volatile unsigned int cri_atomic_t;

// Single-word reads are naturally atomic
CR_INLINE unsigned int
cri_atomic_read(cri_atomic_t *p)
{
    __asm__ __volatile__("": : :"memory");
    return *p;
}

// Single-word writes are naturally atomic
CR_INLINE void
cri_atomic_write(cri_atomic_t *p, unsigned int val)
{
    *p = val;
    __asm__ __volatile__("": : :"memory");
}

CR_INLINE unsigned int
cri_atomic_add_fetch(cri_atomic_t *p, int op)
{
    register unsigned int result;
    __asm__ __volatile__ (
		"0:\t"
		"lwarx    %0,0,%2 \n\t"
		"add%I3   %0,%0,%3 \n\t"
		"stwcx.   %0,0,%2 \n\t"
		"bne-     0b\n\t"
		"isync"
		: "=&b"(result), "=m" (p)        /* constraint b = "b"ase register (not r0) */
		: "r" (p), "Ir"(op) , "m"(p)
		: "cr0", "memory");
    return result;
}

CR_INLINE void
cri_atomic_inc(cri_atomic_t *p)
{
    (void)cri_atomic_add_fetch(p, 1);
}

// Returns non-zero if value reaches zero
CR_INLINE int
cri_atomic_dec_and_test(cri_atomic_t *p)
{
    return (cri_atomic_add_fetch(p, -1) == 0);
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
CR_INLINE unsigned int
cri_cmp_swap(cri_atomic_t *p, unsigned int oldval, unsigned int newval)
{
    register unsigned int result;
    __asm__ __volatile__ (
		"0:\t"
		"lwarx    %0,0,%2 \n\t"		/* load to result */
		"xor.     %0,%0,%3 \n\t"	/* xor result w/ oldval */
		"bne      1f\n\t"		/* branch on mismatch */
		"stwcx.   %4,0,%2 \n\t"		/* store newval */
		"bne-     0b\n\t"		/* retry on conflict */
		"1:	  isync"
		: "=&r"(result), "=m"(p)
		: "r" (p), "r"(oldval), "r"(newval), "m"(p)
		: "cr0", "memory");
    return (result == 0);
}

#endif
