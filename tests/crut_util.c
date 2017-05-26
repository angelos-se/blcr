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
 * $Id: crut_util.c,v 1.31.6.2 2013/01/03 05:11:25 phargrov Exp $
 *
 * Utility functions for BLCR tests (excluding threaded portions)
 */

#define _GNU_SOURCE /* For strndup() */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <libgen.h>

#include "crut_util.h"

/*
 * Tracing/debugging output
 */
unsigned int crut_trace_mask = CRUT_TRACE_DEFAULT;
char *crut_program_name = "a.out"; /* Silly default */

void
crut_print_trace(const char *file, int line, const char *func, const char * fmt, ...)
{
    va_list args;
    char buf[4096];

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    fprintf(stderr, "%s[%d]: file \"%s\", line %d, in %s: %s\n", crut_program_name, (int)getpid(), file, line, func, buf);
    fflush(stderr);
}

/*
 * statcmp
 * 
 * Compares two struct stats.
 */
int
statcmp(struct stat *s1, struct stat *s2, unsigned long type)
{
    int retval = 0;

    if (type & ST_DEV) {
        retval += s1->st_dev != s2->st_dev;
    }
    if (type & ST_INO) {
        retval += s1->st_ino != s2->st_ino;
    }
    if (type & ST_MODE) {
        retval += s1->st_mode != s2->st_mode;
    }
    if (type & ST_NLINK) {
        retval += s1->st_nlink != s2->st_nlink;
    }
    if (type & ST_UID) {
        retval += s1->st_uid != s2->st_uid;
    }
    if (type & ST_GID) {
        retval += s1->st_gid != s2->st_gid;
    }
    if (type & ST_RDEV) {
        retval += s1->st_rdev != s2->st_rdev;
    }
    if (type & ST_SIZE) {
        retval += s1->st_size != s2->st_size;
    }
    if (type & ST_BLKSIZE) {
        retval += s1->st_blksize != s2->st_blksize;
    }
    if (type & ST_BLOCKS) {
        retval += s1->st_blocks != s2->st_blocks;
    }
    if (type & ST_ATIME) {
        retval += s1->st_atime != s2->st_atime;
    }
    if (type & ST_MTIME) {
        retval += s1->st_mtime != s2->st_mtime;
    }
    if (type & ST_CTIME) {
        retval += s1->st_ctime != s2->st_ctime;
    }

    return retval;
}

void
dump_stat(struct stat *s)
{
    CRUT_DEBUG("dev\t%llu", (unsigned long long)s->st_dev);    /* device */
    CRUT_DEBUG("ino\t%lu", (unsigned long)s->st_ino);     /* inode */
    CRUT_DEBUG("mode\t%u", (unsigned)s->st_mode);    /* protection */
    CRUT_DEBUG("nlink\t%u", (unsigned)s->st_nlink);  /* number of hard links */
    CRUT_DEBUG("uid\t%u", (unsigned)s->st_uid);      /* user ID of owner */
    CRUT_DEBUG("gid\t%u", (unsigned)s->st_gid);      /* group ID of owner */
    CRUT_DEBUG("rdev\t%llu", (unsigned long long)s->st_rdev);  /* device type (if inode device) */
    CRUT_DEBUG("size\t%ld", (long)s->st_size);   /* total size, in bytes */
    CRUT_DEBUG("blksize\t%ld", (long)s->st_blksize);
                                           /* blocksize for filesystem I/O */
    CRUT_DEBUG("blocks\t%ld", (long)s->st_blocks);
                                           /* number of blocks allocated */
    CRUT_DEBUG("atime\t%ld", (long)s->st_atime); /* time of last access */
    CRUT_DEBUG("mtime\t%ld", (long)s->st_mtime); /* time of last modification */
    CRUT_DEBUG("ctime\t%ld", (long)s->st_ctime); /* time of last change */
}

/*
 * pattern_fill
 *
 * Fill a blob of memory with a test pattern
 *
 * This is meant to mimic read on a fake test file.  Would be
 * the same thing as
 * lseek(f, seed)
 * read(f, buf, length)
 */
void
pattern_fill(char *s, int length, int seed)
{
    int i=0;

    while (i<length) {
        s[i] = ((i+seed) % 96) + 33;
        ++i;
    }
}

