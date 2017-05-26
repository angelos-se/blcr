/*
 * Instructions for building BLCR with a cross-compiler.
 *
 * NOTE: Cross-compilation of BLCR is experimental at this point and may
 * not always go smoothly.  We are currently considering only systems for
 * which the cross-compilation tool-chain has been installed in what we
 * believe is a "standard" manner (tool-chain executables prefixed with
 * a host-tuple recognized by config.sub).
 *
 * We welcome your feedback on how these instructions and/or the script
 * output by this program can be made more general and more robust.
 *
 * In the instructions that follow, we use "Target" to refer to the platform
 * on which you plan to run BLCR and "Build" to refer to the platform on
 * which you plan to compile BLCR.
 *
 * We recommend you read all the steps before you begin following any of them.
 *
 * 1. "Build the cross-helper program"
 *    This file is a C source file.  On Build, compile it with the cross-
 *    compiler, using the options you need/want to pass at configure time (to
 *    get the same ABI, libc version, etc.).  Examples of options to pass
 *    include "-m64" if using a biarch compiler that defaults to 32-bit.
 *    Unless you dissable (see below) CHECK_LINUXTHREADS, you will need to
 *    pass -lpthread to link correctly.
 *    If your compilation fails with errors, you may need to disable building
 *    one or more of the tests.  To do so, try changing the CHECK_* settings
 *    below to disable the problematic test(s).  However, if you do so you'll
 *    need some other way to determine the test result.  So, you'll probably
 *    want to contact us at checkpoint@lbl.gov for assistance.
 *
 * 2. "Run the cross-helper program"
 *    Run the built program on Target, saving the output to a file in the top-
 *    level BLCR source directory on Build.  Getting the executable from
 *    Build to Target and the output file back to Build are up to you - we
 *    have no way to portably automate this.
 *    In what follows, we'll assume you've named the file "cross-configure",
 *    but any name is fine.  However, it is crucial that the file be saved
 *    in the top-level source directory (the same directory that contains 
 *    "configure").
 *
 * 3. "Edit the cross-configure script"
 *    Edit the new cross-configure script for completeness, fixing anything
 *    you can identify as incorrect and adding manually-determined answers
 *    for any CHECK_* settings you disabled in step #1.
 *    Look for the string "%MISSING%" to identify anything that is required
 *    before you can move to the next step.
 *    The one thing you will always need to supply is the Target type.
 *    If using a standard installation of a gcc cross compiler, then the
 *    type string is the prefix of the cross-compiler, minus the final
 *    hyphen.  For instance, if in step #1 you build this file using
 *    i586-redhat-linux-gcc, then TARGET_TYPE should be "i586-redhat-linux".
 *
 * 4. "Make the cross-configure script executable"
 *    chmod +x cross-configure
 *
 * 5. "Satisfy the build prerequisites"
 *    Ensure that you have, on the Build system, configured kernel source
 *    for the Target system as described in doc/html/BLCR_Admin_Guide.html
 *    and in the FAQ entry "What if I my kernel sources are unconfigured?"
 *    in doc/html/FAQ/html.  You will also need, on the Build system, either
 *    the "System.map" file or the "vmlinux" file from the Target's kernel.
 *    If you need to use the instructions in the FAQ to configure your kernel
 *    sources, then you'll need to replace "make" in those instructions with
 *    "make ARCH=<host_arch> CROSS_COMPILE=<tool_prefix>".
 *
 * 6. "Prepare configure options"
 *    Read the BLCR Admin Guide (in doc/html) to understand what options you
 *    want or need to pass to "configure", but don't run configure directly!
 *    In the next step we'll run the newly create "cross-configure" script
 *    instead.  In this step just determine the options you'll need.
 *    + You must provide the option --with-linux=[DIR] and one of either
 *    --with-system-map=[FILE] or --with-vmlinux=[FILE].  Otherwise the
 *    configure logic will default to trying to use the currently running
 *    kernel on the Build machine!
 *    + When considering a value for --prefix (or any other options that will
 *    determine placement of BLCR files), specify the values as they will
 *    appear in the Target's filesystem regardless of if/how they may appear
 *    on the Build machine.
 *    + If you want to build a BLCR test suite runnable on Target, include
 *    --enable-testsuite in your options (see the step "Running the tests on
 *    Target", below, for more details on this).  We recommend this option.
 *    + If in step 1 you passed any compiler flags to control ABI, then we
 *    recommend that you add them to the definition of CC.  For instance,
 *    for a biarch compiler defaulting to 32-bits, you might pass
 *    CC='ppc-linux-gcc -m64'  on the cross-configure command line.
 *
 * 7. "Run the cross-configure script"
 *    On Build, run "cross-configure" with the options you determined in
 *    the previous step.
 *
 * 8. "Compile BLCR"
 *    On Build, run "make" to build BLCR using the cross tool-chain.
 *    Don't run "make install" yet; it is covered in the steps that follow.
 *    Don't try to run "make insmod" or "make check" on Build, since they
 *    use binaries that have been built for Target.  However, you may
 *    optionally run "make tests" on the Build machine to compile (but not
 *    run) the test programs, though if you configured with --enable-testsuite
 *    then they will be compiled by default.
 *
 * 9. "Installing BLCR (part 1 of 3)"
 *    When considering --prefix and related configure options above, we told
 *    you to consider only the paths as they appear in the filesystem of the
 *    Target system.  This is important because some of these paths get encoded
 *    in the programs and/or libraries.  However, the next step is to run
 *    the installation step on Build.  So, now we will assume that you have
 *    one of two situations.  If your situation falls somewhere in between,
 *    then we recommend using case B.
 *    A) The Target filesystem is mounted (writable) on the Build machine,
 *       rooted at some single prefix such as /export/example.
 *    B) The Target filesystem is not mounted on the Build machine.
 *    On Build, run "make install DESTDIR=<PATH>" where the value of <PATH>
 *    is your mount point for case A, or any empty directory of your choosing
 *    for case B (something like /tmp/blcr-destdir is a good choice).
 *
 * 10. "Installing BLCR (part 2 of 3)"
 *    If your system fell into case A of the previous step, then your files
 *    are now present on the Target filesystem.  If your system fell into case
 *    B, then you now must use some mechanism of your choice to copy all the
 *    files in the chosen DESTDIR to the Target filesystem.  You can then
 *    remove the temporary directory you used for DESTDIR.
 *
 * 11. "Installing BLCR (part 3 of 3)"
 *    You should now connect to Target, and continue the steps in the BLCR
 *    Admin Guide for "Loading the Kernel Modules", "Updating ld.so.cache"
 *    and "Configuring Users' Environments".
 *
 * 12. [OPTIONAL] "Running the tests on Target"
 *    Note that while releases prior to 0.8.0 required "perl" on Target to
 *    run most of the tests, that is no longer the case.
 *    There are two possible ways to run the BLCR test suite on Target:
 *    A) "Option 1 - cross compilation"
 *      If you passed "--enable-testsuite" to the cross-configure script,
 *      then you will find a script named "RUN_ME" installed in
 *      "$prefix/libexec/blcr-testsuite" on Target. Once you have loaded the
 *      kernel modules and setup your environment correctly (LD_LIBRARY_PATH
 *      in particular), on Target, you can execute "RUN_ME" to run the same
 *      testsuite that "make check" runs in a native build.
 *      A message like
 *        ERROR: ld.so: object 'libcr.so.0' from LD_PRELOAD cannot be preloaded: ignored.
 *      is a sign that you have not set LD_LIBRARY_PATH correctly, or if you
 *      installed in a "system directory" it may mean you need to run
 *      "ldconfig" as root to update /etc/ld.so.cache.
 *      If you are concerned over disk space, you may safely remove the
 *      blcr-testsuite directory when you are done with it - no other parts
 *      of BLCR depend on it existing.
 *    B) "Option 2 - native compilation"
 *      If you have a native tool-chain for compilation on Target you also
 *      have the option to configure BLCR on Target to build only the
 *      testsuite.  To do so, run
 *        "configure --with-installed-util --with-installed-modules \
 *                   --with-installed-libcr"
 *      on Target, adding any --prefix or related options passed to the
 *      cross-configure script on Build.  However, you don't need to pass
 *      the --with-linux, --with-system-map or --with-vmlinux options.
 *      When invoked this way, configure will do only a minimum amount of
 *      work, far less than when configuring for a full build with the
 *      kernel modules.  You can then run "make -C tests check" to build
 *      and run the testsuite (without the lengthy build of the library
 *      and kernel modules).
 *
 * 13. [OPTIONAL] "BLCR Manual pages"
 *    Because of the way BLCR generates its man pages, cross-compiled builds
 *    don't have man pages.  If you need/want man pages on the Target system,
 *    then you can build BLCR natively on any system of your choosing and
 *    manually copy the resulting man pages to Target.  There are no platform-
 *    specific elements to the man pages.
 *
 *
 * $Id: cross_helper.c,v 1.10 2008/08/27 18:47:55 phargrov Exp $
 */

