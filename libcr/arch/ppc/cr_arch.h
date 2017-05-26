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
 * $Id: cr_arch.h,v 1.15.26.1 2009/02/27 08:36:22 phargrov Exp $
 */

#ifndef _CR_ARCH_H
#define _CR_ARCH_H	1

// Catch-all file for misc. arch-specific bits

// syscall macros from glibc (LGPL)

/* On powerpc a system call basically clobbers the same registers like a
 * function call, with the exception of LR (which is needed for the
 * "sc; bnslr" sequence) and CR (where only CR0.SO is clobbered to signal
 * an error return status).
 *
 * Note we are fudging on the errno return, assuming (like x86) that anything
 * lower than -4096 is an error.  This is to allow for our extended errno
 * values while the kernel checks against LAST_ERRNO when setting r3.
 */

#define cri_syscall_nr(nr, type, num, errno_p, args...)			\
	unsigned long cri_sc_ret, cri_sc_err;				\
	{								\
		register unsigned long cri_sc_0  __asm__ ("r0");	\
		register unsigned long cri_sc_3  __asm__ ("r3");	\
		register unsigned long cri_sc_4  __asm__ ("r4");	\
		register unsigned long cri_sc_5  __asm__ ("r5");	\
		register unsigned long cri_sc_6  __asm__ ("r6");	\
		register unsigned long cri_sc_7  __asm__ ("r7");	\
		register unsigned long cri_sc_8  __asm__ ("r8");	\
									\
		cri_sc_loadargs_##nr(num, args);			\
		__asm__ __volatile__					\
			(						\
			 "sc           \n\t"				\
			 "mfcr %0      \n\t"				\
			: "=&r" (cri_sc_0),				\
			  "=&r" (cri_sc_3),  "=&r" (cri_sc_4),		\
			  "=&r" (cri_sc_5),  "=&r" (cri_sc_6),		\
			  "=&r" (cri_sc_7),  "=&r" (cri_sc_8)		\
			: cri_sc_asm_input_##nr				\
			: "cr0", "ctr", "memory",			\
			        "r9", "r10","r11", "r12");		\
		cri_sc_ret = cri_sc_3;					\
		cri_sc_err = cri_sc_0;					\
	}								\
	if (cri_sc_err & 0x10000000)					\
	{								\
		if (errno_p) *errno_p = cri_sc_ret;			\
		cri_sc_ret = -1;					\
	}								\
	else if (cri_sc_ret >= (unsigned long)(-4096))			\
	{								\
		if (errno_p) *errno_p = -cri_sc_ret;			\
		cri_sc_ret = -1;					\
	}								\
	return (type) cri_sc_ret

#define cri_sc_loadargs_0(num, dummy...)				\
	cri_sc_0 = num
#define cri_sc_loadargs_1(num, arg1)					\
	cri_sc_loadargs_0(num);						\
	cri_sc_3 = (unsigned long) (arg1)
#define cri_sc_loadargs_2(num, arg1, arg2)				\
	cri_sc_loadargs_1(num, arg1);					\
	cri_sc_4 = (unsigned long) (arg2)
#define cri_sc_loadargs_3(num, arg1, arg2, arg3)			\
	cri_sc_loadargs_2(num, arg1, arg2);				\
	cri_sc_5 = (unsigned long) (arg3)
#define cri_sc_loadargs_4(num, arg1, arg2, arg3, arg4)			\
	cri_sc_loadargs_3(num, arg1, arg2, arg3);			\
	cri_sc_6 = (unsigned long) (arg4)
#define cri_sc_loadargs_5(num, arg1, arg2, arg3, arg4, arg5)		\
	cri_sc_loadargs_4(num, arg1, arg2, arg3, arg4);			\
	cri_sc_7 = (unsigned long) (arg5)
#define cri_sc_loadargs_6(num, arg1, arg2, arg3, arg4, arg5, arg6)	\
	cri_sc_loadargs_5(num, arg1, arg2, arg3, arg4, arg5);		\
	cri_sc_8 = (unsigned long) (arg6)

