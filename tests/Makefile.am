# Uncomment the following if your automake defaults to parallel tests.
# You'll know becuase your build will fail with output like:
#   Makefile:1923: *** unterminated variable reference.  Stop.
#AUTOMAKE_OPTIONS = serial-tests

# Special target (run before tests) ensures the kernel modules are loaded
check_module:
	@(/sbin/lsmod | grep '^blcr ' > /dev/null 2>&1) || \
	(echo	'#############################################################'; \
	 echo	'#### BLCR modules are not loaded.  Cannot run the tests! ####'; \
	 echo	'#### You must insmod/modprobe the following modules as   ####'; \
	 echo	'#### root (in order) before you can run "make check":    ####'; \
	 echo	'####    blcr_imports   blcr                              ####'; \
	 echo	'#### "make insmod" will automate this if run as a user   ####'; \
	 echo	'#### with sudo(8) rights.                                ####'; \
	 echo	'#############################################################'; \
	false)

# Files we clean at various times
CONTEXTS = context.* .context.*.tmp Context[123] .Context[123].tmp
TEST_TEMPS = tst*
# Special target (run before tests) ensures no temp file conflicts
clean_temps:
	rm -Rf $(CONTEXTS) $(TEST_TEMPS)

# Special target (run before tests) ensures utils are up-to-date (if built)
if CR_INSTALLED_UTIL
util_dir = $(bindir)
else
util_dir = $(TOP_BUILDDIR)/bin
endif
if CR_BUILD_UTIL
build_util:
	@$(MAKE) $(AM_MAKEFLAGS) --no-print-directory -C $(TOP_BUILDDIR) util symlinks
else
build_util:
	@: # Empty
endif


# "SIMPLE" tests, which just return success or failure.
# These tests get listed in the SIMPLE_TESTS variable.
if CR_ENABLE_SHARED
# These test the actual cr_run utility or dlopen(), and thus require shared libs:
SIMPLE_scripts_shared = cr_run cr_targ cr_targ2 cr_omit dlopen
endif
SIMPLE_progs = atomics bug2524
SIMPLE_scripts = $(SIMPLE_scripts_shared) \
	bug2003 run_on save_exe save_priv save_share save_all \
	reloc_exe reloc_file reloc_fifo reloc_dir reloc_all \
	clobber
SIMPLE_TESTS = $(SIMPLE_progs) $(SIMPLE_scripts)

# "SEQ" tests are ones that check for certain events happening in
# proper sequence, by using a wrapper to parse output.
# These tests get listed in the SEQ_TESTS variable.
SEQ_progs = stage0001 stage0002 stage0003 stage0004 \
	    critical_sections replace_cb \
	    failed_cb failed_cb2 pid_in_use cs_enter_leave cs_enter_leave2 \
	    cr_tryenter_cs stopped edeadlk pid_restore
# XXX: cb_exit has moved to "bonus" list for now.  See bug 2244.
# XXX: ptrace has moved to "bonus" list for now.  See bug 2455.
SEQ_scripts = 
SEQ_TESTS = $(SEQ_progs) $(SEQ_scripts)

# C/R unit tests ("CRUT")
# These are tests that use the crut.c infractructure and the crut_wrapper
# script.
CRUT_progs = \
	simple simple_pthread cwd dup filedescriptors pipe named_fifo \
	cloexec get_info orphan overlap child mmaps hugetlbfs readdir dev_null \
	cr_signal linked_fifo sigpending dpipe forward hooks math sigaltstack \
	prctl lam nscd
# hugetlbfs2 moved to "bonus" list due to leak of MAP_PRIVATE pages in some kernels
CRUT_TESTS = $(CRUT_progs)

# These are like SIMPLE_TESTS, SEQ_TESTS and CRUT_TESTS, but for
# maintainer-only tests (the "bonus" tests).
if CR_ENABLE_SHARED
SEQ_scripts2_shared = \
	ash_test bash_test csh_test ksh_test zsh_test \
	perl_test python_test tcl_test expect_test ruby_test \
	guile_test clisp_test rep_test emacslisp_test \
	php_test gst_test gcj_test gij_test jre_test \
	ocaml_test sml_test mosml_test
