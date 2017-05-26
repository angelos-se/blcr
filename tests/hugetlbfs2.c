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
 * $Id: hugetlbfs2.c,v 1.3 2008/08/29 21:36:53 phargrov Exp $
 *
 * Test of PRIVATE mmap()s on hugetlbfs across a checkpoint
 */

#define _GNU_SOURCE /* Ensures we get getline() */
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

static const char MSG[] = "MSG: Hello world!";

static int pagesize = -1;
#define FILESTEM "/tstfile.0"
static char *filename;

enum {
	TEST_UNLINK	= 1,
};

struct testcase {
	char *	addr;
	size_t	len;
	int	prot;
	int	flags;
	int	fd;
	int	test_flags;
};

static int
mmaps_setup_common(void **testdata, void *start, int prot , int flags, int fd, int test_flags)
{
    struct testcase *t = malloc(sizeof(*t));
    size_t len = pagesize;

    t->test_flags = test_flags;
    t->len = len;
    t->prot = prot;
    t->flags = flags;
    t->fd = fd;
    t->addr = mmap(start, len, prot, flags, fd, 0);
    CRUT_DEBUG("mmap(%p, %d, 0x%x, 0x%x, %d, 0) -> %p (%d)", start, (int)len, prot, flags, fd, t->addr, errno);
    if (t->addr == MAP_FAILED) return -1;
    strcpy(t->addr, MSG);
    *testdata = t;
    return 0;
}

static int mmaps_setup_file_common(void **testdata, int prot, int flags, off_t offset, int test_flags)
{
    int fd, retval;
    (void)unlink(filename);
    fd = open(filename, O_CREAT|O_RDWR|O_TRUNC, S_IREAD|S_IWRITE);
    if (fd < 0) return fd;
    retval = mmaps_setup_common(testdata, 0, prot, flags, fd, test_flags);
    close(fd);
    if (test_flags & TEST_UNLINK) (void)unlink(filename);
    return retval;
}

static int mmaps_setup_rw_fp(void **testdata)
{ return mmaps_setup_file_common(testdata, PROT_READ|PROT_WRITE, MAP_PRIVATE, 0, 0); }

static int mmaps_setup_rw_up(void **testdata)
{ return mmaps_setup_file_common(testdata, PROT_READ|PROT_WRITE, MAP_PRIVATE, 0, TEST_UNLINK); }

static int check_private(struct testcase *t)
{
    if (strncmp(t->addr, MSG, sizeof(MSG))) {
	CRUT_FAIL("Wrong value read from memory");
	return -1;
    }

    if (!(t->prot & PROT_WRITE)) {
	if (mprotect(t->addr, t->len, PROT_READ|PROT_WRITE) == 0) {
	    CRUT_FAIL("Unexpected success from mprotect(READ|WRITE) of read-only file");
	    return -1;
        }
	return 0;
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

static int
mmaps_teardown(void *p)
{
    struct testcase *t = (struct testcase *)p;

    (void)unlink(filename);

    return munmap(t->addr, t->len);
}

static void test_init(void)
{
    FILE *fp;
    int rc, fd;
    size_t len;
    char *line = NULL;
    char *mntpnt = NULL;
    void *addr;

    /* Find the pagesize */
    fp = fopen("/proc/meminfo", "r");
    if (!fp) {
	perror("open(/proc/meminfo)");
	exit(1);
    }
    while ((rc = getline(&line, &len, fp)) >= 0) {
	const char key[] = "Hugepagesize:";
	char *s;
	int kb;

	if (strncmp(key, line, sizeof(key) - 1)) { continue; }

	s = strchr(line, ' '); /* space after key */
	if (!s) { continue; }

	if (sscanf(s, " %d", &kb) != 1) { continue; }
	
	pagesize = 1024 * kb;
	break;
    }
    free(line); line = NULL;
    (void)fclose(fp);
    if (pagesize < 0) {
        fprintf(stderr, "Unable to determine huge pagesize, if any (test skipped).\n");
	exit(77);
    }

    /* Find the mount point */
    fp = fopen("/proc/mounts", "r");
    if (!fp) {
	perror("open(/proc/mounts)");
	exit(1);
    }
    while ((rc = getline(&line, &len, fp)) >= 0) {
	const char key[] = "hugetlbfs";
	char *s, *fstype;

	s = strchr(line, ' '); /* space after device */
	if (!s) { continue; }

	mntpnt = s + 1;
	s = strchr(mntpnt, ' '); /* space after mntpnt */
	if (!s) { continue; }

	*s = '\0';
	len = s - mntpnt;
	fstype = s + 1;

	if (strncmp(key, fstype, sizeof(key) - 1)) { continue; }
	
	mntpnt = strdup(mntpnt);
	break;
    }
    free(line); line = NULL;
    (void)fclose(fp);
    if (rc < 0) {
        fprintf(stderr, "No hugetlbfs mount point found (test skipped)\n");
	exit(77);
    }

    /* Build the filename */
    len += sizeof(FILESTEM);
    filename = malloc(len);
    strcpy(filename, mntpnt);
    strcat(filename, FILESTEM);
    free(mntpnt);

    /* Basic test that we can create and mmap the file */
    (void)unlink(filename);
    fd = open(filename, O_CREAT|O_RDWR|O_TRUNC, S_IREAD|S_IWRITE);
    if (fd < 0) {
        fprintf(stderr, "Could not create file on hugetlbfs (test skipped)\n");
	exit(77);
    }
    (void)unlink(filename);
    addr = mmap(0, pagesize,  PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
    (void)close(fd);
    if (addr == MAP_FAILED) {
        fprintf(stderr, "Could not private mmap %dKB file on hugetlbfs (test skipped)\n", pagesize/1024);
	(void)unlink(filename);
	exit(77);
    }
    (void)munmap(addr, pagesize);
}

int
main(int argc, char *argv[])
{
    int ret;
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

    /* Make sure we have a usable hugetlbfs */
    test_init();

    /* "normal" file test */
    crut_add_test(&rw_fp_test_ops);

    /* unlinked file test */
    crut_add_test(&rw_up_test_ops);
    
    ret = crut_main(argc, argv);

    return ret;
}
