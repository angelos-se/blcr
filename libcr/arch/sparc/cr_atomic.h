/* 
 * Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
 * 2008, The Regents of the University of California, through Lawrence
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
 * $Id: cr_atomic.h,v 1.2 2008/12/02 00:17:44 phargrov Exp $
 *
 * Experimental SPARC support contributed to BLCR by Vincentius Robby
 * <vincentius@umich.edu> and Andrea Pellegrini <apellegr@umich.edu>.
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

#if 1
//
// CAS-based add_fetch and cmp_swap - requires V8+ or newer CPU
// By Paul H. Hargrove, from the GASNet atomics (BSD-type license).
//

CR_INLINE unsigned int
__cri_atomic_add_fetch(cri_atomic_t *p, unsigned int op)
{
    register unsigned int oldval, newval;
    __asm__ __volatile__ (
	"membar   #StoreLoad | #LoadLoad\n\t"
	"ld       [%4],%0    \n\t" /* oldval = *addr; */
	"0:                  \t"
	"add      %0,%3,%1   \n\t" /* newval = oldval + op; */
	"cas      [%4],%0,%1 \n\t" /* if (*addr == oldval) SWAP(*addr,newval); else newval = *addr; */
	"cmp      %0, %1     \n\t" /* check if newval == oldval (swap succeeded) */
	"bne,a,pn %%icc, 0b  \n\t" /* otherwise, retry (,pn == predict not taken; ,a == annul) */
	"  mov    %1, %0     \n\t" /* oldval = newval; (branch delay slot, annulled if not taken) */
	"membar   #StoreLoad | #StoreStore"
	: "=&r"(oldval), "=&r"(newval), "=m"(*p)
	: "rn"(op), "r"(p), "m"(*p) );
    return newval;
}

CR_INLINE unsigned int
cri_cmp_swap(cri_atomic_t *p, unsigned int oldval, unsigned int newval)
{
    __asm__ __volatile__ (
	/* if (*p == oldval) SWAP(*p,newval); else newval = *p; */
	"membar   #StoreLoad | #LoadLoad   \n\t"
	"cas      [%3], %2, %0             \n\t"
	"membar   #StoreLoad | #StoreStore"
	: "+r"(newval), "=m"(*p)
	: "r"(oldval), "r"(p), "m"(*p) );
    return (int)(newval == oldval); 
}

#else
//
// LDSTUB-based add_fetch and cmp_swap - not signal safe
//

// XXX: Should automatically enable membar by target CPU type
#if 0
  #define __CRI_MEMBAR(_arg) "membar " _arg
#else
  #define __CRI_MEMBAR(_arg) ""
#endif

/* "Tentative declaration" (aka "common") to ensure one instance
 * of the lock variable will be shared across all linked objects.
 * DO NOT add "extern".
 * DO NOT add an initializer.
 */
unsigned char __cri_atomic_lock_var;

/* Macros to do the lock/unlock.
 * Passing the atomic var as an argument will ease
 * any future transition to a lock array.
 */
#define __cri_atomic_lock(_p) do {                                 \
    register unsigned int _lock_tmp;                               \
    do { /* Try lock */                                            \
      __asm__ __volatile__("ldstub  [%1], %0\n\t"                  \
                           __CRI_MEMBAR("#StoreLoad | #StoreStore")\
                           : "=&r" (_lock_tmp)                     \
                           : "r" (&__cri_atomic_lock_var)          \
                           : "memory");                            \
    } while (_lock_tmp);                                           \
  } while (0)
#define __cri_atomic_unlock(_p)                                    \
  __asm__ __volatile__(__CRI_MEMBAR("#StoreStore | #LoadStore\n\t")\
                       "stb     %%g0, [%0]"                        \
                       : /* no outputs */                          \
                       : "r" (&__cri_atomic_lock_var)              \
                       : "memory")

CR_INLINE unsigned int
__cri_atomic_add_fetch(cri_atomic_t *p, int op)
{
  unsigned int retval;

  __cri_atomic_lock(p);

  retval = *p + op;
  *p = retval;

  __cri_atomic_unlock(p);

  return retval;
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
  unsigned int retval;

  __cri_atomic_lock(p);

  if (*p == oldval) {
    *p = newval;
    retval = 1;
  } else {
    retval = 0;
  }

  __cri_atomic_unlock(p);

  return retval;
}

#endif


CR_INLINE void
cri_atomic_inc(cri_atomic_t *p)
{
    (void)__cri_atomic_add_fetch(p, 1);
}

CR_INLINE int
cri_atomic_dec_and_test(cri_atomic_t *p)
{
    return (__cri_atomic_add_fetch(p, -1) == 0);
}

#endif
