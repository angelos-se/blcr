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
 * $Id: ptrace.c,v 1.13 2008/12/16 23:08:24 phargrov Exp $
 */

/* Tests interaction w/ ptrace
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ptrace.h>

#include "libcr.h"
#include "crut_util.h"

static char *filename;
const char *chkpt_cmd;

enum {
	MSG_CHILD_READY = 42,
	MSG_TEST_ALLOW,
	MSG_TEST_DONE,
	TEST_EXIT_VAL
};

// count+1
static void check_exit(struct crut_pipes *pipes, int count)
{   
    crut_pipes_putchar(pipes, MSG_TEST_DONE);
    crut_waitpid_expect(pipes->child, TEST_EXIT_VAL);
    printf("%03d child %d completed\n", count, pipes->child);
    crut_pipes_close(pipes);
}

// Wait for our direct child to stop
static int wait_child_stop(int child)
{
    int rc, status;

    do {
	rc = waitpid(child, &status, WUNTRACED);
    } while ((rc < 0) && (errno == EINTR));
    if ((rc > 0) && WIFSTOPPED(status)) {
	return WSTOPSIG(status);
    } else {
	printf("XXX waitpid(%d,...) failed rc=%d errno=%d status=%d\n", child, rc, errno, status);
	kill(child, SIGKILL);
	exit(1);
    }
    return -1;
}

static void detach_child(int child)
{
    int rc;
   
    rc = ptrace(PTRACE_CONT, child, NULL, 0);
    if (rc < 0) {
	printf("XXX ptrace(PTRACE_CONT,%d,...) failed\n", child);
	exit(1);
    }
    rc = wait_child_stop(child);
    rc = ptrace(PTRACE_DETACH, child, NULL, rc);
    if (rc < 0) {
	printf("XXX ptrace(PTRACE_DETACH,%d,...) failed\n", child);
	exit(1);
    }
}

int child_main(struct crut_pipes *pipes) {
    crut_pipes_putchar(pipes, MSG_CHILD_READY);
    crut_pipes_expect(pipes, MSG_TEST_DONE);
    return TEST_EXIT_VAL;
}

int parent_main(struct crut_pipes *pipes, int traced) {
    int rc;
    
    rc = ptrace(PTRACE_ATTACH, traced, NULL, 0);
    if (rc < 0) {
	printf("XXX ptrace(PTRACE_ATTACH,%d,...) failed\n", traced);
    }
    (void)wait_child_stop(traced);
    crut_pipes_putchar(pipes, MSG_CHILD_READY);

    /* This is to detach on --ptraced-allow */
    crut_pipes_expect(pipes, MSG_TEST_ALLOW);
    detach_child(traced);

    crut_pipes_expect(pipes, MSG_TEST_DONE);
    return TEST_EXIT_VAL;
}

static void checkpoint(int pid, const char *arg, int status, int count)
{
    char *cmd = NULL;
    int rc;

    printf("#ST_ALARM:10\n");

    cmd = crut_aprintf("exec %s --pid --file %s %s --quiet %d",
			chkpt_cmd, filename, arg, pid);
    rc = system(cmd);
    if (!WIFEXITED(rc)) {
	printf("XXX system(%s) unexpectedly failed\n", cmd);
    } else if ((unsigned char)WEXITSTATUS(rc) != (unsigned char)status) {
	printf("XXX system(%s) unexpectedly exited with %d\n", cmd, WEXITSTATUS(rc));
    } else {
        printf("%03d cr_checkpoint(%d, \"%s\") exited with expected code %d\n", count, pid, arg, WEXITSTATUS(rc));
    }
    free(cmd);
}

// count+2
static int launch_children(struct crut_pipes *pipes, int count)
{
    int traced, tracer;
    
    /* The traced child */
    traced = crut_pipes_fork(pipes+0);
    if (!traced) {
	exit(child_main(pipes+0));
    }
    crut_pipes_expect(pipes+0, MSG_CHILD_READY);
    printf("%03d ptrace child %d is READY\n", count++, traced);

    /* The tracing child */
    tracer = crut_pipes_fork(pipes+1);
    if (!tracer) {
	exit(parent_main(pipes+1,traced));
    }
    crut_pipes_expect(pipes+1, MSG_CHILD_READY);
    printf("%03d ptrace parent %d is READY\n", count++, tracer);

    return 2;
}

int main(int argc, char * const argv[])
{
    int count = 0;
    int mypid = getpid();
    struct crut_pipes pipes[2];
    int parent, child;
	
    setlinebuf(stdout);
    printf("%03d Process started with pid %d\n", count++, mypid);

    filename = crut_aprintf("context.%d", mypid);

    chkpt_cmd = crut_find_cmd(argv[0], "cr_checkpoint");

    count += launch_children(pipes, count);
    child = pipes[0].child;
    parent = pipes[1].child;

    /* Check for expected basic error cases */
    checkpoint(child, "", CR_EPTRACED, count++);
    checkpoint(child, "--ptraced-error", CR_EPTRACED, count++);
    checkpoint(parent, "", CR_EPTRACER, count++);
    checkpoint(parent, "--ptracer-error", CR_EPTRACER, count++);

    /* Check for skip options */
    checkpoint(child, "--ptraced-skip", ESRCH, count++);
    checkpoint(child, "--ptracer-skip", CR_EPTRACED, count++);
    checkpoint(parent, "--ptraced-skip", CR_EPTRACER, count++);
    checkpoint(parent, "--ptracer-skip", ESRCH, count++);

    /* Check for the allow option */
    checkpoint(parent, "--ptraced-allow", CR_EPTRACER, count++);
    crut_pipes_putchar(pipes+1, MSG_TEST_ALLOW);
    checkpoint(child, "--ptraced-allow", 0, count++); // parent is waiting to PTRACE_DETACH

    /* Sanity check, all should pass since no longer ptraced or tracing */
    checkpoint(child, "", 0, count++);
    checkpoint(child, "--ptraced-error", 0, count++);
    checkpoint(child, "--ptraced-skip", 0, count++);
    checkpoint(child, "--ptraced-allow", 0, count++);
    checkpoint(child, "--ptracer-error", 0, count++);
    checkpoint(child, "--ptracer-skip", 0, count++);
    checkpoint(parent, "", 0, count++);
    checkpoint(parent, "--ptraced-error", 0, count++);
    checkpoint(parent, "--ptraced-skip", 0, count++);
    checkpoint(parent, "--ptraced-allow", 0, count++);
    checkpoint(parent, "--ptracer-error", 0, count++);
    checkpoint(parent, "--ptracer-skip", 0, count++);

    check_exit(pipes+0, count++);
    check_exit(pipes+1, count++);

    printf("%03d DONE\n", count);
    (void)unlink(filename);

    return 0;
}
