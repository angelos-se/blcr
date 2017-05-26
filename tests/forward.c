/*
 * Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
 * 2003, The Regents of the University of California, through Lawrence
 * Berkeley National Laboratory (subject to receipt of any required
 * approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * Portions may be copyrighted by others, as may be noted in specific
 * copyright notices within specific files.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: forward.c,v 1.7 2008/08/29 21:36:53 phargrov Exp $
 *
 * Simple example for using crut
 */

#define _GNU_SOURCE     /* To get prototype for getpgid() */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "crut.h"
#include "crut_util.h"

#include "libcr.h"

/*
 * What this test is supposed to do.
 *
 * 1.  Make sure that checkpoint is sent to child
 *     a.  From parent -- phase 1 handler
 *     b.  From parent -- phase 2 handler
 * 2.  Ensure that the child handlers are run
 *     a.  No handlers (XXX: not implemented.)
 *     b.  Phase 1 handler
 *     c.  Phase 2 handler
 * 3.  Test all the scope arguments
 *     PROC, PGRP, SESS, and TREE
       XXX:  Session not implemented.
 * 4.  Can we test the task merging?
 * 5.  Test that we abort correctly if propagating to nothing
 * 6.  Test abort if we propagate to something that's checkpointing
 * 7.  Make SURE that forwarding to YOURSELF doesn't send you into an infinite
 *     loop!
 * 8.  See what happens if we forward to something that isn't checkpointable
 */

enum child_state {
    child_started=1,
    child_waiting_for_handler,
    child_handled_checkpoint,
    child_handled_restart,
    child_handled_error,
    child_finished,
    child_error,
    child_ready,
};

enum parent_state {
    parent_forked=1,
    parent_checkpointed,
};

/*
 * Arguments for the child
 */
struct child_s {
    int pid;
    int do_setpgrp;           /* 0 = false, 1 = true */
    int to_context;           /* CR_SIGNAL_CONTEXT or CR_THREAD_CONTEXT */
};

/*
 * Arguments for the PARENT
 */
struct forwardrequest_s {
    int from_context;         /* CR_SIGNAL_CONTEXT or CR_THREAD_CONTEXT */
    cr_scope_t scope;
    int id;
    int retval;
    int saved_errno;
};

/* the parent */
struct forwardrequest_s the_forwardrequest;

/* the child */
static enum child_state child_status;
struct crut_pipes *child_pipes;

/* shared between parent and child */
static int pipefds[2];

struct child_s thechild;

/*
 * handlers for the parent
 */
static int parents_handler(void *arg)
{
    int ret;

    /* forward the request to the child */
    the_forwardrequest.retval = cr_forward_checkpoint(the_forwardrequest.scope, the_forwardrequest.id);
    the_forwardrequest.saved_errno = errno;

    /* we still need to do this when forwarding */
    ret = cr_checkpoint(0);
    if (ret > 0) ret = 0;

    return ret;
}

/*
 * handler for the child
 */
static int childs_handler(void *arg)
{
    int ret;

    ret = cr_checkpoint(0);
    if (ret > 0) {
        child_status = child_handled_restart;
        ret = 0;
    } else if (ret == 0) {
        child_status = child_handled_checkpoint;
    } else {
        child_status = child_handled_error;
    }

    return ret;
}

/*
 * Generic forwarding tests
 *
 * This next set of tests
 * check for normal operation of forwarding.  The next set of tests makes
 * sure that forwarding works both from and to phase1 and phase2 handlers
 * in in all of the various target scopes
 */

static void
do_child_work(void)
{
    int ret;
    int child_handler_context = thechild.to_context;
    cr_client_id_t client_id = 0;

    client_id = cr_init();
    if (client_id < 0) {
        child_status = -1;
        CRUT_FAIL("child: cr_init() failed");
    } else {
        /* register a checkpoint handler */
        ret = cr_register_callback(childs_handler, NULL, child_handler_context);
        if (ret < 0) {
            CRUT_FAIL("forward_setup:  Child failed to register callback.");
        }
    }

    if (thechild.do_setpgrp) {
        ret = setpgrp();
        if (ret < 0) {
            perror("setpgrp(child)");
            goto out;
        }
    }

    child_status = child_waiting_for_handler;

    /* tell pop that we're ready */
    CRUT_DEBUG("child: Child successfully registered handler %p", childs_handler);
    ret = write(pipefds[1], &child_status, sizeof(child_status));
    if (ret < 0) {
        perror("write(child)");
        goto out;
    }

    /* wait for the checkpoint -- is there a better way? */
    CRUT_DEBUG("Waiting for handler to run.");
    while (child_status == child_waiting_for_handler) { 
        sleep(1); 
    }

    /* tell pop I ran my handler */
    CRUT_DEBUG("child: Child ran handler.");
    ret = write(pipefds[1], &child_status, sizeof(child_status));
    if (ret < 0) {
        perror("write(child)");
        goto out;
    }

    CRUT_DEBUG("Handler ran.  Exiting");

    exit(0);

out:
    exit(-1);
}

