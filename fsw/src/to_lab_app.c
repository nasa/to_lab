/************************************************************************
**
**      GSC-18128-1, "Core Flight Executive Version 6.7"
**
**      Copyright (c) 2006-2019 United States Government as represented by
**      the Administrator of the National Aeronautics and Space Administration.
**      All Rights Reserved.
**
**      Licensed under the Apache License, Version 2.0 (the "License");
**      you may not use this file except in compliance with the License.
**      You may obtain a copy of the License at
**
**        http://www.apache.org/licenses/LICENSE-2.0
**
**      Unless required by applicable law or agreed to in writing, software
**      distributed under the License is distributed on an "AS IS" BASIS,
**      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**      See the License for the specific language governing permissions and
**      limitations under the License.
**
** File: to_lab_app.c
**
** Purpose: 
**  his file contains the source code for the TO lab application
**
** Notes:
**
*************************************************************************/

#include "to_lab_app.h"
#include "to_lab_msg.h"
#include "to_lab_events.h"
#include "to_lab_msgids.h"
#include "to_lab_perfids.h"
#include "to_lab_version.h"


/*
** Global Data Section
*/
typedef union
{
    CFE_SB_Msg_t     MsgHdr;
    to_hk_tlm_t      HkTlm;
} TO_LAB_HkTlm_Buffer_t;

typedef union
{
    CFE_SB_Msg_t       MsgHdr;
    to_data_types_fmt  DataTypes;
} TO_LAB_DataTypes_Buffer_t;

typedef struct
{
    CFE_SB_PipeId_t    Tlm_pipe;
    CFE_SB_PipeId_t    Cmd_pipe;
    uint32             TLMsockid;
    bool               downlink_on;
    char               tlm_dest_IP[17];
    bool               suppress_sendto;

    TO_LAB_HkTlm_Buffer_t     HkBuf;
    TO_LAB_DataTypes_Buffer_t DataTypesBuf;
} TO_LAB_GlobalData_t;

TO_LAB_GlobalData_t TO_LAB_Global;

/*
** Include the TO subscription table
**  This header is in the platform include directory
**  and can be changed for default TO subscriptions in 
**  each CPU.
*/
#include "to_lab_sub_table.h"

/*
** Event Filter Table
*/
static CFE_EVS_BinFilter_t  CFE_TO_EVS_Filters[] =
          { /* Event ID    mask */
            {TO_INIT_INF_EID,           0x0000},
            {TO_CRCMDPIPE_ERR_EID,      0x0000},
            {TO_SUBSCRIBE_ERR_EID,      0x0000},
            {TO_TLMOUTSOCKET_ERR_EID,   0x0000},
            {TO_TLMOUTSTOP_ERR_EID,     0x0000},
            {TO_MSGID_ERR_EID,          0x0000},
            {TO_FNCODE_ERR_EID,         0x0000},
            {TO_NOOP_INF_EID,           0x0000}
          };

/*
** Prototypes Section
*/
static void TO_openTLM(void);
static void TO_init(void);
static void TO_process_commands(void);
static void TO_exec_local_command(CFE_SB_Msg_t  *MsgPtr);
static void TO_reset_status(void);
static void TO_output_data_types_packet(void);
static void TO_output_status(void);
static void TO_AddPkt(TO_ADD_PKT_t * cmd);
static void TO_RemovePkt(TO_REMOVE_PKT_t * cmd);
static void TO_RemoveAllPkt(void);
static void TO_forward_telemetry(void);
static void TO_StartSending( TO_OUTPUT_ENABLE_PKT_t * pCmd );

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                   */
/* TO_Lab_AppMain() -- Application entry point and main process loop */
/*                                                                   */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void TO_Lab_AppMain(void)
{
   uint32                  RunStatus = CFE_ES_RunStatus_APP_RUN;
   
   CFE_ES_PerfLogEntry(TO_MAIN_TASK_PERF_ID);

   TO_init();

   /*
   ** TO RunLoop
   */
   while(CFE_ES_RunLoop(&RunStatus) == true)
   {
        CFE_ES_PerfLogExit(TO_MAIN_TASK_PERF_ID);

        OS_TaskDelay(TO_TASK_MSEC);    /*2 Hz*/

        CFE_ES_PerfLogEntry(TO_MAIN_TASK_PERF_ID);

        TO_forward_telemetry();

        TO_process_commands();
    }

   CFE_ES_ExitApp(RunStatus);

} /* End of TO_Lab_AppMain() */

