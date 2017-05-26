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
 * $Id: pipe_counting.c,v 1.4 2008/09/03 04:13:02 phargrov Exp $
 */


#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

static int limit = 120; /* default */

int do_read(int fd, char *p, size_t len) {
    int rc;
    while (len) {
	    rc = read(fd, p ,len);
	    if (rc < 0 && errno == EINTR) continue;
	    if (rc <= 0) return rc;
	    len -= rc;
	    p += rc;
    }
    return 1;
}

int main(int argc, char *argv[])
{
    int exitcode = 0;
    int fds[2];
    pid_t pid;
    int i;
	
    printf("Counting demo starting with pid %d, pgid %d\n", (int)getpid(), (int)getpgrp());

    if (argc > 1) {
	limit = (i = atoi(argv[1])) > 0 ? i : limit;
    }

    (void)pipe(fds);
    pid = fork();
    if (pid == 0) {
	/* CHILD */
	int value;
	close(fds[1]);
	while (do_read(fds[0], (char*)&value, sizeof(int))) {
	    printf("Count = %d\n", value);
	    fflush(stdout);
	}
	if (value != (limit-1)) {
	    fprintf(stderr, "FAILED: premature EOF at value = %d\n", value);
	    exitcode = 1;
	}
    } else if (pid > 0) {
	/* PARENT */
        int i, status;
	close(fds[0]);
        for (i=0; i<limit; ++i) {
	    write(fds[1], &i, sizeof(int));
	    sleep(1);
        }
	close(fds[1]); /* generates EOF in child */
	i = waitpid(pid, &status, 0);
	if (i != (int)pid) {
	    fprintf(stderr, "FAILED: waitpid(%d) returned %d (errno=%d %s)\n", (int)pid, i, errno, strerror(errno));
	    exitcode = 2;
	} else if (WIFEXITED(status)) {
	    exitcode = WEXITSTATUS(status);
	}
	if (!exitcode) {
	    printf("SUCCESS\n");
	}
    }

    return exitcode;
}
