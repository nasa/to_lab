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
#include "cfe_config.h"

#include "to_lab_app.h"
#include "to_lab_encode.h"
#include "to_lab_eventids.h"
#include "to_lab_msgids.h"
#include "to_lab_perfids.h"
#include "to_lab_version.h"
#include "to_lab_msg.h"
#include "to_lab_tbl.h"

/*
** TO Global Data Section
*/
TO_LAB_GlobalData_t TO_LAB_Global;

/*
** TO Local Function Prototypes
*/
CFE_Status_t TO_LAB_CmdSubscribe(CFE_SB_MsgId_Atom_t MsgIdValue);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                   */
/* TO_LAB_AppMain() -- Application entry point and main process loop */
/*                                                                   */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void TO_LAB_AppMain(void)
{
    uint32       RunStatus = CFE_ES_RunStatus_APP_RUN;
    CFE_Status_t status;

    CFE_ES_PerfLogEntry(TO_LAB_MAIN_TASK_PERF_ID);

    status = TO_LAB_init();

    if (status != CFE_SUCCESS)
    {
        /*
        ** Set request to terminate main loop...
        */
        RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }
    else
    {
        /* Defer subscribing until the system is operational will avoid
         * possibly seeing MsgLimit errors due to the apps sending many
         * events at start up */
        CFE_ES_WaitForStartupSync(TO_LAB_STARTUP_SYNC_TIMEOUT);
        TO_LAB_UpdateSubscriptionsFromTable();
    }

    /*
    ** TO RunLoop
    */
    while (CFE_ES_RunLoop(&RunStatus) == true)
    {
        CFE_ES_PerfLogExit(TO_LAB_MAIN_TASK_PERF_ID);

        /* NOTE: activity in this function is indicated by TO_LAB_SOCKET_SEND_PERF_ID,
         * so it does not need to be included within TO_LAB_MAIN_TASK_PERF_ID. */
        TO_LAB_forward_telemetry();

        CFE_ES_PerfLogEntry(TO_LAB_MAIN_TASK_PERF_ID);

        TO_LAB_process_commands();

        TO_LAB_ManageTables();
    }

    CFE_ES_ExitApp(RunStatus);
}

