noinst_PROGRAMS = io_bench

if CR_INSTALLED_LIBCR
libcr_ldflag = -L$(libdir)
libcr_includes = -I$(includedir)
else
libcr_ldflag = -L$(top_builddir)/libcr
libcr_includes = -I$(top_srcdir)/include -I$(top_builddir)/include
io_bench_DEPENDENCIES = $(top_builddir)/libcr/libcr.la
$(top_builddir)/libcr/libcr.la: FORCE
	@$(MAKE) $(AM_MAKEFLAGS) --no-print-directory -C $(top_builddir)/libcr libcr.la
.PHONY: FORCE
FORCE:
endif

LDADD = $(libcr_ldflag) -lcr -lpthread @CR_CLIENT_LDADD@
INCLUDES = $(libcr_includes)
AM_CFLAGS = -Wall
MOSTLYCLEANFILES = context.*