#define cri_sc_asm_input_0 "0" (cri_sc_0)
#define cri_sc_asm_input_1 cri_sc_asm_input_0, "1" (cri_sc_3)
#define cri_sc_asm_input_2 cri_sc_asm_input_1, "2" (cri_sc_4)
#define cri_sc_asm_input_3 cri_sc_asm_input_2, "3" (cri_sc_5)
#define cri_sc_asm_input_4 cri_sc_asm_input_3, "4" (cri_sc_6)
#define cri_sc_asm_input_5 cri_sc_asm_input_4, "5" (cri_sc_7)
#define cri_sc_asm_input_6 cri_sc_asm_input_5, "6" (cri_sc_8)

#define cri_syscall0(type,name,nr)						\
type name(int *errno_p)								\
{										\
	cri_syscall_nr(0, type, nr, errno_p);					\
}

#define cri_syscall1(type,name,nr,type1)					\
type name(type1 arg1,int *errno_p)						\
{										\
	cri_syscall_nr(1, type, nr, errno_p, arg1);				\
}

#define cri_syscall2(type,name,nr,type1,type2)					\
type name(type1 arg1,type2 arg2,int *errno_p)					\
{										\
	cri_syscall_nr(2, type, nr, errno_p, arg1, arg2);			\
}

#define cri_syscall3(type,name,nr,type1,type2,type3)				\
type name(type1 arg1,type2 arg2,type3 arg3,int *errno_p)			\
{										\
	cri_syscall_nr(3, type, nr, errno_p, arg1, arg2, arg3);			\
}

#define cri_syscall4(type,name,nr,type1,type2,type3,type4) \
type name(type1 arg1,type2 arg2,type3 arg3,type4 arg4,int *errno_p)		\
{										\
	cri_syscall_nr(4, type, nr, errno_p, arg1, arg2, arg3, arg4);		\
}

#define cri_syscall5(type,name,nr,type1,type2,type3,type4,type5)		\
type name(type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5,int *errno_p)	\
{										\
	cri_syscall_nr(5, type, nr, errno_p, arg1, arg2, arg3, arg4, arg5);	\
}

#define cri_syscall6(type,name,nr,type1,type2,type3,type4,type5,type6)		\
type name(type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5,type6 arg6,int *errno_p)\
{										\
	cri_syscall_nr(6, type, nr, errno_p, arg1, arg2, arg3, arg4, arg5, arg6);\
}

/*
 * Because we enter the kernel via a "normal" syscall, the kernel
 * will not save the caller-saved r13-29 in the ptregs struct.
 * Thus vmadump can't restore them correctly at restart.
 * We work around that here telling gcc they are clobbered.
 * Since gcc ignores clobbers of r30 and r31, we deal with them
 * on our own.
 */

#define cri_sc_asm_clobber_3X "r13", "r14", "r15", "r16", "r17", "r18", "r19", \
	 "r20", "r21", "r22", "r23", "r24", "r25", "r26", "r27", "r28", "r29",

#if SIZEOF_VOID_P == 8
  #define cri_sc_st	"std  30,%8   \n\t" \
			"std  31,%7   \n\t"
  #define cri_sc_ld	"ld   31,%7   \n\t" \
			"ld   30,%8   \n\t"
#elif SIZEOF_VOID_P == 4
  #define cri_sc_st	"stw  30,%8   \n\t" \
			"stw  31,%7   \n\t"
  #define cri_sc_ld	"lwz  31,%7   \n\t" \
			"lwz  30,%8   \n\t"
#else
  #error "No SIZEOF_VOID_P"
#endif

