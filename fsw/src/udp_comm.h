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
**   Manage UDP communications and JSON-SB message translations 
**
** Notes:
**   TODO: Resolve MQTT_GW topic translation dependency
**
*/

#ifndef _udp_comm_
#define _udp_comm_

/*
** Includes
*/

#include "app_cfg.h"


/***********************/
/** Macro Definitions **/
/***********************/


/*
** Event Message IDs
*/

#define UDP_COMM_CONSTRUCTOR_EID    (UDP_COMM_BASE_EID + 0)
#define UDP_COMM_RX_CHILD_TASK_EID  (UDP_COMM_BASE_EID + 1)


/**********************/
/** Type Definitions **/
/**********************/


typedef struct
{

   bool            Connected;   
   osal_id_t       SocketId;
   OS_SockAddr_t   SocketAddr;
   char            Buffer[UDP_COMM_BUF_LEN];
   uint32          MsgCnt;
   uint32          MsgErrCnt;
   
} UDP_COMM_Socket_t;


typedef struct
{

   const INITBL_Class_t  *IniTbl; 

   UDP_COMM_Socket_t Rx;
   UDP_COMM_Socket_t Tx;
      
} UDP_COMM_Class_t;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: UDP_COMM_Constructor
**
** Initialize the UDP Comm object
**
** Notes:
**   1. This must be called prior to any other member functions.
**
*/
void UDP_COMM_Constructor(UDP_COMM_Class_t *UdpCommPtr, const INITBL_Class_t *IniTbl);


/******************************************************************************
** Function: UDP_COMM_RxChildTask
**
*/
bool UDP_COMM_RxChildTask(CHILDMGR_Class_t *ChildMgr);


/******************************************************************************
** Function: UDP_COMM_TxChildTask
**
*/
bool UDP_COMM_TxChildTask(CHILDMGR_Class_t *ChildMgr);


/******************************************************************************
** Function: UDP_COMM_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
*/
void UDP_COMM_ResetStatus(void);


#endif /* _udp_comm_ */
