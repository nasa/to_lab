/************************************************************************
**
**      GSC-18128-1, "Core Flight Executive Version 6.7"
**
**      Copyright (c) 2006-2002 United States Government as represented by
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
** File: to_lab_sub_table.h
**
** Purpose:
**  Define TO Lab CPU specific subscription table
**
** Notes:
**
*************************************************************************/
#ifndef to_lab_sub_table_h_
#define to_lab_sub_table_h_

#include "cfe_msgids.h"
#include "cfe_platform_cfg.h"
#include "cfe_sb.h"

typedef struct
{
    CFE_SB_MsgId_t Stream;
    CFE_SB_Qos_t   Flags;
    uint16         BufLimit;
} TO_LAB_Sub_t;

typedef struct
{
    TO_LAB_Sub_t Subs[CFE_PLATFORM_SB_MAX_MSG_IDS];
}
TO_LAB_Subs_t;

#endif /* to_lab_sub_table_h_ */
