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
 * $Id: cr_restart.c,v 1.68.4.1 2009/06/06 20:27:09 phargrov Exp $
 */

#define _LARGEFILE64_SOURCE 1   /* For O_LARGEFILE */

#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <sys/wait.h>
#include <sched.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>

#ifndef O_LARGEFILE
  #define O_LARGEFILE 0
#endif

#include "libcr.h"

/* can't use argv[0] to get name, since libtool screws it up */
#define MY_NAME	"cr_restart"

static int verbose = 0;
static enum {
	kmsg_none = 0,
	kmsg_err,
	kmsg_warn,
} kmsg_level = kmsg_err;

enum {
    HOOK_SUCCESS = 0,
    HOOK_FAIL_TEMP,	/* transient condition, such as pid conflict */
    HOOK_FAIL_ENV,	/* "environmental" condition, such as file missing or bad permissions */
    HOOK_FAIL_ARGS,	/* permanent failure due to invalid args (inc. missing or unreadable file) */
    HOOK_FAIL_PERM,	/* permanent failure, such as corrupted context file (or unknown failure) */
    NUMHOOKS
};
static char *event_hooks[NUMHOOKS];

static int run_hook(int hook) {
    const char *cmd = event_hooks[hook];
    int retval = 0;

    if (cmd && cmd[0]) {
	int status = system(cmd);
	retval = WIFEXITED(status) ?  WEXITSTATUS(status) : -1;
    }

    return retval;
}

static void usage(FILE *stream, int fail)
{
    if (fail) {
	int hookval = run_hook(HOOK_FAIL_ARGS);
	if (hookval) exit(hookval);
    }
    if ((verbose < 0) && (stream == stderr)) {
        exit(fail ? -1 : 0);
    }

    fprintf(stream,
"Usage: " MY_NAME " [options] [checkpoint_file]\n"
"\n"
"Options:\n"
"General options:\n"
"  -?, --help          print this help message.\n"
"  -v, --version       print version information.\n" 
"  -q, --quiet         suppress error/warning messages to stderr.\n" 
"\n"

"Options for source location of the checkpoint:\n"
"  -d, --dir DIR       checkpoint read from directory DIR, with one\n"
"                      'context.ID' file per process (unimplemented).\n"
"  -f, --file FILE     checkpoint read from FILE.\n"
"  -F, --fd FD         checkpoint read from an open file descriptor.\n"
"  Options in this group are mutually exclusive.\n"
"  If no option is given from this group, the default is to take\n"
"  the final argument as FILE.\n"
"\n"
"Options for signal sent to process(es) after restart:\n"
"      --run           no signal sent: continue execution (default).\n"
"  -S, --signal NUM    signal NUM sent to all processes/threads.\n"
"      --stop          SIGSTOP sent to all processes.\n"
"      --term          SIGTERM sent to all processes.\n"
"      --abort         SIGABRT sent to all processes.\n"
"      --kill          SIGKILL sent to all processes.\n"
"      --cont          SIGCONT sent to all processes.\n"
//"      --early-stop    freeze processes for a debugger.\n"
//"      --early-abrt    abort processes (dumps core if allowed).\n"
"  Options in this group are mutually exclusive.\n"
"  If more than one is given then only the last will be honored.\n"
"\n"
"Options for checkpoints of restarted process(es):\n"
"      --omit-maybe    use a heuristic to omit cr_restart from checkpoints (default)\n"
"      --omit-always   always omit cr_restart from checkpoints\n"
"      --omit-never    never omit cr_restart from checkpoints\n"
"\n"
"Options for alternate error handling:\n"
"      --run-on-success='cmd'    run the given command on success\n"
"      --run-on-fail-args='cmd'  run the given command invalid arguments\n"
"      --run-on-fail-temp='cmd'  run the given command on 'temporary' failure\n"
"      --run-on-fail-env='cmd'   run the given command on 'environmental' failure\n"
"      --run-on-fail-perm='cmd'  run the given command on 'permanent' failure\n"
"      --run-on-failure='cmd'    run the given command on any failure\n"
"\n"
"Options for relocation:\n"
"      --relocate OLDPATH=NEWPATH    map paths of files and directories to\n"
"                                    new locations by prefix replacement.\n"
"\n"
"Options for restoring pid, process group and session ids\n"
"      --restore-pid       restore pids to saved values (default).\n"
"      --no-restore-pid    restart with new pids.\n"
"      --restore-pgid      restore pgid to saved values.\n"
"      --no-restore-pgid   restart with new pgids (default).\n"
"      --restore-sid       restore sid to saved values.\n"
"      --no-restore-sid    restart with new sids (default).\n"
"  Options in each restore/no-restore pair are mutually exclusive.\n"
"  If both are given then only the last will be honored.\n"
"\n"
"Options for kernel log messages (default is --kmsg-error):\n"
"      --kmsg-none     don't report any kernel messages.\n"
"      --kmsg-error    on restart failure, report on stderr any kernel\n"
"                      messages associated with the restart request.\n"
"      --kmsg-warning  report on stderr any kernel messages associated\n"
"                      with the restart request, regardless of success\n"
"                      or failure.  Messages generated in the absence of\n"
"                      failure are considered to be warnings.\n"
"  Options in this group are mutually exclusive.\n"
"  If more than one is given then only the last will be honored.\n"
"  Note that --quiet suppresses all stderr output, including these messages.\n"
    );

    exit(fail ? -1 : 0);
}

