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
 * $Id: cr_core.c,v 1.206.6.3 2014/09/18 23:46:08 phargrov Exp $
 */

#include <sys/ioctl.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <ucontext.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <dlfcn.h>
#include <features.h>

#include "cr_private.h"

//
// Private variables
//

//
// Public variables
//

//
// Internal variables
//

// How many threads are actively trying to checkpoint (PENDING + ACTIVE)
cri_atomic_t cri_live_count = {0,};

// Red-black lock for critical sections
cri_rb_lock_t cri_cs_lock = CRI_RB_LOCK_INITIALIZER;

// Default values for hold flags
cri_atomic_t cri_hold_init   = {CR_HOLD_BOTH,};
cri_atomic_t cri_hold_uninit = {CR_HOLD_BOTH,};

// Table of hooks
volatile cr_hook_fn_t cri_hook_tbl[CR_NUM_HOOKS];

//
// Private functions
//

// Tell the kernel we are ready to be saved
// This version just invokes __cri_chkpt(flags)
static int do_checkpoint(int token, unsigned long flags)
{
    int rc, local_errno;

    // "GLOBAL" CHECKPOINT STUFF HERE:


    // This does the actual kernel call to take the checkpoint:
    rc = __cri_chkpt(token, CR_OP_HAND_CHKPT, (void *)flags, &local_errno);
    if (rc < 0) {
	return -local_errno;
    }

    if (rc) {
	// "GLOBAL" RESTART STUFF HERE
    } else {
	// "GLOBAL" CONTINUE STUFF HERE
    }

    return rc;
}

// enter_idle_state(info)
//
// Transition to CR_STATE_IDLE, regardless of starting state
static inline void enter_idle_state(cri_info_t *info)
{
    cri_atomic_write(&info->cr_state, CR_STATE_IDLE);
    cri_atomic_write(&info->cr_cs_count, 1);
}

// Signal handler to dispatch the request
//
// Must NOT be static because we need to be able to dlsym() it
// when trying to mix static and shared libs.
//
// WARNING: Must only be called as a signal handler.
// Any other calls may not return correctly (see final lines).
/* static */ void cri_sig_handler(int signr, siginfo_t *siginfo, void *context)
{
    int saved_errno = errno;
#if LIBCR_TRACING
    int pid = (int)getpid();
#endif
    cri_info_t *info = cri_info_location();	// thread-specific
    const int token = siginfo->si_pid;

    LIBCR_TRACE(LIBCR_TRACE_INFO, "[%d] signal %d received", pid, signr);

#ifdef SI_KERNEL
    if (siginfo->si_code != SI_KERNEL) {
	LIBCR_TRACE(LIBCR_TRACE_INFO, "[%d] Ignoring false signal", pid);
	goto out;
    }
#endif

    if (!info || !info->handler) {
	int rc;

	// No state and/or handler, so do this the "easy way"
	LIBCR_TRACE(LIBCR_TRACE_INFO, "[%d] START", pid);
	cri_atomic_inc(&cri_live_count);
	rc = do_checkpoint(token, 0);
	(void)cri_atomic_dec_and_test(&cri_live_count);
	if (rc < 0) {
	    LIBCR_TRACE(LIBCR_TRACE_INFO, "[%d] do_checkpoint failed w/ rc=%d", pid, rc);
	} else {
	    CRI_RUN_HOOK(rc ? CR_HOOK_RSTRT_NO_CALLBACKS : CR_HOOK_CONT_NO_CALLBACKS);
	    rc = cri_syscall_token(token, CR_OP_HAND_DONE, cri_atomic_read(&cri_hold_uninit));
	    if (rc < 0) {
		CRI_ABORT("CR_OP_HAND_DONE failed w/ rc=%d errno=%d", rc, errno);
	    }
	}
	LIBCR_TRACE(LIBCR_TRACE_INFO, "[%d] DONE", pid);
    } else {
	// Run the handler appropriate to this thread
	LIBCR_TRACE(LIBCR_TRACE_INFO, "[%d] running handler %p", pid, info->handler);
	info->run.token = token;
	info->handler(signr, siginfo, context);
    }

out:
    errno = saved_errno;

    // Ugh!! Bug 2003: sa_restorer might be corrupted.
    // So, we return "directly" when supported
#if defined(cri_sigreturn)
    cri_sigreturn(signr, siginfo, context);
#endif
}

//
// Public functions
//

