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
 * $Id: cr_ftb.c,v 1.3 2008/12/12 05:13:25 phargrov Exp $
 *
 * FTB interface code common to blrc's utils
 */

#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "libftb.h"

/* Single client using static variables */

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static FTB_client_t cr_ftb_client_info = {
    event_space: "FTB.checkpoint_sw.blcr",
    client_name: "",
    client_jobid: "",
    client_subscription_style: "FTB_SUBSCRIPTION_NONE"
};

static FTB_event_info_t cr_ftb_event_info[] = {
  {"CHKPT_BEGIN", "INFO"},  /* No payload */
  {"CHKPT_END",   "INFO"},  /* payload = event_handle */
  {"CHKPT_ERROR", "ERROR"}, /* payload = event_handle + uint32_t errno */
  {"RSTRT_BEGIN", "INFO"},  /* No payload */
  {"RSTRT_END",   "INFO"},  /* payload = event_handle */
  {"RSTRT_ERROR", "ERROR"}  /* payload = event_handle + uint32_t errno */
};
#define CR_FTB_EVENT_COUNT (sizeof(cr_ftb_event_info) / sizeof(cr_ftb_event_info[0]))

static FTB_client_handle_t cr_ftb_client_handle;

static int cri_ftb_init_count = 0;

int cri_ftb_init(const char *client_name, const char *client_jobid) {
    int rc = FTB_SUCCESS;

    pthread_mutex_lock(&lock);

    if (cri_ftb_init_count++ != 0) goto out;
    
    if (client_name) {
        strncpy(cr_ftb_client_info.client_name, client_name, FTB_MAX_CLIENT_NAME);
    }
    if (client_jobid) {
        strncpy(cr_ftb_client_info.client_jobid, client_jobid, FTB_MAX_CLIENT_JOBID);
    }
    
    rc = FTB_Connect(&cr_ftb_client_info, &cr_ftb_client_handle);
    if (rc != FTB_SUCCESS) goto out;

    rc = FTB_Declare_publishable_events(cr_ftb_client_handle, 0, cr_ftb_event_info, CR_FTB_EVENT_COUNT);
    if (rc == FTB_ERR_DUP_EVENT) { rc = 0; /* Ignore, see CiFTS trac item #44 */ }
    else if (rc != FTB_SUCCESS) goto out;

out:
    pthread_mutex_unlock(&lock);
    return rc;
}

int cri_ftb_fini(void) {
    int rc = FTB_SUCCESS;

    pthread_mutex_lock(&lock);

    if (!cri_ftb_init_count || --cri_ftb_init_count) goto out;

    rc = FTB_Disconnect(cr_ftb_client_handle);

out:
    pthread_mutex_unlock(&lock);
    return rc;
}

int cri_ftb_event(FTB_event_handle_t *event_handle, const char *event_name, int len, const void *data) {
    FTB_event_properties_t prop;
    FTB_event_handle_t tmp;
    
    if (!cri_ftb_init_count) return -1;

    if (len) {
        prop.event_type = 1;
	memset(prop.event_payload, 0, FTB_MAX_PAYLOAD_DATA);
        memcpy(prop.event_payload, data, len);
    }
    return FTB_Publish(cr_ftb_client_handle, event_name,
                       len ? &prop : NULL,
                       event_handle ? event_handle : &tmp);
}

int cri_ftb_event2(FTB_event_handle_t *event_handle, const FTB_event_handle_t *orig_handle, const char *event_name, int len, const void *data) {
    FTB_event_properties_t prop;
    FTB_event_handle_t tmp;

    if (!cri_ftb_init_count) return -1;

    prop.event_type = 2;
    memset(prop.event_payload, 0, FTB_MAX_PAYLOAD_DATA);
    memcpy(prop.event_payload, orig_handle, sizeof(FTB_event_handle_t));
    if (len) {
        memcpy(prop.event_payload + sizeof(FTB_event_handle_t), data, len);
    }
    return FTB_Publish(cr_ftb_client_handle, event_name, &prop,
                       event_handle ? event_handle : &tmp);
}

