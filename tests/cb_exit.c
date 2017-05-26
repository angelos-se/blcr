/*
 * Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
 * 2007, The Regents of the University of California, through Lawrence
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
 * $Id: cb_exit.c,v 1.15 2008/12/05 23:15:21 phargrov Exp $
 */

#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdint.h>

#include "libcr.h"
#include "crut_util.h"

static char *filename = NULL;
static const char *chkpt_cmd;
static const char *rstrt_cmd;

volatile int done = 0;

enum {
	MSG_CHILD_READY = 42,
	MSG_CHILD_DONE,
	MSG_PARENT_DONE,
	TEST_EXIT_VAL1,
	TEST_EXIT_VAL2,
	TEST_EXIT_VAL3,
	TEST_EXIT_VAL4
};

static void do_segv(void) {
#if 0
  struct rlimit r;
  int rc;

  /* disable generation of a 'core' */
  r.rlim_cur = r.rlim_max = 0;
  rc = setrlimit(RLIMIT_CORE, &r);
  if (rc < 0) {
    perror("setrlimit(RLIMIT_CORE->0):");
    exit(-1);
  }
#endif

  *(int *)0UL = 0;
  raise(SIGSEGV);
}

static void uncore(struct crut_pipes *pipes)
{
    char *corename = crut_aprintf("core.%d", pipes->child);
    (void)unlink(corename);
    free(corename);
    (void)unlink("core");
}

static void exec_exit(int code)
{
    execl("/bin/bash", "sh", "-c", crut_aprintf("exit %d", code), NULL);
}

// Callback that exits from CHECKPOINT portion of callback
static int cb_chkpt_exit(void* arg)
{
    int count = (uintptr_t)arg;

    printf("%03d enter cb_chkpt_exit\n", count++);
    printf("%03d child exit()\n", count++);
    fflush(NULL);
    exit(TEST_EXIT_VAL1);
    printf("XXX exit() returned unexpectedly\n");
    exit(-1);

    return 1;
}

// Callback that SEGVs from CHECKPOINT portion of callback
static int cb_chkpt_segv(void* arg)
{
    int count = (uintptr_t)arg;

    printf("%03d enter cb_chkpt_segv\n", count++);
    printf("%03d child SEGV\n", count++);
    fflush(NULL);
    do_segv();
    printf("XXX do_segv() returned unexpectedly\n");
    exit(-1);

    return 1;
}

// Callback that exec()s from CHECKPOINT portion of callback
static int cb_chkpt_exec(void* arg)
{
    int count = (uintptr_t)arg;

    printf("%03d enter cb_chkpt_exec\n", count++);
    printf("%03d child exec()\n", count++);
    fflush(NULL);
    exec_exit(TEST_EXIT_VAL1);
    printf("XXX exec() returned unexpectedly\n");
    exit(-1);

    return 1;
}

// Callback that exits from CONTINUE and RESTART portions of callback
static int cb_exit23(void* arg)
{
    int count = (uintptr_t)arg;
    int rc;

    printf("%03d enter cb_exit23\n", count++);
    rc = cr_checkpoint(CR_CHECKPOINT_READY);
    if (!rc) {
      printf("%03d child CONT exit()\n", count++);
      fflush(NULL);
      exit(TEST_EXIT_VAL2);
      printf("XXX exit() returned unexpectedly\n");
    } else if (rc > 0) {
      ++count; // match the CONT
      printf("%03d child RSTRT exit()\n", count++);
      exit(TEST_EXIT_VAL3);
      printf("XXX exit() returned unexpectedly\n");
    } else {
      printf("XXX cr_checkpoint() failed\n");
    }

    return 0;
}

// Callback that SEGVs from CONTINUE and RESTART portions of callback
static int cb_segv23(void* arg)
{
    int count = (uintptr_t)arg;
    int rc;

    printf("%03d enter cb_segv23\n", count++);
    rc = cr_checkpoint(CR_CHECKPOINT_READY);
    if (!rc) {
      printf("%03d child CONT SEGV\n", count++);
      fflush(NULL);
      do_segv();
      printf("XXX do_segv() returned unexpectedly\n");
    } else if (rc > 0) {
      ++count; // match the CONT
      printf("%03d child RSTRT SEGV\n", count++);
      do_segv();
      printf("XXX do_segv() returned unexpectedly\n");
    } else {
      printf("XXX cr_checkpoint() failed\n");
    }

    return 0;
}

