/*
 * Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
 * 2009, The Regents of the University of California, through Lawrence
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
 * $Id: bug2524.c,v 1.1.2.3 2009/02/27 20:31:36 phargrov Exp $
 *
 * Look for bug 2524 (ppc-specific reg save problem)
 */

#include <stdio.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>

#include "blcr_config.h"

#if !(defined(__ppc64__) || defined(__powerpc64__) || defined(__ppc__) || defined(__powerpc__))
/* Test is ppc-specific (uses asm) */
int main(void) {
    return 77; // automake's "SKIP" code
}
#else

void handler(int sig) {
   asm volatile ("li %r28,99");
}

int test_it(void) {
    int i, q = 111;
    sigset_t mask;

    sigemptyset(&mask);
    sigprocmask(SIG_BLOCK, NULL, &mask);

    for (i=0; i<1000; ++i) { 
	asm volatile ( // r28 = 0; sigsuspend(&mask); q = %r28
		"	li	%%r28,0\n"
		"	li	%%r0,%3\n"
		"	mr	%%r3,%2\n"
		"	li	%%r4,%4\n"
		"	sc\n"
		"	mr	%0,%%r28\n"
		: "=&r" (q)
		: "0" (q), "r" (&mask), "i" (__NR_rt_sigsuspend), "i" (_NSIG/8)
		: "cr0", "ctr", "memory", "r0", "r3", "r4", "r9", "r10","r11", "r12", "r28");
	if (q) break;
    }

    return q;
}

int main(void) {
    struct sigaction sa;
    int rc;

    sa.sa_sigaction = (void*)handler;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    rc = sigaction(SIGUSR1, &sa, NULL);
    if (rc < 0) {
	perror("sigaction()");
	exit(1);
    }

    rc = fork();
    if (rc < 0) {
	perror("fork()");
	exit(1);
    } else if (!rc) {
	while (!kill(getppid(), SIGUSR1)) {/*empty*/}
    } else if (test_it()) {
      #if !CR_HAVE_BUG2524
	fputs("ERROR: Your kernel appears to be affected by a bug, seen in kernels 2.6.15 and older.  Please reconfigure blcr with '--with-bug2524' to enable blcr's work around for this bug.\n", stderr);
	exit(1);
      #endif
    } else {
      #if CR_HAVE_BUG2524
	fputs("INFO: Your kernel appears NOT to be affected by bug 2524, but BLCR has been configured to work around it anyway.  You could safely reconfigure BLCR with '--without-bug2524' if you wish.\n", stderr);
      #endif
    }

    return 0;
}
#endif /* PPC */
