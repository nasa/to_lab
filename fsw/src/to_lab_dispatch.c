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
 *  This file contains the source code for the TO lab application
 */

#include "cfe.h"

#include "to_lab_app.h"
#include "to_lab_dispatch.h"
#include "to_lab_cmds.h"
#include "to_lab_msg.h"
#include "to_lab_eventids.h"
#include "to_lab_msgids.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/*  TO_LAB_ProcessGroundCommand() -- Process local message           */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void TO_LAB_ProcessGroundCommand(const CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_MSG_FcnCode_t FcnCode = 0;

    CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &FcnCode);

    switch (FcnCode)
    {
        case TO_LAB_NOOP_CC:
            TO_LAB_NoopCmd((const TO_LAB_NoopCmd_t *)SBBufPtr);
            break;

        case TO_LAB_RESET_STATUS_CC:
            TO_LAB_ResetCountersCmd((const TO_LAB_ResetCountersCmd_t *)SBBufPtr);
            break;

        case TO_LAB_SEND_DATA_TYPES_CC:
            TO_LAB_SendDataTypesCmd((const TO_LAB_SendDataTypesCmd_t *)SBBufPtr);
            break;

        case TO_LAB_ADD_PKT_CC:
            TO_LAB_AddPacketCmd((const TO_LAB_AddPacketCmd_t *)SBBufPtr);
            break;

        case TO_LAB_REMOVE_PKT_CC:
            TO_LAB_RemovePacketCmd((const TO_LAB_RemovePacketCmd_t *)SBBufPtr);
            break;

        case TO_LAB_REMOVE_ALL_PKT_CC:
            TO_LAB_RemoveAllCmd((const TO_LAB_RemoveAllCmd_t *)SBBufPtr);
            break;

        case TO_LAB_OUTPUT_ENABLE_CC:
            TO_LAB_EnableOutputCmd((const TO_LAB_EnableOutputCmd_t *)SBBufPtr);
            break;

        default:
            CFE_EVS_SendEvent(TO_LAB_FNCODE_ERR_EID,
                              CFE_EVS_EventType_ERROR,
                              "L%d TO: Invalid Function Code Rcvd In Ground Command 0x%x",
                              __LINE__,
                              (unsigned int)FcnCode);
            ++TO_LAB_Global.HkTlm.Payload.CommandErrorCounter;
            break;
    }
}

void TO_LAB_TaskPipe(const CFE_SB_Buffer_t *SBBufPtr)
{
    static CFE_SB_MsgId_t CMD_MID     = CFE_SB_MSGID_RESERVED;
    static CFE_SB_MsgId_t SEND_HK_MID = CFE_SB_MSGID_RESERVED;

    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;

    /* cache the local MID Values here, this avoids repeat lookups */
    if (!CFE_SB_IsValidMsgId(CMD_MID))
    {
        CMD_MID     = CFE_SB_ValueToMsgId(TO_LAB_CMD_MID);
        SEND_HK_MID = CFE_SB_ValueToMsgId(TO_LAB_SEND_HK_MID);
    }

    CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);

    /* Process all SB messages */
    if (CFE_SB_MsgId_Equal(MsgId, SEND_HK_MID))
    {
        /* Housekeeping request */
        TO_LAB_SendHkCmd((const TO_LAB_SendHkCmd_t *)SBBufPtr);
    }
    else if (CFE_SB_MsgId_Equal(MsgId, CMD_MID))
    {
        /* Ground command */
        TO_LAB_ProcessGroundCommand(SBBufPtr);
    }
    else
    {
        /* Unknown command */
        CFE_EVS_SendEvent(TO_LAB_MID_ERR_EID,
                          CFE_EVS_EventType_ERROR,
                          "L%d TO: Invalid Msg ID Rcvd 0x%x",
                          __LINE__,
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId));
    }
}