static int
generic_forward_setup(struct child_s *thischild)
{
    int ret;

    memcpy(&thechild, thischild, sizeof(thechild));

    ret = pipe(pipefds);
    if (ret < 0) {
        perror("pipe");
        goto out;
    }

    /* Create a pipe to read data back from the child */

    CRUT_DEBUG("Forking a child.");

    thechild.pid = fork();
    if (thechild.pid < 0) {
        perror("fork");
        goto out;
    } else if (thechild.pid == 0) {

        close(pipefds[0]);

        do_child_work();

        /* --- NOT REACHED --- */
        CRUT_FAIL("I should never have reached this far!!!");
        exit(-1);

    } else {
        /* PARENT */

        close(pipefds[1]);

    }

out:
    return ret;
}

static int
to_signal_proc_setup(void **testdata)
{
    struct child_s mychild = {
        .do_setpgrp = 0,
        .to_context = CR_SIGNAL_CONTEXT,
    };

    return generic_forward_setup(&mychild);
}

static int
to_thread_proc_setup(void **testdata)
{
    struct child_s mychild = {
        .do_setpgrp = 0,
        .to_context = CR_THREAD_CONTEXT,
    };

    return generic_forward_setup(&mychild);
}

static int
to_signal_pgrp_setup(void **testdata)
{
    struct child_s mychild = {
        .do_setpgrp = 1,
        .to_context = CR_SIGNAL_CONTEXT,
    };

    return generic_forward_setup(&mychild);
}

static int
to_thread_pgrp_setup(void **testdata)
{
    struct child_s mychild = {
        .do_setpgrp = 1,
        .to_context = CR_THREAD_CONTEXT,
    };

    return generic_forward_setup(&mychild);
}

static int
generic_precheckpoint(struct forwardrequest_s *this_forwardrequest)
{
    int ret;
    int status;

    memcpy(&the_forwardrequest, this_forwardrequest, sizeof(the_forwardrequest));

    /* register a checkpoint handler */
    ret = cr_register_callback(parents_handler, NULL, the_forwardrequest.from_context);
    if (ret < 0) {
        CRUT_FAIL("generic_precheckpoint:  Failed to register callback");
    }
    CRUT_DEBUG("parent registered handler %p", parents_handler);

    /* wait for the child to start up */
    ret = read(pipefds[0], &status, sizeof(status));
    if (ret < 0) {
        perror("read(parent)");
    }
    CRUT_DEBUG("parent: child reports status = %d.", status);

    if (status < 0) {
        CRUT_FAIL("forward_setup:  Child has a problem.");
    }

    /* proceed to the checkpoint */
    return 0;
}

static int
from_signal_to_process(void *p)
{
    struct forwardrequest_s myreq = {
        .scope = CR_SCOPE_PROC,
        .id    = thechild.pid,
        .from_context = CR_SIGNAL_CONTEXT,
    };

    return generic_precheckpoint(&myreq);
}

static int
from_thread_to_process(void *p)
{
    struct forwardrequest_s myreq = {
        .scope = CR_SCOPE_PROC,
        .id    = thechild.pid,
        .from_context = CR_THREAD_CONTEXT,
    };
    
    return generic_precheckpoint(&myreq);
}

static int
from_signal_to_tree(void *p)
{
    struct forwardrequest_s myreq = {
        .scope = CR_SCOPE_TREE,
        .id    = getpid(),
        .from_context = CR_SIGNAL_CONTEXT,
    };

    return generic_precheckpoint(&myreq);
}

