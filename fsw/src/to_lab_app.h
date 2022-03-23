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

/*****************************************************************************/

#define TO_TASK_MSEC 500 /* run at 2 Hz */
#define TO_UNUSED    CFE_SB_MSGID_RESERVED

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

/******************************************************************************/

/*
** Prototypes Section
*/
void TO_Lab_AppMain(void);

/******************************************************************************/

#endif
