# Process this file with autoconf to produce a configure script.
#
#   Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
#   2014, The Regents of the University of California, through Lawrence
#   Berkeley National Laboratory (subject to receipt of any required
#   approvals from the U.S. Dept. of Energy).  All rights reserved.
#
#   Portions may be copyrighted by others, as may be noted in specific
#   copyright notices within specific files.
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# $Id: configure.ac,v 1.410.2.63 2014/10/21 00:14:39 phargrov Exp $
#

################################################################################
# version numbers:  separate for 'release' vs. 'libcr' vs. 'kernel'
# - only change these for a new **RELEASE**
################################################################################
#
# 1. Release version:
# -------------------
# For marketing purposes.  Determines the tarball and RPM names,
# and the string that our utilities print for --version.  Bumping the major
# number does NOT cause incompatility, so we can bump from 1.0 to 2.0, etc., w/o
# causing users grief.
#
# For non-released versions, we want RELEASE_PATCH to include some non-numeric
# qualifier.  For instance, after releasing X.Y.0 the RELEASE_PATCH in the CVS
# HEAD should be "pre1".  While preparing for X.Y.1, RELEASE_PATCH might be
# "1_rc0" or similar.  For snapshots sent to collaborators, the RELEASE_PATCH
# should be something like "pre1_snapshot_YYYY_MM_DD" where YYYY, MM and DD
# form a date code.
#
# AC_INIT wants to see constants, not shell vars, so we define these with m4.
m4_define(RELEASE_MAJOR, 0)
m4_define(RELEASE_MINOR, 8)
m4_define(RELEASE_PATCH, 6_b4)

m4_define(RELEASE_VERSION, RELEASE_MAJOR.RELEASE_MINOR.RELEASE_PATCH)
AC_INIT([blcr],[RELEASE_VERSION], [http://ftg.lbl.gov/checkpoint])

# 2. Library version: 
# -------------------
# Determines which versions of the library are compatible with one another. 
#
# 'major' number: increment if backward-compatibility has just been broken
#       - set MINOR and PATCH to 0 whenever MAJOR is incremented.
# 'minor' number: increment if added new feature, but still backward-compatible
#       - set PATCH to 0 whenever MINOR incremented.
# 'patch' number:  increment if changes are bug fixes, not interface changes.
#
# Note: automake doesn't detect changes to the interface number, so you need to
#       'make clean' and rebuild everything to see the new library names.
LIBCR_MAJOR=0
LIBCR_MINOR=5
LIBCR_PATCH=5

# 3. Kernel module version
# ------------------------
# Determines if a particular version of our library/utilities will work with the
# the checkpoint module that's loaded in the kernel.
# 
# A library/utility should die if the kernel's major number is different that
# the one it was built to run against.  If the kernel's minor number is less
# than the interface it expects, it can either die, or restrict its kernel calls
# to those supported by the older interface.
#
# Observe same rules as for library (ie patch->0 when changing minor, etc).
CR_MODULE_MAJOR=0
CR_MODULE_MINOR=10
CR_MODULE_PATCH=3

# Derived version variables ###
#   - Exported, so you can use @CR_FOO@ to get them in any files that configure
#     generates.  
#   - They are all also #defined in blcr_common.h.
CR_RELEASE_MAJOR=RELEASE_MAJOR
CR_RELEASE_MINOR=RELEASE_MINOR
CR_RELEASE_PATCH=RELEASE_PATCH
CR_RELEASE_VERSION="$CR_RELEASE_MAJOR.$CR_RELEASE_MINOR.$CR_RELEASE_PATCH"
AC_SUBST(CR_RELEASE_MAJOR)
AC_SUBST(CR_RELEASE_MINOR)
AC_SUBST(CR_RELEASE_PATCH)
AC_SUBST(CR_RELEASE_VERSION)
LIBCR_VERSION="$LIBCR_MAJOR.$LIBCR_MINOR.$LIBCR_PATCH"
AC_SUBST(LIBCR_MAJOR)
AC_SUBST(LIBCR_MINOR)
AC_SUBST(LIBCR_PATCH)
AC_SUBST(LIBCR_VERSION)
CR_MODULE_VERSION="$CR_MODULE_MAJOR.$CR_MODULE_MINOR.$CR_MODULE_PATCH"
AC_SUBST(CR_MODULE_MAJOR)
AC_SUBST(CR_MODULE_MINOR)
AC_SUBST(CR_MODULE_PATCH)
AC_SUBST(CR_MODULE_VERSION)

# Libtool versioning: they just had to be different...
LIBTOOL_INTERFACE=`expr $LIBCR_MAJOR + $LIBCR_MINOR`
LIBTOOL_AGE=$LIBCR_MINOR
LIBTOOL_REVISION=$LIBCR_PATCH
AC_SUBST(LIBTOOL_INTERFACE)
AC_SUBST(LIBTOOL_AGE)
AC_SUBST(LIBTOOL_REVISION)

# Save our ARGS for use in an RPM spec file
AC_SUBST(CONFIGURE_ARGS)
CONFIGURE_ARGS=$ac_configure_args

### rest of init setup
AC_CONFIG_SRCDIR(cr_module/cr_module.c)
AC_CONFIG_AUX_DIR(./config)
AM_INIT_AUTOMAKE([foreign no-exeext std-options])

################################################################################
# Boilerplate for versions and copyrights in generated files
################################################################################
AC_REVISION($Revision: 1.410.2.63 $)
AC_COPYRIGHT([[
    Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
    2012, The Regents of the University of California, through Lawrence
    Berkeley National Laboratory (subject to receipt of any required
    approvals from the U.S. Dept. of Energy).  All rights reserved.

    Portions may be copyrighted by others, as may be noted in specific
    copyright notices within specific files.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
]])

AH_TOP([/*
 *  Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
 *  2012, The Regents of the University of California, through Lawrence
 *  Berkeley National Laboratory (subject to receipt of any required
 *  approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 *  Portions may be copyrighted by others, as may be noted in specific
 *  copyright notices within specific files.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */])

################################################################################
# Absolute paths to build and source dirs
# need to substitute these into cscope script
################################################################################
AC_PATH_PROG(PWD_PROG, pwd, "pwd -P")
TOP_BUILDDIR=`${PWD_PROG}`
AC_SUBST(TOP_BUILDDIR)
TOP_SRCDIR=`cd $srcdir && $PWD_PROG`
AC_SUBST(TOP_SRCDIR)

################################################################################
# Check which components are enabled.
################################################################################
CR_WITH_COMPONENTS([modules util libcr include tests examples contrib])

################################################################################
# Check platform
################################################################################
AC_CANONICAL_HOST
case $host_os in
  linux*)
    ;;
  *)
    AC_CACHE_SAVE
    AC_MSG_ERROR([[the host system must be Linux (have $host_os)]])
    ;;
esac
AC_SUBST([CR_ARCH],$host_cpu)
AC_SUBST([CR_CPU],$host_cpu)
case $host_cpu in
  i386)
    AC_CACHE_SAVE
    AC_MSG_ERROR([[i386 not supported, i486 or higher is required]])
    ;;
  i?86)
    CR_ARCH=i386
    cr_wordsize=4
    ;;
  x86_64)
    CR_ARCH32=i686
    CR_LIBARCH32=i386
    cr_wordsize=8
    ;;
  ppc64|powerpc64)
    CR_ARCH=ppc64
    CR_CPU=ppc64
    CR_ARCH32=ppc
    cr_wordsize=8
    cr_arch_experimental=yes
    ;;
  ppc|powerpc)
    CR_ARCH=ppc
    CR_CPU=ppc
    cr_wordsize=4
    cr_arch_experimental=yes
    ;;
  arm|armv*)
    CR_ARCH=arm
    CR_CPU=arm
    cr_wordsize=4
    cr_arch_experimental=yes
    ;;
  sparc)
    CR_ARCH=sparc
    cr_wordsize=4
    cr_arch_incomplete=yes
    ;;
  sparc64)
    CR_ARCH=sparc64
    CR_ARCH32=sparc
    cr_wordsize=8
    cr_arch_incomplete=yes
    ;;
  *)
    AC_MSG_ERROR([[Sorry, architecture $host_cpu is not supported at this time.]])
    ;;
esac

################################################################################
# Parse user args into variables
# Try not to call any actual configuration probes yet.
################################################################################

# Let user specify location of Linux kernel headers and build
AC_ARG_WITH([linux],
  [AC_HELP_STRING([--with-linux=DIR],[specify Linux kernel release (e.g. 2.6.7-123) or full path to build directory (default is to search for currently running kernel in some standard locations)])],
  [AS_IF([test x"$with_linux" != xyes],[LINUX_OBJ_ARG=$with_linux])],)
AC_ARG_WITH([linux-src],
  [AC_HELP_STRING([--with-linux-src=DIR],[specify full path to Linux kernel source directory if needed (default is to search in some standard locations)])],
  [AS_IF([test x"$with_linux_src" != xyes],[LINUX_SRC_ARG=$with_linux_src])],)
AC_ARG_WITH([linux-src-ver],
  [AC_HELP_STRING([--with-linux-src-ver=VERSION],[force configure to accept a given kernel source version to use, even if it does not appear to match the build])],
  [AS_IF([test x"$with_linux_src_ver" != xyes],[LINUX_SRC_VER=$with_linux_src_ver])],)

# Let user specify location of kernel System.map and/or vmlinux
AC_ARG_WITH([system-map],
  [AC_HELP_STRING([--with-system-map=FILE],[specify location of Linux kernel System.map file (default is to search some standard locations)])],
  [AS_IF([test x"$with_system_map" != xyes],[LINUX_SYSTEM_MAP=$with_system_map])],)
AC_ARG_WITH([vmlinux],
  [AC_HELP_STRING([--with-vmlinux=FILE],[specify location of kernel image (default is to search some standard locations)])],
  [AS_IF([test x"$with_vmlinux" != xyes],[LINUX_VMLINUX=$with_vmlinux])],)

# Let user specify dir to install kernel modules
AC_ARG_WITH([kmod-dir],
  [AC_HELP_STRING([--with-kmod-dir=DIR],[specify location to install kernel modules])],
  [AS_IF([test x"$with_kmod_dir" != xyes],[CR_MODULE_DIR=$with_kmod_dir])],)

# Let user force UP, SMP or HUGEMEM kernel configs on RH (and similar) systems
AC_ARG_WITH([kernel-type],
  [AC_HELP_STRING([--with-kernel-type=TYPE],[specify kernel type (UP, SMP, BIGMEM, HUGEMEM, etc.) to override
/boot/kernel.h when present (default is not to override)])])

# Let user supress the printing of the final configuration report
CR_ARG_ENABLE([config-report],
	      [enable report at the conclusion of the configure script (default is yes)],
	      [yes])

# Let the user request/supress use of V=1 with kbuild
CR_ARG_ENABLE([kbuild-verbose],
	      [enable verbose output from the kbuild facility.  (default is no)],
	      [no])

# Debugging (implies tracing enabled by default)
CR_ARG_ENABLE([debug], [enable debugging of BLCR (default is no)],[no])
AM_CONDITIONAL(CRI_DEBUG,[test x$enable_debug = xyes])
CR_DEFINE(CRI_DEBUG,[test x$enable_debug = xyes],
	  [Define to 1 to enable debugging of BLCR])
AS_IF([test x$enable_debug = xyes],[CR_NDEBUG=""],[CR_NDEBUG="-DNDEBUG"])
AC_SUBST(CR_NDEBUG)

# Feature libcr-tracing
CR_ARG_ENABLE([libcr-tracing], [enable debugging/tracing output from libcr, controlled by the 'LIBCR_TRACE_MASK' environment variable (default is no)],$enable_debug)
AM_CONDITIONAL(LIBCR_TRACING,[test x$enable_libcr_tracing = xyes])
CR_DEFINE(LIBCR_TRACING,[test x$enable_libcr_tracing = xyes],
	  [Define to 1 to enable debugging/tracing output from libcr, controlled by the 'LIBCR_TRACE_MASK' environment variable])

# Feature kernel-tracing
CR_ARG_ENABLE([kernel-tracing], [enable debugging/tracing output from kernel modules, controlled by the 'cr_ktrace_mask' module option (default is no)], $enable_debug)
AM_CONDITIONAL(CR_KERNEL_TRACING,[test x$enable_kernel_tracing = xyes])
CR_DEFINE(CR_KERNEL_TRACING,[test x$enable_kernel_tracing = xyes],
	  [Define to 1 to enable debugging/tracing output from kernel modules, controlled by the 'cr_ktrace_mask' module option])

# Multilib support
CR_ARG_ENABLE([multilib], [enable both 32- and 64-bit libs from a single build (default is no)], [no])

# Optional /etc/init.d/blcr file
CR_ARG_ENABLE([init-script], [install /etc/init.d/blcr to load modules at boot time (default is no)], [no])
AM_CONDITIONAL(CR_BUILD_ETC,[test x$enable_init_script = xyes])