static int
from_thread_to_tree(void *p)
{
    struct forwardrequest_s myreq = {
        .scope = CR_SCOPE_TREE,
        .id    = getpid(),
        .from_context = CR_THREAD_CONTEXT,
    };

    return generic_precheckpoint(&myreq);
}

static int
from_signal_to_pgrp(void *p)
{
    struct forwardrequest_s myreq = {
        .scope = CR_SCOPE_PGRP,
        .id    = thechild.pid,
        .from_context = CR_SIGNAL_CONTEXT,
    };

    return generic_precheckpoint(&myreq);
}

static int
from_thread_to_pgrp(void *p)
{
    struct forwardrequest_s myreq = {
        .scope = CR_SCOPE_PGRP,
        .id    = thechild.pid,
        .from_context = CR_THREAD_CONTEXT,
    };

    return generic_precheckpoint(&myreq);
}

static int
forward_continue_or_restart(void *p)
{
    int ret;
    int exit_status;
    int status;

    
    if (the_forwardrequest.retval < 0) {
        CRUT_DEBUG("parent: saved_errno = %d.", the_forwardrequest.saved_errno);
        CRUT_FAIL("Could not forward the checkpoint to the child.");
        ret = -1;
        goto out;
    }

    /* wait for the child to run its handler */
    ret = read(pipefds[0], &status, sizeof(status));
    if (ret < 0) {
        perror("read(parent)");
    }
    CRUT_DEBUG("parent: child reports status = %d.", status);

    /* wait for the child */
    ret = waitpid(thechild.pid, &exit_status, 0);
    if (ret < 0) {
        perror("waitpid");
        goto out;
    } else if (ret == 0) {
        CRUT_FAIL("waitpid returned 0!  This should be impossible!");
        ret = -1;
        goto out;
    } 

    /* check the exit status of the child */
    if (WIFEXITED(exit_status)) {
        if (WEXITSTATUS(exit_status)) {
            CRUT_FAIL("Child returned %d on exit.", WEXITSTATUS(exit_status));
            ret = -1;
        } else {
            CRUT_DEBUG("Child exited normally.");
        }
    } else if (WIFSIGNALED(exit_status)) {
        CRUT_FAIL("Child terminated by signal %d", WTERMSIG(exit_status));   
        ret = -1;
    } else {
        /* This is nonsense */
        CRUT_FAIL("Confused!  exit_status=%d", exit_status);
        ret = -1;
    }

out:
    return ret;
}

static int
forward_teardown(void *p)
{
    return 0;
}

/*
 * The nochild tests.
 * 
 * This bunch is set up to test forwarding routines when we don't have a 
 * child present.  These test certain error cases.
 */

static int
nochild_setup(void **testdata)
{
    return 0;
}

static int
nochild_precheckpoint(struct forwardrequest_s *this_forwardrequest)
{
    int ret;
    memcpy(&the_forwardrequest, this_forwardrequest, sizeof(the_forwardrequest));
    /* register a checkpoint handler */
    ret = cr_register_callback(parents_handler, NULL, the_forwardrequest.from_context);
    if (ret < 0) {
        CRUT_FAIL("nochild_precheckpoint:  Failed to register callback");
    }
    CRUT_DEBUG("parent registered handler %p", parents_handler);

    return 0;
}

static int
from_sig_to_self(void *p)
{
    struct forwardrequest_s myreq = {
        .scope = CR_SCOPE_TREE,
        .id    = getpid(),
        .from_context = CR_SIGNAL_CONTEXT,
    };

    return nochild_precheckpoint(&myreq);
}

static int
from_sig_to_init(void *p)
{
    struct forwardrequest_s myreq = {
        .scope = CR_SCOPE_TREE,
        .id    = 1,
        .from_context = CR_SIGNAL_CONTEXT,
    };

    return nochild_precheckpoint(&myreq);
}

static int
from_sig_to_junk(void *p)
{
    struct forwardrequest_s myreq = {
        .scope = CR_SCOPE_TREE,
        .id    = -1,
        .from_context = CR_SIGNAL_CONTEXT,
    };

    return nochild_precheckpoint(&myreq);
}

static int
nochild_continue(void *p)
{
    int ret;

    ret = 0;
    if (the_forwardrequest.retval < 0) {
        CRUT_FAIL("Could not forward the checkpoint to the child.");
        ret = -1;
    }

    return ret;
}

