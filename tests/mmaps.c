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
 * $Id: mmaps.c,v 1.15 2008/08/29 21:36:53 phargrov Exp $
 *
 * Test of various mmap()s across a checkpoint
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

#include "crut.h"

#define FILENAME "tstfile"

enum {
 MSG_PARENT_DONE = 17,
 MSG_CHILD_READY,
 MSG_PARENT_REQUEST,
 MSG_CHILD_GOOD,
 MSG_CHILD_BAD
};

/* Note: per bug 2290 MSG1 must be at least as long as the others */
static const char MSG1[] = "MSG1: Hello world!  This message must be the longest.";
static const char MSG2[] = "MSG2: The quick brown fox jumps over the lazy dog.";
static const char MSG3[] = "MSG3: All good things come to those who waitpid().";
static int pagesize = -1;

enum {
	TEST_UNLINK	= 1,
	TEST_WRITE_FD	= 2
};

struct testcase {
	char *	addr;
	size_t	len;
	int	prot;
	int	flags;
	int	fd;
	off_t	offset;
	struct crut_pipes checker;
	int	test_flags;
};

static void sigpipe(int signo) {
    fprintf(stderr, "%d exiting on SIGPIPE\n", getpid());
    exit(-1);
}

static void checker_child(struct testcase *t) {

    /* If shared mapping of a file, them map via a separate file descriptor */
    if ((t->fd >= 0) && (t->flags & MAP_SHARED)) {
       	int err, fd;
	
	err = fcntl(t->fd, F_GETFL);
	if (err < 0) {
	    perror("fcntl()");
	    exit(-1);
	}
	fd = open(FILENAME, err&O_ACCMODE, 0);
	if (fd < 0) {
	    perror("open()");
	    exit(-1);
	}
	t->addr = mmap(0, t->len, t->prot, t->flags, fd, t->offset);
	if (t->addr == MAP_FAILED) {
	    perror("mmap()");
	    exit(-1);
	}
	(void)close(fd);
    }
    if ((t->fd >= 0) && !(t->test_flags & TEST_WRITE_FD)) (void)close(t->fd);

    crut_pipes_putchar(&t->checker, MSG_CHILD_READY);
    while (1) {
        crut_pipes_expect(&t->checker, MSG_PARENT_REQUEST);
        if (strncmp(t->addr, MSG2, sizeof(MSG2))) {
            crut_pipes_putchar(&t->checker, MSG_CHILD_BAD);
	    CRUT_FAIL("Wrong value read.  Pre-checkpoint sharing lost? '%s' '%s'", t->addr, MSG2);
	    exit(-1);
	}
	if (t->test_flags & TEST_WRITE_FD) {
	    if (lseek(t->fd, t->offset, SEEK_SET) == (off_t)-1) {
		CRUT_FAIL("lseek failed in checker child: %s (%d)", strerror(errno), errno);
	        exit(-1);
	    }
	    if (write(t->fd, MSG3, sizeof(MSG3)) != sizeof(MSG3)) {
		CRUT_FAIL("write failed in checker child: %s (%d)", strerror(errno), errno);
	        exit(-1);
	    }
	} else {
	    strcpy(t->addr, MSG3);
	}
        crut_pipes_putchar(&t->checker, MSG_CHILD_GOOD);
    }
}

