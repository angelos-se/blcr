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
 * $Id: cr_trace.h,v 1.15 2008/02/13 22:41:59 phargrov Exp $
 */

#ifndef _CR_TRACE_H
#define _CR_TRACE_H	1

#include <stdlib.h>	// for abort()
#include <signal.h>	// for signal()
#include <unistd.h>	// for _exit()

extern void libcr_trace_init(void);

extern void libcr_trace(const char *filename,
			int line,
			const char *function,
			const char * format,
			...)
	__attribute__ ((format (printf, 4, 5)));

#if LIBCR_TRACING || !defined(LIBCR_SIGNAL_ONLY)
  #define __cri_abort	libcr_trace
#else
  #include <stdio.h>
  #define __cri_abort(__FILE__, __LINE__, __FUNCTION__, args...) \
	fprintf(stderr, args)
#endif
#define CRI_ABORT(args...)					\
    do {							\
	__cri_abort(__FILE__, __LINE__, __FUNCTION__, args);	\
	signal(SIGABRT, SIG_DFL);				\
	abort();						\
	_exit(42);						\
    } while(0);


#if LIBCR_TRACING

extern unsigned int libcr_trace_mask;

#define	LIBCR_TRACE_NONE	0
#define	LIBCR_TRACE_ALL		(~0)

#define	LIBCR_TRACE_INFO	0x00000001

#define LIBCR_TRACE(trace, args...)				\
  do {								\
    if (libcr_trace_mask & (trace)) {				\
	libcr_trace(__FILE__, __LINE__, __FUNCTION__, args);	\
    }								\
  } while (0)

#define LIBCR_TRACE_INIT() libcr_trace_init()

#else

/* If not tracing then just eat the args */
#define LIBCR_TRACE(args...)	do {} while (0)
#define LIBCR_TRACE_INIT()	do {} while (0)

#endif

#endif
