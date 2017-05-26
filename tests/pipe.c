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
 * $Id: pipe.c,v 1.15.8.4 2011/10/03 00:36:51 phargrov Exp $
 *
 * Simple tests of pipe save and restore.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>

#include <linux/limits.h>

#include "blcr_config.h"
#include "crut.h"
#include "crut_util.h"

/* should we try to run the pipe resize test?  We only try if the kernel
 * has the pipe_fcntl() call, and if we pulled the right definitions.  */
#if defined(F_SETPIPE_SZ) && defined(F_GETPIPE_SZ)
    #define TEST_PIPE_FCNTL 1
#elif defined(HAVE_PIPE_FCNTL) && HAVE_PIPE_FCNTL
    #if !defined(F_SETPIPE_SZ) && defined(CR_F_SETPIPE_SZ)
        #define F_SETPIPE_SZ CR_F_SETPIPE_SZ
    #endif
    #if !defined(F_GETPIPE_SZ) && defined(CR_F_GETPIPE_SZ)
        #define F_GETPIPE_SZ CR_F_GETPIPE_SZ
    #endif
    #if defined(F_SETPIPE_SZ) && defined(F_GETPIPE_SZ) 
        #define TEST_PIPE_FCNTL 1
    #else
        #warning "Found pipe_fcntl in kernel, but could not find F_SETPIPE_SZ."
        #define TEST_PIPE_FCNTL 0
    #endif
#else
    #define TEST_PIPE_FCNTL 0
#endif

#define PIPE_LARGE_MIN 1048576

#define LARGE_BUF_SIZE 4*PIPE_BUF
#define SMALL_BUF_SIZE 256

#define PIPEDATA "Hello world!\n\0"
#define PIPES_MAX 8

#define TEST_BLOCKING 0
#define TEST_NONBLOCKING 1
#define TEST_SETSIZE 2

#if TEST_PIPE_FCNTL
#define SETPIPE_SZ_SIZE 1048576L
int have_pipe_fcntl = 1;
long test_pipe_size;
#endif

int barrier = -1;

int sigpiped = 0;

struct pipe_struct {
   int pipe_fd;
   struct stat pipe_stat;
   int pipe_flags;
   int pipe_mate_fd;
};

struct pipe_struct pipe_array[PIPES_MAX];

static void * do_pipe_block_io(void *p);

void
sigpipe_handler(int num)
{
   sigpiped = 1;
}

static int
update_pipe_struct(struct pipe_struct *p)
{
    int retval;

    retval = fstat(p->pipe_fd, &p->pipe_stat);
    if (retval < 0) {
        perror("fstat");
        goto out;
    }

    retval = fcntl(p->pipe_fd, F_GETFL);
    if (retval < 0) {
        perror("fcntl");
        goto out;
    } else {
        p->pipe_flags = retval;
    }

out:
    return retval;
}

#if 0 /* unused */
/*
 * finds a pipe struct corresponding to fd
 *
 * returns NULL if no corresponding pipe_struct was found
 */
static struct pipe_struct *
find_pipe_struct_mate(int fd)
{
    int i;
    struct pipe_struct *mate = NULL;

    for (i=0; i<PIPES_MAX; ++i) {
        mate = &pipe_array[i];
	if (mate->pipe_fd == fd)
	    break;
    }

    return mate;
}
#endif

/*
 * this should probably be protected by some kind of lock...
 *
 * For now, don't mess with this unless there are no other threads around.
 */
static int
init_pipe_struct_pair(struct pipe_struct *read_pipe, struct pipe_struct *write_pipe)
{
    int retval;
    int pipefds[2];

    retval = pipe(pipefds);
    if (retval < 0) {
        perror("Unable to create a new pipe!");
	goto out;
    }

    read_pipe->pipe_fd       = pipefds[0];
    read_pipe->pipe_mate_fd  = pipefds[1];
    write_pipe->pipe_fd      = pipefds[1];
    write_pipe->pipe_mate_fd = pipefds[0];

    retval = update_pipe_struct(read_pipe);
    if (retval < 0) {
        goto out;
    }
    retval = update_pipe_struct(write_pipe);
    
out:
    return retval;
}

/*
 * initializes num_pairs pairs of pipe_structs (each pair has one reader,
 * one writer)
 */
