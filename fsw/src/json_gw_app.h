/*
** Copyright 2022 bitValence, Inc.
** All Rights Reserved.
**
** This program is free software; you can modify and/or redistribute it
** under the terms of the GNU Affero General Public License
** as published by the Free Software Foundation; version 3 with
** attribution addendums as found in the LICENSE.txt
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Affero General Public License for more details.
**
** Purpose:
**   Manage a gateway between the cFS Software Bus and a UDP connection
**   sending JSON messages.
**
** Notes:
**   1. This app does not provide a generic message translation capability. It
**      is expected to be used in constrained environments so a trade for
**      simplicity and extendability was made over complexity and genericity.
**      A separate JSON table is defined for each supported MQTT topic combined
**      with a custom function to manage the translation.
**
*/
#ifndef _json_gw_app_
#define _json_gw_app_

/*
** Includes
*/

#include "app_cfg.h"
#include "udp_comm.h"

/***********************/
/** Macro Definitions **/
/***********************/

/*
** Events
*/

#define JSON_GW_INIT_APP_EID      (JSON_GW_BASE_EID + 0)
#define JSON_GW_NOOP_EID          (JSON_GW_BASE_EID + 1)
#define JSON_GW_EXIT_EID          (JSON_GW_BASE_EID + 2)
#define JSON_GW_INVALID_MID_EID   (JSON_GW_BASE_EID + 3)


/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Command Packets
*/
/* See EDS */

/******************************************************************************
** Telemetry Packets
*/
/* See EDS */

/******************************************************************************
** JSON_Class
*/
typedef struct 
{

   /* 
   ** App Framework
   */ 

   INITBL_Class_t    IniTbl; 
   CFE_SB_PipeId_t   CmdPipe;
   CMDMGR_Class_t    CmdMgr;
   CHILDMGR_Class_t  RxChildMgr;
   CHILDMGR_Class_t  TxChildMgr;
      
   /*
   ** Telemetry Packets
   */
   
   JSON_GW_StatusTlm_t   StatusTlm;

   
   /*
   ** JSON_GW State & Contained Objects
   */ 
   
   uint32 PerfId;
   
   CFE_SB_MsgId_t  CmdMid;
   CFE_SB_MsgId_t  SendStatusMid;
       
   UDP_COMM_Class_t  UdpComm;

} JSON_GW_Class_t;


/*******************/
/** Exported Data **/
/*******************/

extern JSON_GW_Class_t  JsonGw;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: JSON_AppMain
**
*/
void JSON_GW_AppMain(void);


#endif /* _json_gw_app_ */
