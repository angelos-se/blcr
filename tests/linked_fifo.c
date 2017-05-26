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
 * $Id: linked_fifo.c,v 1.9 2008/08/29 21:36:53 phargrov Exp $
 *
 * tests of named fifos in which the two ends are through distinct hard links
 */

/*
 * NOTE:  We rely on the Linux-specific (POSIX undefined) behavior for O_RDWR 
 * pipes.  Linux does not block while opening a named FIFO with O_RDWR
 */

#define _LARGEFILE64_SOURCE 1   /* For O_LARGEFILE */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include "crut.h"

#define MAX_FILENAME_LENGTH 255
#define FIFO_FILENAME "tstfifo"
#define BUFLEN 256
#define PIPEDATA "Hello, world!\n"
#define FIFO_MODE 0644

/*
 * 64-bit kernels are some what "free" with O_LARGEFILE since it is just a
 * no-op for sizeof(long) == 8
 */
#if O_LARGEFILE
  #define FLAGS_TO_IGNORE	O_LARGEFILE
#else
  #define FLAGS_TO_IGNORE	0
#endif

/*
 * name is somewhat of a misnomer, since this is really a pair...
 */
struct test_fifo_struct {
    char *filename;
    char *filename2;
    int rdwr_fd;                        /* open first to avoid blocking */
    int client_fd;                      /* test most IO ops to this */
    int flags;                          /* rd, wr, or rdwr */
    struct stat stat;
};

static int sigpiped = 0;

void
sigpipe_handler(int num)
{
   sigpiped = 1;
}

struct test_fifo_struct *
get_fifo_struct(const char *filename, int flags)
{
    int ret=-1;
    struct test_fifo_struct *fifo;
    int tmp_fd;

    fifo = malloc(sizeof(*fifo));
    if (fifo == NULL) {
        perror("malloc");
	goto out;
    }

    fifo->filename = strdup(filename);
    if (fifo->filename == NULL) {
        perror("strdup");
	goto out_free;
    }

    fifo->filename2 = malloc(strlen(filename) + 2);
    if (fifo->filename2 == NULL) {
	perror("malloc filename2");
	goto out_free;
    }
    strcpy(fifo->filename2, fifo->filename);
    strcat(fifo->filename2, "2");

    (void)unlink(fifo->filename);
    (void)unlink(fifo->filename2);

    ret = mknod(fifo->filename, FIFO_MODE | S_IFIFO, 0);
    if (ret < 0) {
        perror("mknod");
	goto out_free;
    }

    ret = link(fifo->filename, fifo->filename2);
    if (ret) {
	perror("link");
	goto out_unlink;
    }

    fifo->rdwr_fd = open(fifo->filename, O_RDWR);
    if (fifo->rdwr_fd < 0) {
        perror("open(rdwr_fd)");
	goto out_unlink;
    }

    fifo->client_fd = open(fifo->filename2, flags);
    if (fifo->client_fd < 0) {
        perror("open(client_fd)");
        goto out_unlink;
    }

    ret = fcntl(fifo->client_fd, F_GETFL);
    if (ret < 0) {
	perror("fcntl");
	goto out_unlink;
    }
    fifo->flags = ret & ~FLAGS_TO_IGNORE;

    /* Swap order to test the restart code */
    tmp_fd = fifo->rdwr_fd;
    fifo->rdwr_fd = dup(fifo->rdwr_fd);
    if (fifo->rdwr_fd < 0) {
        perror("dup(rdwr_fd)");
	goto out_unlink;
    }
    ret = close(tmp_fd);
    if (ret < 0) {
        perror("close(tmp_fd)");
	goto out_unlink;
    }

out:
    return fifo;

out_unlink:
    (void)unlink(fifo->filename);
    (void)unlink(fifo->filename2);
out_free:
    free(fifo->filename); /* Note: NULL is OK by spec. */
    free(fifo->filename2); /* Note: NULL is OK by spec. */
    free(fifo);
    return NULL;
}

/*
 * This should write a atomic amount of data.  Small means that
 * the pipe does not block.
 *
 * Returns number of bytes written.
 */
static int
write_atomic(int fd)
{
    char buf[BUFLEN];
    int retval;

    (void)strncpy(buf, PIPEDATA, sizeof(PIPEDATA));

    CRUT_DEBUG("Writing to pipe");
    retval = write(fd, buf, BUFLEN);
    if (sigpiped) {
        retval = -1;
        CRUT_FAIL("SIGPIPE on write");
    }
    if (retval < 0) {
        perror("write_atomic");
    } else if (retval != BUFLEN) {
        CRUT_FAIL("Wrote wrong length to pipe %d", fd);
    } else if(retval == BUFLEN) {
        retval = 0;
    }

    return retval;
}

