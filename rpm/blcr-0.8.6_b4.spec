Name: blcr
Version: 0.8.6_b4
Release: 1
Summary: Berkeley Lab Checkpoint/Restart for Linux
Url: http://ftg.lbl.gov/checkpoint

## Documentation for end-users:

# You can "--define 'with_autoreconf 1'" on the rpmbuild commandline
# to force use of site-specific or distro-specific autotools.
# The default is NO.

# You can "--define 'with_multilib 0'" to disable configuration of
# BLCR with --enable-multilib (which builds 32-bit libs on a 64-bit
# platform, in addition to the 64-bit ones).
# This is ignored on 32-bit targets.

# If not building against the running kernel, then you'll need to
# add exactly one of the following two mutually exclusive options
# to your rpmbuild (or rpm) command line:
# 1) "--define 'kernel_ver <SOME_VERSION>'"
# 2) "--define 'kernel_obj /lib/modules/<SOME_VERSION>/build'",
# When using either of these, you may also  want/need to add
# "--define 'kernel_src /usr/src/linux-<SOME_VERSION>'".
# Of course the values assigned to kernel_obj and kernel_src
# above are just examples.
# Use of "--define 'kernel_type <SOMETHING>'" will allow one to
# override /boot/kernel.h, where SOMETHING=SMP, UP, etc. if using
# kernel sources configured via that file.

## END of end-user documentation

# Control use of local autotools
%define run_autoreconf %{?with_autoreconf:1}%{!?with_autoreconf:0}

# Read next line as "kernel = defined(kernel_ver) ? kernel_ver : `uname -r`"
%define kernel %{?kernel_ver:%{kernel_ver}}%{!?kernel_ver:%(uname -r)}

# Name the kernel modules package w/o dashes in the kernel version:
%define modsubpkg modules_%(echo %{kernel} | tr - _)
%define moduledir /lib/modules/%{kernel}/extra

# Name of the unpacked source directory and stem of the tarball name
%define distname %{name}-%{version}

# Disable RedHat's automatic build of a debuginfo subpackage:
%define debug_package %{nil}

# Macro for scanning configure arguments
# First arg is default value, second is option name.
%define is_enabled() %(X=%1; eval set -- ; for x in "$@"; do if [ x"$x" = "x--disable-%2" -o x"$x" = "x--enable-%2=no" ]; then X=0; elif [ x"$x" = "x--enable-%2" -o x"$x" = "x--enable-%2=yes" ]; then X=1; fi; done; echo $X)

# Are we building shared and/or static libs?
%define build_shared %{is_enabled 1 shared}
%if %{build_shared}
%define build_static %{is_enabled 0 static}
%else
%define build_static 1
%endif

# Are we building static executables
%define build_all_static %{is_enabled 0 all-static}

# Are we installing the test-suite
%define build_testsuite %{is_enabled 1 testsuite}

# Are we building both 32- and 64-bit libcr?
%ifarch x86_64 ppc64
  # Honour with_multilib if set, otherwise use is_enabled
  %define build_libdir32 %{?with_multilib:%{with_multilib}}%{!?with_multilib:%{is_enabled 1 multilib}}
%else
  %define build_libdir32 0
%endif

# Where to put 32-bit libs on a 64-bit platform
%if %{build_libdir32}
  %define libdir32 %(echo %{_libdir} | sed -e s/lib64/lib/)
%endif

Group: System Environment/Base
License: GPLv2+
Source: %{distname}.tar.gz
BuildRoot: %{_tmppath}/buildroot-%{name}-%{version}
BuildRequires: perl sed
Requires(post): /sbin/chkconfig
Requires(preun): /sbin/chkconfig
# Kernel and asm support only ported to certain architectures
# i386 is omitted because it lacks required atomic instructions
ExclusiveArch: i486 i586 i686 athlon x86_64 ppc ppc64 arm
ExclusiveOs: Linux
Requires: %{name}-modules >= %{version}-%{release}

# DON'T require since many clusters are built w/ non-RPM kernels:
# BuildPreReq: kernel-source = %{kernel}

%description
Berkeley Lab Checkpoint/Restart for Linux (BLCR)

This package implements system-level checkpointing of scientific applications
in a manner suitable for implementing preemption, migration and fault recovery
by a batch scheduler.

BLCR includes documented interfaces for a cooperating applications or
libraries to implement extensions to the checkpoint system, such as
consistent checkpointing of distributed MPI applications.
Using this package with an appropriate MPI implementation, the vast majority
of scientific applications which use MPI for communication on Linux clusters
are checkpointable without any modifications to the application source code.

