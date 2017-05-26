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
 * $Id: cr_cs.c,v 1.67.14.3 2011/08/24 02:35:39 phargrov Exp $
 */

#include <errno.h>

#include "cr_private.h"

//
// Private data
//

#define CRI_CONTEXT_MASK    (CR_THREAD_CONTEXT | CR_SIGNAL_CONTEXT)

//
// Private functions
//

static inline void
poll_checkpoint(cri_info_t *info)
{
    if (cri_atomic_dec_and_test(&info->cr_cs_count)) {
	cri_black_lock(&cri_cs_lock);
	cri_start_checkpoint(info);
	cri_black_unlock(&cri_cs_lock);
    }
}

// signal_handler()
//
// Signal handler for threads w/ signal-context handlers
static void
my_handler(int signr, siginfo_t *siginfo, void *context)
{
    int saved_errno = errno;
    cri_info_t *info = cri_info_location();	// thread-specific

    cri_checkpoint_info_init(info);

    if (cri_cmp_swap(&info->cr_state, CR_STATE_IDLE, CR_STATE_PENDING)) {
	cri_atomic_inc(&cri_live_count);
	poll_checkpoint(info);
    }

    errno = saved_errno;
}

static int
do_state(cri_info_t	*info)
{
    int state = cri_atomic_read(&info->cr_state);

    if ((state == CR_STATE_IDLE) && cri_atomic_read(&cri_live_count)) {
	state = CR_STATE_PENDING;
    }

    return state;
}

// This mess is to prevent starvation by not entering an
// outermost critical section when a checkpoint is pending
// or running in ANY thread.
static int
try_enter(cri_info_t *info) {
    cri_atomic_t *count = &info->cr_cs_count;
    int old;

    // First advance the counter
    do {
	old = (int)cri_atomic_read(count);
    } while (!cri_cmp_swap(count, old, old + 1));

    if (old > 1) {
	// Not the outermost, so proceed normally
	cri_red_lock(&cri_cs_lock);
    } else if (cri_atomic_read(&cri_live_count) || cri_red_trylock(&cri_cs_lock)) {
	// PENDING - let it proceed if ready, but retry regardless
#if 1
        /* XXX: What the heck is this??
           This is a no-op that causes the kernel to re-evaluate the set of
           pending signals.
           It is a work around for some lost signals I sometimes encounter.
           The real problem is most likely somewhere on the kernel side of things. */
        sigset_t mask;
        sigemptyset(&mask);
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
#endif
	poll_checkpoint(info); /* decrements counter as a side effect */
	return 0;
    }
    // else lock was acquired by the cri_read_trylock in the conditional.

    return 1;
}

// XXX: Unused 'id' argument should be used for debugging.
// XXX: we could also double check that (id == CR_ID_CALLBACK)
// if and only if the state is CR_STATE_ACTIVE.
//
// Returns the current state.
//
// Signal safe because if a signal takes us into the ACTIVE state
// it will also leave the ACTIVE state before we regain control.
//
// NOTE: Now "internal" rather than "private"
int
cri_do_enter(cri_info_t		*info,
             cr_client_id_t	id)
{
    int state;

    state = cri_atomic_read(&info->cr_state);

    if (state != CR_STATE_ACTIVE) {
	CRI_WHILE_COND_YIELD(!try_enter(info));
    }

    return state;
}

// XXX: Unused 'id' argument should be used for debugging.
// XXX: we could also double check that (id == CR_ID_CALLBACK)
// if and only if the state is CR_STATE_ACTIVE.
//
// Returns non-zero on successful entry
//
// Signal safe because if a signal takes us into the ACTIVE state
// it will also leave the ACTIVE state before we regain control.
//
// NOTE: Now "internal" rather than "private"
int
cri_do_tryenter(cri_info_t	*info,
		cr_client_id_t	id)
{
    return ((cri_atomic_read(&info->cr_state) == CR_STATE_ACTIVE) || try_enter(info));
}
	 
// XXX: Unused 'id' argument should be used for debugging.
//
// NOTE: Now "internal" rather than "private"
void
cri_do_leave(cri_info_t		*info,
	     cr_client_id_t	id)
{
    if (cri_atomic_read(&info->cr_state) != CR_STATE_ACTIVE) {
	cri_red_unlock(&cri_cs_lock);
	poll_checkpoint(info);
    }
}

// Not reentrant.  Must be called inside a critical section.
static cr_callback_id_t
cri_register_signal(cri_info_t*		info,
		    cr_callback_t	func,
		    void*		arg,
		    int			flags)
{
    return cri_do_register(info, func, arg, flags);
}

// Not reentrant.  Must be called inside a critical section.
static int
cri_replace_signal(cri_info_t		*info,
		   cr_callback_id_t	id,
		   cr_callback_t	func,
		   void*		arg,
		   int			flags)
{
    return cri_do_replace(info, id, func, arg, flags);
}

