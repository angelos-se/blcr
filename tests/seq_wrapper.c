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
 * $Id: seq_wrapper.c,v 1.12.12.1 2009/03/12 20:07:38 phargrov Exp $
 *
 * This is a program to parse the output of a test
 * which produces output with lines tagged w/ consecutive integers.
 * The return code is 0 if no failures are detected.
 *
 * usage: testname.st
 * Where this script should be named testname.st, and the actual test is testname.
 *
 * The expected output from a test is:
 *  0 whatever
 *  1 something more
 *  ...
 *  XXX DONE
 * Where XXX is some number and the DONE is required exactly.
 * There is no whitespace allowed before the integer line number.
 *
 * Lines beginning with '#' are ignored, except: 
 * + A line beginning "#ST_ALARM:" call alarm() with the argument.
 * + Lines beginning with "#ST_IGNORE:" give a regexp, starting right
 *   after the colon.  Later lines matching this regexp are ignored.
 * + A line beginning "#ST_SIGNAL:" indicates that the wrapper should
 *   expect the test to die from the signal number following the colon.
 * + A line beginning "#ST_RETURN:" indicates that the wrapper should
 *   expect the test to return the value following the colon.
 * Multiple #ST_IGNORE lines add to the list of patterns to ignore.
 * Only the last occurance of #ST_SIGNAL or #ST_RETURN has any effect.
 *
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <regex.h>
#include <stdarg.h>

#include "crut_util.h"

volatile struct {
	int limit;
	int next;
	char **array;
} output;

struct ignore_s {
	struct ignore_s *next;
	regex_t regex;
	char *patt; // For debuging
} *ignore_list = NULL;

volatile int fail = 0;
volatile int child_pid = 0;

int opt_verbose = 0;

static void verbose(const char *fmt, ...) __attribute__((format (printf, 1, 2)));
static void verbose(const char *fmt, ...) {
    if (opt_verbose) {
	    va_list args;
	    va_start(args, fmt);
	    vfprintf(stderr, fmt, args);
	    va_end(args);
    }
}

/* Spawn the child
 * Like popen(cmd, "r"), but w/ a setpgrp() and storing the pid
 */
FILE *spawn_child(const char *argv0)
{
    char *tmp, *dir, *test, *cmd;
    int pipefds[2];
    int rc;

    /* Extract dirname from argv0 */
    dir = crut_find_testsdir(argv0);

    /* Extract basename from argv0 */
    test = crut_basename(argv0);

    /* Chop the .st suffix */
    tmp = strstr(test, ".st");
    if (!tmp) {
	fprintf(stderr, "'%s' has no .st suffix\n", test);
	return NULL;
    }
    *tmp = '\0';

    /* check the path */
    cmd = crut_aprintf("%s/%s", dir, test);
    free(test);
    free(dir);
    if (access(cmd, X_OK)) {
	fprintf(stderr, "Cannot access '%s': %s\n", cmd, strerror(errno));
	return NULL;
    }

    /* prepare pipe for test output */
    rc = pipe(pipefds);
    if (rc) {
	perror("pipe()");
	return NULL;
    }

    /* fork() and exec() it */
    crut_block_sigttou();
    rc = fork();
    if (rc < 0) {
	perror("fork()");
	return NULL;
    } else if (!rc) {
	/* Child */
	setpgid(0,0);
	alarm(0);
	rc = dup2(pipefds[1], STDOUT_FILENO);
	if (rc < 0) {
	    perror("dup2(stdout)");
	    exit(-1);
	}
	(void)close(pipefds[0]);
	(void)close(pipefds[1]);
	rc = execl(cmd, cmd, NULL);
	perror("execl()");
	exit(-1);
    }

    setpgid(rc,rc);
    (void)close(pipefds[1]);
    child_pid = rc;
    free(cmd);
    return fdopen(pipefds[0], "r");
}

/* Add a (compiled) regex to the list of patterns to ignore.
 * "eats" expr.
 */
void push_ignore(char *expr) {
    struct ignore_s *elem = malloc(sizeof(struct ignore_s));
    int rc = regcomp(&elem->regex, expr, REG_EXTENDED|REG_NOSUB);

    if (rc) {
	fprintf(stderr, "ERROR: bad regexp '%s'\n", expr);
	free(elem);
    } else {
	elem->patt = expr;
	elem->next = ignore_list;
	ignore_list = elem;
    }
}

/* Check a string against the list of patterns */
int check_ignore(const char *line) {
    struct ignore_s *elem;

    for (elem = ignore_list; elem; elem = elem->next) {
	if (0 == regexec(&elem->regex, line, 0, NULL, 0)) {
	    verbose("@ ignore line '%s' by pattern '%s'\n", line, elem->patt);
	    return 1;
	}
    }

    return 0;
}

/* Init the array of saved lines of output */
void init_output(void) {
    const char *str = getenv("SEQ_STDERR_LIMIT");
    int val;

    output.limit = 40;
    if (str && (sscanf(str, "%d", &val) == 1) && (val > 0)) {
	output.limit = val;
    }

    output.next = 0;
    output.array = calloc(output.limit, sizeof(char *));
}

/* Add a line to the saved output, discarding an old one if needed */
void push_line(char *line) {
    int i = output.next;

    output.next = i+1;
    if (output.next == output.limit) output.next = 0;

    free(output.array[i]);
    output.array[i] = line;
}