You must also install the %{name}-libs package and a %{name}-modules_* package
matching your kernel version.
%prep
%setup -q -n %{distname}

%build

# Work with rpm's various botched ideas of host vs. target
%define _host_cpu %{_target_cpu}
%define _host %{_target}
%define _build_cpu %{_target_cpu}
%define _build %{_target}

# Allow user to request use of local autotools
%if %{run_autoreconf}
autoreconf --force --install
%endif

# VPATH build required to ensure --enable-multilib will work
mkdir -p builddir
cd builddir
ln -s ../configure .

# Configure the thing
# Order arguments such that user's configure arguments can disable multilib, and
# enable the config-report, but doesn't clobber kernel version info from the
# rpmbuild command line
%configure  \
	--srcdir=.. \
	%{?libdir32:--enable-multilib} \
	--enable-testsuite \
	--disable-config-report \
	 \
	%{!?kernel_obj:--with-linux=%{kernel}}%{?kernel_obj:--with-linux=%{kernel_obj}} \
	%{?kernel_src:--with-linux-src=%{kernel_src}} \
	%{?kernel_type:--with-kernel-type=%{kernel_type}}

# Now build it
make

%clean
rm -Rf ${RPM_BUILD_ROOT}

%install
cd builddir
rm -Rf ${RPM_BUILD_ROOT}
make install-strip DESTDIR=${RPM_BUILD_ROOT}
# Ensure man pages are gzipped, regardless of brp-compress
if [ -n "${RPM_BUILD_ROOT}" -a "${RPM_BUILD_ROOT}" != "/" ]; then
  find ${RPM_BUILD_ROOT}/%{_mandir} -name '*.[1-9]' | xargs gzip -9
fi
# Install the init script
make -C etc install DESTDIR=${RPM_BUILD_ROOT}

# On some systems rpmbuild dislikes having an RPATH that points
# to a system directory.  Some versions of libtool get this right
# on their own, while others don't.
# So, we try to clean it up here if we have chrpath.
if chrpath --version >& /dev/null; then
%if !%{build_all_static}
  chrpath -d ${RPM_BUILD_ROOT}/%{_bindir}/cr_checkpoint
  chrpath -d ${RPM_BUILD_ROOT}/%{_bindir}/cr_restart
%endif
%if !%{build_all_static} && %{build_testsuite}
  list=`make -C tests --no-print-directory echoval VARNAME=testsexec_PROGRAMS`
  ( cd ${RPM_BUILD_ROOT}/%{_libexecdir}/blcr-testsuite && chrpath -d $list )
%endif
  : # ensure non-empty body
fi

%post
if [ $1 = 1 ]; then
 if [ -x /usr/lib/lsb/install_initd ]; then
  /usr/lib/lsb/install_initd /etc/init.d/blcr
 else
  /sbin/chkconfig --add blcr
 fi
fi
exit 0

%preun
if [ $1 = 0 ]; then
 if [ -x /usr/lib/lsb/remove_initd ]; then
  /usr/lib/lsb/remove_initd /etc/init.d/blcr
 else
  /sbin/chkconfig --del blcr
 fi
fi
exit 0

%files
%defattr(-,root,root)
%doc util/license.txt
%doc COPYING
%doc NEWS
%doc doc/README
%doc doc/html
%doc %{_mandir}/man1/cr_checkpoint.1.gz
%doc %{_mandir}/man1/cr_restart.1.gz
%doc %{_mandir}/man1/cr_run.1.gz
%{_bindir}/cr_checkpoint
#%{_bindir}/cr_info
%{_bindir}/cr_restart
%{_bindir}/cr_run
#%{_libexecdir}/vmadcheck
%{_sysconfdir}/init.d/blcr

%if %{build_shared}
#
# Libs in a separate package
# TODO: separate 32-bit package in multilib builds
#
%package libs
Group: System Environment/Libraries
Summary: Libraries for Berkeley Lab Checkpoint/Restart for Linux
License: LGPLv2+
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description libs
Runtime libraries for Berkeley Lab Checkpoint/Restart for Linux (BLCR)

%post libs -p /sbin/ldconfig
%postun libs -p /sbin/ldconfig

