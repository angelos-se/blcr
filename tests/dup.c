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
 * $Id: dup.c,v 1.15 2008/08/29 21:36:53 phargrov Exp $
 *
 * Simple example for using crut
 */

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
#include "libcr.h"

#define TEST_PREFIX "tstfile"
#define TEST_FILE_MODE 0660
#define MAX_TEST_FILENAME 255

#define MAX_FD 8*1024

/*
 * describes a pair of dup'd file descriptors
 *
 * everything about 'em should be the same
 */
struct dup_fd {
    int fd1;
    int fd2;
    char *filename;
};

extern cr_checkpoint_handle_t crut_cr_handle;
static int is_cr_fd(int fd) {
    /* This is a horrible hack! */
    return (fd == (int)crut_cr_handle);
}

/*
 * get_dup_status
 */
static int *
get_dup_stats(void)
{
    int *dup_stats;
    int tmp_fd;
    int i;

    CRUT_DEBUG("in get_dup_stats");
    dup_stats = malloc(sizeof(*dup_stats) * MAX_FD);
    if (dup_stats == NULL) {
        perror("malloc");
	goto out;
    }

    for (i=MAX_FD-1; i>=0; --i) {
        tmp_fd = dup(i);
	if (tmp_fd < 0) {
	    dup_stats[i] = errno;
	} else {
	    CRUT_DEBUG("Closing %d (dup of %d)", tmp_fd, i);
	    close(tmp_fd);
	    dup_stats[i] = 0;
	}
    }

    return dup_stats;

out:
    return NULL;
}


/*
 * The whole stat structure ought to match...
 */
static int
check_stat_all(int fd1, int fd2)
{
    struct stat stat1;
    struct stat stat2;
    int retval;

    retval = fstat(fd1, &stat1);
    if (retval < 0) {
	perror("fstat");
        goto out;
    }

    retval = fstat(fd2, &stat2);
    if (retval < 0) {
	perror("fstat");
        goto out;
    }

    retval = statcmp(&stat1, &stat2, ST_DEV | ST_INO | ST_NLINK | ST_UID | ST_GID | ST_RDEV | ST_SIZE | ST_BLKSIZE | ST_BLOCKS);
    /*
     * return < 0 on mismatch
     */
    if (retval) {
	CRUT_FAIL("stat structures did not match");
	retval = -retval; 
    }

out:
    return retval;
}

static int
check_flags(int fd1, int fd2)
{
    int flags1;
    int flags2;
    int retval;

    flags1 = fcntl(fd1, F_GETFL);
    if (flags1 < 0) {
        perror("fcntl");
	retval = flags1;
	goto out;
    }

    flags2 = fcntl(fd2, F_GETFL);
    if (flags2 < 0) {
        perror("fcntl");
	retval = flags2;
	goto out;
    }

    retval = 0;
    if (flags1 != flags2) {
	CRUT_FAIL("flags did not match %d != %d", flags1, flags2);
	retval = -1;
    }

out:
    return retval;
}

static int
check_offset(int fd1, int fd2)
{
    int offset1;
    int offset2;
    int retval;

    offset1 = lseek(fd1, 0, SEEK_CUR);
    if (offset1 < 0) {
        perror("fcntl");
	retval = offset1;
	goto out;
    }

    offset2 = lseek(fd2, 0, SEEK_CUR);
    if (offset2 < 0) {
        perror("fcntl");
	retval = offset2;
	goto out;
    }

    if (offset1 != offset2) {
	CRUT_FAIL("offset did not match %d != %d", offset1, offset2);
	retval = -1;
    }

    retval = 0;

out:
    return retval;
}

