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
 * $Id: cr_syscall.c,v 1.33 2008/05/20 18:47:56 phargrov Exp $
 *
 *
 * This file abstracts away the ioctl() interface so that we
 * can painlessly switch to a "real" syscall someday.
 */

#include <sys/ioctl.h>	// for ioctl()
#include <fcntl.h>	// for O_WRONLY
#include <errno.h>	// for errno
#include <unistd.h>	// for dup2 

#include "cr_private.h"

//
// Private data
//

static cri_atomic_t	local_fd	= {(unsigned int)(-1L)};

//
// Private functions
//

//
// Public Functions
//

// Open an instance of the control node.
//
// returns < 0 on failure, the fd on success
int cri_connect_token(void)
{
    int fd;

    // Try to open the control node
    fd = __cri_open(CR_CTRL_FILE, O_WRONLY, 0, NULL);
    if (fd < 0) {
        // Fail: not present
        errno = ENOSYS;
        goto out;
    }

    // Set close-on-exec flag, failing silently if at all
    (void)fcntl(fd, F_SETFD, FD_CLOEXEC);

    // Check kernel interface version
    if (ioctl(fd, CR_OP_VERSION, (CR_MODULE_MAJOR << 16) | CR_MODULE_MINOR) < 0) {
	// Kernel module doesn't support our version of the interface
	// ioctl() has set errno to CR_EVERSION
	(void)__cri_close(fd, NULL); // may fail silently, doesn't touch errno
	fd = -1;
    }

out:
    return fd;
}

// Close an instance of the control descriptor.
// Note that this is signal and thread safe.
void cri_disconnect_token(int fd)
{
    (void)__cri_close(fd, NULL);  // may fail silently
}

// Call kernel w/ a specific location for errno
int __cri_syscall_token(int fd, int op, uintptr_t arg, int *errno_p)
{
    return __cri_ioctl(fd, op, (void *)arg, errno_p);
}

// Call kernel w/ the default location for errno
int cri_syscall_token(int fd, int op, uintptr_t arg)
{
    return __cri_syscall_token(fd, op, arg, &errno);
}


// Open a single, shared, instance of the control node.
// Note that this is signal and thread safe.
//
// returns < 0 on failure, the fd on success
int cri_connect(void)
{
    int fd;

    while ((fd = (int)cri_atomic_read(&local_fd)) < 0) {
	int old = fd;

	// Try to open the control node
	fd = cri_connect_token();

	// Try to install our fd
	if (!cri_cmp_swap(&local_fd, old, fd)) {
	    // We lost the race: another thread has opened the file
	    (void)__cri_close(fd, NULL);  // may fail silently
	    continue;
	}

	break;
    }

    return fd;
}
// Close the single shared instance of the control descriptor.
// Note that this is signal and thread safe.
// HOWEVER: not safe to race against cri_syscall().
void cri_disconnect(void)
{
    int fd;

    if ((fd = (int)cri_atomic_read(&local_fd)) >= 0) {
	// Remove it
	if (cri_cmp_swap(&local_fd, fd, -1)) {
	    cri_disconnect_token(fd);
	} else {
	    // We lost the race: another thread has closed the file
	}
    }
}

/* Special version of cri_syscall that works even w/o a fully initialized thread environment.
 * However, does not check cri_connect().
 */
int __cri_syscall(int op, uintptr_t arg, int *errno_p)
{
    return __cri_ioctl((int)cri_atomic_read(&local_fd), op, (void *)arg, errno_p);
}

int cri_syscall(int op, uintptr_t arg)
{
    int token = cri_connect();
    return (token < 0) ? token : __cri_syscall_token(token, op, arg, &errno);
}


/* Functions for direct syscalls */
/* These don't require any thread environment and use a caller-provided errno */
#include <errno.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>
#include <asm/unistd.h>
#include <linux/unistd.h>

cri_syscall3(int, __cri_ioctl, __NR_ioctl, int, int, void*)
cri_syscall3(int, __cri_open, __NR_open, const char*, int, int)
cri_syscall1(int, __cri_close, __NR_close, int);
cri_syscall0(int, __cri_sched_yield, __NR_sched_yield)
cri_syscall2(int, __cri_nanosleep, __NR_nanosleep, const struct timespec*, struct timespec*)
cri_syscall1(int, __cri_exit, __NR_exit, int)
#ifdef __NR_exit_group
  cri_syscall1(int, __cri_exit_group, __NR_exit_group, int)
#else
  int __cri_exit_group(int code, int *errno_p) {
    if (errno_p) { *errno_p = ENOSYS; }
    return -1;
  }
#endif
cri_syscall4(int, __cri_ksigaction, __NR_rt_sigaction, int, const struct k_sigaction*, struct k_sigaction*, size_t)

/* Special-case the checkpoint call (which is still an ioctl) to allow for
 * arch-specific code that deals with caller-saved registers (which don't
 * get restored by the kernel to the right values at restart).
 */
cri_syscall3X(int, __cri_chkpt, __NR_ioctl, int, int, void*)

