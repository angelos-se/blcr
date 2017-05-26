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
 * $Id: cr_private.h,v 1.107.4.4 2012/12/21 07:21:25 phargrov Exp $
 */

#ifndef _CR_PRIVATE_H
#define _CR_PRIVATE_H	1

#include "blcr_config.h"
#include <features.h>

#include <sys/types.h>
#include <signal.h>		// for siginfo_t
#include <pthread.h>
#include <limits.h>		// for PATH_MAX

#ifndef _STRINGIFY
  #define _STRINGIFY_HELPER(x) #x
  #define _STRINGIFY(x) _STRINGIFY_HELPER(x)
#endif

// Name-shift the preload lib
#if defined(LIBCR_SIGNAL_ONLY)
 #define __cri_ksigaction crsig_ksigaction
 #if LIBCR_TRACING
  #define __cri_nanosleep	crsig_nanosleep
  #define __cri_sched_yield	crsig_sched_yield
  #define libcr_trace		crsig_trace
  #define libcr_trace_init	crsig_trace_init
  #define libcr_trace_mask	crsig_trace_mask
  #define cri_barrier_enter	crsig_barrier_enter
  #define cr_spinlock_init	crsig_spinlock_init
  #define cr_spinlock_lock	crsig_spinlock_lock
  #define cr_spinlock_trylock	crsig_spinlock_trylock
  #define cr_spinlock_unlock	crsig_spinlock_unlock
 #endif /* LIBCR_TRACING */
#endif /* defined(LIBCR_SIGNAL_ONLY) */

#include <libcr.h>
#include "cr_syscall.h"
#include "cr_trace.h"
#include "cr_atomic.h"
#include "cr_yield.h"
#include "cr_rb_lock.h"
#include "cr_arch.h"

// Default to using libc's sigaction (may override in cr_arch.h)
#ifndef CR_USE_SIGACTION
  #define CR_USE_SIGACTION 1
#endif
// Default if not provided in cr_arch.h
#ifndef cri_syscall3X
  #define cri_syscall3X cri_syscall3
#endif

// Used by internal critical sections
#define CRI_ID_INTERNAL ((cr_client_id_t)(-2))

typedef void (*cri_sighandler_t)(int sig, siginfo_t *info, void *context);

typedef struct cri_info_s {
    /* Current state */
    cri_atomic_t	cr_state;

    /* Count of critical sections */
    cri_atomic_t	cr_cs_count;

    /* Vector of registered checkpoint callbacks and their private data */
    unsigned int	cr_cb_count;
    struct {
	cr_callback_t		func;	// The function to invoke
	void*			arg;	// The argument to the function
	int			flags;	// The flags
    }			*cr_cb;

    /* What identifier to return next from cr_init() */
    cr_client_id_t	next_id;

    /* What signal handler to run for this thread? */
    cri_sighandler_t	handler;

    /* Are we the thread to run thread-context callbacks? */
    int			is_thread;

    /* Hold flags for CR_OP_HAND_DONE */
    cri_atomic_t	hold;

    /* Data needed to invoke all the callbacks in order */
    struct {
	int			token;
	int			index;	/* Counts down only */
	int			id;	/* Tracks id of running callback (down and up again) */
	int			rc;	/* Saves the return code */
    }			run;

    /* Thread-local info about the checkpoint request */
    struct cr_checkpoint_info	cr_checkpoint_info;

    /* Thread-local info about the restart request */
    struct cr_restart_info	cr_restart_info;

    /* Do we need to keep the info past thread destruction? */
    int			persist;

    /* Space for the cr_checkpoint_info.dest and cr_restart_info.src */
    char		path[PATH_MAX];
} cri_info_t;

// How many threads are actively trying to checkpoint (PENDING + ACTIVE)
extern cri_atomic_t cri_live_count;	// GLOBAL

// Red-black lock for critical sections
extern cri_rb_lock_t cri_cs_lock;	// GLOBAL

// Thread-specific data key for cr_info
extern pthread_key_t cri_info_key;	// GLOBAL

// Default values for hold flags
extern cri_atomic_t cri_hold_init;	// GLOBAL
extern cri_atomic_t cri_hold_uninit;	// GLOBAL

// Table of hooks and a macro to invoke them
extern volatile cr_hook_fn_t cri_hook_tbl[CR_NUM_HOOKS];	// GLOBAL
#if CRI_DEBUG
    #define CRI_CHECK_HOOK(_event) do {                \
	int _e2 = (int)(_event);                       \
	if ((_e2 < 0) || (_e2 >= CR_NUM_HOOKS)) {      \
	    CRI_ABORT("Invalid event number %d", _e2); \
	}                                              \
    } while (0)
