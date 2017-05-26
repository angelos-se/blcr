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
 * $Id: dlopen_aux.c,v 1.1.30.1 2013/03/26 01:11:57 phargrov Exp $
 */

#define _GNU_SOURCE 1 // For RTLD_DEFAULT
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <features.h>
#include <string.h>

#include "libcr.h"

#ifndef _STRINGIFY
  #define _STRINGIFY_HELPER(x) #x
  #define _STRINGIFY(x) _STRINGIFY_HELPER(x)
#endif

int main(void)
{
    char filename[] = "libcr.so." _STRINGIFY(LIBCR_MAJOR);
    cr_client_id_t (*my_cr_init)(void);
    void *self_handle = dlopen(NULL, RTLD_LAZY);
    void *libcr_handle;
    cr_client_id_t client_id;

    my_cr_init = dlsym(self_handle, "cr_init");
    if (my_cr_init != NULL) {
	fprintf(stderr, "cr_init found unexpectedly in 'self', before dlopen()\n");
	exit(1);
    }

    my_cr_init = dlsym(RTLD_DEFAULT, "cr_init");
    if (my_cr_init != NULL) {
	fprintf(stderr, "cr_init found unexpectedly in default search, before dlopen()\n");
	exit(1);
    }

    libcr_handle = dlopen(filename, RTLD_NOW);
    if (libcr_handle == NULL) {
	fprintf(stderr, "dlopen(%s) failed unexpectedly.  Bad LD_LIBRARY_PATH?\n", filename);
	exit(1);
    }

    my_cr_init = dlsym(self_handle, "cr_init");
    if (my_cr_init != NULL) {
	fprintf(stderr, "cr_init found unexpectedly in 'self', after dlopen()\n");
	exit(1);
    }

    my_cr_init = dlsym(RTLD_DEFAULT, "cr_init");
    if (my_cr_init != NULL) {
	fprintf(stderr, "cr_init found unexpectedly in default search, after dlopen()\n");
	exit(1);
    }

    my_cr_init = dlsym(libcr_handle, "cr_init");
    if (my_cr_init == NULL) {
	fprintf(stderr, "cr_init not in dlopen()ed library\n");
	exit(1);
    }

    client_id = my_cr_init();
    if (client_id < 0) {
	fprintf(stderr, "cr_init() call failed\n");
	exit(1);
    }

    return 0;
}
