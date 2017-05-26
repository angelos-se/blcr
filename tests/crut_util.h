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
 * $Id: crut_util.h,v 1.22.4.2 2009/03/12 20:07:38 phargrov Exp $
 *
 * Header file for BLCR utility functions
 */

#ifndef _CRUT_UTIL_H
#define _CRUT_UTIL_H

#include "libcr.h"
#include "stdio.h"

__BEGIN_DECLS

/*
 * Debug/vebose output
 * VERBOSE:  Overall test progress
 * FAIL: When a test fails
 * DEBUG: When you want to see the gory details...
 */
extern unsigned int crut_trace_mask;
extern char *crut_program_name;
extern void
crut_print_trace(const char *file, int line, const char *func, const char *fmt, ...)
                 __attribute__((format (printf, 4, 5)));

# define CRUT_FAIL(args...) \
         CRUT_TRACE(CRUT_TRACETYPE_FAIL, args)
# define CRUT_VERBOSE(args...) \
         CRUT_TRACE(CRUT_TRACETYPE_VERBOSE, args)
# define CRUT_DEBUG(args...) \
         CRUT_TRACE(CRUT_TRACETYPE_DEBUG, args)

# define CRUT_TRACETYPE_FAIL            0x00000001
# define CRUT_TRACETYPE_VERBOSE         0x00000002
# define CRUT_TRACETYPE_DEBUG           0x00000004

# define CRUT_TRACE_DEFAULT (CRUT_TRACETYPE_FAIL)

# define CRUT_TRACE(type, args...)                                           \
         if (crut_trace_mask & (type)) {                                       \
                crut_print_trace(__FILE__, __LINE__, __FUNCTION__, args); \
         }


/*
 * Compare stat entries
 */
#include <sys/types.h>
#include <sys/stat.h>

#define ST_DEV     0x00000001 /* device */
#define ST_INO     0x00000002 /* inode */
#define ST_MODE    0x00000004 /* protection */
#define ST_NLINK   0x00000008 /* number of hard links */
#define ST_UID     0x00000010 /* user ID of owner */
#define ST_GID     0x00000020 /* group ID of owner */
#define ST_RDEV    0x00000040 /* device type (if inode device) */
#define ST_SIZE    0x00000080 /* total size, in bytes */
#define ST_BLKSIZE 0x00000100 /* blocksize for filesystem I/O */
#define ST_BLOCKS  0x00000200 /* number of blocks allocated */
#define ST_ATIME   0x00000400 /* time of last access */
#define ST_MTIME   0x00000800 /* time of last modification */
#define ST_CTIME   0x00001000 /* time of last change */

extern int statcmp(struct stat *s1, struct stat *s2, unsigned long type);
extern void dump_stat(struct stat *s);

/*
 * Generate test pattern
 */
extern void pattern_fill(char *s, int length, int seed);
extern char *pattern_get_data(int length, int seed);
extern int pattern_compare(char *buf, int length, int seed);

/*
 * Synchronize threads
 */
extern void crut_barrier(int *counter);

/*
 * IPC for multi-process tests
 */
struct crut_pipes {
    int parent, child;	 /* Pids */
    int inpipe, outpipe; /* FDs */
};
extern int crut_pipes_fork(struct crut_pipes *pipes);
extern int crut_pipes_getchar(struct crut_pipes *pipes);
extern void crut_pipes_expect(struct crut_pipes *pipes, unsigned char expect);
extern void crut_pipes_putchar(struct crut_pipes *pipes, unsigned char c);
extern void crut_pipes_write(struct crut_pipes *pipes, const char *s, size_t len);
extern int crut_pipes_close(struct crut_pipes *pipes);
extern void crut_waitpid_expect(int child, unsigned char expect);
extern void crut_waitpid_expect_signal(int child, unsigned char expect);

extern char *crut_find_testsdir(const char *argv0);
extern char *crut_find_cmd(const char *argv0, const char *prog);

/*
 * Tools for progress output to stderr
 *
 * crut_progress_start() performs initialization:
 *   'startval' is the initial value of the "counter", which might have any unit
 *   such as seconds, milliseconds or iterations
 *   'length' is the number of units until the end of the test.  In other words,
 *   the (startval+length) is 100%
 *   'steps' is the maximum number of times to print the percentage.  The output
 *   will be printed fewer times if (length/steps) would be less than 1.
 * crut_progress_step() performs output and boundary check:
 *   'currval' is tested against startval+length, the return is 0 when currval
 *   has reached the end.
 * Ex1: Run a loop for 'Interval' seconds, printing 10%, 20% ... 100%
 *    crut_progress_start(time(NULL), Interval, 10);
 *    do { something; } while (crut_progress_step(time(NULL)));
 * Ex2: Run a for(i=0; i<=MAX; ++i) loop, printing 5%, 10% ... 100%
 *    crut_progress_start(0, MAX, 20);
 *    for (i=0; crut_progress_step(i); ++i) { something; }
 */
extern void crut_progress_start(long startval, long length, int steps);
extern int crut_progress_step(long currval);

/*
 * Functions for use of pthreads
 */
extern int crut_is_linuxthreads(void);
extern int crut_pthread_create(pthread_t *thread, pthread_attr_t *attr, void *(*start_routine)(void *), void *arg);

/*
 * Simple wrappers for request/poll
 */
extern int crut_checkpoint_request(cr_checkpoint_handle_t *handle_p, const char *filename);
extern int crut_checkpoint_wait(cr_checkpoint_handle_t *handle_p, int fd);
extern int crut_checkpoint_block(const char *filename);

/* Remove trailing newline if present */
extern char *crut_chomp(char *line);

/* Like glibc's getline(), but not assuming has are using glibc */
extern int crut_getline(char **line_p, size_t *len_p, FILE *stream);

/* Prints to a string and handles allocation automatically */
extern char *crut_aprintf(const char *fmt, ...);

/* Appends to a string and handles reallocation automatically */
extern char *crut_sappendf(char *s, const char *fmt, ...);

/* basename() that returns new storage rather than modifying the argument */
extern char *crut_basename(const char *s);

/* If we are going to use setpgrp() we may need to worry about SIGTTOU */
extern void crut_block_sigttou(void);

__END_DECLS

#endif /* _CRUT_UTIL_H */
