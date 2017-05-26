bin_PROGRAMS = cr_checkpoint

if CR_INSTALLED_LIBCR
LDADD = -L$(libdir) -lcr @CR_CLIENT_LDADD@
INCLUDES = -I$(includedir)
else
LDADD = -L$(top_builddir)/libcr -lcr @CR_CLIENT_LDADD@
INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include
LIBCR = $(top_builddir)/libcr/libcr.la
cr_checkpoint_DEPENDENCIES = $(LIBCR)
$(LIBCR):
	@$(MAKE) $(AM_MAKEFLAGS) --no-print-directory -C $(@D)
endif
AM_CFLAGS = -Wall
AM_CPPFLAGS = @CR_NDEBUG@

EXTRA_DIST  = cr_checkpoint.extraman

if CR_BUILD_MAN
manfiles    = cr_checkpoint.1
man_MANS    = $(manfiles)

# delete man page sources during 'clean', since they're generated files.
CLEANFILES          = $(manfiles)

cr_checkpoint.1: $(top_srcdir)/util/cr_checkpoint/cr_checkpoint.extraman cr_checkpoint
	@PERL@ $(top_srcdir)/man/help2man \
            -n "checkpoints a process, process group, or session." \
            -i $(top_srcdir)/util/cr_checkpoint/cr_checkpoint.extraman -N \
            -o cr_checkpoint.1 -s 1 -S "Berkeley Lab Checkpoint/Restart" \
            $(top_builddir)/util/cr_checkpoint/cr_checkpoint

# target for man page writers: stores man page in 'man1' subdir, so you can 
# peruse it with 'man foo' (assuming you have '.' in your MANPATH)
dev: clean $(manfiles)
	if test ! -d man1; then mkdir man1; fi
	cp $(manfiles) man1

dev-clean:
	-rm -rf man1 $(manfiles)
endif # CR_BUILD_MAN

