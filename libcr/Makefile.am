lib_LTLIBRARIES = libcr.la libcr_run.la libcr_omit.la

CR_LIB_VERSION = -version-info @LIBTOOL_INTERFACE@:@LIBTOOL_REVISION@:@LIBTOOL_AGE@

noinst_HEADERS = \
		cr_syscall.h\
		cr_private.h\
		cr_rb_lock.h\
		cr_trace.h\
		cr_yield.h

if CR_HAVE_FTB
libcr_ftb_sources = cr_ftb.c
endif

libcr_la_SOURCES = \
		$(libcr_ftb_sources) \
		cr_async.c\
		cr_core.c\
		cr_cs.c\
		cr_pthread.c\
		cr_sig_sync.c\
		cr_syscall.c\
		cr_trace.c\
		cr_strerror.c\
		cr_request.c\
		cr_omit.c\
		cr_run.c
libcr_la_LIBADD = -ldl -lpthread @CR_FTB_LDADD@
libcr_la_LDFLAGS = $(CR_LIB_VERSION) @CR_FTB_LDFLAGS@
libcr_la_CFLAGS = @CR_LIBCR_CFLAGS@ @CR_FTB_INCLUDES@ $(AM_CFLAGS)

if LIBCR_TRACING
libcr_tracing = cr_trace.c cr_sig_sync.c
endif

# "target" libs
libcr_run_la_SOURCES = cr_run.c $(libcr_tracing)
libcr_run_la_LIBADD = -ldl
libcr_run_la_LDFLAGS = $(CR_LIB_VERSION)
libcr_run_la_CFLAGS = @CR_LIBCR_CFLAGS@ $(AM_CFLAGS) -DLIBCR_SIGNAL_ONLY
libcr_omit_la_SOURCES = cr_omit.c $(libcr_tracing)
libcr_omit_la_LIBADD = -ldl
libcr_omit_la_LDFLAGS = $(CR_LIB_VERSION)
libcr_omit_la_CFLAGS = @CR_LIBCR_CFLAGS@ $(AM_CFLAGS) -DLIBCR_SIGNAL_ONLY

if CR_ENABLE_SHARED
# XXX: should split off arch-specific portions of the symbol filter
cr_lib_check:
	@if (nm -D .libs/libcr_run.so | egrep -v \
			-e ' [^TDB] ' \
			-e ' [TDB] \.?crsig_' \
			-e ' [TDB] \.?cri?_run_' \
			-e ' [TD] \.?_init' \
			-e ' [TD] \.?_fini' \
			-e ' D __data_start' \
			-e ' D _edata' \
			-e ' B _{1,2}bss_start_{0,2}' \
			-e ' B _{1,2}bss_end_{0,2}' \
			-e ' B _{1,2}end_{0,2}' \
			-e ' T _(save|rest)[gf]pr_[123][0-9](_x)?'); \
		then \
		echo "ERROR: libcr_run.so leaks symbol(s), above."; \
		exit 1; \
	 fi
if CRI_DEBUG
# Curently only do this for a debug build, due to fragile nm and grep arguments
cr_lib_check = cr_lib_check
endif
endif

INCLUDES =\
		-D_GNU_SOURCE\
		-D_REENTRANT\
		-I$(top_builddir)/include\
		-I$(top_srcdir)/include\
		-I$(srcdir)/arch/@CR_LIBARCH@/

EXTRA_DIST=arch license.txt cr_libinit.c
dist-hook:
	find $(distdir)/arch -name CVS | xargs rm -rf
all-local: $(cr_lib_check)
	if test -d $(top_builddir)/libcr32/libcr; then $(MAKE) $(AM_MAKEFLAGS) -C $(top_builddir)/libcr32/libcr all; fi
