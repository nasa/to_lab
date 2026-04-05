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
 * \file
 *  Define TO Lab CPU specific subscription table
 */

#include "cfe_tbl_filedef.h" /* Required to obtain the CFE_TBL_FILEDEF macro definition */
#include "cfe_sb_api_typedefs.h"
#include "to_lab_tbl.h"
#include "cfe_msgids.h"

/*
** Add the proper include file for the message IDs below
*/
#include "to_lab_msgids.h"

/*
** Cross-app MsgId values (numeric) to avoid cross-app include path issues in table builds.
** CMD base = 0x1800, TLM base = 0x0800, OR'd with topic ID from each app's topicids.h
**
** CI_LAB:     HK_TLM topic=0x84 → TLM MID=0x0884
** SAMPLE_APP: HK_TLM topic=0x83 → TLM MID=0x0883
*/
#define NUMERIC_CI_LAB_HK_TLM_MID      0x0884
#define NUMERIC_SAMPLE_APP_HK_TLM_MID  0x0883

TO_LAB_Subs_t Subscriptions = {
    .Subs = {
        /* TO Lab own HK */
        {CFE_SB_MSGID_WRAP_VALUE(TO_LAB_HK_TLM_MID), {0, 0}, 4},
        {CFE_SB_MSGID_WRAP_VALUE(TO_LAB_DATA_TYPES_MID), {0, 0}, 4},

        /* CI Lab HK */
        {CFE_SB_MSGID_WRAP_VALUE(NUMERIC_CI_LAB_HK_TLM_MID), {0, 0}, 4},

        /* Sample App HK */
        {CFE_SB_MSGID_WRAP_VALUE(NUMERIC_SAMPLE_APP_HK_TLM_MID), {0, 0}, 4},

        /* cFE Core HK telemetry */
        {CFE_SB_MSGID_WRAP_VALUE(CFE_ES_HK_TLM_MID), {0, 0}, 4},
        {CFE_SB_MSGID_WRAP_VALUE(CFE_EVS_HK_TLM_MID), {0, 0}, 4},
        {CFE_SB_MSGID_WRAP_VALUE(CFE_SB_HK_TLM_MID), {0, 0}, 4},
        {CFE_SB_MSGID_WRAP_VALUE(CFE_TBL_HK_TLM_MID), {0, 0}, 4},
        {CFE_SB_MSGID_WRAP_VALUE(CFE_TIME_HK_TLM_MID), {0, 0}, 4},

        /* cFE Event messages - this is what you want for visibility */
        {CFE_SB_MSGID_WRAP_VALUE(CFE_EVS_LONG_EVENT_MSG_MID), {0, 0}, 32},

        /* CFE_SB_MSGID_RESERVED entry to mark the end of valid MsgIds */
        {CFE_SB_MSGID_RESERVED, {0, 0}, 0}
    }
};

CFE_TBL_FILEDEF(Subscriptions, TO_LAB.Subscriptions, TO Lab Sub Tbl, to_lab_sub.tbl)
