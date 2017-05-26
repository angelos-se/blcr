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
 * $Id: reloc_aux.c,v 1.8 2008/07/26 06:38:15 phargrov Exp $
 *
 * Simple program for testing --relocate
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>


#define FILENAME_FMT "%s/reloc_tmp"
#define UFILENAME_FMT "%s/reloc_utmp"
#define FIFONAME_FMT "%s/reloc_fifo"
#define UFIFONAME_FMT "%s/reloc_ufifo"
#define DIRNAME_FMT "%s/reloc_dir"

#include "crut_util.h" /* For crut_find_cmd() */

static const char *chkpt_cmd;

void checkpoint_self(const char *filename)
{
    char *cmd = NULL;
    int ret;

    cmd = crut_aprintf("exec %s --file %s %d", chkpt_cmd, filename, (int)getpid());
    ret = system(cmd);
    if (!WIFEXITED(ret) || WEXITSTATUS(ret)) {
	fprintf(stderr, "'%s' failed %d\n", cmd, ret);
	exit(1);
    }
    free(cmd);
}

static const char msg[] = "This is just fine.";

static int prep_file(const char *name) {
    int fd = open(name, O_CREAT|O_RDWR|O_TRUNC, S_IREAD|S_IWRITE); 
    if (fd < 0) {
    	perror("open(file)");
    	exit(-1);
    }
    if (write(fd, msg, sizeof(msg)) < 0) {
    	perror("write(file)");
    	exit(-1);
    }
    return fd;
}

static void check_file(int fd) {
    int rc;
    char buf[1024];
    if (lseek(fd, 0, SEEK_SET) == (loff_t)(-1)) {
    	perror("lseek(file)");
    	exit(-1);
    }
    rc = read(fd, buf, sizeof(msg));
    if (rc != sizeof(msg)) {
    	perror("read(file)");
    	exit(-1);
    }
    if (strcmp(msg, buf)) {
    	fprintf(stderr, "Compare of file to original failed\n");
    	exit(-1);
    }
    close(fd);
}

static int prep_fifo(const char *name) {
    int fd;
    if (mknod(name, 0600 | S_IFIFO, 0)) {
    	perror("mknod(fifo)");
    	exit(-1);
    }
    fd = open(name, O_RDWR);
    if (fd < 0) {
    	perror("open(fifo)");
    	exit(-1);
    }
    return fd;
}

static void check_fifo(int fd) {
    int rc;
    char buf[1024];
    rc = write(fd, msg, sizeof(msg));
    if (rc != sizeof(msg)) {
    	perror("write(fifo)");
    	exit(-1);
    }
    rc = read(fd, buf, sizeof(msg));
    if (rc != sizeof(msg)) {
    	perror("read(fifo)");
    	exit(-1);
    }
    if (strcmp(msg, buf)) {
    	fprintf(stderr, "Data in fifo is incorrect\n");
    	exit(-1);
    }
    close(fd);
}

int main(int argc, const char *argv[]) {
    int file_fd = -1;
    int ufile_fd = -1;
    int fifo_fd = -1;
    int ufifo_fd = -1;
    char *filename;
    const char *dirname;
    const char *context;

    if (argc != 3) {
    	fprintf(stderr, "%s expects exactly 2 arguments: DIR CONTEXTFILE\n", argv[0]);
    	exit(-1);
    }
    chkpt_cmd = crut_find_cmd(argv[0], "cr_checkpoint");
    dirname = argv[1];
    context = argv[2];

    /* Create and write a file */
    filename = crut_aprintf(FILENAME_FMT, dirname);
    file_fd = prep_file(filename);
    free(filename);

    /* Create and write a filem, then unlink */
    filename = crut_aprintf(UFILENAME_FMT, dirname);
    ufile_fd = prep_file(filename);
    if (unlink(filename)) {
    	perror("unlink(file)");
    	exit(-1);
    }
    free(filename);

    /* mknod() and open() a FIFO */
    filename = crut_aprintf(FIFONAME_FMT, dirname);
    fifo_fd = prep_fifo(filename);
    free(filename);

    /* mknod() and open() a FIFO, then unlink */
    filename = crut_aprintf(UFIFONAME_FMT, dirname);
    ufifo_fd = prep_fifo(filename);
    if (unlink(filename)) {
    	perror("unlink(fifo)");
    	exit(-1);
    }
    free(filename);

    /* MKDIR and CD to the working directory */
    filename = crut_aprintf(DIRNAME_FMT, dirname);
    if (mkdir(filename, 0755)) {
    	perror("mkdir()");
    	exit(-1);
    }
    if (chdir(filename)) {
    	perror("chdir()");
    	exit(-1);
    }
    free(filename);

    /* Take the checkpoint */
    checkpoint_self(context);

    /* Seek, read and compare open() files */
    check_file(file_fd);
    check_file(ufile_fd);

    /* write, read and compare open() fifos */
    check_fifo(fifo_fd);
    check_fifo(ufifo_fd);

    return 0;
}
