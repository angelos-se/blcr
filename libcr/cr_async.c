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
 * $Id: cr_async.c,v 1.60.8.1 2009/02/14 02:55:37 phargrov Exp $
 *
 * This file deals with the checkpoint thread(s) run to execute callbacks
 * registered as CR_THREAD_CONTEXT.
 * The filename "cr_async.c" is legacy from when these callbacks were
 * known as "asynchronous handlers".
 */

#include <signal.h>	// for sigfillmask()
#include <errno.h>	// for errno
#include <sys/types.h>	// for pid_t
#include <unistd.h>	// for getpid() and sysconf()

#include "cr_private.h"

//
// Private data
//

enum {
    CRI_THREAD_STOPPED,
    CRI_THREAD_STARTING,
    CRI_THREAD_RUNNING,
};

// Stuff for the callback thread, all protected by thread_lock:
static cri_info_t	*thread_info_p;
static pthread_mutex_t	thread_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t	thread_init_cond = PTHREAD_COND_INITIALIZER;
static int		thread_state = CRI_THREAD_STOPPED;
static pthread_t	my_thread;

//
// Private functions
//

// my_handler()
//
// Signal handler for threads running thread-context callbacks
static void my_handler(int signr, siginfo_t *siginfo, void *context)
{
    int saved_errno = errno;
    cri_info_t *info = cri_info_location();	// thread-specific

    cri_checkpoint_info_init(info);

    // Simply enter the PENDING state
    // The thread will then wake up and run the callback(s)
    if (cri_cmp_swap(&info->cr_state, CR_STATE_IDLE, CR_STATE_PENDING)) {
	cri_atomic_inc(&cri_live_count);
    }

    errno = saved_errno;
}

// thread_reset()
//
// This is a atfork callback to reset the state in the child process
static void thread_reset(void)
{
	pthread_mutex_init(&thread_lock, NULL);
	pthread_cond_init(&thread_init_cond, NULL);
	thread_state = CRI_THREAD_STOPPED;
}

// thread_main()
//
// This is the main loop of a thread running thread-context callbacks
static void* thread_main(void* arg)
{
#if LIBCR_TRACING
    int pid = (int)getpid();
#endif
    int token;
    cri_info_t *info;
    int rc;

    // Find the shared fd
    token = cri_connect();
    if (token < 0) {
	CRI_ABORT("cri_connect() failed");
    }

    // Reduce false wake-ups by blocking all signals.
    {
    sigset_t mask;
    sigfillset(&mask);
    pthread_sigmask(SIG_SETMASK, &mask, NULL);
    }

    // Tell the kernel code we are a Phase I thread.
    //
    // If we race against a checkpoint request, then after the
    // checkpoint has been taken, the cr_syscall() fails w/
    // (errno == EAGAIN) and we try again.
    do {
	rc = cri_syscall_token(token, CR_OP_HAND_PHASE1, token);
    } while ((rc < 0) && (errno == EAGAIN));
    if (rc != 0) {
	CRI_ABORT("CR_OP_HAND_PHASE1 failed w/ errno = %d", errno);
    }

    // Initialize our cri_info as a PHASE1 thread.
    // The write to 'handler' is naturally an atomic operation.
    //
    // If a request arrives any time before 'info->handler'
    // is set, then the kernel and/or the signal handler will still
    // think we are a PHASE II thread and we'll respond to the request
    // from signal context.  This is acceptable since we cannot possibly
    // have any callbacks registered until after we signal the condition
    // variable and subsequently release the thread_lock.
    info = cri_info_init();
    info->is_thread = 1;
    info->handler = &my_handler;

    LIBCR_TRACE(LIBCR_TRACE_INFO, "[%d] started checkpoint thread", pid);

    // Wakeup the thread(s) blocked waiting for our initialization.
    // They can register callbacks only after we release thread_lock.
    //
    // NOTE: For the purpose of avoiding "bug2520 deadlock", this use
    // of pthread_mutex_lock() is equivalent to holding a critical
    // section because we entered "deferred" mode above when we set
    // info->handler.
    pthread_mutex_lock(&thread_lock);
    thread_info_p = info;
    thread_state = CRI_THREAD_RUNNING;
    pthread_cond_broadcast(&thread_init_cond);
    pthread_mutex_unlock(&thread_lock);

    while (1) {
	// Block until the next request (or pthread cancellation) arrives
	int rc = cri_syscall_token(token, CR_OP_HAND_SUSP, (uintptr_t)NULL);
	LIBCR_TRACE(LIBCR_TRACE_INFO, "[%d] checkpoint thread woke up", pid);
	if (rc && (errno != EINTR)) {
	  CRI_ABORT("[%d] checkpoint thread wait failed (rc=%d errno=%d)", (int)getpid(), rc, errno);
	}

	// Check for cancellation.
	pthread_testcancel();

	// Get locks
	cri_black_lock(&cri_cs_lock);
	// NOTE: For the purpose of avoiding "bug2520 deadlock", this use
	// of pthread_mutex_lock() is safe because cri_black_lock() has
	// ensured that no critical sections are held in any thread.
	pthread_mutex_lock(&thread_lock);

	// Respond to a request iff one is pending, does nothing otherwise.
	cri_start_checkpoint(info);

	// Release locks
	pthread_mutex_unlock(&thread_lock);
	cri_black_unlock(&cri_cs_lock);
    }

    // NOT REACHED
    return NULL;
}

