# This file is read by Autoconf.                       -*- Autoconf -*-
#
#   Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
#   2008, The Regents of the University of California, through Lawrence
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
# $Id: acinclude.m4,v 1.177.14.22 2014/10/21 00:14:39 phargrov Exp $
AC_REVISION($Revision: 1.177.14.22 $)

# Match all kernels major/minor we might accept
m4_define([cr_kern_maj_min_patt],[[\(2\.6\|3\.[0-9][0-9]*\)[-.]]])[]dnl  No SUBLEVEL or following
m4_define([cr_kern_maj_min_perl],[[(2\.6|3\.[0-9]+)[-.]]])[]dnl  No SUBLEVEL or following

# cr_substr(STRING,OFFSET,[LEN])
# ------------------------------------------------------
# Wrapper around substr to implement perl-style negative
# values of offset and len
AC_DEFUN([cr_substr],[dnl
pushdef([length],len([$1]))dnl
pushdef([offset],m4_eval($2+length*($2<0)))dnl
m4_ifval([$3],[m4_substr($1,offset,m4_eval($3+(length-offset)*($3<0)))],[m4_substr($1,offset)])[]dnl
popdef([offset])dnl
popdef([length])])

# cr_trim_ws(STRING)
# ------------------------------------------------------
# Trims leading and trailing whitespace
AC_DEFUN([cr_trim_ws],[regexp([$1],[^[ 	]*\(.*[^ 	]\)[ 	]*$],[\1])])dnl

# cr_type_check(EXPR,TYPE)
# ------------------------------------------------------
# Expands to one or more C declarations and statements that
# try to compile-time check that the given EXPR has the given TYPE.
# Uses tc_ prefix for identifiers.
# Current checks:
#   For TYPE=""
#     Just evaluate the expression (which might actually be a statement macro).
#     No type checking, but ensures EXPR is syntax checked.
#   For TYPE="void"
#     Evaluate "(void)(EXPR);"
#     Ensures that EXPR is a valid "rvalue".  NOT WHAT CALLER MIGHT EXCPECT.
#   For TYPE="void *"
#     Evaluate "void * tc_1 = EXPR"
#     Ensures that EXPR has *some* pointer type.
#   For TYPE="foo *"
#     Evaluate "foo tc_1 = *(EXPR)"
#     Ensures the type really is exactly what we said.
#   Default:
#     Evaluate "TYPE tc_1 = EXPR"
#     Ensures the type is assignment compatible (pretty weak).
# XXX: probably broken for array types.
AC_DEFUN([cr_type_check],[dnl
  pushdef([cr_type],cr_trim_ws([$2]))dnl
  m4_if(cr_type,[],[$1;],
        cr_type,[void],[(void)($1);],
        regexp(cr_type,[void[ 	]*\*$]),0,[void * tc_1 = [$1];],
        regexp(cr_type,[.*\*$]),0, cr_substr(cr_type,0,-1)[ tc_1 = *([$1]);],
	  [cr_type tc_1 = [$1];])[]dnl
  popdef([cr_type])dnl
])

# CR_CACHE_REVALIDATE(FILE,KEY,DESCR)
# ------------------------------------------------------
# Check that FILE is still the same as our cached version.
# Invalidates corresponding autoconf cache values if not.
AC_DEFUN([CR_CACHE_REVALIDATE],[
 pushdef([cr_cached_file],[.cached_]$2)[]dnl
 pushdef([cr_cvprefix],[cr_cv_]$2[_])[]dnl
 if test -z "$cache_file" || \
    test "$cache_file" = /dev/null || \
    cmp "$1" cr_cached_file >/dev/null 2>/dev/null; then
   : # OK - either not caching or cached version still matches
 else
   if test -f cr_cached_file; then
     AC_MSG_WARN([$3 has changed... discarding cached results.])
     rm -f cr_cached_file
   fi
   for cr_var in cr_cvprefix[]_NON_EMPTY_HACK `(set) | grep "^cr_cvprefix" | cut -d= -f1`; do
     unset $cr_var
   done
   cp "$1" cr_cached_file
 fi
 popdef([cr_cvprefix])[]dnl
 popdef([cr_cached_file])[]dnl
])

# CR_GET_CACHE_VAR(VARNAME)
# CR_SET_CACHE_VAR(VARNAME)
# ------------------------------------------------------
# Manage cached values of LINUX_{SRC,OBJ,etc.}
# Includes checking against current (command line) settings
AC_DEFUN([CR_GET_CACHE_VAR],[
  pushdef([cr_cvname],cr_cv_var_[$1])[]dnl
  if test "${[$1]+set}${cr_cvname+set}" = setset; then
    if test "$[$1]" != "$cr_cvname"; then
      AC_MSG_ERROR([Cached [$1] ($cr_cvname) does not match current value ($[$1]).  Remove '$cache_file' before re-running configure.])
    fi
  elif test "${cr_cvname+set}" = set; then
    [$1]="$cr_cvname"
  fi
  popdef([cr_cvname])[]dnl
])
AC_DEFUN([CR_SET_CACHE_VAR],[test -n "$[$1]" && cr_cv_var_[$1]="$[$1]"])

# CR_IF(test,[ACTION-IF-FOUND],[ACTION-IF-NOT-FOUND])
# ------------------------------------------------------
# Basic functionality of AS_IF, but not as "optimized"
# Implemented here because m4sh is not yet documented.
AC_DEFUN([CR_IF],[
  if $1; then
    m4_default([$2], :)
  else
    m4_default([$3], :)
  fi
])

# CR_PROG_GCC([LIST],[ACTION-IF-FOUND],[ACTION-IF-NOT-FOUND])
# ------------------------------------------------------
# Find the first compiler in the list and verify that it is gcc.
AC_DEFUN([CR_PROG_GCC],[
  AC_PROG_CC($1)
  CR_IF([test "$GCC" = yes],[$2],[$3])
])


# CR_PROG_CXX
# ------------------------------------------------------
# Find a C++ compiler and verify that it is word-size compatible with $CC
# Returns with CXX set to the compiler, or "no"
# Note bug 2619 reports that CR_PROG_CXX must not be called conditionally
# because of some internal use of AM_CONDITIONAL()
AC_DEFUN([CR_PROG_CXX],[
  AC_REQUIRE([AC_PROG_CC])
  # Totally gross way to perform a non-fatal probe for CXX
  pushdef([AC_MSG_ERROR], [CXX=no])
  AC_REQUIRE([AC_PROG_CXX])
  AC_REQUIRE([AC_PROG_CXXCPP])
  popdef([AC_MSG_ERROR])
  # Now validate the choice
  if test "x$CXX" != xno; then
    AC_LANG_SAVE
    AC_LANG_C
    AC_CHECK_SIZEOF(void *, $cross_void_P)
    AC_LANG_CPLUSPLUS
    AC_CACHE_CHECK([[whether CXX='$CXX' acts like a C++ compiler]], cr_cv_cxx_is_cxx, [
      cr_cv_cxx_is_cxx=no
      AC_TRY_COMPILE([
	#ifndef __cplusplus
	    #error __cplusplus must be defined in a C++ compilation!
	#endif
      ], [ int x = 1; ], [cr_cv_cxx_is_cxx=yes])
    ])
    if test x"$cr_cv_cxx_is_cxx" = xyes; then
      AC_CACHE_CHECK([[whether CXX='$CXX' matches wordsize of CC]], cr_cv_cxx_voidp, [
        cr_cv_cxx_voidp=no
        AC_TRY_COMPILE([
	  #ifndef __cplusplus
	      #error __cplusplus must be defined in a C++ compilation!
	  #endif
        ], [ int a[(($ac_cv_sizeof_void_p == sizeof(void *))? 1 : -1)];], [cr_cv_cxx_voidp=yes])
      ])
    fi
    AC_LANG_RESTORE
    if test x"$cr_cv_cxx_is_cxx$cr_cv_cxx_voidp" != xyesyes; then
      CXX=no
    fi
  fi
])

# CR_CHECK_GLIBC(MAJOR,MINOR,[ACTION-IF-FOUND],[ACTION-IF-NOT-FOUND])
# ------------------------------------------------------
# Check for glibc >= MAJOR.MINOR and sets the variable
# cr_cv_check_glibc_MAJOR_MINOR_or_higher to 'yes' or 'no'
AC_DEFUN([CR_CHECK_GLIBC],[
  pushdef([cr_cvname],cr_cv_check_glibc_[$1]_[$2]_or_higher)[]dnl
  AC_CACHE_CHECK([[for GNU libc version >= $1.$2]],cr_cvname,[
    AC_TRY_LINK([
	#include <features.h>
	#ifndef __GLIBC_PREREQ
	    #define __GLIBC_PREREQ(maj, min) \
		((__GLIBC__ << 16) + __GLIBC_MINOR__ >= ((maj) << 16) + (min))
	#endif
	#if !__GLIBC_PREREQ($1, $2)
	    #error "Bad glibc version"
	#endif
	extern int gnu_get_libc_version(void); /* Ensures this *is* glibc */
    ],[
	return !gnu_get_libc_version();
    ], [cr_cvname=yes], [cr_cvname=no])
  ])
  CR_IF([eval test $cr_cvname = yes],[$3],[$4])
  popdef([cr_cvname])[]dnl
])

