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
 * $Id: cr_checkpoint.c,v 1.90 2008/08/27 21:57:40 phargrov Exp $
 */

#define _LARGEFILE64_SOURCE 1   /* For O_LARGEFILE */
#define _GNU_SOURCE 1	/* For strsignal() */
#define _BSD_SOURCE 1	/* For dirfd() */

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <libgen.h>
#include <dirent.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <assert.h>
#include <pthread.h>
#include <string.h>

#ifndef O_LARGEFILE
  #define O_LARGEFILE 0
#endif

#include "libcr.h"

/* can't use argv[0] to get name, since libtool screws it up */
#define MY_NAME	"cr_checkpoint"

static int verbose = 0;
static enum {
	kmsg_none = 0,
	kmsg_err,
	kmsg_warn,
} kmsg_level = kmsg_err;

static char *to_remove;

static void die(int code, const char *format, ...)
		__attribute__ ((noreturn, format (printf, 2, 3)));
static void print_err(const char *format, ...)
		__attribute__ ((format (printf, 1, 2)));
static void usage(FILE *stream, int exitcode)
		__attribute__ ((noreturn));

static void usage(FILE *stream, int exitcode)
{
    if ((verbose < 0) && (stream == stderr)) {
	exit(exitcode);
    }

    fprintf(stream, 
"Usage: " MY_NAME " [options] ID\n"
"\n"
"Options:\n"
"General options:\n"
"  -v, --verbose          print progress messages to stderr.\n"
"  -q, --quiet            suppress error/warning messages to stderr.\n"
"  -?, --help             print this message and exit.\n"
"      --version          print version information and exit.\n"
"\n"
"Options for scope of the checkpoint:\n"
"  -T, --tree             ID identifies a process id.  It and all\n"
"                         of its descendants are to be checkpointed.\n"
"                         This is the default.\n"
"  -p, --pid, --process   ID identifies a single process id.\n"
"  -g, --pgid, --group    ID identifies a process group id.\n"
"  -s, --sid, --session   ID identifies a session id.\n"
"\n"
"Options for destination location of the checkpoint:\n"
"  -c, --cwd              checkpoint saved as a single 'context.ID' file in\n"
"                         cr_checkpoint's working directory (default).\n"
"  -d, --dir DIR          checkpoint saved in new directory DIR, with one\n"
"                         'context.ID' file per process (unimplemented).\n"
"  -f, --file FILE        checkpoint saved as FILE.\n"
"  -F, --fd FD            checkpoint written to an open file descriptor.\n"
"\n"
"Options for creation/replacement policy for checkpoint files:\n"
"      --atomic           checkpoint created/replaced atomically (default).\n"
"      --backup[=NAME]    checkpoint created atomically, and any existing \n"
"                         checkpoint backed up to NAME or *.~1~, *.~2~, etc.\n"
"      --clobber          checkpoint written incrementally to target, \n"
"                         overwriting any pre-existing checkpoint.\n"
"      --noclobber        checkpoint will fail if the target file exists.\n"
"  These options are ignored if the destination is a file descriptor.\n"
"\n"
"Options for signal sent to process(es) after checkpoint:\n"
"      --run              no signal sent: continue execution (default).\n"
"  -S, --signal NUM       signal NUM sent to all processess.\n"
"      --stop             SIGSTOP sent to all processes.\n"
"      --term             SIGTERM sent to all processes.\n"
"      --abort            SIGABRT sent to all processes.\n"
"      --kill             SIGKILL sent to all processes.\n"
"      --cont             SIGCONT sent to all processes.\n"
//"      --early-stop       freeze processes for a debugger.\n"
//"      --early-abrt       abort processes (dumps core if allowed).\n"
"  Options in this group are mutually exclusive.\n"
"  If more than one is given then only the last will be honored.\n"
"\n"
"Options for file system synchronization (default is --sync):\n"
"      --sync             fsync checkpoint file(s) to disk (default).\n"
"      --nosync           do not fsync checkpoint file(s) to disk.\n"
"\n"
"Options to save optional portions of memory:\n"
"      --save-exe         save the executable file.\n"
"      --save-private     save private mapped files.\n"
"                         (executables and libraries are mapped this way)\n"
"      --save-shared      save shared mapped files.\n"
"                         (System V IPC is mapped this way).\n"
"      --save-all         save all of the above.\n"
"      --save-none        save none of the above (the default).\n"
"\n"
"Options for ptraced processes (default is --ptraced-error):\n"
"      --ptraced-error    return an error if a checkpoint is requested\n"
"                         of a process being ptraced.\n"
"      --ptraced-skip     ptraced processes are silently excluded from the\n"
"                         checkpoint request.  If the checkpoint scope is\n"
"                         --tree, then this will also exclude any children\n"
"                         of such processes.  No error is produced unless\n"
"                         this results in zero processes checkpointed.\n"
"      --ptraced-allow    checkpoint ptraced processes normally.\n"
"                         WARNING: This may require the tracer to \"continue\"\n"
"                         the target process(es), possibly more than once.\n"
"\n"
"Options for processes ptracing others (default is --ptracer-error):\n"
"      --ptracer-error    return an error if a checkpoint is requested\n"
"                         of a process which is ptracing others.\n"
"      --ptracer-skip     processes ptracing others are silently excluded\n"
"                         from the checkpoint request.  If the checkpoint\n"
"                         scope is --tree, then this will also exclude any\n"
"                         children of such processes.  No error is produced\n"
"                         unless this results in zero processes checkpointed.\n"
"\n"
"Options for kernel log messages (default is --kmsg-error):\n"
"      --kmsg-none        don't report any kernel messages.\n"
"      --kmsg-error       on checkpoint failure, report on stderr any kernel\n"
"                         messages associated with the checkpoint request.\n"
"      --kmsg-warning     report on stderr any kernel messages associated\n"
"                         with the checkpoint request, regardless of success\n"
"                         or failure.  Messages generated in the absence of\n"
"                         failure are considered to be warnings.\n"
"  Options in this group are mutually exclusive.\n"
"  If more than one is given then only the last will be honored.\n"
"  Note that --quiet suppresses all stderr output, including these messages.\n"
"\n"
"Misc Options:\n"
"  -t, --time SEC         allow only SEC seconds for target to complete\n"
"                         checkpoint (default: wait indefinitely).\n"
    );

    exit(exitcode);
}

