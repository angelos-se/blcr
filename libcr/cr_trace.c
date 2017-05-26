/* 
 * Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
 * 2003, The Regents of the University of California, through Lawrence
 * Berkeley National Laboratory (subject to receipt of any required
 * approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * Portions may be copyrighted by others, as may be noted in specific
 * copyright notices within specific files.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: cr_trace.c,v 1.12 2008/08/27 21:16:07 phargrov Exp $
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <libgen.h>
#include "cr_private.h"

#define	LIBCR_TRACE_MAX	256
static char libcr_trace_buf1[LIBCR_TRACE_MAX];
static char libcr_trace_buf2[LIBCR_TRACE_MAX];

static int libcr_trace_to_syslog = -1;
static int libcr_trace_fd = -1;

#if LIBCR_TRACING

// CHANGE THIS VALUE TO ENABLE TRACING
unsigned int libcr_trace_mask = LIBCR_TRACE_NONE;

void
libcr_trace_init(void)
{
    const char *mask = getenv("LIBCR_TRACE_MASK");

    if (mask) {
	libcr_trace_mask = strtol(mask, NULL, 0);
    }
}
#endif

void
libcr_trace(const char *filename, int line,
	    const char *function, const char * format, ...)
{
    static cri_atomic_t done_init = {(unsigned int)0};
    int saved_errno = errno;

    char *tmpname, *shortname;
    va_list args;
    int len;

    va_start(args, format);
    vsnprintf(libcr_trace_buf1, LIBCR_TRACE_MAX, format, args);
    va_end(args);

    tmpname = strdup(filename);
    shortname = basename(tmpname);

    /* check destination stuff once */
    if (!cri_atomic_read(&done_init)) {
	static cr_spinlock_t lock = CR_SPINLOCK_INITIALIZER;
	cr_spinlock_lock(&lock);
        if (!cri_atomic_read(&done_init)) {
	    libcr_trace_to_syslog = (getenv("LIBCR_TRACE_TO_SYSLOG") != NULL);
	    if (!libcr_trace_to_syslog) {
	        char *file = getenv("LIBCR_TRACE_FILE");
	        if (file) {
	            libcr_trace_fd = open(file, O_WRONLY|O_APPEND|O_CREAT, 0644);
	        } else {
	            libcr_trace_fd = STDERR_FILENO; /* Default to stderr */
	        }
	    }
	    cri_atomic_write(&done_init, 1);
	}
	cr_spinlock_unlock(&lock);
    }

    if (libcr_trace_to_syslog) {
	syslog(LOG_USER|LOG_NOTICE,
	   "[%d] %s:%d %s: %s", (int)getpid(), shortname, line, function, libcr_trace_buf1);
    } else {
        len = snprintf(libcr_trace_buf2, LIBCR_TRACE_MAX, "%s:%d %s: %s\n",
		       shortname, line, function, libcr_trace_buf1);

        if (len > 0) {
	    if (len > LIBCR_TRACE_MAX) {
	        len = LIBCR_TRACE_MAX;
	    }
	    write(libcr_trace_fd, libcr_trace_buf2, len);
        }
    }
    
    free(tmpname);
    errno = saved_errno;
}
