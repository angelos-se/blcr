/*
 * Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
 * 2007, The Regents of the University of California, through Lawrence
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
 * $Id: bug2003_aux.c,v 1.1 2007/06/01 22:03:38 phargrov Exp $
 *
 * Intentionally screw w/ sa_restorer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "blcr_common.h" /* For CR_SIGNUM */

int main(void) {
    /* corrupt the sa_restorer!!
     * This approximates what guile does when it saves/restores
     * sa_sigaction and sa_flags w/o preserving sa_restorer.
     */
    struct sigaction sa;
    sigset_t mask;

    (void)sigaction(CR_SIGNUM, NULL, &sa); /* Ignore any failure */
    if (sa.sa_restorer) {
	sa.sa_restorer = (void *)&abort;
	(void)sigaction(CR_SIGNUM, &sa, NULL); /* Ignore any failure */
    } else {
	/* Don't mess w/ NULL */
    }

    /* Wait upto 30 seconds for the checkpoint */
    (void)sigemptyset(&mask);
    (void)alarm(30);
    (void)sigsuspend(&mask);
    (void)alarm(0);

    return 0;
}