static int
init_pipe_struct_array(int num_pairs)
{
    int i;
    int retval = -1;

    if (num_pairs*2 > PIPES_MAX) {
        CRUT_FAIL("Too many threads %d, increase PIPES_MAX", num_pairs);
	retval = -1;
	goto out;
    }

    for (i = 0; i<2*num_pairs; i+=2) {
        retval = init_pipe_struct_pair(&pipe_array[i], &pipe_array[i+1]);
        if (retval < 0) {
	    CRUT_FAIL("Error initializing pipe.");
	    goto out;
        }
    }

out:
    return retval;
}

/*
 * NOTE:  Assumes pipe_array has been initialized.
 */
static pthread_t *
init_pipe_block_threads(int num_threads)
{
    pthread_t *thread_vec;
    int retval;
    int i;

    thread_vec = malloc(sizeof(pthread_t) * num_threads);
    if (thread_vec == NULL) {
	perror("malloc");
	goto out;
    }

    barrier = num_threads + 1;

    for (i=0; i<num_threads; ++i) {
        retval = crut_pthread_create(&thread_vec[i], NULL, do_pipe_block_io, 
	        &pipe_array[i]);
        if (retval) {
            perror("pthread_create(reader)");
            goto out_cancel;
        }
    }

    return thread_vec;

out_cancel:
    for (; i>=0; --i) {
	if (pthread_cancel(thread_vec[i])) {
	    perror("pthread_cancel");
	}
    }
//out_free:
    free(thread_vec);
out:
    return NULL;
}

static int
pipe_one_setup(void **p)
{
    int retval;

    signal(SIGPIPE, sigpipe_handler);

    CRUT_DEBUG("Creating pipe.");
    retval = init_pipe_struct_pair(&pipe_array[0], &pipe_array[1]);
    if (retval < 0) {
	CRUT_FAIL("Error initializing pipe.");
	goto out;
    }

    *p = NULL;

out:
    return retval;
}

static int
pipe_teardown(void *p)
{
    int retval = 0;

    return retval;
}

static void __attribute__ ((__unused__))
dump_pipe_struct_stat(struct pipe_struct *p, const char *name)
{
    struct stat *s = &p->pipe_stat;

    CRUT_DEBUG("%s.dev_t\t%llu", name, (unsigned long long)s->st_dev);      /* device */
    CRUT_DEBUG("%s.ino_t\t%lu", name, (unsigned long)s->st_ino);      /* inode */
    CRUT_DEBUG("%s.mode_t\t%u", name, (unsigned)s->st_mode);     /* protection */
    CRUT_DEBUG("%s.nlink_t\t%u", name, (unsigned)s->st_nlink);    /* number of hard links */
    CRUT_DEBUG("%s.uid_t\t%u", name, (unsigned)s->st_uid);      /* user ID of owner */
    CRUT_DEBUG("%s.gid_t\t%u", name, (unsigned)s->st_gid);      /* group ID of owner */
    CRUT_DEBUG("%s.dev_t\t%llu", name, (unsigned long long)s->st_rdev);     /* device type (if inode device) */
    CRUT_DEBUG("%s.off_t\t%ld", name, (long)s->st_size);     /* total size, in bytes */
    CRUT_DEBUG("%s.blksize_t\t%ld", name, (long)s->st_blksize);  /* blocksize for filesystem I/O */
    CRUT_DEBUG("%s.blkcnt_t\t%ld", name, (long)s->st_blocks);   /* number of blocks allocated */
    CRUT_DEBUG("%s.time_t\t%ld", name, (long)s->st_atime);    /* time of last access */
    CRUT_DEBUG("%s.time_t\t%ld", name, (long)s->st_mtime);    /* time of last modification */
    CRUT_DEBUG("%s.time_t\t%ld", name, (long)s->st_ctime);    /* time of last change */
}

static int
write_pipe_small(struct pipe_struct *p)
{
    char buf[SMALL_BUF_SIZE];
    int retval;

    strncpy(buf, PIPEDATA, sizeof(PIPEDATA));

    CRUT_DEBUG("Writing to pipe");
    retval = write(p->pipe_fd, buf, SMALL_BUF_SIZE);
    if (sigpiped) {
	CRUT_FAIL("SIGPIPE on write");
	exit(1);
    }
    if (retval < 0) {
	CRUT_FAIL("Couldn't write to pipe %d", p->pipe_fd);
    }
    if (retval != SMALL_BUF_SIZE) {
	CRUT_FAIL("Wrote wrong length to pipe %d", p->pipe_fd);
    } else if(retval == SMALL_BUF_SIZE) {
        retval = 0;
    }

    return retval;
}

