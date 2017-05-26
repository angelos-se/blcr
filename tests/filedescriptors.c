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
 * $Id: filedescriptors.c,v 1.23 2008/08/29 21:36:53 phargrov Exp $
 *
 * Simple example for using crut
 */

/*
 * >KEYS:  < open()
 * >WHAT:  < Does a read on a file opened with oflag = O_WRONLY  return -1?
 *         < Does a write on a file opened with oflag = O_RDONLY  return -1?
 *         >HOW:   < Open a file with O_WRONLY, test for -1 to pass
 *                 < Open a file with O_RDONLY, test for -1 to pass
 *                 >BUGS:  < DUMMY functions used in BSD
 *                 >AUTHOR:< PERENNIAL
 */

/*
 * seek tests:
 *
 * writes pattern to a file
 * seek backwards
 * checkpoint
 * assert fd->offset == lseek()
 * read 
 * check pattern
 *
 * fill a file with zeros
 * seek to start
 * checkpoint
 * assert fd->offset == lseek()
 * write pattern
 * read
 * check pattern
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
#include <errno.h>

#include "crut.h"

#define DEFAULT_TEST_DATA_LENGTH 8192
#define MAX_TEST_FILENAME 255
#define MAX_TEST_FILES 32
#define TEST_PREFIX "tstfile"
#define TEST_FILE_MODE 0660
#define TEST_FILE_MODE_RO 0440
#define BUF_SIZE 4096

struct fd_struct {
    int fd;
    int flags;
    off_t offset;
    struct stat stat;
    int unlinked;
    char filename[MAX_TEST_FILENAME+1];
};

int test_flags[] = {
#ifdef O_DIRECT
    O_DIRECT,
#endif
#ifdef O_LARGEFILE
    O_LARGEFILE,
#endif
    O_SYNC,
    O_NONBLOCK,
    O_NDELAY,
    O_APPEND,
};

/*
 * Linux 2.4 does not save these flags.  So we assume that they're not going
 * to be saved, and test for them later when you talk to fcntl.
 */
#define FLAGS_NOT_RESTORED (O_CREAT | O_EXCL | O_NOCTTY | O_TRUNC)

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
 * write_pattern
 *
 * write a test pattern to this file descriptor
 *
 * note:
 * write_pattern(f, len1);
 * write_pattern(f, len2);
 *
 * should produce the same output as
 * write_pattern(f, len1+len2);
 */
static int
write_pattern(int fd, int length)
{
    int retval = -1;
    off_t seed;
    char *buf;

    seed = lseek(fd, 0, SEEK_CUR);
    if (seed < 0) {
        goto out;
    }

    buf = pattern_get_data(length, seed);
    if (buf == NULL) {
        goto out;
    }

    retval = write(fd, buf, length);
    if (retval < 0) {
        perror("write");
    }

    free(buf);
out:
    return retval;
}

/*
 * check_pattern
 *
 * checks that length bytes in fd match our test pattern
 */
static int
check_pattern(int fd, int length)
{
    int retval = -1;
    int seed;
    char *buf;

    seed = lseek(fd, 0, SEEK_CUR);
    if (seed < 0) {
        perror("lseek");
        goto out;
    }

    buf = malloc(length);
    if (buf == NULL) {
        perror("malloc");
        goto out;
    }

    retval = read(fd, buf, length);
    if (retval < 0) {
        perror("read");
        goto out_free;
    }

    retval = pattern_compare(buf, length, seed);
    if (retval < 0) {
        CRUT_FAIL("File failed to match expected contents at recovery.");
    }

out_free:
    free(buf);
out:
    return retval;
}

static int
check_flags(struct fd_struct *f)
{
    int retval = -1;
    int flags;

    /* check the flags */
    flags = fcntl(f->fd, F_GETFL);
    if (flags < 0) {
	perror("fcntl");
	return flags;
    }
    flags &= ~FLAGS_TO_IGNORE;
    if (flags != f->flags) {
        CRUT_FAIL("Flag 0%o from fcntl does not match saved value of 0%o", flags, f->flags);
    } else {
        retval = 0;
    }

    /*
     * Linux 2.4 does not save certain flags.  So we assume that they're not 
     * going to be saved, and test for them.
     *
     * All sorts of things will break if this ever happens...
     */
    if (flags & (FLAGS_NOT_RESTORED)) {
        CRUT_FAIL("Kernel flag behavior changed!!!");
	retval = -1;
    }

    return retval;
}