static int
mmaps_setup_common(void **testdata, void *start, int prot , int flags, int fd, off_t offset, int test_flags)
{
    struct testcase *t = malloc(sizeof(*t));
    size_t len = pagesize;
    size_t real_offset = offset;

    if (!(flags & MAP_ANONYMOUS)) {
        if (lseek(fd, offset, SEEK_SET) == (off_t)-1) return -1;
        write(fd, MSG1, sizeof(MSG1)); /* Includes \0 */
        if (!(prot & PROT_WRITE)) {
	    fchmod(fd, S_IREAD);
	    close(fd);
	    fd = open(FILENAME, O_RDONLY);
	    if (fd < 0) return fd;
        }
    }

    t->test_flags = test_flags;
    t->len = len;
    t->prot = prot;
    t->flags = flags;
    t->fd = fd;
    t->offset = offset;
    if ((flags & MAP_SHARED) && offset) {
	len += offset;
	real_offset = 0;
    }
    t->addr = mmap(start, len, prot, flags, fd, real_offset);
    CRUT_DEBUG("mmap(%p, %d, 0x%x, 0x%x, %d, %d) -> %p (%d)", start, (int)len, prot, flags, fd, (int)real_offset, t->addr, errno);
    if (t->addr == MAP_FAILED) return -1;
    if (flags & MAP_SHARED) {
	(void)signal(SIGPIPE, &sigpipe);
	if (offset) {
	    munmap(t->addr, offset);
	    t->addr += offset;
	}
	if (!crut_pipes_fork(&t->checker)) {
	    /* In the child */
	    checker_child(t);
	    exit(-1); /* Not reached */
	} else {
	    /* In the parent */
	    crut_pipes_expect(&t->checker, MSG_CHILD_READY);
	    CRUT_DEBUG("Child ready");
	}
    }
    if (flags & MAP_ANONYMOUS) {
	strcpy(t->addr, MSG1);
    }
    *testdata = t;
    return 0;
}

static int mmaps_setup_anon_common(void **testdata, int prot, int flags, off_t offset)
{
    return mmaps_setup_common(testdata, 0, prot, MAP_ANONYMOUS|flags, -1, offset, 0);
}

static int mmaps_setup_file_common(void **testdata, int prot, int flags, off_t offset, int test_flags)
{
    int fd, retval;
    (void)unlink(FILENAME);
    fd = open(FILENAME, O_CREAT|O_RDWR|O_TRUNC, S_IREAD|S_IWRITE);
    if (fd < 0) return fd;
    retval = mmaps_setup_common(testdata, 0, prot, flags, fd, offset, test_flags);
    close(fd);
    if (test_flags & TEST_UNLINK) (void)unlink(FILENAME);
    return retval;
}

static int mmaps_setup_rw_as(void **testdata)
{ return mmaps_setup_anon_common(testdata, PROT_READ|PROT_WRITE, MAP_SHARED, 0); }

static int mmaps_setup_rw_ap(void **testdata)
{ return mmaps_setup_anon_common(testdata, PROT_READ|PROT_WRITE, MAP_PRIVATE, 0); }

static int mmaps_setup_rw_fs(void **testdata)
{ return mmaps_setup_file_common(testdata, PROT_READ|PROT_WRITE, MAP_SHARED, 0, 0); }

static int mmaps_setup_rw_fp(void **testdata)
{ return mmaps_setup_file_common(testdata, PROT_READ|PROT_WRITE, MAP_PRIVATE, 0, 0); }

static int mmaps_setup_rw_us(void **testdata)
{ return mmaps_setup_file_common(testdata, PROT_READ|PROT_WRITE, MAP_SHARED, 0, TEST_UNLINK); }

static int mmaps_setup_rw_up(void **testdata)
{ return mmaps_setup_file_common(testdata, PROT_READ|PROT_WRITE, MAP_PRIVATE, 0, TEST_UNLINK); }

static int mmaps_setup_ro_fs(void **testdata)
{ return mmaps_setup_file_common(testdata, PROT_READ, MAP_SHARED, 0, 0); }

#define the_OFFSET (2*pagesize)

static int mmaps_setup_rw_as_offset(void **testdata)
{ return mmaps_setup_anon_common(testdata, PROT_READ|PROT_WRITE, MAP_SHARED, the_OFFSET); }

static int mmaps_setup_rw_ap_offset(void **testdata)
{ return mmaps_setup_anon_common(testdata, PROT_READ|PROT_WRITE, MAP_PRIVATE, the_OFFSET); }