static int
pipes_pre(int test_to_run)
{
    int retval;

    if (test_to_run == TEST_NONBLOCKING) {
        CRUT_DEBUG("Setting nonblocking flag");
        fcntl(pipe_array[0].pipe_fd, F_SETFL, O_NONBLOCK);
    }

#if TEST_PIPE_FCNTL
    if (test_to_run == TEST_SETSIZE) {
        long pipe_size = 0;

        pipe_size = fcntl(pipe_array[0].pipe_fd, F_GETPIPE_SZ, 0);
        CRUT_DEBUG("before: F_GETPIPE_SZ = %ld", pipe_size);
	if (pipe_size < 0) {
	    /* disable test if this first call failed */
	    have_pipe_fcntl = 0;
	    return 0;
	}

        CRUT_DEBUG("Resizing the pipe buffer");
        fcntl(pipe_array[0].pipe_fd, F_SETPIPE_SZ, SETPIPE_SZ_SIZE);

        /* Use queried size, even if the F_SETPIPE_SZ had failed */
        test_pipe_size = fcntl(pipe_array[0].pipe_fd, F_GETPIPE_SZ, 0);
        CRUT_DEBUG("after: F_GETPIPE_SZ = %ld", test_pipe_size);
    }
#endif
    
    retval = write_pipe_small(&pipe_array[1]);

//out:
    return retval;
}

