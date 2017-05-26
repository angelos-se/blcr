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
 * $Id: stopped.c,v 1.10 2008/08/29 20:01:44 phargrov Exp $
 */

/* Tests interaction of stopped processes, and the --stop/--cont options
 * of both cr_checkpoint and cr_restart.
 * Tests all (2*2*3*3 = 36) combinations of the folowing variables:
 * + Callback in app registered as SIGNAL or THREAD context (2 choices)
 * + App is RUNNING or STOPPED at time checkpoint is taken (2 choices)
 * + cr_checkpoint invoked with --stop, --cont, or neither (3 choices)
 * + cr_restart invoked with --stop, --cont, or neither (3 choices)
 *
 * Validation consists of checking that:
 * 1) the app is in the proper state (either RUNNING or STOPPED)
 * 2) the callback runs at the expected point in time (but see bug 2216)
 * 3) the app is able to complete its execution (after SIGCONT if appropriate)
 */

#define _GNU_SOURCE // for getline()
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sched.h>
#include <stdarg.h>
#include <stdint.h>

#include "libcr.h"
#include "crut_util.h"

static char *filename = NULL;
const char *chkpt_cmd;
const char *rstrt_cmd;

enum {
	MSG_CHILD_READY = 42,
	MSG_CHILD_CALLBACK,
	MSG_PARENT_DONE,
	TEST_EXIT_VAL
};

static struct {
	int count;
	struct crut_pipes *pipes;
} cb_args;

static int my_cb(void* arg)
{
    cr_checkpoint(CR_CHECKPOINT_READY);
    crut_pipes_putchar(cb_args.pipes, MSG_CHILD_CALLBACK);

    return 0;
}

void child_main(struct crut_pipes *pipes, int count, int flags)
{
    cr_callback_id_t cb_id;
    cr_client_id_t my_id;
    int mypid = getpid();

    printf("#ST_ALARM:60\n");

    my_id = cr_init();
    if (my_id < 0) {
	printf("XXX child %d cr_init() failed, returning %d\n", mypid, my_id);
	exit(-1);
    }

    cb_id = cr_register_callback(my_cb, (void*)(uintptr_t)(count), flags);
    if (cb_id < 0) {
	printf("XXX cr_register_callback() unexpectedly returned %d\n", cb_id);
	exit(-1);
    }

    cb_args.count = count;
    cb_args.pipes = pipes;

    crut_pipes_putchar(pipes, MSG_CHILD_READY);
    crut_pipes_expect(pipes, MSG_PARENT_DONE);

    exit(TEST_EXIT_VAL);
}

static char get_task_state(int pid)
{
    char *procfile;
    char *line = NULL;
    char result = '?';
    FILE *fp;
    size_t len;
    int rc;

    procfile = crut_aprintf("/proc/%d/status", pid);
    fp = fopen(procfile, "r");
    if (fp) {
      while (((rc = getline(&line, &len, fp)) >= 0) &&
             !sscanf(line, "State: %c", &result)) { /* nothing */ }
      (void)fclose(fp);
    }
    free(procfile);
    return result;
}

// Wait for our direct child to stop
static void wait_child_stop(int child, int count)
{
    int rc, status;

    do {
	rc = waitpid(child, &status, WUNTRACED);
    } while ((rc < 0) && (errno == EINTR));
    if ((rc > 0) && WIFSTOPPED(status) && (WSTOPSIG(status) == SIGSTOP)) {
	printf("%03d child %d is STOPped\n", count, child);
    } else {
	printf("XXX waitpid(%d,...) failed\n", child);
	kill(child, SIGKILL);
	exit(1);
    }
    // Sanity check
    if (get_task_state(child) != 'T') {
	printf("XXX waitpid(%d,...) and /proc/%d/status disagree\n", child, child);
	kill(child, SIGKILL);
	exit(1);
    }
}

// Wait for our INdirect child to stop
// count+1
static void wait_other_stop(int child, int count)
{
    // XXX: Fixing bug 1974 will make this *much* easier
    while (get_task_state(child) != 'T') sched_yield();
    printf("%03d child %d is STOPped\n", count, child);
}

// count+1
static void check_callback(struct crut_pipes *pipes, int child, int count)
{
    crut_pipes_expect(pipes, MSG_CHILD_CALLBACK);
    printf("%03d child %d callback ran\n", count, child);
}

// count+1
static void check_exit(struct crut_pipes *pipes, int child, int count)
{
    crut_pipes_putchar(pipes, MSG_PARENT_DONE);
    // Note: cr_restart is the child to wait for when restarting, thus pipes->child
    crut_waitpid_expect(pipes->child, TEST_EXIT_VAL);
    printf("%03d child %d completed\n", count, child);
    crut_pipes_close(pipes);
}

// count+2
static int check_running(struct crut_pipes *pipes, int child, int count)
{
    check_callback(pipes, child, count++);
    check_exit(pipes, child, count++);

    return 2;
}