static void print_version()
{
    printf(MY_NAME " version %s\n", CR_RELEASE_VERSION);
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

/*
 * die(code, format, args...)
 *
 * Sends an error message to stderr and exits program with indicated exit code.
 */
static void die(int code, const char *format, ...)
{
    va_list args;
    struct stat s;

    va_start(args, format);
    if (verbose >= 0) {
      show_kmsgs();
      vfprintf(stderr, format, args);
    }
    va_end(args);

    if (to_remove != NULL) 
	if (!stat(to_remove, &s)) 
	    unlink(to_remove);

    exit(code);
}

/*
 * print_err(format, args...)
 *
 * Works like fprintf(stderr, format, args...), but preserves errno.
 * No return value, since used only on error paths.
 */
static void print_err(const char *format, ...)
{
    int saved_errno = errno;
    va_list args;

    va_start(args, format);
    if (verbose >= 0) {
      vfprintf(stderr, format, args);
    }
    va_end(args);

    errno = saved_errno;
}



static int
readint(const char *arg, const char *argv0)
{
    int val;
    char *endptr;

    val = strtol(arg, &endptr, 10);
    if ((*arg == '\0') || (*endptr != '\0')) {
	usage(stderr, -1);
    }

    return val;
}

static inline int ftype_ok(mode_t mode) 
{
    return S_ISREG(mode) || S_ISFIFO(mode) || S_ISSOCK(mode) || S_ISDIR(mode);
}

static int 
openfile(const char *filename, int do_excl)
{
    int fd;
    struct stat st;
    int flags = O_WRONLY|O_CREAT|O_NOCTTY|O_NONBLOCK|O_LARGEFILE|O_TRUNC;
    
    if (do_excl) {
	flags |= O_EXCL;
    }
    if ((fd = open(filename, flags, 0400)) < 0) {
	print_err("Unable to open file '%s': %s\n", filename, strerror(errno));
	return -1;
    }
    if (fstat(fd, &st) < 0) {
	print_err("Unable to fstat opened file: %s\n", strerror(errno));
	return -1;
    }
    if (!ftype_ok(st.st_mode)) {
	print_err("File '%s' is not an acceptable file type (i.e. not a "
		  "regular file, socket, or FIFO)\n", filename);
	errno = EINVAL;
	return -1;
    } else if (S_ISDIR(st.st_mode)) {
	print_err("File '%s' is a directory: use the '--dir' flag for "
		  "directories\n", filename);
	errno = EISDIR;
	return -1;
    }

    return fd;
}

#if 0
static int 
opendirectory(const char *dirname)
{
    int fd;
    struct stat st;
    
    if ( (fd = open(dirname, O_NOCTTY|O_NONBLOCK)) == -1) {
	print_err("Unable to open directory '%s': %s\n", dirname,
		  strerror(errno));
	return -1;
    }
    if (fstat(fd, &st)) {
	print_err("Unable to fstat opened file: %s\n", strerror(errno));
	return -1;
    }
    if (!S_ISDIR(st.st_mode)) {
	print_err("'%s' is not a directory\n", dirname);
	errno = ENOTDIR;
	return -1;
    }

    return fd;
}
#endif

/* file synchronization behaviors */
enum {
    ATOMIC,
    BACKUP,
    CLOBBER
};

/* Values for long options w/o single-char equivalents. */
enum {
   opt_run = 1000,
   opt_stop,
   opt_term,
   opt_kill,
   opt_abort,
   opt_cont,
   opt_early_stop,
   opt_early_abrt,
   opt_sync,
   opt_nosync,
   opt_atomic,
   opt_backup,
   opt_clobber,
   opt_noclobber,
   opt_version,
   opt_save_exe,
   opt_save_private,
   opt_save_shared,
   opt_save_all,
   opt_save_none,
   opt_ptraced_error,
   opt_ptraced_allow,
   opt_ptraced_skip,
   opt_ptracer_error,
   opt_ptracer_skip,
   opt_kmsg_none,
   opt_kmsg_error,
   opt_kmsg_warning,
};

/* Type of destination */
enum {
   dest_default,
   dest_file,
   dest_dir,
   dest_fd,
   dest_cwd
};

/* Mutex */
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/* "my" pid (done this way due to LinuxThreads limitations */
static int mypid;

/* Callback to ensure consistent checkpoints */
static int my_cb(void *arg)
{
    const struct cr_checkpoint_info *info = cr_get_checkpoint_info();
    int rc;

    if (info->requester == mypid) {
	/* We are checkpointing ourself.  Exclude self. */
	rc = cr_checkpoint(CR_CHECKPOINT_OMIT);
    } else {
        /* Wait for checkpoint to complete (more flexible than a critical section) */
        pthread_mutex_lock(&lock);
        rc = cr_checkpoint(CR_CHECKPOINT_READY);
        pthread_mutex_unlock(&lock);
    }

    return 0;
}

int real_main(int argc, char **argv)
{
    cr_checkpoint_args_t cr_args;
    cr_checkpoint_handle_t cr_handle;
    int dest_type = dest_default;
    int chkpt_fd = -1;
    int do_excl = 0;
    int do_atomic = 1;
    int do_backup = 0;
    int do_sync = 1;
    cr_client_id_t client_id;
    cr_callback_id_t cb_id;
    char * chkpt_file = NULL;	/* file to checkpoint to */
    char * chkpt_dir = NULL;	/* directory to checkpoint to */
    char * chkpt_to = NULL;	/* name of checkpoint file or dir */
    char * rename_to = NULL;	/* final checkpoint file/dir, if different */
    char * backup_to = NULL;	/* backup file/dir, if needed */
    char * parent_dir = NULL;   /* parent directory of checkpoint */

    int secs = 0;
    int err;
    pid_t target = 0;
    cr_scope_t target_type = CR_SCOPE_TREE;
    int signal = 0;
    unsigned int cr_flags = CR_CHKPT_ASYNC_ERR;

    /* Parse cmdline options */
    char * shortflags = "f:d:F:S:pgsTct:qvh";  /* 1 colon == requires argument */
    struct option longflags[] = {
	/* target_type: */
	{ "pid",     no_argument      , 0, 'p' },
	{ "process", no_argument      , 0, 'p' },
	{ "pgid",    no_argument      , 0, 'g' },
	{ "group",   no_argument      , 0, 'g' },
	{ "sid",     no_argument      , 0, 's' },
	{ "session", no_argument      , 0, 's' },
	{ "tree",    no_argument      , 0, 'T' },
	/* destination: */
	{ "file",    required_argument, 0, 'f' },
	{ "dir",     required_argument, 0, 'd' },
	{ "fd",      required_argument, 0, 'F' },
	{ "cwd",     no_argument,       0, 'c' },
	/* creation/replacement policy */
	{ "atomic",  no_argument,	0, opt_atomic },
	{ "backup",  optional_argument,	0, opt_backup },
	{ "clobber", no_argument,	0, opt_clobber },
	{ "noclobber", no_argument,	0, opt_noclobber },
	/* signal options: */
	{ "signal",  required_argument, 0, 'S' },
	{ "run",     no_argument,       0, opt_run },
	{ "stop",    no_argument,       0, opt_stop },
	{ "term",    no_argument,       0, opt_term },
	{ "kill",    no_argument,       0, opt_kill },
	{ "abort",   no_argument,       0, opt_abort },
	{ "cont",    no_argument,       0, opt_cont },
	{ "early-stop", no_argument,    0, opt_early_stop },
	{ "early-abrt", no_argument,    0, opt_early_abrt },
	/* fsync options: */
	{ "sync",    no_argument,       0, opt_sync},
	{ "nosync",  no_argument,       0, opt_nosync},
	/* vmadump options: */
	{ "save-exe",     no_argument,  0, opt_save_exe},
	{ "save-private", no_argument,  0, opt_save_private},
	{ "save-shared",  no_argument,  0, opt_save_shared},
	{ "save-all",     no_argument,  0, opt_save_all},
	{ "save-none",    no_argument,  0, opt_save_none},
	/* ptraced options: */
	{ "ptraced-error",  no_argument,  0, opt_ptraced_error},
	{ "ptraced-allow",  no_argument,  0, opt_ptraced_allow},
	{ "ptraced-skip",   no_argument,  0, opt_ptraced_skip},
	/* ptracer options: */
	{ "ptracer-error",  no_argument,  0, opt_ptracer_error},
	{ "ptracer-skip",   no_argument,  0, opt_ptracer_skip},
	/* kmsg options: */
	{ "kmsg-none",    no_argument,  0, opt_kmsg_none},
	{ "kmsg-error",   no_argument,  0, opt_kmsg_error},
	{ "kmsg-warning", no_argument,  0, opt_kmsg_warning},
	/* misc options: */
	{ "time",    required_argument, 0, 't' },
	{ "quiet",   no_argument,       0, 'q' },
	{ "verbose", no_argument,       0, 'v' },
	{ "help",    no_argument,       0, 'h' },
	{ "version", no_argument,       0, opt_version },
	{ 0,	     0,			0,  0  }
    };

    opterr = 0;
    while (1) {
	int longindex = -1;
	int opt = getopt_long(argc, argv, shortflags, longflags, &longindex);

	if (opt == -1)
	    break;		    /* reached last option */

	switch (opt) {
	/* target_type: */
	    case 'p':
		target_type = CR_SCOPE_PROC;
		break;
	    case 'g':
		target_type = CR_SCOPE_PGRP;
		break;
	    case 's':
		target_type = CR_SCOPE_SESS;
		break;
	    case 'T':
		target_type = CR_SCOPE_TREE;
		break;
	/* destination: */
	    case 'f':
		if (dest_type != dest_default)
		    die (EINVAL, "conflicting destinations provided\n");
	 	dest_type = dest_file;
		chkpt_file = strdup(optarg);
		if (!chkpt_file) 
		    die(ENOMEM, "strdup failed on string '%s'\n", optarg);
		break;
	    case 'd':
#if 1
		die(EINVAL, "-d flag not yet implemented\n");
#else
		if (dest_type != dest_default)
		    die (EINVAL, "conflicting destinations specified\n");
	 	dest_type = dest_dir;
		chkpt_dir = strdup(optarg);
		if (!chkpt_dir) 
		    die(ENOMEM, "strdup failed on string '%s'\n", optarg);
		break;
#endif
	    case 'F':
		if (dest_type != dest_default)
		    die (EINVAL, "conflicting destinations specified\n");
	 	dest_type = dest_fd;
		chkpt_fd = readint(optarg, argv[0]);
		break;
	    case 'c':
		if (dest_type != dest_default)
		    die (EINVAL, "conflicting destinations specified\n");
	 	dest_type = dest_cwd;
		/* nothing to do */
		break;
	/* creation/replacement policy */
	    case opt_atomic:
		do_excl = 0;
		do_atomic = 1;
		do_backup = 0;
		break;
	    case opt_backup:
		do_excl = 0;
		do_atomic = 1;
		do_backup = 1;
		if (optarg) {
		    backup_to = strdup(optarg);
		    if (!backup_to)
			die(errno, "Error during strdup: %s\n", strerror(errno));
		}
		break;
	    case opt_clobber:
		do_excl = 0;
		do_atomic = 0;
		do_backup = 0;
		break;
	    case opt_noclobber:
		do_excl = 1;
		do_atomic = 0;
		do_backup = 0;
		break;
	/* signal options: */
	    case 'S':
		signal = readint(optarg, argv[0]);
		if ((signal <= 0) || (signal >= _NSIG)) {
		    die(EINVAL,
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
	/* fsync options: */
	    case opt_nosync: /* --nosync */
		do_sync = 0;
		break;
	    case opt_sync: /* --sync */
		do_sync = 1;
		break;
	/* vmadump options: */
	    case opt_save_exe:
	        cr_flags |= CR_CHKPT_DUMP_EXEC;
	        break;
	    case opt_save_private:
	        cr_flags |= CR_CHKPT_DUMP_PRIVATE;
	        break;
	    case opt_save_shared:
	        cr_flags |= CR_CHKPT_DUMP_SHARED;
	        break;
	    case opt_save_all:
	        cr_flags |= CR_CHKPT_DUMP_ALL;
	        break;
	    case opt_save_none:
	        cr_flags &= ~CR_CHKPT_DUMP_ALL;
	        break;
	/* ptraced options: */
#define PTRACED_MASK (CR_CHKPT_PTRACED_ALLOW | CR_CHKPT_PTRACED_SKIP)
	    case opt_ptraced_allow:
	        cr_flags &= ~PTRACED_MASK;
	        cr_flags |= CR_CHKPT_PTRACED_ALLOW;
	        break;
	    case opt_ptraced_skip:
	        cr_flags &= ~PTRACED_MASK;
	        cr_flags |= CR_CHKPT_PTRACED_SKIP;
	        break;
	    case opt_ptraced_error:
	        cr_flags &= ~PTRACED_MASK;
	        break;
	/* ptracer options: */
	    case opt_ptracer_skip:
	        cr_flags |= CR_CHKPT_PTRACER_SKIP;
	        break;
	    case opt_ptracer_error:
	        cr_flags &= ~CR_CHKPT_PTRACER_SKIP;
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
	/* misc options: */
	    case 't':
		secs = readint(optarg, argv[0]);
		if (secs < 0) {
		    die(EINVAL, "Time limit must be non-negative.\n");
		}
		break;
	    case 'q':
		verbose = -1;
		break;
	    case 'v':
		++verbose;
		break;
	    case opt_version:
		print_version();
		exit(0);
		break;
	    case '?':
		if (optopt != '?') { /* unrecognized option case */
		    usage(stderr, -1);
		}
		/* fall through to usge on stdout... */
	    case 'h':
		usage(stdout, 0);
	    default: /* reachable */
		usage(stderr, -1);
	}
    }

    /* Grab id if exactly one arg left.
       optind is a magic global set by getopt_long, and is index of first 
       non-flag parameter, if any */
    if (argc - optind == 1) {
	target = readint(argv[optind], argv[0]);
    } else {
	usage(stderr, -1);
    }

    if (dest_type == dest_fd) {
	#define NAMELEN 64 
	char buf[NAMELEN];
	snprintf(buf, NAMELEN, "<fd%d>", chkpt_fd);
	chkpt_to = strdup(buf);
	if (!chkpt_to)
	    die(errno, "Could not duplicate string '%s'\n", buf);
    } else {
	char buf[NAMELEN];

	switch (dest_type) {
	    case dest_dir:
		chkpt_to = chkpt_dir;
		break;

	    case dest_default: /* default -> cwd */
	    case dest_cwd:
		snprintf(buf, NAMELEN, "context.%d", target);
		chkpt_file = strdup(buf);
		if (!chkpt_file)
		    die(errno, "Could not duplicate string '%s'\n", buf);
		/* fall through */
	    case dest_file:
		chkpt_to = chkpt_file;
		break;

	    default:
		die(1, "Invalid dest_type\n");
	}

	/* get parent directory name */
	parent_dir = strdup(chkpt_to);
	if (!parent_dir)
	    die(errno, "Error in strdup: %s", strerror(errno));
	parent_dir = dirname(parent_dir);

	if (do_atomic) {
	    char *base = strdup(chkpt_to);
	    if (!base)
		die(errno, "Error in strdup: %s\n", strerror(errno));
	    /* save final target name */
	    rename_to = chkpt_to;  
	    /* use '.context.pid.tmp'-style name for checkpoint file */
	    chkpt_to = (char *)malloc(strlen(chkpt_to) + 10);
	    if (!chkpt_to)
		die(errno, "Malloc failed!\n");
	    strcpy(chkpt_to, parent_dir);
	    strcat(chkpt_to, "/.");
	    strcat(chkpt_to, basename(base));
	    strcat(chkpt_to, ".tmp");
	    free(base);
	}
    }

    if (verbose > 0)  {
	printf("targetfile='%s', parent dir='%s', rename=%s\n", 
		chkpt_to, parent_dir, rename_to);
    }

    /* TODO:  make sure no other checkpoint is occurring to the same file? */
    if (chkpt_fd >= 0) {
	/* silently ignore the atomic/backup flags */
	do_excl = do_atomic = do_backup = 0;
    } else if (chkpt_file) {
	if (!do_backup && !do_atomic && !do_excl) { /* clobber */
	    err = access(chkpt_to, W_OK);
	    if (err && (errno != ENOENT)) { /* exists, but can't overwrite */
		err = unlink(chkpt_to);
		if (err) { /* couldn't remove it either */
		    die(errno, "Unable to remove existing '%s': %s\n",
			chkpt_to, strerror(errno));
		}
	    }
	}
	if ((chkpt_fd = openfile(chkpt_to, do_excl)) == -1)
	    die(errno, "Failed to open checkpoint file '%s'\n", chkpt_file);
	/* after this point, remove checkpoint file if there's an error */
	to_remove = chkpt_to;
    } else {
	die(EINVAL, "directories not yet supported\n");
    }

    cr_initialize_checkpoint_args_t(&cr_args);
    cr_args.cr_scope  = target_type;
    cr_args.cr_target = target;
    cr_args.cr_fd     = chkpt_fd;
    cr_args.cr_signal = signal;
    cr_args.cr_timeout = secs;	/* 0 == unbounded */
    cr_args.cr_flags  = cr_flags;

    /* Record our pid */
    mypid = getpid();

    /* CONNECT TO THE KERNEL */
    client_id = cr_init();
    if (client_id < 0) {
      if (errno == ENOSYS) {
          die(errno, "Checkpoint failed: support missing from kernel\n");
      } else {
          die(errno, "Failed cr_init(): %s\n", cr_strerror(errno));
      }
    }

    /* Register our callback */
    cb_id = cr_register_callback(&my_cb, NULL, CR_THREAD_CONTEXT);
    if (cb_id < 0) {
      die(errno, "Failed cr_register_callback(): %s\n", cr_strerror(errno));
    }

    /* Begin our critical section */
    pthread_mutex_lock(&lock);

    /* issue the request */
    err = cr_request_checkpoint(&cr_args, &cr_handle);
    if (err < 0) {
	if (errno == CR_ENOSUPPORT) {
	    die(errno, "Checkpoint failed: support missing from application\n");
	} else {
	    die(errno, "cr_request_checkpoint: %s\n", cr_strerror(errno));
	}
    } else if (verbose > 0) {
	fprintf(stderr, "checkpoint request issued\n");
    }

    /* wait for the checkpoint to complete */
    if (verbose > 0) {
	fprintf(stderr, "waiting for checkpoint request to complete\n");
    }
    do {
        /* This loop is necessary in case cr_checkpoint itself was checkpointed (causes EINTR). */
        err = cr_wait_checkpoint(&cr_handle, NULL);
        if (err == 0) {
	    /* 0 would mean timeout, but we passed NULL for the (struct timeval *) */
	    die(1, "cr_wait_checkpoint returned unexpected 0");
	}
    } while ((err < 0) && (errno == EINTR));
    if (err < 0) {
	die(errno, "cr_wait_checkpoint failed: %s\n", cr_strerror(errno));
    }

    if (kmsg_level != kmsg_none) {
	if (verbose > 0) {
	    fprintf(stderr, "collecting any kernel log messages\n");
	}
	/* NOTE: we ignore any failure to malloc (EFAULT) or to collect the log */
        err = cr_log_checkpoint(&cr_handle, 0, NULL);
	if (err > 0) {
	    int len = err;
	    kmsgs = malloc(len);
	    (void)cr_log_checkpoint(&cr_handle, len, kmsgs);
        }
    }

    if (verbose > 0) {
	fprintf(stderr, "reaping checkpoint request\n");
    }
    err = cr_reap_checkpoint(&cr_handle);
    if (err < 0) {
	if (errno == CR_ERESTARTED) {
	    /* restarting -- not an error */
	    /* The pthread mutex actually ensures we won't ever restart here.
	     * However, if modelling your own code after cr_checkpoint.c, then
	     * you'll probably want to recognize this case as not an error.
	     */
	    if (verbose > 0) {
		fprintf(stderr, "restarted from completed checkpoint request\n");
	    }
	} else if (errno == CR_ETEMPFAIL) {
	    die(errno, "Checkpoint cancelled by application: try again later\n");
	} else if (errno == ESRCH) {
	    die(errno, "Checkpoint failed: no processes checkpointed\n");
	} else if (errno == CR_EPERMFAIL) {
	    die(errno, "Checkpoint cancelled by application: unable to checkpoint\n");
	} else if (errno == CR_ENOSUPPORT) {
	    die(errno, "Checkpoint failed: support missing from application\n");
	} else {
	    die(errno, "Checkpoint failed: %s\n", cr_strerror(errno));
	}
    } else if (verbose > 0) {
	fprintf(stderr, "checkpoint request completed\n");
    }

    if (do_backup) {
	struct stat s;

	assert(do_atomic);
	/* only backup if there's a pre-existing checkpoint */
	if (!stat(rename_to, &s)) {
	    if (!backup_to) {
		int bnum = 1;
		int len = strlen(rename_to) + 10;
		backup_to = (char *)malloc(len);
		while (1) {
		    snprintf(backup_to, len, "%s.~%d~", rename_to, bnum++);
		    if (stat(backup_to, &s)) 
			break;
		    if (bnum > (1<<16))
			die(-1, "This is absurd\n");
		}
	    }
	    if (rename(rename_to, backup_to))
		die(errno, "Unable to rename '%s' to '%s': %s\n",
		    rename_to, backup_to, strerror(errno));
	}
    }
    if (do_atomic) {
	assert(rename_to && strlen(rename_to));
	if (rename(chkpt_to, rename_to))
	    die(errno, "Unable to rename '%s' to '%s': %s\n",
		chkpt_to, rename_to, strerror(errno));
    }

    /* End our critical section */
    pthread_mutex_unlock(&lock);

    /* Show kernel warnings if any. */
    if ((verbose >= 0) && (kmsg_level == kmsg_warn)) {
      show_kmsgs();
    }

    if (do_sync) {
	DIR *dir;

	/* COMMIT TO DISK */
	err = fsync(chkpt_fd);
	if ((err < 0) && (errno != EINVAL)) { // EINVAL for non-syncable fd
	    die(errno, "Error syncing checkpoint to disk: %s\n", 
		strerror(errno));
	}
	if (parent_dir != NULL) {
	    /* sync parent directory, too, to ensure checkpoint shows up */
	    if (!(dir = opendir(parent_dir)))
		die(errno, "unable to opendir(%s): %s\n",
		    parent_dir, strerror(errno));
	    /* ignore fsync errors that might be from using NFS */
	    if (fsync(dirfd(dir)) && errno != EROFS && errno != EINVAL && (verbose > 0)) {
		fprintf(stderr, "Warning: unable to sync directory '%s': errno=%d\n",
			parent_dir, errno);
	    }
	    /* TODO: if checkpoint to directory, fsync all context.pid files */
	}
    }

    return 0;
}

/* Just fork a child to do the real work.
 * This allows us to exclude the child from checkpoints while still
 * including the parent so that there is something to wait() for.
 */
int main(int argc, char **argv)
{
    int pid, status, err;

    pid = fork();
    if (!pid) {
	/* In child */
	return real_main(argc, argv);
    } else if (pid < 0) {
	die(1, "fork() failed: %s", strerror(errno));
	/* NOT REACHED */
    }

    do {
        err = wait(&status);
    } while ((err < 0) && (errno == EINTR));
    if (err < 0) {
	if (errno == ECHILD) {
	    /* No child means this is a restart. */
	    err = 0;
	} else {
	    die(1, "wait() failed: %s", strerror(errno));
	    /* NOT REACHED */
	}
    } else if (WIFSIGNALED(status)) {
	int signo = WTERMSIG(status);
	die(1, "child killed by signal %d (%s)", signo, strsignal(signo));
	/* NOT REACHED */
    } else {
	err = WEXITSTATUS(status);
    }

    return err;
}