static int
read_pipe_small(struct pipe_struct *old_read_pipe)
{
    char buf[SMALL_BUF_SIZE];
    int retval;
    struct pipe_struct new_read_pipe_s;

    memcpy(&new_read_pipe_s, old_read_pipe, sizeof(new_read_pipe_s));

    update_pipe_struct(&new_read_pipe_s);

    /* check the stat structure */
    retval =
        statcmp(&old_read_pipe->pipe_stat, &new_read_pipe_s.pipe_stat,
                ST_MODE | ST_NLINK);
    if (retval) {
        CRUT_DEBUG("File attributes changed.  %d mismatches", retval);
        CRUT_DEBUG("--- Old stats %p ---", &(old_read_pipe->pipe_stat));
        dump_stat(&(old_read_pipe->pipe_stat));
        CRUT_DEBUG("--- Current stats ---");
        dump_stat(&new_read_pipe_s.pipe_stat);
        retval = -1;
        goto out;
    }

    memset(buf, 0, SMALL_BUF_SIZE);
    CRUT_DEBUG("Reading from pipe.");
    retval = read(new_read_pipe_s.pipe_fd, buf, SMALL_BUF_SIZE);
    if (retval < 0) {
        CRUT_FAIL("Couldn't read from pipe.");
        CRUT_FAIL("buf='%s'", buf);
        goto out;
    }

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
write_long(int fd, void *buf, int size)
{
    int retval;
    int bytes_written = 0;

    if (size == 0) {
	CRUT_DEBUG("WARNING:  write_long asked for 0 bytes");
	retval = 0;
	goto out;
    }

    /* size_t had better be small enough to write atomically */
    if (sizeof(size) > PIPE_BUF) {
        CRUT_FAIL("PIPE_BUF too small to hold an integer???");
	retval = -1;
	goto out;
    }

    retval = write(fd, &size, sizeof(size));
    if (retval < 0) {
        perror("write");
	goto out;
    }	

    while(bytes_written != size) {
        retval = write(fd, ((char *)buf)+bytes_written, size - bytes_written);
        if (retval < 0) {
	    perror("write");
            retval = bytes_written;
            goto out;
	}
	bytes_written += retval;
    }	

    retval = bytes_written;
out:
    return retval;
}

static int
write_finished(int fd)
{
    int retval;
    int zero = 0;

    retval = write(fd, &zero, sizeof(zero));
    if (retval < 0) {
        perror("write");
    }
    
    return retval;
}

static int
read_long(int fd, void *buf, int bufsize)
{
    int retval;
    int bytes_read = 0;
    int size;

    if (bufsize == 0) {
	CRUT_DEBUG("WARNING:  caller asked for 0 bytes");
        retval = 0;
	goto out;
    }

    /* size_t had better be small enough to read atomically */
    if (sizeof(size) > PIPE_BUF) {
        CRUT_FAIL("PIPE_BUF too small to hold an integer???");
	retval = -1;
	goto out;
    }

    retval = read(fd, &size, sizeof(size));
    if (retval < 0) {
        perror("read");
	goto out;
    }

    /* check for EOT */
    if (size == 0) {
	CRUT_DEBUG("write finished...");
        retval = 0;
        goto out;
    }

    if (bufsize < size) {
        CRUT_FAIL("read buffer too small to hold destination message.");
        retval = -1;
	goto out;
    }

    while(bytes_read != size) {
        retval = read(fd, ((char *)buf) + bytes_read, size - bytes_read);
	if (retval < 0) {
            perror("read");
	    retval = bytes_read;
	    goto out;
	}
	bytes_read += retval;
    }

    retval = bytes_read;
out:
    return retval;
}

static int
read_pipe_large(int fd)
{
    int retval;
    int bytes_read;
    int bufsize = LARGE_BUF_SIZE;
    char buf[bufsize];

    bytes_read = 0;
    do {
        retval = read_long(fd, buf, bufsize);
	bytes_read += retval;
	if (retval != 0 && retval != bufsize) {
	    CRUT_FAIL(
	      "read_long returned wrong number of bytes (%d) from pipe!",
	      retval);
	    retval = -1;
            goto out;
	}
    } while (retval > 0);

    crut_wait(CRUT_EVENT_CONTINUE);
    crut_barrier(&barrier);

    do {
        retval = read_long(fd, buf, bufsize);
	bytes_read += retval;
	if (retval != 0 && retval != bufsize) {
	    CRUT_FAIL(
	      "read_long returned wrong number of bytes (%d) from pipe!",
	      retval);
	    retval = -1;
            goto out;
	}
    } while (retval > 0);

    retval = bytes_read;
out:
    return retval;
}

static int
write_pipe_large(int fd)
{
    int bytes_written;
    int retval = -1;
    int bufsize = LARGE_BUF_SIZE;
    char *buf;

    buf = pattern_get_data(bufsize, 0);
    if (buf == NULL) {
	perror("pattern_get_data");
        goto out;
    }

    bytes_written = 0;
    while (!crut_poll(CRUT_EVENT_CONTINUE) || bytes_written < PIPE_LARGE_MIN) {
        retval = write_long(fd, buf, bufsize);
	bytes_written += retval;
	if (retval != bufsize) {
	    CRUT_FAIL(
	      "write_long wrote only %d bytes (expected %d) to pipe!",
	      retval, bufsize);
	    retval = -1;
	    goto out;
	}
    }

    retval = write_finished(fd);
    if (retval < 0) {
        perror("write");
	goto out;
    }
    crut_barrier(&barrier);

    while (!crut_poll(CRUT_EVENT_TEARDOWN) || bytes_written < PIPE_LARGE_MIN) {
        retval = write_long(fd, buf, bufsize);
	bytes_written += retval;
	if (retval != bufsize) {
	    CRUT_FAIL(
	      "write_long wrote only %d bytes (expected %d) to pipe!",
	      retval, bufsize);
	    retval = -1;
	    goto out;
	}
    }

    retval = write_finished(fd);
    if (retval < 0) {
        perror("write");
	goto out;
    }

    retval = bytes_written;
out:
    return retval;
}

/*
 * our threads for pipe_block sit here and do this for a while
 */
static void *
do_pipe_block_io(void *p)
{
    int retval;
    int *thread_ret;
    struct pipe_struct *testpipe = (struct pipe_struct *) p;
    int total = 0;

    thread_ret = malloc(sizeof(*thread_ret));
    if (thread_ret == NULL) {
        goto out_nomem;
    }

    CRUT_DEBUG("crut_wait(precheckpoint)");
    retval = crut_wait(CRUT_EVENT_PRECHECKPOINT);
    if (retval < 0) {
        goto out;
    }

    switch (testpipe->pipe_flags & O_ACCMODE) {
    case O_RDONLY:
        retval = read_pipe_large(testpipe->pipe_fd);
        if (retval < 0) {
            perror("read");
            goto out;
        }
	CRUT_DEBUG("reader:  returning %d", retval);
        total = retval;
        break;
    case O_WRONLY:
        retval = write_pipe_large(testpipe->pipe_fd);
        if (retval < 0) {
            perror("write");
            goto out;
        }
        total = retval;
	CRUT_DEBUG("writer:  returning %d", retval);
        break;
    default:
        CRUT_FAIL("Bad flags on pipe.");
        retval = -1;
        break;
    }

out:
    *thread_ret = retval;
    pthread_exit(thread_ret);
    /* never reached */
    return NULL;

out_nomem:
    pthread_exit(NULL);
    /* never reached */
    return NULL;

}


static int
pipe_test(void)
{
    return read_pipe_small(&pipe_array[0]);
}

static int
pipe_block_generic_setup(void **testdata, int num_threads)
{
    int retval;
    pthread_t *test_threads;

    signal(SIGPIPE, sigpipe_handler);

    retval = init_pipe_struct_array(num_threads/2);
    if (retval < 0) {
	test_threads = NULL;
        goto out;
    }

    test_threads = init_pipe_block_threads(num_threads);
    if (test_threads == NULL) {
	/* Note.  We don't bother to clean up pipe_array in this case. */
	retval = -1;
	goto out;
    }

out:
    *testdata = test_threads;
    return retval;
}

static int
pipe_block_setup(void **testdata)
{
    return pipe_block_generic_setup(testdata, 2);
}

static int
pipe_block_many_setup(void **testdata)
{
    return pipe_block_generic_setup(testdata, PIPES_MAX);
}

static int
pipe_block_generic_precheckpoint(void *p)
{
    int retval = 0;

    CRUT_DEBUG("Getting ready to checkpoint");

    return retval;
}

static int
pipe_block_generic_continue(void *p)
{
    int retval = 0;

    CRUT_DEBUG("Continuing after checkpoint.");
    crut_barrier(&barrier);

    return retval;
}

static int
pipe_block_generic_restart(void *p)
{
    int retval = 0;

    CRUT_DEBUG("Restarting from checkpoint.");
    crut_barrier(&barrier);

    return retval;
}

static int
pipe_block_generic_teardown(void *p, int num_threads)
{
    int retval;
    int join_ret;
    pthread_t *threads = (pthread_t *) p;
    int thread_ret[num_threads];
    void *result;
    int i;

    retval = 0;
    for (i = 0; i < num_threads; ++i) {
        CRUT_DEBUG("waiting for %d...", i);
        join_ret = pthread_join(threads[i], &result);
        if (join_ret) {
            /* what else can we do? */
            perror("pthread_join");
            retval = join_ret;
        }

        /* check return value from thread */
        if (result != NULL) {
            thread_ret[i] = *(int *) result;
            free(result);

            CRUT_DEBUG("Thread %d.  return *(%p)=%d.", i, result,
                       thread_ret[i]);

            if (thread_ret[i] < 0) {
                CRUT_FAIL("Thread %d failed (return %d)", i, thread_ret[i]);
                retval = thread_ret[i];
            }

        } else {
            CRUT_FAIL("Thread %d suffered a fatal error (returned NULL)", i);
        }
    }

    if (retval < 0) {
        goto out;
    }

    /* now make sure the byte counts match */
    for (i = 0; i < num_threads; i += 2) {
        if (thread_ret[i] != thread_ret[i + 1]) {
            CRUT_FAIL("Byte transfer mismatch!  Reader reported %d, but writer reported %d",
                 thread_ret[i], thread_ret[i + 1]);
	    retval = -1;
        }
    }

out:
    free(p);

    CRUT_DEBUG("returning %d", retval);
    return retval;
}

static int
pipe_block_teardown(void *p)
{
    return pipe_block_generic_teardown(p, 2);
}

static int
pipe_block_many_teardown(void *p)
{
    return pipe_block_generic_teardown(p, PIPES_MAX);
}


static int
pipe_rw_pre(void *p)
{
    return pipes_pre(TEST_BLOCKING);
}

static int
pipe_rw_restart(void *p)
{
    return pipe_test();
}

#if TEST_PIPE_FCNTL
static int
pipe_setsize_pre(void *p)
{
    return pipes_pre(TEST_SETSIZE);
}

static int
pipe_setsize_restart(void *p)
{
    long pipe_size;

    if (!have_pipe_fcntl) return 0; /* Nothing to test */

    CRUT_DEBUG("Checking value of F_GETPIPE_SZ.");
    pipe_size = fcntl(pipe_array[0].pipe_fd, F_GETPIPE_SZ, 0);

    if (pipe_size != test_pipe_size) {
        CRUT_FAIL("F_GETPIPE_SZ returned %ld instead of %ld", pipe_size, 
                  test_pipe_size);
    }

    return pipe_test();
}
#endif


static int
pipe_nonblock_pre(void *p)
{
    return pipes_pre(TEST_NONBLOCKING);
}

static int
pipe_nonblock_restart(void *p)
{
    return pipe_test();
}

static int
pipe_many_setup(void **p)
{
    int retval;

    signal(SIGPIPE, sigpipe_handler);

    CRUT_DEBUG("Creating pipes");
    retval = init_pipe_struct_array(PIPES_MAX/2);
    if (retval < 0) {
	*p = NULL;
        goto out;
    }

    *p=NULL;
out:
    return retval;
}

static int
pipe_many_pre(void *p)
{
    int retval = 0;
    int i;

    for (i=1; i<PIPES_MAX; i+=2) {
        retval = write_pipe_small(&pipe_array[i]);
        if (retval < 0) {
	    break;
	}
    }

    return retval;
}


static int
pipe_many_restart(void *p)
{
    int retval = 0;
    int i;

    for (i = 0; i<PIPES_MAX; i+=2) {
        retval = read_pipe_small(&pipe_array[i]);
        if (retval < 0) {
	    CRUT_FAIL("Could not read pipe pair (%d, %d).", i, i+1);
	    break;
        }
    }

    return retval;
}

static int
pipe_many_teardown(void *p)
{
    return 0;
}

int
main(int argc, char *argv[])
{
    int ret;
    struct crut_operations pipe_test_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"pipe_rw",
        test_description:"Pipe tester.  Tests BLCR pipe recovery.",
	test_setup:pipe_one_setup,
	test_precheckpoint:pipe_rw_pre,
	test_continue:pipe_rw_restart,
	test_restart:pipe_rw_restart,
	test_teardown:pipe_teardown,
    };

#if TEST_PIPE_FCNTL
    struct crut_operations pipe_setpipe_sz_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"pipe_setsize",
        test_description:"Test whether fcntl(pipe, F_SETPIPE_SZ, ...) is honored across a restart.",
	test_setup:pipe_one_setup,
	test_precheckpoint:pipe_setsize_pre,
	test_continue:pipe_setsize_restart,
	test_restart:pipe_setsize_restart,
	test_teardown:pipe_teardown,
    };
