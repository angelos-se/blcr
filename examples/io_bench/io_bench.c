/*
 * Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
 * 2006, The Regents of the University of California, through Lawrence
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
 * $Id: io_bench.c,v 1.12 2008/12/17 04:17:10 phargrov Exp $
 */

#define _FILE_OFFSET_BITS 64 /* For 64-bit LFS in open() and fstat() */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
 
#include "libcr.h"

#ifndef min
  #define min(_x,_y) ((_x)<(_y)?(_x):(_y))
#endif

static cr_checkpoint_handle_t my_handle;
static cr_checkpoint_args_t my_args;

void request_it(int fd)
{
    int ret;

    cr_initialize_checkpoint_args_t(&my_args);
    my_args.cr_fd = fd;
    my_args.cr_scope = CR_SCOPE_PROC;

    /* issue the request */
    ret = cr_request_checkpoint(&my_args, &my_handle);
    if (ret < 0) {
        perror("cr_request_checkpoint");
        exit(1);
    }
}

void poll_it(struct stat *s)
{
    int ret;

    do {
        ret = cr_poll_checkpoint(&my_handle, NULL);
        if (ret < 0) {
            if ((ret == CR_POLL_CHKPT_ERR_POST) && (errno == CR_ERESTARTED)) {
                /* restarting -- not an error */
                return;
            } else if (errno == EINTR) {
                /* poll was interrupted by a signal -- retry */
            } else {
                perror("cr_poll_checkpoint");
                exit(1);
            }
        } else if (ret == 0) {
            fprintf(stderr, "cr_poll_checkpoint returned unexpected 0\n");
            ret = -1;
            exit(1);
        }
    } while (ret < 0);

    ret = fstat(my_args.cr_fd, s);
    if (ret < 0) {
	printf("ERROR: stat() failed\n");
	exit(-1);
    }

    (void)fdatasync(my_args.cr_fd);
    (void)close(my_args.cr_fd);
}


extern uint64_t gettimeofday_us(void) {
  uint64_t retval;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  retval = ((uint64_t)tv.tv_sec) * 1000000 + (uint64_t)tv.tv_usec;
  return retval;
}

int work(int argc, const char **argv) {
    size_t remain;
    int mb;

    if (argc != 2) {
      printf("ERROR: %s requires 1 argument, the memory size in megabytes\n", argv[0]);
      return 0;
    }
    mb = atoi(argv[1]);
    if (mb <= 0) {
      printf("ERROR: '%s' is not a positive integer\n", argv[1]);
      return 0;
    }

    remain = (size_t)mb * 1024 * 1024;
    while (remain) {
      size_t try;
      for (try = min(remain,2UL*1024*1024*1024); try != 0; try /= 2) {
	volatile char *ptr = malloc(try);
	if (ptr != NULL) {
	  size_t i;
	  /* Touch each page (more than once if page size > 4k */
	  for (i = 0; i < try; i += 4096) {
	    ptr[i] = (char)1;
	  }
	  remain -= try;
	  break;
	}
      }
      if (try == 0) {
        printf("ERROR: failed to allocate %d megabytes of memory\n", mb);
        return 0;
      }
    }

    return mb;
}

int main(int argc, const char **argv)
{
    char filename[40];
    cr_client_id_t my_id;
    uint64_t start_time, elapsed;
    int rc, fd, mb;
    int do_unlink = 1;
    struct stat s;

    setlinebuf(stdout);

    /* "-k" is undocumented way to suppress the unlink() */
    if (argc >= 2 && !strcmp(argv[1], "-k")) {
      do_unlink = 0;
      --argc; ++argv;
    }

    snprintf(filename, sizeof(filename), "context.%d", (int)getpid());
    fd = open(filename, O_WRONLY|O_CREAT|O_NOCTTY|O_NONBLOCK|O_TRUNC, 0400);
    if (fd < 0) {
	printf("ERROR: open(%s) failed: %s\n", filename, strerror(errno));
	exit(-1);
    }

    my_id = cr_init();
    if (my_id < 0) {
	printf("ERROR: cr_init() failed, returning %d\n", my_id);
	exit(-1);
    }

    cr_enter_cs(my_id);
    mb = work(argc, argv);
    if (!mb) {
	cr_leave_cs(my_id);
        if (do_unlink) (void)unlink(filename); // may fail silently
        return -1;
    }
    request_it(fd); // Issues request, which will be blocked by the critical section
    rc = cr_status();
    if (rc != CR_STATE_PENDING) {
	printf("ERROR: cr_status() unexpectedly returned 0x%x\n", rc);
	exit(-1);
    }

    start_time = gettimeofday_us();
    cr_leave_cs(my_id);	// should cause checkpoint NOW
    poll_it(&s); // Reap the checkpoint request
    elapsed = gettimeofday_us() - start_time;
    if (do_unlink) (void)unlink(filename); // may fail silently

    rc = cr_status();
    if (rc != CR_STATE_IDLE) {
	printf("ERROR: cr_status() unexpectedly returned 0x%x\n", rc);
	exit(-1);
    }

    printf("Checkpoint w/ %d MB heap took %ld us (%#.2f MB/s total)\n",
           mb, (long)elapsed, ((double)s.st_size) / (1.048576 * elapsed));

    return 0;
}
