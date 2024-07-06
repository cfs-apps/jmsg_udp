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

#ifndef _jmsg_udp_
#define _jmsg_udp_

/*
** Includes
*/

#include "app_cfg.h"
#include "jmsg_trans.h"
#include "jmsg_topic_tbl.h"

/***********************/
/** Macro Definitions **/
/***********************/


/*
** Event Message IDs
*/

#define JMSG_UDP_CONSTRUCTOR_EID             (JMSG_UDP_BASE_EID + 0)
#define JMSG_UDP_CONFIG_SUBSCRIPTIONS_EID    (JMSG_UDP_BASE_EID + 1)
#define JMSG_UDP_RX_CHILD_TASK_EID           (JMSG_UDP_BASE_EID + 2)
#define JMSG_UDP_SUBSCRIBE_TOPIC_PLUGIN_EID  (JMSG_UDP_BASE_EID + 3)


/**********************/
/** Type Definitions **/
/**********************/


typedef struct
{

   bool            Connected;   
   osal_id_t       SocketId;
   OS_SockAddr_t   SocketAddr;
   char            Buffer[JMSG_UDP_BUF_LEN];  
   uint32          MsgCnt;
   uint32          MsgErrCnt;
   
} JMSG_UDP_Socket_t;


typedef struct
{

   const INITBL_Class_t  *IniTbl; 

   JMSG_UDP_Socket_t Rx;
   JMSG_UDP_Socket_t Tx;
   
   CFE_SB_PipeId_t   JMsgPipe;
      
   JMSG_TRANS_Class_t JMsgTrans;
   
} JMSG_UDP_Class_t;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: JMSG_UDP_Constructor
**
** Initialize the UDP Comm object
**
** Notes:
**   1. This must be called prior to any other member functions.
**
*/
void JMSG_UDP_Constructor(JMSG_UDP_Class_t *UdpMgrPtr, const INITBL_Class_t *IniTbl);


/******************************************************************************
** Function: JMSG_UDP_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
*/
void JMSG_UDP_ResetStatus(void);


/******************************************************************************
** Function: JMSG_UDP_RxChildTask
**
*/
bool JMSG_UDP_RxChildTask(CHILDMGR_Class_t *ChildMgr);


/******************************************************************************
** Function: JMSG_UDP_SubscribeToTopicPlugin
**
*/
bool JMSG_UDP_SubscribeToTopicPlugin(const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: JMSG_UDP_TxChildTask
**
*/
bool JMSG_UDP_TxChildTask(CHILDMGR_Class_t *ChildMgr);



#endif /* _jmsg_udp_ */