#define cri_syscall3X(type,name,nr,type1,type2,type3)			\
type name(type1 arg1,type2 arg2,type3 arg3,int *errno_p)		\
{									\
	unsigned long cri_sc_ret, cri_sc_err;				\
	unsigned long cri_sc_tmp[2];					\
	{								\
		register unsigned long cri_sc_0  __asm__ ("r0");	\
		register unsigned long cri_sc_3  __asm__ ("r3");	\
		register unsigned long cri_sc_4  __asm__ ("r4");	\
		register unsigned long cri_sc_5  __asm__ ("r5");	\
		register unsigned long cri_sc_6  __asm__ ("r6");	\
		register unsigned long cri_sc_7  __asm__ ("r7");	\
		register unsigned long cri_sc_8  __asm__ ("r8");	\
									\
		cri_sc_loadargs_3(nr, arg1, arg2, arg3);		\
		__asm__ __volatile__					\
			(						\
			 cri_sc_st					\
			 "sc           \n\t"				\
			 cri_sc_ld					\
			 "mfcr %0      \n\t"				\
			: "=&r" (cri_sc_0),				\
			  "=&r" (cri_sc_3),  "=&r" (cri_sc_4),		\
			  "=&r" (cri_sc_5),  "=&r" (cri_sc_6),		\
			  "=&r" (cri_sc_7),  "=&r" (cri_sc_8)		\
			: "m" (cri_sc_tmp[0]),				\
			  "m" (cri_sc_tmp[1]),				\
			  cri_sc_asm_input_3				\
			: cri_sc_asm_clobber_3X				\
			  "cr0", "ctr", "memory",			\
			        "r9", "r10","r11", "r12");		\
		cri_sc_ret = cri_sc_3;					\
		cri_sc_err = cri_sc_0;					\
	}								\
	if (cri_sc_err & 0x10000000)					\
	{								\
		if (errno_p) *errno_p = cri_sc_ret;			\
		cri_sc_ret = -1;					\
	}								\
	else if (cri_sc_ret >= (unsigned long)(-4096))			\
	{								\
		if (errno_p) *errno_p = -cri_sc_ret;			\
		cri_sc_ret = -1;					\
	}								\
	return (type) cri_sc_ret;					\
}

/* No sr_restorer on ppc/ppc64 */
#undef CRI_SA_RESTORER

/* Template to build "micro" handlers */
#if SIZEOF_VOID_P == 8
  #define _CR_ASM_FUNC_PRE(_name) \
	"	.section \".text\"\n"\
	"	.align	2\n"\
	"	.globl	" _STRINGIFY(_name) "\n"\
	"	.type	" _STRINGIFY(_name) ", @function\n"\
	"	.section	\".opd\",\"aw\"\n"\
	"	.align	3\n"\
	_STRINGIFY(_name) ":\n"\
	"	.quad	.L." _STRINGIFY(_name) ",.TOC.@tocbase\n"\
	"	.previous\n"\
	".L." _STRINGIFY(_name) ":\n"
  #define _CR_ASM_FUNC_POST(_name) \
	"	.size	" _STRINGIFY(_name) ",.-.L." _STRINGIFY(_name) "\n"
#else
  #define _CR_ASM_FUNC_PRE(_name) \
	"	.section \".text\"\n"\
	"	.align	2\n"\
	"	.globl	" _STRINGIFY(_name) "\n"\
	"	.type	" _STRINGIFY(_name) ", @function\n"\
	_STRINGIFY(_name) ":\n"
  #define _CR_ASM_FUNC_POST(_name) \
	"	.size	" _STRINGIFY(_name) ",.-" _STRINGIFY(_name) "\n"
#endif

/* The following counts on _op fitting in 32 bits and _arg in 15 bits */
#define _CR_ASM_HANDLER(_name, _op, _arg) \
    __asm__( \
	_CR_ASM_FUNC_PRE(_name) \
	"	lwz	%r3," _STRINGIFY(CR_ASM_SI_PID_OFFSET) "(%r4)	# r3 = siginfo->si_pid\n"\
	"	lis	%r4," _STRINGIFY(_op) "@h			# r4 =\n"\
	"	ori	%r4,%r4," _STRINGIFY(_op) "@l			# ... _op\n"\
	"	li	%r5," _STRINGIFY(_arg) "			# r5 = _arg\n"\
	"	li	%r0," _STRINGIFY(CR_ASM_NR_ioctl) "		# r0 = __NR_ioctl\n"\
	"	sc							# system call\n"\
	"	blr							# return from signal\n"\
	"	nop\n" \
	_CR_ASM_FUNC_POST(_name));

#if !CR_HAVE_BUG2524
#define CR_RUN_ASM_HANDLER(_name) \
	_CR_ASM_HANDLER(_name, CR_ASM_OP_HAND_CHKPT, CR_ASM_CHECKPOINT_STUB)
#endif

#define CR_OMIT_ASM_HANDLER(_name) \
	_CR_ASM_HANDLER(_name, CR_ASM_OP_HAND_ABORT, CR_ASM_CHECKPOINT_OMIT)

#endif
