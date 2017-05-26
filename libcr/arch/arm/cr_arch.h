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
 * $Id: cr_arch.h,v 1.16 2008/05/20 18:15:14 phargrov Exp $
 *
 * Experimental ARM support contributed by Anton V. Uzunov
 * <anton.uzunov@dsto.defence.gov.au> of the Australian Government
 * Department of Defence, Defence Science and Technology Organisation.
 *
 * ARM-specific questions should be directed to blcr-arm@hpcrd.lbl.gov.
 */

#ifndef _CR_ARCH_H
#define _CR_ARCH_H	1

// Catch-all file for misc. arch-specific bits

// macros for using the syscall instruction (SWI)
#if defined(__thumb__) || defined(__ARM_EABI__)
#  define CRI_SYS_REG(name)            register long __sysreg __asm__( "r7" ) = name;
#  define CRI_SYS_REG_LIST(regs...)    "r" (__sysreg) , regs
#  define CRI_SYS_REG_LIST_EABI_0()    "r" (__sysreg)
#  define CRI_syscall(name)            "swi \t 0"
#  define CRI_ld_r7(__num)             "mov	r7, #" #__num
#else
#  define CRI_SYS_REG(name)
#  define CRI_SYS_REG_LIST(regs...)    regs
#  define CRI_SYS_REG_LIST_EABI_0()
#  define CRI_syscall(name)            "swi \t" #name ""
#  define CRI_ld_r7(__num)
#endif

#define cri_syscall_return(type, res, errno_p)				\
  do {									\
    if ((unsigned long) (res) >= (unsigned long) (-4096)) {		\
      if (errno_p != NULL) { *errno_p = -res; }		        	\
      res = -1;						       		\
    }									\
    return( (type) (res) );						\
  } while (0)

#define cri_syscall0(type,name,nr)					\
  type name(int *errno_p) {						\
    CRI_SYS_REG( nr )							\
    register long __res_r0 __asm__( "r0" );				\
    long __res;								\
    __asm__ __volatile__ (						\
      CRI_syscall( nr )		     			   		\
  	: "=r" (__res_r0)						\
        : CRI_SYS_REG_LIST_EABI_0()					\
	: "memory" );							\
    __res = __res_r0;							\
    cri_syscall_return(type,__res,errno_p);				\
  }

#define cri_syscall1(type,name,nr,type1)				\
  type name(type1 arg1, int *errno_p) { 				\
    CRI_SYS_REG( nr )							\
    register long __r0 __asm__( "r0" ) = (long) arg1;			\
    register long __res_r0 __asm__( "r0" );				\
    long __res;					       			\
    __asm__ __volatile__ (						\
      CRI_syscall( nr )				       			\
  	: "=r" (__res_r0)						\
  	: CRI_SYS_REG_LIST( "0" (__r0) )				\
  	: "memory" );							\
    __res = __res_r0;							\
    cri_syscall_return(type,__res,errno_p);				\
  }

#define cri_syscall2(type,name,nr,type1,type2)				\
  type name(type1 arg1,type2 arg2, int *errno_p) {			\
    CRI_SYS_REG( nr )							\
    register long __r0 __asm__( "r0" ) = (long) arg1;			\
    register long __r1 __asm__( "r1" ) = (long) arg2;			\
    register long __res_r0 __asm__( "r0" );				\
    long __res;					       			\
    __asm__ __volatile__ (						\
      CRI_syscall( nr )				       			\
  	: "=r" (__res_r0)						\
  	: CRI_SYS_REG_LIST( "0" (__r0), "r" (__r1) )			\
  	: "memory" );							\
    __res = __res_r0;							\
    cri_syscall_return( type, __res, errno_p );	       			\
  }

#define cri_syscall3(type,name,nr,type1,type2,type3)			\
  type name(type1 arg1,type2 arg2,type3 arg3, int *errno_p) {		\
    CRI_SYS_REG( nr )							\
    register long __r0 __asm__( "r0" ) = (long) arg1;			\
    register long __r1 __asm__( "r1" ) = (long) arg2;			\
    register long __r2 __asm__( "r2" ) = (long) arg3;			\
    register long __res_r0 __asm__( "r0" );				\
    long __res;					       			\
    __asm__ __volatile__ (						\
      CRI_syscall( nr )				       			\
  	: "=r" (__res_r0)						\
  	: CRI_SYS_REG_LIST( "0" (__r0), "r" (__r1), "r" (__r2) )	\
  	: "memory" );							\
    __res = __res_r0;							\
    cri_syscall_return( type, __res, errno_p );	       			\
  }

