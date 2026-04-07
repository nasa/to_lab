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
 *   Specification for the TO_LAB command and telemetry
 *   message payload and constant definitions.
 */
#ifndef DEFAULT_TO_LAB_MSGDEFS_H
#define DEFAULT_TO_LAB_MSGDEFS_H

#include "common_types.h"
#include "cfe_sb_extern_typedefs.h"
#include "to_lab_fcncodes.h"

typedef struct
{
    uint8 CommandCounter;
    uint8 CommandErrorCounter;
} TO_LAB_HkTlm_Payload_t;

typedef struct
{
    uint16 synch;
    uint8  bl1, bl2; /* boolean */
    int8   b1, b2, b3, b4;
    int16  w1, w2;
    int32  dw1, dw2;
    float  f1, f2;
    double df1, df2;
    char   str[10];
} TO_LAB_DataTypes_Payload_t;

typedef struct
{
    CFE_SB_MsgId_t Stream;
    CFE_SB_Qos_t   Flags;
    uint8          BufLimit;
} TO_LAB_AddPacket_Payload_t;

typedef struct
{
    CFE_SB_MsgId_t Stream;
} TO_LAB_RemovePacket_Payload_t;

typedef struct
{
    char dest_IP[16];
} TO_LAB_EnableOutput_Payload_t;

#endif