/*
 * get_pattern_data
 *
 * Return a pointer to data produced by pattern fill
 *
 * Caller must remember to free() the pattern.
 */
char *
pattern_get_data(int length, int seed)
{
    char *tmp;

    tmp = malloc(length);

    if (tmp != NULL) {
        pattern_fill(tmp, length, seed);
    }

    return tmp;
}

/*
 * pattern_compare
 *
 * Compares length bytes of buf to whatever pattern_fill would have
 * given back.
 *
 * Returns 0 if successful, non-zero on error.
 */
int
pattern_compare(char *buf, int length, int seed)
{
    char *tmp;
    int retval = -1;

    tmp = pattern_get_data(length, seed);
    if (tmp != NULL) {
        retval = memcmp(tmp, buf, length);
        free(tmp);
    }

    return retval;
}

/* Creates the pipes, fills in the struct, and forks.
 * Returns non-negative value from fork().
 * Aborts in case of fork() failure.
 */
int crut_pipes_fork(struct crut_pipes *pipes) {
    int fds1[2], fds2[2];
    int retval;

    pipes->parent = getpid();

    CRUT_DEBUG("Creating pipes.");
    retval = pipe(fds1);
    if (retval < 0) {
	CRUT_FAIL("Error initializing pipes.");
	goto out;
    }
    retval = pipe(fds2);
    if (retval < 0) {
	CRUT_FAIL("Error initializing pipes.");
	goto out;
    }

    CRUT_DEBUG("Forking child.");
    retval = fork();
    if (retval < 0) {
	CRUT_FAIL("Error forking child.");
	exit(errno ? errno : -1);
    } else if (!retval) {
	/* In the child */
	pipes->child = getpid();
	pipes->inpipe  = fds1[0]; (void)close(fds1[1]);
        pipes->outpipe = fds2[1]; (void)close(fds2[0]);
    } else {
	/* In the parent */
	pipes->child = retval;
	CRUT_DEBUG("Forked child %d", retval);
	pipes->inpipe  = fds2[0]; (void)close(fds2[1]);
	pipes->outpipe = fds1[1]; (void)close(fds1[0]);
    }

out:
    return retval;
}

/* Read 1 character from our peer.
 * Returns -1 on EOF, aborts on other errors. */
int crut_pipes_getchar(struct crut_pipes *pipes) {
    char c;
    int retval;

    do { retval = read(pipes->inpipe, &c, 1); } while ((retval < 0) && (errno == EINTR));
    if (retval == 0) {
	retval = -1; /* EOF */
    } else if (retval != 1) {
	CRUT_FAIL("retval=%d errno=%d (%s)", retval, errno, strerror(errno));
	exit(-1);
    } else {
	retval = c;
    }

    return retval;
}

/* Read 1 character from our peer, checking for a specific value.
 * Aborts on EOF and other errors.
 * Does exit(char_read) on mismatch (to enable matching of wait() status).
 */
void crut_pipes_expect(struct crut_pipes *pipes, unsigned char expect) {
    int retval = crut_pipes_getchar(pipes);

    if (retval != (int)expect) {
        CRUT_DEBUG("recv unexpected value %d", retval);
	exit(retval);
    }
    CRUT_DEBUG("recv expected value %d", (int)expect);
}

/* Write 1 character to our peer.
 * Aborts on EOF and other errors (but SIGPIPE handler may also run on EOF).
 */
void crut_pipes_putchar(struct crut_pipes *pipes, unsigned char c) {
    int retval;
    do { retval = write(pipes->outpipe, &c, 1); } while ((retval < 0) && (errno == EINTR));
    if (retval != 1) {
	CRUT_DEBUG("retval=%d errno=%d (%s)", retval, errno, strerror(errno));
	exit(-1);
    }
}

/* Write len characters to our peer.
 * Aborts on EOF and other errors (but SIGPIPE handler may also run on EOF).
 */
void crut_pipes_write(struct crut_pipes *pipes, const char *s, size_t len) {
    int retval;
    while (len) {
        do {
	    retval = write(pipes->outpipe, s, len);
        } while ((retval < 0) && (errno == EINTR));
        if (retval < 1) {
	    CRUT_DEBUG("retval=%d errno=%d (%s)", retval, errno, strerror(errno));
	    exit(-1);
        }
	len -= retval;
	s += retval;
    }
}