/*
 * Logical complement of read atomic.
 * Checks to make sure that the buffer read back is the same as the one
 * written.
 *
 * Returns number of bytes read.
 *    Returns < 0 if buffers don't compare, or other error.
 */
static int
read_atomic(int fd)
{
    int retval;
    int bytes_read;
    char buf[BUFLEN];

    memset(buf, 0, BUFLEN);
    CRUT_DEBUG("Reading from pipe.");
    retval = read(fd, buf, BUFLEN);
    if (retval < 0) {
        CRUT_FAIL("Couldn't read from pipe.");
        CRUT_FAIL("buf='%s'", buf);
        goto out;
    }
    bytes_read = retval;

    CRUT_DEBUG("Comparing pipe data.");
    retval = strncmp(PIPEDATA, buf, sizeof(PIPEDATA));
    if (retval < 0) {
        CRUT_FAIL("strncmp(%s, %s) failure.", PIPEDATA, buf);
        goto out;
    }
    
out:
    return retval;  
}

static int
flags_check_fifo(struct test_fifo_struct *f)
{
    int retval;

    retval = fcntl(f->client_fd, F_GETFL);
    if (retval < 0) {
	perror("fnctl(F_GETFL)");
    } else if ((retval & ~FLAGS_TO_IGNORE) !=  f->flags) {
	CRUT_FAIL("Post-restart flags %d don't match original %d\n", retval, f->flags);
	retval = -1;
    }

    return retval;
}

static int
stat_check_fifo(struct test_fifo_struct *f)
{
    struct stat s1, s2;
    int retval = 0;

    if ((fstat(f->rdwr_fd, &s1) < 0) || (fstat(f->client_fd, &s2) < 0)) {
        perror("fstat");
	retval = -1;
	goto out;
    } else if (statcmp(&s1, &s2, ST_DEV | ST_INO | ST_NLINK)) {
	fprintf(stderr, "Post-restart fifos are not linked\n");
	retval = -1;
	goto out;
    }

    retval = statcmp(&s1, &(f->stat), ST_SIZE | ST_MODE /* | ST_NLINK broken on NFS */);
    if (retval) {
	CRUT_FAIL("File attributes changed.  %d mismatches", retval);
        CRUT_DEBUG("--- Old stats ---");
        dump_stat(&(f->stat));
        CRUT_DEBUG("--- Current stats ---");
        dump_stat(&s1);
	retval = -1;
    }

out:
    return retval;
}

static int
ring_test_fifo(struct test_fifo_struct *f)
{
    int retval;
    
    /*
     * loops some data through the pipes to make sure they still work
     */
    switch((f->flags) & O_ACCMODE) {
      case O_RDONLY:
	retval = write_atomic(f->rdwr_fd);
	if (!retval) retval = read_atomic(f->client_fd);
	break;
      case O_WRONLY:
      case O_RDWR:
	retval = write_atomic(f->client_fd);
	if (!retval) read_atomic(f->rdwr_fd);
	break;
      default:
	retval = -1;
    }

    return retval;
}

static int
linked_fifo_generic_setup(void **testdata, int flags)
{
    int retval=-1;
    struct test_fifo_struct *fifo;

    signal(SIGPIPE, sigpipe_handler);

    fifo = get_fifo_struct(FIFO_FILENAME, flags);
    if (fifo == NULL) {
        goto out;
    }

    *testdata = fifo;
    retval = 0;
out:
    return retval;
}

static int
linked_fifo_read_setup(void **testdata)
{
    int retval;

    retval = linked_fifo_generic_setup(testdata, O_RDONLY);

    return retval;
}

static int
linked_fifo_write_setup(void **testdata)
{
    int retval;

    retval = linked_fifo_generic_setup(testdata, O_WRONLY);

    return retval;
}

static int
linked_fifo_rdwr_setup(void **testdata)
{
    int retval;

    retval = linked_fifo_generic_setup(testdata, O_RDWR);

    return retval;
}

static int
linked_fifo_read_nonblock_setup(void **testdata)
{
    int retval;

    retval = linked_fifo_generic_setup(testdata, O_RDONLY | O_NONBLOCK);

    return retval;
}

