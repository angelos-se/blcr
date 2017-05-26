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
 * $Id: cr_arch.h,v 1.2 2008/12/02 00:17:44 phargrov Exp $
 *
 * Experimental SPARC support contributed to BLCR by Vincentius Robby
 * <vincentius@umich.edu> and Andrea Pellegrini <apellegr@umich.edu>.
 */

#ifndef _CR_ARCH_H
#define _CR_ARCH_H	1

// Catch-all file for misc. arch-specific bits

// syscall macros from glibc (LGPL)

#if (SIZEOF_VOID_P == 4)
   #define CRI_SYSCALL_CLOBBERS "g2", "g3", "g4", "g5", "g6",   \
	"f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",         \
	"f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15",   \
	"f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23", \
	"f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31", \
	"cc", "memory"
   #define CRI_SYSCALL_STRING "ta     0x10;"
#elif (SIZEOF_VOID_P == 8)
   #define CRI_SYSCALL_CLOBBERS "g2", "g3", "g4", "g5", "g6",   \
	"f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",         \
	"f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15",   \
	"f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23", \
	"f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31", \
	"f32", "f34", "f36", "f38", "f40", "f42", "f44", "f46", \
	"f48", "f50", "f52", "f54", "f56", "f58", "f60", "f62", \
	"cc", "memory"
   #define CRI_SYSCALL_STRING "ta     0x6d;"
#else
   #error "Unknown SIZEOF_VOID_P"
#endif 

/* 
 * Note we are fudging on the errno return, assuming (like x86) that anything
 * lower than -4096 (rather than -515) is an error.  This is to allow for our
 * extended errno values.
 */
#define cri_syscall_cleanup(res,errno_p) \
    if ((unsigned long)res >= (unsigned long)(-4096)) {	\
	if (errno_p != NULL) { *errno_p = -res; }	\
	res = -1;					\
    }

#define cri_syscall6(type,name,nr,type1,type2,type3,type4,type5,type6)\
  type name(type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5,type6 arg6,int *errno_p) {\
      register long __o0 __asm__ ("o0") = (long)(arg1);	\
      register long __o1 __asm__ ("o1") = (long)(arg2);	\
      register long __o2 __asm__ ("o2") = (long)(arg3);	\
      register long __o3 __asm__ ("o3") = (long)(arg4);	\
      register long __o4 __asm__ ("o4") = (long)(arg5);	\
      register long __o5 __asm__ ("o5") = (long)(arg6);	\
      register long __g1 __asm__ ("g1") = nr;		\
      __asm __volatile (CRI_SYSCALL_STRING		\
		: "=r" (__g1), "=r" (__o0)		\
		: "0" (__g1), "1" (__o0), "r" (__o1),	\
		  "r" (__o2), "r" (__o3), "r" (__o4),	\
		  "r" (__o5)				\
		: CRI_SYSCALL_CLOBBERS);		\
      cri_syscall_cleanup(__o0, errno_p);		\
      return (type)__o0;				\
  }

#define cri_syscall5(type,name,nr,type1,type2,type3,type4,type5)\
  type name(type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5,int *errno_p) {\
      register long __o0 __asm__ ("o0") = (long)(arg1);	\
      register long __o1 __asm__ ("o1") = (long)(arg2);	\
      register long __o2 __asm__ ("o2") = (long)(arg3);	\
      register long __o3 __asm__ ("o3") = (long)(arg4);	\
      register long __o4 __asm__ ("o4") = (long)(arg5);	\
      register long __g1 __asm__ ("g1") = nr;		\
      __asm __volatile (CRI_SYSCALL_STRING		\
		: "=r" (__g1), "=r" (__o0)		\
		: "0" (__g1), "1" (__o0), "r" (__o1),	\
		  "r" (__o2), "r" (__o3), "r" (__o4)	\
		: CRI_SYSCALL_CLOBBERS);		\
      cri_syscall_cleanup(__o0, errno_p);		\
      return (type)__o0;				\
  }

#define cri_syscall4(type,name,nr,type1,type2,type3,type4)\
  type name(type1 arg1,type2 arg2,type3 arg3,type4 arg4,int *errno_p) {\
      register long __o0 __asm__ ("o0") = (long)(arg1);	\
      register long __o1 __asm__ ("o1") = (long)(arg2);	\
      register long __o2 __asm__ ("o2") = (long)(arg3);	\
      register long __o3 __asm__ ("o3") = (long)(arg4);	\
      register long __g1 __asm__ ("g1") = nr;		\
      __asm __volatile (CRI_SYSCALL_STRING		\
		: "=r" (__g1), "=r" (__o0)		\
		: "0" (__g1), "1" (__o0), "r" (__o1),	\
		  "r" (__o2), "r" (__o3)		\
		: CRI_SYSCALL_CLOBBERS);		\
      cri_syscall_cleanup(__o0, errno_p);		\
      return (type)__o0;				\
  }

#define cri_syscall3(type,name,nr,type1,type2,type3) \
  type name(type1 arg1,type2 arg2,type3 arg3,int *errno_p) {\
      register long __o0 __asm__ ("o0") = (long)(arg1);	\
      register long __o1 __asm__ ("o1") = (long)(arg2);	\
      register long __o2 __asm__ ("o2") = (long)(arg3);	\
      register long __g1 __asm__ ("g1") = nr;		\
      __asm __volatile (CRI_SYSCALL_STRING		\
		: "=r" (__g1), "=r" (__o0)		\
		: "0" (__g1), "1" (__o0), "r" (__o1),	\
		  "r" (__o2)				\
		: CRI_SYSCALL_CLOBBERS);		\
      cri_syscall_cleanup(__o0, errno_p);		\
      return (type)__o0;				\
  }

#define cri_syscall2(type,name,nr,type1,type2) \
  type name(type1 arg1,type2 arg2,int *errno_p) {	\
      register long __o0 __asm__ ("o0") = (long)(arg1);	\
      register long __o1 __asm__ ("o1") = (long)(arg2);	\
      register long __g1 __asm__ ("g1") = nr;		\
      __asm __volatile (CRI_SYSCALL_STRING		\
		: "=r" (__g1), "=r" (__o0)		\
		: "0" (__g1), "1" (__o0), "r" (__o1)	\
		: CRI_SYSCALL_CLOBBERS);		\
      cri_syscall_cleanup(__o0, errno_p);		\
      return (type)__o0;				\
  }

#define cri_syscall1(type,name,nr,type1) \
  type name(type1 arg1,int *errno_p) {			\
      register long __o0 __asm__ ("o0") = (long)(arg1);	\
      register long __g1 __asm__ ("g1") = nr;		\
      __asm __volatile (CRI_SYSCALL_STRING		\
		: "=r" (__g1), "=r" (__o0)		\
		: "0" (__g1), "1" (__o0)		\
		: CRI_SYSCALL_CLOBBERS);		\
      cri_syscall_cleanup(__o0, errno_p);		\
      return (type)__o0;				\
  }

#define cri_syscall0(type,name,nr) \
  type name(int *errno_p) {				\
      register long __o0 __asm__ ("o0");		\
      register long __g1 __asm__ ("g1") = nr;		\
      __asm __volatile (CRI_SYSCALL_STRING		\
		: "=r" (__g1), "=r" (__o0)		\
		: "0" (__g1)				\
		: CRI_SYSCALL_CLOBBERS);		\
      cri_syscall_cleanup(__o0, errno_p);		\
      return (type)__o0;				\
  }

#endif
