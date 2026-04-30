/************************************************************************
 * NASA Docket No. GSC-19,200-1, and identified as "cFS Draco"
 *
 * Copyright (c) 2023 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

/**
 * @file
 *   Define TO Lab Event messages
 */
#ifndef TO_LAB_EVENTIDS_H
#define TO_LAB_EVENTIDS_H

/*****************************************************************************/

/* Event message ID's */
#define TO_LAB_EVM_RESERVED 0

#define TO_LAB_INIT_INF_EID          1
#define TO_LAB_CR_PIPE_ERR_EID       2
#define TO_LAB_TLMOUTENA_INF_EID     3
#define TO_LAB_SUBSCRIBE_ERR_EID     4
#define TO_LAB_TLMPIPE_ERR_EID       5
#define TO_LAB_TLMOUTSOCKET_ERR_EID  6
#define TO_LAB_TLMOUTSTOP_ERR_EID    7
#define TO_LAB_MID_ERR_EID           8
#define TO_LAB_FNCODE_ERR_EID        9
#define TO_LAB_ADDPKT_ERR_EID        10
#define TO_LAB_REMOVEPKT_ERR_EID     11
#define TO_LAB_RESET_INF_EID         12
#define TO_LAB_ADDPKT_INF_EID        13
#define TO_LAB_REMOVEPKT_INF_EID     14
#define TO_LAB_REMOVEALLPKTS_INF_EID 15
#define TO_LAB_NOOP_INF_EID          16
#define TO_LAB_TBL_ERR_EID           17
#define TO_LAB_ENCODE_ERR_EID        18
#define TO_LAB_SUBSCRIBE_INF_EID     19
#define TO_LAB_TBL_MANAGE_ERR_EID    20
#define TO_LAB_UNSUBSCRIBE_ERR_EID   21
#define TO_LAB_TBL_PTR_NULL_ERR_EID  22
#define TO_LAB_TBL_BUF_LIMIT_ERR_EID 23
#define TO_LAB_ADDPKT_BUFLIM_ERR_EID 24
#define TO_LAB_ADDPKT_EXISTS_ERR_EID 25
#define TO_LAB_RMPKT_MISSING_ERR_EID 26
#define TO_LAB_SUBSCRIBE_DBG_EID     27

/******************************************************************************/

#endif
