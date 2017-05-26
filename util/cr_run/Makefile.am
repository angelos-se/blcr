bin_SCRIPTS = cr_run
cr_run: cr_run.in Makefile
	@PERL@ -pe 's|\@top_builddir\@|@TOP_BUILDDIR@|g;\
		    s|\@libcr_major\@|@LIBCR_MAJOR@|;\
		    s|\@release_version\@|@CR_RELEASE_VERSION@|;' $(srcdir)/$@.in >$@
	chmod +x $@

CLEANFILES  = cr_run
EXTRA_DIST  = cr_run.in cr_run.1

install-exec-hook:
	@PERL@ -ni -e 'print unless /###do_not_install###/' $(DESTDIR)$(bindir)/cr_run

if CR_BUILD_MAN
man_MANS    = cr_run.1

# target for man page writers: stores man page in 'man1' subdir, so you can 
# peruse it with 'man foo' (assuming you have '.' in your MANPATH)
dev: 
	if test ! -d man1; then mkdir man1; fi
	cp cr_run.1 man1

dev-clean:
	-rm -rf man1 
endif # CR_BUILD_MAN
