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

#include "jmsg_udp.h"


/***********************/
/** Macro Definitions **/
/***********************/

/* Convenience macros */
#define  INITBL_OBJ   (JMsgUdp->IniTbl)

/**********************/
/** Type Definitions **/
/**********************/


/********************************** **/
/** Local File Function Prototypes **/
/************************************/

static bool ConfigSubscription(const JMSG_TOPIC_TBL_Topic_t *Topic, 
                               JMSG_TOPIC_TBL_SubscriptionOptEnum_t ConfigOpt);
static void TestChildTask(void);

/*****************/
/** Global Data **/
/*****************/

static JMSG_UDP_Class_t *JMsgUdp;


/******************************************************************************
** Function: JMSG_UDP_Constructor
**
** Initialize the UDP Manager object
**
** Notes:
**   1. This must be called prior to any other member functions.
**
*/
void JMSG_UDP_Constructor(JMSG_UDP_Class_t *JMsgUdpPtr, const INITBL_Class_t *IniTbl,
                         TBLMGR_Class_t *TblMgr)
{

   int32  Status;

   JMsgUdp = JMsgUdpPtr;
   
   memset(JMsgUdp, 0, sizeof(JMSG_UDP_Class_t));
   
   JMsgUdp->IniTbl = IniTbl;

   /* Construct contained objects */
   
   JMSG_TRANS_Constructor(&JMsgUdp->JMsgTrans);
 
   JMSG_TOPIC_TBL_RegisterConfigSubscriptionCallback(ConfigSubscription);
                              
   TBLMGR_RegisterTblWithDef(TblMgr, JMSG_TOPIC_TBL_NAME, 
                             JMSG_TOPIC_TBL_LoadCmd, JMSG_TOPIC_TBL_DumpCmd,  
                             INITBL_GetStrConfig(IniTbl, CFG_JMSG_TOPIC_TBL_FILE));   
   /* Create Rx socket */

   Status = OS_SocketOpen(&JMsgUdp->Rx.SocketId, OS_SocketDomain_INET, OS_SocketType_DATAGRAM);
   if (Status == OS_SUCCESS)
   {

      uint32 RxPort = INITBL_GetIntConfig(INITBL_OBJ, CFG_RX_UDP_PORT);
      OS_SocketAddrInit(&JMsgUdp->Rx.SocketAddr, OS_SocketDomain_INET);
      OS_SocketAddrSetPort(&JMsgUdp->Rx.SocketAddr, RxPort);

      Status = OS_SocketBind(JMsgUdp->Rx.SocketId, &JMsgUdp->Rx.SocketAddr);

      if (Status == OS_SUCCESS)
      {
         JMsgUdp->Rx.Connected = true;
         CFE_EVS_SendEvent(JMSG_UDP_CONSTRUCTOR_EID, CFE_EVS_EventType_INFORMATION, 
                           "JMSG UDP Gateway listening on UDP port %u", (unsigned int)RxPort);
      }
      else
      {
         CFE_EVS_SendEvent(JMSG_UDP_CONSTRUCTOR_EID, CFE_EVS_EventType_ERROR, 
                           "Error binding JMSG UDP Gateway Rx socket, status = %d", (int)Status);
      }
        
   } /* Socket opened */
   else
   {
      CFE_EVS_SendEvent(JMSG_UDP_CONSTRUCTOR_EID, CFE_EVS_EventType_ERROR, 
                        "Error creating JMSG UDP Gateway Rx socket, status = %d", (int)Status);
   }

   /* TODO: Add Tx logic
   CFE_SB_CreatePipe(&CI_LAB_Global.CommandPipe, CI_LAB_PIPE_DEPTH, "CI_LAB_CMD_PIPE");
   CFE_SB_Subscribe(CFE_SB_ValueToMsgId(CI_LAB_CMD_MID), CI_LAB_Global.CommandPipe);
   */
   
   JMsgUdp->JMsgTestMid = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(IniTbl, CFG_JMSG_TEST_PLUGIN_TOPICID));   
   CFE_SB_CreatePipe(&JMsgUdp->JMsgPipe, INITBL_GetIntConfig(IniTbl, CFG_JMSG_PIPE_DEPTH), INITBL_GetStrConfig(IniTbl, CFG_JMSG_PIPE_NAME));  
   CFE_SB_Subscribe(JMsgUdp->JMsgTestMid, JMsgUdp->JMsgPipe);

} /* End JMSG_UDP_Constructor() */


/******************************************************************************
** Function: JMSG_UDP_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
*/
void JMSG_UDP_ResetStatus(void)
{

   JMsgUdp->Rx.MsgCnt    = 0;
   JMsgUdp->Rx.MsgErrCnt = 0;
   JMsgUdp->Tx.MsgCnt    = 0;
   JMsgUdp->Tx.MsgErrCnt = 0;

} /* End JMSG_UDP_ResetStatus() */


/******************************************************************************
** Function: JMSG_UDP_RxChildTask
**
*/
bool JMSG_UDP_RxChildTask(CHILDMGR_Class_t *ChildMgr)
{

   int32 Status;
   
   if (JMsgUdp->Rx.Connected)
   {

      Status = OS_SocketRecvFrom(JMsgUdp->Rx.SocketId, JMsgUdp->Rx.Buffer,
                                 sizeof(JMsgUdp->Rx.Buffer), &JMsgUdp->Rx.SocketAddr, OS_PEND);
              
      if (Status >= 0)
      {
         JMsgUdp->Rx.MsgCnt++;
         CFE_EVS_SendEvent(JMSG_UDP_RX_CHILD_TASK_EID, CFE_EVS_EventType_INFORMATION, 
                           "JMSG UDP Gateway Rx received message: %s", JMsgUdp->Rx.Buffer);
         JMSG_TRANS_ProcessJMsg(JMsgUdp->Rx.Buffer);
      }
      else
      {
         JMsgUdp->Rx.MsgErrCnt++;
         CFE_EVS_SendEvent(JMSG_UDP_RX_CHILD_TASK_EID, CFE_EVS_EventType_ERROR, 
                           "JMSG UDP Gateway Rx socket receive error, Status = %u", (unsigned int)Status);
      }
      
   } /* End if connected */
   else
   {
      OS_TaskDelay(2000);
   }
      
   return true;
   
} /* End JMSG_UDP_RxChildTask() */