cr_hook_fn_t cr_register_hook(cr_hook_event_t event, cr_hook_fn_t fn)
{
    cr_hook_fn_t old_fn = CR_HOOK_FN_ERROR; // assume failure
    cri_info_t *info = CRI_INFO_OR_RETURN(old_fn);	// thread-specific

    /* Static data: */
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    /* Argument checking includes:
     * + Range check 'event'
     * + Check that 'fn' is not our reserved error value.
     */
    errno = EINVAL;
    if ((event >= 0) && (event < CR_NUM_HOOKS) && (fn != CR_HOOK_FN_ERROR)) {
	(void)cri_do_enter(info, CRI_ID_INTERNAL);
	pthread_mutex_lock(&lock);
	old_fn = cri_hook_tbl[event];
	cri_hook_tbl[event] = fn;
	pthread_mutex_unlock(&lock);
	cri_do_leave(info, CRI_ID_INTERNAL);
    } else {
	// return value and errno already set
    }

    return old_fn;
}

int cr_checkpoint(int flags)
{
    cri_info_t *info;	// thread-specific
    int my_id;
    int outer_most;
    int index;
    int token;
    int retval;
    int rc;
#if LIBCR_TRACING
    int pid = (int)getpid();
#endif

    // Checks for improper calls
    info = CRI_CB_INFO_OR_RETURN(-CR_ENOTCB);	// thread-specific

    // Save callers id and other bits from info->
    my_id = info->run.index;
    token = info->run.token;
    outer_most = (my_id == info->cr_cb_count);

    // Loop calling callbacks until they have all been called and then
    // invoke do_checkpoint() to do the real work.
    //
    // This strange loop ensures that after the callback with (index == 0)
    // we next call do_checkpoint() even if callback0 is NULL or returns
    // without calling back into cr_checkpoint().
    do {
	if (flags & CR_CHECKPOINT_ABORT_MASK) {
	    // We've detected failure of the invoking callback
	    // We we must now call the kernel and stop stacking of callbacks.
	    LIBCR_TRACE(LIBCR_TRACE_INFO, "[%d] CANCELLING CHECKPOINT (flags=%d)",
			pid, flags);
	    if (cri_syscall_token(token, CR_OP_HAND_ABORT, flags) == -1)
		info->run.rc = -errno;
	    index = info->run.index = -1;
	    break;
	}
    
	index = --info->run.index;
	if (index >= 0) {
	    // Call the next callback, skipping NULLs

	    if (info->cr_cb[index].func != NULL) {
		info->run.id = index;
		rc = (*info->cr_cb[index].func)(info->cr_cb[index].arg);
		if (rc) {
		    LIBCR_TRACE(LIBCR_TRACE_INFO, "Callback %d returned %d - ABORTING\n",
				index, rc);
		    (void)cri_syscall_token(token, CR_OP_HAND_ABORT, CR_CHECKPOINT_PERM_FAILURE);
		    CRI_ABORT("Unexpected return from CR_OP_HAND_ABORT");
		}
	    }
	} else {
	    LIBCR_TRACE(LIBCR_TRACE_INFO, "[%d] CHECKPOINTING (flags=%d)",
			pid, flags);
	    info->run.rc = do_checkpoint(info->run.token, flags);
	    if (info->run.rc < 0) {
		LIBCR_TRACE(LIBCR_TRACE_INFO,
			    "[%d] do_checkpoint() FAILED", pid);
	    } else if (info->run.rc) {
		LIBCR_TRACE(LIBCR_TRACE_INFO, "[%d] RESTORING", pid);
	    } else {
		LIBCR_TRACE(LIBCR_TRACE_INFO, "[%d] CONTINUING", pid);
	    }
	}
    } while (info->run.index >= 0);

    // Once the outer-most invocation calls CR_OP_HAND_{CONT,RSTRT}
    // the kernel might checkpoint us again.  Save the return value now
    // because a second checkpoint could modify info->run.rc.
    retval = info->run.rc;

    /* Restore saved id for possible use by the caller (in replace_self). */
    info->run.id = my_id;

    // Perform cleanup only on the final/outer-most return
    if (outer_most) {
	enter_idle_state(info);
	cri_atomic_dec_and_test(&cri_live_count);
	if (retval >= 0) {
	    int hold;
	    CRI_RUN_HOOK(retval ? (info->is_thread ? CR_HOOK_RSTRT_THREAD_CONTEXT
						   : CR_HOOK_RSTRT_SIGNAL_CONTEXT)
				: (info->is_thread ? CR_HOOK_CONT_THREAD_CONTEXT
						   : CR_HOOK_CONT_SIGNAL_CONTEXT));
	    hold = cri_atomic_read(&info->hold);
	    if (hold == CR_HOLD_DFLT) {
		hold = cri_atomic_read(&cri_hold_init);
	    }
	    rc = cri_syscall_token(info->run.token, CR_OP_HAND_DONE, hold);
	    if (rc < 0) {
		CRI_ABORT("CR_OP_HAND_DONE failed w/ rc=%d errno=%d", rc, errno);
	    }
	}
    }

    return retval;
}

