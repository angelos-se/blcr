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
 * $Id: crut_util_libcr.c,v 1.5 2008/12/26 10:50:35 phargrov Exp $
 *
 * Utility functions for BLCR tests (libcr-dependent portions)
 */

#define _LARGEFILE64_SOURCE 1   /* For O_LARGEFILE */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "crut_util.h"

/* checkpoint request/poll wrappers for simpler code */

int
crut_checkpoint_request(cr_checkpoint_handle_t *handle_p, const char *filename) {
    int rc;
    cr_checkpoint_args_t my_args;

    if (filename) {
        /* remove existing context file, if any */
        (void)unlink(filename);

        /* open the context file */
        rc = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE, 0600);
    } else {
	/* NULL -> /dev/null */
        rc = open("/dev/null", O_WRONLY | O_LARGEFILE);
    }
    if (rc < 0) {
        perror("open");
        return rc;
    }

    cr_initialize_checkpoint_args_t(&my_args);
    my_args.cr_fd = rc; /* still holds the return from open() */
    my_args.cr_scope = CR_SCOPE_PROC;

    /* issue the request */
    rc = cr_request_checkpoint(&my_args, handle_p);
    if (rc < 0) {
        (void)close(my_args.cr_fd);
        if (filename) (void)unlink(filename);
        perror("cr_request_checkpoint");
        return rc;
    }

    return my_args.cr_fd;
}

int
crut_checkpoint_wait(cr_checkpoint_handle_t *handle_p, int fd) {
    int rc, save_err;

    do {
        rc = cr_poll_checkpoint(handle_p, NULL);
        if (rc < 0) {
            if ((rc == CR_POLL_CHKPT_ERR_POST) && (errno == CR_ERESTARTED)) {
                /* restarting -- not an error */
                rc = 1; /* Signify RESTART to caller */
            } else if (errno == EINTR) {
                /* poll was interrupted by a signal -- retry */
		continue;
            } else {
                /* return the error to caller */
                break;
            }
        } else if (rc == 0) {
            fprintf(stderr, "cr_poll_checkpoint returned unexpected 0\n");
            rc = -1;
            goto out;
        } else {
            rc = 0; /* Signify CONTINUE to caller */
	}
    } while (rc < 0);

    save_err = errno;
#if 0 // Nothing in the testsuite needs this, but your APP might want it.
    (void)fsync(fd);
#endif
    (void)close(fd);
    errno = save_err;

out:
    return rc;
}

int
crut_checkpoint_block(const char *filename) {
    cr_checkpoint_handle_t my_handle;
    int ret, fd, save_err;

    fd = crut_checkpoint_request(&my_handle, filename);
    if (fd < 0) return fd;

    ret = crut_checkpoint_wait(&my_handle, fd);

    save_err = errno;
    (void)close(fd);
    errno = save_err;

    return ret;
}
