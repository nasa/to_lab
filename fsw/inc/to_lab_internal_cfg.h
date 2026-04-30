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
 *   TO_LAB Application Private Config Definitions
 *
 * This provides default values for configurable items that are internal
 * to this module and do NOT affect the interface(s) of this module.  Changes
 * to items in this file only affect the local module and will be transparent
 * to external entities that are using the public interface(s).
 *
 * @note This file may be overridden/superceded by mission-provided definitions
 * either by overriding this header or by generating definitions from a command/data
 * dictionary tool.
 */
#ifndef TO_LAB_INTERNAL_CFG_H
#define TO_LAB_INTERNAL_CFG_H

#include "to_lab_mission_cfg.h"
#include "to_lab_internal_cfg_values.h"

/*****************************************************************************/

/**
 * @brief Telemetry pipe timeout
 *
 * When there is no activity on the telemetry pipe for this
 * time period, then the command pipe will be checked.  Smaller
 * values will cause the command pipe to be checked more agressively
 * but will increase CPU usage.
 *
 * Units are the same as CFE_SB_ReceiveBuffer (milliseconds)
 */
#define TO_LAB_PLATFORM_TLM_PIPE_TIMEOUT         TO_LAB_PLATFORM_CFGVAL(TLM_PIPE_TIMEOUT)
#define DEFAULT_TO_LAB_PLATFORM_TLM_PIPE_TIMEOUT 50

/**
 * @brief Maximum number of telemetry packets to send each wakeup
 */
#define TO_LAB_PLATFORM_MAX_TLM_PKTS         TO_LAB_PLATFORM_CFGVAL(MAX_TLM_PKTS)
#define DEFAULT_TO_LAB_PLATFORM_MAX_TLM_PKTS OS_QUEUE_MAX_DEPTH

/**
 * Depth of pipe for commands to the TO_LAB application itself
 */
#define TO_LAB_PLATFORM_CMD_PIPE_DEPTH         TO_LAB_PLATFORM_CFGVAL(CMD_PIPE_DEPTH)
#define DEFAULT_TO_LAB_PLATFORM_CMD_PIPE_DEPTH 8

/**
 * Depth of pipe for telemetry forwarded through the TO_LAB application
 */
#define TO_LAB_PLATFORM_TLM_PIPE_DEPTH         TO_LAB_PLATFORM_CFGVAL(TLM_PIPE_DEPTH)
#define DEFAULT_TO_LAB_PLATFORM_TLM_PIPE_DEPTH OS_QUEUE_MAX_DEPTH

/**
 * Startup Sync timeout
 *
 * This is the maximum amount of time to wait for the system to reach OPERATIONAL state
 * before subscribing to all of the MsgIDs in the table.  Importantly this can defer
 * the subscription until after all apps have sent their startup event, which reduces
 * the likelihood of seeing a MsgLimit error.
 */
#define TO_LAB_STARTUP_SYNC_TIMEOUT                  TO_LAB_PLATFORM_CFGVAL(STARTUP_SYNC_TIMEOUT)
#define DEFAULT_TO_LAB_PLATFORM_STARTUP_SYNC_TIMEOUT 10000

#endif