static int
expect_handler_to_fail(void *p)
{
    int ret;

    ret = 0;
    if (the_forwardrequest.retval >= 0) {
        CRUT_FAIL("Unexpectedly forwarded the checkpoint to the child!");
        ret = -1;
    }

    return ret;
}

/*
 * The busy tests.
 *
 * These make sure that forwarding works correctly (i.e. fails without messing
 * up the rest of the checkpoint) if we try to forward
 * to something that's already checkpointing as part of a DIFFERENT checkpoint
 * request.
 */

/*
 * handler for the child
 */
static int child_busy_handler(void *arg)
{
    int ret;

    crut_pipes_putchar(child_pipes, child_ready);
    crut_pipes_expect(child_pipes, parent_checkpointed);

    ret = cr_checkpoint(0);
    if (ret > 0) {
        child_status = child_handled_restart;
        ret = 0;
    } else if (ret == 0) {
        child_status = child_handled_checkpoint;
    } else {
        child_status = child_handled_error;
    }

    return ret;
}

int request_checkpoint_to_file(const char *filename, int scope)
{
    int ret;
    cr_checkpoint_args_t cr_args;
    cr_checkpoint_handle_t cr_handle;

    /* open the context file */
    CRUT_VERBOSE("requesting checkpoint to file: %s", filename);
    ret = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE, 0600);
    if (ret < 0) {
        perror("open");
        goto out;
    }

    cr_initialize_checkpoint_args_t(&cr_args);
    cr_args.cr_fd = ret;
    cr_args.cr_scope = scope;

    /* issue the request */
    ret = cr_request_checkpoint(&cr_args, &cr_handle);
    if (ret < 0) {
        if (errno == EINVAL) {
            CRUT_VERBOSE("cr_request_checkpoint returned EINVAL");
            /* restarted before cr_request_checkpoint returned! */
            ;
        } else if (errno == EAGAIN) {
            CRUT_VERBOSE("cr_request_checkpoint returned EAGAIN");
            /* the checkpoint has not yet completed...  this is ok */
            ;
        } else {
            perror("cr_request_checkpoint");
        }
    }

    close(cr_args.cr_fd);

out:
    return ret;
}

/*
 * child portion of the busy test
 */
static int
do_child_busy_work(void)
{
    cr_client_id_t client_id = 0;
    int exitval;
    int ret;

    crut_pipes_expect(child_pipes, parent_forked);

    client_id = cr_init();
    if (client_id < 0) {
        child_status = -1;
        CRUT_FAIL("child: cr_init() failed");
    } else {
        /* register a checkpoint handler */
        ret = cr_register_callback(child_busy_handler, NULL, CR_SIGNAL_CONTEXT);
        if (ret < 0) {
            CRUT_FAIL("forward_setup:  Child failed to register callback.");
            exitval = ret;
            goto out;
        }
    }

    crut_pipes_putchar(child_pipes, child_started);

    request_checkpoint_to_file("/dev/null", CR_SCOPE_PROC);

    /* wait for the checkpoint -- is there a better way? */
    CRUT_DEBUG("Waiting for handler to run.");
    while (child_status == child_waiting_for_handler) { 
        sleep(1); 
    }

    exitval = child_finished;

out:
    exit(child_finished);
}

static int
busy_setup(void **p)
{
    struct crut_pipes *pipes = malloc(sizeof(struct crut_pipes));
    int retval;

    CRUT_DEBUG("Creating pipes and forking.");
    retval = crut_pipes_fork(pipes);
    if (!retval) {
        child_pipes = pipes;
        do_child_busy_work();
    }
    crut_pipes_putchar(pipes, parent_forked);
    crut_pipes_expect(pipes, child_started);

    *p = pipes;

    return retval;
}

static int
busy_precheckpoint(void *p)
{
    struct crut_pipes *pipes = p;
    int ret;

    struct forwardrequest_s myreq = {
        .scope = CR_SCOPE_TREE,
        .id    = pipes->parent,
        .from_context = CR_SIGNAL_CONTEXT,
    };

    memcpy(&the_forwardrequest, &myreq, sizeof(myreq));

    /* register a checkpoint handler */
    ret = cr_register_callback(parents_handler, NULL, the_forwardrequest.from_context);
    if (ret < 0) {
        CRUT_FAIL("busy_precheckpoint:  Failed to register callback");
        goto out;
    }

    crut_pipes_expect(pipes, child_ready);
    ret = 0;

out:
    return ret;
}