endif
if CR_HAVE_CXX
SEQ_testcxx = testcxx
testcxx_SOURCES = testcxx.cc
endif
SIMPLE_progs2 = atomics_stress
SIMPLE_scripts2 =
SEQ_progs2 = cb_exit ptrace $(SEQ_testcxx)
SEQ_scripts2 = $(SEQ_scripts2_shared)
CRUT_progs2 = hugetlbfs2
SIMPLE_TESTS2 = $(SIMPLE_progs2) $(SIMPLE_scripts2)
SEQ_TESTS2 = $(SEQ_progs2) $(SEQ_scripts2)
CRUT_TESTS2 = $(CRUT_progs2)

# Need extra ldflags when an object references no symbols in a lib
libcr_ldflags = -u cr_link_me
libcr_run_ldflags = -u cr_run_link_me
libcr_omit_ldflags = -u cr_omit_link_me

if CR_INSTALLED_LIBCR
# Use the installed library and headers
libcr_L = -L$(libdir)
libcr_includes = -I$(includedir)
else
libcr_L = -L$(top_builddir)/libcr
libcr_includes = -I$(top_srcdir)/include -I$(top_builddir)/include
LIBCR = $(top_builddir)/libcr/libcr.la \
	$(top_builddir)/libcr/libcr_run.la \
	$(top_builddir)/libcr/libcr_omit.la
$(tester_progs) $(check_progs2): $(LIBCR)
endif

# The library/ies that (nearly?) every test needs:
libcr_ldadd = $(libcr_L) -lcr
libcr_run_ldadd = $(libcr_L) -lcr_run
libcr_omit_ldadd = $(libcr_L) -lcr_omit

# Gather reusable code into libtest.a
libtest_a_SOURCES = crut.c crut_util.c crut_util_pth.c crut_util_libcr.c
libtest_ldadd = libtest.a

# Generic flags here...
LDADD = $(libtest_ldadd) $(libcr_ldadd) -lpthread @CR_CLIENT_LDADD@
INCLUDES = $(libcr_includes)
AM_CFLAGS = -Wall

# Test-specific flags here...
stage0001_LDADD = $(libtest_ldadd) $(libcr_ldadd) @CR_CLIENT_LDADD@ # NO PTHREADS FOR THIS TEST
atomics_CFLAGS = $(CFLAGS) -I$(top_srcdir)/libcr/arch/@CR_LIBARCH@
atomics_LDADD = @CR_CLIENT_LDADD@ # No libs required
atomics_stress_CFLAGS = $(CFLAGS) -I$(top_srcdir)/libcr/arch/@CR_LIBARCH@
atomics_stress_LDADD = $(libtest_ldadd) -lpthread @CR_CLIENT_LDADD@ # Only need crut_util.o and pthreads
ptrace_LDFLAGS = $(libcr_ldflags) # In static builds need help to ensure libcr is linked (no refs)

# The wrappers
wrapper_progs = seq_wrapper crut_wrapper
seq_wrapper_LDADD = $(libtest_ldadd) # Must not be a lt-exec

# Rules to "build" a shell script
# We "install" to avoid worrying about the permissions in/on $srcdir
# The .sh suffix not meant denote any specific shell
all_scripts = $(SIMPLE_scripts) $(SEQ_scripts) $(helper_scripts) \
	      $(SIMPLE_scripts2) $(SEQ_scripts2) $(helper_scripts2)
all_scripts_src = $(patsubst %,%.sh,$(all_scripts))
$(all_scripts): shellinit
$(all_scripts) : % : %.sh
	$(INSTALL_SCRIPT) $< $@

# Rule to wrap the SEQ tests with the seq_wrapper
SEQ_RUN = $(patsubst %,%.st,$(SEQ_TESTS))
SEQ_RUN2 = $(patsubst %,%.st,$(SEQ_TESTS2))
$(SEQ_RUN) $(SEQ_RUN2) : %.st : % seq_wrapper
	@rm -f $@
	@$(LN_S) -f seq_wrapper $@