/* 
 * make sure the file pointer was correctly positioned
 */
static int
check_offset(struct fd_struct *f)
{
    int retval = -1;
    int offset;

    offset = lseek(f->fd, 0, SEEK_CUR);
    if (offset != f->offset) {
        CRUT_FAIL("File pointer positioned incorrectly at recovery.");
    } else if (offset < 0) {
        perror("lseek");
    } else {
        retval = 0;
    }

    return retval;
}

/*
 * check_stat_simple
 * checks basic stats on a regular file...
 */
static int
check_stat_simple(struct fd_struct *f)
{
    int retval = -1;
    struct stat new_stat;

    /* compare the stat structs */
    retval = fstat(f->fd, &new_stat);
    if (retval < 0) {
        perror("fstat");
    } else {
        retval = statcmp(&new_stat, &(f->stat), 
	             ST_SIZE | ST_MODE | ST_NLINK);
        if (retval) {
            CRUT_FAIL("File attributes changed.  %d mismatches", retval);
            CRUT_DEBUG("--- Old stats %p ---", &(f->stat));
            dump_stat(&(f->stat));
            CRUT_DEBUG("--- Current stats ---");
            dump_stat(&new_stat);
	    retval = -1;
        }
    }

    return retval;
}

static int
create_temp_file(const char *filename, int length)
{
    int fd;
    int retval = -1;

    (void)unlink(filename);
    fd = open(filename, O_WRONLY | O_CREAT, TEST_FILE_MODE);

    if (fd < 0) {
        perror("create_temp_file(open)");
        retval = fd;
    } else {
        retval = write_pattern(fd, length);
	(void)close(fd);
    }
    
    return retval;
}

/*
 * fills out the fields in an fd_struct...
 */
static int
update_fd_struct(struct fd_struct *f)
{
    int retval;

    f->offset = lseek(f->fd, 0, SEEK_CUR);
    if (f->offset < 0) {
        retval = -1;
        perror("lseek");
	goto out;
    }

    f->flags = fcntl(f->fd, F_GETFL);
    if (f->flags < 0) {
        retval = -1;
	perror("fcntl");
	goto out;
    }
    f->flags &= ~FLAGS_TO_IGNORE;

    retval = fstat(f->fd, &f->stat);
    if (retval < 0) {
        perror("fstat");
    }

out:
    return retval;
}

/*
 * open up a file
 */
static struct fd_struct *
open_fd_struct(char *filename, int flags, int do_unlink)
{
    struct fd_struct *f;
    int retval;

    f = malloc(sizeof(*f));
    if (f==NULL) {
        perror("malloc");
        goto out;
    }
    memset(f, 0, sizeof(*f));

    strncpy(f->filename, filename, MAX_TEST_FILENAME);
    f->filename[MAX_TEST_FILENAME] = '\0';

    if ((flags & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL)) {
	(void)unlink(f->filename);
    }

    f->fd = open(f->filename, flags, TEST_FILE_MODE);
    if (f->fd < 0) {
        perror("open");
	retval = f->fd;
	goto out_free;
    }

    retval = update_fd_struct(f);
    if (retval < 0) {
        goto out_free;
    }

    f->unlinked = do_unlink;
    if (do_unlink) {
        retval = unlink(filename);
        if (retval < 0) {
	    perror("unlink");
            goto out_free;
        }
    }

out:
    return f;

out_free:
    free(f);
    return NULL;
}

/*
 * close a file
 */
static int
close_fd_struct(struct fd_struct *f)
{
    int retval;

    retval = close(f->fd);
    if (retval < 0) {
        perror("close");
    } 

    free(f);

    return retval;
}

/*
 * flex our muscles, do some basic things to an open file.  
 * read, write, lseek, stat
 */
