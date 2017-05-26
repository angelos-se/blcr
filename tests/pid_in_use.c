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
 * $Id: pid_in_use.c,v 1.18 2008/11/30 04:36:56 phargrov Exp $
 */


const char description[] = 
"Description of tests/pid_in_use:\n"
"\n"
"This test verifies the correct behavior when restarting from a context\n"
"corresponding to a still running task (the requester itself in this case).\n"
"In a successful run, expect lines of output in SORTED order.\n"
;


#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "libcr.h"
#include "crut_util.h"

static int is_lt = 0;
static pid_t my_pid = 0;
static const char *chkpt_cmd = NULL;
static const char *rstrt_cmd = NULL;
static const char *filename = NULL;

enum {
    MSG_CHILD_READY,
    MSG_CHILD_CHKPT,
    MSG_CHILD_DONE,
    MSG_PARENT_DONE,
};

#define EXIT_PARENT 42

static int child_cb(void *arg)
{
    struct crut_pipes *pipes = arg;

    crut_pipes_putchar(pipes, MSG_CHILD_CHKPT);
    return 0;
}

static volatile int restarting = 0;
static int parent_cb1(void *arg)
{
    int rc = cr_checkpoint(CR_CHECKPOINT_READY);
    restarting = (rc > 0);
    return 0;
}

static int parent_cb2(void *arg)
{
    struct crut_pipes *pipes = arg;
    crut_pipes_expect(pipes, MSG_CHILD_CHKPT);
    sleep(2); /* Cause a delay to encourage placing our child first in the checkpoint */
    return parent_cb1(NULL);
}

static void *thread_main(void *arg) 
{
    (void)pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    while(1) { pthread_testcancel(); sleep(5); }
    return NULL;
}

static void do_checkpoint(int count, const char *scope)
{
    char *cmd;
    int rc;

    cmd = crut_aprintf("exec %s --file %s %s %d", chkpt_cmd, filename, scope, my_pid);
    rc = system(cmd);
    if ((rc < 0) || !WIFEXITED(rc) || WEXITSTATUS(rc)) {
	if (restarting) exit(EXIT_PARENT);
	printf("XXX system(%s) failed (%d)\n", cmd, rc);
	exit(-1);
    } else {
	printf("%03d cr_checkpoint %s OK\n", count, scope);
    }
    free(cmd);
}

static void do_restart_inner(int count, const char *descr, const char *args, int pass, int skip)
{
    char *cmd;
    int rc;

    if (skip) {
	printf("%03d restart %s w/ args='%s' SKIPPED due to LinuxThreads issues\n",
		count, descr, args);
	return;
    }

    cmd = crut_aprintf("exec %s %s --quiet %s", rstrt_cmd, args, filename);
    rc = system(cmd);
    if ((rc < 0) || !WIFEXITED(rc)) {
	printf("XXX system(%s) failed (%d)\n", cmd, rc);
	exit(-1);
    }
    free(cmd);
    rc = WEXITSTATUS(rc);
    if (rc != (pass ? EXIT_PARENT : EBUSY)) {
	printf("XXX restart %s returned unexpected status %d (%s)\n",
		descr, rc, strerror(rc));
	exit(-1);
    } else {
	printf("%03d restart %s w/ args='%s' %s as expected\n",
		count, descr, args, pass ? "succeeded" : "failed");
    }
}

static void do_restart(int count, const char *descr)
{
    do_restart_inner(count++, descr, "", 0, 0);
    do_restart_inner(count++, descr, "--no-restore-pid", 1, is_lt);
    do_restart_inner(count++, descr, "--no-restore-pid --restore-pgid", 0, is_lt);
    do_restart_inner(count++, descr, "--no-restore-pid --restore-sid", 0, is_lt);
}