sig_atomic_t child = 0; /* who did we spawn? */
static void signal_child (int, siginfo_t *, void *);

static void signal_self(int sig)
{
    struct sigaction sa;

    /* restore default (in kernel) handler */
    sa.sa_handler = SIG_DFL;
    sa.sa_flags = SA_RESTART | SA_NOMASK;
    sigemptyset(&sa.sa_mask);
    (void)sigaction(sig, &sa, NULL);

    /* send to self */
    raise(sig);

    /* restore self as handler */
    sa.sa_sigaction = &signal_child;
    sa.sa_flags = SA_RESTART | SA_NOMASK | SA_SIGINFO;
    (void)sigaction(sig, &sa, NULL);
}

/* Best-effort to forward all signals to the restarted child
 * XXX: This is probably not complete in terms of special-cases
 */
static void signal_child (int sig, siginfo_t *siginfo, void *context)
{
    if ((siginfo->si_code > 0) &&	/* si_code > 0 indicates sent by kernel */
	(sig == SIGILL || sig == SIGFPE || sig == SIGBUS || sig == SIGSEGV )) {
	/* This signal is OUR error, so we don't forward */
	signal_self(sig);
    } else if (sig == SIGTSTP || sig == SIGTTIN || sig == SIGTTOU) {
	/* The catchable stop signals go to child AND self */
	(void)kill(child, sig);
	signal_self(sig);
    } else {
	/* default case */
	(void)kill(child, sig);
    }
}

static char *kmsgs = NULL;

/* ... share any error/warning messages from the kernel */
static void show_kmsgs(void)
{
    if (kmsgs) {
	/* prints each line with "- " as a prefix */
	char *p = kmsgs;
	while (*p) {
	    char *n = strchr(p, '\n');
	    if (!n) break;
	    *n = '\0';
	    fputs("- ", stderr);
	    fputs(p, stderr);
	    fputc('\n', stderr);
	    p = n+1;
	}
    }
    free(kmsgs);
    kmsgs = NULL;
}

static void die(int code, int hook, const char *format, ...)
		__attribute__ ((noreturn, format (printf, 3, 4)));
/*
 * die(code, hook, format, args...)
 *
 * Runs the indicated hook, exiting w/ its return code if non-zero.
 * Otherwise sends an error message to stderr and exits program with indicated code.
 */
static void die(int code, int hook, const char *format, ...)
{
    va_list args;
    int hookval;

    hookval = run_hook(hook);
    if (hookval) exit(hookval);

    va_start(args, format);
    if (verbose >= 0) {
      show_kmsgs();
      vfprintf(stderr, format, args);
    }
    va_end(args);

    exit(code);
}

static void warn(const char *format, ...)
		__attribute__ ((format (printf, 1, 2)));
/*
 * warn(format, args...)
 *
 * Sends a warning message to stderr
 */
static void warn(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    if (verbose >= 0) {
      vfprintf(stderr, format, args);
    }
    va_end(args);
}

static void print_version()
{
    printf(MY_NAME " version %s\n", CR_RELEASE_VERSION);
}

static int
readint(const char *arg)
{
    int val;
    char *endptr;

    val = strtol(arg, &endptr, 10);
    if ((*arg == '\0') || (*endptr != '\0')) {
	usage(stderr, 1);
    }

    return val;
}