// count+3
static int check_stopped(struct crut_pipes *pipes, int child, int count)
{
    check_callback(pipes, child, count++); // callback BEFORE stopping
    wait_other_stop(child, count++); // May not be direct child (restart time)
    kill(child, SIGCONT);
    check_exit(pipes, child, count++);

    return 3;
}

// count+1
static int checkpoint(struct crut_pipes *pipes, const char *sigarg, int count)
{
    char *cmd;
    int rc;

    cmd = crut_aprintf("exec %s --pid --file %s %s --quiet %d",
			chkpt_cmd, filename, sigarg, pipes->child);
    rc = system(cmd);
    if (!WIFEXITED(rc) || (WEXITSTATUS(rc) != 0)) {
	printf("XXX system(%s) unexpectedly returned %d\n", cmd, rc);
	exit(1);
    } else {
        printf("%03d ran '%s'\n", count, cmd);
    }
    free(cmd);

    return 1;
}

// count+0
static void restart(struct crut_pipes *pipes, const char *sigarg)
{
    printf("# run: %s %s %s\n", rstrt_cmd, sigarg?sigarg:"", filename);
    if (!crut_pipes_fork(pipes)) {
	if (sigarg) {
	    (void)execlp(rstrt_cmd, "cr_restart", sigarg, filename, NULL);
	} else {
	    (void)execlp(rstrt_cmd, "cr_restart", filename, NULL);
	}
	exit(-1);
    }
}

// count+(2+stopped)
static int run_continue_test(struct crut_pipes *pipes, int count, int stopped)
{
    int child = pipes->child; // The PID of the orignal child

    if (stopped) {
    	(void)check_stopped(pipes, child, count); // +3
    } else {
	(void)check_running(pipes, child, count); // +2
    }

    return 2 + !!stopped;
}

// count+(7+stopped)
static int run_restart_tests(struct crut_pipes *pipes, int count, int stopped)
{
    int child = pipes->child; // The PID of the orignal child

    // restart in default state
    restart(pipes, NULL);
    count += stopped ? check_stopped(pipes, child, count) // +3
		     : check_running(pipes, child, count); // +2

    // restart w/ explicit --cont
    restart(pipes, "--cont");
    count += check_running(pipes, child, count); // +2

    // restart w/ explicit --stop
    restart(pipes, "--stop");
    count += check_stopped(pipes, child, count); // +3

    return 7 + !!stopped;
}

// count+(1+stop)
static int launch_child(struct crut_pipes *pipes, int flags, int count, int stop)
{
    int child = crut_pipes_fork(pipes);
    if (!child) {
	child_main(pipes, count+1, flags);
	exit(1);
    } else {
        crut_pipes_expect(pipes, MSG_CHILD_READY);
	printf("%03d child %d is READY (context=%s stopped=%s)\n", count++, child,
		((flags==CR_SIGNAL_CONTEXT)?"SIGNAL":"THREAD"), (stop?"YES":"NO"));
    }
    if (stop) {
        kill(child, SIGSTOP);
        wait_child_stop(child, count);
    }
    return 1 + !!stop;
}

static int run_tests(int count, int flags, int pre_stopped) {
    struct crut_pipes pipes;
    int base = count;

    // Test 1: checkpoint w/o any args
    count += launch_child(&pipes, flags, count, pre_stopped);
    count += checkpoint(&pipes, "", count);
    count += run_continue_test(&pipes, count, pre_stopped);
    count += run_restart_tests(&pipes, count, pre_stopped);
    (void)unlink(filename);


    // Test 2: checkpoint w/ --stop
    count += launch_child(&pipes, flags, count, pre_stopped);
    count += checkpoint(&pipes, "--stop", count);
    count += run_continue_test(&pipes, count, 1);
    count += run_restart_tests(&pipes, count, pre_stopped);
    (void)unlink(filename);


    // Test 3: checkpoint w/ --cont
    count += launch_child(&pipes, flags, count, pre_stopped);
    count += checkpoint(&pipes, "--cont", count);
    count += run_continue_test(&pipes, count, 0);
    count += run_restart_tests(&pipes, count, pre_stopped);
    (void)unlink(filename);

    return count - base;
}

int main(int argc, char * const argv[])
{
    int count = 0;
    int mypid = getpid();
	
    setlinebuf(stdout);
    printf("%03d Process started with pid %d\n", count++, mypid);

    filename = crut_aprintf("context.%d", mypid);

    chkpt_cmd = crut_find_cmd(argv[0], "cr_checkpoint");
    rstrt_cmd = crut_find_cmd(argv[0], "cr_restart");

    count += run_tests(count, CR_SIGNAL_CONTEXT, 0);
    count += run_tests(count, CR_SIGNAL_CONTEXT, 1);
    if (crut_is_linuxthreads()) {
	fprintf(stderr, "Skipping threaded portion of 'stopped' tests due to LinuxThreads issues\n");
    } else {
	count += run_tests(count, CR_THREAD_CONTEXT, 0);
	count += run_tests(count, CR_THREAD_CONTEXT, 1);
    }

    printf("%03d DONE\n", count);

    return 0;
}