//
// "Public" data
//

//
// "Public" functions
//

// Not reentrant.  Must be called inside a critical section.
cr_callback_id_t
cri_do_register(cri_info_t	*info,
	        cr_callback_t	func,
	        void*		arg,
	        int		flags)
{
    int indx = info->cr_cb_count;

    if (indx < CR_MAX_CALLBACKS) {
	int next = indx + 1;

	// Grow on demand
	info->cr_cb = realloc(info->cr_cb, next * sizeof(*info->cr_cb));
	if (!info->cr_cb) {
	    // Assume that errno==ENOMEM as required Unix98 standard
	    return -1;
	}

	// Set entry before advancing the count
	info->cr_cb[indx].func  = func;
	info->cr_cb[indx].arg   = arg;
	info->cr_cb[indx].flags = flags;
	info->cr_cb_count = next;

	return indx;
    } else {
	errno = ENOSPC;
	return -1;
    }
}

// Note that we don't replace 'func', 'arg' and 'flags' atomically.
// Note that we ensure the context is not changed.
// Such atomicity is the responsibility of the caller.
//
// Not reentrant.  Must be called inside a critical section.
// Not exposed to user.
int
cri_do_replace(cri_info_t	*info,
	       cr_callback_id_t	id,
	       cr_callback_t	func,
	       void*		arg,
	       int		flags)
{
    int retval = -1;

    errno = EINVAL;
    if ((flags & CRI_CONTEXT_MASK) == (id & CRI_CONTEXT_MASK)) {
	id &= ~CRI_CONTEXT_MASK;
	if ((id >= 0) && (id < info->cr_cb_count)) {
	    info->cr_cb[id].func  = func;
	    info->cr_cb[id].arg   = arg;
	    info->cr_cb[id].flags = flags;
	    retval = 0;
	} else {
	    /* id out-of-bounds */
	}
    } else {
	/* Context mismatch */
    }

    return retval;
}

cr_callback_id_t
cr_register_callback(cr_callback_t	func,
		     void*		arg,
		     int		flags)
{
    cri_info_t *info = CRI_INFO_OR_RETURN(-1);	// thread-specific
    int context = 0; // silence a "used uninitialized" warning
    int retval = -1; // assume failure
    int state;

    state = cri_do_enter(info, CRI_ID_INTERNAL);

    errno = EBUSY;
    if (state == CR_STATE_ACTIVE) {
	// Called from a callback!  retval and errno are already set
	goto out;
    }

    errno = EINVAL;
    context = flags & CRI_CONTEXT_MASK;

    switch (context) {
    case CR_SIGNAL_CONTEXT:
	retval = cri_register_signal(info, func, arg, flags);
	break;
	
    case CR_THREAD_CONTEXT:
	retval = cri_register_thread(info, func, arg, flags);
	break;

    //default: Bad flags!  retval and errno are already set
    }

out:
    cri_do_leave(info, CRI_ID_INTERNAL);

    return (retval >= 0) ? (retval | context) : retval;
}

// Not reentrant.
int
cr_replace_callback(cr_callback_id_t	id,
		    cr_callback_t	func,
		    void*		arg,
		    int			flags)
{
    cri_info_t *info = CRI_INFO_OR_RETURN(-1);    // thread-specific
    int retval = -1;  // Assume failure
    int state;

    state = cri_do_enter(info, CRI_ID_INTERNAL);

    errno = EBUSY;
    if (state == CR_STATE_ACTIVE) {
	// Called from a callback!  retval and errno are already set
	goto out;
    }

    errno = EINVAL;
    switch (flags & CRI_CONTEXT_MASK) {
    case CR_SIGNAL_CONTEXT:
	retval = cri_replace_signal(info, id, func, arg, flags);
	break;
	
    case CR_THREAD_CONTEXT:
	retval = cri_replace_thread(info, id, func, arg, flags);
	break;

    //default: Bad flags!  retval and errno are already set
    }

out:
    cri_do_leave(info, CRI_ID_INTERNAL);

    return retval;
}

// Not reentrant.
int
cr_replace_self(cr_callback_t	func,
		void*		arg,
		int		flags)
{
    cri_info_t *info = CRI_CB_INFO_OR_RETURN(-1);	// thread-specific
    cr_callback_id_t id;

    /* form the full id from the index and context */
    id = info->run.id;
    id |= (info->cr_cb[id].flags & CRI_CONTEXT_MASK);

    return cri_do_replace(info, id, func, arg, flags);
}

int
cr_status(void)
{
    cri_info_t *info = CRI_INFO_OR_RETURN(-1);	// thread-specific

    return do_state(info);
}