/*
** TO delete callback function.
** This function will be called in the event that the TO app is killed.
** It will close the network socket for TO
*/
void TO_LAB_delete_callback(void)
{
    OS_printf("TO delete callback -- Closing TO Network socket.\n");
    if (TO_LAB_Global.downlink_on)
    {
        OS_close(TO_LAB_Global.TLMsockid);
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_LAB_init() -- TO initialization                              */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
CFE_Status_t TO_LAB_init(void)
{
    CFE_Status_t status;
    void        *TblPtr;
    char         VersionString[TO_LAB_CFG_MAX_VERSION_STR_LEN];

    /* Zero out the global data structure */
    memset(&TO_LAB_Global, 0, sizeof(TO_LAB_Global));

    TO_LAB_Global.AllowPassthru = true;
    TO_LAB_Global.downlink_on   = false;

    /*
    ** Register with EVS
    */
    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("TO_LAB: Error registering for Event Services, RC = 0x%08X\n", (unsigned int)status);
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Initialize housekeeping packet (clear user data area)...
        */
        CFE_MSG_Init(CFE_MSG_PTR(TO_LAB_Global.HkTlm.TelemetryHeader),
                     CFE_SB_ValueToMsgId(TO_LAB_HK_TLM_MID),
                     sizeof(TO_LAB_Global.HkTlm));

        status = CFE_TBL_Register(&TO_LAB_Global.SubsTblHandle,
                                  "Subscriptions",
                                  sizeof(TO_LAB_Subs_t),
                                  CFE_TBL_OPT_DEFAULT,
                                  TO_LAB_ValidateSubTable);

        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(TO_LAB_TBL_ERR_EID,
                              CFE_EVS_EventType_ERROR,
                              "L%d TO Can't register table status %i",
                              __LINE__,
                              (int)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        status = CFE_TBL_Load(TO_LAB_Global.SubsTblHandle, CFE_TBL_SRC_FILE, "/cf/to_lab_sub.tbl");

        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(TO_LAB_TBL_ERR_EID,
                              CFE_EVS_EventType_ERROR,
                              "L%d TO Can't load table status %i",
                              __LINE__,
                              (int)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        status = CFE_TBL_GetAddress((void **)&TblPtr, TO_LAB_Global.SubsTblHandle);

        if (status != CFE_SUCCESS && status != CFE_TBL_INFO_UPDATED)
        {
            CFE_EVS_SendEvent(TO_LAB_TBL_ERR_EID,
                              CFE_EVS_EventType_ERROR,
                              "L%d TO Can't get table addr status %i",
                              __LINE__,
                              (int)status);
        }
    }

    if (status == CFE_SUCCESS || status == CFE_TBL_INFO_UPDATED)
    {
        TO_LAB_Global.SubsTblPtr = TblPtr; /* Save returned address */

        /* Subscribe to my commands */
        status = CFE_SB_CreatePipe(&TO_LAB_Global.Cmd_pipe, TO_LAB_PLATFORM_CMD_PIPE_DEPTH, "TO_LAB_CMD_PIPE");
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(TO_LAB_CR_PIPE_ERR_EID,
                              CFE_EVS_EventType_ERROR,
                              "L%d TO Can't create cmd pipe status %i",
                              __LINE__,
                              (int)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        status = TO_LAB_CmdSubscribe(TO_LAB_CMD_MID);
    }

    if (status == CFE_SUCCESS)
    {
        status = TO_LAB_CmdSubscribe(TO_LAB_SEND_HK_MID);
    }

    if (status == CFE_SUCCESS)
    {
        /* Create TO TLM pipe */
        status = CFE_SB_CreatePipe(&TO_LAB_Global.Tlm_pipe, TO_LAB_PLATFORM_TLM_PIPE_DEPTH, "TO_LAB_TLM_PIPE");
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(TO_LAB_TLMPIPE_ERR_EID,
                              CFE_EVS_EventType_ERROR,
                              "L%d TO Can't create Tlm pipe status %i",
                              __LINE__,
                              (int)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        CFE_Config_GetVersionString(VersionString,
                                    TO_LAB_CFG_MAX_VERSION_STR_LEN,
                                    "TO Lab",
                                    TO_LAB_VERSION,
                                    TO_LAB_BUILD_CODENAME,
                                    TO_LAB_LAST_OFFICIAL);

        CFE_EVS_SendEvent(TO_LAB_INIT_INF_EID,
                          CFE_EVS_EventType_INFORMATION,
                          "TO Lab Initialized.%s, Awaiting enable command.",
                          VersionString);
    }

    /*
     ** Install the delete handler
     */
    OS_TaskInstallDeleteHandler(&TO_LAB_delete_callback);

    return status;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_LAB_CmdSubscribe() -- Subscribes to command message          */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
CFE_Status_t TO_LAB_CmdSubscribe(CFE_SB_MsgId_Atom_t MsgIdValue)
{
    CFE_Status_t status;
    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(MsgIdValue), TO_LAB_Global.Cmd_pipe);

    if (status != CFE_SUCCESS)
    {
        (void)CFE_EVS_SendEvent(TO_LAB_SUBSCRIBE_ERR_EID,
                                CFE_EVS_EventType_ERROR,
                                "L%d TO Can't subscribe to stream 0x%x status %i",
                                __LINE__,
                                (unsigned int)MsgIdValue,
                                (int)status);
    }

    return status;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_LAB_process_commands() -- Process command pipe message       */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void TO_LAB_process_commands(void)
{
    CFE_SB_Buffer_t *SBBufPtr;
    CFE_Status_t     Status;

    /* Exit command processing loop if no message received. */
    while (1)
    {
        Status = CFE_SB_ReceiveBuffer(&SBBufPtr, TO_LAB_Global.Cmd_pipe, CFE_SB_POLL);
        if (Status != CFE_SUCCESS)
        {
            break;
        }

        TO_LAB_TaskPipe(SBBufPtr);
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_LAB_openTLM() -- Open TLM                                    */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void TO_LAB_openTLM(void)
{
    int32 status;

    status = OS_SocketOpen(&TO_LAB_Global.TLMsockid, OS_SocketDomain_INET, OS_SocketType_DATAGRAM);
    if (status != OS_SUCCESS)
    {
        CFE_EVS_SendEvent(TO_LAB_TLMOUTSOCKET_ERR_EID,
                          CFE_EVS_EventType_ERROR,
                          "L%d, TO TLM socket error: %d",
                          __LINE__,
                          (int)status);
    }

    /*---------------- Add static arp entries ----------------*/
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_LAB_forward_telemetry() -- Forward telemetry                 */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void TO_LAB_forward_telemetry(void)
{
    OS_SockAddr_t    d_addr;
    int32            OsStatus;
    CFE_Status_t     CfeStatus;
    CFE_SB_Buffer_t *SBBufPtr;
    const void      *NetBufPtr;
    size_t           NetBufSize;
    uint32           PktCount = 0;
    uint16           PortNum  = TO_LAB_MISSION_TLM_PORT + CFE_PSP_GetProcessorId() - 1;

    OS_SocketAddrInit(&d_addr, OS_SocketDomain_INET);
    OS_SocketAddrSetPort(&d_addr, PortNum);
    OS_SocketAddrFromString(&d_addr, TO_LAB_Global.tlm_dest_IP);
    OsStatus = 0;

    do
    {
        CfeStatus = CFE_SB_ReceiveBuffer(&SBBufPtr, TO_LAB_Global.Tlm_pipe, TO_LAB_PLATFORM_TLM_PIPE_TIMEOUT);

        if ((CfeStatus == CFE_SUCCESS) && (TO_LAB_Global.suppress_sendto == false))
        {
            OsStatus = OS_SUCCESS;

            if (TO_LAB_Global.downlink_on == true)
            {
                CFE_ES_PerfLogEntry(TO_LAB_SOCKET_SEND_PERF_ID);

                CfeStatus = TO_LAB_EncodeOutputMessage(SBBufPtr, &NetBufPtr, &NetBufSize);

                if (CfeStatus != CFE_SUCCESS)
                {
                    CFE_EVS_SendEvent(TO_LAB_ENCODE_ERR_EID,
                                      CFE_EVS_EventType_ERROR,
                                      "Error packing output: %d\n",
                                      (int)CfeStatus);
                }
                else
                {
                    OsStatus = OS_SocketSendTo(TO_LAB_Global.TLMsockid, NetBufPtr, NetBufSize, &d_addr);
                }

                CFE_ES_PerfLogExit(TO_LAB_SOCKET_SEND_PERF_ID);
            }

            if (OsStatus < 0)
            {
                CFE_EVS_SendEvent(TO_LAB_TLMOUTSTOP_ERR_EID,
                                  CFE_EVS_EventType_ERROR,
                                  "L%d TO sendto error %d. Tlm output suppressed\n",
                                  __LINE__,
                                  (int)OsStatus);
                TO_LAB_Global.suppress_sendto = true;
            }
        }
        /* If CFE_SB_status != CFE_SUCCESS, then no packet was received from CFE_SB_ReceiveBuffer() */

        PktCount++;
    } while (CfeStatus == CFE_SUCCESS && PktCount < TO_LAB_PLATFORM_MAX_TLM_PKTS);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_LAB_ValidateSubTable() -- Validate subscription table input  */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
CFE_Status_t TO_LAB_ValidateSubTable(void *TblPtr)
{
    TO_LAB_Subs_t *SubsTblPtr;
    TO_LAB_Sub_t  *SubEntry;
    CFE_Status_t   ValidationStatus = CFE_SUCCESS; /* default to success unless error is found */
    uint16         TableIndex;

    if (TblPtr == NULL)
    {
        CFE_EVS_SendEvent(TO_LAB_TBL_PTR_NULL_ERR_EID,
                          CFE_EVS_EventType_ERROR,
                          "L%d TO Lab Table Validation Error: null table pointer input",
                          __LINE__);
        ValidationStatus = CFE_STATUS_VALIDATION_FAILURE;
    }
    else
    {
        /* Loop through table entries and validate each entry */
        SubsTblPtr = (TO_LAB_Subs_t *)TblPtr;
        for (TableIndex = 0; TableIndex < TO_LAB_MISSION_MAX_SUBSCRIPTIONS; TableIndex++)
        {
            SubEntry = &SubsTblPtr->Subs[TableIndex];
            if (!CFE_SB_IsValidMsgId(SubEntry->Stream))
            {
                /* An invalid MsgId indicates the end of the table */
                break;
            }
            else if (SubEntry->BufLimit > TO_LAB_PLATFORM_TLM_PIPE_DEPTH)
            {
                CFE_EVS_SendEvent(TO_LAB_TBL_BUF_LIMIT_ERR_EID,
                                  CFE_EVS_EventType_ERROR,
                                  "L%d TO Lab Table Validation Error: entry %u buf limit %u above max %u",
                                  __LINE__,
                                  TableIndex,
                                  SubEntry->BufLimit,
                                  TO_LAB_PLATFORM_TLM_PIPE_DEPTH);
                ValidationStatus = CFE_STATUS_VALIDATION_FAILURE;
            }
        }
    }

    return ValidationStatus;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_LAB_UnsubscribeFromTlmPipe() -- Unsubscribe from telem pipe  */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
uint16 TO_LAB_UnsubscribeFromTlmPipe(void)
{
    CFE_Status_t    UnsubscribeResult;
    uint16          ActiveSubsIndex;
    CFE_SB_MsgId_t *MsgIdPtr;
    uint16          UnsubscribeCount = 0;

    /* Loop through table entries and attempt to subscribe to each packet */
    for (ActiveSubsIndex = 0; ActiveSubsIndex < TO_LAB_MISSION_MAX_SUBSCRIPTIONS; ActiveSubsIndex++)
    {
        MsgIdPtr = &TO_LAB_Global.ActiveSubs[ActiveSubsIndex];
        if (CFE_SB_IsValidMsgId(*MsgIdPtr))
        {
            UnsubscribeResult = CFE_SB_Unsubscribe(*MsgIdPtr, TO_LAB_Global.Tlm_pipe);
            if (UnsubscribeResult != CFE_SUCCESS)
            {
                CFE_EVS_SendEvent(TO_LAB_UNSUBSCRIBE_ERR_EID,
                                  CFE_EVS_EventType_ERROR,
                                  "L%d TO Lab can't unsubscribe from stream 0x%x status %i",
                                  __LINE__,
                                  (unsigned int)CFE_SB_MsgIdToValue(*MsgIdPtr),
                                  (int)UnsubscribeResult);
            }
            else
            {
                TO_LAB_Global.ActiveSubs[ActiveSubsIndex] = CFE_SB_ValueToMsgId(0);
                ++UnsubscribeCount;
            }
        }
    }

    /*
    ** At this point, there should be no active subscriptions on the tlm pipe from table configs
    */
    TO_LAB_Global.ActiveSubCount = 0;

    return UnsubscribeCount;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_LAB_UpdateSubscriptionsFromTable() -- Update Subscriptions   */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
uint16 TO_LAB_UpdateSubscriptionsFromTable(void)
{
    CFE_Status_t  SubscribeResult;
    uint16        TableIndex;
    TO_LAB_Sub_t *SubEntry;
    uint16        NumberOfSubscriptions = 0;
    bool          PrevEntryInvalid      = false;
    uint8         NumInvalidEntries     = 0;

    /* Clear previous subscriptions */
    (void)TO_LAB_UnsubscribeFromTlmPipe();

    /* Loop through table entries and attempt to subscribe to each packet */
    for (TableIndex = 0; TableIndex < TO_LAB_MISSION_MAX_SUBSCRIPTIONS; TableIndex++)
    {
        SubEntry = &TO_LAB_Global.SubsTblPtr->Subs[TableIndex];
        if (CFE_SB_IsValidMsgId(SubEntry->Stream))
        {
            SubscribeResult =
                CFE_SB_SubscribeEx(SubEntry->Stream, TO_LAB_Global.Tlm_pipe, SubEntry->Flags, SubEntry->BufLimit);
            if (SubscribeResult != CFE_SUCCESS)
            {
                (void)CFE_EVS_SendEvent(TO_LAB_SUBSCRIBE_ERR_EID,
                                        CFE_EVS_EventType_ERROR,
                                        "L%d TO Can't subscribe to stream 0x%x status %i",
                                        __LINE__,
                                        (unsigned int)CFE_SB_MsgIdToValue(SubEntry->Stream),
                                        (int)SubscribeResult);
            }
            else
            {
                TO_LAB_Global.ActiveSubs[NumberOfSubscriptions] = SubEntry->Stream;
                ++NumberOfSubscriptions;
            }

            /* A valid TO Subscriptions table entry followed an invalid entry */
            if (PrevEntryInvalid)
            {
                CFE_ES_WriteToSysLog("Entry with stream ID 0x%04X was preceded by %d invalid entries",
                                     (unsigned int)CFE_SB_MsgIdToValue(SubEntry->Stream),
                                     NumInvalidEntries);

                PrevEntryInvalid  = false;
                NumInvalidEntries = 0;
            }
        }
        else
        {
            PrevEntryInvalid = true;
            NumInvalidEntries++;

            CFE_EVS_SendEvent(TO_LAB_SUBSCRIBE_DBG_EID,
                              CFE_EVS_EventType_DEBUG,
                              "TO was unable to subscribe to stream 0x%04X at entry %d",
                              (unsigned int)CFE_SB_MsgIdToValue(SubEntry->Stream),
                              TableIndex + 1);
        }
    }

    /* Update interal count of active subscriptions */
    TO_LAB_Global.ActiveSubCount = NumberOfSubscriptions;

    (void)CFE_EVS_SendEvent(TO_LAB_SUBSCRIBE_INF_EID,
                            CFE_EVS_EventType_INFORMATION,
                            "L%d TO Lab subscribed to %u messages from the table",
                            __LINE__,
                            NumberOfSubscriptions);

    return NumberOfSubscriptions;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_LAB_ManageTables() -- Manage Subscription Table              */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void TO_LAB_ManageTables(void)
{
    CFE_Status_t Result;

    /*
    ** Must release loadable table pointers before allowing updates
    */
    CFE_TBL_ReleaseAddress(TO_LAB_Global.SubsTblHandle);
    CFE_TBL_Manage(TO_LAB_Global.SubsTblHandle);

    /*
    ** Re-acquire the pointers and check for new table data
    */
    Result = CFE_TBL_GetAddress((void *)&TO_LAB_Global.SubsTblPtr, TO_LAB_Global.SubsTblHandle);

    if (Result == CFE_TBL_INFO_UPDATED)
    {
        /* Update subscriptions */
        TO_LAB_UpdateSubscriptionsFromTable();
    }
    else if (Result != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(TO_LAB_TBL_MANAGE_ERR_EID,
                          CFE_EVS_EventType_ERROR,
                          "L%d Error getting new table address, RC=0x%08X",
                          __LINE__,
                          (unsigned int)Result);
    }
}

/************************/
/*  End of File Comment */
/************************/