static int
check_fd_struct_rdwr(struct fd_struct *f)
{
    int retval;

    /* check the flags */
    retval = check_flags(f);
    if (retval < 0) {
	goto out;
    }
    CRUT_DEBUG("check_flags:return = %d", retval);

    /* make sure the file pointer was correctly positioned */
    retval = check_offset(f);
    if (retval < 0) {
	goto out;
    }
    CRUT_DEBUG("check_offset:return = %d", retval);

    /* compare the stat structs */
    retval = check_stat_simple(f);
    if (retval < 0) {
	goto out;
    }
    CRUT_DEBUG("check_stat_simple:return = %d", retval);

    /* write some stuff to the file. */
    retval = write_pattern(f->fd, DEFAULT_TEST_DATA_LENGTH);
    if (retval < 0) {
        goto out;
    }
    CRUT_DEBUG("write_pattern:return = %d", retval);

    /* seek back to beginning */
    retval = lseek(f->fd, 0, SEEK_SET);
    if (retval < 0) {
        perror("lseek");
        goto out;
    }

    /* match the file contents to expected */
    retval = check_pattern(f->fd, 2*DEFAULT_TEST_DATA_LENGTH);
    if (retval < 0) {
        goto out;
    }

    /* truncate back to original length */
    retval = ftruncate(f->fd, DEFAULT_TEST_DATA_LENGTH);
    if (retval < 0) {
        goto out;
    }

out:
    return retval;
}

static int
_fd_read_setup(void **testdata, int do_unlink)
{
    int length = DEFAULT_TEST_DATA_LENGTH;
    int retval;
    struct fd_struct *f = NULL;
    char *filename = crut_aprintf("%s.%d", TEST_PREFIX, 0);

    retval = create_temp_file(filename, length);
    if (retval < 0) {
        goto out;
    }

    f = open_fd_struct(filename, O_RDONLY, do_unlink);
    if (f == NULL) {
        retval = -1;
	goto out;
    }

    retval = lseek(f->fd, 1, SEEK_CUR);
    if (retval < 0) {
        perror("lseek");
        goto out;
    }

    retval = fchmod(f->fd, TEST_FILE_MODE_RO);
    if (retval < 0) {
        perror("fchmod");
        goto out;
    }

out:
    *testdata = f;
    return 0;
}

static int
fd_read_setup(void **testdata) { return _fd_read_setup(testdata, 0); }

static int
fd_read_unlinked_setup(void **testdata) { return _fd_read_setup(testdata, 1); }

/*
 * We create a new file for writing.  After restart, we'll write to it.
 */
static int
_fd_write_setup(void **testdata, int do_unlink) {
    struct fd_struct *f;
    char *filename;

    filename = crut_aprintf("%s.%d", TEST_PREFIX, 0);
    f = open_fd_struct(filename, O_WRONLY | O_CREAT | O_EXCL, do_unlink);
    free(filename);

    *testdata = f;
    if (f == NULL) {
        return -1;
    } else {
        return 0;
    }
}

static int
fd_write_setup(void **testdata) { return _fd_write_setup(testdata, 0); }

static int
fd_write_unlinked_setup(void **testdata) { return _fd_write_setup(testdata, 1); }

/*
 * We create a new file opened for read-write.  We're going to write
 * some data into it now.  At restart we write some more data in,
 * then read it all back.
 */
static int
_fd_rdwr_setup(void **testdata, int do_unlink) {
    int retval = -1;
    struct fd_struct *f;
    char *filename;

    filename = crut_aprintf("%s.%d", TEST_PREFIX, 0);
    f = open_fd_struct(filename, O_RDWR | O_CREAT | O_EXCL, do_unlink);
    if (f == NULL) {
        goto out;
    }

    /* write some stuff to the file. */
    retval = write_pattern(f->fd, DEFAULT_TEST_DATA_LENGTH);
    if (retval < 0) {
        CRUT_FAIL("Unable to write test pattern to file opened O_RDWR");
        goto out_close_unlink;
    }

out:
    *testdata = f;
    free(filename);
    return retval;

out_close_unlink:
    close_fd_struct(f);
    if (!do_unlink && (unlink(filename) < 0)) {
        perror("unlink");
    }
    free(filename);
    return retval;
}

static int
fd_rdwr_setup(void **testdata) { return _fd_rdwr_setup(testdata, 0); }

