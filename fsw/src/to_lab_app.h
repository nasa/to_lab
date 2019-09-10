/************************************************************************
**
**      GSC-18128-1, "Core Flight Executive Version 6.6"
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
** File: to_lab_app.h
**
** Purpose: 
**  Define TO Lab Application header file
**
** Notes:
**
*************************************************************************/

#ifndef _to_lab_app_h_
#define _to_lab_app_h_

#include "network_includes.h"

#include "cfe_error.h"
#include "cfe_sb.h"
#include "cfe_time.h"
#include "cfe_evs.h"
#include "cfe_sb.h"
#include "cfe_es.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "common_types.h"
#include "osapi.h"

/*****************************************************************************/

#define TO_TASK_MSEC             500          /* run at 2 Hz */
#define TO_UNUSED                  0

#define cfgTLM_ADDR "192.168.1.81"
#define cfgTLM_PORT 1235
#define TO_LAB_VERSION_NUM "5.1.0"

/******************************************************************************/

/*
** Prototypes Section
*/
void TO_Lab_AppMain(void);

/******************************************************************************/

#endif  /* _to_lab_app_h_ */

