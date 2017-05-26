/* 
 * Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
 * 2003, The Regents of the University of California, through Lawrence
 * Berkeley National Laboratory (subject to receipt of any required
 * approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * Portions may be copyrighted by others, as may be noted in specific
 * copyright notices within specific files.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: cr_strerror.c,v 1.4 2004/12/09 17:19:27 phargrov Exp $
 */

#include <string.h>
#include "cr_private.h"

/* Create array of error descriptions */
#define CR_ERROR_DEF(name, desc)     desc,
static const char * cr_strerrors[] = {
#include <blcr_errcodes.h>
    "for fussy compilers that don't want to see a comma at the end"
};


const char *
cr_strerror(int errnum)
{
    if (errnum <= CR_MIN_ERRCODE || errnum >= CR_MAX_ERRCODE)
	return strerror(errnum);
    else
	return cr_strerrors[errnum - (CR_MIN_ERRCODE+1)];
}
