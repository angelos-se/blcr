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
 * $Id: cr_arch.h,v 1.23 2008/05/20 18:15:16 phargrov Exp $
 */

#ifndef _CR_ARCH_H
#define _CR_ARCH_H	1

// Catch-all file for misc. arch-specific bits

#ifdef PIC
  #define CRI_SYSCALL_LD_ARG1 \
	"\tmovl %2,%%ebx\n"
  #define CRI_SYSCALL_PRE \
	"\tpushl %%ebx\n"				\
	CRI_SYSCALL_LD_ARG1
  #define CRI_SYSCALL_POST \
	"\tpopl %%ebx\n"
  #define CRI_SYSCALL_ARG1 "r"
#else
  #define CRI_SYSCALL_LD_ARG1
  #define CRI_SYSCALL_PRE
  #define CRI_SYSCALL_POST
  #define CRI_SYSCALL_ARG1 "b"
#endif


#define cri_syscall_cleanup(res,errno_p) \
    __asm__ volatile ("" ::: "memory", "cc", "cx", "dx");\
    if ((unsigned long)res >= (unsigned long)(-4096)) {	\
	if (errno_p != NULL) { *errno_p = -res; }	\
	res = -1;					\
    }

// XXX: does syscall5 work w/ PIC?

#define cri_syscall5(type,name,nr,type1,type2,type3,type4,type5)\
  type name(type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5,int *errno_p) {\
      long _res;					\
      __asm__ volatile (				\
	CRI_SYSCALL_PRE					\
	"\tint $0x80\n"					\
	CRI_SYSCALL_POST				\
        : "=a" (_res)					\
        : "0" (nr),					\
	  CRI_SYSCALL_ARG1 ((long)(arg1)),		\
	  "c" ((long)(arg2)),				\
	  "d" ((long)(arg3)),				\
	  "S" ((long)(arg4)),				\
	  "D" ((long)(arg5)));				\
      cri_syscall_cleanup(_res, errno_p);		\
      return (type)_res;				\
  }

#define cri_syscall4(type,name,nr,type1,type2,type3,type4)\
  type name(type1 arg1,type2 arg2,type3 arg3,type4 arg4,int *errno_p) {\
      long _res;					\
      __asm__ volatile (				\
	CRI_SYSCALL_PRE					\
	"\tint $0x80\n"					\
	CRI_SYSCALL_POST				\
        : "=a" (_res)					\
        : "0" (nr),					\
	  CRI_SYSCALL_ARG1 ((long)(arg1)),		\
	  "c" ((long)(arg2)),				\
	  "d" ((long)(arg3)),				\
	  "S" ((long)(arg4)));				\
      cri_syscall_cleanup(_res, errno_p);		\
      return (type)_res;				\
  }

#define cri_syscall3(type,name,nr,type1,type2,type3)\
  type name(type1 arg1,type2 arg2,type3 arg3,int *errno_p) {\
      long _res;					\
      __asm__ volatile (				\
	CRI_SYSCALL_PRE					\
	"\tint $0x80\n"					\
	CRI_SYSCALL_POST				\
        : "=a" (_res)					\
        : "0" (nr),					\
	  CRI_SYSCALL_ARG1 ((long)(arg1)),		\
	  "c" ((long)(arg2)),				\
	  "d" ((long)(arg3)));				\
      cri_syscall_cleanup(_res, errno_p);		\
      return (type)_res;				\
  }

#define cri_syscall2(type,name,nr,type1,type2)\
  type name(type1 arg1,type2 arg2,int *errno_p) {\
      long _res;					\
      __asm__ volatile (				\
	CRI_SYSCALL_PRE					\
	"\tint $0x80\n"					\
	CRI_SYSCALL_POST				\
        : "=a" (_res)					\
        : "0" (nr),					\
	  CRI_SYSCALL_ARG1 ((long)(arg1)),		\
	  "c" ((long)(arg2)));				\
      cri_syscall_cleanup(_res, errno_p);		\
      return (type)_res;				\
  }