// XXX: Unused 'id' argument should be used for debugging.
int
cr_enter_cs(cr_client_id_t id)
{
    cri_info_t *info = CRI_INFO_OR_RETURN(-1);	// thread-specific

    cri_do_enter(info, id);
    return 0;
}

// XXX: Unused 'id' argument should be used for debugging.
int
cr_tryenter_cs(cr_client_id_t id)
{
    cri_info_t *info = CRI_INFO_OR_RETURN(-1);	// thread-specific

    return !cri_do_tryenter(info, id);
}

// XXX: Unused 'id' argument should be used for debugging.
int
cr_leave_cs(cr_client_id_t id)
{
    cri_info_t *info = CRI_INFO_OR_RETURN(-1);	// thread-specific

    cri_do_leave(info, id);
    return 0;
}

// Return address of the per-checkpoint/per-thread info
const struct cr_checkpoint_info * cr_get_checkpoint_info(void)
{
    cri_info_t *info = cri_info_location();		// thread-specific
    struct cr_checkpoint_info *result;

    if (!info || cri_atomic_read(&info->cr_state) != CR_STATE_ACTIVE) {
	return NULL;
    }

    result =  &(info->cr_checkpoint_info);
    if (result->dest == NULL) {
	struct cr_chkpt_info tmp;
    	int rc;

	tmp.dest = info->path;
	rc = cri_syscall_token(info->run.token, CR_OP_HAND_CHKPT_INFO, (uintptr_t)&tmp);
	if (rc != 0) {
	    CRI_ABORT("CR_OP_HAND_CHKPT_INFO returned w/ errno=%d",
		      errno);
	}

	result->requester = tmp.requester;
	result->target = tmp.target;
	result->scope = tmp.scope;
	result->signal = tmp.signal;
	result->dest = tmp.dest;
	LIBCR_TRACE(LIBCR_TRACE_INFO, "requester=%d target=%d scope=%d signal=%d dest='%s'",
		    tmp.requester, tmp.target, tmp.scope, tmp.signal, tmp.dest);
    }

    return result;
}

// Return address of the per-restart/per-thread info
const struct cr_restart_info * cr_get_restart_info(void)
{
    cri_info_t *info = cri_info_location();		// thread-specific
    struct cr_restart_info *result;

    if (!info || cri_atomic_read(&info->cr_state) != CR_STATE_ACTIVE) {
	return NULL;
    }

    result =  &(info->cr_restart_info);
    result->requester = info->run.rc;	// Pid of cr_restart was returned from syscall

    if (result->src == NULL) {
    	int rc = cri_syscall_token(info->run.token, CR_OP_HAND_SRC, (uintptr_t)info->path);
	if (rc != 0) {
	    CRI_ABORT("CR_OP_HAND_SRC returned w/ errno=%d",
		      errno);
	}
	result->src = info->path;
    }
    return result;
}

// Return a client identifier used to invoke cr_{enter,leave}_cs().
// Also registers the handler on first call per thread.
// May later setup additional state.
cr_client_id_t cr_init(void)
{
    cri_info_t *info;
    int token;
    int rc;

    // First establish connection to verify that we have kernel support
    token = cri_connect();
    if (token < 0) {
	return token;
    }

    info = cri_info_init();		// thread-specific
    if (!info->handler) {
	// Set the handler
	info->handler = &my_handler;

	// Register ourself with the kernel
	do {
	    rc = cri_syscall_token(token, CR_OP_HAND_PHASE2, token);
	} while ((rc < 0) && (errno == EAGAIN));
	if (rc != 0) {
	    CRI_ABORT("CR_OP_HAND_PHASE2 returned w/ errno = %d", errno);
	}
    }

    return info->next_id++;
}

// Read and potentialy write info->hold
// This controls whether the post-handler barrier is blocking or not.
int cr_hold_ctrl(int scope, int flags)
{
    cri_info_t *info = CRI_INFO_OR_RETURN(-1);	// thread-specific
    cri_atomic_t *p;
    int oldval;

    switch (scope) {
    case CR_HOLD_SCOPE_THREAD:
	p = &info->hold;
	break;
    case CR_HOLD_SCOPE_INIT:
	p = &cri_hold_init;
	break;
    case CR_HOLD_SCOPE_UNINIT:
	p = &cri_hold_uninit;
	break;
    default:
	errno = EINVAL;
	return -1;
    }

    oldval = cri_atomic_read(p);
    if (flags == CR_HOLD_READ) {
	// Nothing more to do
    } else if (!(flags & ~CR_HOLD_BOTH) ||
	       ((scope == CR_HOLD_SCOPE_THREAD) && (flags == CR_HOLD_DFLT))) {
	/* ensure meaningful oldval in presence of threads and signals: */
	while (!cri_cmp_swap(p, oldval, flags)) { oldval = cri_atomic_read(p); }
    } else {
	errno = EINVAL;
	oldval = -1;
    }

    return oldval;
}