static int
linked_fifo_write_nonblock_setup(void **testdata)
{
    int retval;

    retval = linked_fifo_generic_setup(testdata, O_WRONLY | O_NONBLOCK);

    return retval;
}

static int
linked_fifo_rdwr_nonblock_setup(void **testdata)
{
    int retval;

    retval = linked_fifo_generic_setup(testdata, O_RDWR | O_NONBLOCK);

    return retval;
}

static int
linked_fifo_buffered_precheckpoint(void *p)
{
    int retval;
    struct test_fifo_struct *f = p;
    
    switch((f->flags) & O_ACCMODE) {
      case O_RDONLY:
	/* reads from client_fd, so write to rdwr_fd */
	retval = write_atomic(f->rdwr_fd);
	break;
      case O_WRONLY:
      case O_RDWR:
	/* writes to client_fd */
	retval = write_atomic(f->client_fd);
	break;
      default:
	retval = -1;
    }
    if (retval < 0) goto out;

    retval = fstat(f->rdwr_fd, &f->stat);
    if (retval < 0) {
	perror("fstat");
    }

out:
    return retval;
}

static int
linked_fifo_precheckpoint(void *p)
{
    struct test_fifo_struct *fifo = (struct test_fifo_struct *)p;
    int retval;

    retval = fstat(fifo->rdwr_fd, &fifo->stat);
    if (retval < 0) {
	perror("fstat");
    }

    return retval;
}

static int
linked_fifo_precheckpoint_unlink(void *p)
{
    struct test_fifo_struct *fifo = (struct test_fifo_struct *)p;
    int retval;

    retval = unlink(fifo->filename);
    if (retval) {
	perror("unlink pre");
	goto out;
    }
    retval = unlink(fifo->filename2);
    if (retval) {
	perror("unlink2 pre");
	goto out;
    }

    retval = linked_fifo_precheckpoint(p);

out:
    return retval;
}

/*
 * Unlinks the pipe after a checkpoint, to see if we can mkfifo in the
 * kernel correctly on restart.
 */
static int
linked_fifo_continue_unlink(void *p)
{
    struct test_fifo_struct *fifo = (struct test_fifo_struct *)p;
    int retval;

    retval = unlink(fifo->filename);
    if (retval) {
	perror("unlink continue");
	goto out;
    }
    retval = unlink(fifo->filename2);
    if (retval) {
	perror("unlink2 continue");
	goto out;
    }

out:
    return retval;
}

static int
linked_fifo_continue(void *p)
{
    int retval;

    retval = 0;

    return retval;
}

/*
 * This should take care of everything...
 */
static int
linked_fifo_buffered_restart(void *p)
{
    int retval;
    struct test_fifo_struct *f = p;
    
    /* Check that correct flags have been restored */
    retval = flags_check_fifo(f);
    if (retval < 0)
	return retval;

    /* Check that they are still linked and that stat()s are OK*/
    retval = stat_check_fifo(f);
    if (retval < 0)
	return retval;

    switch((f->flags) & O_ACCMODE) {
      case O_RDONLY:
	/* read from client_fd */
	retval = read_atomic(f->client_fd);
	break;
      case O_WRONLY:
      case O_RDWR:
	/* wrote to client_fd, read from rdwr_fd */
	retval = read_atomic(f->rdwr_fd);
	break;
      default:
	retval = -1;
    }

    /* if that worked, ring the pipe once just to check sanity. */
    if (!retval) {
        retval = ring_test_fifo(f);
    }

    return retval;
}

static int
linked_fifo_restart(void *p)
{
    struct test_fifo_struct *f = p;
    int retval;
    
    /* Check that correct flags have been restored */
    retval = flags_check_fifo(f);
    if (retval < 0) {
	return retval;
    }

    /* Check that they are still linked and that stat()s are OK*/
    retval = stat_check_fifo(f);
    if (retval < 0)
	return retval;

    /* if that worked, ring the pipe once just to check sanity. */
    return ring_test_fifo(f);
}

static int
linked_fifo_teardown(void *p)
{
    struct test_fifo_struct *fifo = (struct test_fifo_struct *)p;
    int retval;

    retval = unlink(fifo->filename);
    if (retval) {
	perror("unlink teardown");
	goto out;
    }
    retval = unlink(fifo->filename2);
    if (retval) {
	perror("unlink2 teardown");
	goto out;
    }

out:
    free(fifo->filename);
    free(fifo->filename2);
    free(fifo);

    return retval;
}

