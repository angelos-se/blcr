/*
 * Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
 * 2008, The Regents of the University of California, through Lawrence
 * Berkeley National Laboratory (subject to receipt of any required
 * approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * Portions may be copyrighted by others, as may be noted in specific
 * copyright notices within specific files.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: crut_wrapper.c,v 1.16.4.2 2014/10/06 23:12:46 phargrov Exp $
 *
 * Runs a crut-style test.
 *
 * Runs the checkpoint.  Checks the error code.  
 * Restarts from context file.  Checks error code again.
 *
 */

#define _LARGEFILE64_SOURCE 1

#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <libgen.h>
#include <fcntl.h>
#ifndef O_LARGEFILE
  #define O_LARGEFILE 0
#endif

#include "crut_util.h"

static int test_timeout = 60;
static const char *argv0 = ",";
static char *test_path = ",";
static char *crut_arguments = NULL;
static const char *crut_cmd[6]; /* Max is PROG, -v, -d, -F, subtest, NULL */
static int crut_argc = 1;
static int opt_verbose = 0;
static int opt_debug = 0;
static int opt_keep = 0;
static int opt_fake = 0;

/*
 * die(format, args...)
 *
 * Sends an error message to stderr and exits
 */
static void die(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);

    exit(1);
}

static void blabber(const char *format, ...)
{
    va_list args;

    if (opt_verbose >= 2) {
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
	if (opt_debug) putchar('\n');
    }
}

static void verbose(const char *format, ...)
{
    va_list args;

    if (opt_verbose == 1) {
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
	if (opt_debug) putchar('\n');
    }
}

static void subdued(const char *format, ...)
{
    va_list args;

    if (opt_verbose == 0) {
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
	if (opt_debug) putchar('\n');
    }
}

/*
 * Setup variables from env, args, etc.
 * Returns NULL-terminated array of tests
 */
static char **setup(int argc, const char **argv)
{
    char **retval = NULL;
    const char *val;
    int opt_quiet = 0;
    int argi;

    val = getenv("CRUT_TIMEOUT");
    if (val && (atoi(val) > 0)) {
	test_timeout = atoi(val);
    }

    test_path = crut_find_testsdir(argv[0]);
    argv0 = argv[0];

    for (argi = 1; argi < argc; ++argi) {
	val = argv[argi];
	if ((val[0] != '-') || (strlen(val) != 2)) break;
	switch (val[1]) {
	case 'k': opt_keep=1;
		  break;
	case 'v': ++opt_verbose;
		  break;
	case 'q': opt_quiet = 1;
		  break;
	case 'F': opt_fake=1;
		  break;
	case 'd': opt_debug=1;
		  break;
	default:  die("Unknown option '%s'", val);
        }
    }

    if (opt_verbose == 3) {
	crut_arguments = strdup("-d");
	crut_cmd[crut_argc++] = "-d";
	putenv("LIBCR_TRACE_MASK=0xffff");
    } else if (opt_verbose == 2) {
	crut_arguments = strdup("-v");
	crut_cmd[crut_argc++] = "-v";
    } else {
	crut_arguments = strdup("");
    }

    if (opt_fake) {
	crut_arguments = crut_sappendf(crut_arguments, " -F");
	crut_cmd[crut_argc++] = "-F";
    }
    if (opt_debug) {
	crut_arguments = crut_sappendf(crut_arguments, " -d");
	crut_cmd[crut_argc++] = "-d";
    }

    crut_cmd[crut_argc+1] = NULL;
    if (crut_argc+1 > 5) die("OOPS");

    if (argi < argc) {
	int len = argc - argi;
	int i;
	retval = malloc((1+len) * sizeof(char *));
	for (i=0; i<len; ++i) {
		retval[i] = strdup(argv[argi+i]);
	}
	retval[len] = NULL;
    } else {
	char *test = crut_basename(argv[0]);
	char *tmp = strstr(test, ".ct");
	if (!tmp) die("'%s' does not have a .ct suffix", argv[0]);
	*tmp = '\0';
	retval = malloc(2 * sizeof(char *));
	retval[0] = test;
	retval[1] = NULL;
	opt_quiet = 1;
    }

    if (opt_quiet && !opt_verbose) opt_verbose = -1;

    return retval;
}

static void cleanup(void)
{
    free(test_path);
    free(crut_arguments);
}

volatile int expired;
void expire(int signo) { expired = 1; }

void wait_timeout(const char *step, int pid)
{
    int timeout = test_timeout;
    struct sigaction sa, osa;
    int status;
    int rc;

    sa.sa_handler = &expire;
    sa.sa_flags = SA_NOMASK;
    sigemptyset(&sa.sa_mask);
    (void)sigaction(SIGALRM, &sa, &osa);

    do {
	expired = 0;
	alarm(timeout);
	rc = waitpid(pid, &status, 0);
	timeout = 1 + alarm(0);
	if ((rc < 0) && expired) goto timeout;
    } while ((rc < 0) && (errno == EINTR));

    while (!kill(-pid, 0)) {
	if (! --timeout) goto timeout;
	sleep(1);
    }

out:
    if (!status) {
	/* OK */
    } else if (WIFEXITED(status)) {
	die(crut_aprintf("%s/nonzeroexit (%d)", step, WEXITSTATUS(status)));
    } else {
	die(crut_aprintf("%s/nonzeroexit (signal %d)", step, WTERMSIG(status)));
    }

    return;

timeout:
    kill(-pid, SIGTERM);
    sleep(2);
    kill(-pid, SIGKILL);
    die(crut_aprintf("%s/timeout", step));
    status = SIGKILL; /* XXX: assumes knowledge of status format */
    goto out;
}