/* Close our pipes
 */
int crut_pipes_close(struct crut_pipes *pipes) {
    int retval = close(pipes->inpipe);
    if (retval >= 0) { retval = close(pipes->outpipe); }
    return retval;
}


/* Wait for the child, expecting a specific exit code. */
void crut_waitpid_expect(int child, unsigned char expect) {
    int rc, status;

    CRUT_DEBUG("waitpid(%d)", child);
    do {
	rc = waitpid(child, &status, 0);
    } while ((rc < 0) && (errno == EINTR)); /* retry on non-fatal signals */
    if (rc < 0) {
	CRUT_FAIL("Failed to waitpid(%d) errno=%d (%s)", child, errno, strerror(errno));
	exit(-1);
    }
    if ((rc != child) || !WIFEXITED(status) || (WEXITSTATUS(status) != expect)) {
	CRUT_FAIL("Error running child %d, waitpid returned %d, errno=%d, status=%d/%d (expecting rc=%d and status=%d/0)", child, rc, errno, status>>8, status&0xff, child, expect);
	exit(-1);
    }
    CRUT_DEBUG("Child %d exited with expected status %d", child, (int)expect);
}

/* Wait for the child, expecting a specific signal. */
void crut_waitpid_expect_signal(int child, unsigned char expect) {
    int rc, status;

    CRUT_DEBUG("waitpid(%d)", child);
    do {
	rc = waitpid(child, &status, 0);
    } while ((rc < 0) && (errno == EINTR)); /* retry on non-fatal signals */
    if (rc < 0) {
	CRUT_FAIL("Failed to waitpid(%d) errno=%d (%s)", child, errno, strerror(errno));
	exit(-1);
    }
    if ((rc != child) || !WIFSIGNALED(status) || (WTERMSIG(status) != expect)) {
	CRUT_FAIL("Error running child %d, waitpid returned %d, errno=%d, status=%d/%d (expecting rc=%d and status=0/%d)", child, rc, errno, status>>8, status&0xff, child, expect);
	exit(-1);
    }
    CRUT_DEBUG("Child %d exited with expected signal %d", child, (int)expect);
}

/* Look for the tests dir */
char *crut_find_testsdir(const char *argv0) {
    char *dalloc, *dir;
    char *base;
    char *result;
    size_t dirlen;

    base = crut_basename(argv0);
    dir = dirname(dalloc = strdup(argv0));

    if (!strncmp("lt-", base, 3) && !strcmp(dir, ".")) {
	/* ARGV0=basename $PATH=testsdir/.libs:ORIG_PATH */
	const char *p = getenv("PATH");
	const char *q = strchr(p, ':');
	free(dalloc);
	dir = dalloc = q ? strndup(p, q-p) : strdup(p);
    }

    /* Strip trailing "/.libs" from dir if present */
    dirlen = strlen(dir);
    if ((dirlen > 5) && !strcmp("/.libs", dir + (dirlen - 6))) {
	/* ARGV0=testsdir/.libs/basename */
	dir[dirlen - 6] = '\0';
    }

    result = strdup(dir);
    free(base);
    free(dalloc);
    if (0 == access(result, R_OK)) return result;

    free(result);
    return NULL;
}

/* Look for the indicated cr_* command */
char *crut_find_cmd(const char *argv0, const char *prog) {
    char *dir;
    char *result;

    /* When run from Makefile or RUN_ME, fullpath is in environment */
    if (NULL != (result = getenv(prog))) {
	if (0 == access(result, X_OK)) return strdup(result);
    }

    /* When run manually, try to use argv0 to locate the tests directory */
    dir = crut_find_testsdir(argv0);
    if (dir) {
	result = crut_aprintf("%s/../bin/%s", dir, prog);
	free(dir);
	if (0 == access(result, X_OK)) return result;
	free(result);
    }

    /* All else has failed, let $PATH deal with it */
    return strdup(prog);
}

/* For progress output */
static double crut_progress_mult, crut_progress_width, crut_progress_next;
static long crut_progress_lo, crut_progress_hi;

/* Convert positive integer to string.
 * Returns count of digits actually written.
 */
