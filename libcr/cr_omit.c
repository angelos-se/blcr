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
 * $Id: cr_omit.c,v 1.12.8.1 2011/08/03 19:24:28 eroman Exp $
 *
 * This file builds the "libcr_omit" target library.
 */

#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <ucontext.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <features.h>

#include "cr_private.h"

#define CR_SIG_HANDLER cri_omit_sig_handler
#define CR_LINK_ME_VAR cr_omit_link_me

#ifdef LIBCR_SIGNAL_ONLY
  /* Initialization logic. */
  #define CR_BUILDING_OMIT
  #include "cr_libinit.c"
#endif /* LIBCR_SIGNAL_ONLY */

// Signal handler to dispatch the request
//
// WARNING: Must only be called as a signal handler.
// Any other calls may not return correctly (see final lines).
#ifdef CR_OMIT_ASM_HANDLER
  CR_OMIT_ASM_HANDLER(CR_SIG_HANDLER)
#else
cri_syscall3(int, crsig_ioctl, CR_ASM_NR_ioctl, int, int, void*)
void CR_SIG_HANDLER(int signr, siginfo_t *siginfo, void *context)
{
#if LIBCR_TRACING
    int pid = (int)getpid();
#endif
    int token = siginfo->si_pid;
    int rc, local_errno;

    LIBCR_TRACE(LIBCR_TRACE_INFO, "[%d] signal %d received, fd=%d", pid, signr, token);

#ifdef SI_KERNEL
    if (siginfo->si_code != SI_KERNEL) {
	LIBCR_TRACE(LIBCR_TRACE_INFO, "[%d] Ignoring false signal", pid);
	goto out;
    }
#endif

    LIBCR_TRACE(LIBCR_TRACE_INFO, "[%d] call cr_abort(OMIT)", pid);
    rc = crsig_ioctl(token, CR_OP_HAND_ABORT, (void *)CR_CHECKPOINT_OMIT, &local_errno);
    if (rc < 0) {
	LIBCR_TRACE(LIBCR_TRACE_INFO, "[%d] cr_abort(OMIT) failed w/ errno=%d", pid, local_errno);
    }
    LIBCR_TRACE(LIBCR_TRACE_INFO, "[%d] DONE", pid);

out:
    // Ugh!! Bug 2003: sa_restorer might be corrupted.
    // So, we return "directly" when supported
#if defined(cri_sigreturn)
    cri_sigreturn(signr, siginfo, context);
#endif
    return; /* so "out:" is never the last statement (avoid gcc error) */
}
#endif /* CR_OMIT_ASM_HANDLER */