static int
linked_fifo_teardown_preunlink(void *p)
{
    struct test_fifo_struct *fifo = (struct test_fifo_struct *)p;
    int retval;

    /* Unlink must fail, since fifo should have been renamed */
    retval = !unlink(fifo->filename);
    if (retval) {
	fprintf(stderr, "unlink succeeded unexpectedly\n");
	goto out;
    }
    retval = !unlink(fifo->filename2);
    if (retval) {
        fprintf(stderr, "unlink2 succeeded unexpectedly\n");
	goto out;
    }

out:
    free(fifo->filename);
    free(fifo->filename2);
    free(fifo);

    return retval;
}

int
main(int argc, char *argv[])
{
    int ret;

    /*
     * cases for named fifos...
     *
     * rd, wr, rdwr - normal
     * rd, wr, rdwr - unlinked after checkpoint
     * rd, wr, rdwr - unlinked before checkpoint
     * rd, wr, rdwr - unlinked
     * rd, wr, rdwr - data in the pipe
     * rd, wr, rdwr - nonblocking
     *
     * Only care about the above, this tests most code paths, could form
     * arbitrarily silly combinations, though...
     */

    struct crut_operations linked_fifo_rw_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"linked_fifo_rw",
        test_description:"Tests basic IO on fifos",
	test_setup:linked_fifo_rdwr_setup,
	test_precheckpoint:linked_fifo_precheckpoint,
	test_continue:linked_fifo_continue,
	test_restart:linked_fifo_restart,
	test_teardown:linked_fifo_teardown,
    };

    struct crut_operations linked_fifo_rw_unlink_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"linked_fifo_rw_unlink",
        test_description:"Tests whether we mknod FIFOs on restart",
	test_setup:linked_fifo_rdwr_setup,
	test_precheckpoint:linked_fifo_precheckpoint,
	test_continue:linked_fifo_continue_unlink,
	test_restart:linked_fifo_restart,
	test_teardown:linked_fifo_teardown,
    };

    struct crut_operations linked_fifo_rw_preunlink_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"linked_fifo_rw_preunlink",
        test_description:"Tests whether we checkpoint unlinked FIFOs",
	test_setup:linked_fifo_rdwr_setup,
	test_precheckpoint:linked_fifo_precheckpoint_unlink,
	test_continue:linked_fifo_continue,
	test_restart:linked_fifo_restart,
	test_teardown:linked_fifo_teardown_preunlink,
    };

    struct crut_operations linked_fifo_rw_buffered_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"linked_fifo_rw_buffered",
        test_description:"Makes sure that data in the FIFO is recovered after a checkpoint.",
	test_setup:linked_fifo_rdwr_setup,
	test_precheckpoint:linked_fifo_buffered_precheckpoint,
	test_continue:linked_fifo_continue,
	test_restart:linked_fifo_buffered_restart,
	test_teardown:linked_fifo_teardown,
    };

    struct crut_operations linked_fifo_rw_nonblock_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"linked_fifo_rw_nonblock",
        test_description:"Makes sure that data in the FIFO is recovered after a checkpoint.",
	test_setup:linked_fifo_rdwr_nonblock_setup,
	test_precheckpoint:linked_fifo_buffered_precheckpoint,
	test_continue:linked_fifo_continue,
	test_restart:linked_fifo_buffered_restart,
	test_teardown:linked_fifo_teardown,
    };

    struct crut_operations linked_fifo_read_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"linked_fifo_read",
        test_description:"Tests basic IO on fifos",
	test_setup:linked_fifo_read_setup,
	test_precheckpoint:linked_fifo_precheckpoint,
	test_continue:linked_fifo_continue,
	test_restart:linked_fifo_restart,
	test_teardown:linked_fifo_teardown,
    };

    struct crut_operations linked_fifo_read_unlink_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"linked_fifo_read_unlink",
        test_description:"Tests whether we mknod FIFOs on restart",
	test_setup:linked_fifo_read_setup,
	test_precheckpoint:linked_fifo_precheckpoint,
	test_continue:linked_fifo_continue_unlink,
	test_restart:linked_fifo_restart,
	test_teardown:linked_fifo_teardown,
    };

    struct crut_operations linked_fifo_read_preunlink_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"linked_fifo_read_preunlink",
        test_description:"Tests whether we checkpoint unlinked FIFOs",
	test_setup:linked_fifo_rdwr_setup,
	test_precheckpoint:linked_fifo_precheckpoint_unlink,
	test_continue:linked_fifo_continue,
	test_restart:linked_fifo_restart,
	test_teardown:linked_fifo_teardown_preunlink,
    };

    struct crut_operations linked_fifo_read_buffered_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"linked_fifo_read_buffered",
        test_description:"Makes sure that data in the FIFO is recovered after a checkpoint.",
	test_setup:linked_fifo_read_setup,
	test_precheckpoint:linked_fifo_buffered_precheckpoint,
	test_continue:linked_fifo_continue,
	test_restart:linked_fifo_buffered_restart,
	test_teardown:linked_fifo_teardown,
    };

    struct crut_operations linked_fifo_read_nonblock_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"linked_fifo_read_nonblock",
        test_description:"Makes sure that data in the FIFO is recovered after a checkpoint.",
	test_setup:linked_fifo_read_nonblock_setup,
	test_precheckpoint:linked_fifo_buffered_precheckpoint,
	test_continue:linked_fifo_continue,
	test_restart:linked_fifo_buffered_restart,
	test_teardown:linked_fifo_teardown,
    };

    struct crut_operations linked_fifo_write_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"linked_fifo_write",
        test_description:"Tests basic IO on fifos",
	test_setup:linked_fifo_write_setup,
	test_precheckpoint:linked_fifo_precheckpoint,
	test_continue:linked_fifo_continue,
	test_restart:linked_fifo_restart,
	test_teardown:linked_fifo_teardown,
    };

    struct crut_operations linked_fifo_write_unlink_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"linked_fifo_write_unlink",
        test_description:"Tests whether we mknod FIFOs on restart",
	test_setup:linked_fifo_write_setup,
	test_precheckpoint:linked_fifo_precheckpoint,
	test_continue:linked_fifo_continue_unlink,
	test_restart:linked_fifo_restart,
	test_teardown:linked_fifo_teardown,
    };

    struct crut_operations linked_fifo_write_preunlink_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"linked_fifo_write_preunlink",
        test_description:"Tests whether we checkpoint unlinked FIFOs",
	test_setup:linked_fifo_rdwr_setup,
	test_precheckpoint:linked_fifo_precheckpoint_unlink,
	test_continue:linked_fifo_continue,
	test_restart:linked_fifo_restart,
	test_teardown:linked_fifo_teardown_preunlink,
    };

    struct crut_operations linked_fifo_write_buffered_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"linked_fifo_write_buffered",
        test_description:"Makes sure that data in the FIFO is recovered after a checkpoint.",
	test_setup:linked_fifo_write_setup,
	test_precheckpoint:linked_fifo_buffered_precheckpoint,
	test_continue:linked_fifo_continue,
	test_restart:linked_fifo_buffered_restart,
	test_teardown:linked_fifo_teardown,
    };

    struct crut_operations linked_fifo_write_nonblock_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"linked_fifo_write_nonblock",
        test_description:"Makes sure that data in the FIFO is recovered after a checkpoint.",
	test_setup:linked_fifo_write_nonblock_setup,
	test_precheckpoint:linked_fifo_buffered_precheckpoint,
	test_continue:linked_fifo_continue,
	test_restart:linked_fifo_buffered_restart,
	test_teardown:linked_fifo_teardown,
    };

    /* add the basic tests */
    crut_add_test(&linked_fifo_read_ops);
    crut_add_test(&linked_fifo_write_ops);
    crut_add_test(&linked_fifo_rw_ops);

    crut_add_test(&linked_fifo_read_unlink_ops);
    crut_add_test(&linked_fifo_write_unlink_ops);
    crut_add_test(&linked_fifo_rw_unlink_ops);

    crut_add_test(&linked_fifo_read_preunlink_ops);
    crut_add_test(&linked_fifo_write_preunlink_ops);
    crut_add_test(&linked_fifo_rw_preunlink_ops);

    crut_add_test(&linked_fifo_read_buffered_ops);
    crut_add_test(&linked_fifo_write_buffered_ops);
    crut_add_test(&linked_fifo_rw_buffered_ops);

    crut_add_test(&linked_fifo_read_nonblock_ops);
    crut_add_test(&linked_fifo_write_nonblock_ops);
    crut_add_test(&linked_fifo_rw_nonblock_ops);


    ret = crut_main(argc, argv);

    exit(ret);
}
