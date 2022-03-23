/**
 * @file
 *   Define TO Lab CPU specific subscription table
 */
#ifndef TO_LAB_SUB_TABLE_H
#define TO_LAB_SUB_TABLE_H

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
} TO_LAB_Subs_t;

#endif
