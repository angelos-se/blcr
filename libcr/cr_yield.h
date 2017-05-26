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
 * $Id: cr_yield.h,v 1.11 2008/04/23 01:39:19 phargrov Exp $
 *
 *
 * cr_yield.h: macros for yielding
 */

#ifndef _CR_YIELD_H
#define _CR_YIELD_H 1

#include "blcr_config.h"

#include <time.h>
#include <signal.h>

/* how many calls to cri_sched_yield() before sleeping */
#define CRI_MAX_YIELD	50		/* times to yield before sleeping */

/* How many nanoseconds to sleep, must be larger than 2ms
   or the kernel might just busy wait without yielding. */
#define CRI_SLEEP_NS	2000001

CR_INLINE void cri_yield(int *count)
{
    static const struct timespec ts = {tv_sec: 0, tv_nsec: CRI_SLEEP_NS};

    if ((*count)++ < CRI_MAX_YIELD) {
	/* Try to yielding to allow the lock to be released.
	   If we have higher priority than the thread which holds
	   the lock then this won't actually let that thread run. */
	(void)__cri_sched_yield(NULL);
    } else {
	/* Try sleeping to allow the lock to be released.
	   By sleeping for > 2ms we ensure that the kernel will
	   actually sleep, rather than busy wait as it may for
	   some realtime threads. */
	(void)__cri_nanosleep(&ts, NULL, NULL);
	*count = 0;
    }
}

#if 0
#	include <stdio.h>
#	define CRI_YIELD_DEBUG(COND)	fprintf(stderr, "Yield: " #COND "\n")
#else
#	define CRI_YIELD_DEBUG(COND)	do {} while (0)
#endif 

#define CRI_DO_YIELD_WHILE_COND(COND)			\
	do {						\
	    int _count = 0;				\
	    do {					\
		CRI_YIELD_DEBUG(COND);			\
		cri_yield(&_count);			\
	    } while (COND);				\
	} while(0)

#define CRI_WHILE_COND_YIELD(COND)			\
	do {						\
	    int _count = 0;				\
	    while (COND) {				\
		CRI_YIELD_DEBUG(COND);			\
		cri_yield(&_count);			\
	    }						\
	} while(0)

#endif /* _CR_YIELD_H */
