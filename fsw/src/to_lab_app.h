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
 *   Define TO Lab Application header file
 */

#ifndef TO_LAB_APP_H
#define TO_LAB_APP_H

#include "common_types.h"
#include "osapi.h"
#include "cfe.h"

#include "to_lab_mission_cfg.h"
#include "to_lab_platform_cfg.h"
#include "to_lab_cmds.h"
#include "to_lab_dispatch.h"
#include "to_lab_msg.h"
#include "to_lab_tbl.h"

/************************************************************************
** Type Definitions
*************************************************************************/

/*
** Status Codes are 32 bit values (format defined in cfe_error.h).
** A snippet of the format is shown below, for reference:
**
**  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
**  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
** +---+---+-----+-----------------+-------------------------------+
** |Sev| R | Srv | Mission Defined |               Code            |
** +---+---+-----+-----------------+-------------------------------+
**
** As indicated above:
** - bits 24 down to 16 indicate the "mission defined" group
** - bits 15 down to 0 indicate the status codes within that group
**
** Instead of defining a TO_LAB-specific success value, CFE_SUCCESS is used.
** If an "informational" severity code is needed, it can be added below.
*/
#define TO_LAB_STATUS_IDENTIFIER              ((CFE_Status_t)0x00010000)
#define TO_LAB_ERROR_CODES                    ((CFE_Status_t)CFE_SEVERITY_ERROR | TO_LAB_STATUS_IDENTIFIER)
#define TO_LAB_ERROR_MAX_PACKET_LIMIT_REACHED ((CFE_Status_t)TO_LAB_ERROR_CODES | 0x0001)
#define TO_LAB_ERROR_ADD_PACKET_BUF_LIMIT_ERR ((CFE_Status_t)TO_LAB_ERROR_CODES | 0x0002)
#define TO_LAB_ERROR_ADD_PACKET_REDUNDANT_ERR ((CFE_Status_t)TO_LAB_ERROR_CODES | 0x0003)
#define TO_LAB_ERROR_RM_PACKET_MISSING_ERR    ((CFE_Status_t)TO_LAB_ERROR_CODES | 0x0004)

/**
 * TO global data structure
 */
typedef struct
{
    CFE_SB_PipeId_t Tlm_pipe;
    CFE_SB_PipeId_t Cmd_pipe;
    osal_id_t       TLMsockid;
    bool            downlink_on;
    char            tlm_dest_IP[17];
    bool            suppress_sendto;
    bool            AllowPassthru;

    TO_LAB_HkTlm_t        HkTlm;
    TO_LAB_DataTypesTlm_t DataTypesTlm;

    TO_LAB_Subs_t   *SubsTblPtr;
    CFE_TBL_Handle_t SubsTblHandle;
    CFE_SB_MsgId_t   ActiveSubs[TO_LAB_MISSION_MAX_SUBSCRIPTIONS];
    uint16           ActiveSubCount;

} TO_LAB_GlobalData_t;

/************************************************************************
 * Function Prototypes
 ************************************************************************/

void         TO_LAB_AppMain(void);
void         TO_LAB_openTLM(void);
int32        TO_LAB_init(void);
void         TO_LAB_process_commands(void);
void         TO_LAB_forward_telemetry(void);
CFE_Status_t TO_LAB_ValidateSubTable(void *TblPtr);
uint16       TO_LAB_UnsubscribeFromTlmPipe(void);
uint16       TO_LAB_UpdateSubscriptionsFromTable(void);
void         TO_LAB_ManageTables(void);

/******************************************************************************/

/* Global State Object */
extern TO_LAB_GlobalData_t TO_LAB_Global;

#endif