// Callback that exec()s from CONTINUE and RESTART portions of callback
static int cb_exec23(void* arg)
{
    int count = (uintptr_t)arg;
    int rc;

    printf("%03d enter cb_exec23\n", count++);
    rc = cr_checkpoint(CR_CHECKPOINT_READY);
    if (!rc) {
      printf("%03d child CONT exec()\n", count++);
      fflush(NULL);
      exec_exit(TEST_EXIT_VAL2);
      printf("XXX exec() returned unexpectedly\n");
    } else if (rc > 0) {
      ++count; // match the CONT
      printf("%03d child RSTRT exec()\n", count++);
      exec_exit(TEST_EXIT_VAL3);
      printf("XXX exec() returned unexpectedly\n");
    } else {
      printf("XXX cr_checkpoint() failed\n");
    }

    return 0;
}

// Callback that returns non-zero from CONTINUE portion of callback
static int cb_return_cont(void* arg)
{
    int count = (uintptr_t)arg;
    int rc;

    printf("%03d enter cb_return_cont\n", count++);
    rc = cr_checkpoint(CR_CHECKPOINT_READY);
    if (!rc) {
      printf("%03d child CONT return(1)\n", count++);
      fflush(NULL);
      return 1;
    } else if (rc > 0) {
      printf("XXX restarted unexpectedly failed\n");
    } else {
      printf("XXX cr_checkpoint() failed\n");
    }

    return 0;
}

// Callback that returns non-zero from RESTART portion of callback
static int cb_return_rstrt(void* arg)
{
    int count = (uintptr_t)arg;
    int rc;

    printf("%03d enter cb_return_rstrt\n", count++);
    rc = cr_checkpoint(CR_CHECKPOINT_READY);
    if (rc > 0) {
      printf("%03d child RESTART return(1)\n", count++);
      fflush(NULL);
      return 1;
    } else if (rc < 0) {
      printf("XXX cr_checkpoint() failed\n");
    }

    done = 1;
    return 0;
}

void child_main(struct crut_pipes *pipes, int count, int flags, int (*cb)(void*))
{
    cr_callback_id_t cb_id;
    cr_client_id_t my_id;
    int mypid = getpid();

    printf("#ST_ALARM:60\n");
    filename = crut_aprintf("context.%d", mypid);

    my_id = cr_init();
    if (my_id < 0) {
	printf("XXX child %d cr_init() failed, returning %d\n", mypid, my_id);
	exit(-1);
    } else {
	printf("%03d child %d cr_init() succeeded\n", count++, mypid);
    }

    cb_id = cr_register_callback(cb, (void*)(uintptr_t)(count+1), flags);
    if (cb_id < 0) {
	printf("XXX cr_register_callback() unexpectedly returned %d\n", cb_id);
	exit(-1);
    } else {
	printf("%03d child %d cr_register_callback() succeeded\n", count++, mypid);
    }

    crut_pipes_putchar(pipes, MSG_CHILD_READY);
    while (!done) pause();

    exit(TEST_EXIT_VAL4);
}