#else
    #define CRI_CHECK_HOOK(_event) do {} while (0)
#endif
#define CRI_RUN_HOOK(_event) do { \
	int _e = (int)(_event);   \
	cr_hook_fn_t _fn;         \
	CRI_CHECK_HOOK(_e);       \
	_fn = cri_hook_tbl[_e];   \
	if (_fn) (_fn)(_e);       \
    } while (0)

// Location of the (possibly thread-specific) cr_info
#define cri_info_location()	((cri_info_t *)pthread_getspecific(cri_info_key))

// Error-handling wrappers for cri_info_location()
#define CRI_INFO_OR_RETURN(_val) ({                                           \
        cri_info_t *_tmp = cri_info_location();                               \
        if (!_tmp) {                                                          \
            LIBCR_TRACE(LIBCR_TRACE_INFO,                                     \
                        "[%d] FAIL w/ errno=CR_ENOINIT", getpid());           \
            errno = CR_ENOINIT;                                               \
            return (_val);                                                    \
        }                                                                     \
        _tmp;                                                                 \
    })
#define CRI_CB_INFO_OR_RETURN(_val) ({                                        \
        cri_info_t *_tmp = cri_info_location();                               \
        if (!_tmp || (cri_atomic_read(&_tmp->cr_state) != CR_STATE_ACTIVE)) { \
            LIBCR_TRACE(LIBCR_TRACE_INFO,                                     \
                        "[%d] FAIL w/ errno=CR_ENOTCB", getpid());            \
            errno = CR_ENOTCB;                                                \
            return (_val);                                                    \
        }                                                                     \
        _tmp;                                                                 \
    })

// Start the checkpoint
extern void cri_start_checkpoint(cri_info_t *info);

// Perform pthreads-related initialization if needed.
extern void cri_pthread_init(void);
#if PIC && HAVE___REGISTER_ATFORK
  extern int cri_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void));
#else
  #define cri_atfork pthread_atfork
#endif

// Register a callback
extern cr_callback_id_t cri_do_register(cri_info_t*, cr_callback_t, void*, int);
extern cr_callback_id_t cri_register_thread(cri_info_t*, cr_callback_t, void*, int);

// Replace a callback
extern int cri_do_replace(cri_info_t*, cr_callback_id_t, cr_callback_t, void*, int);
extern int cri_replace_thread(cri_info_t*, cr_callback_id_t, cr_callback_t, void*, int);

// Enter/leave a critical section
extern int cri_do_enter(cri_info_t *info, cr_client_id_t id);
extern int cri_do_tryenter(cri_info_t *info, cr_client_id_t id);
extern void cri_do_leave(cri_info_t *info, cr_client_id_t id);

// Create or destroy cri_info_t
extern cri_info_t* cri_info_init(void);
extern void cri_info_free(void *);

// Initialize checkpoint info
extern void cri_checkpoint_info_init(cri_info_t *info);

// cr_sig_sync.c
extern int cri_barrier_enter(cri_atomic_t *x);

// libc/libpthread internal routines:
extern int __libc_current_sigrtmax(void);
extern int __libc_allocate_rtsig(int);
extern pid_t __fork(void);
extern void __nss_disable_nscd(void *);

// Alternate signal handlers
extern void cri_run_sig_handler(int, siginfo_t *, void *);
extern void cri_omit_sig_handler(int, siginfo_t *, void *);

// FTB support
#if HAVE_FTB && !defined(LIBCR_SIGNAL_ONLY)
  #include <libftb.h>
  extern int cri_ftb_init(const char *client_name, const char *client_jobid);
  extern int cri_ftb_fini(void);
  extern int cri_ftb_event(FTB_event_handle_t *event_handle, const char *event_name, int len, const void *data);
  extern int cri_ftb_event2(FTB_event_handle_t *event_handle, const FTB_event_handle_t *orig_handle, const char *event_name, int len, const void *data);
#endif

// Magic voodoo to produce weak aliases with gcc > 2.7
#ifndef weak_alias
# define weak_alias(name, aname) \
  extern __typeof (name) aname __attribute__ ((weak, alias (#name)));
#endif

// Macros for making calls and looping on negative return w/ errno == EINTR
#define CRI_EINTR_LOOP(__the_call) \
	while (((__the_call) < 0) && (errno == EINTR)) { /* empty */ }
#define __CRI_EINTR_LOOP(__the_call, __errno_p) \
	while (((__the_call) < 0) && (*(__errno_p) == EINTR)) { /* empty */ }

#endif