static int
dup_setup(void **testdata)
{
    struct dup_fd *dupfds;
    int retval = -1;
    
    *testdata = NULL;

    dupfds = malloc(sizeof(*dupfds));
    if (dupfds == NULL) {
	perror("malloc");
        goto out;
    }

    dupfds->filename = crut_aprintf("%s.%d", TEST_PREFIX, 0);
    (void)unlink(dupfds->filename);
    dupfds->fd1 = open(dupfds->filename, 
	    O_RDWR | O_CREAT | O_EXCL, TEST_FILE_MODE);
    if (dupfds->fd1 < 0) {
        perror("open");
	retval = dupfds->fd1;
	goto out_free;
    }

    dupfds->fd2 = dup2(dupfds->fd1, 100);
    if (dupfds->fd2 < 0) {
        perror("dup2");
	goto out_unlink;
    }

    *testdata = dupfds;
    retval = 0;
    return retval;

out_unlink:
    if (unlink(dupfds->filename) < 0) {
        perror("unlink");	
    }
out_free:
    free(dupfds);
out:
    return retval;
}

/*
 * Tries to dup every available fd (even closed) to make sure nothing
 * succeeds unexpectedly (if cr_rstrt_req.c forgets to close something from
 * cr_restart), or 
 * fails unexpectedly (if cr_rstrt_req.c forgot to open something).
 */
static int
dup_spurious_setup(void **testdata)
{
    int *dup_stats;

    dup_stats = get_dup_stats();
    if (dup_stats == NULL) {
	return -1;
    }

    *testdata = dup_stats;
    return 0;
}

static int
dup_precheckpoint(void *p)
{
    return 0;
}

static int
dup_spurious_check(void *p)
{
    int *old_dup_stats = (int *)p;
    int *new_dup_stats;
    int retval=-1;
    int i;

    new_dup_stats = get_dup_stats();

    for (i=0; i<MAX_FD; ++i) {
        retval = (new_dup_stats[i] != old_dup_stats[i]);
        if (retval && !is_cr_fd(i)) {
            CRUT_FAIL("dup(%d).  old errno %d (%s) != new errno %d (%s)", i, 
		    old_dup_stats[i], strerror(old_dup_stats[i]),
		    new_dup_stats[i], strerror(new_dup_stats[i]));
	    break;
        }

    }

    free(new_dup_stats);

    /* return < 0 on error */
    return -retval;
}

static int
dup_continue(void *p)
{
    return 0;
}

static int
dup_restart(void *p)
{
    int retval;
    struct dup_fd *dupfds = (struct dup_fd *) p;

    retval = check_stat_all(dupfds->fd1, dupfds->fd2);
    if (retval < 0) {
        goto out;
    }

    retval = check_flags(dupfds->fd1, dupfds->fd2);
    if (retval < 0) {
        goto out;
    }

    retval = check_offset(dupfds->fd1, dupfds->fd2);
    if (retval < 0) {
        goto out;
    }

    retval = 0;
out:
    CRUT_DEBUG("retval = %d", retval);
    return retval;
}

static int
dup_teardown(void *p)
{
    struct dup_fd *dupfds = (struct dup_fd *)p;

    if (unlink(dupfds->filename) < 0) {
        perror("unlink");	
    }

    free(p);

    return 0;
}

static int
dup_spurious_teardown(void *p)
{
    free(p);

    return 0;
}

int
main(int argc, char *argv[])
{
    int ret;
    struct crut_operations dup_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"dup_simple",
        test_description:"Tests whether a dup'd file works after restart.",
	test_setup:dup_setup,
	test_precheckpoint:dup_precheckpoint,
	test_continue:dup_continue,
	test_restart:dup_restart,
	test_teardown:dup_teardown,
    };

    struct crut_operations dup_spurious_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"dup_spurious",
        test_description:"Makes sure you can't dup things that you shouldn't.",
	test_setup:dup_spurious_setup,
	test_precheckpoint:dup_spurious_check,
	test_continue:dup_spurious_check,
	test_restart:dup_spurious_check,
	test_teardown:dup_spurious_teardown,
    };

    /* add the basic tests */
    crut_add_test(&dup_test_ops);
    crut_add_test(&dup_spurious_test_ops);

    ret = crut_main(argc, argv);

    return ret;
}
