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
 * $Id: file_counting.c,v 1.2 2007/04/28 05:39:21 phargrov Exp $
 */


#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

static int limit = 120; /* default */

int main(int argc, char *argv[])
{
    int i;
    FILE *fp;
    const char *filename = "outfile";
	
    printf("Counting demo starting with pid %d, sending to '%s'\n", (int)getpid(), filename);
    fp = fopen(filename, "w");

    if (argc > 1) {
	limit = (i = atoi(argv[1])) > 0 ? i : limit;
    }

    for (i=0; i<limit; ++i) {
	fprintf(fp, "Count = %d\n", i);
	fflush(fp);
	sleep(1);
    }

    return 0;
}