# Rule to wrap the CRUT tests with the crut_wrapper
# We use a 1-line script, not a symlink, in the build directory
# to allow crut_wrapper to be a libtool wrapper itself.
CRUT_RUN = $(patsubst %,%.ct,$(CRUT_TESTS))
CRUT_RUN2 = $(patsubst %,%.ct,$(CRUT_TESTS2))
$(CRUT_RUN) $(CRUT_RUN2) : %.ct : % crut_wrapper
	@rm -f $@
	@(echo '#!/bin/sh'; \
	  echo 'exec `dirname $$0`/crut_wrapper -q "$$@" $*' ) > $@
	@chmod +x $@

# Script to run all the CRUT tests standalone
rununittests: $(CRUT_TESTS)
	(echo '#!/bin/sh'; echo './crut_wrapper $$@ $(CRUT_TESTS)') > $@
	chmod +x $@

# Rules to help w/ some dependencies in other directories:
if CR_BUILD_LIBCR
$(LIBCR): FORCE
	@$(MAKE) $(AM_MAKEFLAGS) --no-print-directory -C $(@D)
endif

# Prog(s) needed indirectly by test(s)
cr_run: hello
hello_LDADD = # NO LIBS HERE
cr_targ cr_tagr2 cr_omit: pause
bug2003: bug2003_aux
save_exe save_priv save_share save_all: save_aux
reloc_exe reloc_file reloc_fifo reloc_dir reloc_all: reloc_aux
run_on: save_aux pause
clobber: save_aux
if CR_ENABLE_SHARED
pause_LDADD = # NO LIBS HERE
bug2003_aux_LDADD = # NO LIBS HERE
save_aux_LDADD =  $(libtest_ldadd) # NO LIBCR HERE - must not be a lt-exec
reloc_aux_LDADD = $(libtest_ldadd) # NO LIBCR HERE - must not be a lt-exec
dlopen_aux_LDADD = -ldl # NO LIBCR HERE
helper_progs_shared = hello dlopen_aux
dlopen: dlopen_aux
else
pause_LDADD =  $(libcr_run_ldadd) @CR_CLIENT_LDADD@
pause_LDFLAGS = $(libcr_run_ldflags)
bug2003_aux_LDADD = $(libcr_run_ldadd) @CR_CLIENT_LDADD@
bug2003_aux_LDFLAGS = $(libcr_run_ldflags)
save_aux_LDADD =  $(libcr_run_ldadd) $(libtest_ldadd) @CR_CLIENT_LDADD@
save_aux_LDFLAGS = $(libcr_run_ldflags)
reloc_aux_LDADD =  $(libcr_run_ldadd) $(libtest_ldadd) @CR_CLIENT_LDADD@
reloc_aux_LDFLAGS = $(libcr_run_ldflags)
endif
helper_progs = $(helper_progs_shared) bug2003_aux pause save_aux reloc_aux
helper_progs2 =
helper_scripts = 
helper_scripts2 =

# Maintainer-only tests
BONUS_TESTS = $(SIMPLE_TESTS2) $(SEQ_RUN2) $(CRUT_RUN2)
bonus-tests: $(BONUS_TESTS)
	@$(MAKE) $(AM_MAKEFLAGS) --no-print-directory check TESTS="$(BONUS_TESTS)"
bonus-check: $(BONUS_TESTS)
	@$(MAKE) $(AM_MAKEFLAGS) --no-print-directory check TESTS="$(TESTS) $(BONUS_TESTS)"

# Create our own target for building (but NOT running) the tests
build-tests: tests # Legacy target
tests: $(TESTS) $(BONUS_TESTS) rununittests
	@echo '################################################################################'
	@echo 'HOW TO RUN THESE TESTS:'
	@echo 
	@echo 'SIMPLE TESTS ($(SIMPLE_TESTS)):'
	@echo '--- use ./test'
	@echo 'UNIT TESTS ($(CRUT_TESTS)):'
	@echo '--- use ./<test>.ct'
	@echo '--- or use ./rununittests to run them all'
	@echo 'SEQ TESTS ($(SEQ_TESTS)):'
	@echo '--- use ./<test>.st'
	@echo '################################################################################'

