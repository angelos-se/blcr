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
 * $Id: cr_request.c,v 1.222.4.2 2014/09/18 23:46:08 phargrov Exp $
 *
 * Code for clients to request checkpoints and restarts, poll and forward them.
 */

#define _LARGEFILE64_SOURCE 1   /* For O_LARGEFILE in LEGACY interfaces */

#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>

#include "cr_private.h"

//
// Private variables
//

//
// Public variables
//

//
// Private functions
//

/* Accessors for fd "component" of the handle.
 * These are trivial now, but might eventually become less so.
 */
static inline
int cri_chkpt_hndl_get_token(cr_checkpoint_handle_t *handle) {
	return (int)*handle;
}
static inline
int cri_rstrt_hndl_get_token(cr_restart_handle_t *handle) {
	return (int)*handle;
}
static inline
void cri_chkpt_hndl_set_token(cr_checkpoint_handle_t *handle, int token) {
	*handle = token;
}
static inline
void cri_rstrt_hndl_set_token(cr_restart_handle_t *handle, int token) {
	*handle = token;
}

static inline
int do_wait_fd(int fd, struct timeval *timeout) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    return select(fd+1, &rfds, NULL, NULL, timeout);
}

/* This replaced cri_initialize_checkpoint_args_t() in 0.7.0 */
/* This is "semi-private" */
int cri_init_checkpoint_args_t(cr_version_t ver, cr_checkpoint_args_t *cr_args)
{
    cr_args->cr_version = ver;
    switch (ver) {
#if 0 // Not yet
    case 2:
	Set fields added in version 2
	// fall through to get older fields...
#endif
    case 1: // Interface as of 0.6.0.  Still current in 0.7.0
	cr_args->cr_scope   = -1;	// Invalid - user must set
	cr_args->cr_target  = 0;	// Default target of zero is 'self'
	cr_args->cr_fd      = -1;	// Invalid - user must set
	cr_args->cr_signal  = 0;	// Default is no signal
	cr_args->cr_timeout = 0;	// Default is unbounded
	cr_args->cr_flags   = 0;	// Default is no special behaviors
	break;

    default: // Unknown/unsupported version
	errno = EINVAL;
	return -1;
    }
    return 0;
}

/* This is "semi-private" */
int cri_init_restart_args_t(cr_version_t ver, cr_restart_args_t *cr_args)
{
    cr_args->cr_version = ver;
    switch (ver) {
#if 0 // Not yet
    case 2:
	Set fields added in version 2
	// fall through to get older fields...
#endif
    case 1:
	cr_args->cr_fd       = -1;	// Invalid - user must set
	cr_args->cr_signal   = 0;	// Default is no signal
	cr_args->cr_flags    = CR_RSTRT_RESTORE_PID;	// Default is no special behaviors
	cr_args->cr_relocate = NULL;	// Default is no relocations
	break;

    default: // Unknown/unsupported version
	errno = EINVAL;
	return -1;
    }
    return 0;
}

static int cri_rstrt_hndl_child_inner(void *arg) {
    int token = (uintptr_t)arg;
    int local_errno;

    /* Now overlay ourselves with the new image */
    (void) __cri_syscall_token(token, CR_OP_RSTRT_CHILD, CRI_SYSCALL_NOARG, &local_errno);

    /* Not reached unless we've encountered a fatal error.
     * ********** Things are hairy here **********
     * We may have an incomplete thread environment, so we can't use libc fully.
     */

    /* Don't even try to print an error message */

    /* Try to make the entire thread group exit, if supported */
    __cri_exit_group(local_errno, NULL);
    /* We might return here in the case of ENOSYS */

    /* Note that this is really _exit() */
    __cri_exit(local_errno, NULL);
    /* NOTREACHED */

    return 0;
}

