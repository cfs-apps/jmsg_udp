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
**   Manage UDP Tx and Rx interfaces
**
** Notes:
**   None
**
*/

/*
** Includes
*/

#include "udp_comm.h"


/***********************/
/** Macro Definitions **/
/***********************/

/* Convenience macros */
#define  INITBL_OBJ   (UdpComm->IniTbl)

/**********************/
/** Type Definitions **/
/**********************/


/*******************************/
/** Local Function Prototypes **/
/*******************************/


/*****************/
/** Global Data **/
/*****************/

static UDP_COMM_Class_t *UdpComm;


/******************************************************************************
** Function: UDP_COMM_Constructor
**
** Initialize the UDP Comm object
**
** Notes:
**   1. This must be called prior to any other member functions.
**
*/
void UDP_COMM_Constructor(UDP_COMM_Class_t *UdpCommPtr, const INITBL_Class_t *IniTbl)
{

   int32  Status;

   UdpComm = UdpCommPtr;
   
   memset(UdpComm, 0, sizeof(UDP_COMM_Class_t));
   
   UdpComm->IniTbl = IniTbl;

   /* Create Rx socket */

   Status = OS_SocketOpen(&UdpComm->Rx.SocketId, OS_SocketDomain_INET, OS_SocketType_DATAGRAM);
   if (Status == OS_SUCCESS)
   {

      uint32 RxPort = INITBL_GetIntConfig(INITBL_OBJ, CFG_RX_UDP_PORT);
      OS_SocketAddrInit(&UdpComm->Rx.SocketAddr, OS_SocketDomain_INET);
      OS_SocketAddrSetPort(&UdpComm->Rx.SocketAddr, RxPort);

      Status = OS_SocketBind(UdpComm->Rx.SocketId, &UdpComm->Rx.SocketAddr);

      if (Status == OS_SUCCESS)
      {
         UdpComm->Rx.Connected = true;
         CFE_EVS_SendEvent(UDP_COMM_CONSTRUCTOR_EID, CFE_EVS_EventType_INFORMATION, 
                           "JSON_GW listening on UDP port %u", (unsigned int)RxPort);
      }
      else
      {
         CFE_EVS_SendEvent(UDP_COMM_CONSTRUCTOR_EID, CFE_EVS_EventType_ERROR, 
                           "Error binding JSON_GW Rx socket, status = %d", (int)Status);
      }
        
   } /* Socket opened */
   else
   {
      CFE_EVS_SendEvent(UDP_COMM_CONSTRUCTOR_EID, CFE_EVS_EventType_ERROR, 
                        "Error creating JSON_GW Rx socket, status = %d", (int)Status);
   }

   /* TODO: Add Tx logic
   CFE_SB_CreatePipe(&CI_LAB_Global.CommandPipe, CI_LAB_PIPE_DEPTH, "CI_LAB_CMD_PIPE");
   CFE_SB_Subscribe(CFE_SB_ValueToMsgId(CI_LAB_CMD_MID), CI_LAB_Global.CommandPipe);
   */
   
} /* End UDP_COMM_Constructor() */


/******************************************************************************
** Function: UDP_COMM_RxChildTask
**
*/
bool UDP_COMM_RxChildTask(CHILDMGR_Class_t *ChildMgr)
{

   int32 Status;
   
   if (UdpComm->Rx.Connected)
   {

      Status = OS_SocketRecvFrom(UdpComm->Rx.SocketId, UdpComm->Rx.Buffer,
                                 sizeof(UdpComm->Rx.Buffer), &UdpComm->Rx.SocketAddr, OS_PEND);
              
      if (Status >= 0)
      {
         UdpComm->Rx.MsgCnt++;
         CFE_EVS_SendEvent(UDP_COMM_RX_CHILD_TASK_EID, CFE_EVS_EventType_INFORMATION, 
                           "JSON_GW Rx received message: %s", UdpComm->Rx.Buffer);
      }
      else
      {
         UdpComm->Rx.MsgErrCnt++;
         CFE_EVS_SendEvent(UDP_COMM_RX_CHILD_TASK_EID, CFE_EVS_EventType_ERROR, 
                           "JSON GW Rx socket receive error, Status = %u", (unsigned int)Status);
      }
      
   } /* End if connected */
   else
   {
      OS_TaskDelay(2000);
   }
      
   return true;
   
} /* End UDP_COMM_RxChildTask() */


/******************************************************************************
** Function: UDP_COMM_TxChildTask
**
*/
bool UDP_COMM_TxChildTask(CHILDMGR_Class_t *ChildMgr)
{

   UdpComm->Tx.MsgCnt++; //TODO: Replace with real logic
   OS_TaskDelay(2000);
   
   return true;
   
} /* End UDP_COMM_TxChildTask() */


/******************************************************************************
** Function: UDP_COMM_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
*/
void UDP_COMM_ResetStatus(void)
{

   UdpComm->Rx.MsgCnt    = 0;
   UdpComm->Rx.MsgErrCnt = 0;
   UdpComm->Tx.MsgCnt    = 0;
   UdpComm->Tx.MsgErrCnt = 0;

} /* End UDP_COMM_ResetStatus() */