# CR_KERNEL_TYPE(TYPE)
# ------------------------------------------------------
# Check for /boot/kernel.h and establish user-requested overrides
# if possible.
AC_DEFUN([CR_KERNEL_TYPE],[
  cr_kernel_type=`echo $1 | tr 'a-z' 'A-Z'`
  cr_header="/boot/kernel.h"
  AC_MSG_CHECKING([for $cr_header]);
  cr_result=ok
  if test \! -e "$cr_header"; then
    cr_result='missing'
  elif test \! -r "$cr_header"; then
    cr_result='not readable'
  fi
  AC_MSG_RESULT([$cr_result])
  if test "$cr_result" != "ok"; then
    AC_MSG_ERROR([You have requested '--with-kernel-type=$cr_kernel_type', but $cr_header is $cr_result.])
  fi
  cr_kernel_var="__BOOT_KERNEL_$cr_kernel_type"
  if test -z "`grep \"$cr_kernel_var\" $cr_header 2>/dev/null`"; then
    AC_MSG_ERROR([You have requested '--with-kernel-type=$cr_kernel_type', but $cr_header does not appear to support that type.])
  fi
  # The following sed command transforms all the #ifndef lines from kernel.h into
  # corresponding preprocessor flags (on one line) which select the desired kernel type.
  # Note use of [] for m4 quoting of a sed command containing [ and ]
  CR_KTYPE_CPPFLAGS=[`sed -n -e '/^#ifndef \('$cr_kernel_var'\)$/ {s//-D\1=1 /;H;}' \
			     -e '/^#ifndef \(__BOOT_KERNEL_[A-Z]*\)$/ {s//-D\1=0 /;H;}' \
			     -e '$ {x;s/[ \t\n]\+/ /g;s/^ //;s/ $//;p;}' \
			     $cr_header`]
])

