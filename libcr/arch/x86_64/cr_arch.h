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
 * $Id: cr_arch.h,v 1.23 2008/09/06 00:33:18 phargrov Exp $
 */

#ifndef _CR_ARCH_H
#define _CR_ARCH_H	1

// Catch-all file for misc. arch-specific bits

#define CRI_CLOBBERS "cc", "r11", "rcx", "memory"
#define CRI_CLOBBERS_6 CRI_CLOBBERS, "r8", "r9", "r10"
#define CRI_CLOBBERS_5 CRI_CLOBBERS, "r8", "r10"
#define CRI_CLOBBERS_4 CRI_CLOBBERS, "r10"
#define CRI_CLOBBERS_3 CRI_CLOBBERS
#define CRI_CLOBBERS_2 CRI_CLOBBERS
#define CRI_CLOBBERS_1 CRI_CLOBBERS
#define CRI_CLOBBERS_0 CRI_CLOBBERS

#define cri_syscall_cleanup(res,errno_p) \
    if ((unsigned long)res >= (unsigned long)(-4096)) {	\
	if (errno_p != NULL) { *errno_p = -res; }	\
	res = -1;					\
    }

#define cri_syscall6(type,name,nr,type1,type2,type3,type4,type5,type6)\
  type name(type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5,type6 arg6,int *errno_p) {\
      long _res;					\
      __asm__ volatile (				\
	"\tmovq %5,%%r10\n"				\
	"\tmovq %6,%%r8\n"				\
	"\tmovq %7,%%r9\n"				\
	"\tsyscall\n"					\
        : "=a" (_res)					\
        : "0" (nr),					\
	  "D" ((long)(arg1)),				\
	  "S" ((long)(arg2)),				\
	  "d" ((long)(arg3)),				\
	  "g" ((long)(arg4)),				\
	  "g" ((long)(arg5)),				\
	  "g" ((long)(arg6))				\
	: CRI_CLOBBERS_6);				\
      cri_syscall_cleanup(_res, errno_p);		\
      return (type)_res;				\
  }

#define cri_syscall5(type,name,nr,type1,type2,type3,type4,type5)\
  type name(type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5,int *errno_p) {\
      long _res;					\
      __asm__ volatile (				\
	"\tmovq %5,%%r10\n"				\
	"\tmovq %6,%%r8\n"				\
	"\tsyscall\n"					\
        : "=a" (_res)					\
        : "0" (nr),					\
	  "D" ((long)(arg1)),				\
	  "S" ((long)(arg2)),				\
	  "d" ((long)(arg3)),				\
	  "g" ((long)(arg4)),				\
	  "g" ((long)(arg5))				\
	: CRI_CLOBBERS_5);				\
      cri_syscall_cleanup(_res, errno_p);		\
      return (type)_res;				\
  }

#define cri_syscall4(type,name,nr,type1,type2,type3,type4)\
  type name(type1 arg1,type2 arg2,type3 arg3,type4 arg4,int *errno_p) {\
      long _res;					\
      __asm__ volatile (				\
	"\tmovq %5,%%r10\n"				\
	"\tsyscall\n"					\
        : "=a" (_res)					\
        : "0" (nr),					\
	  "D" ((long)(arg1)),				\
	  "S" ((long)(arg2)),				\
	  "d" ((long)(arg3)),				\
	  "g" ((long)(arg4))				\
	: CRI_CLOBBERS_4);				\
      cri_syscall_cleanup(_res, errno_p);		\
      return (type)_res;				\
  }

#define cri_syscall3(type,name,nr,type1,type2,type3) \
  type name(type1 arg1,type2 arg2,type3 arg3,int *errno_p) {\
      long _res;					\
      __asm__ volatile (				\
	"\tsyscall\n"					\
        : "=a" (_res)					\
        : "0" (nr),					\
	  "D" ((long)(arg1)),				\
	  "S" ((long)(arg2)),				\
	  "d" ((long)(arg3))				\
	: CRI_CLOBBERS_3);				\
      cri_syscall_cleanup(_res, errno_p);		\
      return (type)_res;				\
  }

#define cri_syscall2(type,name,nr,type1,type2) \
  type name(type1 arg1,type2 arg2,int *errno_p) {	\
      long _res;					\
      __asm__ volatile (				\
	"\tsyscall\n"					\
        : "=a" (_res)					\
        : "0" (nr),					\
	  "D" ((long)(arg1)),				\
	  "S" ((long)(arg2))				\
	: CRI_CLOBBERS_2);				\
      cri_syscall_cleanup(_res, errno_p);		\
      return (type)_res;				\
  }