static char *
strip_trailing(char *path, char c)
{
    size_t len;

    len = strlen(path);
    while (len--) {
	if (path[len] != c) {
	    break;
	}

	path[len] = '\0';
    }

    return path;
}

/* Note: path[n].oldpath and .newpath share the storage addressed by .oldpath */
static struct cr_rstrt_relocate *
parse_relocate_arg(struct cr_rstrt_relocate *reloc, const char *arg)
{
    char *oldpath, *newpath;
    char *p;
    int count;

    if (!reloc) {
	reloc = calloc(1, CR_RSTRT_RELOCATE_SIZE(CR_MAX_RSTRT_RELOC));
	if (!reloc) {
	    die(ENOMEM, HOOK_FAIL_ARGS, "Out of memory\n");
	}
    }

    count = reloc->count;
    if (count >= CR_MAX_RSTRT_RELOC) {
	die(EINVAL, HOOK_FAIL_ARGS,
	    "Too many relocations (max is %d).\n", CR_MAX_RSTRT_RELOC);
    }

    oldpath = strdup(arg);
    if (!oldpath) {
        die(ENOMEM, HOOK_FAIL_ARGS, "Out of memory\n");
    }

    /* Parse arg looking for '=' while parsing "\\" and "\=" sequences */
    p = oldpath;
    newpath = NULL;
    while (p[0]) {
	if (p[0] == '\\') { /* Handle "\\" and "\=" */
	    const char c = p[1];
	    if ((c == '\\') || (c == '=')) {
		/* Squeeze out the current character */
		memmove(p, p+1, strlen(p+1) + 1/* for \0 */);
	    } else {
		/* '\' not special in any other context */
	    }
	} else if (p[0] == '=') {
	    if (!newpath) {
		p[0] = '\0';
		newpath = p+1;
		/* parsing continues for escape sequences */
	    } else {
		die(EINVAL, HOOK_FAIL_ARGS,
		    "Relocation argument '%s' contains multiple unescaped '=' characters.\n", arg);
	    }
	}
	++p;
    }
    if (!newpath) {
	    die(EINVAL, HOOK_FAIL_ARGS, "Relocation argument '%s' contains no unescaped '=' characters.\n", arg);
    }

    /* Require first char to be '/' - a full path */
    if (oldpath[0] != '/' || newpath[0] != '/') {
	die(EINVAL, HOOK_FAIL_ARGS, "Relocation paths must begin with '/', but have '%s'\n", arg);
    }

    /* "Trim" trailing slashes, always keeping the initial one */
    strip_trailing(oldpath+1, '/');
    strip_trailing(newpath+1, '/');

    /* Silently discard useless identity relocations */
    if (!strcmp(oldpath, newpath)) {
	free(oldpath);
	return reloc;
    }

    /* Install */
    reloc->path[count].oldpath = oldpath;
    reloc->path[count].newpath = newpath;
    reloc->count++;

    return reloc;
}

/* getopt enum */
enum {
    opt_run = 1000,
    opt_stop,
    opt_term,
    opt_kill,
    opt_abort,
    opt_cont,
    opt_early_stop,
    opt_early_abrt,
    opt_omit_maybe,
    opt_omit_always,
    opt_omit_never,
    opt_hook_success,
    opt_hook_fail_temp,
    opt_hook_fail_env,
    opt_hook_fail_args,
    opt_hook_fail_perm,
    opt_hook_failure, /* equiv to opt_hook_fail_* */
    opt_relocate,
    opt_kmsg_none,
    opt_kmsg_error,
    opt_kmsg_warning,
    opt_restore_pid,
    opt_no_restore_pid,
    opt_restore_pgid,
    opt_no_restore_pgid,
    opt_restore_sid,
    opt_no_restore_sid,
};