static int
my_itoa(int val, int digits, char *buf) {
  int i;
  char *p = buf + digits - 1;

  /* Work right-aligned in buf */
  for (i = 0; val && (i < digits); i++, p--) {
    div_t d = div(val, 10);
    *p = '0' + d.rem;
    val = d.quot;
  }

  /* Move to be left-aligned in buf */
  memmove(buf, p+1, i);

  return i;
}

void
crut_progress_start(long startval, long length, int steps)
{
   crut_progress_lo = startval;
   crut_progress_hi = startval + length;
   crut_progress_width = (double)length / steps;
   if (crut_progress_width < 1.) crut_progress_width = 1.;
   crut_progress_next = crut_progress_lo + crut_progress_width;
   if (crut_progress_next > crut_progress_hi) crut_progress_next = crut_progress_hi;
   crut_progress_mult = 100.0 / (double)length;
   fflush(stderr);
}

int
crut_progress_step(long currval)
{
  if (currval >= crut_progress_next) {
    char buf[5];
    int val = (crut_progress_mult * (double)(currval - crut_progress_lo));
    int len = my_itoa(val, sizeof(buf)-2, buf+1);
    buf[0] = ' ';
    buf[len+1] = '%';
    write(STDERR_FILENO, buf, len+2);
    do {
      crut_progress_next += crut_progress_width;
    } while (currval >= crut_progress_next);
    if (crut_progress_next > crut_progress_hi) crut_progress_next = crut_progress_hi;
  }
  if (currval >= crut_progress_hi) {
    write(STDERR_FILENO, "\n", 1);
    return 0;
  } else {
    return 1;
  }
}

/* Remove trailing newline if present */
char *crut_chomp(char *line) {
    char *p = line + strlen(line) - 1;
    if (*p == '\n') *p = '\0';
    return line;
}

/* Like glibc's getline(), but not assuming has are using glibc */
int crut_getline(char **line_p, size_t *len_p, FILE *stream) {
    char *line = *line_p;
    size_t len = *len_p;
    size_t have;
    int retval = 0;

    if (!line) {
	len = 32;
	line = malloc(len);
    }

    if (!fgets(line, len, stream)) {
	retval = -1;
	goto out;
    }

    have = strlen(line);
    while ((have == (len-1)) && (line[have-1] != '\n')) {
	line = realloc(line, len*2);

	if (!fgets(line+have, len+1, stream)) {
	    retval = -1;
	    goto out;
	}

	len *= 2;
	have = strlen(line);
    }

out:
    *len_p = len;
    *line_p = line;
    return retval;
}

static char *crut_vsappendf(char *s, const char *fmt, va_list args) {
  int old_len, add_len;

  /* Only one pass permitted per va_start() */
  va_list args2;
  va_copy(args2, args);

  /* compute length of thing to append */
  add_len = vsnprintf(NULL, 0, fmt, args);

  /* grow the string, including space for '\0': */
  if (s) {
    old_len = strlen(s);
    s = realloc(s, old_len + add_len + 1);
  } else {
    old_len = 0;
    s = malloc(add_len + 1);
  }

  /* append */
  vsnprintf((s + old_len), (old_len + add_len + 1), fmt, args2);

  return s;
}

/* Works like sprintf, but appends to first argument w/ realloc() to grow it */
char *crut_sappendf(char *s, const char *fmt, ...) {
  va_list args;

  va_start(args, fmt);
  s = crut_vsappendf(s, fmt, args);
  va_end(args);

  return s;
}

/* Works like sprintf, but allocates the string */
char *crut_aprintf(const char *fmt, ...) {
  va_list args;
  char *s;

  va_start(args, fmt);
  s = crut_vsappendf(NULL, fmt, args);
  va_end(args);

  return s;
}

/* Like basename(), but doesn't modify input.
 * Caller should free() the result */
char *crut_basename(const char *s) {
  char *tmp = strdup(s);
  char *retval = strdup(basename(tmp));
  free(tmp);
  return retval;
}

/* If we are changing our process group then block or ignore
 * SIGTTOU to ensure our output doesn't foul up.
 * NOTE: this is not the most polite way to deal with this
 */
void crut_block_sigttou(void) {
  sigset_t mask;

  sigemptyset(&mask);
  sigaddset(&mask, SIGTTOU);
  sigprocmask(SIG_BLOCK, &mask, NULL);
}

