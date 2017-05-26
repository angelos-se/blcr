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
 * $Id: cr_proc.c,v 1.19.12.1 2014/09/18 23:46:07 phargrov Exp $
 */

#include "cr_module.h"

#include <linux/proc_fs.h>
#include <linux/init.h>


#if HAVE_PROC_ROOT
  #define cr_proc_root (&proc_root)
#else
  #define cr_proc_root NULL
#endif

/*
 * Our proc_fs entries
 */
static struct proc_dir_entry *proc_checkpoint;
static struct proc_dir_entry *proc_ctrl;

/**
 * cr_proc_init - Initialization code
 *
 * DESCRIPTION:
 * This function registers /proc/checkpoint and /proc/checkpoint/ctrl
 */
int __init cr_proc_init(void)
{
	CR_KTRACE_FUNC_ENTRY();

#if HAVE_PROC_MKDIR
	proc_checkpoint = proc_mkdir("checkpoint", cr_proc_root);
#else
	proc_checkpoint = create_proc_entry("checkpoint", S_IFDIR, cr_proc_root);
#endif
	if (proc_checkpoint == NULL) {
		CR_ERR("proc_create_entry(/proc/checkpoint/) failed");
		return -ENOMEM;
	}

#if HAVE_PROC_CREATE
	proc_ctrl = proc_create("ctrl", S_IFREG | S_IRUGO | S_IWUGO,
				proc_checkpoint, &cr_ctrl_fops);
#else
	proc_ctrl = create_proc_entry("ctrl", S_IFREG | S_IRUGO | S_IWUGO,
				      proc_checkpoint);
	if (proc_ctrl) {
		proc_ctrl->proc_fops = &cr_ctrl_fops;
	}
#endif
	if (proc_ctrl == NULL) {
		CR_ERR("proc_create_entry(/proc/checkpoint/ctrl) failed");
		return -ENOMEM;
	}

	return 0;
}

/**
 * cr_proc_cleanup - Initialization code
 *
 * DESCRIPTION:
 * This function removes /proc/checkpoint and /proc/checkpoint/ctrl
 */
void cr_proc_cleanup(void)
{
	CR_KTRACE_FUNC_ENTRY();

#if HAVE_PROC_REMOVE
	proc_remove(proc_ctrl);
	proc_remove(proc_checkpoint);
#else
	remove_proc_entry("ctrl", proc_checkpoint);
	remove_proc_entry("checkpoint", cr_proc_root);
#endif
}
