/* 
 * Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
 * 2007, The Regents of the University of California, through Lawrence
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
 * $Id: cr_atomic.h,v 1.5.8.1 2012/12/22 00:14:29 phargrov Exp $
 *
 * Experimental ARM support contributed by Anton V. Uzunov
 * <anton.uzunov@dsto.defence.gov.au> of the Australian Government
 * Department of Defence, Defence Science and Technology Organisation.
 *
 * ARM-specific questions should be directed to blcr-arm@hpcrd.lbl.gov.
 */

#ifndef _CR_ATOMIC_H
#define _CR_ATOMIC_H	1

#if defined(__ARM_ARCH_2__) || defined(__ARM_ARCH_3__)
  // Sanity-check that we're not building on a really old architecture,
  // so that the using #ifdef __ARM_ARCH_4__ works to test for
  // lack of blx <register> support.
  #error "ARM Architecture versions prior to ARMv4 not supported."
#elif defined(__ARM_ARCH_4T__) && defined(__thumb__)
  // The inline asm is not compatible with Thumb-1 anyway, but in particular
  // we assume later that if __ARM_ARCH_4__ is not defined, we have ARMv5
  // or above.  Ensure here that this assumption will be valid.
  #error "Building for Thumb on ARMv4 is not supported."
#endif

// Determine whether to use BLX <register> for function calls to
// computed addresses:
#undef ARM_HAVE_BLX_REG
#if !(defined(__ARM_ARCH_4__) || defined(__ARM_ARCH_4T__))
  #define ARM_HAVE_BLX_REG 1
#endif

#include "blcr_config.h"

#ifndef _STRINGIFY
  #define _STRINGIFY_HELPER(x) #x
  #define _STRINGIFY(x) _STRINGIFY_HELPER(x)
#endif

// Define cri_atomic_t and five required operations:
//   read, write, inc, dec-and-test, compare-and-swap

typedef volatile unsigned int cri_atomic_t;

// Single-word reads are naturally atomic
CR_INLINE unsigned int
cri_atomic_read(cri_atomic_t *p)
{
    __asm__ __volatile__("": : :"memory");
    return( *p );
}

// Single-word writes are naturally atomic
CR_INLINE void
cri_atomic_write(cri_atomic_t *p, unsigned int val)
{
    *p = val;
    __asm__ __volatile__("": : :"memory");
}

// Was '#if defined(CR_KCODE___kuser_cmpxchg), but that prevented separate user/kerel builds.
#if 1
// For kernel >= 2.6.12, we use __kernel_cmpxchg()
//    See linux-2.6.12/arch/arm/kernel/entry-armv.S
// For >= ARM6 we could/should be using load-exclusive directly.

// To construct constants from (8-bit immediates + shifts)
// we use a "base" that fits that constraint, and also lies a
// distance from __kuser_cmpxchg fitting that constraint.
// Specifically 0xffff0fff = ~(0xf0 << 8) = __kuser_cmpxchg + 0x3f
#define cri_kuser_cmpxchg	0xffff0fc0
#define cri_kuser_base		0xffff0fff
#define cri_kuser_offset	(cri_kuser_base - cri_kuser_cmpxchg)


CR_INLINE unsigned int
__cri_atomic_add_fetch(cri_atomic_t *p, unsigned int op)
{
    register unsigned long __sum asm("r1");
    register unsigned long __ptr asm("r2") = (unsigned long)(p);

    __asm__ __volatile__ (
	"0:	ldr	r0, [r2]	@ r0 = *p		\n"
	"	add	r1, r0, %2	@ r1 = r0 + op		\n"
	"	mov	r3, #" _STRINGIFY(cri_kuser_base) "	\n"
#ifdef ARM_HAVE_BLX_REG
	"	sub	r3, r3, #" _STRINGIFY(cri_kuser_offset) "\n"
	"	blx	r3\n"
#else // ARMv4T and below
	"	adr	lr, 1f		@ lr = return address	\n"
	"	sub	pc, r3, #" _STRINGIFY(cri_kuser_offset) "\n"
#endif
	"1:	bcc     0b		@ retry on Carry Clear"
	: "=&r" (__sum)
	: "r" (__ptr), "rIL" (op)
	: "r0", "r3", "ip", "lr", "cc", "memory" );

    return __sum;
}

CR_INLINE void
cri_atomic_inc(cri_atomic_t *p)
{
    (void)__cri_atomic_add_fetch(p, 1);
}