static int mmaps_setup_rw_fs_offset(void **testdata)
{ return mmaps_setup_file_common(testdata, PROT_READ|PROT_WRITE, MAP_SHARED, the_OFFSET, 0); }

static int mmaps_setup_rw_fp_offset(void **testdata)
{ return mmaps_setup_file_common(testdata, PROT_READ|PROT_WRITE, MAP_PRIVATE, the_OFFSET, 0); }

static int mmaps_setup_rw_us_offset(void **testdata)
{ return mmaps_setup_file_common(testdata, PROT_READ|PROT_WRITE, MAP_SHARED, the_OFFSET, TEST_UNLINK); }

static int mmaps_setup_rw_up_offset(void **testdata)
{ return mmaps_setup_file_common(testdata, PROT_READ|PROT_WRITE, MAP_PRIVATE, the_OFFSET, TEST_UNLINK); }

static int mmaps_setup_ro_fs_offset(void **testdata)
{ return mmaps_setup_file_common(testdata, PROT_READ, MAP_SHARED, the_OFFSET, 0); }

static int mmaps_setup_rw_fs2(void **testdata)
{ return mmaps_setup_file_common(testdata, PROT_READ|PROT_WRITE, MAP_SHARED, 0, TEST_WRITE_FD); }

static int mmaps_setup_rw_fs2_offset(void **testdata)
{ return mmaps_setup_file_common(testdata, PROT_READ|PROT_WRITE, MAP_SHARED, the_OFFSET, TEST_WRITE_FD); }

static int mmaps_setup_rw_us2(void **testdata)
{ return mmaps_setup_file_common(testdata, PROT_READ|PROT_WRITE, MAP_SHARED, 0, TEST_UNLINK|TEST_WRITE_FD); }

static int mmaps_setup_rw_us2_offset(void **testdata)
{ return mmaps_setup_file_common(testdata, PROT_READ|PROT_WRITE, MAP_SHARED, the_OFFSET, TEST_UNLINK|TEST_WRITE_FD); }

static int check_private(struct testcase *t)
{
    if (strncmp(t->addr, MSG1, sizeof(MSG1))) {
	CRUT_FAIL("Wrong value read from memory");
	return -1;
    }
    
    return 0;
}

static int
mmaps_precheckpoint_private(void *p)
{
    CRUT_DEBUG("Sanity check before checkpoint.");
    return check_private(p);
}

static int
mmaps_continue_private(void *p)
{
    CRUT_DEBUG("Continuing after checkpoint.");
    return check_private(p);
}

static int
mmaps_restart_private(void *p)
{
    CRUT_DEBUG("Restarting from checkpoint.");
    return check_private(p);
}

static int check_shared(struct testcase *t)
{
    int rc;
    pid_t pid;

    rc = check_private(t);
    if (rc < 0) return rc;

    if (!(t->prot & PROT_WRITE)) {
	if (mprotect(t->addr, t->len, PROT_READ|PROT_WRITE) == 0) {
	    CRUT_FAIL("Unexpected success from mprotect(READ|WRITE) of read-only file");
	    return -1;
        }
	return 0;
    }

    /* Fork a transient child to read MSG1 and write MSG2 */
    pid = fork();
    if (pid < 0) {
	CRUT_FAIL("fork() failed: %s", strerror(errno));
	return -1;
    }
    if (!pid) {
	/* Child */
        if (strncmp(t->addr, MSG1, sizeof(MSG1))) {
	    CRUT_FAIL("Wrong value read in child.  Post-checkpoint sharing lost? '%s' '%s'", t->addr, MSG1);
	    exit(-1);
	}
        strcpy(t->addr, MSG2);
	exit(0);
    }
    crut_waitpid_expect(pid, 0);

    /* Now ask the permanent child to read MSG2 and write MSG3 */
    crut_pipes_putchar(&t->checker, MSG_PARENT_REQUEST);
    crut_pipes_expect(&t->checker, MSG_CHILD_GOOD);
    if (strncmp(t->addr, MSG3, sizeof(MSG3))) {
	CRUT_FAIL("Wrong value read from memory.  Pre-checkpoint sharing lost? '%s' '%s'", t->addr, MSG3);
	return -1;
    }

    strcpy(t->addr, MSG1);
    
    return 0;
}

