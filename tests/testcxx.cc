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
 * $Id: testcxx.cc,v 1.2.2.2 2009/02/14 01:48:56 phargrov Exp $
 */


const char description[] = 
"This test just verifies we can compile and link a C++ program\n";

// Run these all past C++ (even though some are redundant)
#include "crut.h"
#include "crut_util.h"
#include "libcr.h"

#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>
using namespace std;

int misc_stuff(void)
{
    // Misc stuff to test for regression of past known problems
    struct cr_rstrt_relocate *s;
    s = (struct cr_rstrt_relocate *)malloc(CR_RSTRT_RELOCATE_SIZE(CR_MAX_RSTRT_RELOC));

    return 0;
}

static int my_callback(void* arg)
{
    int rc;

    cout << "003 in callback before calling cr_checkpoint()" << endl;
    rc = cr_checkpoint(0);
    if (!rc) {
	cout << "004 in callback continuing after calling cr_checkpoint()" << endl;
    } else {
	cout << "XXX cr_checkpoint(0) unexpectedly returned " << rc << endl;
    }
    return 0;
}

int main(void)
{
    cr_callback_id_t cb_id;
    cr_client_id_t my_id;
    pid_t my_pid;
    char *filename;
    int rc;
    struct stat s;

    rc = misc_stuff();

    my_pid = getpid();
    filename = crut_aprintf("context.%d", my_pid);
    (void)unlink(filename); // might fail silently

    cout << "000 Process started with pid " << my_pid << endl;
    cout << "#ST_ALARM:120" << endl;

    my_id = cr_init();
    if (my_id < 0) {
	cout << "XXX cr_init() failed, returning " << my_id << endl;
	exit(-1);
    } else {
	cout << "001 cr_init() succeeded" << endl;
    }

    cb_id = cr_register_callback(my_callback, NULL, CR_SIGNAL_CONTEXT);
    if (cb_id < 0) {
	cout << "XXX cr_register_callback() unexpectedly returned " << cb_id << endl;
	exit(-1);
    } else {
	cout << "002 cr_register_callback() correctly returned " << cb_id << endl;
    }

    // Request a blocking checkpoint of ourself
    rc = crut_checkpoint_block(filename);
    if (rc < 0) {
	cout << "XXX crut_checkpoint_block() unexpectedly returned " << rc
             << " (errno: " << cr_strerror(errno) << ")" << endl;
	exit(-1);
    }

    // Did we at least create the file?
    rc = stat(filename, &s);
    if (rc) {
	cout << "XXX stat() unexpectedly returned " << rc << endl;
	exit(-1);
    } else {
	cout << "005 stat(" << filename << ") correctly returned 0" << endl;
    }

    // Is the file non-empty
    if (s.st_size == 0) {
	cout << "XXX context file unexpectedly empty" << endl;
	exit(-1);
    } else {
	cout << "006 " << filename << " is non-empty" << endl;
    }
    (void)unlink(filename); // might fail silently

    cout << "007 DONE" << endl;

    return 0;
}
