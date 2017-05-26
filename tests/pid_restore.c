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
 * $Id: pid_restore.c,v 1.3 2008/09/05 18:53:22 phargrov Exp $
 */


#define _LARGEFILE64_SOURCE 1 // For O_LARGEFILE
#define _XOPEN_SOURCE 500 // for getsid()
#define _BSD_SOURCE 1 // for setlinebuf()
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>

#include "libcr.h"
#include "crut_util.h"

#ifndef O_LARGEFILE
  #define O_LARGEFILE 0
#endif

static const char *chkpt_cmd = NULL;
static const char *rstrt_cmd = NULL;
static const char *filename = NULL;

static int check_ids(int orig)
{
    int result = 0;
#if 0 // glibc caching polutes the result
    int pid = getpid();
#else
    int pid = syscall(SYS_getpid);
#endif
    int pgid = getpgrp();
    int sid = getsid(0);
    if (pid  == orig) result |= CR_RSTRT_RESTORE_PID;
    if (pgid == orig) result |= CR_RSTRT_RESTORE_PGID;
    if (sid  == orig) result |= CR_RSTRT_RESTORE_SID;
    return result;
}

static volatile int cb_ran = 0;
static int child_cb(void *arg) {
    cb_ran = 1;
    return 0;
}

static int child_main(void)
{
    int pid = getpid();
    cr_client_id_t my_id = cr_init();
    cr_callback_id_t cb_id = cr_register_callback(child_cb, NULL, CR_SIGNAL_CONTEXT);

    if (my_id < 0) {
        printf("XXX cr_init() failed, returning %d\n", my_id);
        return -1;
    }
    if (cb_id < 0) {
        printf("XXX cr_register_callbask() failed, returning %d\n", cb_id);
        return -1;
    }
    if (setsid() < 0) {
        printf("XXX setsid() failed\n");
	return -1;
    }

    while (!cb_ran) sched_yield();

    return check_ids(pid) + EBUSY + 1; // Ensure no accidental collision w/ EBUSY
}

static void do_checkpoint(int count, int pid)
{
    char *cmd;
    int rc;

    cmd = crut_aprintf("exec %s --file %s --pid %d", chkpt_cmd, filename, pid);
    rc = system(cmd);
    if ((rc < 0) || !WIFEXITED(rc) || WEXITSTATUS(rc)) {
	printf("XXX system(%s) failed (%d)\n", cmd, rc);
	exit(-1);
    } else {
	printf("%03d cr_checkpoint OK\n", count);
    }
    free(cmd);
}

static void do_restart(int count, int orig_pid, int pid_opt, int pgid_opt, int sid_opt)
{
    cr_restart_args_t args;
    cr_restart_handle_t handle;
    int flags;
    int rc;
    int fd;
    int pid;

    flags  = pid_opt  ? CR_RSTRT_RESTORE_PID  : 0;
    flags |= pgid_opt ? CR_RSTRT_RESTORE_PGID : 0;
    flags |= sid_opt  ? CR_RSTRT_RESTORE_SID  : 0;

    fd = open(filename, O_RDONLY|O_LARGEFILE);
    if (fd < 0) {
        perror("XXX open(context_file) failed");
    }

    cr_initialize_restart_args_t(&args);
    args.cr_fd = fd;
    args.cr_flags &= ~(CR_RSTRT_RESTORE_PID|CR_RSTRT_RESTORE_PGID|CR_RSTRT_RESTORE_SID);
    args.cr_flags |= flags;

    rc = cr_request_restart(&args, &handle);
    if (rc < 0) {
	printf("XXX: cr_restart_request() failed: %s", cr_strerror(errno));
    }

    rc = cr_poll_restart(&handle, NULL);
    if (rc < 0) {
	printf("XXX: cr_poll_request() failed: %s", cr_strerror(errno));
    }
    pid = rc;

    if (pid_opt) {
	if (pid != orig_pid) {
	    printf("XXX restart w/ flags=0x%x has the wrong pid\n", flags);
	}
    } else {
	if (pid == orig_pid) {
	    printf("XXX restart w/ flags=0x%x unexpectedly has the original pid\n", flags);
	}
    }

    crut_waitpid_expect(pid, EBUSY + 1 + flags);

    printf("%03d restart w/ flags=0x%x produced the expected result\n", count, flags);
}

int main(int argc, char * const argv[])
{
    cr_client_id_t my_id;
    int my_pid = getpid();
    int pid;
    int count, i, j, k;

    setlinebuf(stdout);

    /* Intialize our globals */
    chkpt_cmd = crut_find_cmd(argv[0], "cr_checkpoint");
    rstrt_cmd = crut_find_cmd(argv[0], "cr_restart");

    printf("000 Process started with pid %d\n", my_pid);
    printf("#ST_ALARM:120\n");

    my_id = cr_init();
    if (my_id < 0) {
	printf("XXX cr_init() failed, returning %d\n", my_id);
	exit(-1);
    } else {
	printf("001 cr_init() succeeded\n");
    }

    pid = fork();
    if (!pid) {
	/* In the child */
	exit(child_main());
    } else if (pid < 0) {
	printf("XXX fork() failed\n");
	exit(-1);
    }
    printf("002 child started with pid %d\n", pid);
    filename = crut_aprintf("context.%d", pid);

    do_checkpoint(3, pid);

    alarm(20);
    crut_waitpid_expect(pid, EBUSY + 1 + (CR_RSTRT_RESTORE_PID|CR_RSTRT_RESTORE_PGID|CR_RSTRT_RESTORE_SID));
    alarm(0);
    printf("004 child with pid %d terminated\n", pid);

    /* Tests all 8 possible combinations */
    count = 5;
    for (i=0; i<2; ++i) {
	for (j=0; j<2; ++j) {
	    for (k=0; k<2; ++k) {
		do_restart(count++, pid, i, j, k);
	    }
	}
    }

    (void)unlink(filename); // might fail silently
    printf("%03d DONE\n", count);

    return 0;
}
