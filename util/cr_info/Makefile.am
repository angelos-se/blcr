vmadump_dir = @TOP_BUILDDIR@/vmadump4

bin_SCRIPTS = cr_info
cr_info: cr_info.in Makefile
	@PERL@ -pe 's|\@vmadump_dir\@|$(vmadump_dir)|;\
		    s|\@release_version\@|@CR_RELEASE_VERSION@|;' $(srcdir)/$@.in >$@
	chmod +x $@

CLEANFILES  = cr_info
EXTRA_DIST  = cr_info.in

# Deal with the possibility that we install to a different directory
# than we had at build time by delaying expansion of @libexecdir@
install-exec-hook:
	@PERL@ -ni -e 's|\@libexecdir\@|$(libexecdir)|; \
		     print unless /###do_not_install###/' \
				$(DESTDIR)$(bindir)/cr_info

