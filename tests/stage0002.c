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
 * $Id: stage0002.c,v 1.13.8.1 2009/02/27 22:00:05 phargrov Exp $
 */

const char description[] =
"Description of tests/Stage0002/cr_test:\n"
"\n"
"This test verifies the basic 'Stage II' functionality of both libcr and the\n"
"blcr.o kernel module.  In addition to 'Stage I', this includes the following:\n"
" + cr_register_callback()\n"
"        Registers an thread context callback.\n"
" + cr_{enter,leave}_cs(CR_ID_CALLBACK) does not block in callbacks.\n"
" + Externally initiated checkpoint (runs cr_checkpoint utility).\n"
" + Ability to fork() and waitpid() from the thread-context callback.\n"
;

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "libcr.h"
#include "crut_util.h"

static volatile int flag = 0;

enum {
	MSG_GRANDCHILD_READY = 86,
	MSG_PARENT_DONE,
	MSG_EXIT_VAL,
};

static int my_thread_callback(void *arg)
{
    pid_t child;

    cr_enter_cs(CR_ID_CALLBACK);
    printf("002 thread-context callback running in pid %d\n", (int)getpid());
    child = fork();
    if (child > 0) {
	waitpid(child, NULL, 0);
    } else if (child == 0) {
	printf("003 in fork()ed child pid=%d\n", (int)getpid());
	sleep(1);
	exit(0);
    } else {
	printf("XXX thread-context callback failed to fork()\n");
	exit(-1);
    }
    printf("004 thread-context callback complete\n");
    cr_leave_cs(CR_ID_CALLBACK);

    return 0;
}

static int my_signal_callback(void *arg)
{
    flag = 1;
    return 0;
}

static int child_main(struct crut_pipes *pipes)
{
    pid_t child;
    int rc;
    
    (void)cr_init();
    (void)cr_register_callback(my_signal_callback, NULL, CR_SIGNAL_CONTEXT);
    (void)cr_register_callback(my_thread_callback, NULL, CR_THREAD_CONTEXT);

    printf("000 First process started with pid %d\n", getpid());
    printf("#ST_ALARM:120\n");

    child = fork();
    if (child < 0) {
	printf("XXX First process failed to fork\n");
	exit(-1);
    }

    if (!child) {
	int pid = getpid();
	printf("001 Second process started with pid %d\n", pid);
	crut_pipes_putchar(pipes, MSG_GRANDCHILD_READY);
	crut_pipes_expect(pipes, MSG_PARENT_DONE);
	printf("005 Exiting from pid %d\n", pid);
    } else {
	do {
	    rc = waitpid(child, NULL, 0);
	} while ((rc < 0) && (errno == EINTR));
	if (flag) {
	    printf("006 verified that signal-context callback ran\n");
	}
	printf("007 Exiting from pid %d\n", getpid());
    }

    return MSG_EXIT_VAL;
}

int main(int argc, char **argv)
{
    struct crut_pipes pipes;
    char *buf = NULL;
    pid_t child;
    struct stat s;
    int rc;

    setlinebuf(stdout);

    child = crut_pipes_fork(&pipes);
    if (child == 0) {
	return child_main(&pipes);
    } else if (child < 0) {
	printf("XXX main failed to fork()\n");
	exit(-1);
    }

    crut_pipes_expect(&pipes, MSG_GRANDCHILD_READY);

    buf = crut_aprintf("exec %s -f context.%d --quiet %d",
			crut_find_cmd(argv[0], "cr_checkpoint"), child, child);
    rc = system(buf);
    if (rc) {
	printf("XXX got %d from system(%s)\n", rc, buf);
	exit(-1);
    }
    free(buf);

    crut_pipes_putchar(&pipes, MSG_PARENT_DONE);
    crut_waitpid_expect(child, MSG_EXIT_VAL);

    buf = crut_aprintf("context.%d", child);
    rc = stat(buf, &s);
    if (rc < 0) {
	printf("XXX failed to stat() %s\n", buf);
	exit(-1);
    } else if (s.st_size == 0) {
	printf("XXX failed to stat() %s\n", buf);
	exit(-1);
    } else {
	printf("008 %s has size %lu\n", buf, (unsigned long)s.st_size);
    }

    (void)unlink(buf); // might fail silently
    free(buf);

    printf("009 DONE\n");

    return 0;
}