int run_tests(int base, int flags) {
    char *cmd;
    struct crut_pipes pipes;
    int rc;

    printf("# test: exit() in CHECKPOINT portion of a callback\n");
    if (!crut_pipes_fork(&pipes)) {
	child_main(&pipes, base, flags, cb_chkpt_exit);
	exit(1);
    } else {
	cmd = crut_aprintf("exec %s --pid %d --kill --file %s --quiet", chkpt_cmd, pipes.child, filename);
	crut_pipes_expect(&pipes, MSG_CHILD_READY);
	rc = system(cmd);
	if (!WIFEXITED(rc) || (WEXITSTATUS(rc) != ESRCH)) {
	    printf("XXX system(%s) unexpectedly returned %d\n", cmd, rc);
	    exit(1);
	}
	free(cmd);
	crut_waitpid_expect(pipes.child, TEST_EXIT_VAL1);
    }
    crut_pipes_close(&pipes);
    (void)unlink(filename);

    printf("# test: SEGV in CHECKPOINT portion of a callback\n");
    if (!crut_pipes_fork(&pipes)) {
	child_main(&pipes, base+4, flags, cb_chkpt_segv);
	exit(1);
    } else {
	cmd = crut_aprintf("exec %s --pid %d --kill --file %s --quiet", chkpt_cmd, pipes.child, filename);
	crut_pipes_expect(&pipes, MSG_CHILD_READY);
	rc = system(cmd);
	if (!WIFEXITED(rc) || (WEXITSTATUS(rc) != ESRCH)) {
	    printf("XXX system(%s) unexpectedly returned %d\n", cmd, rc);
	    exit(1);
	}
	free(cmd);
	crut_waitpid_expect_signal(pipes.child, SIGSEGV);
    }
    crut_pipes_close(&pipes);
    uncore(&pipes);
    (void)unlink(filename);

    printf("# test: exec() in CHECKPOINT portion of a callback\n");
    if (!crut_pipes_fork(&pipes)) {
	child_main(&pipes, base+8, flags, cb_chkpt_exec);
	exit(1);
    } else {
	cmd = crut_aprintf("exec %s --pid %d --kill --file %s --quiet", chkpt_cmd, pipes.child, filename);
	crut_pipes_expect(&pipes, MSG_CHILD_READY);
	rc = system(cmd);
	if (!WIFEXITED(rc) || (WEXITSTATUS(rc) != ESRCH)) {
	    printf("XXX system(%s) unexpectedly returned %d\n", cmd, rc);
	    exit(1);
	}
	free(cmd);
	crut_waitpid_expect(pipes.child, TEST_EXIT_VAL1);
    }
    crut_pipes_close(&pipes);
    (void)unlink(filename);

    printf("# test: exit() in CONTINUE and RESTART portions of a callback\n");
    if (!crut_pipes_fork(&pipes)) {
	child_main(&pipes, base+12, flags, cb_exit23);
	exit(1);
    } else {
	cmd = crut_aprintf("exec %s --pid %d --kill --file %s --quiet", chkpt_cmd, pipes.child, filename);
	crut_pipes_expect(&pipes, MSG_CHILD_READY);
	rc = system(cmd);
	if (!WIFEXITED(rc) || (WEXITSTATUS(rc) != 0)) {
	    printf("XXX system(%s) unexpectedly returned %d\n", cmd, rc);
	    exit(1);
	}
	free(cmd);
	crut_waitpid_expect(pipes.child, TEST_EXIT_VAL2);
    }
    crut_pipes_close(&pipes);
    cmd = crut_aprintf("exec %s %s", rstrt_cmd, filename);
    rc = system(cmd);
    if (!WIFEXITED(rc) || (WEXITSTATUS(rc) != TEST_EXIT_VAL3)) {
	printf("XXX system(%s) unexpectedly returned %d\n", cmd, rc);
	exit(1);
    }
    free(cmd);
    (void)unlink(filename);

    printf("# test: repeat previous test w/ --kill in the restart flags\n");
    if (!crut_pipes_fork(&pipes)) {
	child_main(&pipes, base+17, flags, cb_exit23);
	exit(1);
    } else {
	cmd = crut_aprintf("exec %s --pid %d --kill --file %s --quiet", chkpt_cmd, pipes.child, filename);
	crut_pipes_expect(&pipes, MSG_CHILD_READY);
	rc = system(cmd);
	if (!WIFEXITED(rc) || (WEXITSTATUS(rc) != 0)) {
	    printf("XXX system(%s) unexpectedly returned %d\n", cmd, rc);
	    exit(1);
	}
	free(cmd);
	crut_waitpid_expect(pipes.child, TEST_EXIT_VAL2);
    }
    crut_pipes_close(&pipes);
    cmd = crut_aprintf("exec %s --kill %s", rstrt_cmd, filename);
    rc = system(cmd);
    if (!WIFEXITED(rc) || (WEXITSTATUS(rc) != TEST_EXIT_VAL3)) {
	printf("XXX system(%s) unexpectedly returned %d\n", cmd, rc);
	exit(1);
    }
    free(cmd);
    (void)unlink(filename);

    printf("# test: SEGV in CONTINUE and RESTART portions of a callback\n");
    if (!crut_pipes_fork(&pipes)) {
	child_main(&pipes, base+22, flags, cb_segv23);
	exit(1);
    } else {
	cmd = crut_aprintf("exec %s --pid %d --kill --file %s --quiet", chkpt_cmd, pipes.child, filename);
	crut_pipes_expect(&pipes, MSG_CHILD_READY);
	rc = system(cmd);
	if (!WIFEXITED(rc) || (WEXITSTATUS(rc) != 0)) {
	    printf("XXX system(%s) unexpectedly returned %d\n", cmd, rc);
	    exit(1);
	}
	free(cmd);
	crut_waitpid_expect_signal(pipes.child, SIGSEGV);
    }
    crut_pipes_close(&pipes);
    cmd = crut_aprintf("exec %s %s", rstrt_cmd, filename);
    rc = system(cmd);
    if (!WIFSIGNALED(rc) || (WTERMSIG(rc) != SIGSEGV)) {
	printf("XXX system(%s) unexpectedly returned %d\n", cmd, rc);
	exit(1);
    }
    free(cmd);
    uncore(&pipes);
    (void)unlink(filename);

    printf("# test: repeat previous test w/ --kill in the restart flags\n");
    if (!crut_pipes_fork(&pipes)) {
	child_main(&pipes, base+27, flags, cb_segv23);
	exit(1);
    } else {
	cmd = crut_aprintf("exec %s --pid %d --kill --file %s --quiet", chkpt_cmd, pipes.child, filename);
	crut_pipes_expect(&pipes, MSG_CHILD_READY);
	rc = system(cmd);
	if (!WIFEXITED(rc) || (WEXITSTATUS(rc) != 0)) {
	    printf("XXX system(%s) unexpectedly returned %d\n", cmd, rc);
	    exit(1);
	}
	free(cmd);
	crut_waitpid_expect_signal(pipes.child, SIGSEGV);
    }
    crut_pipes_close(&pipes);
    cmd = crut_aprintf("exec %s --kill %s", rstrt_cmd, filename);
    rc = system(cmd);
    if (!WIFSIGNALED(rc) || (WTERMSIG(rc) != SIGSEGV)) {
	printf("XXX system(%s) unexpectedly returned %d\n", cmd, rc);
	exit(1);
    }
    free(cmd);
    uncore(&pipes);
    (void)unlink(filename);

    printf("# test: exec() in CONTINUE and RESTART portions of a callback\n");
    if (!crut_pipes_fork(&pipes)) {
	child_main(&pipes, base+32, flags, cb_exec23);
	exit(1);
    } else {
	cmd = crut_aprintf("exec %s --pid %d --kill --file %s --quiet", chkpt_cmd, pipes.child, filename);
	crut_pipes_expect(&pipes, MSG_CHILD_READY);
	rc = system(cmd);
	if (!WIFEXITED(rc) || (WEXITSTATUS(rc) != 0)) {
	    printf("XXX system(%s) unexpectedly returned %d\n", cmd, rc);
	    exit(1);
	}
	free(cmd);
	crut_waitpid_expect(pipes.child, TEST_EXIT_VAL2);
    }
    crut_pipes_close(&pipes);
    cmd = crut_aprintf("exec %s %s", rstrt_cmd, filename);
    rc = system(cmd);
    if (!WIFEXITED(rc) || (WEXITSTATUS(rc) != TEST_EXIT_VAL3)) {
	printf("XXX system(%s) unexpectedly returned %d\n", cmd, rc);
	exit(1);
    }
    free(cmd);
    (void)unlink(filename);

    printf("# test: repeat previous test w/ --kill in the restart flags\n");
    if (!crut_pipes_fork(&pipes)) {
	child_main(&pipes, base+37, flags, cb_exec23);
	exit(1);
    } else {
	cmd = crut_aprintf("exec %s --pid %d --kill --file %s --quiet", chkpt_cmd, pipes.child, filename);
	crut_pipes_expect(&pipes, MSG_CHILD_READY);
	rc = system(cmd);
	if (!WIFEXITED(rc) || (WEXITSTATUS(rc) != 0)) {
	    printf("XXX system(%s) unexpectedly returned %d\n", cmd, rc);
	    exit(1);
	}
	free(cmd);
	crut_waitpid_expect(pipes.child, TEST_EXIT_VAL2);
    }
    crut_pipes_close(&pipes);
    cmd = crut_aprintf("exec %s --kill %s", rstrt_cmd, filename);
    rc = system(cmd);
    if (!WIFEXITED(rc) || (WEXITSTATUS(rc) != TEST_EXIT_VAL3)) {
	printf("XXX system(%s) unexpectedly returned %d\n", cmd, rc);
	exit(1);
    }
    free(cmd);
    (void)unlink(filename);

    printf("# test: non-zero return from CONTINUE portion of a callback\n");
    if (!crut_pipes_fork(&pipes)) {
	child_main(&pipes, base+42, flags, cb_return_cont);
	exit(1);
    } else {
	cmd = crut_aprintf("exec %s --pid %d --file %s --quiet", chkpt_cmd, pipes.child, filename);
	crut_pipes_expect(&pipes, MSG_CHILD_READY);
	rc = system(cmd);
	if (!WIFEXITED(rc) || (WEXITSTATUS(rc) != (CR_EPERMFAIL & 255))) {
	    printf("XXX system(%s) unexpectedly returned %d\n", cmd, rc);
	    exit(1);
	}
	free(cmd);
	crut_waitpid_expect_signal(pipes.child, SIGKILL);
    }
    crut_pipes_close(&pipes);
    (void)unlink(filename);

    printf("# test: non-zero return from RESTART portion of a callback\n");
    if (!crut_pipes_fork(&pipes)) {
	child_main(&pipes, base+46, flags, cb_return_rstrt);
	exit(1);
    } else {
	cmd = crut_aprintf("exec %s --pid %d --file %s --quiet", chkpt_cmd, pipes.child, filename);
	crut_pipes_expect(&pipes, MSG_CHILD_READY);
	rc = system(cmd);
	if (!WIFEXITED(rc) || (WEXITSTATUS(rc) != 0)) {
	    printf("XXX system(%s) unexpectedly returned %d\n", cmd, rc);
	    exit(1);
	}
	free(cmd);
	crut_waitpid_expect(pipes.child, TEST_EXIT_VAL4);
    }
    crut_pipes_close(&pipes);
    cmd = crut_aprintf("exec %s %s --quiet", rstrt_cmd, filename);
    rc = system(cmd);
    if (!WIFEXITED(rc) || (WEXITSTATUS(rc) != (CR_ERSTRTABRT & 255))) {
	printf("XXX system(%s) unexpectedly returned %d\n", cmd, rc);
	exit(1);
    }
    free(cmd);
    (void)unlink(filename);

    return 50;
}

int main(int argc, char * const argv[])
{
    int count = 0;
    int mypid = getpid();
	
    chkpt_cmd = crut_find_cmd(argv[0], "cr_checkpoint");
    rstrt_cmd = crut_find_cmd(argv[0], "cr_restart");

    setlinebuf(stdout);
    printf("%03d Process started with pid %d\n", count++, mypid);

    filename = crut_aprintf("context.%d", mypid);

    printf("## Begin CR_SIGNAL_CONTEXT tests\n");
    count += run_tests(count, CR_SIGNAL_CONTEXT);
    if (crut_is_linuxthreads()) {
	fprintf(stderr, "Skipping threaded portion of 'cb_exit' tests due to LinuxThreads issues\n");
    } else {
        printf("## Begin CR_THREAD_CONTEXT tests\n");
	count += run_tests(count, CR_THREAD_CONTEXT);
    }

    printf("%03d DONE\n", count);

    return 0;
}