# _CR_EXTRACT_UTS_VERSION(DIR,CPP_CMD)
# ------------------------------------------------------
# Helper to extract UTS_VERSION from Linux kernel sources.
AC_DEFUN([_CR_EXTRACT_UTS_VERSION],[dnl
  $PERL -- - "$1" "$2" <<'_EOF_'
  [
    my ($srcdir, $cpp_cmd) = @ARGV;
    my $stamp = time;
    $cpp_cmd =~ s/([#()])/\\$1/g; # quote problematic shell metachars
    FILE: foreach my $file (qw(linux/version.h linux/utsrelease.h generated/utsrelease.h)) {
      my $path = "$srcdir/include/$file";
      next FILE unless (-f $path);
      open(F, "echo '=${stamp}->UTS_RELEASE<-' | ${cpp_cmd} -include ${path} - |") || exit 1;
      LINE: while (<F>) {
        next LINE if(/^#/);
        if ((my $uts) = /=${stamp}->"(]cr_kern_maj_min_perl[.*)"<-/o) {
          print "$uts\n";
          exit 0;
        }
      }
      close(F) || exit 1;
    }
    exit 1;
  ]
_EOF_
])

# _CR_CHECK_VERSION_H(DIR,VAR)
# ------------------------------------------------------
# Check if the indicated DIR contains version.h.
# Sets $VAR to the full UTS_RELEASE string, or an error string
AC_DEFUN([_CR_CHECK_VERSION_H],[
  for cr_version_h in $1/include/linux/version.h $1/include/generated/uapi/linux/version.h none; do
    test -r $cr_version_h && break
  done
  if test $cr_version_h = none; then
    [$2]='version.h missing'
  else
    [$2]=`_CR_EXTRACT_UTS_VERSION($1, [$KCC -E -I$1/include -D__KERNEL__ -DMODULE $CR_KTYPE_CPPFLAGS $CPPFLAGS])`
    test $? = 0 || [$2]='no UTS_RELEASE could be extracted'
    AC_SUBST([LINUX_VERSION_H],[$cr_version_h])
  fi
])

# _CR_CHECK_LINUX_SRC(VERSION,DIR)
# ------------------------------------------------------
# Check if the indicated DIR contains source for the given
# kernel version.  Verification consists of a Makefile with
# the proper version info.
#
# Sets cr_linux_src_ver to the version found (w/ EXTRAVERSION), or to the empty string
# Note that a build dir with a Makefile created by scripts/mkmakefile
# is acceptible as a "source" directory for our purposes.
AC_DEFUN([_CR_CHECK_LINUX_SRC],[
  AC_MSG_CHECKING([[for Linux kernel source in $2]])
  cr_cvname=cr_cv_kernel_src_[]AS_TR_SH($2)
  AC_CACHE_VAL([${cr_cvname}],[
    cr_tmp=''
    if test -e "[$2]/Makefile"; then
      # First try using version.h, as some distros play odd games w/ the Makefile
      _CR_CHECK_VERSION_H([$2],cr_tmp)
      # Now trim EXTRAVERSION, or yield empty if no pattern match
      cr_tmp=`echo $cr_tmp | sed -n -e '/^\(cr_kern_maj_min_patt[[0-9]]\+\).*$/ {s//\1/p;q;}'`

      # Next try "asking" the Makefile
      if test -z "$cr_tmp"; then
        # If a dependency does not exist, then make may complain.
        # The -k and 2>/dev/null take care of that.
        cr_tmp=`(${MAKE} -k echo_kver --no-print-directory -C $2 -f - 2>/dev/null | grep '^cr_kern_maj_min_patt') <<'_EOF_'
echo_kver:
	@echo '$(VERSION).$(PATCHLEVEL).$(SUBLEVEL)'

include Makefile
_EOF_`
        expr "$cr_tmp" : 'cr_kern_maj_min_patt' >/dev/null || cr_tmp='' # Reject if not matched to pattern
      fi

      # Finally try grepping the Makefile
      if test -z "$cr_tmp"; then
        # Note the use of [] for m4 quoting, since the pattern contains [ and ]
        cr_linux_ver1=[`sed -n -e '/^VERSION[ \t]*=[ \t]*\([0-9]\+\).*$/ {s//\1/p;q;}' "$2/Makefile"`]
        cr_linux_ver2=[`sed -n -e '/^PATCHLEVEL[ \t]*=[ \t]*\([0-9]\+\).*$/ {s//\1/p;q;}' "$2/Makefile"`]
        cr_linux_ver3=[`sed -n -e '/^SUBLEVEL[ \t]*=[ \t]*\([0-9]\+\).*$/ {s//\1/p;q;}' "$2/Makefile"`]
        cr_tmp="${cr_linux_ver1}.${cr_linux_ver2}.${cr_linux_ver3}"
        expr "$cr_tmp" : 'cr_kern_maj_min_patt' >/dev/null || cr_tmp='' # Reject if not matched to pattern
      fi

      test -n "$cr_tmp" || cr_tmp='not found'
    elif test -d "$2"; then
      cr_tmp='Makefile missing'
    else
      cr_tmp='not found'
    fi
    eval "$cr_cvname='$cr_tmp'"
    unset cr_tmp
  ])
  eval "cr_result=\$$cr_cvname"
  unset cr_cvname
  if expr "$cr_result" : 'cr_kern_maj_min_patt' >/dev/null; then
    AC_MSG_RESULT([found version $cr_result])
  else
    AC_MSG_RESULT([$cr_result])
  fi
  # Check that version is acceptible (exact match, or a prefix with the next char non-numeric), including matches forced by --with-linux-src-ver
  case "$cr_linux_obj_ver" in
    [${cr_result}|${cr_result}[!0-9]*])      # the outer [] is m4 quoting
        cr_linux_src_ver="$cr_result";;
    *)  cr_linux_src_ver=''
	if test x"$cr_result" = x"$LINUX_SRC_VER"; then
	  AC_MSG_WARN([Accepting $2 as kernel source due to --with-linux-src-ver=$LINUX_SRC_VER])
	  cr_linux_src_ver="$LINUX_SRC_VER"
	fi
	;;
  esac
])

# CR_FIND_LINUX_SRC(VERSION)
# ------------------------------------------------------
# Search for Linux src dir matching the given version
# Sets cr_linux_src_ver to the version found, or to the empty string
# Sets LINUX_SRC if found.
AC_DEFUN([CR_FIND_LINUX_SRC],[
  cr_linux_src_ver=''
  CR_GET_CACHE_VAR([LINUX_SRC_ARG])
  if expr X"$LINUX_SRC_ARG" : X/ >/dev/null; then
    cr_list="$LINUX_SRC_ARG"
  elif test -n "$LINUX_SRC_ARG"; then
    AC_MSG_ERROR([[--with-linux-src argument '$LINUX_SRC_ARG' is not a full path]])
  else
    # Search standard locations
    cr_list="${LINUX_OBJ} \
	     /lib/modules/[$1]/source \
	     /usr/src/linux-[$1] \
	     /usr/src/linux-headers-[$1] \
	     /usr/src/kernels/[$1]"
  fi
  for cr_linux_dir in $cr_list; do
      _CR_CHECK_LINUX_SRC([$1],[$cr_linux_dir])
      if test -n "$cr_linux_src_ver"; then
	LINUX_SRC="$cr_linux_dir"
	break
      fi
  done
  CR_IF([test -z "$cr_linux_src_ver"],
	[AC_MSG_ERROR([Could not locate source directory corresponding to build directory '${LINUX_OBJ}'.  Please use --with-linux-src=FULL_PATH_TO_KERNEL_SRC to specify a directory and/or --with-linux-src-ver=VERSION to force use of a source directory with a version which does not appear to match the build])])
])

# _CR_CHECK_LINUX_OBJ(VER_PATT,DIR,VAR)
# ------------------------------------------------------
# Check if the indicated DIR contains a kernel build with version
# matching the given patterm (expr).  Verification consists of:
# + include/linux/version.h w/ proper version
# + .config
# Sets cr_linux_obj_ver to the version found, or to the empty string
AC_DEFUN([_CR_CHECK_LINUX_OBJ],[
  AC_MSG_CHECKING([[for Linux kernel build in $2]])
  cr_cvname=cr_cv_kernel_obj_[]AS_TR_SH($2)
  AC_CACHE_VAL([${cr_cvname}],[
    if test -d "$2"; then
      # Check for version.h
      _CR_CHECK_VERSION_H([$2],[cr_tmp])
      # Check for .config if required
      if expr "$cr_tmp" : 'cr_kern_maj_min_patt' >/dev/null; then
        test -r "$2/.config" || cr_tmp='.config missing'
      fi
    else
      cr_tmp='not found'
    fi
    eval "$cr_cvname='$cr_tmp'"
    unset cr_tmp
  ])
  eval "cr_result=\$$cr_cvname"
  unset cr_cvname
  if expr "$cr_result" : 'cr_kern_maj_min_patt' >/dev/null; then
    AC_MSG_RESULT([found version $cr_result])
  else
    AC_MSG_RESULT([$cr_result])
  fi
  # Check that version appears acceptible
  CR_IF([expr "$cr_result" : $1 >/dev/null],
        [cr_linux_obj_ver="$cr_result"],
        [cr_linux_obj_ver=''])
])

# CR_FIND_LINUX_OBJ(VER_PATT)
# ------------------------------------------------------
# Search for Linux obj (aka build) dir.
# Sets cr_linux_obj_ver to the version found, or to the empty string
# Sets LINUX_OBJ if found.
AC_DEFUN([CR_FIND_LINUX_OBJ],[
  CR_GET_CACHE_VAR([LINUX_OBJ_ARG])
  if expr X"$LINUX_OBJ_ARG" : X/ >/dev/null; then
    # User provided a path
    _CR_CHECK_LINUX_OBJ(['cr_kern_maj_min_patt'], [${LINUX_OBJ_ARG}])
    CR_IF([test -z "$cr_linux_obj_ver"],
          [AC_MSG_ERROR([Directory ${LINUX_OBJ_ARG} does not appear to contain a Linux kernel build])])
    LINUX_OBJ="${LINUX_OBJ_ARG}"
  else
    if test -z "$LINUX_OBJ_ARG"; then
      cr_tmp_ver=`uname -r`
    elif expr "$LINUX_OBJ_ARG" : 'cr_kern_maj_min_patt' >/dev/null; then
      cr_tmp_ver="$LINUX_OBJ_ARG"
    else
      AC_MSG_ERROR([[--with-linux argument '$LINUX_OBJ_ARG' is neither a kernel version string nor a full path]])
    fi
    cr_ver_patt="`echo $cr_tmp_ver | sed -e 's/\./\\\\./g;'`\$"
    # Search standard locations
    for cr_linux_dir in \
			/lib/modules/${cr_tmp_ver}/build \
			/usr/src/linux-${cr_tmp_ver}-obj \
			/usr/src/linux-${cr_tmp_ver} \
			/usr/src/linux-headers-${cr_tmp_ver} \
	     		/usr/src/kernels/${cr_tmp_ver} \
			; do
      _CR_CHECK_LINUX_OBJ([${cr_ver_patt}],[${cr_linux_dir}])
      if test -n "$cr_linux_obj_ver"; then
        LINUX_OBJ="${cr_linux_dir}"
        break
      fi
    done
    CR_IF([test -z "$cr_linux_obj_ver"],
          [AC_MSG_ERROR([Could not find a directory containing a Linux kernel ${cr_tmp_ver} build.  Perhaps try --with-linux=FULL_PATH_TO_KERNEL_BUILD])])
    unset cr_tmp_ver
  fi
])

# CR_CHECK_LINUX()
# ------------------------------------------------------
# Check for Linux source and build dirs.
# Sets LINUX_SRC, LINUX_OBJ and LINUX_VER accordingly.
# Also sets HAVE_LINUX_2_6 or HAVE_LINUX_3 on success
AC_DEFUN([CR_CHECK_LINUX],[
  AC_SUBST([LINUX_SRC])
  AC_SUBST([LINUX_OBJ])
  AC_SUBST([LINUX_VER])
  AC_SUBST([CR_KERNEL])
  AC_SUBST([CR_KERNEL_BASE])
  CR_FIND_LINUX_OBJ()
  CR_IF([test -n "$cr_linux_obj_ver"],[CR_FIND_LINUX_SRC($cr_linux_obj_ver)])
  CR_IF([test -n "$cr_linux_src_ver" -a -n "$cr_linux_obj_ver"],[
    case "$cr_linux_obj_ver" in
      2.6.[[0-9]]*) HAVE_LINUX_2_6=yes;;
      3.[[0-9]]*) HAVE_LINUX_3=yes;;
    esac
    LINUX_VER="$cr_linux_obj_ver"
    CR_KERNEL=`echo $cr_linux_obj_ver | tr - _`
    CR_KERNEL_BASE=`echo $CR_KERNEL | sed -e 's:smp\|enterprise\|bigmem\|hugemem::g'`
    CR_SET_CACHE_VAR([LINUX_SRC_ARG])
    CR_SET_CACHE_VAR([LINUX_OBJ_ARG])
    CR_CACHE_REVALIDATE([${LINUX_OBJ}/.config],[kconfig],[kernel configuration])
  ])
])

# CR_CHECK_KBUILD()
# ------------------------------------------------------
# Sets up the automake to kbuild interface
AC_DEFUN([CR_CHECK_KBUILD],[
  pushdef([cr_cvname],cr_cv_KBUILD_MAKE_ARGS)[]dnl
  AC_SUBST([KBUILD_MAKE_ARGS])
  AC_CACHE_CHECK([[for parameters to interface GNU automake with Linux kbuild]],cr_cvname,[
    if grep KBUILD_EXTMOD ${LINUX_SRC}/Makefile >/dev/null 2>/dev/null; then
      cr_cvname='KBUILD_EXTMOD=$(builddir)'
    else
      cr_cvname='SUBDIRS=$(builddir) modules'
    fi
    if test "${LINUX_OBJ}" != "${LINUX_SRC}"; then
      cr_cvname="${cr_cvname} O=${LINUX_OBJ}"
    fi
    if test x$enable_kbuild_verbose = xyes; then
      cr_cvname="${cr_cvname} V=1"
    fi
    if test x$cross_compiling = xyes; then
      cr_cvname="$cr_cvname ARCH=$CR_KARCH CROSS_COMPILE=$host_alias-"
    fi
  ])
  KBUILD_MAKE_ARGS="$cr_cvname"
  popdef([cr_cvname])[]dnl
  # Note: we'll actually try the result in CR_SET_KCFLAGS
])

