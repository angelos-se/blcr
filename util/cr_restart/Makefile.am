bin_PROGRAMS = cr_restart

if CR_INSTALLED_LIBCR
LDADD = -L$(libdir) -lcr @CR_CLIENT_LDADD@
INCLUDES = -I$(includedir)
else
LDADD = -L$(top_builddir)/libcr -lcr @CR_CLIENT_LDADD@
INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include
LIBCR = $(top_builddir)/libcr/libcr.la
cr_restart_DEPENDENCIES = $(LIBCR)
$(LIBCR):
	@$(MAKE) $(AM_MAKEFLAGS) --no-print-directory -C $(@D)
endif
AM_CFLAGS = -Wall
AM_CPPFLAGS = @CR_NDEBUG@

EXTRA_DIST  = cr_restart.extraman

if CR_BUILD_MAN
manfiles    = cr_restart.1
man_MANS    = $(manfiles)

# delete man page sources during 'clean', since they're generated files.
CLEANFILES          = $(manfiles)

cr_restart.1: $(top_srcdir)/util/cr_restart/cr_restart.extraman cr_restart
	@PERL@ $(top_srcdir)/man/help2man \
            -n "restarts a process, process group, or session from a checkpoint file."\
            -i $(top_srcdir)/util/cr_restart/cr_restart.extraman -N \
            -o cr_restart.1 -s 1 -S "Berkeley Lab Checkpoint/Restart" \
            $(top_builddir)/util/cr_restart/cr_restart

# target for man page writers: stores man page in 'man1' subdir, so you can 
# peruse it with 'man foo' (assuming you have '.' in your MANPATH)
dev: clean $(manfiles)
	if test ! -d man1; then mkdir man1; fi
	cp $(manfiles) man1

dev-clean:
	-rm -rf man1 $(manfiles)
endif # CR_BUILD_MAN
