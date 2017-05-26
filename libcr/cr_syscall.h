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
 * $Id: cr_syscall.h,v 1.14 2008/05/11 06:52:17 phargrov Exp $
 */

#ifndef _CR_SYSCALL_H
#define _CR_SYSCALL_H	1

#include "blcr_config.h"
#include <features.h>

#include <time.h>		// for (struct timespec)
#include <signal.h>		// for siginfo_t and sigset_t
#include <stdint.h>		// for uintptr_t

#ifndef CRI_SYSCALL_NOARG
  #define CRI_SYSCALL_NOARG ((uintptr_t)(-1L))
#endif

// arguments to rt_sigaction system call
struct k_sigaction {
  void (*ksa_sigaction) (int, siginfo_t *, void *);
  unsigned long ksa_flags;
  void (*ksa_restorer)(void);
  sigset_t ksa_mask;               /* mask moved last for extensibility */
};

// Establish "connection" to blcr kernel module
extern int cri_connect(void);
extern void cri_disconnect(void);

// Call the blcr kernel module w/ the default "connection"
extern int cri_syscall(int op, uintptr_t arg);
extern int __cri_syscall(int op, uintptr_t arg, int * errno_p);

// Manually manage connection(s) to the blcr kernel module
// The value returned by cri_connect_token(), if non-negative,
// should be passed as the 'token' argument to the other calls.
// It is currently just the fd, but that could change some day.
extern int cri_connect_token(void);
extern void cri_disconnect_token(int token);
extern int cri_syscall_token(int token, int op, uintptr_t arg);
extern int __cri_syscall_token(int token, int op, uintptr_t arg, int * errno_p);

// Special case for CR_OP_HAND_CHKPT to allow arch-specific register saves
extern int __cri_chkpt(int fd, int cmd, void * arg, int * errno_p);

// Direct system calls, bypassing any pthread wrappers
extern int __cri_ioctl(int fd, int cmd, void * arg, int * errno_p);
extern int __cri_open(const char * pathname, int flags, int mode, int * errno_p);
extern int __cri_close(int fd, int * errno_p);
extern int __cri_sched_yield(int * errno_p);
extern int __cri_nanosleep(const struct timespec * req, struct timespec * rem, int * errno_p);
extern int __cri_exit(int code, int * errno_p);
extern int __cri_exit_group(int code, int * errno_p);
extern int __cri_ksigaction(int signum, const struct k_sigaction *act,
			    struct k_sigaction *oldact, size_t setsize, int * errno_p);

#endif	/* _CR_SYSCALL_H */