# Some tests use cr_run even w/o shared libs - mask that
if CR_ENABLE_SHARED
CR_RUN_INSTALLED=$(bindir)/cr_run
else
CR_RUN_INSTALLED=env
endif

# Depending whether we build the testsuite or not, the test programs and
# scripts should be in either the testsexec_ or check_ prefix.
#
tester_progs = $(SIMPLE_progs) $(SEQ_progs) $(CRUT_progs) $(helper_progs)
tester_scripts = $(SIMPLE_scripts) $(SEQ_scripts) $(helper_scripts)
if CR_BUILD_TESTSUITE
testsexecdir = $(libexecdir)/blcr-testsuite
testsexec_PROGRAMS = $(tester_progs) $(wrapper_progs)
testsexec_SCRIPTS = $(tester_scripts)
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT = $(testsexec_PROGRAMS) $(testsexec_SCRIPTS)
install-exec-local:
	@$(mkinstalldirs) "$(DESTDIR)$(testsexecdir)"
	@cd "$(DESTDIR)$(testsexecdir)" && \
	 for file in $(SEQ_RUN); do rm -f $$file; $(LN_S) seq_wrapper $$file; done && \
	 for file in $(CRUT_RUN); do rm -f $$file; $(LN_S) crut_wrapper $$file; done && \
	 echo '# This file is required, but typically empty, in an installed testsuite' > shellinit && \
	 chmod 644 shellinit
	@sed -e 's,%bindir%,$(bindir),;' \
	     -e 's,%tests%,$(TESTS),;' \
	     -e 's,%cr_run%,$(CR_RUN_INSTALLED),;' \
	     -e 's,%cr_pwd%,@PWD_PROG@,;' \
		$(srcdir)/RUN_ME.in >$(DESTDIR)$(testsexecdir)/RUN_ME
	@chmod +x $(DESTDIR)$(testsexecdir)/RUN_ME
uninstall-local:
	@cd $(DESTDIR)$(testsexecdir) && rm -f RUN_ME $(SEQ_RUN) $(CRUT_RUN)
else
check_progs = $(tester_progs)
check_scripts = $(tester_scripts)
endif

# Maintainer-only tests progs and scripts (always in check_ prefix)
check_progs2 = $(SIMPLE_progs2) $(SEQ_progs2) $(CRUT_progs2) $(helper_progs2)
check_scripts2 = $(SIMPLE_scripts2) $(SEQ_scripts2) $(helper_scripts2)

# Environment variables visible to the test programs
TESTS_ENVIRONMENT = top_srcdir="$(TOP_SRCDIR)" LIBCR_DISABLE_NSCD=1
# Automake magic here...
check_LIBRARIES = libtest.a
check_HEADERS = crut.h crut_util.h
check_SCRIPTS = check_module clean_temps build_util $(check_scripts) $(check_scripts2)
check_PROGRAMS = $(check_progs) $(check_progs2) $(wrapper_progs)
TESTS = $(SIMPLE_TESTS) $(SEQ_RUN) $(CRUT_RUN) # Things to actually run
EXTRA_DIST = license.txt CountingApp.java CountingApp.class RUN_ME.in $(all_scripts_src)
MOSTLYCLEANFILES = core core.* $(CONTEXTS)
CLEANFILES = $(SEQ_RUN) $(CRUT_RUN) $(SEQ_RUN2) $(CRUT_RUN2) $(all_scripts) \
             rununittests RUN_ME
clean-local: clean_temps
echoval: FORCE
	@echo $($(VARNAME)) | sed 's/"/\\"/'g
.PHONY: check_module clean_temps build_util tests build-tests echoval FORCE
FORCE:

# Preserve the empty line above!