/* Add a line to saved output, making a copy of the argument */
void push_dup(const char *line) {
    push_line(strdup(line));
}

/* Dump the saved output */
void dump_lines(void) {
    int i = output.next;
    int j;

    if (output.array[i]) {
	fprintf(stderr, "...showing only last %i lines...\n", output.limit);
    }
    
    for (j = 0; j < output.limit; ++j) {
	if (output.array[i]) {
	    fputs(output.array[i], stderr);
	}
	if (++i == output.limit) i = 0;
    }
}

void cleanup(void) {
    struct ignore_s *elem;
    int i;

    for (i = 0; i < output.limit; ++i) {
	free(output.array[i]);
    }
    free(output.array);

    elem = ignore_list;
    while (elem) {
	struct ignore_s *next = elem->next;
	regfree(&elem->regex);
	free(elem->patt);
	free(elem);
	elem = next;
    }
}

/* Kill the child's pgrp on SIGALRM */
void alarm_handler(int signo) {
    if (child_pid) {
	kill(-child_pid, SIGKILL);
    }
    push_dup("!!! Alarm clock expired\n");
    fail++;
}

int main(int argc, char **argv)
{
    FILE *child;
    char *line = NULL;
    size_t len = 0;
    int lineno = 0;
    int done = 0;
    int exit_status = 0;
    int exit_signal = -1;
    int rc;

    /* Initialization */
    init_output();
    push_ignore(strdup("^#"));
    signal(SIGALRM, alarm_handler);

    /* Parse command line */
    if ((argc > 1) && !strcmp(argv[1], "-v")) {
	opt_verbose = 1;
    }

    /* Start the actual test program */
    child = spawn_child(argv[0]);
    if (!child) {
	fprintf(stderr, "Failed to start the test\n");
	return -1;
    }

    /* Process the tests output */
    /* loop over lines of the child's output */
    while (crut_getline(&line, &len, child) != -1) {
	int tmp_int;
	char *tmp_str;

	/* Save in the output ring buffer */
	push_dup(line);

	/* Drop the newline */
	line = crut_chomp(line);

	/* Process directive lines and ignored patterns */
	if (!strncmp(line, "#ST_IGNORE:", 11)) {
	    tmp_str = strdup(line+11);
	    push_ignore(tmp_str);
	    verbose("@ ignore pattern '%s'\n", tmp_str);
	    continue;
	} else if (!strncmp(line, "#ST_SIGNAL:", 11)) {
	    exit_signal = atoi(line+11);
	    exit_status = -1;
	    verbose("@ expect fatal signal %d\n", exit_signal);
	    continue;
	} else if (!strncmp(line, "#ST_RETURN:", 11)) {
	    exit_status = atoi(line+11);
	    exit_signal = -1;
	    verbose("@ expect exit status %d\n", exit_status);
	    continue;
	} else if (!strncmp(line, "#ST_ALARM:", 10)) {
	    alarm(atoi(line+10));
	    verbose("@ set alarm %d\n", atoi(line+10));
	    continue;
	} else if (check_ignore(line)) {
	    continue;
	}

	/* look for the required "tag": an integer at the start of the line.
	 * Note the check of strspn() here ensures we DON'T allow any leading
	 * whitespace as use of sscanf() alone would do.
	 */
        tmp_str = line + strspn(line, "0123456789");
	if ((tmp_str == line) || (1 != sscanf(line, "%d ", &tmp_int)) || (tmp_int != lineno)) {
	    push_line(crut_aprintf("!!! Expecting tag %d, but got '%s'\n", lineno, line));
	    ++fail;
	    continue;
	}

	/* Look for possible whitespace+"DONE" */
        tmp_int = strspn(tmp_str, "\t ");
	if (tmp_int && !strcmp(tmp_str+tmp_int, "DONE")) {
	    done = lineno + 1;
	}

	/* Advance the lineno that the tag will be checked against */
	++lineno;
    }
    fclose(child);
    free(line);

    /* Check that the output is properly terminated */
    if (!done) {
	push_dup("!!! Missing final DONE\n");
	++fail;
    } else if (done != lineno) {
	push_dup("!!! Output follows DONE\n");
	++fail;
    }

    /* Reap the child and check the exit status */
    while ((waitpid(child_pid, &rc, 0) < 0) && (errno == EINTR)) {
	    ; /* retry on non-fatal signals */
    }
    if (WIFEXITED(rc) && (WEXITSTATUS(rc) == 77)) {
	/* Automake's special "skipped test" case. */
	return 77;
    } else if (WIFEXITED(rc)) {
	int tmp = WEXITSTATUS(rc);
	if (tmp != exit_status) {
	    push_line(crut_aprintf("!!! Test exited with unexpected status %d\n", tmp));
	    fail++;
	}
    } else {
	int tmp = WTERMSIG(rc);
	if (tmp != exit_signal) {
	    push_line(crut_aprintf("!!! Test killed unexpectedly by signal %d\n", tmp));
	    fail++;
	}
    }

    if (fail) {
	fprintf(stderr, "Detected %d failures in %s:\n", fail, argv[0]);
	dump_lines();
    }

    cleanup();

    return fail ? 1 : 0;
}