%files libs
%defattr(-,root,root)
%doc libcr/license.txt
%doc COPYING.LIB
%doc NEWS
%{_libdir}/libcr.so.0
%{_libdir}/libcr.so.0.5.5
%{_libdir}/libcr_run.so.0
%{_libdir}/libcr_run.so.0.5.5
%{_libdir}/libcr_omit.so.0
%{_libdir}/libcr_omit.so.0.5.5
%endif
%if %{build_shared} && %{build_libdir32}
%{libdir32}/libcr.so.0
%{libdir32}/libcr.so.0.5.5
%{libdir32}/libcr_run.so.0
%{libdir32}/libcr_run.so.0.5.5
%{libdir32}/libcr_omit.so.0
%{libdir32}/libcr_omit.so.0.5.5
%endif

#
# Separate -devel package
# TODO: separate 32-bit package in multilib builds
#
%package devel
%if %{build_shared}
Requires: %{name}-libs = %{version}-%{release}
%endif
Group: Development/Libraries
Summary: Header and object files for Berkeley Lab Checkpoint/Restart for Linux
License: LGPLv2+

%description devel
Header and object files for Berkeley Lab Checkpoint/Restart for Linux
You must also install the %{name}-libs package.

%files devel
%defattr(-,root,root)
%doc README.devel
%doc libcr/license.txt
%doc COPYING.LIB
%{_includedir}/blcr_common.h
%{_includedir}/blcr_errcodes.h
%{_includedir}/blcr_ioctl.h
%{_includedir}/blcr_proc.h
%{_includedir}/libcr.h
# .la files
%{_libdir}/libcr.la
%{_libdir}/libcr_run.la
%{_libdir}/libcr_omit.la
%if %{build_libdir32}
%{libdir32}/libcr.la
%{libdir32}/libcr_run.la
%{libdir32}/libcr_omit.la
%endif
# .so files
%if %{build_shared}
%{_libdir}/libcr.so
%{_libdir}/libcr_run.so
%{_libdir}/libcr_omit.so
%endif
%if %{build_shared} && %{build_libdir32}
%{libdir32}/libcr.so
%{libdir32}/libcr_run.so
%{libdir32}/libcr_omit.so
%endif
# .a files
%if %{build_static}
%{_libdir}/libcr.a
%{_libdir}/libcr_run.a
%{_libdir}/libcr_omit.a
%endif
%if %{build_static} && %{build_libdir32}
%{libdir32}/libcr.a
%{libdir32}/libcr_run.a
%{libdir32}/libcr_omit.a
%endif

#
# Kernel modules as a separate package
#
%package %{modsubpkg}
Group: System Environment/Kernel
Summary: Kernel modules for Berkeley Lab Checkpoint/Restart for Linux
Provides: %{name}-modules = %{version}-%{release}
Requires: %{name} = %{version}
# DON'T require since many clusters are built w/ non-RPM kernels:
# Requires: kernel = %{kernel}

%description %{modsubpkg}
Kernel modules for Berkeley Lab Checkpoint/Restart for Linux (BLCR)
These kernel modules are built for Linux %{kernel}.

You must also install the base %{name} package.
%post %{modsubpkg}
/sbin/depmod -a -F /boot/System.map-%{kernel} %{kernel}
if [ $1 = 2 ]; then
 # conditional reload on upgrade
 /etc/init.d/blcr reload >& /dev/null
fi
exit 0
%preun %{modsubpkg}
if [ $1 = 0 ]; then
 /etc/init.d/blcr stop >& /dev/null
fi
exit 0
%postun %{modsubpkg}
if [ $1 = 0 ]; then
 /sbin/depmod -a -F /boot/System.map-%{kernel} %{kernel}
fi
exit 0

%files %{modsubpkg}
%defattr(-,root,root)
%doc cr_module/license.txt
%doc COPYING
%dir %{moduledir}
%{moduledir}/blcr.ko
%{moduledir}/blcr_imports.ko

##
## testsuite as an additional package if configured in
##
%if %{build_testsuite}
%package testsuite
Group: System Environment/Base
Summary: Test suite for Berkeley Lab Checkpoint/Restart for Linux
License: GPLv2+
Requires: %{name} = %{version}
%description testsuite
This package includes tests for Berkeley Lab Checkpoint/Restart for Linux
%files testsuite
%defattr(-,root,root)
%doc tests/license.txt
%doc COPYING
%{_libexecdir}/blcr-testsuite
%endif

%changelog
* Mon Oct 20 2014 Paul H. Hargrove <PHHargrove@lbl.gov> 0.8.6_b4-1
- Add this autogenerated %%changelog to quiet rpmlint