static int
fd_rdwr_unlinked_setup(void **testdata) { return _fd_rdwr_setup(testdata, 1); }

/*
 * Test open files w/ a bunch of different modes.
 */
static int
_fd_flags_setup(void **testdata, int do_unlink) {
    int i;
    int retval = -1;
    int num_tests = sizeof(test_flags)/sizeof(int);
    int length = DEFAULT_TEST_DATA_LENGTH;
    char *filename;
    struct fd_struct **fds;

    /* get us some space and init... */
    fds = malloc(num_tests*sizeof(struct fd_struct));
    if (fds == NULL) {
        goto out;
    }
    for (i=0; i<num_tests; ++i) 
	fds[i] = NULL;

    /* make a temp file and fill it with stuff */
    filename = crut_aprintf("%s.%d", TEST_PREFIX, 0);
    retval = create_temp_file(filename, length);
    if (retval < 0) {
        goto out_free;
    }

    /* open a bunch of instances of the temp file... */
    for (i=0; i<num_tests; ++i) {
        fds[i] = open_fd_struct(filename, O_RDWR | test_flags[i], 0);
	if (fds[i] == NULL) {
            goto out_close;
	}
    }

    /* unlink, if requested, only after last open */
    fds[0]->unlinked = do_unlink;
    if (do_unlink) {
        retval = unlink(filename);
        if (retval < 0) {
	    perror("unlink");
            goto out_close;
        }
    }

    *testdata = fds;
    return 0;

out_close:
    /* close anything we opened */
    for (i=0; i<num_tests; ++i) {
        if (fds[i] != NULL) {
            close_fd_struct(fds[i]);
	}
    }
    /* nuke the test file */
    if (!do_unlink && (unlink(filename) < 0)) {
        perror("unlink");
    }
out_free:
    free(fds);
out:
    *testdata = NULL;
    return retval;
}

static int
fd_flags_setup(void **testdata) { return _fd_flags_setup(testdata, 0); }

static int
fd_flags_unlinked_setup(void **testdata) { return _fd_flags_setup(testdata, 1); }

static int
fd_generic_precheckpoint(void *p)
{
    int retval;

    CRUT_DEBUG("Testing sanity before we checkpoint");
    retval = update_fd_struct((struct fd_struct *)p);
    if (retval < 0) {
        CRUT_FAIL("update_fd_struct failed before checkpoint...");
    }

    return retval;
}

static int
fd_flags_precheckpoint_continue(void *p)
{
    struct fd_struct **fds = (struct fd_struct **)p;
    int num_tests = sizeof(test_flags)/sizeof(int);
    int i;
    int retval = 0;

    CRUT_DEBUG("Testing sanity before we checkpoint");
    for (i=0; i<num_tests; ++i) {
        retval = update_fd_struct(fds[i]);
        if (retval < 0) {
            CRUT_FAIL("update_fd_struct failed before checkpoint...");
	    break;
	}

	/* HACK */
	fds[i]->flags = fds[i]->flags | O_RDWR;
    }

    return retval;
}

static int
fd_generic_continue(void *p)
{
    int retval;

    CRUT_DEBUG("Continuing after checkpoint.");
    retval = update_fd_struct((struct fd_struct *)p);

    return retval;
}

static int
fd_read_restart(void *p)
{
    int retval;
    struct fd_struct *f = (struct fd_struct *)p;

    CRUT_DEBUG("Restarting from checkpoint.");

    /* check the flags */
    retval = check_flags(f);
    if (retval < 0) {
	goto out;
    }

    /* make sure the file pointer was correctly positioned */
    retval = check_offset(f);
    if (retval < 0) {
	goto out;
    }

    /* compare the stat structs */
    retval = check_stat_simple(f);
    if (retval < 0) {
	goto out;
    }

    /* match the file contents to expected */
    retval = check_pattern(f->fd, DEFAULT_TEST_DATA_LENGTH);
    if (retval < 0) {
        goto out;
    }

    /* make sure we can't write to one of these things. */
    retval = write(f->fd, f, sizeof(*f));
    if (retval >= 0) {
        perror("write");
        CRUT_FAIL("Unexpectedly able to write to a read-only file!");
    } else if (errno != EBADF) {
        CRUT_FAIL("Write failed, but for the wrong reason!");
        perror("write");
    } else {
        retval = 0;
    }

out:
    return retval;
}