// Returns non-zero if value reaches zero
CR_INLINE int
cri_atomic_dec_and_test(cri_atomic_t *p)
{
    return (__cri_atomic_add_fetch(p, -1) == 0);
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
CR_INLINE unsigned int
cri_cmp_swap(cri_atomic_t *p, unsigned int oldval, unsigned int newval)
{
    register unsigned int result asm("r0");
    register unsigned int _newval asm("r1") = newval;
    register unsigned int _p asm("r2") = (unsigned long)p;
    register unsigned int _oldval asm("r4") = oldval;

    /* Transient failure is possible if interrupted.
     * Since we can't distinguish the cause of the failure,
     * we must retry as long as the failure looks "improper"
     * which is defined as (!swapped && (*p == oldval))
     */
    __asm__ __volatile__ (
	"0:     mov     r0, r4          @ r0 = oldval           \n"
	"	mov	r3, #" _STRINGIFY(cri_kuser_base) "	\n"
#ifdef ARM_HAVE_BLX_REG
	"	sub	r3, r3, #" _STRINGIFY(cri_kuser_offset) "\n"
	"	blx	r3\n"
#else // ARMv4T and below
	"	adr	lr, 1f		@ lr = return addr	\n"
	"	sub	pc, r3, #" _STRINGIFY(cri_kuser_offset) "\n"
#endif
	"1:	        \n"
	"       ite cc                  @ needed in Thumb2 mode \n"
	"       ldrcc   ip, [r2]        @ if (!swapped) ip=*p   \n"
	"       eorcs   ip, r4, #1      @ else ip=oldval^1      \n"
	"       teq     r4, ip          @ if (ip == oldval)     \n"
	"       beq     0b              @    then retry           "
	: "=&r" (result)
	: "r" (_oldval), "r" (_p), "r" (_newval)
	: "r3", "ip", "lr", "cc", "memory" );

    return !result;
}

#else

// The remainder of this file is based on various glibc and
// linuxthreads revisions, and on the following post by
// Daniel Jacobowitz to the libc-ports mailing list:
//   http://sourceware.org/ml/libc-ports/2005-10/msg00016.html
// As Daniel says:
// <Quote>
/* Atomic compare and exchange.  These sequences are not actually atomic;
   there is a race if *MEM != OLDVAL and we are preempted between the two
   swaps.  However, they are very close to atomic, and are the best that a
   pre-ARMv6 implementation can do without operating system support.
   LinuxThreads has been using these sequences for many years.  */
// </Quote>
// That is true of our inc and dec-and-test as well.
//
// All of this will fail on ARM1 and ARM2, where SWP is missing.
//
// For kernel >= 2.6.12, we use __kernel_cmpxchg() (see above)

CR_INLINE void
cri_atomic_inc(cri_atomic_t *p)
{
    unsigned int tmp1;
    unsigned int tmp2;
    unsigned int tmp3;

    __asm__ __volatile__ ("\n"
		"0:	ldr	%0,[%3]\n"	// tmp1 = *p
		"	add	%1,%0,#1\n"	// tmp2 = tmp1 + 1
		"	swp	%2,%1,[%3]\n"	// atomically(tmp3 = *p; *p = tmp2)
		"	cmp	%0,%2\n"	// compare(tmp1, tmp3)
		"	swpne	%1,%2,[%3]\n"	// if (tmp1 != tmp3) undo the swap...
		"	bne	0b"		//     ... and restart
		: "=&r" (tmp1), "=&r" (tmp2), "=&r" (tmp3)
		: "r" (p)
		: "cc", "memory");
    return;
}

// Returns non-zero if value reaches zero
CR_INLINE int
cri_atomic_dec_and_test(cri_atomic_t *p)
{
    unsigned int tmp1;
    unsigned int tmp2;
    unsigned int tmp3;

    __asm__ __volatile__ ("\n"
		"0:	ldr	%0,[%3]\n"	// tmp1 = *p
		"	sub	%1,%0,#1\n"	// tmp2 = tmp1 - 1
		"	swp	%2,%1,[%3]\n"	// atomically(tmp3 = *p; *p = tmp2)
		"	cmp	%0,%2\n"	// compare(tmp1, tmp3)
		"	swpne	%1,%2,[%3]\n"	// if (tmp1 != tmp3) undo the swap...
		"	bne	0b"		//     ... and restart
		: "=&r" (tmp1), "=&r" (tmp2), "=&r" (tmp3)
		: "r" (p)
		: "cc", "memory");
    return( tmp2 == 0 );
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
CR_INLINE unsigned int
cri_cmp_swap(cri_atomic_t *p, unsigned int oldval, unsigned int newval)
{
    int readval, tmp;
    __asm__ __volatile__ ("\n"
		"0:	ldr	%1,[%2]\n"	// tmp = *p
		"	cmp	%1,%4\n"	// compare(tmp, oldval)
		"	movne	%0,%1\n"	// if (tmp != oldval) readval = tmp ...
		"	bne	1f\n"		//     ... and return
		"	swp	%0,%3,[%2]\n"	// atomically(readval = *p; *p = newval)
		"	cmp	%1,%0\n"	// compare(tmp, readval)  NOTE: tmp==oldval
		"	swpne	%1,%0,[%2]\n"	// if (tmp != readval) undo the swap...
		"	bne	0b\n"		//     ... and restart
		"1:"
		: "=&r" (readval), "=&r" (tmp)
		: "r" (p), "r" (newval), "r" (oldval)
		: "cc", "memory");					      
    return( readval == oldval );
}
#endif /* defined(CR_KCODE___kuser_cmpxchg) */

#endif
