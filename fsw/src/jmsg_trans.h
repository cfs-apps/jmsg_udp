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
**   Translate between JSON message topics and SB messages
**
** Notes:
**   1. Each supported JSON topic is listed in a JSON file and each
**      topic has a JSON file that defines the topic's content.
**   2. The Basecamp JSON table coding idiom is to use a separate object to manage 
**      the table. Since UDP_COMM has very little functionality beyond
**      processing the table, a single object is used for management functions
**      and table processing.
**
*/
#ifndef _jmsg_trans_
#define _jmsg_trans_

/*
** Includes
*/

#include "app_cfg.h"


/***********************/
/** Macro Definitions **/
/***********************/


/*
** Events
*/

// TODO: Decide if need separate debug message EIDs that can be filtered
#define JMSG_TRANS_PROCESS_JMSG_EID       (JMSG_TRANS_BASE_EID + 0)
#define JMSG_TRANS_PROCESS_SB_MSG_EID     (JMSG_TRANS_BASE_EID + 1)

/**********************/
/** Type Definitions **/
/**********************/

/*
** JSON Message
*/

typedef struct
{
   
   char   Data[JMSG_UDP_BUF_LEN];

}  JMSG_Payload_t;

typedef struct
{
   
   CFE_MSG_TelemetryHeader_t TelemetryHeader;
   JMSG_Payload_t            Payload;

}  JMSG_Pkt_t;


/*
** Class Definition
*/

typedef struct 
{

   uint32  ValidJMsgCnt;
   uint32  InvalidJMsgCnt;
   uint32  ValidSbMsgCnt;
   uint32  InvalidSbMsgCnt;
   
   /*
   ** Telemetry Messages
   */
   
   JMSG_Pkt_t  JMsgPkt;
   
   /*
   ** Contained Objects
   */


} JMSG_TRANS_Class_t;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: JMSG_TRANS_Constructor
**
** Notes:
**    1. This function must be called prior to any other functions
*/
void JMSG_TRANS_Constructor(JMSG_TRANS_Class_t *JMsgTransPtr);


/******************************************************************************
** Function: JMSG_TRANS_ProcessJMsg
**
** Notes:
**   1. Assumes caller has ensured a null terminated string
**
*/
bool JMSG_TRANS_ProcessJMsg(const char *MsgData);


/******************************************************************************
** Function: JMSG_TRANS_ProcessSbMsg
**
** Notes:
**   None
**
*/
bool JMSG_TRANS_ProcessSbMsg(const CFE_MSG_Message_t *CfeMsgPtr,
                             const char **Topic, const char **Payload);


/******************************************************************************
** Function: JMSG_TRANS_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
*/
void JMSG_TRANS_ResetStatus(void);

#endif /* _msg_trans_ */