static int
busy_continue(void *p)
{
    struct crut_pipes *pipes = p;
    int ret;

    ret = 0; 
    if ((the_forwardrequest.retval != -1) || (the_forwardrequest.saved_errno != EBUSY)) {
        CRUT_FAIL("Forward request failed unexpectedly!  saved_errno = %d", the_forwardrequest.saved_errno);
        ret = -1;
    }

    crut_pipes_putchar(pipes, parent_checkpointed);
    crut_waitpid_expect(pipes->child, child_finished);

    return ret;
}

static int
busy_restart(void *p)
{
    int status;
    pid_t wait_ret;
    int retval;

    /* make sure we didn't forward successfully to the child */

    wait_ret = wait(&status);
    if (wait_ret != -1) {
        CRUT_FAIL("wait succeeded?  We shouldn't have a child!");
        retval = -1;
        goto out;
    }

    if (errno != ECHILD) {
        perror("wait");
        retval = -1;
        goto out;
    }

    retval = 0;
out:
    return retval;
}

static int
busy_teardown(void *p)
{
    struct crut_pipes *pipes = p;

    free(pipes);

    return 0;
}

int
main(int argc, char *argv[])
{
    int is_lt = crut_is_linuxthreads();
    int ret;

    struct crut_operations process_signal_to_signal_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"proc_sig_to_sig",
        test_description:"Forward from a signal context to a process with a thread handler",
	test_setup:to_signal_proc_setup,
	test_precheckpoint:from_signal_to_process,
	test_continue:forward_continue_or_restart,
	test_restart:forward_continue_or_restart,
	test_teardown:forward_teardown,
    };

    struct crut_operations process_signal_to_thread_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"proc_sig_to_thr",
        test_description:"Forward from a signal context to a process with a thread handler",
	test_setup:to_thread_proc_setup,
	test_precheckpoint:from_signal_to_process,
	test_continue:forward_continue_or_restart,
	test_restart:forward_continue_or_restart,
	test_teardown:forward_teardown,
    };

    struct crut_operations process_thread_to_signal_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"proc_thr_to_sig",
        test_description:"Forward from a thread context to a process with a signal handler",
	test_setup:to_signal_proc_setup,
	test_precheckpoint:from_thread_to_process,
	test_continue:forward_continue_or_restart,
	test_restart:forward_continue_or_restart,
	test_teardown:forward_teardown,
    };

    struct crut_operations process_thread_to_thread_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"proc_thr_to_thr",
        test_description:"Forward from a thread context to a process with a thread handler",
	test_setup:to_thread_proc_setup,
	test_precheckpoint:from_thread_to_process,
	test_continue:forward_continue_or_restart,
	test_restart:forward_continue_or_restart,
	test_teardown:forward_teardown,
    };

    struct crut_operations tree_signal_to_signal_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"tree_sig_to_sig",
        test_description:"Forward from a signal context to a tree with a thread handler",
	test_setup:to_signal_proc_setup,
	test_precheckpoint:from_signal_to_tree,
	test_continue:forward_continue_or_restart,
	test_restart:forward_continue_or_restart,
	test_teardown:forward_teardown,
    };

    struct crut_operations tree_signal_to_thread_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"tree_sig_to_thr",
        test_description:"Forward from a signal context to a tree with a thread handler",
	test_setup:to_thread_proc_setup,
	test_precheckpoint:from_signal_to_tree,
	test_continue:forward_continue_or_restart,
	test_restart:forward_continue_or_restart,
	test_teardown:forward_teardown,
    };

    struct crut_operations tree_thread_to_signal_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"tree_thr_to_sig",
        test_description:"Forward from a thread context to a tree with a signal handler",
	test_setup:to_signal_proc_setup,
	test_precheckpoint:from_thread_to_tree,
	test_continue:forward_continue_or_restart,
	test_restart:forward_continue_or_restart,
	test_teardown:forward_teardown,
    };

    struct crut_operations tree_thread_to_thread_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"tree_thr_to_thr",
        test_description:"Forward from a thread context to a tree with a thread handler",
	test_setup:to_thread_proc_setup,
	test_precheckpoint:from_thread_to_tree,
	test_continue:forward_continue_or_restart,
	test_restart:forward_continue_or_restart,
	test_teardown:forward_teardown,
    };

    struct crut_operations pgrp_signal_to_signal_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"pgrp_sig_to_sig",
        test_description:"Forward from a signal context to a pgrp with a thread handler",
	test_setup:to_signal_pgrp_setup,
	test_precheckpoint:from_signal_to_pgrp,
	test_continue:forward_continue_or_restart,
	test_restart:forward_continue_or_restart,
	test_teardown:forward_teardown,
    };

    struct crut_operations pgrp_signal_to_thread_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"pgrp_sig_to_thr",
        test_description:"Forward from a signal context to a pgrp with a thread handler",
	test_setup:to_thread_pgrp_setup,
	test_precheckpoint:from_signal_to_pgrp,
	test_continue:forward_continue_or_restart,
	test_restart:forward_continue_or_restart,
	test_teardown:forward_teardown,
    };

    struct crut_operations pgrp_thread_to_signal_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"pgrp_thr_to_sig",
        test_description:"Forward from a thread context to a pgrp with a signal handler",
	test_setup:to_signal_pgrp_setup,
	test_precheckpoint:from_thread_to_pgrp,
	test_continue:forward_continue_or_restart,
	test_restart:forward_continue_or_restart,
	test_teardown:forward_teardown,
    };

    struct crut_operations pgrp_thread_to_thread_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"pgrp_thr_to_thr",
        test_description:"Forward from a thread context to a pgrp with a thread handler",
	test_setup:to_thread_pgrp_setup,
	test_precheckpoint:from_thread_to_pgrp,
	test_continue:forward_continue_or_restart,
	test_restart:forward_continue_or_restart,
	test_teardown:forward_teardown,
    };

    struct crut_operations fwd_to_self_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"fwd_to_self",
        test_description:"Forward to the current process (should succeed)",
	test_setup:nochild_setup,
	test_precheckpoint:from_sig_to_self,
	test_continue:nochild_continue,
	test_restart:nochild_continue,
	test_teardown:forward_teardown,
    };

    struct crut_operations fwd_to_init_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"fwd_to_init",
        test_description:"Forward to the init process (should fail)",
	test_setup:nochild_setup,
	test_precheckpoint:from_sig_to_init,
	test_continue:expect_handler_to_fail,
	test_restart:expect_handler_to_fail,
	test_teardown:forward_teardown,
    };

    struct crut_operations fwd_to_junk_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"fwd_to_junk",
        test_description:"Forward to the junk process (should fail)",
	test_setup:nochild_setup,
	test_precheckpoint:from_sig_to_junk,
	test_continue:expect_handler_to_fail,
	test_restart:expect_handler_to_fail,
	test_teardown:forward_teardown,
    };

    struct crut_operations fwd_to_busy_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"fwd_to_busy",
        test_description:"Forward to to a checkpointing process",
	test_setup:busy_setup,
	test_precheckpoint:busy_precheckpoint,
	test_continue:busy_continue,
	test_restart:busy_restart,
	test_teardown:busy_teardown,
    };

    /* add the basic tests */
    crut_add_test(&process_signal_to_signal_test_ops);
    crut_add_test(&process_signal_to_thread_test_ops);
    crut_add_test(&process_thread_to_signal_test_ops);
    crut_add_test(&process_thread_to_thread_test_ops);

    crut_add_test(&tree_signal_to_signal_test_ops);
    crut_add_test(&tree_signal_to_thread_test_ops);
    crut_add_test(&tree_thread_to_signal_test_ops);
    crut_add_test(&tree_thread_to_thread_test_ops);

    crut_add_test(&pgrp_signal_to_signal_test_ops);
    if (!is_lt) crut_add_test(&pgrp_signal_to_thread_test_ops); // Known bug 2243
    crut_add_test(&pgrp_thread_to_signal_test_ops);
    if (!is_lt) crut_add_test(&pgrp_thread_to_thread_test_ops); // Known bug 2243

    /* add the silly tests */
    crut_add_test(&fwd_to_self_test_ops);
    crut_add_test(&fwd_to_init_test_ops);
    crut_add_test(&fwd_to_junk_test_ops);

    /* test forwarding to something that's checkpointing already */
    crut_add_test(&fwd_to_busy_test_ops);

    ret = crut_main(argc, argv);

    return ret;
}
