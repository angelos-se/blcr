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
 * $Id: save_aux.c,v 1.8 2008/07/26 06:38:15 phargrov Exp $
 *
 * Simple program to generate mmaps() as requred for testing --save-* options
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#include "crut_util.h"

static const char *chkpt_cmd;
void checkpoint_self(const char *chkpt_args)
{
    char *cmd = NULL;
    int ret;

    cmd = crut_aprintf("exec %s %s %d", chkpt_cmd, chkpt_args, (int)getpid());
    ret = system(cmd);
    if (!WIFEXITED(ret) || WEXITSTATUS(ret)) {
	fprintf(stderr, "'%s' failed %d (%d)\n", cmd, ret, WEXITSTATUS(ret));
	exit(1);
    }
    free(cmd);
}

void validate(volatile char **addr, int count, int pagesize) {
    int i;

    for (i=0; i <count; ++i) {
	if (*addr[i] != 1) {
	    fprintf(stderr, "Failed to read back from first page\n");
	    exit(1);
	}
	if (*(addr[i] + pagesize) != 0) {
	    fprintf(stderr, "Failed to read back from last page\n");
	    exit(1);
	}
    }

}

enum {
	MSG_CHILD_READY = 12,
	MSG_CHKPT_DONE
};

int main(int argc, const char **argv) {
    struct crut_pipes pipes;
    volatile char **addr;
    const char *chkpt_args;
    pid_t pid;
    int pagesize = getpagesize();
    int i;

    if (argc < 2) {
	fprintf(stderr, "%s: need chkpt_args as 1st argument\n", argv[0]);
	exit(1);
    }
    chkpt_cmd = crut_find_cmd(argv[0], "cr_checkpoint");
    chkpt_args = argv[1];
    addr = calloc(argc-2, sizeof(char *));

    for (i=2; i <argc; ++i) {
	char *filename;
	int flags, fd;
	volatile char *tmp;

	/* Parse mode arg */
	switch (argv[i][0]) {
	case 'S': flags = MAP_SHARED; break;
	case 'P': flags = MAP_PRIVATE; break;
	default:  fprintf(stderr, "Invalid argument '%s'\n", argv[i]);
		  exit(-1);
	}

	/* Create a file */
	filename = crut_aprintf("tstmap%d", i-1);
	fd = open(filename, O_CREAT|O_RDWR|O_TRUNC, S_IREAD|S_IWRITE); 
	if (fd < 0) {
		perror("open()");
		exit(-1);
	}

	/* Extend to 2 pages in length */
	if (ftruncate(fd, 2*pagesize) < 0) {
		perror("ftruncate()");
		exit(-1);
	}

	/* mmap() it */
	tmp = mmap(NULL, 2*pagesize, PROT_READ|PROT_WRITE, flags, fd, 0);
	if (tmp == MAP_FAILED) {
		perror("mmap()");
		exit(-1);
	}
	(void)close(fd);

	/* dirty first page */
	*tmp = 1;

	addr[i-2] = tmp;
    }

    /* Fork a child to do like us */
    pid = crut_pipes_fork(&pipes);

    if (pid) {
        /* Parent */
	crut_pipes_expect(&pipes, MSG_CHILD_READY);
	checkpoint_self(chkpt_args);
	crut_pipes_putchar(&pipes, MSG_CHKPT_DONE);
	validate(addr, argc-2, pagesize);
	crut_waitpid_expect(pipes.child, 0);
	crut_pipes_close(&pipes);
    } else {
	/* Child */
	crut_pipes_putchar(&pipes, MSG_CHILD_READY);
	crut_pipes_expect(&pipes, MSG_CHKPT_DONE);
	validate(addr, argc-2, pagesize);
    }

    return 0;
}
