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
 * $Id: cr_pthread.c,v 1.31 2007/09/20 23:53:34 phargrov Exp $
 */

#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
#include "cr_private.h"

#if PIC && HAVE___REGISTER_ATFORK
// pthread_atfork() handling for shared objects
// glibc has started putting pthread_atfork() in libpthread_nonshared.a, which is causing
// problems when we call from a shared library.  The problem is that the code in the
// static lib gets pulled into libcr.so, but its was not PIC-compiled.
// Therefore, we use our own private re-implementation.
// This implementation is closely based on glibc-2.3.2-20030313/nptl/pthread_atfork.c
extern void *__dso_handle __attribute__((__weak__));
extern int __register_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void), void *dso_handle);
extern int
cri_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void))
{ 
  void *my_handle = &__dso_handle ? __dso_handle : NULL;
  return __register_atfork(prepare, parent, child, my_handle);
}
#endif

// Holds thread-specific data key for cri_info
pthread_key_t cri_info_key;

// atfork callback to reset the state
static void
child_reset(void)
{
    cri_rb_init(&cri_cs_lock);
    cri_atomic_write(&cri_live_count, 0);
    cri_info_free(cri_info_location());
}

#if !HAVE___REGISTER_ATFORK
// Interpose on fork() to ensure the pthreads version is called
// if the caller linked us, but not the pthreads library.
pid_t cri_fork(void)
{
  return __fork();
}
weak_alias(cri_fork,fork);
#endif

//
// Initialize pthread-dependent parts
//
void
cri_pthread_init(void)
{
    int rc;

  #if !HAVE___REGISTER_ATFORK
    // See bug #460
    {
	// dlopen will fail if this is a static exectuable.
	// If so, then the pthread library references must
	// have been resolved.
	void *handle = dlopen(NULL /* the executable */, RTLD_LAZY);
	if (handle) {
	    if (dlsym(handle, "__fork") == dlsym(handle, "__libc_fork")) {
		CRI_ABORT("Unable to ensure pthread_atfork() will run.\n"
			  "If you dlopen(libcr.so) or use some versions of the libraries/loaders, "
			  "then you may need to link libpthread explicitly (or use LD_PRELOAD).");
	    }
	    dlclose(handle);
	}
    }
  #endif /* !HAVE___REGISTER_ATFORK */

    // Install a atfork callback to cleanup state in children
    rc = cri_atfork(NULL, NULL, &child_reset);
    if (rc != 0) {
	CRI_ABORT("cri_atfork() returned %d", rc);
    }

    // Setup the thread-specific data for cri_info
    rc = pthread_key_create(&cri_info_key, &cri_info_free);
    if (rc != 0) {
	CRI_ABORT("pthread_key_create() returned %d", rc);
    }
}