// thread_init()
// 
// Ensure that the thread is started exactly once.
// Note that more than one thread might get here between the time the
// first enters pthread_cond_wait(), thus dropping the lock, and the
// time the thread gets far enough through its initialization
// to acquire the lock and signal the condition variable.
// 
// Called w/ thread_lock held and inside a critical section.
static void thread_init(void)
{
    if (thread_state != CRI_THREAD_RUNNING) {
	if (thread_state == CRI_THREAD_STOPPED) {
	    int rc;

	    rc = cri_atfork(NULL, NULL, &thread_reset);
	    if (rc != 0) {
		CRI_ABORT("cri_atfork() returned %d", rc);
	    }

	    rc = pthread_create(&my_thread, NULL, &thread_main, NULL);
#if HAVE_PTHREAD_ATTR_SETSTACKSIZE
	    if (rc == ENOMEM) {
		// As per bug 2322, we try to use a smaller stack
		const uintptr_t pagemask = sysconf(_SC_PAGESIZE) - 1;
		pthread_attr_t my_attr;
		size_t size;

		// Start at half the default stack limit
		rc = pthread_attr_init(&my_attr);
		if (rc != 0) {
		    CRI_ABORT("pthread_attr_init() returned %d", rc);
		}
		rc = pthread_attr_getstacksize(&my_attr, &size);
		if (rc != 0) {
		    CRI_ABORT("pthread_attr_getstacksize() returned %d", rc);
		}
		size = ((size >> 1) + pagemask) & ~pagemask; // half and round

		// MAX(current, 4MB, PTHREAD_STACK_MIN)
		if (size < 4 * 1024 * 1024) size = 4 * 1024 * 1024;
		if (size < PTHREAD_STACK_MIN) size = PTHREAD_STACK_MIN;

		do {
		    (void)pthread_attr_setstacksize(&my_attr, size);
		    rc = pthread_create(&my_thread, &my_attr, &thread_main, NULL);
		    size = ((size >> 1) + pagemask) & ~pagemask; // half and round
		} while ((rc == ENOMEM) && (size >= PTHREAD_STACK_MIN));

		(void)pthread_attr_destroy(&my_attr);
	    }
#endif
	    if (rc != 0) {
		CRI_ABORT("pthread_create() returned %d", rc);
	    }

	    thread_state = CRI_THREAD_STARTING;
	}

	// Wait for the thread to complete initialization before returning.
	while (thread_state != CRI_THREAD_RUNNING) {
	    pthread_cond_wait(&thread_init_cond, &thread_lock);
	}
    }
}

//
// Internal functions
//

// Not reentrant, but thread safe.
// Must be called inside a critical section.
cr_callback_id_t
cri_register_thread(cri_info_t		*info,
		    cr_callback_t	func,
		    void*		arg,
		    int			flags)
{
    cr_callback_id_t	result;

    pthread_mutex_lock(&thread_lock);
    thread_init();
    result = cri_do_register(thread_info_p, func, arg, flags);
    pthread_mutex_unlock(&thread_lock);

    return result;
}

// Not reentrant, but thread safe.
// Must be called inside a critical section.
int
cri_replace_thread(cri_info_t		*info,
		   cr_callback_id_t	id,
		   cr_callback_t	func,
		   void*		arg,
		   int			flags)
{
    int retval;

    pthread_mutex_lock(&thread_lock);
    retval = cri_do_replace(thread_info_p, id, func, arg, flags);
    pthread_mutex_unlock(&thread_lock);

    return retval;
}