int main(int argc, char * const argv[])
{
    struct crut_pipes pipes;
    pthread_t th;
    cr_client_id_t my_id;
    cr_callback_id_t parent_cbid;
    void *th_result;
    int rc;

    setlinebuf(stdout);

    /* Intialize our globals */
    is_lt = crut_is_linuxthreads();
    my_pid = getpid();
    filename = crut_aprintf("context.%d", my_pid);
    chkpt_cmd = crut_find_cmd(argv[0], "cr_checkpoint");
    rstrt_cmd = crut_find_cmd(argv[0], "cr_restart");

    printf("000 Process started with pid %d\n", my_pid);
    printf("#ST_ALARM:120\n");

    if (is_lt) {
	fprintf(stderr, "Skipping portions of 'pid_in_use' test due to LinuxThreads issues\n");
    }

    my_id = cr_init();
    if (my_id < 0) {
	printf("XXX cr_init() failed, returning %d\n", my_id);
	exit(-1);
    } else {
	printf("001 cr_init() succeeded\n");
    }

    parent_cbid = cr_register_callback(parent_cb1, NULL, CR_SIGNAL_CONTEXT);
    if (parent_cbid < 0) {
	printf("XXX cr_register_callbask() failed, returning %d\n", parent_cbid);
	exit(-1);
    }

    /*
     * TEST 1: Just a single-threaded process
     */

    do_checkpoint(2, "--pid");
    do_restart(3, "single process");
    (void)unlink(filename); // might fail silently

    /*
     * TEST 2a: A pthreaded process
     */

    if (crut_pthread_create(&th, NULL, &thread_main, NULL) != 0) {
	printf("XXX pthread_create() failed\n");
	exit(-1);
    }

    do_checkpoint(7, "--pid");
    do_restart(8, "pthreaded process");

    /*
     * TEST 2b: A pthreaded process, with only a partial conflict
     */

    rc = pthread_cancel(th);
    if (rc) {
	printf("XXX pthread_cancel() returned %d\n", rc);
	exit(-1);
    }
    rc = pthread_join(th, &th_result);
    if (rc) {
	printf("XXX pthread_join() returned %d\n", rc);
	exit(-1);
    }
    if (th_result != PTHREAD_CANCELED) {
	printf("XXX pthread_join() returned unexpected value\n");
	exit(-1);
    }
    printf("012 pthread_cancel/join of thread complete\n");

    do_restart(13, "pthreaded/partial");
    (void)unlink(filename); // might fail silently

    /*
     * TEST 3a: A process tree
     */

    rc = crut_pipes_fork(&pipes);
    if (!rc) {
	/* In the child */
        my_id = cr_init();
	(void)cr_register_callback(child_cb, &pipes, CR_SIGNAL_CONTEXT);
	crut_pipes_putchar(&pipes, MSG_CHILD_READY);
	crut_pipes_expect(&pipes, MSG_PARENT_DONE);
	crut_pipes_putchar(&pipes, MSG_CHILD_DONE);
	exit(0);
    } else if (rc < 0) {
	printf("XXX fork() failed\n");
	exit(-1);
    }

    rc = cr_replace_callback(parent_cbid, parent_cb2, &pipes, CR_SIGNAL_CONTEXT);
    if (rc < 0) {
	printf("XXX cr_replace_callback() failed, rc=%d, errno=%d (%s)\n", rc, errno, cr_strerror(errno));
    }

    crut_pipes_expect(&pipes, MSG_CHILD_READY);
    printf("017 child started with pid %d\n", pipes.child);

    do_checkpoint(18, "--tree");
    do_restart(19, "process tree");

    /*
     * TEST 3b: A process tree with only a partial conflict
     */

    alarm(30);
    crut_pipes_putchar(&pipes, MSG_PARENT_DONE);
    crut_pipes_expect(&pipes, MSG_CHILD_DONE);
    crut_waitpid_expect(pipes.child, 0);
    alarm(0);
    printf("023 child with pid %d terminated\n", pipes.child);

    do_restart(24, "tree/partial");
    (void)unlink(filename); // might fail silently

    printf("028 DONE\n");

    return 0;
}