# Check is restoring uid, gid and supplemental groups is requested
CR_ARG_ENABLE([restore-ids], [enable restoring uid, gid and supplemental groups when invoked by root (default is no)], [no])
CR_DEFINE(CR_RESTORE_IDS,[test x$enable_restore_ids = xyes],
          [Define to 1 to enable restore of uid, gid and supplemental groups when invoked by root.  (default is no)])

# Let user build against an already installed libcr
AC_ARG_WITH([installed-libcr],
  [AC_HELP_STRING([--with-installed-libcr],[link executables against an existing libcr in $includedir and $libdir.  Assumes LD_LIBRARY_PATH is set correctly. (default is to build the library)])],[
	cr_build_libcr=no
	cr_build_include=no ])
AM_CONDITIONAL(CR_INSTALLED_LIBCR,[test x$with_installed_libcr = xyes])

# Let user build/test against already installed utilities
AC_ARG_WITH([installed-util],
  [AC_HELP_STRING([--with-installed-util],[run tests using utilities installed in $bindir (default is to build the utilities)])],[
	cr_build_util=no ])
AM_CONDITIONAL(CR_INSTALLED_UTIL,[test x$with_installed_util = xyes])

# Let user build/run/test against already installed kernel modules
AC_ARG_WITH([installed-modules],
  [AC_HELP_STRING([--with-installed-modules],[use existing blcr kernel modules in $libdir (default is to build the kernel modules)])],[
	cr_build_modules=no ])

# Let user request installation of the test suite
CR_ARG_ENABLE([testsuite], [enable installation of the BLCR testsuite (default is no)], [no])
AM_CONDITIONAL(CR_BUILD_TESTSUITE,[test x$enable_testsuite = xyes])

# Let user request static utils, tests and examples
CR_ARG_ENABLE([all-static],
  [enable linking programs (utilities, tests and examples) with -static flag (default is no)],[no])

# Let user request optional "cr_info" util
CR_ARG_ENABLE([cr-info],
	      [build cr_info utility (default is no, because this is currently broken)],
	      [no])

# Let user specify location of optional FTB headers & libs
AC_ARG_WITH([ftb],
  [AC_HELP_STRING([--with-ftb],[specify path to optional FTB headers and libraries (default /usr/local/ftb)])],
  [FTB_HOME=$withval; cr_force_ftb=yes], [FTB_HOME='/usr/local/ftb'])
#XXX: HACK to disable FTB unless explictly requested
if test x"$cr_force_ftb" != xyes; then FTB_HOME=no; fi

# Let user request work-around for bug 2524 (ppc only)
# TODO: automate when building both kernel modules and libcr
if expr $CR_ARCH : ppc >/dev/null; then
  AC_ARG_WITH([bug2524],
    [AC_HELP_STRING([--with-bug2524],[request that blcr use a work around for bug 2524 (ppc specific)])],
    [force_bug2524=$withval])
fi

################################################################################
# Check and configure the build environment (compiler, linker, etc.)
################################################################################

# Collect doc strings together here
AC_ARG_VAR([PERL],[perl interpreter (required: for configure, build and testsuite)])
AC_ARG_VAR([MAKE],[GNU make (required: for configure and build only)])
AC_ARG_VAR([RPMBUILD],[rpmbuild command (optional: for building RPMs)])
AC_ARG_VAR([KCC],[C compiler command to build kernel modules (default: '$CC')])

