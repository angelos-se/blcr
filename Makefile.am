###
# Optional directories
###
# Example init script
if CR_BUILD_ETC
etc_dir = etc
endif
# Kernel modules
if CR_BUILD_MODULES
module_dirs = blcr_imports cr_module
module_blcr = @TOP_BUILDDIR@/cr_module/kbuild/blcr.ko
module_imports = @TOP_BUILDDIR@/blcr_imports/kbuild/blcr_imports.ko
endif
# Libraries
if CR_BUILD_LIBCR
libcr_dirs = libcr
endif
# User-space utils
if CR_BUILD_UTIL
util_dirs = util man
endif
# Tests
if CR_BUILD_TESTS
tests_dirs = tests
endif
# Contrib
if CR_BUILD_CONTRIB
contrib_dirs = contrib
endif
# cr_info is disabled since vmadcheck is currently broken -PHH 12.5.03
if BUILD_CR_INFO
cr_info=cr_info
else
cr_info=
endif

if CRI_DEBUG
cr_ktrace_mask = 0xffff
cr_read_fault_rate = 0
cr_write_fault_rate = 0
insmod_blcr_args = \
	cr_ktrace_mask=$(cr_ktrace_mask) \
	cr_read_fault_rate=$(cr_read_fault_rate) \
	cr_write_fault_rate=$(cr_write_fault_rate)
endif


# Never build as part of 'all', 'check' or 'install', but may be built manually.
NOAUTO_DIRS = examples rpm doc

# include comes first to ensure up-to-date headers for all later steps
# module_dirs preceedes . due to _depmod rule
# libcr_dirs preceedes util_dirs to ensure up-to-date libs for the utils
SUBDIRS = include $(module_dirs) $(libcr_dirs) $(util_dirs) \
	  $(etc_dir) $(tests_dirs) $(contrib_dirs) .
# Automake has been observed to build DIST_SUBDIRS incorrectly, so we build it
DIST_SUBDIRS = man include libcr util tests contrib etc \
		blcr_imports cr_module vmadump4 $(NOAUTO_DIRS)
EXTRA_DIST = README README.devel LICENSE.txt COPYING COPYING.LIB NEWS \
	     config/kbuild.mak.in config/cross_helper.c autogen.sh \
	     README.FTB

# Clean out CVS droppings
dist-hook:
	find $(distdir) -depth -name CVS -type d -exec rm -Rf '{}' \;
	find $(distdir) -name .#\* -type f -exec rm -f '{}' \;

# PHH 07_Dec_2004: disabled, since our dist_blcr script does even more
# We want to check that our RPM building works
#distcheck-hook:
#	$(MAKE) $(AM_MAKEFLAGS) -C rpm $@

# Target to build the source rpm for distribution, creating the
# source tarball (blcr-X.Y.Z.tar.gz) as a prereq.
dist-srpm:
	$(MAKE) $(AM_MAKEFLAGS) -C rpm srpm

# Target to build source and binary rpms
rpms:
	$(MAKE) $(AM_MAKEFLAGS) -C rpm all

if CR_BUILD_MODULES
# Target to build the kernel modules
modules: $(CONFIG_HEADER)
	$(MAKE) $(AM_MAKEFLAGS) --no-print-directory -C include
	for dir in $(module_dirs); do \
	  $(MAKE) $(AM_MAKEFLAGS) --no-print-directory -C $$dir || exit 1; \
	done

# Target to build and check the kernel modules against System.map
_depmod: # No rebuild of modules
	@@LINUX_SYMTAB_CMD@ | env NM='$(NM)' @PERL@ -- $(srcdir)/contrib/cr_depmod $(module_imports) $(module_blcr) >.depmod.err 2>&1; \
	 rc=$$?; \
	 if test $$rc '!=' 0; then \
	     echo "ERROR: 'depmod' check FAILED (rc=$$rc)"; \
	     cat .depmod.err; \
	 else \
	     echo "'depmod' check PASSED"; \
	 fi >&2; \
	 rm -f .depmod.err; \
	 exit $$rc
depmod: modules
	@$(MAKE) $(AM_MAKEFLAGS) _$@ # For strong dependency ordering w/ -j