#define cri_syscall1(type,name,nr,type1)\
  type name(type1 arg1,int *errno_p) {\
      long _res;					\
      __asm__ volatile (				\
	CRI_SYSCALL_PRE					\
	"\tint $0x80\n"					\
	CRI_SYSCALL_POST				\
        : "=a" (_res)					\
        : "0" (nr),					\
	  CRI_SYSCALL_ARG1 ((long)(arg1)));		\
      cri_syscall_cleanup(_res, errno_p);		\
      return (type)_res;				\
  }

#define cri_syscall0(type,name,nr)\
  type name(int *errno_p) {\
      long _res;					\
      __asm__ volatile (				\
	"\tint $0x80\n"					\
        : "=a" (_res)					\
        : "0" (nr));					\
      cri_syscall_cleanup(_res, errno_p);		\
      return (type)_res;				\
  }

/* On an x86_64 the registers ebx and ebp are callee-saved.
 * Currently we lack a way to know this is a 64-bit CPU.
 * So, we do this unconditionally.
 */
#define cri_syscall3X(type,name,nr,type1,type2,type3)\
  type name(type1 arg1,type2 arg2,type3 arg3,int *errno_p) {\
      long _res;					\
      __asm__ volatile (				\
	"\tpushl %%ebp\n"				\
	"\tpushl %%ebx\n"				\
	CRI_SYSCALL_LD_ARG1				\
	"\tint $0x80\n"					\
	"\tpopl %%ebx\n"				\
	"\tpopl %%ebp\n"				\
        : "=a" (_res)					\
        : "0" (nr),					\
	  CRI_SYSCALL_ARG1 ((long)(arg1)),		\
	  "c" ((long)(arg2)),				\
	  "d" ((long)(arg3)));				\
      cri_syscall_cleanup(_res, errno_p);		\
      return (type)_res;				\
  }

#define cri_sigreturn(__signr, __siginfo, __context)	\
    __asm__ __volatile__ (				\
        "mov    %0,%%esp\n\t"				\
        "mov    %1,%%eax\n\t"				\
        "int    $0x80"					\
        : : "r" (&(__signr)), "i" (CR_ASM_NR_rt_sigreturn))

/* Template to build the "micro" handlers */
#define _CR_ASM_HANDLER(_name,_op,_arg) \
__asm__("	.text\n"\
	"	.align 16\n"\
	".globl	" _STRINGIFY(_name) "\n"\
	"	.type	" _STRINGIFY(_name) ",@function\n"\
	_STRINGIFY(_name) ":\n"\
	"	mov	8(%esp),%ebx					# B = siginfo (2nd arg)\n"\
	"	mov	" _STRINGIFY(CR_ASM_SI_PID_OFFSET) "(%ebx),%ebx	# B = siginfo->si_pid\n"\
	"	mov	$" _STRINGIFY(CR_ASM_NR_ioctl) ", %eax		# A = __NR_ioctl\n"\
	"	mov	$" _STRINGIFY(_op) ", %ecx			# C = _op\n"\
	"	mov	$" _STRINGIFY(_arg) ", %edx			# D = _arg\n"\
	"	int	$0x80						# syscall\n"\
	"	pop	%ebx						# pop return addr\n"\
	"	mov	$" _STRINGIFY(CR_ASM_NR_rt_sigreturn) ", %eax	# A = __NR_rt_sigreturn\n"\
	"	int	$0x80						# syscall\n"\
	"	.size	" _STRINGIFY(_name) ",.-" _STRINGIFY(_name) "\n"\
	);

/* "micro" handler for libcr_run.so */
#define CR_RUN_ASM_HANDLER(_name) \
	_CR_ASM_HANDLER(_name, CR_ASM_OP_HAND_CHKPT, CR_ASM_CHECKPOINT_STUB)

/* "micro" handler for libcr_omit.so */
#define CR_OMIT_ASM_HANDLER(_name) \
	_CR_ASM_HANDLER(_name, CR_ASM_OP_HAND_ABORT, CR_ASM_CHECKPOINT_OMIT)

#endif
