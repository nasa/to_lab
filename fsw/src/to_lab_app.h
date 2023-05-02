/************************************************************************
 * NASA Docket No. GSC-18,719-1, and identified as “core Flight System: Bootes”
 *
 * Copyright (c) 2020 United States Government as represented by the
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

#include "cfe.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "common_types.h"
#include "osapi.h"

#include "to_lab_msg.h"

/************************************************************************
 * Macro Definitions
 ************************************************************************/

#define TO_LAB_TASK_MSEC 500 /* run at 2 Hz */
#define TO_LAB_UNUSED    CFE_SB_MSGID_RESERVED

/**
 * Depth of pipe for commands to the TO_LAB application itself
 */
#define TO_LAB_CMD_PIPE_DEPTH 8

/**
 * Depth of pipe for telemetry forwarded through the TO_LAB application
 */
#define TO_LAB_TLM_PIPE_DEPTH OS_QUEUE_MAX_DEPTH

#define cfgTLM_ADDR        "192.168.1.81"
#define cfgTLM_PORT        1235
#define TO_LAB_VERSION_NUM "5.1.0"

/************************************************************************
** Type Definitions
*************************************************************************/

/**
 * CI global data structure
 */
typedef struct
{
    CFE_SB_PipeId_t Tlm_pipe;
    CFE_SB_PipeId_t Cmd_pipe;
    osal_id_t       TLMsockid;
    bool            downlink_on;
    char            tlm_dest_IP[17];
    bool            suppress_sendto;

    TO_LAB_HkTlm_t        HkTlm;
    TO_LAB_DataTypesTlm_t DataTypesTlm;
} TO_LAB_GlobalData_t;

/************************************************************************
 * Function Prototypes
 ************************************************************************/

void  TO_LAB_AppMain(void);
void  TO_LAB_openTLM(void);
int32 TO_LAB_init(void);
void  TO_LAB_exec_local_command(CFE_SB_Buffer_t *SBBufPtr);
void  TO_LAB_process_commands(void);
void  TO_LAB_forward_telemetry(void);

/*
 * Individual Command Handler prototypes
 */
int32 TO_LAB_AddPacket(const TO_LAB_AddPacketCmd_t *data);
int32 TO_LAB_Noop(const TO_LAB_NoopCmd_t *data);
int32 TO_LAB_EnableOutput(const TO_LAB_EnableOutputCmd_t *data);
int32 TO_LAB_RemoveAll(const TO_LAB_RemoveAllCmd_t *data);
int32 TO_LAB_RemovePacket(const TO_LAB_RemovePacketCmd_t *data);
int32 TO_LAB_ResetCounters(const TO_LAB_ResetCountersCmd_t *data);
int32 TO_LAB_SendDataTypes(const TO_LAB_SendDataTypesCmd_t *data);
int32 TO_LAB_SendHousekeeping(const CFE_MSG_CommandHeader_t *data);

#endif