# CR_SET_KCFLAGS()
# ------------------------------------------------------
# Finds CFLAGS needed for compiler-based probes of kernel sources
AC_DEFUN([CR_SET_KCFLAGS],[
 AC_REQUIRE([AC_PROG_EGREP])
 pushdef([cr_cvname],[cr_cv_kconfig_kcflags])[]dnl
 AC_CACHE_CHECK([[for flags to compile Linux kernel probes]],cr_cvname,[
  rm -rf conftestdir
  mkdir conftestdir
  echo '#include <linux/sched.h>' > conftestdir/conftest.c
  echo 'int foo = 0;' >> conftestdir/conftest.c
  echo 'obj-m := conftest.o' >conftestdir/Makefile
  unset cr_tmp
  echo "${MAKE} -C ${LINUX_SRC} builddir=\"`pwd`/conftestdir\" ${KBUILD_MAKE_ARGS} CC=\"${KCC}\" V=1" >&5
  ${MAKE} -C ${LINUX_SRC} builddir="`pwd`/conftestdir" ${KBUILD_MAKE_ARGS} CC="${KCC}" V=1 >conftestdir/output 2>&1 </dev/null
  if test $? = 0; then cr_tmp=`grep -m1 conftest\\.c conftestdir/output | [sed -e "s:^[ 	]*${KCC}::"]`; fi
  cat conftestdir/output >&5
  if test "${cr_tmp:+OK}" != OK; then
      AC_MSG_RESULT([FAILED])
      cat conftestdir/output
      if $EGREP ['include/(asm|linux)/[a-zA-Z0-9_-]+\.h:'] conftestdir/output >/dev/null 2>&1; then
        AC_MSG_WARN([Apparent compilation problem in ${LINUX_SRC}])
	ver=`$KCC --version | head -1`
        AC_MSG_WARN([Perhaps KCC='$KCC' ($ver) is not compatible with this kernel source])
	if test $cr_wordsize -gt $ac_cv_sizeof_void_p; then
	  echo "$KCC" | grep -e '-m64' >/dev/null 2>/dev/null
	  if test $? != 0; then
            AC_MSG_WARN([You might try setting KCC='$KCC -m64'])
          fi
        fi
      fi
      if grep -i 'permission denied' conftestdir/output >/dev/null 2>&1; then
        AC_MSG_WARN([Apparent permissions problem in ${LINUX_SRC}])
      fi
      rm -rf conftestdir
      AC_MSG_ERROR([Failed test run of kernel make/kbuild failed (see above)])
  fi
  rm -rf conftestdir
  cr_cvname=''
  prev_del=''
  prev_inc=''
  for arg in ${cr_tmp}; do
    if test -n "$prev_del"; then # skip this arg at request of prev arg
      prev_del=''
      continue
    fi
    arg=`echo $arg | tr -d "\"'"` # remove quote marks
    if test -n "$prev_inc"; then # prev arg says this arg is an -include
      prev_inc=''
      case "$arg" in
	*include/linux/modversions.h) continue;;
	/*) arg="-include $arg";;
	*) arg="-include ${LINUX_OBJ}/$arg";;
      esac
    else
      case "$arg" in
	-o) prev_del=1; continue;;
	-include) prev_inc=1; continue;;
	-c) continue;;
	conftest.c) continue;;
	/*/conftest.c) continue;;
	-Wp,-MD,*) continue;;
	-Wp,-MMD,*) continue;;
	-I/*) ;;
	-I*) arg=`echo $arg | [sed -e "s:-I:-I${LINUX_OBJ}/:"]`;;
	-Werror=strict-prototypes) continue;; # Breaks "int main()" in our probes
      esac
    fi
    cr_cvname="$cr_cvname $arg"
  done])
  KCFLAGS="$cr_cvname"
  popdef([cr_cvname])[]dnl
  AC_MSG_CHECKING([if autoconf.h or kconfig.h is included implicitly])
  if echo "$KCFLAGS" | grep -e 'include [[^ ]]*/autoconf\.h' -e 'include [[^ ]]*/kconfig\.h' >/dev/null 2>&1; then
    AC_MSG_RESULT([yes]);
  else
    AC_MSG_RESULT([no]);
    AC_DEFINE([CR_NEED_AUTOCONF_H], [1])
    AH_TEMPLATE([CR_NEED_AUTOCONF_H], [Define to 1 if linux/autoconf.h must be included explicitly])
  fi
  # Do these init steps early, in case first CR_FIND_KSYM is a conditional call
  AC_REQUIRE([_CR_KSYM_INIT_PATTS])
  AC_REQUIRE([_CR_KSYM_INIT_FILES])
])

# CR_DEFINE(VARIABLE,TEST,[TEMPLATE_TEXT])
# ------------------------------------------------------
# Wrapper to invoke AC_DEFINE for a boolean, and optionally AH_TEMPLATE
AC_DEFUN([CR_DEFINE],[
  if [$2]; then
    AC_DEFINE([$1], [1])
  else
    AC_DEFINE([$1], [0])
  fi
  m4_ifval([$3], AH_TEMPLATE([$1], [$3]))
])

# CR_DEFINE_AND_SET(VARIABLE,TEST,[TEMPLATE_TEXT])
# ------------------------------------------------------
# Wrapper like CR_DEFINE which also sets the corresponding shell variable
# to either 1 or the empty string
AC_DEFUN([CR_DEFINE_AND_SET],[
  if [$2]; then
    AC_DEFINE([$1], [1])
    eval "$1=1"
  else
    AC_DEFINE([$1], [0])
    eval "$1=''"
  fi
  m4_ifval([$3], AH_TEMPLATE([$1], [$3]))
])

# CR_ARG_ENABLE(NAME,HELP,[DEFAULT])
# ------------------------------------------------------
# Our own wrapper for AC_ARG_ENABLE
# Default for DEFAULT is "no"
AC_DEFUN([CR_ARG_ENABLE],[
  AC_ARG_ENABLE([$1],
    [AC_HELP_STRING([--enable-$1], [$2])],
    :,
    [[enable_]translit([$1],[-],[_])=m4_default([$3], [no])])
])

# CR_WITH_COMPONENTS(LIST)
# ------------------------------------------------------
# If no flag is passed, then ALL components are built.
# If a flag IS passed, then ONLY the given components are built.
# This is for internal/maintainer use only, thus no --help.
# ex: --with-components=bin,lib
AC_DEFUN([CR_WITH_COMPONENTS],[
  AC_ARG_WITH([components])
  case x"$with_components" in
    xyes)
      AC_MSG_ERROR([--with-components requires an argument.  Known components are: $1]);
      ;;
    x)
      for cr_lcv in $1; do
	eval cr_build_${cr_lcv}=yes
      done
      ;;
    *)
      for cr_lcv in $1; do
	if expr "$with_components" : '.*'"$cr_lcv" >/dev/null; then
	  eval cr_build_${cr_lcv}=yes
	else
	  eval cr_build_${cr_lcv}=no
	fi
      done
      ;;
  esac
])

# CR_CHECK_SIGNUM
# ------------------------------------------------------
# Probes for value to assign to CR_SIGNUM
# Care is taken to use the value from the lib if preloaded
AC_DEFUN([CR_CHECK_SIGNUM],[
  pushdef([cr_cvname],[cr_cv_check_cr_signum])[]dnl
  AC_CACHE_CHECK([[for value of CR_SIGNUM]],cr_cvname,[
    cr_cvname="failed"
    SAVE_LIBS="$LIBS"
    LIBS="-ldl -lpthread $LIBS"
    AC_TRY_RUN(
       [#include <stdio.h>
	#include <dlfcn.h>

	extern int __libc_allocate_rtsig(int);
	int main(void)
	{
	  int s = -1;
	  FILE *f=fopen("conftestval", "w");
	  void *dlhandle = dlopen(NULL, RTLD_LAZY);
	  if (dlhandle) {
	    int *tmp = (int *)dlsym(dlhandle, "cri_signum");
	    dlclose(dlhandle);
	    if (tmp) s = *tmp;
	  }
	  if (s <= 0) {
	    s=__libc_allocate_rtsig(0);
	  }
	  if (!f || s<=0) return(1);
	  fprintf(f, "%d\n", s);
	  return(0);
	}],
	[cr_cvname=`cat conftestval`],
	[AC_MSG_ERROR([Failed to probe CR_SIGNUM])],
	[CR_CROSS_VAR(cross_signum)
	 cr_cvname=$cross_signum])
    LIBS="$SAVE_LIBS"
  ])
  AC_SUBST([CR_SIGNUM],[$cr_cvname])
  popdef([cr_cvname])[]dnl
])

# CR_CONFIG_REPORT
# ------------------------------------------------------
# Reports certain probed/configured values
AC_DEFUN([CR_CONFIG_REPORT],[
  echo "======================================================================"
  echo "Please review the following configuration information:"
  echo "  Kernel source directory = $LINUX_SRC"
  echo "  Kernel build directory = $LINUX_OBJ"
  echo "  Kernel symbol table = ${LINUX_SYSTEM_MAP}${LINUX_VMLINUX}"
  echo "  Kernel version probed from kernel build = $LINUX_VER"
  echo "  Kernel running currently = `uname -r`"
  echo "======================================================================"
])

