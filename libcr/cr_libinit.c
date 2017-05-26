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
 * $Id: cr_libinit.c,v 1.14.6.5 2012/12/21 07:21:25 phargrov Exp $
 */

#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <dlfcn.h>
#include <features.h>

#include "cr_private.h"

#ifdef LIBCR_SIGNAL_ONLY
  /* Functions for direct syscalls */
  /* These don't require any thread environment and use a caller-provided errno */
  #include <asm/unistd.h>
  #include <linux/unistd.h>
  #if !CR_USE_SIGACTION
    cri_syscall4(int, crsig_ksigaction, __NR_rt_sigaction, int, const struct k_sigaction*, struct k_sigaction*, size_t)
  #endif
  #if LIBCR_TRACING
    cri_syscall2(int, crsig_nanosleep, __NR_nanosleep, const struct timespec*, struct timespec*)
    cri_syscall0(int, crsig_sched_yield, __NR_sched_yield)
  #endif
#endif /* LIBCR_SIGNAL_ONLY */

/* Global var
 * We want this exactly once in each lib, so here is as good a place as any.
 */
const int cri_signum = CR_SIGNUM;

/* Initialization logic.
 */

#if CR_USE_SIGACTION
  #define cri_sigaction sigaction
#else
  #if defined(CRI_SA_RESTORER)
  CRI_SA_RESTORER
  #endif
static int cri_sigaction(int signum, const struct sigaction *act, struct sigaction *oact) {
    struct k_sigaction ksa, oksa;
    int rc;

    if (act) {
      ksa.ksa_sigaction = act->sa_sigaction;
      memcpy(&ksa.ksa_mask, &act->sa_mask, sizeof (sigset_t));
      ksa.ksa_flags = act->sa_flags;
      #if defined(CRI_SA_RESTORER)
        ksa.ksa_flags |= SA_RESTORER;
        ksa.ksa_restorer = &cri_sa_restorer;
      #endif
    }
    rc = __cri_ksigaction(signum, (act ? &ksa : NULL), (oact ? &oksa : NULL), (_NSIG/8), &errno);
    if (oact) {
      oact->sa_sigaction = oksa.ksa_sigaction;
      memcpy(&oact->sa_mask, &oksa.ksa_mask, sizeof (sigset_t));
      oact->sa_flags = oksa.ksa_flags;
      oact->sa_restorer = oksa.ksa_restorer;
    }
    return rc;
}
#endif

/* Since glibc-2.15 __nss_disable_nscd takes a callback:
 *  void cb(size_t dbidx, struct traced_file *info)
 * This is our stand-in for that callback.
 */
static void empty_nscd_cb(size_t dbidx, void *info) { return; }

/* Initialization entry point.
 *
 * We use the 'constructor' attribute to run at startup.
 *
 * TODO:  This doesn't completely solve the problem:  
 *  1)  When statically linked, if no other functions from this library used,
 *  linker won't link the library into the app, and so constructor not run.  
 *	If linking libcr and not calling any functions, then you get what you
 *	deserve.  However, since every client of libcr must call cr_init(),
 *	we need only ensure cr_init() will reference something in cr_core.o.
 *	In fact, cr_init() calls cri_info_init() which is in cr_core.o.
 *	Thus we are certain that any client that actually uses the library
 *	will include the constructor.  This issue is solved.
 *  2) If statically linked AND started with cr_run, init function will be run
 *  twice (and with dynamic lib having its own copy of all data structures).
 *	This is now (almost) solved by ensuring that only the statically linked
 *	version is ever run.  This works because the shared version will
 *	run its initializer first and the static one will overwrite the
 *	signal handler with the one in the static library.  Thus nobody
 *	will ever invoke code in the shared library once the static lib's
 *	constructor has run.  Since this is certain to happen before main(),
 *	we are certain that in the unlikely case that a checkpoint is
 *	requested before the static lib's constructor runs, no callbacks
 *	will be missed. XXX: untested
 *  3) We are unable to interpose on pthread_create, fork or vfork.
 *
 *  For now we still build only shared libs by default.
 */
