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

#include "cfe_config.h"
#include "cfe_sb.h"
#include "cfe_msg.h"
#include "cfe_error.h"

#include "to_lab_app.h"
#include "to_lab_encode.h"

#include "edslib_global.h"
#include "edslib_datatypedb.h"
#include "edslib_intfdb.h"
#include "cfe_missionlib_api.h"
#include "cfe_missionlib_runtime.h"
#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"

#include "cfe_hdr_eds_datatypes.h"

/*
 * ---------------------------------------
 * Helper function: Encodes the header and payload in the outgoing message
 * This is the preferred approach where everything is defined by EDS.
 * ---------------------------------------
 */
CFE_Status_t TO_LAB_EncodeOutputPayload(void                    *DestBufPtr,
                                        EdsLib_Id_t              ParentIntfId,
                                        const CFE_SB_Buffer_t   *SourceBuffer,
                                        const EdsLib_SizeInfo_t *MaxSize,
                                        EdsLib_SizeInfo_t       *ProcessedSize)
{
    int32        EdsStatus;
    CFE_Status_t ResultStatus;
    EdsLib_Id_t  EdsId;

    static const EdsLib_Id_t CFE_SB_TELEMETRY_ID =
        EDSLIB_INTF_ID(EDS_INDEX(CFE_SB), EdsCommand_CFE_SB_Telemetry_indication_DECLARATION);

    const EdsLib_DatabaseObject_t *EDS_DB = CFE_Config_GetObjPointer(CFE_CONFIGID_MISSION_EDS_DB);

    ResultStatus = CFE_SUCCESS;

    /* Note on the encode side, this does not need to process the header separately from the payload. */
    EdsStatus = EdsLib_IntfDB_FindAllArgumentTypes(EDS_DB, CFE_SB_TELEMETRY_ID, ParentIntfId, &EdsId, 1);
    if (EdsStatus != EDSLIB_SUCCESS)
    {
        OS_printf("EdsLib_IntfDB_FindAllArgumentTypes(): %d\n", (int)EdsStatus);
        ResultStatus = CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    else
    {
        EdsStatus = EdsLib_DataTypeDB_PackPartialObjectVarSize(EDS_DB,
                                                               &EdsId,
                                                               DestBufPtr,
                                                               SourceBuffer,
                                                               MaxSize,
                                                               ProcessedSize);
        if (EdsStatus != EDSLIB_SUCCESS)
        {
            OS_printf("EdsLib_DataTypeDB_PackPartialObjectVarSize(Payload): %d\n", (int)EdsStatus);
            ResultStatus = CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
        }
    }

    if (ResultStatus == CFE_SUCCESS)
    {
        EdsStatus =
            EdsLib_DataTypeDB_FinalizePackedObjectVarSize(EDS_DB, EdsId, SourceBuffer, DestBufPtr, ProcessedSize);
        if (EdsStatus != EDSLIB_SUCCESS)
        {
            OS_printf("EdsLib_DataTypeDB_FinalizePackedObjectVarSize(): %d\n", (int)EdsStatus);
            ResultStatus = CFE_STATUS_VALIDATION_FAILURE;
        }
    }

    return ResultStatus;
}

/*
 * ---------------------------------------
 * Helper function: Passes through the payload from the outgoing message
 * This is the historical behavior and may be used as a fallback option
 * if the application does not have an EDS.  The bits will be copied verbatim.
 * (The header is still encoded with EDS)
 * ---------------------------------------
 */
CFE_Status_t TO_LAB_PassthruOutputPayload(void                    *DestBufPtr,
                                          const CFE_SB_Buffer_t   *SourceBuffer,
                                          const EdsLib_SizeInfo_t *MaxSize,
                                          EdsLib_SizeInfo_t       *ProcessedSize)
{
    int32        EdsStatus;
    CFE_Status_t ResultStatus;
    EdsLib_Id_t  EdsId;
    uint8       *Dest;
    const uint8 *Src;
    size_t       HeaderSize;
    size_t       TotalSize;

    const EdsLib_DatabaseObject_t *EDS_DB = CFE_Config_GetObjPointer(CFE_CONFIGID_MISSION_EDS_DB);

    ResultStatus = CFE_SUCCESS;

    EdsId = EDSLIB_MAKE_ID(EDS_INDEX(CFE_HDR), EdsContainer_CFE_HDR_TelemetryHeader_DATADICTIONARY);
    EdsStatus =
        EdsLib_DataTypeDB_PackPartialObjectVarSize(EDS_DB, &EdsId, DestBufPtr, SourceBuffer, MaxSize, ProcessedSize);
    if (EdsStatus != EDSLIB_SUCCESS)
    {
        OS_printf("EdsLib_DataTypeDB_PackPartialObjectVarSize(Header): %d\n", (int)EdsStatus);
        ResultStatus = CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    else
    {
        HeaderSize = EdsLib_BITS_TO_OCTETS(ProcessedSize->Bits);
        TotalSize  = MaxSize->Bytes;

        if ((EdsLib_BITS_TO_OCTETS(MaxSize->Bits) - HeaderSize) < (MaxSize->Bytes - ProcessedSize->Bytes))
        {
            /* will not fit in dest buffer */
            ResultStatus = CFE_SB_MSG_TOO_BIG;
        }
        else
        {
            /* If necessary, copy the payload.  Some commands (e.g. Noop) do not have a payload */
            if (TotalSize > HeaderSize)
            {
                Dest = (uint8 *)DestBufPtr;
                Src  = (const uint8 *)&SourceBuffer->Msg;

                Dest += HeaderSize;
                Src  += ProcessedSize->Bytes;

                memcpy(Dest, Src, MaxSize->Bytes - ProcessedSize->Bytes);
            }

            /* Need to adjust the bit length, because the header was encoded based on EDS */
            TotalSize -= ProcessedSize->Bytes;
            TotalSize += HeaderSize;

            /* the byte length reflects the octets consumed from the source buffer */
            ProcessedSize->Bytes = MaxSize->Bytes;
            ProcessedSize->Bits  = EdsLib_OCTETS_TO_BITS(TotalSize);
        }
    }

    if (ResultStatus == CFE_SUCCESS)
    {
        EdsStatus =
            EdsLib_DataTypeDB_FinalizePackedObjectVarSize(EDS_DB, EdsId, SourceBuffer, DestBufPtr, ProcessedSize);
        if (EdsStatus != EDSLIB_SUCCESS)
        {
            OS_printf("EdsLib_DataTypeDB_FinalizePackedObjectVarSize(): %d\n", (int)EdsStatus);
            ResultStatus = CFE_STATUS_VALIDATION_FAILURE;
        }
    }

    return ResultStatus;
}

/*
 * ---------------------------------------
 * Application-scope internal function: Encodes the full message
 * according to the output policy.
 * ---------------------------------------
 */
CFE_Status_t
TO_LAB_EncodeOutputMessage(const CFE_SB_Buffer_t *SourceBuffer, const void **DestBufferOut, size_t *DestSizeOut)
{
    CFE_MissionLib_TopicInfo_t            TopicInfo;
    CFE_SB_SoftwareBus_PubSub_Interface_t PubSubParams;
    CFE_SB_Publisher_Component_t          PublisherParams;
    int32                                 EdsStatus;
    CFE_Status_t                          ResultStatus;
    CFE_MSG_Size_t                        SourceBufferSize;
    EdsLib_SizeInfo_t                     MaxSize;
    EdsLib_SizeInfo_t                     ProcessedSize;

    memset(&MaxSize, 0, sizeof(MaxSize));
    memset(&ProcessedSize, 0, sizeof(ProcessedSize));

    static EdsPackedBuffer_CFE_HDR_TelemetryHeader_t NetworkBuffer;

    ResultStatus = CFE_MSG_GetSize(&SourceBuffer->Msg, &SourceBufferSize);
    if (ResultStatus == CFE_SUCCESS)
    {
        MaxSize.Bits  = EdsLib_OCTETS_TO_BITS(sizeof(NetworkBuffer));
        MaxSize.Bytes = SourceBufferSize;
    }

    CFE_MissionLib_Get_PubSub_Parameters(&PubSubParams, &SourceBuffer->Msg.BaseMsg);
    CFE_MissionLib_UnmapPublisherComponent(&PublisherParams, &PubSubParams);

    EdsStatus = CFE_MissionLib_GetTopicInfo(&CFE_SOFTWAREBUS_INTERFACE, PublisherParams.Telemetry.TopicId, &TopicInfo);
    if (EdsStatus != CFE_MISSIONLIB_SUCCESS)
    {
        OS_printf("CFE_MissionLib_GetTopicInfo(): %d\n", (int)EdsStatus);
        ResultStatus = CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    else if (EdsLib_Is_Valid(TopicInfo.ParentIntfId))
    {
        ResultStatus =
            TO_LAB_EncodeOutputPayload(NetworkBuffer, TopicInfo.ParentIntfId, SourceBuffer, &MaxSize, &ProcessedSize);
    }
    else if (TO_LAB_Global.AllowPassthru)
    {
        ResultStatus = TO_LAB_PassthruOutputPayload(NetworkBuffer, SourceBuffer, &MaxSize, &ProcessedSize);
    }
    else
    {
        OS_printf("TO_LAB_EncodeOutputMessage(): No EDS definition for TopicId=%u\n",
                  (unsigned int)PublisherParams.Telemetry.TopicId);
        ResultStatus = CFE_STATUS_VALIDATION_FAILURE;
    }

    *DestSizeOut   = EdsLib_BITS_TO_OCTETS(ProcessedSize.Bits);
    *DestBufferOut = NetworkBuffer;

    return ResultStatus;
}