#define cri_syscall4(type,name,nr,type1,type2,type3,type4)		\
  type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, int *errno_p) { \
    CRI_SYS_REG( nr )							\
    register long __r0 __asm__( "r0" ) = (long) arg1;			\
    register long __r1 __asm__( "r1" ) = (long) arg2;			\
    register long __r2 __asm__( "r2" ) = (long) arg3;			\
    register long __r3 __asm__( "r3" ) = (long) arg4;			\
    register long __res_r0 __asm__( "r0" );				\
    long __res;					       			\
    __asm__ __volatile__ (						\
      CRI_syscall( nr )				       			\
  	: "=r" (__res_r0)						\
  	: CRI_SYS_REG_LIST( "0" (__r0), "r" (__r1), "r" (__r2), "r" (__r3) ) \
  	: "memory" );							\
    __res = __res_r0;							\
    cri_syscall_return( type, __res, errno_p );	       			\
  }

#define cri_syscall5(type,name,nr,type1,type2,type3,type4,type5)	\
  type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, int *errno_p) { \
    CRI_SYS_REG( nr )							\
    register long __r0 __asm__( "r0" ) = (long) arg1;			\
    register long __r1 __asm__( "r1" ) = (long) arg2;			\
    register long __r2 __asm__( "r2" ) = (long) arg3;			\
    register long __r3 __asm__( "r3" ) = (long) arg4;			\
    register long __r4 __asm__( "r4" ) = (long) arg5;			\
    register long __res_r0 __asm__( "r0" );				\
    long __res;					       			\
    __asm__ __volatile__ (						\
      CRI_syscall( nr )				       			\
  	: "=r" (__res_r0)						\
  	: CRI_SYS_REG_LIST( "0" (__r0), "r" (__r1), "r" (__r2),		\
  		  	    "r" (__r3), "r" (__r4) )			\
  	: "memory" );							\
    __res = __res_r0;							\
    cri_syscall_return( type, __res, errno_p );	       			\
  }

#define cri_syscall6(type,name,nr,type1,type2,type3,type4,type5,type6)	\
  type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, int *errno_p) { \
    CRI_SYS_REG( nr )							\
    register long __r0 __asm__( "r0" ) = (long) arg1;			\
    register long __r1 __asm__( "r1" ) = (long) arg2;			\
    register long __r2 __asm__( "r2" ) = (long) arg3;			\
    register long __r3 __asm__( "r3" ) = (long) arg4;			\
    register long __r4 __asm__( "r4" ) = (long) arg5;			\
    register long __r5 __asm__( "r5" ) = (long) arg6;			\
    register long __res_r0 __asm__( "r0" );				\
    long __res;					       			\
    __asm__ __volatile__ (						\
      CRI_syscall( nr )				       			\
  	: "=r" (__res_r0)						\
  	: CRI_SYS_REG_LIST( "0" (__r0), "r" (__r1), "r" (__r2),		\
  			    "r" (__r3), "r" (__r4), "r" (__r5) )	\
  	: "memory" );							\
    __res = __res_r0;							\
    cri_syscall_return( type, __res,errno_p );				\
  }

/* Template to build "micro" handlers */
#define _CR_ASM_HANDLER2(__ioctl,__rt_sigreturn,__name,__op,__arg)		\
  __asm__(									\
"	.text\n"								\
"	.align  2\n"								\
"	.global	" _STRINGIFY(__name) "\n"					\
"	.type	" _STRINGIFY(__name) ", %function\n"				\
_STRINGIFY(__name) ":\n"							\
"	mov	r3, r1\n"							\
"	ldr     r0, [r1, #" _STRINGIFY(CR_ASM_SI_PID_OFFSET) "]\n"		\
"	ldr     r1, =" _STRINGIFY(__op) "\n"					\
"	ldr     r2, =" _STRINGIFY(__arg) "\n"					\
	CRI_ld_r7(__ioctl) "\n\t"						\
	CRI_syscall(__ioctl) "\n"						\
"	mov	sp, r3\n"							\
	CRI_ld_r7(__rt_sigreturn) "\n\t"					\
	CRI_syscall(__rt_sigreturn) "\n"					\
"	.size   " _STRINGIFY(__name) ", .-" _STRINGIFY(__name) 			\
	 );
#define _CR_ASM_HANDLER(__name,__op,__arg) \
	_CR_ASM_HANDLER2(CR_ASM_NR_ioctl, CR_ASM_NR_rt_sigreturn,__name,__op,__arg)

#define CR_RUN_ASM_HANDLER(__name) \
	_CR_ASM_HANDLER(__name, CR_ASM_OP_HAND_CHKPT, CR_ASM_CHECKPOINT_STUB)
#define CR_OMIT_ASM_HANDLER(__name) \
	_CR_ASM_HANDLER(__name, CR_ASM_OP_HAND_ABORT, CR_ASM_CHECKPOINT_OMIT)

#define cri_sigreturn2(__siginfo, __number)				\
    __asm__ __volatile__ (						\
	"mov	sp, %0\n\t"						\
	CRI_ld_r7(__number) "\n\t"					\
	CRI_syscall(__number)						\
	: : "r" ((unsigned long)(__siginfo)));
#define cri_sigreturn(__signr, __siginfo, __context)			\
	cri_sigreturn2(__siginfo, CR_ASM_NR_rt_sigreturn)

// Use our own cri_ksigaction
#define CR_USE_SIGACTION 0

#endif // ifdef _CR_ARCH_H