/*
** TO delete callback function.
** This function will be called in the event that the TO app is killed. 
** It will close the network socket for TO
*/
void TO_delete_callback(void)
{
    OS_printf("TO delete callback -- Closing TO Network socket.\n");
    if ( TO_LAB_Global.downlink_on )
    {
        OS_close(TO_LAB_Global.TLMsockid);
    }
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_init() -- TO initialization                                  */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void TO_init(void)
{
    int32            status;
    char             PipeName[16];
    uint16           PipeDepth;
    uint16           i;
    char             ToTlmPipeName[16];
    uint16           ToTlmPipeDepth;

    CFE_ES_RegisterApp();
    TO_LAB_Global.downlink_on = false;
    PipeDepth = 8;
    strcpy(PipeName,  "TO_LAB_CMD_PIPE");
    ToTlmPipeDepth = 64;
    strcpy(ToTlmPipeName,  "TO_LAB_TLM_PIPE");

    /*
    ** Register event filter table...
    */
    CFE_EVS_Register(CFE_TO_EVS_Filters,
                     sizeof(CFE_TO_EVS_Filters)/sizeof(CFE_EVS_BinFilter_t),
                     CFE_EVS_EventFilter_BINARY);
    /*
    ** Initialize housekeeping packet (clear user data area)...
    */
    CFE_SB_InitMsg(&TO_LAB_Global.HkBuf.MsgHdr,
                    TO_LAB_HK_TLM_MID,
                    sizeof(TO_LAB_Global.HkBuf.HkTlm), true);

    /* Subscribe to my commands */
    status = CFE_SB_CreatePipe(&TO_LAB_Global.Cmd_pipe, PipeDepth, PipeName);
    if (status == CFE_SUCCESS)
    {
       CFE_SB_Subscribe(TO_LAB_CMD_MID,     TO_LAB_Global.Cmd_pipe);
       CFE_SB_Subscribe(TO_LAB_SEND_HK_MID, TO_LAB_Global.Cmd_pipe);
    }
    else
       CFE_EVS_SendEvent(TO_CRCMDPIPE_ERR_EID,CFE_EVS_EventType_ERROR,
             "L%d TO Can't create cmd pipe status %i",__LINE__,(int)status);

    /* Create TO TLM pipe */
    status = CFE_SB_CreatePipe(&TO_LAB_Global.Tlm_pipe, ToTlmPipeDepth, ToTlmPipeName);
    if (status != CFE_SUCCESS)
    {
       CFE_EVS_SendEvent(TO_TLMPIPE_ERR_EID,CFE_EVS_EventType_ERROR,
             "L%d TO Can't create Tlm pipe status %i",__LINE__,(int)status);
    }

    /* Subscriptions for TLM pipe*/
    for (i=0; (i < (sizeof(TO_SubTable)/sizeof(TO_subscription_t))); i++)
    {
       if(TO_SubTable[i].Stream != TO_UNUSED )
          status = CFE_SB_SubscribeEx(TO_SubTable[i].Stream,
                                      TO_LAB_Global.Tlm_pipe,
                                      TO_SubTable[i].Flags,
                                      TO_SubTable[i].BufLimit);

       if (status != CFE_SUCCESS)
           CFE_EVS_SendEvent(TO_SUBSCRIBE_ERR_EID,CFE_EVS_EventType_ERROR,
              "L%d TO Can't subscribe to stream 0x%x status %i", __LINE__,
                             TO_SubTable[i].Stream,(int)status);
    }
    
    /*
    ** Install the delete handler
    */
    OS_TaskInstallDeleteHandler(&TO_delete_callback);

    CFE_EVS_SendEvent (TO_INIT_INF_EID, CFE_EVS_EventType_INFORMATION,
               "TO Lab Initialized. Version %d.%d.%d.%d Awaiting enable command.",
                TO_LAB_MAJOR_VERSION,
                TO_LAB_MINOR_VERSION, 
                TO_LAB_REVISION, 
                TO_LAB_MISSION_REV);
    
} /* End of TO_Init() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_StartSending() -- TLM output enabled                         */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void TO_StartSending( TO_OUTPUT_ENABLE_PKT_t * pCmd )
{
    (void) CFE_SB_MessageStringGet(TO_LAB_Global.tlm_dest_IP, pCmd->dest_IP, "",
                                   sizeof (TO_LAB_Global.tlm_dest_IP),
                                   sizeof (pCmd->dest_IP));
    TO_LAB_Global.suppress_sendto = false;
    CFE_EVS_SendEvent(TO_TLMOUTENA_INF_EID,CFE_EVS_EventType_INFORMATION,
                      "TO telemetry output enabled for IP %s", TO_LAB_Global.tlm_dest_IP);

    if( !TO_LAB_Global.downlink_on ) /* Then turn it on, otherwise we will just switch destination addresses*/
    {
       TO_openTLM();
       TO_LAB_Global.downlink_on = true;
    }
} /* End of TO_StartSending() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_process_commands() -- Process command pipe message           */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void TO_process_commands(void)
{
    CFE_SB_Msg_t    *MsgPtr;
    CFE_SB_MsgId_t  MsgId;

    while(1)
    {
       switch (CFE_SB_RcvMsg(&MsgPtr, TO_LAB_Global.Cmd_pipe, CFE_SB_POLL))
       {
          case CFE_SUCCESS:

               MsgId = CFE_SB_GetMsgId(MsgPtr);
               /* For SB return statuses that imply a message: process it. */
               switch (MsgId)
               {
                  case TO_LAB_CMD_MID:
                       TO_exec_local_command(MsgPtr);
                       break;

                  case TO_LAB_SEND_HK_MID:
                       TO_output_status();
                       break;

                  default:
                       CFE_EVS_SendEvent(TO_MSGID_ERR_EID,CFE_EVS_EventType_ERROR,
                            "L%d TO: Invalid Msg ID Rcvd 0x%x",__LINE__,MsgId);
                       break;
               }
               break;
            default:
               /* Exit command processing loop if no message received. */
               return;
       }
    }
} /* End of TO_process_commands() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/*  TO_exec_local_command() -- Process local message               */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void TO_exec_local_command(CFE_SB_MsgPtr_t cmd)
{
    uint16 CommandCode;
    bool valid = true;
    CommandCode = CFE_SB_GetCmdCode(cmd);

    switch (CommandCode)
    {
       case TO_NOP_CC:
            CFE_EVS_SendEvent(TO_NOOP_INF_EID,CFE_EVS_EventType_INFORMATION,
                              "No-op command");
            break;

       case TO_RESET_STATUS_CC:
            TO_reset_status();
            --TO_LAB_Global.HkBuf.HkTlm.command_counter;
            break;

       case TO_SEND_DATA_TYPES_CC:
            TO_output_data_types_packet();
            break;

       case TO_ADD_PKT_CC:
            TO_AddPkt((TO_ADD_PKT_t *)cmd);
            break;

       case TO_REMOVE_PKT_CC:
            TO_RemovePkt( (TO_REMOVE_PKT_t *)cmd);
            break;

       case TO_REMOVE_ALL_PKT_CC:
            TO_RemoveAllPkt();
            break;

       case TO_OUTPUT_ENABLE_CC:
            TO_StartSending( (TO_OUTPUT_ENABLE_PKT_t *)cmd );
            TO_LAB_Global.downlink_on = TRUE;
            break;

       default:
            CFE_EVS_SendEvent(TO_FNCODE_ERR_EID,CFE_EVS_EventType_ERROR,
                "L%d TO: Invalid Function Code Rcvd In Ground Command 0x%x",__LINE__,
                              CommandCode);
            valid = false;
    }

    if (valid)
       ++TO_LAB_Global.HkBuf.HkTlm.command_counter;
    else
       ++TO_LAB_Global.HkBuf.HkTlm.command_error_counter;
} /* End of TO_exec_local_command() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_reset_status() -- Reset counters                             */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void TO_reset_status(void)
{
    TO_LAB_Global.HkBuf.HkTlm.command_error_counter = 0;
    TO_LAB_Global.HkBuf.HkTlm.command_counter = 0;
} /* End of TO_reset_status() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_output_data_types_packet()  -- Output data types             */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void TO_output_data_types_packet(void)
{
    int16             i;
    char             string_variable[10] = "ABCDEFGHIJ";

    /* initialize data types packet */
    CFE_SB_InitMsg(&TO_LAB_Global.DataTypesBuf.MsgHdr,
                   TO_LAB_DATA_TYPES_MID,
                   sizeof(TO_LAB_Global.DataTypesBuf.DataTypes), true);

    CFE_SB_TimeStampMsg(&TO_LAB_Global.DataTypesBuf.MsgHdr);

    /* initialize the packet data */
    TO_LAB_Global.DataTypesBuf.DataTypes.synch = 0x6969;
#if 0
    TO_LAB_Global.DataTypesBuf.DataTypes.bit1 = 1;
    TO_LAB_Global.DataTypesBuf.DataTypes.bit2 = 0;
    TO_LAB_Global.DataTypesBuf.DataTypes.bit34 = 2;
    TO_LAB_Global.DataTypesBuf.DataTypes.bit56 = 3;
    TO_LAB_Global.DataTypesBuf.DataTypes.bit78 = 1;
    TO_LAB_Global.DataTypesBuf.DataTypes.nibble1 = 0xA;
    TO_LAB_Global.DataTypesBuf.DataTypes.nibble2 = 0x4;
#endif
    TO_LAB_Global.DataTypesBuf.DataTypes.bl1 = false;
    TO_LAB_Global.DataTypesBuf.DataTypes.bl2 = true;
    TO_LAB_Global.DataTypesBuf.DataTypes.b1 = 16;
    TO_LAB_Global.DataTypesBuf.DataTypes.b2 = 127;
    TO_LAB_Global.DataTypesBuf.DataTypes.b3 = 0x7F;
    TO_LAB_Global.DataTypesBuf.DataTypes.b4 = 0x45;
    TO_LAB_Global.DataTypesBuf.DataTypes.w1 = 0x2468;
    TO_LAB_Global.DataTypesBuf.DataTypes.w2 = 0x7FFF;
    TO_LAB_Global.DataTypesBuf.DataTypes.dw1 = 0x12345678;
    TO_LAB_Global.DataTypesBuf.DataTypes.dw2 = 0x87654321;
    TO_LAB_Global.DataTypesBuf.DataTypes.f1 = 90.01;
    TO_LAB_Global.DataTypesBuf.DataTypes.f2 = .0000045;
    TO_LAB_Global.DataTypesBuf.DataTypes.df1 = 99.9;
    TO_LAB_Global.DataTypesBuf.DataTypes.df2 = .4444;

    for (i=0; i < 10; i++) TO_LAB_Global.DataTypesBuf.DataTypes.str[i] = string_variable[i];

    CFE_SB_SendMsg(&TO_LAB_Global.DataTypesBuf.MsgHdr);
} /* End of TO_output_data_types_packet() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_output_status() -- HK status                                 */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void TO_output_status(void)
{
    CFE_SB_TimeStampMsg(&TO_LAB_Global.HkBuf.MsgHdr);
    CFE_SB_SendMsg(&TO_LAB_Global.HkBuf.MsgHdr);
} /* End of TO_output_status() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_openTLM() -- Open TLM                                        */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void TO_openTLM(void)
{
   int32 status;

   status = OS_SocketOpen(&TO_LAB_Global.TLMsockid, OS_SocketDomain_INET, OS_SocketType_DATAGRAM);
   if ( status != OS_SUCCESS )
   {
      CFE_EVS_SendEvent(TO_TLMOUTSOCKET_ERR_EID,CFE_EVS_EventType_ERROR, "L%d, TO TLM socket error: %d",__LINE__, (int)status);
   }

/*---------------- Add static arp entries ----------------*/

} /* End of TO_open_TLM() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_AddPkt() -- Add packets                                      */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void TO_AddPkt( TO_ADD_PKT_t * pCmd)
{
    int32  status;

    status = CFE_SB_SubscribeEx(pCmd->Stream,
                                TO_LAB_Global.Tlm_pipe,
                                pCmd->Flags,
                                pCmd->BufLimit);

    if(status != CFE_SUCCESS)
       CFE_EVS_SendEvent(TO_ADDPKT_ERR_EID,CFE_EVS_EventType_ERROR,
                         "L%d TO Can't subscribe 0x%x status %i",__LINE__,
                         pCmd->Stream, (int)status);
    else
       CFE_EVS_SendEvent(TO_ADDPKT_INF_EID,CFE_EVS_EventType_INFORMATION,
                         "L%d TO AddPkt 0x%x, QoS %d.%d, limit %d",__LINE__,
                         pCmd->Stream,
                         pCmd->Flags.Priority,
                         pCmd->Flags.Reliability,
                         pCmd->BufLimit);
} /* End of TO_AddPkt() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_RemovePkt() -- Remove Packet                                 */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void TO_RemovePkt(TO_REMOVE_PKT_t * pCmd)
{
    int32  status;

    status = CFE_SB_Unsubscribe(pCmd->Stream, TO_LAB_Global.Tlm_pipe);
    if(status != CFE_SUCCESS)
       CFE_EVS_SendEvent(TO_REMOVEPKT_ERR_EID,CFE_EVS_EventType_ERROR,
           "L%d TO Can't Unsubscribe to Stream 0x%x on pipe %d, status %i",__LINE__,
                         pCmd->Stream, TO_LAB_Global.Tlm_pipe, (int)status);
    else
       CFE_EVS_SendEvent(TO_REMOVEPKT_INF_EID,CFE_EVS_EventType_INFORMATION,
           "L%d TO RemovePkt 0x%x",__LINE__, pCmd->Stream);
} /* End of TO_RemovePkt() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_RemoveAllPkt() --  Remove All Packets                        */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void TO_RemoveAllPkt(void)
{
    int32  status;
    int i;

    for (i=0; (i < (sizeof(TO_SubTable)/sizeof(TO_subscription_t))); i++)
    {
       if (TO_SubTable[i].Stream != TO_UNUSED )
       {
          status = CFE_SB_Unsubscribe(TO_SubTable[i].Stream, TO_LAB_Global.Tlm_pipe);

          if(status != CFE_SUCCESS)
             CFE_EVS_SendEvent(TO_REMOVEALLPTKS_ERR_EID,CFE_EVS_EventType_ERROR,
                  "L%d TO Can't Unsubscribe to stream 0x%x status %i", __LINE__,
                               TO_SubTable[i].Stream, (int)status);
       }
    }

    /* remove commands as well */
    status = CFE_SB_Unsubscribe(TO_LAB_CMD_MID, TO_LAB_Global.Cmd_pipe);
    if(status != CFE_SUCCESS)
       CFE_EVS_SendEvent(TO_REMOVECMDTO_ERR_EID,CFE_EVS_EventType_ERROR,
                   "L%d TO Can't Unsubscribe to cmd stream 0x%x status %i", __LINE__,
                         TO_LAB_CMD_MID, (int)status);

    status = CFE_SB_Unsubscribe(TO_LAB_SEND_HK_MID, TO_LAB_Global.Cmd_pipe);
    if (status != CFE_SUCCESS)
       CFE_EVS_SendEvent(TO_REMOVEHKTO_ERR_EID,CFE_EVS_EventType_ERROR,
            "L%d TO Can't Unsubscribe to cmd stream 0x%x status %i", __LINE__,
                         TO_LAB_CMD_MID, (int)status);

    CFE_EVS_SendEvent(TO_REMOVEALLPKTS_INF_EID,CFE_EVS_EventType_INFORMATION,
            "L%d TO Unsubscribed to all Commands and Telemetry", __LINE__);
} /* End of TO_RemoveAllPkt() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_forward_telemetry() -- Forward telemetry                     */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void TO_forward_telemetry(void)
{
    OS_SockAddr_t             d_addr;
    int32                     status;
    int32                     CFE_SB_status;
    uint16                    size;
    CFE_SB_Msg_t              *PktPtr;

    OS_SocketAddrInit(&d_addr, OS_SocketDomain_INET);
    OS_SocketAddrSetPort(&d_addr, cfgTLM_PORT);
    OS_SocketAddrFromString(&d_addr, TO_LAB_Global.tlm_dest_IP);
    status = 0;

    do
    {
       CFE_SB_status = CFE_SB_RcvMsg(&PktPtr, TO_LAB_Global.Tlm_pipe, CFE_SB_POLL);

       if ( (CFE_SB_status == CFE_SUCCESS) && (TO_LAB_Global.suppress_sendto == false) )
       {
          size = CFE_SB_GetTotalMsgLength(PktPtr);
          
          if(TO_LAB_Global.downlink_on == true)
          {
             CFE_ES_PerfLogEntry(TO_SOCKET_SEND_PERF_ID);

             status = OS_SocketSendTo(TO_LAB_Global.TLMsockid, PktPtr, size, &d_addr);
                                         
             CFE_ES_PerfLogExit(TO_SOCKET_SEND_PERF_ID);                             
          }
          else
          {
              status = 0;
          }
          if (status < 0)
          {
             CFE_EVS_SendEvent(TO_TLMOUTSTOP_ERR_EID,CFE_EVS_EventType_ERROR,
                               "L%d TO sendto error %d. Tlm output supressed\n", __LINE__, (int)status);
             TO_LAB_Global.suppress_sendto = true;
          }
       }
    /* If CFE_SB_status != CFE_SUCCESS, then no packet was received from CFE_SB_RcvMsg() */
    }while(CFE_SB_status == CFE_SUCCESS);
} /* End of TO_forward_telemetry() */

/************************/
/*  End of File Comment */
/************************/