int forkexec(char * const cmd[])
{
    int pid;

    crut_block_sigttou();
    pid = fork();
    if (pid < 0) {
	die("fork() failed: %s", strerror(errno));
    } else if (!pid) {
	setpgid(0, 0);
	/* ICK: */
	execv(cmd[0], cmd);
        die("Failed to exec '%s': %s", cmd[0], strerror(errno));
    }
    setpgid(pid, pid);

    return pid;
}

static char **get_test_names(const char *test_filename)
{
    char *cmd;
    FILE *pipe;
    char *line = NULL;
    size_t len = 0;
    int count = 8;
    int next = 0;
    int status;
    char **retval = malloc(count * sizeof(char *));

    cmd = crut_aprintf("%s -l", test_filename);
    pipe = popen(cmd, "r");
    if (!pipe) {
	die("cannot execute '%s'", test_filename);
    }
    free(cmd);

    while (crut_getline(&line, &len, pipe) != -1) {
	retval[next++] = strdup(crut_chomp(line));
	if (next == count) {
		count *= 2;
		retval = realloc(retval, count * sizeof(char *));
	}
    }
    retval[next] = NULL;
    free(line);

    status = pclose(pipe);
    if (!next && WIFEXITED(status) && (77 == WEXITSTATUS(status))) {
	/* Empty test list and exit code of 77.
	 * We exit with the same 77, which is automake's "SKIP" code. */
	exit(77);
    } else if (!next) {
        die("No tests found in %s", test_filename);
    }

    return retval;
}

int try_checkpoint(char * const cmd[])
{
    int child_pid;

    child_pid = forkexec(cmd);
    wait_timeout("checkpoint", child_pid);

    return child_pid;
}

void try_restart(int orig_pid)
{
    char *context_filename = crut_aprintf("context.%d", orig_pid);
    int pid;
    int fd;
    int rc, status;

    fd = open(context_filename, O_RDONLY|O_LARGEFILE);
    if (fd < 0) {
	die("open(%d) failed: %s", context_filename, strerror(errno));
    }

    pid = fork();
    if (pid < 0) {
	die("fork() failed: %s", strerror(errno));
    } else if (!pid) {
	char *kmsgs = NULL;
	cr_restart_args_t args;
 	cr_restart_handle_t handle;
	int rc;

	setpgid(0, 0);

	cr_initialize_restart_args_t(&args);
	args.cr_fd = fd;

	rc = cr_request_restart(&args, &handle);
	if (rc < 0) {
	    die("cr_restart_request() failed: %s", cr_strerror(errno));
	}

	rc = cr_poll_restart_msg(&handle, NULL, &kmsgs);
	if (rc < 0) {
	    if (kmsgs) {
		fputs(kmsgs, stderr);
	    }
	    die("cr_poll_request() failed: %s", cr_strerror(errno));
	}

	wait_timeout("restart", rc);
	exit(0);
    }
    setpgid(pid, pid);
    (void)close(fd);

    do {
	rc = waitpid(pid, &status, 0);
    } while ((rc < 0) && (errno == EINTR));
    if (rc < 0) {
	die("waitpid(restart_cmd) failed: %s", cr_strerror(errno));
    } else if (status) {
	// Child reported its error to stderr
	exit(1);
    }

    if (!opt_keep) (void)unlink(context_filename);

    free(context_filename);
}

void try_cpr(const char *test_filename, const char *sub_test)
{
    char *command = crut_aprintf("%s %s %s", test_filename, crut_arguments, sub_test);
    int orig_pid;

    /* Build crut_command from the partial one at global scope */
    crut_cmd[0] = test_filename;
    /* [1..crut_argc) are the optional arguments */
    crut_cmd[crut_argc] = sub_test;

    verbose("%s (checkpoint, ", command);
    blabber("%s: Checkpointing '%s'\n", argv0, command);

    orig_pid = try_checkpoint((char * const *)crut_cmd);

    if (!opt_fake) {
        verbose("restart, ");
	blabber("%s: Restarting '%s'\n", argv0, command);
        try_restart(orig_pid);
    }

    verbose("ok.)\n");
    free(command);
}

int run_crut_tests(char *test_filename)
{
    char **tests = get_test_names(test_filename);
    int passed = 0;
    int i;

    for (i = 0 ; tests[i]; ++i) {
        try_cpr(test_filename, tests[i]);
        ++passed;
        subdued(".");
	free(tests[i]);
    }
    free(tests);
    return passed;
}

int main(int argc, const char **argv)
{
	char **tests;
	int i;

	setbuf(stdout, NULL);
	tests = setup(argc, argv);
	
	for (i=0; tests[i]; ++i) {
            char *test_filename = crut_aprintf("%s/%s", test_path, tests[i]);
            run_crut_tests(test_filename);
	    free(test_filename);
	    free(tests[i]);
	}
	subdued("\n");
	free(tests);

	cleanup();
	return 0;
}
