noinst_PROGRAMS = pipe_counting

if CR_INSTALLED_LIBCR
libcr_ldflag = -L$(libdir)
else
libcr_ldflag = -L$(top_builddir)/libcr
pipe_counting_DEPENDENCIES = $(top_builddir)/libcr/libcr.la
$(top_builddir)/libcr/libcr.la: FORCE
	@$(MAKE) $(AM_MAKEFLAGS) --no-print-directory -C $(top_builddir)/libcr libcr.la
.PHONY: FORCE
FORCE:
endif

LDADD = $(libcr_ldflag) -lcr -lpthread @CR_CLIENT_LDADD@
AM_CFLAGS = -Wall
MOSTLYCLEANFILES = context.*