/* BEGIN CHECK_* variables for optional configuration.  */
#define CHECK_STACK_DIRECTION 1
#define CHECK_SIGNUM 1
#define CHECK_LINUXTHREADS 1
/* END optional configuration.
 * If you think you need to modify anything below this line,
 * then please contact us at checkpoint@lbl.gov.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#ifndef CHECK_STACK_DIRECTION
  #define CHECK_STACK_DIRECTION 1
#endif
#ifndef CHECK_SIGNUM
  #define CHECK_SIGNUM 1
#endif
#ifndef CHECK_LINUXTHREADS
  #define CHECK_LINUXTHREADS 1
#endif

static void die(const char *msg) {
	fprintf(stderr, msg);
	exit(1);
}

#if CHECK_STACK_DIRECTION
static void cross_stack_direction(void) {
	static char *addr = 0;
	auto char dummy;
	if (addr == 0) {
		addr = &dummy;
		cross_stack_direction();
		return;
	}
	printf("cross_stack_direction=%d\n", (&dummy > addr) ? 1 : -1);
}
#else
static void cross_stack_direction(void) {
	printf("cross_stack_direction=%%MISSING%%\n");
}
#endif

#if CHECK_SIGNUM
extern int __libc_allocate_rtsig(int);
static void cross_signum(void) {
	int i = __libc_allocate_rtsig(0);
	if (i <= 0) die("Unable to determine CR_SIGNUM\n");
	printf("cross_signum=%d\n", i);
}
#else
static void cross_signum(void) {
	printf("cross_signum=%%MISSING%%\n");
}
#endif

#if CHECK_LINUXTHREADS
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
static void *thread_pid(void *arg) { return (void *)(long)getpid(); }
static void cross_linuxthreads(void) {
	pthread_t th;
	void *join_val;

	if (0 != pthread_create(&th, NULL, &thread_pid, NULL)) {
		die("Error calling pthread_create()\n");
	}
	if (0 != pthread_join(th, &join_val)) {
		die("Error calling pthread_join()\n");
	}

	printf("cross_linuxthreads=%i\n", (long)join_val != (long)getpid());

}
#else
static void cross_linuxthreads(void) {
	printf("cross_linuxthreads=%%MISSING%%\n");
}
#endif

static void print_header(void) {
	puts(
		"# Automatically-generated file for cross-configuration of BLCR.\n"
		"# You will always need to perform at least some manual editing.\n"
		"# So, see the file \"cross_helper.c\" for instructions.\n"
		"\n"
		"# You always need to set this one manually.\n"
		"TARGET_TYPE=%MISSING%\n"
		"\n"
	);
}

static void print_trailer(void) {
	puts(
		"extra_conf_args=\"cross_signum=$cross_signum cross_stack_direction=$cross_stack_direction cross_linuxthreads=$cross_linuxthreads cross_linuxthreads_static=$cross_linuxthreads_static\"\n"
		"\n"
		"srcdir=`dirname $0`\n"
		"if test ! -f \"$srcdir/configure\" ; then\n"
		"  echo \"The script $0 must be in the same directory as the configure script.\"\n"
		"  exit 1\n"
		"fi\n"
		"${srcdir}/configure --host=$TARGET_TYPE --program-prefix='' $extra_conf_args \"$@\"\n"
	);
}


int main(void) {
	print_header();
	cross_stack_direction();
	cross_signum();
	cross_linuxthreads();
	puts(	"\n"
		"# ONLY if configuring with --enable-static or --disable-shared, you'll need to\n"
		"# set cross_linuxthreads_static on the following line.  The value to use can be\n"
		"# taken from the cross_linuxthreads value output by a version of the \"cross_helper\"\n"
		"# compiled with -static.\n"
		"# If NOT passing --enable-static or --disable-shared at configure time, this\n"
		"# value will be unused and need not be set.\n"
		"cross_linuxthreads_static=%MISSING%\n"
		"\n"
	    );
	print_trailer();
	return 0;
}