/* try to exit such that our exit code is same as our child */
static void mimic_exit(int status)
{
    if (WIFEXITED(status)) {
	/* easy to mimic normal return */
	exit(WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
	/* disable generation of a 'core' */
	struct rlimit r;
	r.rlim_cur = r.rlim_max = 0;
	(void)setrlimit(RLIMIT_CORE, &r);

	/* now raise the signal */
	signal_self(WTERMSIG(status));
    } else {
	warn("Unexpected status from child\n");
	exit(-1);
    }
}

static volatile enum {
	CR_RSTRT_OMIT_MAYBE = 0,
	CR_RSTRT_OMIT_ALWAYS,
	CR_RSTRT_OMIT_NEVER
} omit_self = CR_RSTRT_OMIT_MAYBE;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/* "my" pid (done this way due to LinuxThreads limitations */
static int mypid;

/* Callback to perform a critical section OR get us omitted from the checkpoint */
static int my_cb(void *arg) {
    int status = CR_CHECKPOINT_PERM_FAILURE;
    const struct cr_checkpoint_info *info = cr_get_checkpoint_info();

    switch (omit_self) {
	case CR_RSTRT_OMIT_ALWAYS:
	    status = CR_CHECKPOINT_OMIT;
	    break;

	case CR_RSTRT_OMIT_NEVER:
	    status = CR_CHECKPOINT_READY;
	    break;

	case CR_RSTRT_OMIT_MAYBE:
	    if ((info->target == mypid) || (getppid() == 1)) {
		/* I am "root" of the checkpoint or an orphan (child of init) */
		status = CR_CHECKPOINT_OMIT;
	    } else {
		status = CR_CHECKPOINT_READY;
	    }
	    break;
    }

    /* Ensure that either all or none of the chilren exist */
    pthread_mutex_lock(&lock);

    if (child) {
        /* Include my children */
        int rc = cr_forward_checkpoint(CR_SCOPE_TREE, mypid);
        if (rc < 0) {
	    if (errno == ESRCH) {
	        /* Empty request, perhaps all children did OMIT. */
	    } else {
	        warn("WARNING: cr_forward_checkpoint() returned %d (%s)\n",
		     errno, cr_strerror(errno));
	    }
        }
    } else {
	/* Don't omit if we didn't do anything yet */
	status = CR_CHECKPOINT_READY;
    }

    /* Go! */
    cr_checkpoint(status);

    pthread_mutex_unlock(&lock);
    return 0;
}


/* Type of source */
enum {
   src_default,
   src_file,
   src_dir,
   src_fd,
};

/* Terminate as appropriate for the given error code */
static void failure(void) {
    int save_errno = errno;
    int hook = HOOK_FAIL_PERM; /* The default */

    switch (save_errno) {
	/* "TEMPORARY" failures are ones that might resolve themselves: */
	case ENOMEM:
	case EBUSY:	 /* e.g. PID in use */
	    hook = HOOK_FAIL_TEMP;
	    break;

	/* "ENVIRONMENTAL" failures are ones that might be resolved manually: */
	case EACCES:	/* e.g. file permissions */
	case EPERM:	/* e.g. directory permissions */
	case ENOENT:	/* e.g. file is missing */
	case EEXIST:	/* e.g. fs object conflicts with an existing object */
	case EISDIR:	/* e.g. fs object conflicts with an existing object */
	case ENOTDIR:	/* e.g. fs object conflicts with an existing object */
	    hook = HOOK_FAIL_ENV;
	    break;

	/* All remaining errno values (including unknown ones) are "PERMANENT" failure */
	case ENOSYS:	/* e.g. unsupported/unimplemented "thingy" to restore */
	case EIO:	/* e.g. fs problems */
	case EINVAL:	/* e.g. corrupted value(s) */
	    hook = HOOK_FAIL_PERM;
    }

    die(save_errno, hook, "Restart failed: %s\n", cr_strerror(errno));
}

int main(int argc, char **argv)
{
    cr_restart_args_t args;
    struct cr_rstrt_relocate *reloc = NULL;
    int err;
    char *context_filename = NULL;
    /*char *context_dirname = NULL;*/
    int context_fd = -1;
    int src_type = src_default;
    struct sigaction sa;
    sigset_t sigmask, oldmask;
    int sig;
    int child_status;
    int signal = 0;
    cr_client_id_t client_id;
    cr_callback_id_t cb_id;
    cr_restart_handle_t cr_handle;
    int flags = CR_RSTRT_ASYNC_ERR | CR_RSTRT_RESTORE_PID;

    char * shortflags = "qvhS:F:d:f:";  /* 1 colon == requires argument */
    struct option longflags[] = {
	/* misc options: */
	{ "help",         no_argument,  0, 'h' },
	{ "version",      no_argument,  0, 'v' },
	{ "quiet",        no_argument,  0, 'q' },
	/* source: */
	{ "file",         required_argument, 0, 'f' },
	{ "dir",          required_argument, 0, 'd' },
	{ "fd",           required_argument, 0, 'F' },
	/* signal options: */
	{ "signal",       required_argument, 0, 'S' },
	{ "run",          no_argument,  0, opt_run },
	{ "stop",         no_argument,  0, opt_stop },
	{ "freeze",       no_argument,  0, opt_stop },
	{ "term",         no_argument,  0, opt_term },
	{ "kill",         no_argument,  0, opt_kill },
	{ "abort",        no_argument,  0, opt_abort },
	{ "cont",         no_argument,  0, opt_cont },
	{ "early-stop",   no_argument,  0, opt_early_stop },
	{ "early-abrt",   no_argument,  0, opt_early_abrt },
	/* omit options: */
	{ "omit-maybe",   no_argument,  0, opt_omit_maybe },
	{ "omit-always",  no_argument,  0, opt_omit_always },
	{ "omit-never",   no_argument,  0, opt_omit_never },
	{ "run-on-success",	required_argument,  0, opt_hook_success },
	{ "run-on-fail-temp",	required_argument,  0, opt_hook_fail_temp },
	{ "run-on-fail-env",	required_argument,  0, opt_hook_fail_env },
	{ "run-on-fail-args",	required_argument,  0, opt_hook_fail_args },
	{ "run-on-fail-perm",	required_argument,  0, opt_hook_fail_perm },
	{ "run-on-failure",	required_argument,  0, opt_hook_failure },
	/* map relocation option: */
	{ "relocate",		required_argument,  0, opt_relocate },
	/* kmsg options: */
	{ "kmsg-none",    no_argument,  0, opt_kmsg_none},
	{ "kmsg-error",   no_argument,  0, opt_kmsg_error},
	{ "kmsg-warning", no_argument,  0, opt_kmsg_warning},
        /* pid/pgid/sid restore: */
	{ "restore-pid",     no_argument,  0, opt_restore_pid},
	{ "no-restore-pid",  no_argument,  0, opt_no_restore_pid},
	{ "restore-pgid",     no_argument,  0, opt_restore_pgid},
	{ "no-restore-pgid",  no_argument,  0, opt_no_restore_pgid},
	{ "restore-sid",     no_argument,  0, opt_restore_sid},
	{ "no-restore-sid",  no_argument,  0, opt_no_restore_sid},
	{ 0,	     0,		        0, 0  }
    };

    opterr = 0;
    while (1) {
	int longindex = -1;
	int opt = getopt_long(argc, argv, shortflags, longflags, &longindex);

	if (opt == -1)
	    break;		    /* reached last option */

	switch (opt) {
	    /* source: */
	    case 'f':
		if (src_type != src_default) {
		    die(EINVAL, HOOK_FAIL_ARGS, "multiple source arguments specified\n");
		}
	 	src_type = src_file;
		context_filename = strdup(optarg);
		if (!context_filename) {
		    die(ENOMEM, HOOK_FAIL_TEMP, "strdup failed on string '%s'\n", optarg);
		}
		break;
	    case 'd':
		if (src_type != src_default) {
		    die(EINVAL, HOOK_FAIL_ARGS, "multiple source arguments specified\n");
		}
	 	src_type = src_dir;
#if 1
		die(EINVAL, HOOK_FAIL_ARGS, "-d flag not yet implemented\n");
#else
		context_dirname = strdup(optarg);
		if (!context_dirname) {
		    die(ENOMEM, HOOK_FAIL_TEMP, "strdup failed on string '%s'\n", optarg);
		}
		break;
#endif
	    case 'F':
		if (src_type != src_default) {
		    die(EINVAL, HOOK_FAIL_ARGS, "multiple source arguments specified\n");
		}
	 	src_type = src_fd;
		context_fd = readint(optarg);
		if (context_fd < 0) {
	            die(EINVAL, HOOK_FAIL_ARGS,
			"FD argument must be a non-negatvive integer (%d was given)\n",
			context_fd);
		}
		break;
	    /* Signal */
	    case 'S':
		signal = readint(optarg);
		if ((signal <= 0) || (signal >= _NSIG)) {
		    die(EINVAL, HOOK_FAIL_ARGS,
			"Valid signal numbers are from 1 to %d, inclusive.\n",
			_NSIG - 1);
		}
		break;
	    case opt_run: /* --run */
		signal = 0;
		break;
	    case opt_stop: /* --stop */
		signal = SIGSTOP;
		break;
	    case opt_term: /* --term */
		signal = SIGTERM;
		break;
	    case opt_kill: /* --kill */
		signal = SIGKILL;
		break;
	    case opt_abort: /* --abort */
		signal = SIGABRT;
		break;
	    case opt_cont: /* --cont */
		signal = SIGCONT;
		break;
	    case opt_early_stop: /* --early_stop */
		signal = -SIGSTOP;
		break;
	    case opt_early_abrt: /* --early_abrt */
		signal = -SIGABRT;
		break;
	    /* Omit */
	    case opt_omit_maybe:
		omit_self = CR_RSTRT_OMIT_MAYBE;
		break;
	    case opt_omit_always:
		omit_self = CR_RSTRT_OMIT_ALWAYS;
		break;
	    case opt_omit_never:
		omit_self = CR_RSTRT_OMIT_NEVER;
		break;
	    /* Hooks */
	    case opt_hook_success:
		event_hooks[HOOK_SUCCESS] = strdup(optarg);
		break;
	    case opt_hook_fail_temp:
		event_hooks[HOOK_FAIL_TEMP] = strdup(optarg);
		break;
	    case opt_hook_fail_env:
		event_hooks[HOOK_FAIL_ENV] = strdup(optarg);
		break;
	    case opt_hook_fail_args:
		event_hooks[HOOK_FAIL_ARGS] = strdup(optarg);
		break;
	    case opt_hook_fail_perm:
		event_hooks[HOOK_FAIL_PERM] = strdup(optarg);
		break;
	    case opt_hook_failure:
		event_hooks[HOOK_FAIL_TEMP] = strdup(optarg);
		event_hooks[HOOK_FAIL_ENV]  = strdup(optarg);
		event_hooks[HOOK_FAIL_ARGS] = strdup(optarg);
		event_hooks[HOOK_FAIL_PERM] = strdup(optarg);
		break;
	    /* Map Relocation */
	    case opt_relocate:
		reloc = parse_relocate_arg(reloc, optarg);
		break;
	    /* kmsg options: */
	    case opt_kmsg_none:
		kmsg_level = kmsg_none;
		break;
	    case opt_kmsg_error:
		kmsg_level = kmsg_err;
		break;
	    case opt_kmsg_warning:
		kmsg_level = kmsg_warn;
		break;
	    /* pid/pgid/sid restore options: */
	    case opt_restore_pid:
		flags |= CR_RSTRT_RESTORE_PID;
		break;
	    case opt_no_restore_pid:
		flags &= ~CR_RSTRT_RESTORE_PID;
		break;
	    case opt_restore_pgid:
		flags |= CR_RSTRT_RESTORE_PGID;
		break;
	    case opt_no_restore_pgid:
		flags &= ~CR_RSTRT_RESTORE_PGID;
		break;
	    case opt_restore_sid:
		flags |= CR_RSTRT_RESTORE_SID;
		break;
	    case opt_no_restore_sid:
		flags &= ~CR_RSTRT_RESTORE_SID;
		break;
	    /* General */
	    case 'q':
		verbose = -1;
		break;
	    case 'v':
		print_version();
		return 0;
	    case '?':
		if (optopt != '?') { /* Unrecognized option case */
		  usage(stderr, 1);
		}
	        /* fall through for help... */
	    case 'h':
		usage(stdout, 0);
		return 0;
	    default: /* reachable? */
		usage(stderr, 1);
	}
    }

    /* Require either exactly one filename argument, or none at all */
    if (src_type == src_default) {
	if ((argc - optind) != 1) {
            usage(stderr, 1);
	} else {
	    context_filename = argv[optind];
	    src_type = src_file;
	}
    } else if (argc != optind) {
        usage(stderr, 1);
    }

    /* Record our pid */
    mypid = getpid();

    /* ... connect to the kernel */
    client_id = cr_init();
    if (client_id < 0) {
        die(client_id, HOOK_FAIL_PERM, "Failed cr_init(): %d\n", client_id);
    }

    /* ... register callback to exclude us from the checkpoint */
    cb_id = cr_register_callback(&my_cb, NULL, CR_THREAD_CONTEXT);
    if (cb_id < 0) {
        die(cb_id, HOOK_FAIL_PERM, "Failed cr_regsiter_callback(): %d\n", cb_id);
    }

    /* ... open up the context file */
    if (src_type == src_file) {
	context_fd = open(context_filename, O_RDONLY | O_LARGEFILE);
	if (context_fd < 0) {
            die(context_fd, HOOK_FAIL_ARGS, "Failed to open(%s, O_RDONLY): %s\n",
		context_filename, strerror(errno));
	}
    } else if (src_type == src_fd) {
	/* OK */
    } else {
        die(cb_id, HOOK_FAIL_PERM, "Internal error - unknow source type %d\n", src_type);
    }

    /* ... initialize the request structure */
    cr_initialize_restart_args_t(&args);
    args.cr_fd       = context_fd;
    args.cr_signal   = signal;
    args.cr_relocate = reloc;
    args.cr_flags    = flags;

    /* START of critical section.
     * Since we don't (yet?) try to checkpoint the state associated with
     * a partially completed restart, we must exclude checkpoints between
     * CR_OP_RSTRT_REQ and CR_OP_RSTRT_REAP
     */
    pthread_mutex_lock(&lock);

    /* ... issue the request */
    err = cr_request_restart(&args, &cr_handle);
    if (err < 0) {
        failure();
    }

    /* ... prepare to forward signals */
    /* XXX: what should we use in place of _NSIG? */
    sigfillset(&sigmask);
    sigdelset(&sigmask, SIGSTOP);
    sigdelset(&sigmask, SIGKILL);
    sigdelset(&sigmask, SIGCHLD);
    sigprocmask(SIG_SETMASK, &sigmask, &oldmask);
    sa.sa_sigaction = &signal_child;
    sa.sa_flags = SA_RESTART | SA_NOMASK | SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    for (sig = 1; sig < _NSIG; ++sig) {
	if (sig == SIGCHLD) continue;
	if (sig == SIGKILL) continue;
	if (sig == SIGSTOP) continue;
	if (sig == CR_SIGNUM) continue;

	/* best-effort so we ignore failures */
	(void)sigaction(sig, &sa, NULL);
    }

    /* ... close the context file so a restarted cr_restart won't depend on it */
    /* XXX: How early can we do this safely?  No documented answer! */
    (void)close(context_fd);

    /* ... wait for completion */
    do {
	err = cr_wait_restart(&cr_handle, NULL);
	if (err == 0) {
	    /* 0 would mean timeout, but we passed NULL for the (struct timeval *) */
	    die(1, HOOK_FAIL_PERM, "cr_wait_restart returned unexpected 0");
        }
    } while ((err < 0) && (errno == EINTR));
    if (err < 0) {
	die(errno, HOOK_FAIL_PERM, "cr_wait_restart: %s\n", cr_strerror(errno));
    }

    /* ... collect kernel logs if requested */
    if (kmsg_level != kmsg_none) {
        /* NOTE: we ignore any failure to malloc (EFAULT) or to collect the log */
        err = cr_log_restart(&cr_handle, 0, NULL);
        if (err > 0) {
            int len = err;
	    kmsgs = malloc(len);
            (void)cr_log_restart(&cr_handle, len, kmsgs);
        }
    }

    /* ... collect result and triage any errors */
    err = cr_reap_restart(&cr_handle);
    if (err < 0) {
	failure();
    } else {
	int hookval = run_hook(HOOK_SUCCESS);
	if (hookval) return hookval; /* XXX: do we really want to return here? */

	/* Record the pid for signal forwarding */
	child = (pid_t)err;
    }

    /* END of critical section */
    pthread_mutex_unlock(&lock);

    /* show any warnings from the kernel */
    if ((verbose >= 0) && (kmsg_level == kmsg_warn)) {
	show_kmsgs();
    }

    /* If there is no child (target was a child of init), return now */
    if (!child) return 0;

    /* ... now we forward the signals */
    sigprocmask(SIG_SETMASK, &oldmask, NULL);

    /* XXX should have the option of a non-blocking case */
    do {
        /* wait for the child we've inherited */
        err = waitpid(child, &child_status, __WCLONE | __WALL);
    } while ((err < 0) && (errno == EINTR)); /* retry on non-fatal signals */
    /* XXX wait for additional children? */
    if (err < 0) {
        /* Unable to reap the child */
        die(err, HOOK_FAIL_PERM, "waitpid(%d): %s\n", child, cr_strerror(errno));
    }	

    mimic_exit(child_status);
    return -1; /* NOT REACHED */
}