# CR_TRY_GCC_FLAG(FLAG,[ACTION-IF-TRUE],[ACTION-IF_FALSE])
# ------------------------------------------------------
# Wrapper to invoke AC_TRY_COMPILE for checking a gcc flag
AC_DEFUN([CR_TRY_GCC_FLAG],[
  pushdef([cr_cvname],[cr_cv_gcc_flag]AS_TR_CPP($1))[]dnl
  AC_CACHE_CHECK([[whether gcc accepts $1]],cr_cvname,[
    SAVE_CFLAGS=$CFLAGS
    CFLAGS="$CFLAGS $1"
    AC_TRY_COMPILE([], [], [cr_cvname=yes], [cr_cvname=no])
    CFLAGS=$SAVE_CFLAGS
  ])
  CR_IF([eval test $cr_cvname = yes],[$2],[$3])
  popdef([cr_cvname])[]dnl
])

# CR_EGREP_KERNEL_CPP(PATTERN,PROGRAM,[ACTION-IF-TRUE],[ACTION-IF_FALSE])
# ------------------------------------------------------
# Wrapper to invoke AC_EGREP_CPP against the kernel headers
AC_DEFUN([CR_EGREP_KERNEL_CPP],[
  AC_REQUIRE([CR_SET_KCFLAGS])
  SAVE_CPP=$CPP
  SAVE_CPPFLAGS=$CPPFLAGS
  CPP="$KCC -E"
  CPPFLAGS="$KCFLAGS"
  AC_EGREP_CPP([$1],[
		 #include <linux/kernel.h>
		 #ifndef FASTCALL
		   #define FASTCALL(_decl) _decl
		 #endif
		 #include <linux/types.h>
		 $2],
       [CPP=$SAVE_CPP
	CPPFLAGS=$SAVE_CPPFLAGS
	$3],
       [CPP=$SAVE_CPP
	CPPFLAGS=$SAVE_CPPFLAGS
	$4])
])