static int
fd_write_restart(void *p)
{
    int retval;
    struct fd_struct *f = (struct fd_struct *)p;
    struct fd_struct *readf;
    char buf[BUF_SIZE];

    /* check the flags */
    retval = check_flags(f);
    if (retval < 0) {
	goto out;
    }
    CRUT_DEBUG("check_flags:return = %d", retval);

    /* make sure the file pointer was correctly positioned */
    retval = check_offset(f);
    if (retval < 0) {
	goto out;
    }
    CRUT_DEBUG("check_offset:return = %d", retval);

    /* compare the stat structs */
    retval = check_stat_simple(f);
    if (retval < 0) {
	goto out;
    }
    CRUT_DEBUG("check_stat_simple:return = %d", retval);

    /* write some stuff to the file. */
    retval = write_pattern(f->fd, DEFAULT_TEST_DATA_LENGTH);
    if (retval < 0) {
        goto out;
    }
    CRUT_DEBUG("write_pattern:return = %d", retval);

    /* try to read from a file opened O_WRONLY.  Better fail */
    retval = read(f->fd, buf, sizeof(*f));
    if (retval >= 0) {
        perror("read");
        CRUT_FAIL("Unexpectedly able to read from a write-only file!");
    } else if (errno != EBADF) {
        CRUT_FAIL("Read failed, but for the wrong reason!");
        perror("read");
    } else {
        retval = 0;
    }
    
    /* skip reopen for the unlinked case */
    if (f->unlinked) goto out;

    /* reopen to check file contents */
    readf = open_fd_struct(f->filename, O_RDONLY, 0);
    if (readf == NULL) {
        retval = -1;
	goto out;
    }
    p = readf;

    /* match the file contents to expected */
    retval = check_pattern(readf->fd, DEFAULT_TEST_DATA_LENGTH);
    if (retval < 0) {
        goto out;
    }

out:
    return retval;
}

static int
fd_rdwr_restart(void *p)
{
    int retval;
    struct fd_struct *f = (struct fd_struct *)p;

    /* check the flags */
    retval = check_flags(f);
    if (retval < 0) {
	goto out;
    }
    CRUT_DEBUG("check_flags:return = %d", retval);

    /* make sure the file pointer was correctly positioned */
    retval = check_offset(f);
    if (retval < 0) {
	goto out;
    }
    CRUT_DEBUG("check_offset:return = %d", retval);

    /* compare the stat structs */
    retval = check_stat_simple(f);
    if (retval < 0) {
	goto out;
    }
    CRUT_DEBUG("check_stat_simple:return = %d", retval);

    /* write some stuff to the file. */
    retval = write_pattern(f->fd, DEFAULT_TEST_DATA_LENGTH);
    if (retval < 0) {
        goto out;
    }
    CRUT_DEBUG("write_pattern:return = %d", retval);

    /* seek back to beginning */
    retval = lseek(f->fd, 0, SEEK_SET);
    if (retval < 0) {
        perror("lseek");
        goto out;
    }

    /* match the file contents to expected */
    retval = check_pattern(f->fd, 2*DEFAULT_TEST_DATA_LENGTH);
    if (retval < 0) {
        goto out;
    }

out:
    return retval;
}

static int
fd_flags_restart(void *p)
{
    struct fd_struct **fds = (struct fd_struct **)p;
    int num_tests = sizeof(test_flags)/sizeof(int);
    int i;
    int retval = 0;

    for (i=0; i<num_tests; ++i) {
        retval = check_fd_struct_rdwr(fds[i]);
        if (retval < 0) {
            CRUT_FAIL("Test failed for fd %d (flag 0%o)", fds[i]->fd, test_flags[i]);
	    break;
	}
    }

    return retval;
}

static int
fd_generic_teardown(void *p)
{
    int retval;
    struct fd_struct *f = (struct fd_struct *)p;

    retval = unlink(f->filename);
    if (f->unlinked) {
        if ((retval >= 0) || (errno != ENOENT)) {
            perror("duplicate unlink");
	    retval = -1;
        } else {
	    retval = 0;
	}
    } else if (retval < 0) {
        perror("unlink");
    }

    free(p);

    return retval;
}

