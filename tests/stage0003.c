/*
 * Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
 * 2008, The Regents of the University of California, through Lawrence
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
 * $Id: stage0003.c,v 1.13.12.1 2009/03/12 20:07:38 phargrov Exp $
 */

const char description[] =
"Description of tests/Stage0003:\n"
"\n"
"This test verifies the basic 'Stage III' functionality of both libcr and the\n"
"blcr.o kernel module:\n"
" + Externally initiated checkpoint of process group (runs cr_checkpoint utility).\n"
;

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sched.h>
#include "crut_util.h"

static volatile int done = 0;

enum {
	CHILD_MAIN = 0,
	PARENT_PRE,
	PARENT_POST,
	CHILD_PRE,
	CHILD_POST
};

static int parent_signal_callback(void *arg)
{
    struct crut_pipes *pipes = arg;
    int rc;

    printf("002 callback running in parent\n");
    fflush(stdout);
    crut_pipes_putchar(pipes, PARENT_PRE);
    crut_pipes_expect(pipes, CHILD_PRE);
    printf("004 callback recv PRE ack from child\n");
    fflush(stdout);

    rc = cr_checkpoint(0);

    if (rc == 0) {
        printf("005 callback continuing in parent\n");
        fflush(stdout);
        crut_pipes_putchar(pipes, PARENT_POST);
        crut_pipes_expect(pipes, CHILD_POST);
        printf("007 callback recv POST ack from child\n");
        fflush(stdout);
    }

    /* Pthreads not safe in signal context: use a volatile int */
    done = 1;

    return 0;
}

static int child_signal_callback(void *arg)
{
    struct crut_pipes *pipes = arg;
    int rc;

    crut_pipes_expect(pipes, PARENT_PRE);
    printf("003 child recv PRE from parent\n");
    fflush(stdout);
    crut_pipes_putchar(pipes, CHILD_PRE);

    rc = cr_checkpoint(0);

    if (rc == 0) {
        crut_pipes_expect(pipes, PARENT_POST);
        printf("006 child recv POST from parent\n");
        fflush(stdout);
        crut_pipes_putchar(pipes, CHILD_POST);
    }

    /* Pthreads not safe in signal context: use a volatile int */
    done = 1;

    return 0;
}

int child_main(struct crut_pipes *pipes) {
    printf("001 Child process started with pid %d\n", getpid());
    fflush(stdout);
    (void)cr_init();
    (void)cr_register_callback(child_signal_callback, pipes, CR_SIGNAL_CONTEXT);
    crut_pipes_putchar(pipes, CHILD_MAIN);
    while (!done) { sched_yield(); } /* Pthreads not safe in signal context */
    exit(0);
}

int main(int argc, char **argv)
{
    struct crut_pipes *pipes = malloc(sizeof(*pipes));
    pid_t child2;
    struct stat s;
    char *filename;
    int pgid;
    int rc;

    crut_program_name = argv[0]; /* For crut output */

    setlinebuf(stdout);

    (void)cr_init();

    printf("000 Parent process started with pid %d\n", getpid());
    printf("#ST_ALARM:120\n");
    fflush(stdout);

    crut_block_sigttou();
    setpgid(0,0);
    pgid = getpgrp();

    if (!crut_pipes_fork(pipes)) {
	child_main(pipes);
	exit(-1);
    }
    (void)cr_register_callback(parent_signal_callback, pipes, CR_SIGNAL_CONTEXT);

    crut_pipes_expect(pipes, CHILD_MAIN);

    child2 = fork();
    if (child2 == 0) {
	const char *cmd = crut_find_cmd(argv[0], "cr_checkpoint");
	const char arg1[] = "-g";
	char *arg2;
        setpgid(0,0);
        arg2 = crut_aprintf("%d", pgid);
	execlp(cmd, cmd, arg1, arg2, NULL);
	printf("XXX child2 failed to execl()\n");
	exit(-1);
    } else if (child2 < 0) {
	printf("XXX main failed second fork()\n");
	exit(-1);
    }

    while (!done) { sched_yield(); } /* Pthreads not safe in signal context */

    crut_waitpid_expect(child2, 0);
    crut_waitpid_expect(pipes->child, 0);

    filename = crut_aprintf("context.%d", pgid);
    rc = stat(filename, &s);
    if (rc < 0) {
	printf("XXX failed to stat() %s\n", filename);
	exit(-1);
    } else if (s.st_size == 0) {
	printf("XXX failed to stat() %s\n", filename);
	exit(-1);
    } else {
	printf("008 %s has size %lu\n", filename, (unsigned long)s.st_size);
    }

    (void)unlink(filename); // might fail silently
    free(filename);

    printf("009 DONE\n");

    return 0;
}