#define cri_syscall1(type,name,nr,type1) \
  type name(type1 arg1,int *errno_p) {			\
      long _res;					\
      __asm__ volatile (				\
	"\tsyscall\n"					\
        : "=a" (_res)					\
        : "0" (nr),					\
	  "D" ((long)(arg1))				\
	: CRI_CLOBBERS_1);				\
      cri_syscall_cleanup(_res, errno_p);		\
      return (type)_res;				\
  }

#define cri_syscall0(type,name,nr) \
  type name(int *errno_p) {				\
      long _res;					\
      __asm__ volatile (				\
	"\tsyscall\n"					\
        : "=a" (_res)					\
        : "0" (nr)					\
	: CRI_CLOBBERS_0);				\
      cri_syscall_cleanup(_res, errno_p);		\
      return (type)_res;				\
  }

/* 
 * Because we enter the kernel via a "normal" syscall, the kernel
 * will not save the caller-saved rbp, rbx or r12-15 in the ptregs
 * struct (they are individually spilled as needed).
 * Thus vmadump can't restore them correctly at restart.
 * We work around that here telling gcc they are clobbered.
 */
#define CRI_CLOBBERS_3X CRI_CLOBBERS, "rbx", "rbp", "r12", "r13", "r14", "r15"
#define cri_syscall3X(type,name,nr,type1,type2,type3) \
  type name(type1 arg1,type2 arg2,type3 arg3,int *errno_p) {\
      long _res;					\
      __asm__ volatile (				\
	"\tsyscall\n"					\
        : "=a" (_res)					\
        : "0" (nr),					\
	  "D" ((long)(arg1)),				\
	  "S" ((long)(arg2)),				\
	  "d" ((long)(arg3))				\
	: CRI_CLOBBERS_3X);				\
      cri_syscall_cleanup(_res, errno_p);		\
      return (type)_res;				\
  }

#if !CR_USE_SIGACTION
  /* signal return trampoline, from glibc-2.3.2 */
  /* We only need this when not calling sigaction() directly;
   * and then only because x86_64 kernels *require* that the
   * SA_RESTORER bit be set in sa_flags.
   */
  #include <asm/unistd.h>
  #ifndef __NR_rt_sigreturn
    #error "no value for __NR_rt_sigreturn"
  #endif
  #define CRI_SA_RESTORER2(name, syscall) \
    __asm__ (                             \
      ".text\n"                         \
      ".align 16\n"                     \
      #name ":\n"                       \
      "    movq $" _STRINGIFY(syscall) ", %rax\n"  \
      "    syscall\n"                   \
    );                                    \
    static void cri_sa_restorer(void) __asm__(#name);
  #define CRI_SA_RESTORER CRI_SA_RESTORER2(__restore_rt, __NR_rt_sigreturn)
  #ifndef SA_RESTORER
    #define SA_RESTORER 0x04000000
  #endif
#endif /* !CR_USE_SIGACTION */

#define cri_sigreturn(__signr, __siginfo, __context) \
    __asm__ __volatile__ (				\
        "mov    %0,%%rsp\n\t"				\
        "mov    %1,%%rax\n\t"				\
        "syscall"					\
        : : "r" (__context), "i" (CR_ASM_NR_rt_sigreturn))

/* Template to build the "micro" handlers */
#define _CR_ASM_HANDLER(_name, _op, _arg) \
__asm__("	.text\n"\
	"	.align 16\n"\
	".globl	" _STRINGIFY(_name) "\n"\
	"	.type	" _STRINGIFY(_name) ",@function\n"\
	"" _STRINGIFY(_name) ":\n"\
	"	mov	" _STRINGIFY(CR_ASM_SI_PID_OFFSET) "(%rsi),%rdi	# di = siginfo->si_pid\n"\
	"	mov	$" _STRINGIFY(CR_ASM_NR_ioctl) ", %rax		# ax = __NR_ioctl\n"\
	"	mov	$" _STRINGIFY(_op) ", %rsi			# si = _op\n"\
	"	mov	$" _STRINGIFY(_arg) ", %rdx			# dx = _arg\n"\
	"	syscall							# syscall\n"\
	"	pop	%rdi						# pop return addr\n"\
	"	mov	$" _STRINGIFY(CR_ASM_NR_rt_sigreturn) ", %rax	# ax = __NR_rt_sigreturn\n"\
	"	syscall							# syscall\n"\
	"	.size	" _STRINGIFY(_name) ",.-" _STRINGIFY(_name) "\n"\
	);

/* "micro" handler for libcr_run.so */
#define CR_RUN_ASM_HANDLER(_name) \
	_CR_ASM_HANDLER(_name, CR_ASM_OP_HAND_CHKPT, CR_ASM_CHECKPOINT_STUB)

/* "micro" handler for libcr_omit.so */
#define CR_OMIT_ASM_HANDLER(_name) \
	_CR_ASM_HANDLER(_name, CR_ASM_OP_HAND_ABORT, CR_ASM_CHECKPOINT_OMIT)

#endif