#endif

    struct crut_operations pipe_nonblock_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"pipe_nonblock",
        test_description:"Tests whether non-blocking flag works on a pipe.",
	test_setup:pipe_one_setup,
	test_precheckpoint:pipe_nonblock_pre,
	test_continue:pipe_nonblock_restart,
	test_restart:pipe_nonblock_restart,
	test_teardown:pipe_teardown,
    };

    struct crut_operations pipe_many_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"pipe_many",
        test_description:"Tests whether multiple pipes can be restored.",
	test_setup:pipe_many_setup,
	test_precheckpoint:pipe_many_pre,
	test_continue:pipe_many_restart,
	test_restart:pipe_many_restart,
	test_teardown:pipe_many_teardown,
    };

    struct crut_operations pipe_block_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"pipe_block",
        test_description:"Tests blocking IO to a pipe (2 threads).",
	test_setup:pipe_block_setup,
	test_precheckpoint:pipe_block_generic_precheckpoint,
	test_continue:pipe_block_generic_continue,
	test_restart:pipe_block_generic_restart,
	test_teardown:pipe_block_teardown,
    };

    struct crut_operations pipe_block_many_ops = {
	test_scope:CR_SCOPE_PROC,
	test_name:"pipe_block_many",
        test_description:"Tests blocking IO to pipes. (>2 threads)",
	test_setup:pipe_block_many_setup,
	test_precheckpoint:pipe_block_generic_precheckpoint,
	test_continue:pipe_block_generic_continue,
	test_restart:pipe_block_generic_restart,
	test_teardown:pipe_block_many_teardown,
    };

    /* add the basic tests */
    crut_add_test(&pipe_test_ops);

#if TEST_PIPE_FCNTL
    /* add the basic tests */
    crut_add_test(&pipe_setpipe_sz_ops);
#endif

    /* add the non-blocking pipe test */
    crut_add_test(&pipe_nonblock_ops);

    /* add the many pipe test */
    crut_add_test(&pipe_many_ops);

    /* add the blocking pipe test */
    crut_add_test(&pipe_block_ops);

    /* add the many-thread pipe test */
    crut_add_test(&pipe_block_many_ops);

    ret = crut_main(argc, argv);

    return ret;
}