static void __attribute__((constructor)) cri_init(void)
{
    static int raninit = 0;
    int rc;
    struct sigaction sa;
    int signum;

    if (raninit++) {
	return;
    }

    //
    // Initialize tracing
    //
    LIBCR_TRACE_INIT();

#ifndef LIBCR_SIGNAL_ONLY
    //
    // Initialize pthread-dependent parts if appropriate
    //
    cri_pthread_init();
#endif /* LIBCR_SIGNAL_ONLY */

    //
    // Setup signal handler, with preference to "full" (as opposed to "stub") lib
    //
    if (CR_SIGNUM != __libc_current_sigrtmax()) {
	// Signal is already allocated.  Should we keep or replace?
	void *full_handler = NULL;
	void *dlhandle = dlopen(NULL, RTLD_LAZY);
	if (dlhandle) {
	    // Note that the preloaded one has been name-shifted
	    full_handler = dlsym(dlhandle, "cri_sig_handler");
	    dlclose(dlhandle);
        }

	rc = cri_sigaction(CR_SIGNUM, NULL, &sa);
	if ((sa.sa_sigaction == CR_SIG_HANDLER) ||
	    (full_handler && sa.sa_sigaction == full_handler)) {
	    // Nothing to do
	    return;
	}
#ifndef CR_SIG_IS_FULL
	// Don't displace an unrecognized handler unless we are the "full" handler
	else if (CR_SIG_HANDLER != full_handler) {
	    // XXX: This will fire if one tries to preload/link both libcr_run and libcr_omit.
	    //      However, we probably need to pick a precedence there too.
	    CRI_ABORT("Failed to reregister signal %d in process %d.  "
		      "Saw %p when expecting %p (%s) or %p (cri_sig_handler).",
		      CR_SIGNUM, (int)getpid(), sa.sa_sigaction,
		      CR_SIG_HANDLER, _STRINGIFY(CR_SIG_HANDLER), full_handler);
	}
#endif
    } else if (CR_SIGNUM != (signum = __libc_allocate_rtsig(0))) {
	CRI_ABORT("Failed to allocate signal %d in process %d: got signal %d instead",
		  CR_SIGNUM, (int)getpid(), signum);
    }
    sa.sa_sigaction = CR_SIG_HANDLER;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    rc = sigfillset(&sa.sa_mask);
    if (rc != 0) {
	CRI_ABORT("sigfillset() failed: %s", strerror(errno));
    }
    rc = cri_sigaction(CR_SIGNUM, &sa, NULL);
    if (rc != 0) {
	CRI_ABORT("sigaction() failed: %s", strerror(errno));
    }

#ifdef CR_BUILDING_OMIT
    // Don't try to disable NSCD
#else
    // Disable NSCD if requested (bugs 1962 and 2560)
    {
	const char *val = getenv("LIBCR_DISABLE_NSCD");
	if (val && val[0]) { // Exists and not the empty string
    #if HAVE___NSS_DISABLE_NSCD && 0 // Cannot use a GLIBC_PRIVATE symbol w/ RPMS
	    __nss_disable_nscd(&empty_nscd_cb);
    #else
    	    // If not found at configure time, try via dynamic linker
	    void *dlhandle = dlopen(NULL, RTLD_LAZY);
	    if (dlhandle) {
		void (*disable_nscd)(void *) = dlsym(dlhandle, "__nss_disable_nscd");
		if (disable_nscd) disable_nscd(&empty_nscd_cb);
		dlclose(dlhandle);
	    }
    #endif
	}
    }
#endif
}

/* One symbol (different name in each lib) to help with linking the .a
 *
 * Given the classic "Hello, World!" program (no calls to BLCR), the following
 * won't link libcr, since doing so wouldn't resolve any undefined symbols:
 *  $ gcc -o hello hello.c -static -lcr -ldl -lpthread
 * However, the following do link libcr:
 *  $ gcc -o hello hello.c -static -lcr -ldl -lpthread -u cr_link_me
 *  $ gcc -o hello hello.c -static -lcr_run -ldl -u cr_run_link_me
 *  $ gcc -o hello hello.c -static -lcr_omit -ldl -u cr_omit_link_me
 *
 * Strictly speaking, this is only needed w/ the .a, since just mentioning a .so
 * appears to be sufficient to get all of its constructors run.  However, that
 * might depend on arch or binutils version.  So, we provide symbols always.
 */
#ifndef CR_LINK_ME_VAR
  #error "CR_LINK_ME_VAR is undefined"
#endif
int CR_LINK_ME_VAR = 0;