/******************************************************************************
** Function: JMSG_UDP_TxChildTask
**
** Notes:
**   1. TODO: This is a first cut. Think through loop control options. 
**
*/
bool JMSG_UDP_TxChildTask(CHILDMGR_Class_t *ChildMgr)
{

   int32  RetStatus = true;
   int32  SbStatus;
   CFE_SB_Buffer_t  *SbBufPtr;
   const char *JMsgStr;
   
   while (true)
   {
      SbStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, JMsgUdp->JMsgPipe, CFE_SB_PEND_FOREVER);

      if (SbStatus == CFE_SUCCESS)
      {
         if (JMSG_TRANS_ProcessSbMsg(&SbBufPtr->Msg,&JMsgStr))
         {
            JMsgUdp->Tx.MsgCnt++; //TODO: Replace with real logic
         }
      }
      
   } /* End while loop */
   
   return RetStatus;
   
} /* End JMSG_UDP_TxChildTask() */


/******************************************************************************
** Function: ConfigSubscription
**
** Callback function that is called when a topic plugin's configuration
** is changed. Perform functions that apply to the network layer.
**
*/
static bool ConfigSubscription(const JMSG_TOPIC_TBL_Topic_t *Topic, 
                               JMSG_TOPIC_TBL_SubscriptionOptEnum_t ConfigOpt)
{

   bool RetStatus = true;
   
   switch (ConfigOpt)
   {

      case JMSG_TOPIC_TBL_SUB_SB:
         // CFE_SB_Qos_t Qos;
         // Qos.Priority    = 0;
         // Qos.Reliability = 0;
         // TODO: CFE_SB_SubscribeEx(CFE_SB_ValueToMsgId(Topic->Cfe), MqttMgr->TopicPipe, Qos, 20);
         CFE_EVS_SendEvent(JMSG_TRANS_CONFIG_SUBSCRIPTIONS_EID, CFE_EVS_EventType_INFORMATION, 
                           "JMSG_TOPIC_TBL_SUB_SB for topic %s", Topic->Name);
         break;
         
      case JMSG_TOPIC_TBL_SUB_JMSG:
         CFE_EVS_SendEvent(JMSG_TRANS_CONFIG_SUBSCRIPTIONS_EID, CFE_EVS_EventType_INFORMATION, 
                           "JMSG_TOPIC_TBL_SUB_JMSG for topic %s", Topic->Name);
         break;
         
      case JMSG_TOPIC_TBL_UNSUB_SB:
         // TODO: SbStatus = CFE_SB_Unsubscribe(CFE_SB_ValueToMsgId(Topic->Cfe), MqttMgr->TopicPipe);
         // TODO: if(SbStatus == CFE_SUCCESS)
         CFE_EVS_SendEvent(JMSG_TRANS_CONFIG_SUBSCRIPTIONS_EID, CFE_EVS_EventType_INFORMATION, 
                           "JMSG_TOPIC_TBL_UNSUB_SB for topic %s", Topic->Name);
         break;
      
      case JMSG_TOPIC_TBL_UNSUB_JMSG:
         CFE_EVS_SendEvent(JMSG_TRANS_CONFIG_SUBSCRIPTIONS_EID, CFE_EVS_EventType_INFORMATION, 
                           "JMSG_TOPIC_TBL_UNSUB_JMSG for topic %s", Topic->Name);
         break;
      
      default:
         RetStatus = false;
         CFE_EVS_SendEvent(JMSG_TRANS_CONFIG_SUBSCRIPTIONS_EID, CFE_EVS_EventType_ERROR, 
                           "Invalid subscription option for topic %s", Topic->Name);
         break;

   } /** End switch */

   return RetStatus;
   
} /* End ConfigSubscription() */


/******************************************************************************
** Function: JMSG_UDP_StartTestCmd
**
** Notes:
**   1. Signature must match CMDMGR_CmdFuncPtr_t
**   2. DataObjPtr is not used
*/
bool JMSG_UDP_StartTestCmd(void* DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   JMsgUdp->TestActive = true;
   JMsgUdp->TestId     = JMSG_USR_TopicPlugin_TEST;
   JMsgUdp->TestParam = 0;
   
   CFE_ES_CreateChildTask(&JMsgUdp->TestChildTaskId, "JMSG_UDP Test",
                          TestChildTask, 0, 16384, 80, 0);
   
   return true;
   
} /* JMSG_UDP_StartTestCmd() */


/******************************************************************************
** Function: JMSG_UDP_StopTestCmd
**
** Notes:
**   1. Signature must match CMDMGR_CmdFuncPtr_t
**   2. DataObjPtr is not used
*/
bool JMSG_UDP_StopTestCmd(void* DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   JMsgUdp->TestActive = false;
   return true;
   
} /* JMSG_UDP_StopTestCmd() */


/******************************************************************************
** Function: TestChildTask
**
*/
static void TestChildTask(void)
{

   while (JMsgUdp->TestActive)
   {
      JMSG_TOPIC_TBL_RunSbMsgTest(JMsgUdp->TestId, false, JMsgUdp->TestParam);
      OS_TaskDelay(2000);
   }

   
} /* End JMSG_UDP_TxChildTask() */
   
   