# CR_EGREP_KERNEL_HEADER(PATTERN,HEADER,[ACTION-IF-TRUE],[ACTION-IF_FALSE])
# ------------------------------------------------------
# Wrapper to invoke (approximately) AC_EGREP_HEADER against the kernel headers
AC_DEFUN([CR_EGREP_KERNEL_HEADER],[
  CR_EGREP_KERNEL_CPP([$1],[#include <$2>],[$3],[$4])
])

# CR_TRY_KERNEL_COMPILE(INCLUDES,PROGRAM,[ACTION-IF-TRUE],[ACTION-IF_FALSE])
# ------------------------------------------------------
# Wrapper to invoke AC_TRY_COMPILE against the kernel headers
AC_DEFUN([CR_TRY_KERNEL_COMPILE],[
  AC_REQUIRE([CR_SET_KCFLAGS])
  SAVE_CC=$CC
  SAVE_CFLAGS=$CFLAGS
  SAVE_CPPFLAGS=$CPPFLAGS
  CC=$KCC
  CFLAGS=""
  CPPFLAGS="$KCFLAGS"
  AC_TRY_COMPILE([
		 #include <linux/kernel.h>
		 #ifndef FASTCALL
		   #define FASTCALL(_decl) _decl
		 #endif
		 #include <linux/types.h>
		 $1],
		 [$2], 
	[CC=$SAVE_CC
	 CFLAGS=$SAVE_CFLAGS
	 CPPFLAGS=$SAVE_CPPFLAGS
	 $3],
        [CC=$SAVE_CC
	 CFLAGS=$SAVE_CFLAGS
	 CPPFLAGS=$SAVE_CPPFLAGS
	 $4])
])

# CR_CACHED_KERNEL_COMPILE(KEY,INCLUDES,PROGRAM,[ACTION-IF-TRUE],[ACTION-IF_FALSE])
# ------------------------------------------------------
# Wrapper to invoke CR_TRY_KERNEL_COMPILE w/ caching
# Note KEY should be un-prefixed cache key
# Should be called only between AC_MSG_CHECKING and AC_MSG_RESULT
# Result appears as "yes" or "no" in cr_result (valid in the ACTIONs)
AC_DEFUN([CR_CACHED_KERNEL_COMPILE],[
  pushdef([cr_cvname],cr_cv_kconfig_[]$1)[]dnl
  AC_CACHE_VAL([cr_cvname],[
    CR_TRY_KERNEL_COMPILE([$2],[$3],[cr_cvname=yes],[cr_cvname=no])
  ])
  cr_result=$cr_cvname
  CR_IF([test $cr_result = yes],[$4],[$5])
  popdef([cr_cvname])[]dnl
])

# CR_CHECK_KERNEL_COMPILE(SYMBOL,INCLUDES,PROGRAM,[TEMPLATE_TEXT])
# ------------------------------------------------------
# Wrapper to invoke CR_CACHED_KERNEL_COMPILE then AC_DEFINE.
# Calls AH_TEMPLATE with either default or supplied text.
# Also sets HAVE_... to 1 or empty string
AC_DEFUN([CR_CHECK_KERNEL_COMPILE],[
  pushdef([cr_name],AS_TR_CPP(HAVE_[]$1))[]dnl
  AH_TEMPLATE(cr_name,m4_default([$4], [[Define to 1 if the kernel has $1.]]))
  AC_MSG_CHECKING([kernel for $1])
  CR_CACHED_KERNEL_COMPILE(cr_name,[$2],[$3],
    [AC_DEFINE(cr_name, [1])
     cr_name=1],
    [AC_DEFINE(cr_name, [0])
     cr_name=''])
  AC_MSG_RESULT([$cr_result])
  popdef([cr_name])[]dnl
])

# CR_CHECK_KERNEL_HEADER(FILE,[INCLUDES],[TEMPLATE_TEXT])
# ------------------------------------------------------
# Wrapper to invoke CR_CHECK_KERNEL_COMPILE for a given header
AC_DEFUN([CR_CHECK_KERNEL_HEADER],[
  CR_CHECK_KERNEL_COMPILE([$1],[
   $2
   #include <$1>
  ],[ ], m4_default([$3], [[Define to 1 if the kernel has the <$1> header file.]]))
])

# CR_CHECK_KERNEL_SYMBOL(SYMBOL,[INCLUDES],[TEMPLATE_TEXT])
# ------------------------------------------------------
# Wrapper to invoke CR_CHECK_KERNEL_COMPILE for a given symbol
AC_DEFUN([CR_CHECK_KERNEL_SYMBOL],[
  CR_CHECK_KERNEL_COMPILE([$1],[$2],[
     int x = sizeof(&$1);
  ],[$3])
])

# CR_CHECK_KERNEL_CALL(SYMBOL,[INCLUDES],[TEMPLATE_TEXT])
# ------------------------------------------------------
# Wrapper to invoke CR_CHECK_KERNEL_COMPILE for a given function or macro
AC_DEFUN([CR_CHECK_KERNEL_CALL],[
  CR_CHECK_KERNEL_COMPILE([$1],[$2],[
   #ifdef $1
     /* OK, it exists and is a macro */
   #else
     /* Check for function case */
     int x = sizeof(&$1);
   #endif
  ],m4_default([$3], [[Define to 1 if the kernel has the macro or function $1().]]))
])

# CR_CHECK_KERNEL_CALL_FULL(SHORTNAME,SYMBOL,INCLUDES,TYPE,ARGS,[TEMPLATE_TEXT])
# ------------------------------------------------------
# Wrapper to invoke CR_CHECK_KERNEL_COMPILE for a given function or macro
# Includes checking for compatible return type and arguments
AC_DEFUN([CR_CHECK_KERNEL_CALL_FULL],[
  CR_CHECK_KERNEL_COMPILE([$1],[$3],[
	#ifndef $2 /* Must be macro or have a decl */
	  int x = sizeof(&$2);
	#endif
	]cr_type_check([$2($5)],[$4]),
      m4_default([$6], m4_ifval([$4], [Define to 1 if the kernel has "$4 $2($5)".],
                                      [Define to 1 if the kernel has "$2($5)".])))
])

# CR_CHECK_KERNEL_CALL_NARGS(SYMBOL,INCLUDES,ARGS1[,ARGS2...])
# ------------------------------------------------------
# See if each given SYMBOL(ARGSn) will compile
# Defines HAVE_[N]_ARG_[uppercase($1)] for N equal to argument
# count for any successful compilations.
AC_DEFUN([CR_CHECK_KERNEL_CALL_NARGS],[
  m4_if(m4_eval([$# >= 3]),1,[
	  pushdef([cr_nargs],[len(patsubst([$3],[[^,]+,?],[@]))])
	  CR_CHECK_KERNEL_CALL_FULL(cr_nargs[-arg $1],[$1],[$2],[],[$3],
			[[Define to 1 if the kernel has ]cr_nargs[-arg $1().]])
	  popdef([cr_nargs])])
  m4_if(m4_eval([$# > 3]),1,[$0([$1],[$2],m4_shift(m4_shift(m4_shift($@))))])dnl tail recursion
])

# CR_CHECK_KERNEL_TYPE(NAME,[INCLUDES],[TEMPLATE_TEXT])
# ------------------------------------------------------
# Wrapper to invoke CR_CHECK_KERNEL_COMPILE for a given type
AC_DEFUN([CR_CHECK_KERNEL_TYPE],[
  CR_CHECK_KERNEL_COMPILE([$1],[$2],[
     $1 y;
     int x = sizeof(y);
  ],m4_default([$3], [[Define to 1 if the kernel has the type '$1'.]]))
])

# CR_CHECK_KERNEL_MEMBER(SHORTNAME,INCLUDES,AGGREGATE,TYPE,MEMBER,[TEMPLATE_TEXT])
# ------------------------------------------------------
# Wrapper to invoke CR_CHECK_KERNEL_COMPILE for a given aggregate member w/ type check
AC_DEFUN([CR_CHECK_KERNEL_MEMBER],[
  CR_CHECK_KERNEL_COMPILE([$1],[$2],
      [$3 x; ]
      cr_type_check([x.$5],[$4]),
      m4_default([$6], m4_ifval([$4],
                                [Define to 1 if the kernel type '$3' has member '$5' of type '$4'.],
                                [Define to 1 if the kernel type '$3' has member '$5'.])))
])

# CR_CHECK_KERNEL_CONSTANT(SYMBOL,[INCLUDES],[TEMPLATE_TEXT])
# ------------------------------------------------------
# Wrapper to invoke CR_CHECK_KERNEL_COMPILE for a given constant
AC_DEFUN([CR_CHECK_KERNEL_CONSTANT],[
  CR_CHECK_KERNEL_COMPILE([$1],[$2],[
     typeof($1) y;
     int x = sizeof(y);
  ],m4_default([$3], [[Define to 1 if the kernel defines the constant $1.]]))
])

# CR_CHECK_KERNEL_MACRO(SYMBOL,[INCLUDES],[TEMPLATE_TEXT])
# ------------------------------------------------------
# Wrapper to invoke CR_CHECK_KERNEL_COMPILE for a given macro
AC_DEFUN([CR_CHECK_KERNEL_MACRO],[
  CR_CHECK_KERNEL_COMPILE([$1],[$2],[
   #ifndef $1
     choke me
   #endif
  ],m4_default([$3], [[Define to 1 if the kernel defines the macro $1.]]))
])

# CR_CHECK_KERNEL_DECL(SHORTNAME,SYMBOL,INCLUDES,DECL,[TEMPLATE_TEXT])
# ------------------------------------------------------
# Wrapper to invoke CR_CHECK_KERNEL_COMPILE to check a protoype/declaration.
# The prototype/declaration is tried at file scope to check for conflicting
# declarations, and the symbol is referenced in main to catch lack of any
# declaration.
AC_DEFUN([CR_CHECK_KERNEL_DECL],[
  CR_CHECK_KERNEL_COMPILE([$1],
  [$3
   $4],[int x = sizeof(&$2)],
  m4_default([$5], [[Define to 1 if the kernel has $1.]]))
])

# CR_BAD_KERNEL([REASON])
# ------------------------------------------------------
# Die, informing the user that the kernel cannot be used.
# Optionally provide a reason.
AC_DEFUN([CR_BAD_KERNEL],[
  AC_CACHE_SAVE
  CR_CONFIG_REPORT
  m4_ifval([$1],
   [ AC_MSG_ERROR([Unable to use kernel $LINUX_VER - $1]) ],
   [ AC_MSG_ERROR([Unable to use kernel $LINUX_VER]) ])
])

# CR_LINUX_SYMTAB()
# ------------------------------------------------------
# Check for System.map in some standard places
# If LINUX_SYSTEM_MAP is set, just verifies existance,
# else if LINUX_VMLINUX is set, just verifies existance,
# else search for either file in standard locations.
# When complete sets LINUX_SYMTAB_CMD such that
# "eval $LINUX_SYMTAB_CMD" will produce a System.map on stdout.
cr_stripped_maps=''
m4_define([cr_ksymtab_patt],[[-e '[TD] sys_open' -e '[AB] _end']])
AC_DEFUN([_CR_CHECK_SYSTEM_MAP],[
  if test -n "$1" -a -r "$1" && grep cr_ksymtab_patt <"$1" >/dev/null 2>/dev/null; then
    if grep -B1 '[[AB]] _end' <"$1" | grep _stext >/dev/null 2>/dev/null; then
      # Reject "stripped" files (such as in FC2)
      # Recognized (poorly) by _stext and _end as last two entries.
      cr_stripped_maps="$cr_stripped_maps $1"
    else 
      LINUX_SYSTEM_MAP="$1"
      LINUX_SYMTAB_FILE="$1"
      LINUX_SYMTAB_CMD="cat $1 2>/dev/null"
    fi
  fi
])
AC_DEFUN([_CR_CHECK_VMLINUX],[
  AC_REQUIRE([AC_PROG_NM])
  if test -n "$1" -a -r "$1" && ($NM "$1" | grep cr_ksymtab_patt) >/dev/null 2>/dev/null; then
    LINUX_VMLINUX="$1"
    LINUX_SYMTAB_FILE="$1"
    LINUX_SYMTAB_CMD="$NM $1 2>/dev/null"
  fi
])
AC_DEFUN([CR_LINUX_SYMTAB],[  
  AC_REQUIRE([CR_CHECK_LINUX])
  AC_MSG_CHECKING([[for Linux kernel symbol table]])
  AC_SUBST([LINUX_SYMTAB_FILE])
  AC_SUBST([LINUX_SYMTAB_CMD])
  AC_SUBST([LINUX_SYMTAB_CONF])
  CR_GET_CACHE_VAR([LINUX_SYSTEM_MAP])
  CR_GET_CACHE_VAR([LINUX_VMLINUX])
  LINUX_SYMTAB_CMD=""
  # First try validating the user's (or cached) selection
  if test -n "$LINUX_SYSTEM_MAP" ; then
    _CR_CHECK_SYSTEM_MAP(["$LINUX_SYSTEM_MAP"])
    if test -z "$LINUX_SYMTAB_CMD"; then
      # The user specified a file, but we can't use it.  Abort.
      AC_MSG_RESULT([failed])
      AC_MSG_ERROR([Failed to validate "$LINUX_SYSTEM_MAP"])
    fi
  fi
  if test -z "$LINUX_SYMTAB_CMD" -a -n "$LINUX_VMLINUX" ; then
    _CR_CHECK_VMLINUX(["$LINUX_VMLINUX"])
    if test -z "$LINUX_SYMTAB_CMD"; then
      # The user specified a file, but we can't use it.  Abort.
      AC_MSG_RESULT([failed])
      AC_MSG_ERROR([Failed to validate "$LINUX_VMLINUX"])
    fi
  fi
  # Next try searching for System.map or vmlinux in standard locations
  # Note we use the kernel version found in the headers, not `uname -r`
  if test -z "$LINUX_SYMTAB_CMD" ; then
    for cr_file_pattern in "$LINUX_OBJ/@-$LINUX_VER" \
			   "/boot/@-$LINUX_VER"      \
			   "/@-$LINUX_VER"           \
			   "$LINUX_OBJ/@"            \
			   "/usr/lib/debug/boot/@-$LINUX_VER" \
			   "/usr/lib/debug/lib/modules/$LINUX_VER/@" \
			; do
      # Try System.map in the given location
      cr_file=`echo $cr_file_pattern | sed -e 's|@|System.map|'`
      _CR_CHECK_SYSTEM_MAP([$cr_file])
      if test -n "$LINUX_SYMTAB_CMD"; then
	break
      fi
      # Try vmlinux in the given location
      cr_file=`echo $cr_file_pattern | sed -e 's|@|vmlinux|'`
      _CR_CHECK_VMLINUX([$cr_file])
      if test -n "$LINUX_SYMTAB_CMD"; then
	break
      fi
    done
  fi
  # Announce our result
  if test -z "$LINUX_SYMTAB_CMD"; then
    LINUX_SYMTAB_CMD="true"
    AC_MSG_RESULT([failed])
    AC_CACHE_SAVE
    if test -n "$cr_stripped_maps"; then
      AC_MSG_WARN([Skipped stripped System.map file(s): $cr_stripped_maps])
      AC_MSG_ERROR([Failed to locate kernel symbol table.  Try installing the kernel-debuginfo package matching your kernel, or using --with-system-map or --with-vmlinux.])
    else
      AC_MSG_ERROR([Failed to locate kernel symbol table.  Try using --with-system-map or --with-vmlinux.])
    fi
  else
    AC_MSG_RESULT([$LINUX_SYMTAB_FILE])
    CR_SET_CACHE_VAR([LINUX_SYSTEM_MAP])
    CR_SET_CACHE_VAR([LINUX_VMLINUX])
    CR_CACHE_REVALIDATE([${LINUX_SYMTAB_FILE}],[ksymtab],[kernel symbol table])
  fi
])

# Now check for SYMTAB consistency w/ the kernel source
# XXX: Currently just check SMPness. Can this be more aggressive?
AC_DEFUN([CR_LINUX_SYMTAB_VALIDATE],[
  AC_MSG_CHECKING([for SMP kernel source])
  CR_CACHED_KERNEL_COMPILE([smp_source],[
	#ifdef CR_NEED_AUTOCONF_H
	  #include <linux/autoconf.h>
	#endif
	#ifndef CONFIG_SMP
	    choke me
	#endif
	],[])
  AC_MSG_RESULT([$cr_result]);
  cr_kernel_smp=$cr_result
  AC_MSG_CHECKING([for SMP kernel symbol table])
  cr_symtab_smp=no
  if test -n "`eval $LINUX_SYMTAB_CMD | grep del_timer_sync 2>/dev/null | grep -v try_to_del_`"; then
    cr_symtab_smp=yes
  fi
  AC_MSG_RESULT([$cr_symtab_smp]);
  if test "$cr_kernel_smp" != "$cr_symtab_smp"; then
    CR_CONFIG_REPORT
    if test "$cr_kernel_smp" = yes; then
      AC_MSG_ERROR([Kernel source is configured SMP but the kernel symbol table is not.  Consider specifying a symbol table with --with-system-map or --with-vmlinux.  Or, if using kernel sources that are configured by /boot/kernel.h, you may try --with-kernel-type=UP to force a uni-processor interpretation of the sources.])
    else
      AC_MSG_ERROR([Kernel source is configured uni-processor but the kernel symbol table is SMP.  Consider specifying a symbol table with --with-system-map or --with-vmlinux.  Or, if using kernel sources that are configured by /boot/kernel.h, you may try --with-kernel-type=SMP to force an SMP interpretation of the sources.])
    fi
  fi
])

# ------------------------------------------------------
# Helpers for CR_FIND_KSYM() and CR_FIND_EXPORTED_KSYM()
AC_DEFUN([_CR_KSYM_INIT_PATTS],[
  case "$CR_KARCH" in
    ppc64)
      CR_KSYM_PATTERN_DATA=['[bBdDgGrRsStTvV] ']
      CR_KSYM_PATTERN_CODE=['[dD] ']   dnl Function descriptor is data
      ;;
    *)
      CR_KSYM_PATTERN_DATA=['[bBdDgGrRsStTvV] ']
      CR_KSYM_PATTERN_CODE=['[tT] ']
      ;;
  esac
])
AC_DEFUN([_CR_KSYM_INIT_FILE],[dnl
pushdef([cr_name],CR_KSYM_[]AS_TR_CPP([$1]))
cr_name="${TOP_BUILDDIR}/.$1"
AC_SUBST_FILE(cr_name)
echo '/* This file is autogenerated - do not edit or remove */' > $cr_name
popdef([cr_name])
])
AC_DEFUN([_CR_KSYM_INIT_FILES],[dnl
  _CR_KSYM_INIT_FILE([import_decls])
  _CR_KSYM_INIT_FILE([import_calls])
])
AC_DEFUN([_CR_FIND_KSYM],[dnl
pushdef([cr_pattern],[${CR_KSYM_PATTERN_$2}$1$])dnl
`eval $LINUX_SYMTAB_CMD | sed -n -e "/cr_pattern/ {s/ .*//p;q;}"`[]dnl
popdef([cr_pattern])dnl
])

# CR_KSYM_FIXUP(NAME,TYPE)
# The System.map for ARM THUMB2 kernels does not currently
# include the lowest-bit-must-be-set on function pointers.
# This helps us fix that up at configure time, using a bitwise-OR as
# insurance against the possibility the bit IS set in later kernels.
#
# It also is a hook for any similar problems in the future.
AC_DEFUN([CR_KSYM_FIXUP],[
  if test "[$2]${HAVE_CONFIG_THUMB2_KERNEL}" = 'CODE1'; then
    [$1]=`$PERL -e "printf '%x', 1 | hex '$[$1]';"`
  fi
])
# Helper to collect data used by CR_KSYM_FIXUP
AC_DEFUN([_CR_KSYM_FIXUP],[
  if test "$CR_ARCH" = 'arm'; then
    CR_CHECK_KERNEL_MACRO([CONFIG_THUMB2_KERNEL])
  fi
])

# CR_FIND_KSYM(SYMBOL,TYPE,[DECL])
# ------------------------------------------------------
# Search System.map for address of the given symbol and deposit the
# neccessary bits in $CR_KSYM_IMPORT_DECLS and $CR_KSYM_IMPORT_CALLS
# to generate blcr_imports.h and imports.c, respectively:
# If the symbol is not found, nothing is deposited.
# If the symbol is not found to be not exported, then, as appropriate,
#  deposit a call to either _CR_IMPORT_KDATA or _CR_IMPORT_KCODE into
#  the file $CR_KSYM_IMPORT_CALLS
# If the symbol is found (exported or nor) but does not have a declaration
#  aftern including blcr_imports.h.in, then we deposit the supplied
#  declaration into the file $CR_KSYM_IMPORT_DECLS, or issue an error if
#  no declaration was supplied.
# We also conditionally define a symbol in $(CONFIG_HEADER) as follows:
# If not found, leave CR_K${TYPE}_${symbol} undefined
# If found to be exported, "#define CR_K${TYPE}_${symbol} 0"
# If found not to be exported, "#define CR_K${TYPE}_${symbol} 0x<value>"
# On return, cr_addr is set (or empty) the same way.
AC_DEFUN([CR_FIND_KSYM],[
  AC_REQUIRE([CR_LINUX_SYMTAB])
  AC_REQUIRE([_CR_KSYM_FIXUP])
  AC_REQUIRE([_CR_KSYM_INIT_PATTS])
  AC_MSG_CHECKING([[kernel symbol table for $1]])
  # Our cacheval is encoded with 'Y' or 'N' as the first char to indicate
  # if a declaration was found or not, and the address or 0 as the rest.
  pushdef([cr_cvname],cr_cv_ksymtab_[$1])[]dnl
  AC_CACHE_VAL([cr_cvname],[
    cr_cvname=_CR_FIND_KSYM([$1],[$2])
    if test -n "$cr_cvname"; then
      if eval $LINUX_SYMTAB_CMD | grep " __ksymtab_$1\$" >/dev/null ; then
        cr_cvname=0
      else
        CR_KSYM_FIXUP(cr_cvname,[$2])
      fi
      CR_TRY_KERNEL_COMPILE([
		#define IN_CONFIGURE 1
		#include "${TOP_SRCDIR}/include/blcr_imports.h.in"
      ],[int x = sizeof(&$1);],[cr_cvname="Y$cr_cvname"],[cr_cvname="N$cr_cvname"])
    fi
  ])
  cr_addr=''
  if test -z "$cr_cvname"; then
    cr_result='not found'
  else
    if expr "$cr_cvname" : N >/dev/null; then
      m4_ifval([$3],[cat >>$CR_KSYM_IMPORT_DECLS <<_EOF
[$3]
_EOF
      ], AC_MSG_ERROR([Found symbol $1 but no declaration -- please file a bug report.]))
    fi
    cr_result=`echo $cr_cvname | tr -d 'YN'`
    if test $cr_result = 0; then
      cr_result=exported
      cr_addr=0
    else
      cr_addr="0x$cr_result"
      echo "_CR_IMPORT_K$2($1, $cr_addr)" >>$CR_KSYM_IMPORT_CALLS
    fi
    AC_DEFINE_UNQUOTED(CR_K[$2]_[$1],$cr_addr,
		[Define to address of non-exported kernel symbol $1, or 0 if exported])
  fi
  popdef([cr_cvname])[]dnl
  AC_MSG_RESULT([$cr_result])
])


# CR_FIND_EXPORTED_KSYM(SYMBOL,TYPE)
# ------------------------------------------------------
# Search System.map for address of the given symbol and
# add the following to $(CONFIG_HEADER):
#   "#define CR_EXPORTED_K${TYPE}_foo 0x<value>"
# If not found, cr_addr will be 0 on return.
AC_DEFUN([CR_FIND_EXPORTED_KSYM],[
  AC_REQUIRE([CR_LINUX_SYMTAB])
  AC_REQUIRE([_CR_KSYM_FIXUP])
  AC_REQUIRE([_CR_KSYM_INIT_PATTS])
  AC_MSG_CHECKING([[kernel symbol table for exported $1]])
  pushdef([cr_cvname],cr_cv_ksymtab_exp_[$1])[]dnl
  AC_CACHE_VAL([cr_cvname],[
    cr_cvname=_CR_FIND_KSYM([$1],[$2])
    if test -n "$cr_cvname" && eval $LINUX_SYMTAB_CMD | grep " __ksymtab_$1\$" >/dev/null; then
      : # keep it
    else
      cr_cvname=''
    fi])
  if test -n "$cr_cvname"; then
    CR_KSYM_FIXUP(cr_cvname,[$2])
    cr_result="$cr_cvname"
    cr_addr="0x$cr_cvname"
    AC_DEFINE_UNQUOTED(CR_EXPORTED_K[$2]_[$1],$cr_addr,
		[Define to address of exported kernel symbol $1])
  else
    cr_result='not found'
    cr_addr=0
  fi
  popdef([cr_cvname])[]dnl
  AC_MSG_RESULT([$cr_result])
])

# Find how to rpmbuild (if possible at all)
AC_DEFUN([CR_PROG_RPMBUILD],[
  AC_PATH_PROGS([RPMBUILD],[rpmbuild rpmb rpm],[none],[$PATH:/usr/lib/rpm])
  if $RPMBUILD -bs 2>&1 | grep 'no spec' >/dev/null 2>/dev/null; then
    :
  else
    RPMBUILD=none
  fi
])

# CR_STACK_DIRECTION()
# ------------------------------------------------------
# Determine direction of stack grown
# Copied (and slightly modified) from autoconf's alloca code
AC_DEFUN([CR_STACK_DIRECTION],[
  pushdef([cr_cvname],cr_cv_check_stack_direction)[]dnl
  AC_CACHE_CHECK([[for direction of stack growth]],cr_cvname,[
   AC_TRY_RUN([
    int find_stack_direction (void) {
      static char *addr = 0;
      auto char dummy;
      if (addr == 0) {
        addr = &dummy;
        return find_stack_direction();
      } else {
        return (&dummy > addr) ? 0 : 1;
      }
    }
    int main(void) {
      return find_stack_direction();
    }], [cr_cvname=up],
	[cr_cvname=down],
	[CR_CROSS_VAR(cross_stack_direction)
	 if test $cross_stack_direction = 1; then
	   cr_cvname=up
	 else
	   cr_cvname=down
	 fi])
  ])
  if test $cr_cvname = up; then
    cr_stack_direction=1
  else
    cr_stack_direction=-1
  fi
  AC_DEFINE_UNQUOTED([CR_STACK_GROWTH], $cr_stack_direction)
  AH_TEMPLATE([CR_STACK_GROWTH],[Positive if stack grows up, negative if it grows down])
  popdef([cr_cvname])[]dnl
])

# CR_CHECK_KMALLOC_MAX()
# ------------------------------------------------------
# Determine maximum kmalloc allocation
AC_DEFUN([CR_CHECK_KMALLOC_MAX],[
  pushdef([cr_cvname],cr_cv_kconfig_kmalloc_max)[]dnl
  AC_CACHE_CHECK([[kernel for maximum kmalloc() allocation]],cr_cvname,[
    cr_kmalloc_default="131072 (default)"
    cr_header="${LINUX_OBJ}/include/generated/autoconf.h"
    test -e "$cr_header" || cr_header="${LINUX_OBJ}/include/linux/autoconf.h"
    cr_cvname=`eval "$CPP $CPPFLAGS -I${LINUX_OBJ}/include \
			-include ${cr_header} \
			${LINUX_OBJ}/include/linux/kmalloc_sizes.h" 2>/dev/null | \
		$PERL -n -e 'BEGIN {$max=0;}' \
			 -e ['if (/CACHE\s*\(\s*([0-9]+)\s*\)/ && ($][1 > $max)) { $max = $][1; }'] \
			 -e 'END {print "$max\n";}'`
    if test $? = 0 -a "$cr_cvname" != 0; then
      : # OK - keep it
    else
      cr_cvname="$cr_kmalloc_default"
    fi
  ])
  cr_kmalloc_max=`echo $cr_cvname | cut -d' ' -f1`
  AC_DEFINE_UNQUOTED([CR_KMALLOC_MAX], $cr_kmalloc_max, [Maximum legal size to kmalloc()])
  popdef([cr_cvname])[]dnl
])

# CR_CROSS_VAR(varname)
# ------------------------------------------------------
# Check for a value of given variable, or AC_MSG_ERROR if unset
AC_DEFUN([CR_CROSS_VAR],[
  if test "${$1-unset}" = unset; then
    AC_MSG_ERROR([When cross-compiling, variable $1 must be set.])
  fi
])

# CR_COMPUTE_INT(VAR,EXPRESSION,[INCLUDES])
# ------------------------------------------------------
# Compute a constant (ulong) C expression, even when cross compiling
AC_DEFUN([CR_COMPUTE_INT],[
  pushdef([cr_cvname],cr_cv_compute_int_[$1])[]dnl
  AC_CACHE_CHECK([[for value for $1]],cr_cvname,[
    cr_cvname="not found"
    m4_ifdef([AC_COMPUTE_INT],
	[AC_COMPUTE_INT([cr_cvname], [$2], [$3])],
	[_AC_COMPUTE_INT([$2], [cr_cvname], [$3])])
  ])
  if test "$cr_cvname" != "not found"; then
    [$1]="$cr_cvname"
  fi
  popdef([cr_cvname])[]dnl
])

# CR_DEFINE_INT(VAR,EXPRESSION,[INCLUDES],[TEMPLATE_TEXT])
# ------------------------------------------------------
# Like CR_COMPUTE_INT, but also doing AC_DEFINE
AC_DEFUN([CR_DEFINE_INT],[
  pushdef([cr_varname],[$1])[]dnl
  cr_varname=""
  CR_COMPUTE_INT(cr_varname,[$2],[$3])
  if test -n "$cr_varname"; then
    AC_DEFINE_UNQUOTED(cr_varname, $cr_varname)
    AH_TEMPLATE(cr_varname,m4_default([$4], [[Computed value of '$2']]))
  fi
  popdef([cr_varname])[]dnl
])

# CR_COMPUTE_KERNEL_INT(VAR,EXPRESSION,[INCLUDES])
# ------------------------------------------------------
# Compute a constant (ulong) C expression, even when cross compiling, using 
# the kernel compilation environment.  I had to kludge this to get it to work
# by setting the cross_compiling flag to yes.  Otherwise, the program that
# autoconf builds tries to use stdio calls, and the test fails.
#
# By setting this flag, autoconf uses _AC_COMPUTE_INT_RUN to evaluate the
# expression, rather than _AC_COMPUTE_INT_COMPILE, which uses stdio by default.
AC_DEFUN([CR_COMPUTE_KERNEL_INT],[
  AC_REQUIRE([CR_SET_KCFLAGS])
  pushdef([cr_cvname],cr_cv_compute_kernel_int_[$1])[]dnl
  SAVE_CPP=$CPP
  SAVE_CPPFLAGS=$CPPFLAGS
  SAVE_cross_compiling="$cross_compiling"
  CPP="$KCC -E"
  CPPFLAGS="$KCFLAGS"
  cross_compiling="yes"
  AC_CACHE_CHECK([[kernel for value for $1]],cr_cvname,[
    cr_cvname="not found"
    m4_ifdef([AC_COMPUTE_INT],
	[AC_COMPUTE_INT([cr_cvname], [$2], [$3])],
	[_AC_COMPUTE_INT([$2], [cr_cvname], [$3])])
  ])
  CPP=$SAVE_CPP
  CPPFLAGS=$SAVE_CPPFLAGS
  cross_compiling="$SAVE_cross_compiling"
  if test "$cr_cvname" != "not found"; then
    [$1]="$cr_cvname"
  fi
  popdef([cr_cvname])[]dnl
])

# CR_DEFINE_KERNEL_INT(VAR,EXPRESSION,[INCLUDES],[TEMPLATE_TEXT])
# ------------------------------------------------------
# Like CR_COMPUTE_KERNEL_INT, but also doing AC_DEFINE
AC_DEFUN([CR_DEFINE_KERNEL_INT],[
  pushdef([cr_varname],[$1])[]dnl
  cr_varname=""
  CR_COMPUTE_KERNEL_INT(cr_varname,[$2],[$3])
  if test -n "$cr_varname"; then
    AC_DEFINE_UNQUOTED(cr_varname, $cr_varname)
    AH_TEMPLATE(cr_varname,m4_default([$4], [[Computed value of '$2']]))
  fi
  popdef([cr_varname])[]dnl
])