SUDO = $(shell [ `id -u ` = 0 ] || echo sudo)
# Targets to (un)load the kernel modules
# NOTE: we rmmod blcr_vmadump in case of upgrade from 0.7.X
rmmod:
	if /sbin/lsmod | grep '^blcr '         > /dev/null 2>&1; then $(SUDO) /sbin/rmmod blcr; fi
	if /sbin/lsmod | grep '^blcr_vmadump ' > /dev/null 2>&1; then $(SUDO) /sbin/rmmod blcr_vmadump; fi
	if /sbin/lsmod | grep '^blcr_imports ' > /dev/null 2>&1; then $(SUDO) /sbin/rmmod blcr_imports; fi
_insmod: rmmod	# No rebuild of modules or depmod check
	$(SUDO) /sbin/insmod $(module_imports) $(insmod_imports_args)
	$(SUDO) /sbin/insmod $(module_blcr) $(insmod_blcr_args)
insmod: depmod
	@$(MAKE) $(AM_MAKEFLAGS) _$@ # For strong dependency ordering w/ -j
 
module_phony = modules _depmod depmod _insmod insmod rmmod

# Reconfigure if the kernel config, version or symbol table have changed
# or if our imports template has changed
CONFIG_STATUS_DEPENDENCIES = \
	@LINUX_OBJ@/.config \
	@LINUX_VERSION_H@ \
	@LINUX_SYMTAB_FILE@ \
	@TOP_SRCDIR@/include/blcr_imports.h.in

# Check module dependencies at end of build
all_local_depmod = _depmod
endif

# Convenience
util libcr examples:
	$(MAKE) $(AM_MAKEFLAGS) -C $@ all
tests:
	$(MAKE) $(AM_MAKEFLAGS) -C $@ all $@
	if test -d libcr32/$@; then $(MAKE) $(AM_MAKEFLAGS) -C libcr32/$@ all $@; fi
bonus-tests:
	$(MAKE) $(AM_MAKEFLAGS) -C tests all $@
bonus-check:
	$(MAKE) $(AM_MAKEFLAGS) all
	$(MAKE) $(AM_MAKEFLAGS) -C tests bonus-check
	if test -d libcr32/tests; then $(MAKE) $(AM_MAKEFLAGS) -C libcr32 check; fi

if CR_INSTALLED_UTIL
symlinks:
	mkdir -p bin
	for f in cr_checkpoint cr_restart cr_run $(cr_info); do \
		test -h bin/$$f || ln -s $(bindir)/$$f bin/; \
	done
else
symlinks:
	mkdir -p bin
	for f in cr_checkpoint cr_restart cr_run $(cr_info); do \
		test -h bin/$$f || ln -s @TOP_BUILDDIR@/util/$$f/$$f bin/; \
	done
endif

# Local hooks/extensions
clean-local:
	@for dir in $(NOAUTO_DIRS); do \
	  test -f $$dir/Makefile && $(MAKE) $(AM_MAKEFLAGS) -C $$dir clean; \
	done; true
	if test -d libcr32; then $(MAKE) $(AM_MAKEFLAGS) -C libcr32 clean; fi
all-local: symlinks
	if test -d libcr32; then $(MAKE) $(AM_MAKEFLAGS) -C libcr32 all; fi
	if test -n "$(all_local_depmod)"; then $(MAKE)  $(AM_MAKEFLAGS) $(all_local_depmod); fi
check-local:
	if test -d libcr32; then $(MAKE) $(AM_MAKEFLAGS) libcr util; fi
	if test -d libcr32; then $(MAKE) $(AM_MAKEFLAGS) -C libcr32 check; fi
install-data-local:
	if test -d libcr32; then $(MAKE) $(AM_MAKEFLAGS) -C libcr32 install-data; fi
install-exec-local:
	if test -d libcr32; then $(MAKE) $(AM_MAKEFLAGS) -C libcr32 install-exec; fi
uninstall-local:
	if test -d libcr32; then $(MAKE) $(AM_MAKEFLAGS) -C libcr32 uninstall; fi
	-if test -d "$(DESTDIR)@CR_MODULE_DIR@"; then rmdir "$(DESTDIR)@CR_MODULE_DIR@"; fi
distclean-local:
	rm -rf libcr32 bin

# Our rpm specfile wants to query some vars
LINUX_VER=@LINUX_VER@
LINUX_SRC=@LINUX_SRC@
echoval:
	@echo $($(VARNAME)) | sed 's/"/\\"/'g

.PHONY: dist-srpm dist-oscar rpms examples util libcr tests bonus-check echoval $(module_phony)

DISTCLEANFILES = .import_decls .import_calls .cached_kconfig .cached_ksymtab
