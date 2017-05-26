EXTRA_DIST = license.txt blcr.rc

module_dir=$(shell echo '@CR_MODULE_DIR@' | sed -e 's/@LINUX_VER@/`uname -r`/')

install-exec-local:
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)/init.d
	@PERL@ -pe 's|\@MODULE_DIR\@|$(module_dir)|' \
		< $(srcdir)/blcr.rc > $(DESTDIR)$(sysconfdir)/init.d/blcr
	chmod 755 $(DESTDIR)$(sysconfdir)/init.d/blcr
uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/init.d/blcr