static int
fd_flags_teardown(void *p)
{
    int retval;
    struct fd_struct **f = (struct fd_struct **)p;

    retval = unlink(f[0]->filename);
    if (f[0]->unlinked) {
        if ((retval >= 0) || (errno != ENOENT)) {
            perror("duplicate unlink");
	    retval = -1;
        } else {
	    retval = 0;
	}
    } else if (retval < 0) {
        perror("unlink");
    }

    free(p);

    return retval;
}

int
main(int argc, char *argv[])
{
    int ret;
    struct crut_operations fd_read_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"fd_read",
        test_description:"Test that fd's open for read work.",
	test_setup:fd_read_setup,
	test_precheckpoint:fd_generic_precheckpoint,
	test_continue:fd_generic_continue,
	test_restart:fd_read_restart,
	test_teardown:fd_generic_teardown,
    };

    struct crut_operations fd_write_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"fd_write",
        test_description:"Test fd's open for writing.",
	test_setup:fd_write_setup,
	test_precheckpoint:fd_generic_precheckpoint,
	test_continue:fd_generic_continue,
	test_restart:fd_write_restart,
	test_teardown:fd_generic_teardown,
    };

    struct crut_operations fd_rdwr_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"fd_rdwr",
        test_description:"Test fd's open O_RDWR.",
	test_setup:fd_rdwr_setup,
	test_precheckpoint:fd_generic_precheckpoint,
	test_continue:fd_generic_continue,
	test_restart:fd_rdwr_restart,
	test_teardown:fd_generic_teardown,
    };

    struct crut_operations fd_flags_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"fd_flags",
        test_description:"Test flags on open files",
	test_setup:fd_flags_setup,
	test_precheckpoint:fd_flags_precheckpoint_continue,
	test_continue:fd_flags_precheckpoint_continue,
	test_restart:fd_flags_restart,
	test_teardown:fd_flags_teardown,
    };

    struct crut_operations fd_read_unlinked_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"fd_read_unlinked",
        test_description:"Test that fd's open for read work.",
	test_setup:fd_read_unlinked_setup,
	test_precheckpoint:fd_generic_precheckpoint,
	test_continue:fd_generic_continue,
	test_restart:fd_read_restart,
	test_teardown:fd_generic_teardown,
    };

    struct crut_operations fd_write_unlinked_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"fd_write_unlinked",
        test_description:"Test fd's open for writing.",
	test_setup:fd_write_unlinked_setup,
	test_precheckpoint:fd_generic_precheckpoint,
	test_continue:fd_generic_continue,
	test_restart:fd_write_restart,
	test_teardown:fd_generic_teardown,
    };

    struct crut_operations fd_rdwr_unlinked_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"fd_rdwr_unlinked",
        test_description:"Test fd's open O_RDWR.",
	test_setup:fd_rdwr_unlinked_setup,
	test_precheckpoint:fd_generic_precheckpoint,
	test_continue:fd_generic_continue,
	test_restart:fd_rdwr_restart,
	test_teardown:fd_generic_teardown,
    };

    struct crut_operations fd_flags_unlinked_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"fd_flags_unlinked",
        test_description:"Test flags on open files",
	test_setup:fd_flags_unlinked_setup,
	test_precheckpoint:fd_flags_precheckpoint_continue,
	test_continue:fd_flags_precheckpoint_continue,
	test_restart:fd_flags_restart,
	test_teardown:fd_flags_teardown,
    };

    /* add the basic tests */
    crut_add_test(&fd_read_test_ops);
    crut_add_test(&fd_write_test_ops);
    crut_add_test(&fd_rdwr_test_ops);
    crut_add_test(&fd_flags_test_ops);

    /* add the unlinked tests */
    crut_add_test(&fd_read_unlinked_test_ops);
    crut_add_test(&fd_write_unlinked_test_ops);
    crut_add_test(&fd_rdwr_unlinked_test_ops);
    crut_add_test(&fd_flags_unlinked_test_ops);

    ret = crut_main(argc, argv);

    return ret;
}