static int cri_rstrt_hndl_child_main(int token, int threads, int flags) {
    struct sigaction sa;
    uintptr_t incr = sysconf(_SC_PAGESIZE);
    uintptr_t stack;
    int err;
    int i;

    /* To help close the race between creation of our children and
     * any checkpoint requests, they need to OMIT.  However, other than
     * the first, they don't have a full pthreads library available
     * (e.g. to pthread_getspecific) to run the normal signal handler.
     */
    sa.sa_sigaction = cri_omit_sig_handler;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    (void)sigfillset(&sa.sa_mask);
    (void)sigaction(CR_SIGNUM, &sa, NULL); /* XXX: Add error checking */

    if (threads > 1) {
	stack = (uintptr_t)mmap(0, (threads - 1) * incr, PROT_READ | PROT_WRITE | PROT_EXEC,
				MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	if ((void *)stack == MAP_FAILED) {
	    fprintf(stderr, "Failed to allocate stack space for child thread(s)\n");
	    exit(1);
	}
#if (CR_STACK_GROWTH < 0) /* Stack grows down */
	stack += incr;
#elif (CR_STACK_GROWTH > 0) /* Stack grows up */
	/* Nothing to do */
#else
	#error "Don't know which way the stack grows"
#endif

        for (i = 1; i < threads; ++i) {
	    err = clone(&cri_rstrt_hndl_child_inner, (void *)stack, flags, (void *)(uintptr_t)token);
	    if (err < 0) {
	        fprintf(stderr, "clone() failed while creating child thread(s)\n");
	        exit(1);
	    }
	    stack += incr;
	}
    }

    cri_rstrt_hndl_child_inner((void *)(uintptr_t)token);
    /* NOTREACHED */

    return 0;
}

static int do_fetch_log(int token, int op, char **msg_p)
{
    struct cr_log_args req = { 0, NULL };
    int rc;

    rc = cri_syscall_token(token, op, (uintptr_t)&req);
    if (rc <= 0) {
	/* Error (<0) or empty (0) */
	return rc;
    }

    req.len = rc;
    req.buf = malloc(rc);
    if (!req.buf) {
	errno = ENOMEM;
	return -1;
    }

    rc = cri_syscall_token(token, op, (uintptr_t)&req);
    if (rc < 0) {
	free(req.buf);
        return -1;
    }

    *msg_p = req.buf;
    return 0;
}

#if HAVE_FTB
static FTB_event_handle_t cri_my_event;
static void my_log_event(const char *name) {
    // Note: we don't preserve errno here since our callers clobber it
    cri_info_t *info = cri_info_init();
    (void)cri_do_enter(info, CRI_ID_INTERNAL);
    if (!cri_ftb_init(NULL,NULL)) {
	(void)cri_ftb_event(&cri_my_event, name, 0, NULL);
	(void)cri_ftb_fini();
    }
    cri_do_leave(info, CRI_ID_INTERNAL);
}
static void my_log_event2(const char *name) {
    int save_errno = errno;
    cri_info_t *info = cri_info_init();
    (void)cri_do_enter(info, CRI_ID_INTERNAL);
    if (!cri_ftb_init(NULL,NULL)) {
	(void)cri_ftb_event2(NULL, &cri_my_event, name, 0, NULL);
	(void)cri_ftb_fini();
    }
    cri_do_leave(info, CRI_ID_INTERNAL);
    errno = save_errno;
}
static void my_log_error(const char *name) 
{
    int32_t save_errno = errno;
    cri_info_t *info = cri_info_init();
    (void)cri_do_enter(info, CRI_ID_INTERNAL);
    if (!cri_ftb_init(NULL,NULL)) {
	(void)cri_ftb_event2(NULL, &cri_my_event, name, 4, &save_errno);
	(void)cri_ftb_fini();
    }
    cri_do_leave(info, CRI_ID_INTERNAL);
    errno = save_errno;
}
#endif

//
// Public functions
//

int cr_request_checkpoint(cr_checkpoint_args_t *args,
                          cr_checkpoint_handle_t *handle)
{
    struct cr_chkpt_args req;
    int rc;
    int token;
#if LIBCR_TRACING
    int pid = (int)getpid();
#endif

    rc = -1;
    errno = EINVAL;
    if (args->cr_version != CR_CHECKPOINT_ARGS_VERSION) {
	// XXX: Only support a single version at the moment
        goto out;
    }

    // initialize the handle
    token = cri_connect_token();
    if (token < 0) {
        goto out;
    }
    cri_chkpt_hndl_set_token(handle, token);

    // initialize the request
    req.cr_target = args->cr_target;
    req.cr_scope  = args->cr_scope;
    req.cr_fd     = args->cr_fd;
    req.cr_secs   = args->cr_timeout;
    req.dump_format = cr_format_vmadump;
    req.signal    = args->cr_signal;
    req.flags     = args->cr_flags;

#if HAVE_FTB
    (void)my_log_event("CHKPT_BEGIN");
#endif

    // issue the request
    LIBCR_TRACE(LIBCR_TRACE_INFO, "[%d] requesting the checkpoint.", pid);
    rc = cri_syscall_token(token, CR_OP_CHKPT_REQ, (uintptr_t)&req);
    if (rc < 0) {
        cri_disconnect_token(token); /* Might silently fail? */
	cri_chkpt_hndl_set_token(handle, -1);
        goto out;
    }

out:
#if HAVE_FTB
    if (rc < 0) {
	my_log_error("CHKPT_ERROR");
    }
#endif
    LIBCR_TRACE(LIBCR_TRACE_INFO, "[%d] leaving with rc=%d.", pid, rc);
    return rc;
}

int cr_forward_checkpoint(cr_scope_t cr_scope, pid_t cr_target)
{
    cri_info_t *info;
    int rc;
    struct cr_fwd_args req;
#if LIBCR_TRACING
    int pid = (int)getpid();
#endif

    // Checks for improper calls
    info = CRI_CB_INFO_OR_RETURN(-1);	// thread-specific

    req.cr_scope  = cr_scope;
    req.cr_target = cr_target;

    // forward the checkpoint
    LIBCR_TRACE(LIBCR_TRACE_INFO, "[%d] forwarding checkpoint.", pid);
    rc = cri_syscall_token(info->run.token, CR_OP_CHKPT_FWD, (uintptr_t)&req);
    if (rc < 0) {
        goto out;
    }

out:
    LIBCR_TRACE(LIBCR_TRACE_INFO, "[%d] leaving with rc=%d.", pid, rc);
    return rc;
}

// Block until the checkpoint is complete or we exceed the timeout
int cr_wait_checkpoint(cr_checkpoint_handle_t *handle, struct timeval *timeout)
{
    return do_wait_fd(cri_chkpt_hndl_get_token(handle), timeout);
}

// Collect log messages
int cr_log_checkpoint(cr_checkpoint_handle_t *handle, unsigned int len, char *msg)
{
    int token = cri_chkpt_hndl_get_token(handle);
    struct cr_log_args req = { len, msg };
    return cri_syscall_token(token, CR_OP_CHKPT_LOG, (uintptr_t)&req);
}

// Collect the result
int cr_reap_checkpoint(cr_checkpoint_handle_t *handle)
{
    int token = cri_chkpt_hndl_get_token(handle);
    int err = cri_syscall_token(token, CR_OP_CHKPT_REAP, CRI_SYSCALL_NOARG);

    if ((err < 0) && (errno == ENOTTY)) {
	/* "Bad ioctl()" - this is NOT our handle.  So, don't close it. */
    } else {
	cri_disconnect_token(token); /* Might silently fail? */
	cri_chkpt_hndl_set_token(handle, -1);
    }

#if HAVE_FTB
    if (err >= 0) {
	(void)my_log_event2("CHKPT_END");
    } else if ((errno != CR_ERESTARTED) && (errno != EINTR)) {
	(void)my_log_error("CHKPT_ERROR");
    }
#endif

    return err;
}

// Block until the checkpoint request is complete or we exceed the timeout
// then collect log messages and reap result/status
int cr_poll_checkpoint_msg(cr_checkpoint_handle_t *handle, struct timeval *timeout, char **msg_p)
{
    int token = cri_chkpt_hndl_get_token(handle);
    int err;

    /* Wait/poll for the request to complete */
    err = do_wait_fd(token, timeout);
    if (!err) {
	return 0; /* Not done */
    } else if (err < 0) {
#if HAVE_FTB
	if (errno != EINTR) {
	    my_log_error("CHKPT_ERROR");
	}
#endif
        return CR_POLL_CHKPT_ERR_PRE;
    } /* fall though on > 0 */

    /* Fetch the message log if requested */
    if (msg_p && (do_fetch_log(token, CR_OP_CHKPT_LOG, msg_p) < 0)) {
	return CR_POLL_CHKPT_ERR_LOG;
    }

    /* Reap the completed request */
    err = cr_reap_checkpoint(handle);
    return (err < 0) ? CR_POLL_CHKPT_ERR_POST : 1;
}

int cr_poll_checkpoint(cr_checkpoint_handle_t *handle, struct timeval *timeout) {
    return cr_poll_checkpoint_msg(handle, timeout, NULL);
}


int cr_request_restart(cr_restart_args_t *args, cr_restart_handle_t *handle)
{
    struct cr_rstrt_args req;
    struct cr_procs_tbl procs;
    int token;
    int err;

    errno = EINVAL;
    if (args->cr_version != CR_RESTART_ARGS_VERSION) {
	// XXX: Only support a single version at the moment
        return -1;
    }

    token = cri_connect_token();
    if (token < 0) {
	return token;
    }
    cri_rstrt_hndl_set_token(handle, token);

    /* ... initialize the request from user's args */
    req.cr_fd     = args->cr_fd;
    req.signal    = args->cr_signal;
    req.flags     = args->cr_flags;
    req.relocate  = args->cr_relocate;

#if HAVE_FTB
    (void)my_log_event("RSTRT_BEGIN");
#endif

    /* ... tell the kernel to go off and restart something */
    err = cri_syscall_token(token, CR_OP_RSTRT_REQ, (uintptr_t)&req);
    if (err < 0) {
        /* The restart request was unsuccessful.  */
        goto fail;
    }

    /* ... fork child(ren) to contain the restarted task(s)
     * Note that CR_OP_RSTRT_PROCS might block if it cannot yet
     * be determined if more processes are needed.
     */ 
    while ((err = cri_syscall_token(token, CR_OP_RSTRT_PROCS, (uintptr_t)&procs)) > 0) {
        err = fork();
        if (err < 0) {
            /* The fork was unsuccessful.  */
	    goto fail;
        } else if (err == 0) {
	    cri_rstrt_hndl_child_main(token, procs.threads, procs.clone_flags);
	    /* NOTREACHED */
	}
    }
    if (err < 0) {
        goto fail;
    }

    return 0;

fail:
#if HAVE_FTB
    my_log_error("RSTRT_ERROR");
#endif
    cri_disconnect_token(token); /* Might silently fail? */
    cri_rstrt_hndl_set_token(handle, -1);
    return -1;
}

// Block until the restart is complete or we exceed the timeout
int cr_wait_restart(cr_restart_handle_t *handle, struct timeval *timeout)
{
    return do_wait_fd(cri_rstrt_hndl_get_token(handle), timeout);
}

// Collect log messages
int cr_log_restart(cr_restart_handle_t *handle, unsigned int len, char *msg)
{
    int token = cri_rstrt_hndl_get_token(handle);
    struct cr_log_args req = { len, msg };
    return cri_syscall_token(token, CR_OP_RSTRT_LOG, (uintptr_t)&req);
}

// Collect the result
int cr_reap_restart(cr_restart_handle_t *handle)
{
    int token = cri_rstrt_hndl_get_token(handle);
    int err = cri_syscall_token(token, CR_OP_RSTRT_REAP, CRI_SYSCALL_NOARG);

    if ((err < 0) && (errno == ENOTTY)) {
	/* "Bad ioctl()" - this is NOT our handle.  So, don't close it. */
    } else {
	cri_disconnect_token(token); /* Might silently fail? */
	cri_rstrt_hndl_set_token(handle, -1);
    }

#if HAVE_FTB
    if (err >= 0) {
	(void)my_log_event2("RSTRT_END");
    } else if (errno != EINTR) {
	(void)my_log_error("RSTRT_ERROR");
    }
#endif

    return err;
}

// Block until the restart request is complete or we exceed the timeout
// then collect log messages and reap result/status
int cr_poll_restart_msg(cr_restart_handle_t *handle, struct timeval *timeout, char **msg_p)
{
    int token = cri_rstrt_hndl_get_token(handle);
    int err;

    /* Wait/poll for the request to complete */
    err = do_wait_fd(token, timeout);
    if (!err) {
	return 0; /* Not done */
    } else if (err < 0) {
#if HAVE_FTB
	if (errno != EINTR) {
	    my_log_error("RSTRT_ERROR");
	}
#endif
        return CR_POLL_RSTRT_ERR_PRE;
    } /* fall though on > 0 */

    /* Fetch the message log if requested */
    if (msg_p && (do_fetch_log(token, CR_OP_RSTRT_LOG, msg_p) < 0)) {
	return CR_POLL_RSTRT_ERR_LOG;
    }

    /* Reap the completed request */
    err = cr_reap_restart(handle);
    return (err < 0) ? CR_POLL_RSTRT_ERR_POST : err;
}

int cr_poll_restart(cr_restart_handle_t *handle, struct timeval *timeout) {
    return cr_poll_restart_msg(handle, timeout, NULL);
}

//
// LEGACY checkpoint request interfaces, in terms of the current ones:
//

#include <fcntl.h>
#include <string.h>

#ifndef O_LARGEFILE
  #define O_LARGEFILE 0
#endif

static void cri_request(int fd, const char *filename, int scope)
{
    /* Static data: */
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    static char prev_name[PATH_MAX];
    static cr_checkpoint_handle_t cr_handle = (cr_checkpoint_handle_t)(-1);

    int rc;
    cr_checkpoint_args_t cr_args;

    /* Serialize */
    pthread_mutex_lock(&lock);

    cr_initialize_checkpoint_args_t(&cr_args);
    cr_args.cr_scope = scope;
    cr_args.cr_fd    = fd;

    // If there is a previous request un-reaped then reap it now.
    if ((int)cr_handle >= 0) {
	do {
	    rc = cr_poll_checkpoint(&cr_handle, NULL);
	} while ((rc < 0) && (errno == EINTR));
        if (rc < 0) {
	    if ((rc == CR_POLL_CHKPT_ERR_POST) && (errno == CR_ERESTARTED)) {
                // Previous request was invalidated across a restart - not an error.
            } else if (prev_name[0]) {
                // Failed - cleanup file from previous failure
                LIBCR_TRACE(LIBCR_TRACE_INFO, "Removing supposedly bogus context file "
                            "(%s), because rc=%d and errno=%d (%s)",
                            prev_name, rc, errno, cr_strerror(errno));
		if (!strcmp(prev_name, filename)) {
		    /* Don't remove the NEW file */
                } else if (unlink(prev_name) < 0) {
                    CRI_ABORT("unable to remove aborted/failed checkpoint file %s: %s\n",
                              prev_name, strerror(errno));
                }
            }
	} else if (rc == 0) {
	    CRI_ABORT("unexpected/impossible zero return from cr_poll_checkpoint");
	}
    }

    rc = cr_request_checkpoint(&cr_args, &cr_handle);
    if (rc) {
        CRI_ABORT("cr_request_checkpoint returned %d w/ errno=%d (%s)\n", rc, errno, cr_strerror(errno));
    }

    // Don't return until the request is either pending or completed.
    // This is important; otherwise we would have races with critical
    // sections which follow the cr_request() call.
    //
    // We can detect pending state because the live count is non-zero.
    // We can detect completed because a reap will succeed (or fail
    // with an errno other than EAGAIN).
    // When restarting we expect the reap to fail w/
    //    errno = CR_ERESTARTED if the fd is restored normally
    //    errno = ENOTTY if the fd is restored to something else
    //    errno = EBADF if the fd is not restored at all
    rc = 0;
    CRI_WHILE_COND_YIELD(
	!cri_atomic_read(&cri_live_count) &&
	((rc = cri_syscall_token((int)cr_handle, CR_OP_CHKPT_REAP, CRI_SYSCALL_NOARG)) < 0) &&
	errno == EAGAIN
    );
    LIBCR_TRACE(LIBCR_TRACE_INFO,
		"Reap loop finished with rc=%d and errno=%d (%s)",
		rc, errno, cr_strerror(errno));

    if (filename) {
        /* Remove checkpoint file if an unexpected error occurred (see above) */
        if (rc && (errno != EAGAIN) && (errno != CR_ERESTARTED) && (errno != ENOTTY) && (errno != EBADF)) {
	    LIBCR_TRACE(LIBCR_TRACE_INFO, "Removing supposedly bogus context file "
		        "(%s), because rc=%d and errno=%d (%s)",
		        filename, rc, errno, cr_strerror(errno));
	    if (unlink(filename) < 0) {
	        CRI_ABORT("unable to remove aborted/failed checkpoint file %s: %s\n", 
		          filename, strerror(errno));
	    }
	    prev_name[0] = '\0';
	} else {
	    /* Save name for possible later clean up */
	    strncpy(prev_name, filename, PATH_MAX);
	}
    } else {
	prev_name[0] = '\0';
    }

    pthread_mutex_unlock(&lock);
}

static void cri_request_file(const char *filename)
{
    int fd;

    CRI_EINTR_LOOP((fd = __cri_open(filename,
				    O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE,
				    0600, &errno)));
    if (fd < 0) {
        CRI_ABORT("open(%s)", filename);
    }
    cri_request(fd, filename, CR_SCOPE_PROC);
    CRI_EINTR_LOOP(__cri_close(fd, &errno));        /* silent failure possible */
}

// Request an application-initiated checkpoint
// The actual processing might be delayed by critical sections.
//
// Thread-safe, but not reentrant
void cr_request_fd(int fd)
{
    if (!cri_info_location()) {
        CRI_ABORT("called w/o first calling cr_init()");
    }

    cri_request(fd, NULL, CR_SCOPE_PROC);
}

// Request an application-initiated checkpoint of own process
// The actual processing might be delayed by critical sections.
//
// Thread-safe, but not reentrant
// XXX: error handling needs work (ABORT is not the best policy)
void cr_request_file(const char *filename)
{
    if (!cri_info_location()) {
        CRI_ABORT("called w/o first calling cr_init()");
    }

    cri_request_file(filename);
}

// Request an application-initiated checkpoint
// The actual processing might be delayed by critical sections.
//
// Thread-safe, but not reentrant
void cr_request(void)
{
    #define CONTEXT_NAMELEN 16 /* context.XXXXX + \0 + extra */
    char filename[CONTEXT_NAMELEN];
    if (!cri_info_location()) {
        CRI_ABORT("called w/o first calling cr_init()");
    }
    snprintf(filename, CONTEXT_NAMELEN, "context.%d", (int)getpid());
    cri_request_file(filename);
    #undef CONTEXT_NAMELEN
}