# XXX: need to verify version of perl?
AC_PATH_PROGS([PERL],[perl5 perl])
if test -z "$PERL"; then
  AC_MSG_ERROR(cannot find a version of perl - please install perl and/or set \$PERL to indicate where it's located)
fi
AC_SUBST([PERL],$PERL)

# Check for GNU make
AC_PATH_PROGS([MAKE],[gmake make])
if test -n "$MAKE"; then
  AC_MSG_CHECKING(if $MAKE is GNU make)
  MAKE_VERSTR=`$MAKE --version | grep GNU 2> /dev/null`
  if test -z "$MAKE_VERSTR" ; then
    AC_MSG_RESULT(no)
  else
    #MAKE_VER=`echo "$MAKE_VERSTR" | perl -n -e 'if (/([[0-9]]+)\.([[0-9]]+)(\.[[0-9]]+)?/) { $maj=$1; $min=$2; $pat=$3; printf "%i%03i%03i",($maj,$min,($pat?$pat=~s/\.//:0)) }'`
    MAKE_VERSION=`echo "$MAKE_VERSTR" | perl -n -e 'if (/(([[0-9]]+)\.([[0-9]]+)(\.[[0-9]]+)?)/) { print $1 }'`
    AC_MSG_RESULT([yes ($MAKE_VERSION)])
  fi
fi
if test -z "$MAKE_VERSTR" ; then
  AC_MSG_ERROR(cannot find a version of GNU make - please install GNU make and/or set \$MAKE to indicate where it's located)
fi

# What language(s) do we need
AC_LANG([C])

# For static convenience libs
AC_PROG_RANLIB

# For symbolic links
AC_PROG_LN_S

# Need our own probe prior to any done in AC_PROG_LIBTOOL
# Note bug 2619 reports that we can't call this conditionally:
CR_PROG_CXX

# We use libtool to build
#   - only build shared lib for now
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

if test x"$enable_static$enable_all_static" = xnoyes; then
  AC_MSG_ERROR([You cannot pass --enable-all-static without one of --enable-static or --disable-shared])
fi
AM_CONDITIONAL(CR_ENABLE_STATIC,[test $enable_static = yes])
AM_CONDITIONAL(CR_ENABLE_SHARED,[test $enable_shared = yes])

# Check for rpmbuild
CR_PROG_RPMBUILD
AM_CONDITIONAL(HAVE_RPMBUILD,[test $RPMBUILD != none])

# Check for a GNU C compiler (or a work alike?)
CR_PROG_GCC(,,
  [AC_CACHE_SAVE
   AC_MSG_ERROR([[the C compiler '$CC' is not a GNU C compiler.]])])

# Configure inlining (XXX: need to probe gcc for support?)
AC_DEFINE(CR_INLINE,[static __inline__ __attribute__ ((__unused__))],
	  [Define to the proper incantation for inline functions])

# Check for additional flags needed when building libcr
if test x"$cr_build_libcr" = xyes; then
CR_LIBCR_CFLAGS=""
CR_TRY_GCC_FLAG([-Wall], [CR_LIBCR_CFLAGS="$CR_LIBCR_CFLAGS -Wall"])
CR_TRY_GCC_FLAG([-Wno-unused-function], [CR_LIBCR_CFLAGS="$CR_LIBCR_CFLAGS -Wno-unused-function"])
AC_SUBST(CR_LIBCR_CFLAGS)
fi # cr_build_libcr

# Additional flags needed when linking utils, tests and examples
CR_CLIENT_LDADD=""
if test x"$enable_all_static" = xyes; then
  CR_CLIENT_LDADD="$CR_CLIENT_LDADD -all-static"
fi
AC_SUBST(CR_CLIENT_LDADD)

################################################################################
# Check libraries
################################################################################

if test x"$cr_build_libcr" = xyes; then
# Check for a sufficiently new glibc
# XXX: What is our true lower bound?
CR_CHECK_GLIBC(2,2,,
  [AC_CACHE_SAVE
   AC_MSG_ERROR([[the C library does not appear to be glibc 2.2 or higher.]])])

# Checks for required libraries and fail if they are not found 
# Note that we are NOT adding them to LIBS, just verifying their presence.
AC_CHECK_LIB([dl], [dlsym], [:],
  [AC_CACHE_SAVE
   AC_MSG_ERROR([[required library libdl not found.]])])
AC_CHECK_LIB([pthread], [pthread_create], [:],
  [AC_CACHE_SAVE
   AC_MSG_ERROR([[required library libpthread not found.]])])

# Prep a variable for possible use in messages below
if test x"$enable_multilib" = xyes; then
  ldsuggest='-L/usr/lib/nptl -L/usr/lib64/nptl'
elif test x"$ac_cv_sizeof_void_p" = x8; then
  ldsuggest='-L/usr/lib64/nptl'
else
  ldsuggest='-L/usr/lib/nptl'
fi

# Helper for LinuxThreads probes
# CR_LT_PROBE(cv_varname, cross_varname)
AC_DEFUN([CR_LT_PROBE],[
  AC_TRY_RUN([
	#include <sys/types.h>
	#include <unistd.h>
	#include <stdio.h>
	#include <pthread.h>

	static void *thread_pid(void *arg) { return (void *)(long)getpid(); }

	int main(void) {
	    pthread_t th;
	    void *join_val;

	    if (0 != pthread_create(&th, NULL, &thread_pid, NULL)) {
		fputs("Error calling pthread_create()\n", stderr);
		return -1;
	    }
	    if (0 != pthread_join(th, &join_val)) {
		fputs("Error calling pthread_join()\n", stderr);
		return -1;
	    }

	    /* zero = NOT equal pids = LinuxThreads */
	    return ((long)join_val == (long)getpid());
	}], [$1=yes], [$1=no],[
    CR_CROSS_VAR($2)
    AS_IF([test x"$[]$2" != x0], [$1=yes], [$1=no])])
])

# Look for LinuxThreads in the default (probably dynamic) library
AC_CACHE_CHECK([[whether default pthreads library is LinuxThreads]], cr_cv_pt_default_lt, [
  SAVE_LIBS="$LIBS"
  LIBS="-lpthread $LIBS"
  CR_LT_PROBE([cr_cv_pt_default_lt],[cross_linuxthreads])
  LIBS="$SAVE_LIBS"
])
if test x"$cr_cv_pt_default_lt" = xyes; then
  AC_MSG_ERROR([[linking of BLCR to LinuxThreads is NOT supported.  Please try LDFLAGS='$ldsuggest' (or similar) on the configure command line to get NPTL.]])
fi

# Now look for LinuxThreads in the static libs if we are building static ones ourself
if test x"$enable_static" = xyes; then
  # Look for LinuxThreads in the static library
  AC_CACHE_CHECK([[whether static pthreads library is LinuxThreads]], cr_cv_pt_static_lt, [
    SAVE_LIBS="$LIBS"
    SAVE_LDFLAGS="$LDFLAGS"
    LDFLAGS="$LDFLAGS -static"
    LIBS="-lpthread $LIBS"
    CR_LT_PROBE([cr_cv_pt_static_lt],[cross_linuxthreads_static])
    LIBS="$SAVE_LIBS"
    LDFLAGS="$SAVE_LDFLAGS"
  ])
  if test x"$cr_cv_pt_static_lt" = xyes; then
    AC_MSG_WARN([[static linking with pthreads appears to yield LinuxThreads, but BLCR supports only NPTL.]])
    cr_static_link_warning="yes"
    if test x"$enable_all_static" = xyes; then
      AC_CACHE_SAVE
      AC_MSG_ERROR([[static linking of BLCR to LinuxThreads is NOT supported.  Please try LDFLAGS='$ldsuggest' (or similar) on the configure command line to get NPTL.]])
    fi
  fi
fi

# Check for atfork handling
AC_CHECK_FUNCS([__register_atfork])

# Check for call to disable NSCD
AC_CHECK_FUNCS([__nss_disable_nscd])

# Check for pthread_attr_setstacksize
SAVE_LIBS="$LIBS"
LIBS="-lpthread $LIBS"
AC_CHECK_FUNCS([pthread_attr_setstacksize])
LIBS="$SAVE_LIBS"

# Look for prctl(PR_SET_PDEATHSIG, ...)
AC_CACHE_CHECK([[for pctrl()]], cr_cv_prctl, [
  AC_TRY_COMPILE([
    #include <sys/prctl.h>
  ], [
    int rc = prctl(PR_SET_PDEATHSIG, 0);
  ], [cr_cv_prctl=yes], [cr_cv_prctl=no])
])
CR_DEFINE(HAVE_PRCTL,[test x$cr_cv_prctl = xyes],
          [Define to 1 if prctl() function is available.])

# Check various constants needed for asm
SAVE_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -I${TOP_SRCDIR}/include"
CR_DEFINE_INT(CR_ASM_OP_HAND_CHKPT,
	[CR_OP_HAND_CHKPT],
	[#include <sys/ioctl.h>
	 #include "blcr_ioctl.h"])
CR_DEFINE_INT(CR_ASM_CHECKPOINT_STUB,
	[_CR_CHECKPOINT_STUB],
	[#include "blcr_common.h.in"])  # ICK!
CR_DEFINE_INT(CR_ASM_OP_HAND_ABORT,
	[CR_OP_HAND_ABORT],
	[#include <sys/ioctl.h>
	 #include "blcr_ioctl.h"])
CR_DEFINE_INT(CR_ASM_CHECKPOINT_OMIT,
	[CR_CHECKPOINT_OMIT],
	[#include "blcr_common.h.in"])  # ICK!
CR_DEFINE_INT(CR_ASM_SI_PID_OFFSET,
	[offsetof(siginfo_t, si_pid)],
	[#include <signal.h>
	 #include <stddef.h>
	 #ifndef offsetof
  	    #define offsetof(TYPE, MEMBER) ((unsigned long) &((TYPE *)0)->MEMBER)
	 #endif])
CR_DEFINE_INT(CR_ASM_NR_ioctl,[__NR_ioctl],
	[#include <asm/unistd.h>
	 #include <linux/unistd.h>])
CR_DEFINE_INT(CR_ASM_NR_rt_sigreturn,[__NR_rt_sigreturn],
	[#include <asm/unistd.h>
	 #include <linux/unistd.h>])
CFLAGS="$SAVE_CFLAGS"

# Probe for direction of stack growth
CR_STACK_DIRECTION

# Look for FTB header and libs
 cr_have_ftb=no
 if test x"$FTB_HOME" != xno; then
  cr_have_ftb=yes
  SAVE_CPPFLAGS="$CPPFLAGS"
  SAVE_LDFLAGS="$LDFLAGS"
  SAVE_LIBS="$LIBS"
  CR_FTB_INCLUDES=""
  CR_FTB_LDFLAGS=""
  AC_CACHE_CHECK([[for FTB headers]],cr_cv_ftb_incdir,[
    cr_result=no
    for cr_tmp in "" "${FTB_HOME}/include"; do
      CPPFLAGS="$SAVE_CPPFLAGS${cr_tmp:+ -I$cr_tmp}"
      AC_TRY_COMPILE([
        #include <libftb.h>
      ],[
        FTB_client_t x;
      ], [cr_result="$cr_tmp"; break])
    done
    if test -z "$cr_result"; then
      cr_cv_ftb_incdir="default"
    else
      cr_cv_ftb_incdir="$cr_result"
    fi
  ])
  if test x"$cr_cv_ftb_incdir" = xno; then
    cr_have_ftb=no
  elif test x"$cr_cv_ftb_incdir" != "xdefault"; then
    CR_FTB_INCLUDES="-I$cr_cv_ftb_incdir"
  fi
  AC_CACHE_CHECK([[for FTB libs]],cr_cv_ftb_libdir,[
    CPPFLAGS="$SAVE_CPPFLAGS $CR_FTB_INCLUDES"
    LIBS="-lftb -lpthread $SAVE_LIBS"
    cr_result=no
    for cr_tmp in "" "${FTB_HOME}/lib64" "${FTB_HOME}/lib32" "${FTB_HOME}/lib"; do
      LDFLAGS="$SAVE_LDFLAGS${cr_tmp:+-L$cr_tmp}"
      AC_TRY_LINK([
        #include <libftb.h>
      ],[
        return FTB_Connect(NULL, NULL);
      ], [cr_result="$cr_tmp"; break])
    done
    if test -z "$cr_result"; then
      cr_cv_ftb_libdir="default"
    else
      cr_cv_ftb_libdir="$cr_result"
    fi
  ])
  if test x"$cr_cv_ftb_libdir" = xno; then
    cr_have_ftb=no
  elif test x"$cr_cv_ftb_libdir" != "xdefault"; then
    CR_FTB_LDFLAGS="-L$cr_cv_ftb_libdir"
    CR_FTB_LDADD="-lftb"
  fi
  LIBS="$SAVE_LIBS"
  LDFLAGS="$SAVE_LDFLAGS"
  CPPFLAGS="$SAVE_CPPFLAGS"
  if test x"$cr_have_ftb$cr_force_ftb" = xnoyes; then
    AC_MSG_ERROR([Configured with --with-ftb='$FTB_HOME', but unable to build FTB test case])
  fi
  AC_SUBST([CR_FTB_INCLUDES])
  AC_SUBST([CR_FTB_LDFLAGS])
  AC_SUBST([CR_FTB_LDADD])
 fi
 CR_DEFINE(HAVE_FTB,[test x$cr_have_ftb = xyes],
            [Define to 1 if the Fault Tolerance Backplane is available.])

fi # cr_build_libcr

# Probe libc for the RT signal number to use
CR_CHECK_SIGNUM

# Check for 32- or 64-bit architecture
AC_CHECK_SIZEOF(void *, $cross_void_P)
CR_LIBARCH=$CR_ARCH
if test $ac_cv_sizeof_void_p != $cr_wordsize; then
  if test $cr_wordsize = 8; then
    CR_LIBARCH=${CR_LIBARCH32:-$CR_ARCH32}
  else
    AC_MSG_ERROR([CC='$CC' yields sizeof(void *) = $ac_cv_sizeof_void_p when expecting $cr_wordsize.$clue])
  fi
fi
AC_SUBST([CR_LIBARCH])

################################################################################
# Sub-configure for 32-bit libs on 64-bit platforms
################################################################################

cr_build_libcr32=no
if test x"$enable_multilib" = xyes; then
  if test x"$cr_build_libcr" != xyes; then
    AC_MSG_ERROR([--enable-multilib requested but not building libcr])
  elif test -f "${TOP_BUILDDIR}/configure.ac"; then # More robust than TOP_BUILDDIR == TOP_SRCDIR
    AC_MSG_ERROR([When requesting --enable-multilib, you must configure/build in a directory other than the one holding the BLCR sources])
  elif test -z "$CR_ARCH32"; then
    AC_MSG_ERROR([--enable-multilib not supported on architecture $CR_ARCH])
  elif test $ac_cv_sizeof_void_p = 4; then
    AC_MSG_ERROR([--enable-multilib requested but CC='$CC' generates 32-bit objects.  Perhaps try configuring with CC='$CC -m64' or similar.])
  elif test \! -d /lib64; then
    AC_MSG_ERROR([--enable-multilib requested but /lib64 does not exist])
  fi
  if test -z "$libdir32"; then
    if expr "$libdir" : '.*/lib64$' >/dev/null; then
      libdir32=`echo $libdir | sed -e 's/64$//'`
    elif test "$libdir" = '${exec_prefix}/lib'; then
      libdir32='${exec_prefix}/lib'
      libdir='${exec_prefix}/lib64'
    else
      AC_MSG_ERROR([--enable-multilib requested but libdir='$libdir' does not end in 'lib64']);
    fi
  fi
  SUBCMD="${TOP_SRCDIR}/configure '--srcdir=${TOP_SRCDIR}' '--build=${CR_ARCH32}-linux' '--libdir=${libdir32}' --with-components=libcr,tests,examples"
  eval set -- $ac_configure_args
  for x in "$@"; do
    case "$x" in
      --host=*|--build=*|--target=*|host_alias=*|build_alias=*|target_alias=*|--libdir=*|--srcdir=*|--with-components=*|--enable-multilib|--enable-testsuite|--enable-init-script|CFLAGS=*|CXXFLAGS=*)
	# skip
	;;
      *)
	# keep as-is
	SUBCMD="$SUBCMD '$x'"
	;;
    esac
  done
  # Strip -m64 out of CFLAGS and CXXFLAGS, if present
  if test -n "$CFLAGS32"; then
    SUBCMD="$SUBCMD 'CFLAGS=$CFLAGS32'"
  elif test -n "$CFLAGS"; then
    set --
    for x in $CFLAGS; do test x"$x" = "x-m64" || set -- "$@" "$x"; done
    SUBCMD="$SUBCMD 'CFLAGS=$@'"
  fi
  if test -n "$CXXFLAGS32"; then
    SUBCMD="$SUBCMD 'CXXFLAGS=$CXXFLAGS32'"
  elif test -n "$CXXFLAGS"; then
    set --
    for x in $CXXFLAGS; do test x"$x" = "x-m64" || set -- "$@" "$x"; done
    SUBCMD="$SUBCMD 'CXXFLAGS=$@'"
  fi
  # Strip -m64 from definitions of CC and CXX, adding -m32
  if test -n "$CC32"; then
    SUBCMD="$SUBCMD 'CC=$CC32'"
  else
    set --
    for x in $CC; do test x"$x" = "x-m64" || set -- "$@" "$x"; done
    SUBCMD="$SUBCMD 'CC=$@ -m32'"
  fi
  if test -n "$CXX32"; then
    SUBCMD="$SUBCMD 'CXX=$CXX32'"
  elif test -n "$CXX" -a "$CXX" != no; then
    set --
    for x in $CXX; do test x"$x" = "x-m64" || set -- "$@" "$x"; done
    SUBCMD="$SUBCMD 'CXX=$@ -m32'"
  fi
  # Look for 32-bit versions of remaining bin tools
  if test -n "$CPP32"; then
    SUBCMD="$SUBCMD 'CPP=$CPP32'"
  fi
  if test -n "$LD32"; then
    SUBCMD="$SUBCMD 'LD=$LD32'"
  fi
  if test -n "$AR32"; then
    SUBCMD="$SUBCMD 'AR=$AR32'"
  fi
  if test -n "$NM32"; then
    SUBCMD="$SUBCMD 'NM=$NM32'"
  fi
  if test -n "$RANLIB32"; then
    SUBCMD="$SUBCMD 'RANLIB=$RANLIB32'"
  fi
  # Others?
  AC_MSG_NOTICE([>>>> BEGIN sub-configure for 32-bit libs >>>>])
  AC_MSG_NOTICE([${SUBCMD}])
  ( mkdir -p libcr32 && cd libcr32 && ln -sf ../util . && eval "${SUBCMD}" )
  if test "$?" = 0; then
    AC_MSG_NOTICE([<<<< END sub-configure for 32-bit libs <<<<])
    cr_build_libcr32=yes
  else
    rm -rf libcr32
    AC_MSG_ERROR([--enable-multilib requested but FAILED sub-configure for 32-bit libs])
  fi
fi
AM_CONDITIONAL(CR_BUILD_LIBCR32,[test x$cr_build_libcr32 = xyes])

################################################################################
# Check kernel (source location, symbol table, features, etc.)
################################################################################

if test x"$cr_build_modules" = xyes; then

# Compiler to build kernel modules
AC_MSG_CHECKING([compiler to build kernel modules])
if test -n "${KCC}"; then
  AC_MSG_RESULT([$KCC (user setting)])
else
  KCC=$CC
  AC_MSG_RESULT([$KCC (default)])
fi
AC_SUBST([KCC])

# Find BSD-compatible NM command
AC_PROG_NM
AC_SUBST([NM],$NM)

# First deal with user-specified kernel type (UP, SMP, BIGMEM, etc.) if any.
case x"$with_kernel_type" in
  x|xyes|xno) ;;
  *) CR_KERNEL_TYPE("$with_kernel_type")
esac
# Checks for required kernel src/obj dirs and fail if they are not found 
CR_CHECK_LINUX()
AS_IF([test -z "${HAVE_LINUX_2_6}" -a -z "${HAVE_LINUX_3}"],
  [AC_CACHE_SAVE
   AC_MSG_ERROR([[required Linux 2.6.x or 3.x.y kernel headers and/or build not found.]])])

# Module install dir
AC_SUBST([CR_MODULE_DIR])
AC_MSG_CHECKING([directory to install kernel modules])
if test -n "${CR_MODULE_DIR}"; then
  cr_why='passed via --with-kmod-dir'
elif test ${sysconfdir} = /etc; then
  CR_MODULE_DIR="/lib/modules/${LINUX_VER}/extra"
  cr_why='default when sysconfdir=/etc'
else
  CR_MODULE_DIR="${libdir}/${PACKAGE}/${LINUX_VER}"
  cr_why='default'
fi
AC_MSG_RESULT([${CR_MODULE_DIR} ($cr_why)])

# Checks for matching (we hope) symbol table
CR_LINUX_SYMTAB

# Setup CR_KARCH to enable 32-bit user-space on 64-bit kernels
CR_KARCH=$CR_ARCH
if test $cr_wordsize = 4; then
  # Count pointer hex digits in the System.map to id a 64-bit kernel
  # Based on logic provided by Alan Woodland
  AC_PROG_EGREP
  AC_MSG_CHECKING([if kernel and user-space wordsize match])
  if eval $LINUX_SYMTAB_CMD | head | $EGREP '^[[a-f0-9]]{16}' >/dev/null 2>&1;then
    AC_MSG_RESULT([no (32-bit user-space with 64-bit kernel)])
    case $CR_KARCH in
      i386)  CR_KARCH=x86_64;;
      ppc)   CR_KARCH=ppc64;;
      sparc) CR_KARCH=sparc64;;
    esac
  else
    AC_MSG_RESULT([yes])
  fi
fi
AC_SUBST(CR_KARCH)

# Configure automake/kbuild glue
CR_CHECK_KBUILD

# Configure module compile flags and perform a sanity check
CR_SET_KCFLAGS

# Now we can check if SYMTAB matches the kernel
CR_LINUX_SYMTAB_VALIDATE

## Check for optional kernel headers...
CR_CHECK_KERNEL_HEADER([linux/syscalls.h])
CR_CHECK_KERNEL_HEADER([linux/pspace.h])
CR_CHECK_KERNEL_HEADER([linux/pid_namespace.h])
CR_CHECK_KERNEL_HEADER([linux/lockdep.h])
CR_CHECK_KERNEL_HEADER([linux/compile.h])
CR_CHECK_KERNEL_HEADER([linux/fdtable.h])
CR_CHECK_KERNEL_HEADER([linux/utrace.h])
CR_CHECK_KERNEL_HEADER([linux/perf_event.h])
CR_CHECK_KERNEL_HEADER([linux/audit.h])
CR_CHECK_KERNEL_HEADER([asm/elf.h])
CR_CHECK_KERNEL_HEADER([asm/desc.h],[#include <linux/sched.h>])
CR_CHECK_KERNEL_HEADER([asm/vsyscall32.h])
CR_CHECK_KERNEL_HEADER([asm/i387.h])
CR_CHECK_KERNEL_HEADER([asm/switch_to.h],[#include <linux/sched.h>])
CR_CHECK_KERNEL_HEADER([asm/tls.h],[#include <linux/sched.h>])

CR_CHECK_KERNEL_HEADER([linux/utsrelease.h])
CR_CHECK_KERNEL_HEADER([generated/utsrelease.h])

## Now we check for specific kernel features...

CR_FIND_KSYM([timeval_to_jiffies],[CODE])
CR_FIND_KSYM([jiffies_to_timeval],[CODE])

CR_CHECK_KERNEL_TYPE([struct fdtable],[
    #include <linux/sched.h>
    #include <linux/file.h>
    #if HAVE_LINUX_FDTABLE_H
      #include <linux/fdtable.h>
    #endif
  ])

if test -n "${HAVE_STRUCT_FDTABLE}"; then
  CR_CHECK_KERNEL_MEMBER([open_fds.fds_bits],[
      #include <linux/sched.h>
      #include <linux/file.h>
      #if HAVE_LINUX_FDTABLE_H
        #include <linux/fdtable.h>
      #endif
    ],[struct fdtable],[unsigned long *],[open_fds->fds_bits])
  CR_CHECK_KERNEL_CALL([fd_is_open],[
      #include <linux/sched.h>
      #include <linux/file.h>
      #if HAVE_LINUX_FDTABLE_H
        #include <linux/fdtable.h>
      #endif
    ])
  CR_CHECK_KERNEL_CALL([__set_open_fd],[
      #include <linux/sched.h>
      #include <linux/file.h>
      #if HAVE_LINUX_FDTABLE_H
        #include <linux/fdtable.h>
      #endif
    ])
  CR_CHECK_KERNEL_CALL([close_on_exec],[
      #include <linux/sched.h>
      #include <linux/file.h>
      #if HAVE_LINUX_FDTABLE_H
        #include <linux/fdtable.h>
      #endif
    ])
  CR_CHECK_KERNEL_CALL([__set_close_on_exec],[
      #include <linux/sched.h>
      #include <linux/file.h>
      #if HAVE_LINUX_FDTABLE_H
        #include <linux/fdtable.h>
      #endif
    ])
  # OK if those are missing
else
  CR_CHECK_KERNEL_MEMBER([open_fds.fds_bits],[
      #include <linux/sched.h>
      #include <linux/file.h>
      #if HAVE_LINUX_FDTABLE_H
        #include <linux/fdtable.h>
      #endif
    ],[struct files_struct],[unsigned long *],[open_fds->fds_bits])
fi

CR_CHECK_KERNEL_MEMBER([files_struct.next_fd],[
    #include <linux/sched.h>
    #include <linux/file.h>
    #if HAVE_LINUX_FDTABLE_H
      #include <linux/fdtable.h>
    #endif
  ],[struct files_struct],[int],[next_fd])
CR_CHECK_KERNEL_MEMBER([fdtable.next_fd],[
    #include <linux/sched.h>
    #include <linux/file.h>
    #if HAVE_LINUX_FDTABLE_H
      #include <linux/fdtable.h>
    #endif
  ],[struct fdtable],[int],[next_fd])
# Allow only exactly one match
if test "${HAVE_FILES_STRUCT_NEXT_FD}${HAVE_FDTABLE_NEXT_FD}" != "1"; then
  CR_BAD_KERNEL([unable to determine where to find next_fd])
fi

CR_CHECK_KERNEL_MEMBER([files_struct.max_fdset],[
    #include <linux/sched.h>
    #include <linux/file.h>
    #if HAVE_LINUX_FDTABLE_H
      #include <linux/fdtable.h>
    #endif
  ],[struct files_struct],[int],[max_fdset])

CR_CHECK_KERNEL_TYPE([struct path],[#include <linux/fs.h>])
CR_CHECK_KERNEL_COMPILE([nameidata.path],[
    #include <linux/sched.h>
    #include <linux/fs.h>
    #include <linux/dcache.h>
    #include <linux/namei.h>
    #include <linux/fs_struct.h>
  ],[
    /* Expect "struct path pwd" in fs_struct */
    struct fs_struct s1;
    struct path t1 = s1.pwd;

    /* Expect "struct path path" used in nameidata */
    struct nameidata s2;
    struct path t2 = s2.path;

    /* Expect d_path() and set_fs_pwd() to pass path */
    (void)d_path(&t1, NULL, 0);
    (void)set_fs_pwd(&s1, &t2);
  ],[Define to 1 if fs functions pass 'struct path'])
CR_CHECK_KERNEL_COMPILE([nameidata.dentry],[
    #include <linux/sched.h>
    #include <linux/fs.h>
    #include <linux/dcache.h>
    #include <linux/namei.h>
    #include <linux/fs_struct.h>
  ],[
    /* Expect "struct dentry *pwd" in fs_struct */
    struct fs_struct s1;
    struct dentry t1 = *s1.pwd;

    /* Expect "struct dentry *dentry" in nameidata */
    struct nameidata s2;
    struct dentry t2 = *s2.dentry;

    /* Expect d_path() and set_fs_pwd() to pass dentry */
    (void)d_path(&t1, NULL, NULL, 0);
    (void)set_fs_pwd(&s1, NULL, &t2);
  ])
if test "${HAVE_NAMEIDATA_PATH}${HAVE_NAMEIDATA_DENTRY}" != "1"; then
  CR_BAD_KERNEL([unable to determine composition of struct nameidata])
fi
CR_CHECK_KERNEL_MEMBER([file.f_path],[#include <linux/fs.h>],
		       [struct file],[struct path],[f_path])

CR_CHECK_KERNEL_CALL([path_lookup],[#include <linux/namei.h>])
CR_CHECK_KERNEL_CALL([kern_path],[#include <linux/namei.h>])
# Require 1 or more match
if test -z "${HAVE_PATH_LOOKUP}{HAVE_KERN_PATH}"; then
  CR_BAD_KERNEL([unable to find either path_lookup() or kern_path()])
fi

CR_CHECK_KERNEL_CALL([__putname],[#include <linux/fs.h>])

CR_CHECK_KERNEL_CALL_NARGS([vfs_getattr],[#include <linux/fs.h>],
  [NULL,NULL],[NULL,NULL,NULL])

CR_CHECK_KERNEL_TYPE([struct delayed_work],[#include <linux/workqueue.h>])

# Order for "best" match
CR_CHECK_KERNEL_MACRO([do_each_pid_task],[#include <linux/sched.h>])
CR_CHECK_KERNEL_MACRO([do_each_task_pid],[#include <linux/sched.h>])
if test -z "${HAVE_DO_EACH_TASK_PID}"; then
  CR_CHECK_KERNEL_MACRO([for_each_task_pid],[#include <linux/sched.h>])
fi
# Require 1 or more match
if test -z "${HAVE_DO_EACH_PID_TASK}${HAVE_DO_EACH_TASK_PID}${HAVE_FOR_EACH_TASK_PID}"; then
  CR_BAD_KERNEL([unable to find a task iterator])
fi

CR_CHECK_KERNEL_MEMBER([2.6.0 task ids],[#include <linux/sched.h>],
  [struct task_struct],[int],[__pgrp])
CR_CHECK_KERNEL_MEMBER([2.6.6 task ids],[#include <linux/sched.h>],
  [struct task_struct],[int],[signal->session])
CR_CHECK_KERNEL_CALL_FULL([2.6.20 task ids],[process_session],[#include <linux/sched.h>],[int],[NULL])
CR_CHECK_KERNEL_CALL_FULL([2.6.24 task ids],[task_session_vnr],[#include <linux/sched.h>],[int],[NULL])
# Should allow only exactly one match, except that 2.6.6 and 2.6.20/24 overlap during transitional period
case "${HAVE_2_6_0_TASK_IDS}X${HAVE_2_6_6_TASK_IDS}X${HAVE_2_6_20_TASK_IDS}${HAVE_2_6_24_TASK_IDS}" in
    1XX|X1X|X1X1|XX1) # OK
	;;
    *)	CR_BAD_KERNEL([can't find pgrp/session of a task_struct])
	;;
esac
CR_CHECK_KERNEL_CALL([set_task_pgrp],[#include <linux/sched.h>])
CR_CHECK_KERNEL_CALL([set_task_session],[#include <linux/sched.h>])

CR_CHECK_KERNEL_CALL([valid_signal],[#include <linux/sched.h>])

CR_CHECK_KERNEL_MACRO([REMOVE_LINKS],[#include <linux/sched.h>])

CR_CHECK_KERNEL_MEMBER([task.pids pid_link],[#include <linux/sched.h>],
    [struct task_struct], [struct pid_link], [pids[0]])
CR_CHECK_KERNEL_MEMBER([task.pids pid],[#include <linux/sched.h>],
    [struct task_struct], [struct pid], [pids[0]])
# Allow zero or one match
if test "${HAVE_TASK_PIDS_PID_LINK}${HAVE_TASK_PIDS_PID}" = "11"; then
  CR_BAD_KERNEL([unable to determine type of task.pids array])
fi

CR_FIND_KSYM([signal_wake_up],[CODE],
             [extern void signal_wake_up(struct task_struct *, int);])
CR_FIND_KSYM([signal_wake_up_state],[CODE],
             [extern void signal_wake_up_state(struct task_struct *, int);])

# put_task_struct() requires one of these:
CR_FIND_KSYM([__put_task_struct],[CODE],[extern void __put_task_struct(struct task_struct *);])
CR_FIND_KSYM([__put_task_struct_cb],[CODE])

CR_CHECK_KERNEL_MEMBER([mm.task_size],[#include <linux/sched.h>],
  [struct mm_struct],[unsigned long],[task_size])
CR_CHECK_KERNEL_MEMBER([mm.exe_file],[
	#include <linux/mm_types.h>
	#include <linux/fs.h>],
  [struct mm_struct],[struct file *],[exe_file])
CR_CHECK_KERNEL_MEMBER([vma.vm_mm],[#include <linux/mm_types.h>],
  [struct vm_area_struct],[struct mm_struct *],[vm_mm])

CR_CHECK_KERNEL_CALL([PageAnon],[#include <linux/mm.h>])

# Look for various VDSO-related bits (arch and version dependent)
CR_CHECK_KERNEL_MEMBER([thread.vdso_base],[#include <asm/processor.h>],
  [struct thread_struct],[unsigned long],[vdso_base])
CR_CHECK_KERNEL_MEMBER([mm.context.vdso_base],[#include <linux/sched.h>],
  [mm_context_t],[unsigned long],[vdso_base])
CR_CHECK_KERNEL_MEMBER([mm.context.vdso],[#include <linux/sched.h>],
  [mm_context_t],[unsigned long],[vdso])
CR_CHECK_KERNEL_MEMBER([thread_info.sysenter_return],[#include <linux/thread_info.h>],
  [struct thread_info],[void *],[sysenter_return])
if test $CR_KARCH = arm; then
  CR_CHECK_KERNEL_MEMBER([mm.context.sigpage],[#include <linux/sched.h>],
    [mm_context_t],[unsigned long],[sigpage])
  CR_CHECK_KERNEL_MEMBER([thread_info.tp_value],[#include <linux/thread_info.h>],
    [struct thread_info],[unsigned long],[tp_value])
  pushdef([cr_cvname],cr_cv_kconfig_HAVE_THREAD_INFO_TP_VALUE_ARRAY)[]dnl
  AC_CACHE_CHECK([kernel thread_info.tp_value as array],cr_cvname,[
    CR_TRY_KERNEL_COMPILE([
	#include <linux/thread_info.h>
    ],[
	struct thread_info x;
	unsigned long i = x.tp_value[1];
    ],[cr_cvname=yes],[cr_cvname=no])
  ])
  CR_DEFINE(HAVE_THREAD_INFO_TP_VALUE_ARRAY,[test x$]cr_cvname[ = xyes],
    [Define to 1 if struct thread_info.tp_value has array member 'tp_value'])
  popdef([cr_cvname])[]dnl
  CR_FIND_KSYM([__kuser_helper_start],[CODE],[extern int __kuser_helper_start(void);])
  CR_FIND_KSYM([__kuser_cmpxchg],[CODE],[extern int __kuser_cmpxchg(void);])
fi
CR_CHECK_KERNEL_CONSTANT([VSYSCALL_BASE],[#include <asm/elf.h>])
CR_CHECK_KERNEL_CALL_NARGS([arch_setup_additional_pages],[
    #include <linux/sched.h>
    #include <asm/elf.h>
  ],[NULL,0],[NULL,0,0,0])
if test -n "${HAVE_2_ARG_ARCH_SETUP_ADDITIONAL_PAGES}${HAVE_4_ARG_ARCH_SETUP_ADDITIONAL_PAGES}"; then
  CR_FIND_KSYM([arch_setup_additional_pages],[CODE])
fi
CR_CHECK_KERNEL_CALL([map_vsyscall],[
    #include <linux/sched.h>
    #include <asm/elf.h>
    #ifndef __HAVE_ARCH_VSYSCALL
      choke me
    #endif])
if test -n "${HAVE_MAP_VSYSCALL}"; then
  CR_FIND_KSYM([map_vsyscall],[CODE])
fi
case $CR_KARCH in
x86_64)
  CR_FIND_KSYM([syscall32_setup_pages],[CODE],
	[extern int syscall32_setup_pages(void *, int);])
  CR_FIND_KSYM([compat_arch_setup_additional_pages],[CODE],
	[extern int compat_arch_setup_additional_pages(void *, int);])
  ;;
arm)
  CR_FIND_KSYM([vectors_user_mapping],[CODE],
	[extern int vectors_user_mapping(void);])
  ;;
esac

pushdef([cr_cvname],cr_cv_kconfig_HAVE_VM_OPS_NAME)[]dnl
AC_CACHE_CHECK([kernel for vm_operations_struct.name],cr_cvname,[
  CR_TRY_KERNEL_COMPILE([
	#include <linux/sched.h>
	#include <linux/mm.h>
    ],[
	struct vm_operations_struct x;
	const char *(*name)(struct vm_area_struct *vma) = x.name;
    ],[cr_cvname=yes],[cr_cvname=no])
])
popdef([cr_cvname])[]dnl
CR_DEFINE(HAVE_VM_OPS_NAME,[test x$cr_cv_kconfig_HAVE_VM_OPS_NAME = xyes],
	    [Define to 1 if struct vm_operations_struct has member 'name'])

CR_FIND_KSYM([arch_pick_mmap_layout],[CODE])
CR_FIND_KSYM([arch_get_unmaped_area],[CODE])
CR_FIND_KSYM([arch_unmap_area],[CODE])
CR_CHECK_KERNEL_MEMBER([mm.mmap_base],[#include <linux/sched.h>],
	[struct mm_struct],[unsigned long],[mmap_base])
CR_CHECK_KERNEL_MEMBER([mm.cached_hole_size],[#include <linux/sched.h>],
	[struct mm_struct],[unsigned long],[cached_hole_size])
CR_CHECK_KERNEL_MEMBER([mm.free_area_cache],[#include <linux/sched.h>],
	[struct mm_struct],[unsigned long],[free_area_cache])

CR_FIND_KSYM([do_munmap],[CODE])
CR_CHECK_KERNEL_CALL([do_mmap],[#include <linux/mm.h>])
CR_CHECK_KERNEL_CALL([do_mmap_pgoff],[#include <linux/mm.h>])

CR_CHECK_KERNEL_CALL_NARGS([do_mmap_pgoff],[#include <linux/mm.h>],
  [NULL,0,0,0,0,0],[NULL,0,0,0,0,0,NULL])
if test -n "${HAVE_6_ARG_DO_MMAP_PGOFF}${HAVE_7_ARG_DO_MMAP_PGOFF}"; then
  CR_FIND_KSYM([do_mmap_pgoff],[CODE])
elif test -n "${HAVE_DO_MMAP}"; then
  CR_FIND_KSYM([do_mmap],[CODE])
else
  CR_BAD_KERNEL([could not locate either do_mmap() or do_mmap_pgoff()])
fi

CR_CHECK_KERNEL_MACRO([CONFIG_HUGETLBFS])
if test -n "${HAVE_CONFIG_HUGETLBFS}"; then
  CR_FIND_KSYM([hugetlbfs_file_operations],[DATA],
	[extern const struct file_operations hugetlbfs_file_operations;])
  CR_FIND_KSYM([is_file_shm_hugepages],[CODE],
	[extern int is_file_shm_hugepages(struct file *file);])
  CR_FIND_KSYM([hugepage_vma],[CODE],
	[extern struct vm_area_struct *hugepage_vma(struct mm_struct *, unsigned long);])
  CR_FIND_KSYM([pmd_huge],[CODE],
	[extern int pmd_huge(pmd_t pmd);])
  CR_FIND_KSYM([follow_huge_pmd],[CODE],
	[extern struct page * follow_huge_pmd(struct mm_struct *, unsigned long  pmd_t *, int);])
  CR_FIND_KSYM([follow_huge_addr],[CODE]) # Must have prototype, or NARGS probe will fail
  CR_CHECK_KERNEL_CALL_NARGS([follow_huge_addr],[#include <linux/mm.h>
						 #include <linux/hugetlb.h>],[NULL,0,0],[NULL,NULL,0,0])
  if test "${HAVE_3_ARG_FOLLOW_HUGE_ADDR}${HAVE_4_ARG_FOLLOW_HUGE_ADDR}" != "1"; then
    CR_BAD_KERNEL([CONFIG_HUGETLBFS set, but unable to determine how invoke follow_huge_addr()])
  fi
else
  # Avoid gripes from gcc
  AC_DEFINE([HAVE_3_ARG_FOLLOW_HUGE_ADDR], [0])
  AC_DEFINE([HAVE_4_ARG_FOLLOW_HUGE_ADDR], [0])
fi

# Import the correct symbol for arch-specific ZERO_PAGE(),
# which must have a prototype in asm/pgtable.h (arch-specific)
# If we support ia64 or mips in the future we'll need to add more cases here.
# NOTE: don't try to factor the CR_FIND_KSYM() call; it requires a literal.
case $CR_KARCH in
  sparc*)
    CR_FIND_KSYM([mem_map_zero],[DATA])
    ZERO_PAGE_SYMBOL=mem_map_zero
    ;;
  *)
    CR_FIND_KSYM([empty_zero_page],[DATA])
    ZERO_PAGE_SYMBOL=empty_zero_page
    ;;
esac
if test -z "$cr_addr"; then
  AC_MSG_ERROR([failed to find symbol $ZERO_PAGE_SYMBOL])
fi

# How to check for permission
CR_CHECK_KERNEL_CALL([inode_permission],[#include <linux/fs.h>])
CR_CHECK_KERNEL_CALL([permission],[#include <linux/fs.h>])
if test -z "${HAVE_INODE_PERMISSION}${HAVE_PERMISSION}"; then
  CR_BAD_KERNEL([unable to determine how to invoke permission() or inode_permission])
fi

# Bits for save/restore of pending signals
CR_FIND_KSYM([copy_siginfo_to_user],[CODE],
	[extern int copy_siginfo_to_user(siginfo_t *to, siginfo_t *from);])
CR_FIND_KSYM([group_send_sig_info],[CODE],
	[extern int group_send_sig_info(int, struct siginfo *, struct task_struct *);])

# For save/restore of sigaltstack
CR_FIND_KSYM([do_sigaltstack],[CODE],
	[extern int do_sigaltstack(const stack_t *, stack_t *, unsigned long);])

if test $CR_KARCH = i386 -o $CR_KARCH = x86_64; then
  CR_CHECK_KERNEL_TYPE([struct n_desc_struct],[
	#include <linux/sched.h>
	#include <asm/desc.h>])
  CR_CHECK_KERNEL_SYMBOL([cpu_gdt_descr],[#include <linux/sched.h>
					  #include <asm/desc.h>])
  if test -n "${HAVE_CPU_GDT_DESCR}"; then
    CR_FIND_KSYM([cpu_gdt_descr],[DATA])
  fi
  CR_FIND_KSYM([cpu_gdt_table],[DATA])
  CR_FIND_KSYM([per_cpu__cpu_gdt_descr],[DATA])

  CR_CHECK_KERNEL_MEMBER([thread.sp0],[#include <asm/processor.h>],
	[struct thread_struct],[unsigned long],[sp0])
  CR_CHECK_KERNEL_MEMBER([thread.esp0],[#include <asm/processor.h>],
	[struct thread_struct],[unsigned long],[esp0])
  CR_CHECK_KERNEL_MEMBER([thread.rsp0],[#include <asm/processor.h>],
	[struct thread_struct],[unsigned long],[rsp0])
  CR_CHECK_KERNEL_MEMBER([thread.usersp],[#include <asm/processor.h>],
	[struct thread_struct],[unsigned long],[usersp])
  CR_CHECK_KERNEL_MEMBER([thread.userrsp],[#include <asm/processor.h>],
	[struct thread_struct],[unsigned long],[userrsp])
  CR_CHECK_KERNEL_MEMBER([thread.debugreg0],[#include <asm/processor.h>],
	[struct thread_struct],[unsigned long],[debugreg0])
  CR_CHECK_KERNEL_MEMBER([thread.debugregs],[#include <asm/processor.h>],
	[struct thread_struct],[unsigned long],[debugreg[1]])
  CR_CHECK_KERNEL_MEMBER([thread.ptrace_bps],[
	#include <linux/perf_event.h>
	#include <asm/processor.h>],
	[struct thread_struct],[struct perf_event *],[ptrace_bps[1]])
  CR_CHECK_KERNEL_MEMBER([thread.i387],[#include <asm/processor.h>],
	[struct thread_struct],[union i387_union],[i387])
  CR_CHECK_KERNEL_MEMBER([thread.xstate],[#include <asm/processor.h>],
	[struct thread_struct],[union thread_xstate *],[xstate])
  CR_CHECK_KERNEL_MEMBER([thread.fpu],[#include <asm/processor.h>],
	[struct thread_struct],[struct fpu],[fpu])

  CR_CHECK_KERNEL_MEMBER([pt_regs.eflags],[#include <asm/ptrace.h>],[struct pt_regs],[unsigned long],[eflags])
  CR_CHECK_KERNEL_MEMBER([pt_regs.flags],[#include <asm/ptrace.h>],[struct pt_regs],[unsigned long],[flags])

  CR_FIND_KSYM([init_fpu],[CODE],[extern void init_fpu(struct task_struct *);])
  CR_FIND_KSYM([xstate_size],[DATA],[extern unsigned int xstate_size;])

  CR_CHECK_KERNEL_CALL([restore_fpu_checking],[#include <asm/i387.h>])
  if test -n "${HAVE_RESTORE_FPU_CHECKING}"; then
    CR_CHECK_KERNEL_DECL([2.6.0 restore_fpu_checking],[restore_fpu_checking],
     [#include <asm/i387.h>],
     [int restore_fpu_checking(struct i387_fxsave_struct *);])
    CR_CHECK_KERNEL_DECL([2.6.28 restore_fpu_checking],[restore_fpu_checking],
     [#include <asm/i387.h>],
     [int restore_fpu_checking(struct task_struct *);])
    if test "${HAVE_2_6_0_RESTORE_FPU_CHECKING}${HAVE_2_6_28_RESTORE_FPU_CHECKING}" != "1"; then
      CR_BAD_KERNEL([unable to determine how to call restore_fpu_checking()])
    fi
  fi
fi

if test $CR_KARCH = i386; then
  CR_CHECK_KERNEL_MEMBER([pt_regs.xcs],[#include <asm/ptrace.h>],[struct pt_regs],[int],[xcs])
  CR_CHECK_KERNEL_MEMBER([pt_regs.xfs],[#include <asm/ptrace.h>],[struct pt_regs],[int],[xfs])
  CR_CHECK_KERNEL_MEMBER([pt_regs.xgs],[#include <asm/ptrace.h>],[struct pt_regs],[int],[xgs])
  CR_CHECK_KERNEL_MEMBER([pt_regs.cs],[#include <asm/ptrace.h>],[struct pt_regs],[int],[cs])
  CR_CHECK_KERNEL_MEMBER([pt_regs.fs],[#include <asm/ptrace.h>],[struct pt_regs],[int],[fs])
  CR_CHECK_KERNEL_MEMBER([pt_regs.gs],[#include <asm/ptrace.h>],[struct pt_regs],[int],[gs])
fi

if test $CR_KARCH = ppc -o $CR_KARCH = ppc64; then
  CR_CHECK_KERNEL_MEMBER([thread.fpr],[#include <linux/sched.h>],
			 [struct thread_struct],[void*],[fpr])
  CR_CHECK_KERNEL_MEMBER([thread.vr],[#include <linux/sched.h>],
			 [struct thread_struct],[void*],[vr])
  CR_CHECK_KERNEL_MEMBER([thread.fp_state],[#include <linux/sched.h>],
			 [struct thread_struct],[struct thread_fp_state],[fp_state])
  CR_CHECK_KERNEL_MEMBER([thread.vr_state],[#include <linux/sched.h>],
			 [struct thread_struct],[struct thread_vr_state],[vr_state])
fi

CR_CHECK_KERNEL_MEMBER([inode sem],[
    #include <linux/sched.h>
    #include <linux/fs.h>
  ],[struct inode],[struct semaphore],[i_sem])
CR_CHECK_KERNEL_MEMBER([inode mutex],[
    #include <linux/sched.h>
    #include <linux/fs.h>
    #include <linux/mutex.h>
  ],[struct inode],[struct mutex],[i_mutex])
# Allow exactly one match
if test "${HAVE_INODE_SEM}${HAVE_INODE_MUTEX}" != "1"; then
  CR_BAD_KERNEL([unrecognized lock type for struct inode])
fi

CR_CHECK_KERNEL_MEMBER([mm.dumpable],[#include <linux/sched.h>],
  [struct mm_struct],[int],[dumpable])
CR_CHECK_KERNEL_CALL([get_dumpable],[#include <linux/sched.h>])
if test -n "${HAVE_GET_DUMPABLE}"; then
  # Note: may be an inline in some kernels
  CR_FIND_KSYM([get_dumpable],[CODE],[extern int get_dumpable(struct mm_struct *mm);])
fi
# Allow one or more match(es)
if test -z "${HAVE_MM_DUMPABLE}${HAVE_GET_DUMPABLE}"; then
  CR_BAD_KERNEL([unable to determine how to identify non-dumpable tasks])
fi

CR_CHECK_KERNEL_MEMBER([task.rlim],[#include <linux/sched.h>],
  [struct task_struct],[struct rlimit],[rlim[0]])
CR_CHECK_KERNEL_MEMBER([signal.rlim],[#include <linux/sched.h>],
  [struct task_struct],[struct rlimit],[signal->rlim[0]])
# Allow exactly one match
if test "${HAVE_TASK_RLIM}${HAVE_SIGNAL_RLIM}" != "1"; then
  CR_BAD_KERNEL([unable to determine location of rlim structure])
fi

CR_CHECK_KERNEL_MEMBER([task.cred],[#include <linux/sched.h>],
  [struct task_struct],[const struct cred *],[cred])
if test -n "${HAVE_TASK_CRED}"; then
  CR_CHECK_KERNEL_CALL_NARGS([dentry_open],[#include <linux/fs.h>],
	[NULL,0,NULL],[NULL,NULL,0,NULL])
fi
CR_CHECK_KERNEL_CALL([uid_eq],[#include <linux/sched.h>])
CR_CHECK_KERNEL_CALL([from_kuid],[#include <linux/sched.h>])

CR_CHECK_KERNEL_MEMBER([task.group_info],[#include <linux/sched.h>],
  [struct task_struct],[struct group_info *],[group_info])
CR_CHECK_KERNEL_SYMBOL([suid_dumpable],[#include <linux/binfmts.h>])
if test -n "${HAVE_SUID_DUMPABLE}"; then
  CR_FIND_KSYM([suid_dumpable],[DATA])
fi
CR_CHECK_KERNEL_CALL([set_dumpable],[#include <linux/sched.h>])
if test -n "${HAVE_SET_DUMPABLE}"; then
  CR_FIND_KSYM([set_dumpable],[CODE])
fi
CR_FIND_KSYM([groups_search],[CODE],[extern int groups_search(struct group_info *, gid_t);])
CR_FIND_KSYM([supplemental_group_member],[CODE],[extern int supplemental_group_member(gid_t);])

CR_CHECK_KERNEL_CONSTANT([PIDTYPE_TGID],[#include <linux/sched.h>])
CR_CHECK_KERNEL_MEMBER([task.thread_group],[#include <linux/sched.h>],
  [struct task_struct],[struct list_head],[thread_group])
if test "${HAVE_PIDTYPE_TGID}${HAVE_TASK_THREAD_GROUP}" != "1"; then
  CR_BAD_KERNEL([unable to determine thread group list])
fi

CR_FIND_KSYM([detach_pid],[CODE])
CR_FIND_KSYM([attach_pid],[CODE])
CR_FIND_KSYM([change_pid],[CODE])
CR_FIND_KSYM([find_pid],[CODE])
CR_FIND_KSYM([link_pid],[CODE])

CR_CHECK_KERNEL_CALL_NARGS([find_pid],[#include <linux/sched.h>],[0],[0,0])
CR_CHECK_KERNEL_CALL([find_pid_ns],[#include <linux/sched.h>])
if test -z "${HAVE_1_ARG_FIND_PID}${HAVE_2_ARG_FIND_PID}${HAVE_FIND_PID_NS}"; then
  CR_BAD_KERNEL([unable to determine how to map pid_nr -> struct pid])
fi

CR_CHECK_KERNEL_CALL([find_task_by_pid],[#include <linux/sched.h>])
CR_CHECK_KERNEL_CALL([find_task_by_pid_ns],[#include <linux/sched.h>])
if test -z "${HAVE_FIND_TASK_BY_PID}${HAVE_FIND_TASK_BY_PID_NS}"; then
  CR_BAD_KERNEL([unable to determine how to map pid_nr -> struct task])
fi
CR_FIND_KSYM([find_task_by_pid_ns],[CODE])

CR_CHECK_KERNEL_CALL([find_task_by_pid_type],[#include <linux/sched.h>])
CR_CHECK_KERNEL_CALL([find_task_by_pid_type_ns],[#include <linux/sched.h>])
if test -z "${HAVE_2_ARG_FIND_PID}${HAVE_FIND_TASK_BY_PID}${HAVE_FIND_TASK_BY_PID_NS}"; then
  CR_BAD_KERNEL([unable to determine how to map (type, pid_nr) -> struct task])
fi

CR_CHECK_KERNEL_CALL([alloc_pid],[#include <linux/sched.h>])
if test -n "${HAVE_ALLOC_PID}"; then
  CR_CHECK_KERNEL_CALL_NARGS([alloc_pid],[#include <linux/sched.h>],[],[NULL])
  if test "${HAVE_0_ARG_ALLOC_PID}${HAVE_1_ARG_ALLOC_PID}" != "1"; then
    CR_BAD_KERNEL([unable to determine how to call alloc_pid()])
  fi
  CR_FIND_KSYM([free_pid],[CODE])
  CR_FIND_KSYM([pid_hash],[DATA],[extern struct hlist_head *pid_hash;])
  CR_FIND_KSYM([pidhash_shift],[DATA],[extern int pidhash_shift;])
  # Can't use true type because 'struct kmem_cache' is opaque by design
  CR_CHECK_KERNEL_MEMBER([pid_namespace.pid_cachep],[
	#include <linux/slab.h>
	#include <linux/pid_namespace.h>
    ], [struct pid_namespace],[void *],[pid_cachep])
  CR_CHECK_KERNEL_MEMBER([pid_namespace.nr_hashed],[
	#include <linux/slab.h>
	#include <linux/pid_namespace.h>
    ], [struct pid_namespace],[unsigned int],[nr_hashed])
  CR_FIND_KSYM([pid_cachep],[DATA],[
    #if HAVE_KMEM_CACHE_T
      extern kmem_cache_t *pid_cachep;
    #else
      extern struct kmem_cache *pid_cachep;
    #endif
  ])
  if test -z "${cr_addr}${HAVE_PID_NAMESPACE_PID_CACHEP}"; then
    CR_BAD_KERNEL([unable to determine where to find pid_cachep])
  fi
  if test -n "${HAVE_LINUX_PSPACE_H}"; then
    CR_FIND_KSYM([init_pspace],[DATA])
  elif test -n "${HAVE_LINUX_PID_NAMESPACE_H}"; then
    CR_FIND_KSYM([init_pid_ns],[DATA])
  fi
fi

CR_CHECK_KERNEL_DECL([2.6.0 attach_pid],[attach_pid],
	[#include <linux/sched.h>
	 #include <linux/pid.h>],
	[extern int FASTCALL(attach_pid(struct task_struct *task, enum pid_type type, int nr));])
CR_CHECK_KERNEL_DECL([2.6.22 attach_pid],[attach_pid],
	[#include <linux/sched.h>
	 #include <linux/pid.h>],
	[extern int FASTCALL(attach_pid(struct task_struct *task, enum pid_type type, struct pid *pid));])
CR_CHECK_KERNEL_DECL([2.6.26 attach_pid],[attach_pid],
	[#include <linux/sched.h>
	 #include <linux/pid.h>],
	[extern void attach_pid(struct task_struct *task, enum pid_type type, struct pid *pid);])

CR_CHECK_KERNEL_CALL([change_pid],[
	#include <linux/sched.h>
	#include <linux/pid.h>])
# OK if missing

# Require change_pid or exactly one match to attach_pid family
if test -z "${HAVE_CHANGE_PID}"; then
  if test "${HAVE_2_6_0_ATTACH_PID}${HAVE_2_6_22_ATTACH_PID}${HAVE_2_6_26_ATTACH_PID}" != "1"; then
    CR_BAD_KERNEL([unable to determine how to call attach_pid()])
  fi
fi

CR_CHECK_KERNEL_TYPE([struct pidmap],[
  #if HAVE_LINUX_PSPACE_H
    #include <linux/pspace.h>
  #endif
  #if HAVE_LINUX_PID_NAMESPACE_H
    #include <linux/pid_namespace.h>
  #endif
])
CR_FIND_KSYM([pidmap_array],[DATA],[
  #if !HAVE_STRUCT_PIDMAP
    struct pidmap {
        atomic_t nr_free;
        void *page;
    };
  #endif
  extern struct pidmap pidmap_array[];
])
if test x"$cr_kernel_smp" = xyes; then
  CR_FIND_KSYM([pidmap_lock],[DATA],[extern spinlock_t pidmap_lock;])
  if test -z "$cr_addr"; then
    AC_MSG_ERROR([failed to find symbol pidmap_lock])
  fi
fi

CR_CHECK_KERNEL_CALL(kill_pid,[#include <linux/sched.h>])
CR_CHECK_KERNEL_CALL(kill_proc,[#include <linux/sched.h>])
# Require at least one
if test -z "${HAVE_KILL_PID}${HAVE_KILL_PROC}"; then
  CR_BAD_KERNEL([unable to find kill_pid() or kill_proc()])
fi

CR_CHECK_KERNEL_CALL([child_reaper],[#include <linux/pid_namespace.h>])
CR_CHECK_KERNEL_CALL([task_child_reaper],[#include <linux/pid_namespace.h>])
if test -z "${HAVE_CHILD_REAPER}${HAVE_TASK_CHILD_REAPER}"; then
  CR_FIND_KSYM([child_reaper],[DATA],[extern struct task_struct *child_reaper;])
fi

CR_CHECK_KERNEL_MEMBER([task.parent],[#include <linux/sched.h>],
  [struct task_struct],[struct task_struct *],[parent])
CR_CHECK_KERNEL_MEMBER([task.real_parent],[#include <linux/sched.h>],
  [struct task_struct],[struct task_struct *],[real_parent])

# Look for non-deprecated "typedef struct SOMETHING kmem_cache_t".
# This is slightly complicated by the fact that this is an opaque type (with
# the struct itself forward-declared but not defined in the public headers).
pushdef([cr_cvname],cr_cv_kconfig_HAVE_KMEM_CACHE_T)[]dnl
AC_CACHE_CHECK([kernel for kmem_cache_t],cr_cvname,[
  CR_TRY_KERNEL_COMPILE([#include <linux/slab.h>],[kmem_cache_t *x = NULL;],[
    CR_EGREP_KERNEL_HEADER([typedef.+[[:space:]]kmem_cache_t[[:space:]].+deprecated],
      [linux/slab.h],[cr_cvname=no],[cr_cvname=yes]) # Want *not* found
    ], [cr_cvname=no])
])
popdef([cr_cvname])[]dnl
CR_DEFINE(HAVE_KMEM_CACHE_T,[test x$cr_cv_kconfig_HAVE_KMEM_CACHE_T = xyes],
	    [Define to 1 if the kmem_cache_t typedef exists and is not deprecated])

# type of (struct fs_struct).lock
CR_CHECK_KERNEL_MEMBER([fs_struct rwlock],[
    #include <linux/sched.h>
    #include <linux/fs_struct.h>
  ],[struct fs_struct],[rwlock_t],[lock])
CR_CHECK_KERNEL_MEMBER([fs_struct spinlock],[
    #include <linux/sched.h>
    #include <linux/fs_struct.h>
  ],[struct fs_struct],[spinlock_t],[lock])
if test "$HAVE_FS_STRUCT_RWLOCK$HAVE_FS_STRUCT_SPINLOCK" = "11" \
     -o "$HAVE_FS_STRUCT_RWLOCK$HAVE_FS_STRUCT_SPINLOCK" = "00" ; then
  CR_BAD_KERNEL([unrecognized type for fs_struct.lock])
fi
 
CR_CHECK_KERNEL_TYPE([gfp_t],[#include <linux/slab.h>])
# OK if missing
CR_CHECK_KERNEL_CALL([kzalloc],[#include <linux/slab.h>])
# OK if missing
CR_CHECK_KERNEL_CALL([kmem_cache_zalloc],[#include <linux/slab.h>])
# OK if missing
CR_CHECK_KERNEL_CALL([kmemdup],[#include <linux/string.h>])
# OK if missing

CR_CHECK_KERNEL_MEMBER([pipe_inode_info.base],[
    #include <linux/sched.h>
    #include <linux/pipe_fs_i.h>
  ],[struct pipe_inode_info],[char*],[base])
# OK if missing.
CR_CHECK_KERNEL_MEMBER([pipe_inode_info.buffers],[
    #include <linux/sched.h>
    #include <linux/pipe_fs_i.h>
    ],[struct pipe_inode_info],[unsigned int],[buffers])
# OK if missing.
CR_CHECK_KERNEL_MEMBER([pipe_buf_operations.pin],[
    #include <linux/sched.h>
    #include <linux/pipe_fs_i.h>
  ],[struct pipe_buf_operations],[],[pin])
if test -z "${HAVE_PIPE_BUF_OPERATIONS_PIN}"; then
  CR_CHECK_KERNEL_COMPILE([2-arg pipe_ops.unmap],[
      #include <linux/sched.h>
      #include <linux/pipe_fs_i.h>
    ],[
      struct pipe_buf_operations p;
      (void)p.unmap(NULL,NULL);
    ])
  CR_CHECK_KERNEL_COMPILE([3-arg pipe_ops.unmap],[
      #include <linux/sched.h>
      #include <linux/pipe_fs_i.h>
    ],[
      struct pipe_buf_operations p;
      (void)p.unmap(NULL,NULL,NULL);
    ])
  CR_CHECK_KERNEL_CALL([copy_page_to_iter],[#include <linux/uio.h>])
fi
# Require exactly one match
if test "${HAVE_PIPE_INODE_INFO_BASE}${HAVE_PIPE_BUF_OPERATIONS_PIN}${HAVE_2_ARG_PIPE_OPS_UNMAP}${HAVE_3_ARG_PIPE_OPS_UNMAP}${HAVE_COPY_PAGE_TO_ITER}" != "1"; then
  CR_BAD_KERNEL([unrecognized pipe buf operations])
fi
CR_FIND_KSYM([anon_pipe_buf_ops],[DATA],
	[extern struct pipe_buf_operations anon_pipe_buf_ops;])
CR_CHECK_KERNEL_CALL([pipe_fcntl],[
    #include <linux/sched.h>
    #include <linux/pipe_fs_i.h>
  ])
if test "${HAVE_PIPE_FCNTL}" = "1"; then
    CR_FIND_KSYM([pipe_fcntl],[CODE], 
      [long pipe_fcntl(struct file *file, unsigned int cmd, unsigned long arg);])
fi
# Now try to define CR_F_SETPIPE_SZ and CR_F_GETPIPE_SZ, for the pipe test.
#
# We try the user space headers first.  If we don't find a definition, then we 
# poke around the kernel headers for the definition.
#
# If we still don't have a definition at this point, the crut test also
# uses an #ifdef F_SETPIPE_SZ before using CR_F_SETPIPE_SZ, so we have
# one more chance.
CR_CHECK_KERNEL_MACRO([F_SETPIPE_SZ],[#include <linux/fcntl.h>])
CR_CHECK_KERNEL_MACRO([F_GETPIPE_SZ],[#include <linux/fcntl.h>])
if test "${HAVE_F_SETPIPE_SZ}${HAVE_F_GETPIPE_SZ}" = "11"; then
    CR_DEFINE_INT(CR_F_SETPIPE_SZ,[F_SETPIPE_SZ],[#include <fcntl.h>])
    if test -z "${CR_F_SETPIPE_SZ}"; then 
        CR_DEFINE_KERNEL_INT(
                  CR_F_SETPIPE_SZ,[F_SETPIPE_SZ],[#include <linux/fcntl.h>])
    fi
    CR_DEFINE_INT(CR_F_GETPIPE_SZ,[F_GETPIPE_SZ],[#include <fcntl.h>])
    if test -z "${CR_F_GETPIPE_SZ}"; then
        CR_DEFINE_KERNEL_INT(
                  CR_F_GETPIPE_SZ,[F_GETPIPE_SZ],[#include <linux/fcntl.h>])
    fi
fi
# The user space headers may not define F_SETPIPE_SZ or F_GETPIPE_SZ,
# and we need these values to run the unit test,
# so we define CR_F_SETPIPE_SZ and CR_F_GETPIPE_SZ to hold the values
# that we find in the kernel.

# For O_DIRECT support
CR_CHECK_KERNEL_MEMBER([file_operations.check_flags],[#include <linux/fs.h>],
		       [struct file_operations],[],[check_flags])
CR_CHECK_KERNEL_MEMBER([file.f_lock],[#include <linux/fs.h>],
		       [struct file],[spinlock_t],[f_lock])
CR_CHECK_KERNEL_MEMBER([inode.i_mapping],[#include <linux/fs.h>],
		       [struct inode],[struct address_space*],[i_mapping])
CR_CHECK_KERNEL_MEMBER([file.f_mapping],[#include <linux/fs.h>],
		       [struct file],[struct address_space*],[f_mapping])
# Require one or more match on mapping
if test -z "${HAVE_INODE_I_MAPPING}${HAVE_FILE_F_MAPPING}"; then
  CR_BAD_KERNEL([could not locate address_space in inode or file structs])
fi

CR_CHECK_KERNEL_MEMBER([file_operations.unlocked_ioctl],[#include <linux/fs.h>],
		       [struct file_operations],[],[unlocked_ioctl])

CR_CHECK_KERNEL_CALL_NARGS([do_generic_file_read],[#include <linux/fs.h>],
	[NULL,NULL,NULL,NULL],[NULL,NULL,NULL,NULL,0])
CR_CHECK_KERNEL_MEMBER([read_descriptor_t.arg.data],[#include <linux/fs.h>],
		       [read_descriptor_t],[void*],[arg.data])
CR_CHECK_KERNEL_MEMBER([read_descriptor_t.buf],[#include <linux/fs.h>],
		       [read_descriptor_t],[char*],[buf])
if test "${HAVE_READ_DESCRIPTOR_T_ARG_DATA}${HAVE_READ_DESCRIPTOR_T_BUF}" != "1"; then
  CR_BAD_KERNEL([unable to determine how to use do_generic_file_read()])
fi

CR_CHECK_KERNEL_CALL([splice_direct_to_actor],[
	#include <linux/fs.h>
	#include <linux/splice.h>])

CR_CHECK_KERNEL_MEMBER([task.ptrace],[#include <linux/sched.h>],
		       [struct task_struct],[unsigned long],[ptrace])
CR_CHECK_KERNEL_MEMBER([task.ptracees],[#include <linux/sched.h>],
		       [struct task_struct],[struct list_head],[ptracees])
CR_CHECK_KERNEL_MEMBER([task.ptraced],[#include <linux/sched.h>],
		       [struct task_struct],[struct list_head],[ptraced])
if test -n "${HAVE_TASK_PTRACEES}"; then
  CR_FIND_KSYM([ptrace_utrace_ops],[DATA],
	[extern struct utrace_engine_ops ptrace_utrace_ops;])
  if test -z "$cr_addr"; then
    AC_MSG_ERROR([failed to find symbol ptrace_utrace_ops])
  fi
fi
if test "${HAVE_TASK_PTRACE}${HAVE_TASK_PTRACEES}" != "1"; then
  CR_BAD_KERNEL([unable to determine how to identify ptrace()ed/ing processes])
fi

CR_CHECK_KERNEL_SYMBOL([proc_root],[#include <linux/proc_fs.h>])
CR_CHECK_KERNEL_CALL([proc_mkdir],[#include <linux/proc_fs.h>])
CR_CHECK_KERNEL_CALL([proc_create],[#include <linux/proc_fs.h>])
CR_CHECK_KERNEL_CALL([proc_remove],[#include <linux/proc_fs.h>])

# check kmalloc max size
CR_CHECK_KMALLOC_MAX

## Now we check for some additional (potentially) non-exported kernel symbols...

# DATA
CR_FIND_KSYM([tasklist_lock],[DATA])
CR_FIND_KSYM([shmem_file_operations],[DATA],
	[extern const struct file_operations shmem_file_operations;])
CR_FIND_KSYM([ramfs_file_operations],[DATA],
	[extern const struct file_operations ramfs_file_operations;])
CR_FIND_KSYM([HPAGE_SHIFT],[DATA],[extern unsigned int HPAGE_SHIFT;])
CR_FIND_KSYM([phys_base],[DATA],[extern unsigned long phys_base;])

# CODE
CR_FIND_KSYM([do_pipe],[CODE])
CR_FIND_KSYM([do_pipe_flags],[CODE])
CR_FIND_KSYM([sys_munmap],[CODE],
	[extern asmlinkage long sys_munmap(unsigned long addr, size_t len);])
CR_FIND_KSYM([sys_dup2],[CODE],
	[extern asmlinkage long sys_dup2(unsigned int oldfd, unsigned int newfd);])
CR_FIND_KSYM([sys_lseek],[CODE],
	[extern asmlinkage off_t sys_lseek(unsigned int fd, off_t offset, unsigned int origin);])
CR_FIND_KSYM([sys_ftruncate],[CODE],
	[extern asmlinkage long sys_ftruncate(unsigned int fd, unsigned long length);])
CR_FIND_KSYM([sys_mprotect],[CODE],
	[extern asmlinkage long sys_mprotect(unsigned long start, size_t len, unsigned long prot);])
CR_FIND_KSYM([sys_setitimer],[CODE],
	[extern asmlinkage long sys_setitimer(int which, struct itimerval *value, struct itimerval *ovalue);])
CR_FIND_KSYM([sys_prctl],[CODE],
	[extern asmlinkage long sys_prctl(int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5);])

CR_FIND_KSYM([copy_fs_struct],[CODE])
CR_FIND_KSYM([put_fs_struct],[CODE])
if test -z "$cr_addr"; then
  CR_FIND_KSYM([free_fs_struct],[CODE])
  if test -z "$cr_addr"; then
    AC_MSG_ERROR([failed to find either symbol put_fs_struct or free_fs_struct])
  fi
fi

# Check for auditing support
CR_CHECK_KERNEL_MACRO([CONFIG_AUDITSYSCALL])
if test -n "${HAVE_CONFIG_AUDITSYSCALL}"; then
  CR_CHECK_KERNEL_CALL([audit_dummy_context], [#include <linux/audit.h>])
  CR_CHECK_KERNEL_CALL_FULL([2.6.6 audit_syscall_exit],[audit_syscall_exit],[
        #include <linux/audit.h>
        #ifdef AUDITSC_RESULT
          choke me
        #endif
    ],[ ],[NULL,0])
  CR_CHECK_KERNEL_CALL_FULL([2.6.12 audit_syscall_exit],[audit_syscall_exit],[
        #include <linux/audit.h>
        #ifndef AUDITSC_RESULT
          choke me
        #endif
    ],[ ],[NULL,AUDITSC_RESULT(0),0])
  CR_CHECK_KERNEL_CALL_FULL([2.6.17 audit_syscall_exit],[audit_syscall_exit],[
        #include <linux/audit.h>
        #ifndef AUDITSC_RESULT
          choke me
        #endif
    ],[ ],[AUDITSC_RESULT(0),0])
  CR_CHECK_KERNEL_CALL([__audit_syscall_exit], [#include <linux/audit.h>])
  if test x"${HAVE_2_6_6_AUDIT_SYSCALL_EXIT}${HAVE_2_6_12_AUDIT_SYSCALL_EXIT}${HAVE_2_6_17_AUDIT_SYSCALL_EXIT}${HAVE___AUDIT_SYSCALL_EXIT}" != x"1"; then
    AC_MSG_ERROR([failed to determine arguments to audit_syscall_exit()])
  fi
  if test x"${HAVE_2_6_17_AUDIT_SYSCALL_EXIT}" = x"1"; then
    CR_FIND_KSYM([audit_syscall_exit],[CODE],
        [extern void audit_syscall_exit(int failed, long return_code);])
  elif test x"${HAVE_2_6_12_AUDIT_SYSCALL_EXIT}" = x"1"; then
    CR_FIND_KSYM([audit_syscall_exit],[CODE],
        [extern void audit_syscall_exit(struct task_struct *task, int failed, long return_code);])
  elif test x"${HAVE_2_6_6_AUDIT_SYSCALL_EXIT}" = x"1"; then
    CR_FIND_KSYM([audit_syscall_exit],[CODE],
        [extern void audit_syscall_exit(struct task_struct *task, int return_code);])
  elif test x"${HAVE___AUDIT_SYSCALL_EXIT}" = x"1"; then
    CR_FIND_KSYM([__audit_syscall_exit],[CODE],
        [extern void __audit_syscall_exit(int ret_success, long ret_value);])
  else
    : # Unreachable?
    AC_MSG_ERROR([failed to determine arguments to audit_syscall_exit()])
  fi
  if test -z "$cr_addr"; then
    AC_MSG_ERROR([failed to find symbol audit_syscall_exit()])
  fi
fi

# For blcr
CR_FIND_KSYM([set_fs_pwd],[CODE])
# For vmadump4
CR_FIND_KSYM([set_mm_exe_file],[CODE])
CR_FIND_KSYM([sys_mremap],[CODE],
	[extern asmlinkage unsigned long sys_mremap(unsigned long addr, unsigned long old_len, unsigned long new_len, unsigned long flags, unsigned long new_addr);])
CR_FIND_KSYM([do_sigaction],[CODE])
if test $CR_KARCH = x86_64; then
  CR_FIND_KSYM([load_gs_index],[CODE])

  CR_CHECK_KERNEL_CALL([read_pda],[#include <asm/pda.h>])
  CR_CHECK_KERNEL_CALL([write_pda],[#include <asm/pda.h>])
  if test "$HAVE_READ_PDA$HAVE_WRITE_PDA" != "11"; then
    CR_CHECK_KERNEL_CALL([percpu_read],[#include <asm/percpu.h>])
    CR_CHECK_KERNEL_CALL([percpu_write],[#include <asm/percpu.h>])
    CR_CHECK_KERNEL_CALL([this_cpu_read],[#include <linux/percpu.h>])
    CR_CHECK_KERNEL_CALL([this_cpu_write],[#include <linux/percpu.h>])
    if test "$HAVE_PERCPU_READ$HAVE_PERCPU_WRITE" != "11" -a \
            "$HAVE_THIS_CPU_READ$HAVE_THIS_CPU_WRITE" != "11" ; then
      CR_BAD_KERNEL([found neither {read,write}_pda() nor percpu_{read,write}() nor this_cpu_{read,write}()])
    else
      CR_FIND_KSYM([per_cpu__old_rsp],[DATA],[DECLARE_PER_CPU(unsigned long, old_rsp);])
      if test -z "$cr_addr"; then
        CR_FIND_KSYM([old_rsp],[DATA],[DECLARE_PER_CPU(unsigned long, old_rsp);])
        if test -z "$cr_addr"; then
          AC_MSG_ERROR([failed to locate either symbol per_cpu__old_rsp or old_rsp])
        fi
      fi
    fi
  fi
fi
if test x$enable_restore_ids = xyes; then
  CR_FIND_KSYM([sys_setgroups],[CODE],
	[extern asmlinkage long sys_setgroups(int, gid_t *);])
  CR_FIND_KSYM([sys_setresuid],[CODE],
	[extern asmlinkage long sys_setresuid(uid_t, uid_t, uid_t);])
  CR_FIND_KSYM([sys_setresgid],[CODE],
	[extern asmlinkage long sys_setresgid(gid_t, gid_t, gid_t);])
fi
CR_FIND_KSYM([expand_files],[CODE],
	[extern int expand_files(struct files_struct *, int);])
CR_FIND_KSYM([expand_fdtable],[CODE],
	[extern int expand_fdtable(struct files_struct *, int);])
CR_FIND_KSYM([__flush_icache_range],[CODE])
CR_FIND_KSYM([flush_icache_range],[CODE])

CR_FIND_KSYM([sys_fchmod],[CODE],
	[extern asmlinkage long sys_fchmod(int fd, mode_t mode);])
CR_FIND_KSYM([sys_link],[CODE],
	[extern asmlinkage long sys_link(const char *oldname, const char *newname);])
CR_FIND_KSYM([sys_unlink],[CODE],
	[extern asmlinkage long sys_unlink(const char *pathname);])
CR_FIND_KSYM([sys_mknod],[CODE],
	[extern asmlinkage long sys_mknod(const char *filename, int mode, unsigned int dev);])


# CODE & DATA used for sanity checks
#
# Unless you are adding additional sanity checks, there is
# no good reason to add more calls to CR_FIND_EXPORTED_KSYM.
#
# register_{chr,blk}dev are used at module load to validate 
# the System.map used to locate symbols and configure time.
CR_FIND_EXPORTED_KSYM([register_chrdev],[CODE])
if test "$cr_addr" = "0"; then
  CR_FIND_EXPORTED_KSYM([__register_chrdev],[CODE])
  if test "$cr_addr" = "0"; then
    CR_BAD_KERNEL([kernel does not export register_chrdev or __register_chrdev])
  fi
fi
CR_FIND_EXPORTED_KSYM([register_blkdev],[CODE])
if test "$cr_addr" = "0"; then
  CR_BAD_KERNEL([kernel does not export register_blkdev])
fi
# One of {init,system}_utsname is used to validate UTS_RELEASE
CR_CHECK_KERNEL_CALL([init_utsname],[#include <linux/utsname.h>])
CR_CHECK_KERNEL_SYMBOL([system_utsname],[#include <linux/utsname.h>])
if test -z "${HAVE_INIT_UTSNAME}${HAVE_SYSTEM_UTSNAME}"; then
  CR_BAD_KERNEL([unable to determine how to query UTSNAME])
fi

AC_DEFINE_UNQUOTED([BLCR_CONFIG_TIMESTAMP], "`date`", [Timestamp of BLCR configuration])

fi # cr_build_modules


if test x"$cr_build_libcr" = xyes && expr $CR_ARCH : ppc >/dev/null; then
  AC_MSG_CHECKING([for work around for bug 2524])
  if test "$cr_build_modules,$force_bug2524" = "yes,"; then
    if test [`echo $LINUX_VER | sed -e 's/^2\.6\.\([0-9]*\).*/\1/;' -e 's/^3\..*/999/;'`] -le 15; then
      AC_MSG_ERROR([Because you are building for a PowerPC kernel 2.6.15 or older, you must specify either --with-bug2524 or --without-bug2524.  If in doubt --with-bug2524 is the safe option (with a small performance penalty).])
    fi
  fi
  case "$force_bug2524" in
    yes|no) ;; # nothing to do
    '') force_bug2524=no ;; # empty is no
    *) AC_MSG_ERROR([Argument to --with-bug2524 must be 'yes', 'no' or empty]) ;;
  esac
  AC_MSG_RESULT([$force_bug2524])
  CR_DEFINE(CR_HAVE_BUG2524,[test x$force_bug2524 = xyes],
          [Define to 1 if work around for bug 2524 is requested.])
fi

################################################################################
# AM_CONDITIONAL may not be called contitionally, so these go here
################################################################################
AM_CONDITIONAL(CR_HAVE_SYSTEM_MAP,[echo $LINUX_SYMTAB_CMD | grep ^cat >/dev/null 2>&1])
AM_CONDITIONAL(CR_BUILD_MAN,[test x$cr_build_util$cross_compiling = xyesno])
AM_CONDITIONAL(CR_HAVE_FTB,[test x$cr_have_ftb = xyes])
AM_CONDITIONAL(CR_HAVE_CXX, [test "x$CXX" != xno])

################################################################################
# Automake sillyness
################################################################################
AC_SUBST([GMAKE_IFEQ], ifeq)
AC_SUBST([GMAKE_IFNEQ], ifneq)
AC_SUBST([GMAKE_ELSE], else)
AC_SUBST([GMAKE_ENDIF], endif)

################################################################################
# Kbuild glue
################################################################################
AC_SUBST([KBUILD_MAK], "${TOP_BUILDDIR}/config/kbuild.mak")

################################################################################
# Enable/disable building various pieces of the code
################################################################################
AM_CONDITIONAL(BUILD_CR_INFO, [test x$enable_cr_info = xyes])
AM_CONDITIONAL(BUILD_VMADLIB, false)
AM_CONDITIONAL(CR_BUILD_MODULES,[test x$cr_build_modules = xyes])
AM_CONDITIONAL(CR_BUILD_UTIL,	[test x$cr_build_util = xyes])
AM_CONDITIONAL(CR_BUILD_LIBCR,	[test x$cr_build_libcr = xyes])
AM_CONDITIONAL(CR_BUILD_INCLUDE,[test x$cr_build_include = xyes])
AM_CONDITIONAL(CR_BUILD_TESTS,  [test x$cr_build_tests = xyes])
AM_CONDITIONAL(CR_BUILD_EXAMPLES,[test x$cr_build_examples = xyes])
AM_CONDITIONAL(CR_BUILD_CONTRIB,[test x$cr_build_contrib = xyes])

################################################################################
# Finally generate the files and a report (unless supressed)
################################################################################
if test x"$cr_build_modules" = xyes; then
  AC_CONFIG_FILES([cr_module/Makefile
		   cr_module/kbuild/Makefile
                   vmadump4/Makefile
                   include/blcr_imports.h
                   blcr_imports/imports.c
                   blcr_imports/Makefile
                   blcr_imports/kbuild/Makefile
                   config/kbuild.mak
		  ])
fi
if test x"$cr_build_util" = xyes; then
  AC_CONFIG_FILES([util/Makefile
                   util/cr_checkpoint/Makefile
                   util/cr_restart/Makefile
                   util/cr_run/Makefile
                   util/cr_info/Makefile
                   man/Makefile
		  ])
fi
if test x"$cr_build_libcr" = xyes; then
  AC_CONFIG_FILES([libcr/Makefile])
fi
if test x"$cr_build_tests" = xyes; then
  AC_CONFIG_FILES([tests/Makefile])
  AC_CONFIG_FILES(tests/shellinit, chmod +x tests/shellinit)
fi
if test x"$cr_build_examples" = xyes; then
  AC_CONFIG_FILES([examples/Makefile
                   examples/counting/Makefile
                   examples/pthread_counting/Makefile
                   examples/pthread_misc/Makefile
                   examples/file_counting/Makefile
                   examples/io_bench/Makefile
                   examples/pipe_counting/Makefile
		  ])
fi
if test x"$cr_build_contrib" = xyes; then
  AC_CONFIG_FILES(contrib/Makefile)
  AC_CONFIG_FILES(contrib/cr_insmod, chmod +x contrib/cr_insmod)
  AC_CONFIG_FILES(contrib/gen_cscope, chmod +x contrib/gen_cscope)
  AC_CONFIG_FILES(contrib/batch_build, chmod +x contrib/batch_build)
fi
AC_CONFIG_HEADER(blcr_config.h)
AC_CONFIG_FILES([Makefile
                 etc/Makefile
                 rpm/Makefile
                 rpm/specfile
                 include/Makefile
                 include/blcr_common.h
                 doc/Makefile
		])

AC_OUTPUT

if test x"$enable_config_report$cr_build_modules" = xyesyes; then
  echo ''
  current_kernel=`uname -r`
  if test "`uname -r`" != "$LINUX_VER" -a $cross_compiling = no; then
    echo '*******************************************************************'
    echo '***** WARNING WARNING WARNING WARNING WARNING WARNING WARNING *****'
    echo '*******************************************************************'
    echo '* The kernel source does not match currently the running kernel.  *'
    echo '* Compilation will produce modules unsuitable for the currently   *'
    echo '* running kernel, which may not be what you intended.             *'
    echo '*******************************************************************'
    echo '***** WARNING WARNING WARNING WARNING WARNING WARNING WARNING *****'
    echo '*******************************************************************'
  fi
  if test x"$cr_static_link_warning" = xyes; then
    tmp=`echo "'$ldsuggest'                                          " | cut -c-44`
    echo '*******************************************************************'
    echo '***** WARNING WARNING WARNING WARNING WARNING WARNING WARNING *****'
    echo '*******************************************************************'
    echo '* Static linking with -lpthread appears to yield LinuxThreads,    *'
    echo '* but BLCR supports only the NPTL implementation of pthreads.     *'
    echo "* Please use LDFLAGS=$tmp *"
    echo "* (or similar) when linking static executables with BLCR's libs   *"
    echo '* to ensure you get the supported NPTL pthreads implementation.   *'
    echo '*******************************************************************'
    echo '***** WARNING WARNING WARNING WARNING WARNING WARNING WARNING *****'
    echo '*******************************************************************'
  fi
  CR_CONFIG_REPORT
  if test x"$cr_arch_experimental" = xyes; then
    echo
    echo 'WARNING: BLCR support for architecture "'$host_cpu'" is EXPERIMENTAL at this time.'
    echo
  fi
  if test x"$cr_arch_incomplete" = xyes; then
    echo
    echo 'WARNING: BLCR support for architecture "'$host_cpu'" is INCOMPLETE at this time.'
    echo 'WARNING: You probably won'\''t be able to do anything useful with this build.'
    echo
  fi
fi