// cr_inc_persist()
//
// Routine to request delayed free of a cri_info_t
int cr_inc_persist(void)
{
    cri_info_t *info = CRI_INFO_OR_RETURN(-1);	// thread-specific
    int retval;

    retval = info->persist;
    if (retval == INT_MAX) {
	/* Overflow */
	errno = ERANGE;
	return -1;
    }

    info->persist += 1;
    return retval;
}

// cr_dec_persist()
//
// Routine to cancel a request for delayed free of a cri_info_t
int cr_dec_persist(void)
{
    cri_info_t *info = CRI_INFO_OR_RETURN(-1);	// thread-specific
    int retval;

    retval = info->persist;
    if (retval == 0) {
	/* Underflow */
	errno = ERANGE;
	return -1;
    }

    info->persist -= 1;
    return retval;
}


//
// Internal functions
//

// cri_checkpoint_info_init()
//
// Initialize a thread-local struct cr_checkpoint_info
void cri_checkpoint_info_init(cri_info_t *info)
{
    info->cr_checkpoint_info.dest = NULL;	// filled-in on demand
    info->cr_restart_info.src = NULL;		// filled-in on demand
}

// cri_info_free()
//
// dtor for thread-specific data
//
// Docs claim we are called only for non-NULL val.
// Docs claim we are called w/ stored value set to NULL.
// I've seen both violated, so we code w/ paranoia here.
void
cri_info_free(void *val)
{
    cri_info_t *info = (cri_info_t *)val;

    if (info && !info->persist) {
	free(info);
	info = NULL;
    }

    // Clear if destroyed, reinstall otherwise:
    pthread_setspecific(cri_info_key, info);
}


// cri_info_init()
//
// Routine to initialize the list of registered callbacks and other data
// On first call per thread, allocates the thread-specific cri_info structure.
cri_info_t* cri_info_init(void)
{
    cri_info_t *info = cri_info_location();	// thread-specific
    int rc;

    if (!info) {
	info = malloc(sizeof(cri_info_t));
	if (!info) {
	    CRI_ABORT("Failed to allocate cri_info structure");
	}

	memset(info, 0, sizeof(cri_info_t));
	enter_idle_state(info);
	info->handler = NULL;
	info->next_id = 0;
	info->is_thread = 0;
	info->persist = 0;
	cri_atomic_write(&info->hold, CR_HOLD_DFLT);

	rc = pthread_setspecific(cri_info_key, info);
	if (rc != 0) {
	    CRI_ABORT("pthread_setspecific() returned %d", rc);
	}
    }

    return info;
}

// cri_start_checkpoint()
//
// Routine to process the list of registered callbacks
void cri_start_checkpoint(cri_info_t *info)
{
    if (cri_cmp_swap(&info->cr_state, CR_STATE_PENDING, CR_STATE_ACTIVE)) {
	// State is valid, so start running callbacks
#if LIBCR_TRACING
	int pid = (int)getpid();
#endif
	info->run.index = info->cr_cb_count;
	LIBCR_TRACE(LIBCR_TRACE_INFO, "[%d] START", pid);
	(void) cr_checkpoint(0);
	LIBCR_TRACE(LIBCR_TRACE_INFO, "[%d] DONE", pid);
    } else if (!info->is_thread) {
	CRI_ABORT("STATE ERROR.  Probably an excess call to cr_leave_cs().");
    } else {
	// False wake up of a checkpoint thread - not an error.
    }
}


//
// Library Initialization
//

#define CR_LINK_ME_VAR cr_link_me
#define CR_SIG_HANDLER cri_sig_handler
#define CR_SIG_IS_FULL 1
#include "cr_libinit.c"