static int
mmaps_precheckpoint_shared(void *p)
{
    CRUT_DEBUG("Sanity check before checkpoint.");
    return check_shared(p);
}

static int
mmaps_continue_shared(void *p)
{
    CRUT_DEBUG("Continuing after checkpoint.");
    return check_shared(p);
}

static int
mmaps_restart_shared(void *p)
{
    CRUT_DEBUG("Restarting from checkpoint.");
    return check_shared(p);
}

static int
mmaps_teardown(void *p)
{
    struct testcase *t = (struct testcase *)p;

    (void)unlink(FILENAME);

    if (t->flags & MAP_SHARED) {
        crut_pipes_putchar(&t->checker, MSG_PARENT_DONE);
	crut_waitpid_expect(t->checker.child, MSG_PARENT_DONE);
    }

    return munmap(t->addr, t->len);
}

int
main(int argc, char *argv[])
{
    int ret;
    struct crut_operations rw_as_test_ops = {
	test_scope:CR_SCOPE_TREE,
	test_name:"mmaps_rw_as",
        test_description:"Test mmap(READ|WRITE,ANONYMOUS|SHARED).",
	test_setup:mmaps_setup_rw_as,
	test_precheckpoint:mmaps_precheckpoint_shared,
	test_continue:mmaps_continue_shared,
	test_restart:mmaps_restart_shared,
	test_teardown:mmaps_teardown,
    };
    struct crut_operations rw_ap_test_ops = {
	test_scope:CR_SCOPE_TREE,
	test_name:"mmaps_rw_ap",
        test_description:"Test mmap(READ|WRITE,ANONYMOUS|PRIVATE).",
	test_setup:mmaps_setup_rw_ap,
	test_precheckpoint:mmaps_precheckpoint_private,
	test_continue:mmaps_continue_private,
	test_restart:mmaps_restart_private,
	test_teardown:mmaps_teardown,
    };

    struct crut_operations rw_as_offset_test_ops = {
	test_scope:CR_SCOPE_TREE,
	test_name:"mmaps_rw_as_offset",
        test_description:"Test mmap(READ|WRITE,ANONYMOUS|SHARED) w/ unmapped prefix.",
	test_setup:mmaps_setup_rw_as_offset,
	test_precheckpoint:mmaps_precheckpoint_shared,
	test_continue:mmaps_continue_shared,
	test_restart:mmaps_restart_shared,
	test_teardown:mmaps_teardown,
    };
    struct crut_operations rw_ap_offset_test_ops = {
	test_scope:CR_SCOPE_TREE,
	test_name:"mmaps_rw_ap_offset",
        test_description:"Test mmap(READ|WRITE,ANONYMOUS|PRIVATE) w/ unmapped prefix.",
	test_setup:mmaps_setup_rw_ap_offset,
	test_precheckpoint:mmaps_precheckpoint_private,
	test_continue:mmaps_continue_private,
	test_restart:mmaps_restart_private,
	test_teardown:mmaps_teardown,
    };


    struct crut_operations rw_fs_test_ops = {
	test_scope:CR_SCOPE_TREE,
	test_name:"mmaps_rw_fs",
        test_description:"Test mmap(READ|WRITE,SHARED) of a file.",
	test_setup:mmaps_setup_rw_fs,
	test_precheckpoint:mmaps_precheckpoint_shared,
	test_continue:mmaps_continue_shared,
	test_restart:mmaps_restart_shared,
	test_teardown:mmaps_teardown,
    };
    struct crut_operations rw_fp_test_ops = {
	test_scope:CR_SCOPE_TREE,
	test_name:"mmaps_rw_fp",
        test_description:"Test mmap(READ|WRITE,PRIVATE) of a file.",
	test_setup:mmaps_setup_rw_fp,
	test_precheckpoint:mmaps_precheckpoint_private,
	test_continue:mmaps_continue_private,
	test_restart:mmaps_restart_private,
	test_teardown:mmaps_teardown,
    };

    struct crut_operations rw_us_test_ops = {
	test_scope:CR_SCOPE_TREE,
	test_name:"mmaps_rw_us",
        test_description:"Test mmap(READ|WRITE,SHARED) of an unlinked file.",
	test_setup:mmaps_setup_rw_us,
	test_precheckpoint:mmaps_precheckpoint_shared,
	test_continue:mmaps_continue_shared,
	test_restart:mmaps_restart_shared,
	test_teardown:mmaps_teardown,
    };
    struct crut_operations rw_up_test_ops = {
	test_scope:CR_SCOPE_TREE,
	test_name:"mmaps_rw_up",
        test_description:"Test mmap(READ|WRITE,PRIVATE) of an unlinked file.",
	test_setup:mmaps_setup_rw_up,
	test_precheckpoint:mmaps_precheckpoint_private,
	test_continue:mmaps_continue_private,
	test_restart:mmaps_restart_private,
	test_teardown:mmaps_teardown,
    };

    struct crut_operations ro_fs_test_ops = {
	test_scope:CR_SCOPE_TREE,
	test_name:"mmaps_ro_fs",
        test_description:"Test mmap(READ,SHARED) of a file.",
	test_setup:mmaps_setup_ro_fs,
	test_precheckpoint:mmaps_precheckpoint_shared,
	test_continue:mmaps_continue_shared,
	test_restart:mmaps_restart_shared,
	test_teardown:mmaps_teardown,
    };

    struct crut_operations rw_fs_offset_test_ops = {
	test_scope:CR_SCOPE_TREE,
	test_name:"mmaps_rw_fs_offset",
        test_description:"Test mmap(READ|WRITE,SHARED,OFFSET>0) of a file.",
	test_setup:mmaps_setup_rw_fs_offset,
	test_precheckpoint:mmaps_precheckpoint_shared,
	test_continue:mmaps_continue_shared,
	test_restart:mmaps_restart_shared,
	test_teardown:mmaps_teardown,
    };
    struct crut_operations rw_fp_offset_test_ops = {
	test_scope:CR_SCOPE_TREE,
	test_name:"mmaps_rw_fp_offset",
        test_description:"Test mmap(READ|WRITE,PRIVATE,OFFSET>0) of a file.",
	test_setup:mmaps_setup_rw_fp_offset,
	test_precheckpoint:mmaps_precheckpoint_private,
	test_continue:mmaps_continue_private,
	test_restart:mmaps_restart_private,
	test_teardown:mmaps_teardown,
    };

    struct crut_operations rw_us_offset_test_ops = {
	test_scope:CR_SCOPE_TREE,
	test_name:"mmaps_rw_us_offset",
        test_description:"Test mmap(READ|WRITE,SHARED,OFFSET>0) of an unlinked file.",
	test_setup:mmaps_setup_rw_us_offset,
	test_precheckpoint:mmaps_precheckpoint_shared,
	test_continue:mmaps_continue_shared,
	test_restart:mmaps_restart_shared,
	test_teardown:mmaps_teardown,
    };
    struct crut_operations rw_up_offset_test_ops = {
	test_scope:CR_SCOPE_TREE,
	test_name:"mmaps_rw_up_offset",
        test_description:"Test mmap(READ|WRITE,PRIVATE,OFFSET>0) of an unlinked file.",
	test_setup:mmaps_setup_rw_up_offset,
	test_precheckpoint:mmaps_precheckpoint_private,
	test_continue:mmaps_continue_private,
	test_restart:mmaps_restart_private,
	test_teardown:mmaps_teardown,
    };

    struct crut_operations ro_fs_offset_test_ops = {
	test_scope:CR_SCOPE_TREE,
	test_name:"mmaps_ro_fs_offset",
        test_description:"Test mmap(READ,SHARED,OFFSET>0) of a file.",
	test_setup:mmaps_setup_ro_fs_offset,
	test_precheckpoint:mmaps_precheckpoint_shared,
	test_continue:mmaps_continue_shared,
	test_restart:mmaps_restart_shared,
	test_teardown:mmaps_teardown,
    };

    struct crut_operations rw_fs2_test_ops = {
	test_scope:CR_SCOPE_TREE,
	test_name:"mmaps_rw_fs2",
        test_description:"Test mmap(READ|WRITE,SHARED) of a file via memory and fd.",
	test_setup:mmaps_setup_rw_fs2,
	test_precheckpoint:mmaps_precheckpoint_shared,
	test_continue:mmaps_continue_shared,
	test_restart:mmaps_restart_shared,
	test_teardown:mmaps_teardown,
    };
    struct crut_operations rw_fs2_offset_test_ops = {
	test_scope:CR_SCOPE_TREE,
	test_name:"mmaps_rw_fs2_offset",
        test_description:"Test mmap(READ|WRITE,SHARED,OFFSET>0) of a file via memory and fd.",
	test_setup:mmaps_setup_rw_fs2_offset,
	test_precheckpoint:mmaps_precheckpoint_shared,
	test_continue:mmaps_continue_shared,
	test_restart:mmaps_restart_shared,
	test_teardown:mmaps_teardown,
    };

    struct crut_operations rw_us2_test_ops = {
	test_scope:CR_SCOPE_TREE,
	test_name:"mmaps_rw_us2",
        test_description:"Test mmap(READ|WRITE,SHARED) of an unlinked file via memory and fd.",
	test_setup:mmaps_setup_rw_us2,
	test_precheckpoint:mmaps_precheckpoint_shared,
	test_continue:mmaps_continue_shared,
	test_restart:mmaps_restart_shared,
	test_teardown:mmaps_teardown,
    };
    struct crut_operations rw_us2_offset_test_ops = {
	test_scope:CR_SCOPE_TREE,
	test_name:"mmaps_rw_us2_offset",
        test_description:"Test mmap(READ|WRITE,SHARED,OFFSET>0) of an unlinked file via memory and fd.",
	test_setup:mmaps_setup_rw_us2_offset,
	test_precheckpoint:mmaps_precheckpoint_shared,
	test_continue:mmaps_continue_shared,
	test_restart:mmaps_restart_shared,
	test_teardown:mmaps_teardown,
    };

    /* MAP_ANONYMOUS tests */
    crut_add_test(&rw_as_test_ops);
    crut_add_test(&rw_ap_test_ops);
    crut_add_test(&rw_as_offset_test_ops);
    crut_add_test(&rw_ap_offset_test_ops);

    /* "normal" file tests */
    crut_add_test(&rw_fs_test_ops);
    crut_add_test(&rw_fp_test_ops);
    crut_add_test(&ro_fs_test_ops);
    crut_add_test(&rw_fs_offset_test_ops);
    crut_add_test(&rw_fp_offset_test_ops);
    crut_add_test(&ro_fs_offset_test_ops);
    crut_add_test(&rw_fs2_test_ops);
    crut_add_test(&rw_fs2_offset_test_ops);

    /* unlinked file tests */
    crut_add_test(&rw_us_test_ops);
    crut_add_test(&rw_up_test_ops);
    crut_add_test(&rw_us_offset_test_ops);
    crut_add_test(&rw_up_offset_test_ops);
    crut_add_test(&rw_us2_test_ops);
    crut_add_test(&rw_us2_offset_test_ops);
    
    pagesize = getpagesize();
    ret = crut_main(argc, argv);

    return ret;
}
