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
void JMSG_UDP_Constructor(JMSG_UDP_Class_t *JMsgUdpPtr, const INITBL_Class_t *IniTbl)
{

   int32  Status;

   JMsgUdp = JMsgUdpPtr;
   
   memset(JMsgUdp, 0, sizeof(JMSG_UDP_Class_t));
   
   JMsgUdp->IniTbl = IniTbl;

   /* Construct contained objects */
   
   JMSG_TRANS_Constructor(&JMsgUdp->JMsgTrans);
 
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
         CFE_EVS_SendEvent(JMSG_UDP_CONSTRUCTOR_EID, CFE_EVS_EventType_DEBUG, 
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

   /* Create Tx socket */
      
   Status = OS_SocketOpen(&JMsgUdp->Tx.SocketId, OS_SocketDomain_INET, OS_SocketType_DATAGRAM);
   if (Status == OS_SUCCESS)
   {
      uint32 TxPort = INITBL_GetIntConfig(INITBL_OBJ, CFG_TX_UDP_PORT);
      OS_SocketAddrInit(&JMsgUdp->Tx.SocketAddr, OS_SocketDomain_INET);
      OS_SocketAddrFromString(&JMsgUdp->Tx.SocketAddr,INITBL_GetStrConfig(INITBL_OBJ, CFG_TX_UDP_ADDR));
      OS_SocketAddrSetPort(&JMsgUdp->Tx.SocketAddr,TxPort);

      JMsgUdp->Tx.Connected = true;
      CFE_EVS_SendEvent(JMSG_UDP_CONSTRUCTOR_EID, CFE_EVS_EventType_INFORMATION, 
                           "Initialized UDP Tx port %u", (unsigned int)TxPort);

   } /* Socket opened */
   else
   {
      CFE_EVS_SendEvent(JMSG_UDP_CONSTRUCTOR_EID, CFE_EVS_EventType_ERROR, 
                        "Error creating JMSG UDP Gateway Tx socket, status = %d", (int)Status);
   }

   CFE_SB_CreatePipe(&JMsgUdp->JMsgPipe, INITBL_GetIntConfig(IniTbl, CFG_JMSG_PIPE_DEPTH), INITBL_GetStrConfig(IniTbl, CFG_JMSG_PIPE_NAME));  

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
** Function: JMSG_UDP_SubscribeToTopicPlugin
**
*/
bool JMSG_UDP_SubscribeToTopicPlugin(const CFE_MSG_Message_t *MsgPtr)
{
   const JMSG_LIB_TopicSubscribeTlm_Payload_t *TopicSubscribe = CMDMGR_PAYLOAD_PTR(MsgPtr, JMSG_LIB_TopicSubscribeTlm_t);
   bool RetStatus = true;

   if (TopicSubscribe->Protocol == JMSG_LIB_TopicProtocol_UDP)
   {
      JMSG_TOPIC_TBL_SubscriptionOptEnum_t SubscriptionOpt;

      RetStatus = JMSG_TOPIC_TBL_RegisterConfigSubscriptionCallback(TopicSubscribe->Id, ConfigSubscription);      
     
      if (RetStatus)
      {
         
         const JMSG_TOPIC_TBL_Topic_t *Topic = JMSG_TOPIC_TBL_GetTopic(TopicSubscribe->Id);
         
         SubscriptionOpt = JMSG_TOPIC_TBL_SubscribeToTopicMsg(TopicSubscribe->Id, JMSG_TOPIC_TBL_SUB_TO_ROLE);
         if (SubscriptionOpt == JMSG_TOPIC_TBL_SUB_ERR)
         {
            RetStatus = false;
            CFE_EVS_SendEvent(JMSG_UDP_SUBSCRIBE_TOPIC_PLUGIN_EID, CFE_EVS_EventType_ERROR, 
                              "Error subscribing to topic Id: %d, Name: %s, cFE Msg: 0x%04X(%d)", 
                              TopicSubscribe->Id, Topic->Name, Topic->Cfe, Topic->Cfe);
         }
         else
         {
            CFE_EVS_SendEvent(JMSG_UDP_SUBSCRIBE_TOPIC_PLUGIN_EID, CFE_EVS_EventType_INFORMATION, 
                              "Successfully subscribed to topic Id: %d, Name: %s, cFE Msg: 0x%04X(%d)", 
                              TopicSubscribe->Id, Topic->Name, Topic->Cfe, Topic->Cfe);
         }
         
      } /* End if registered */
   
   } /* End if UDP protocol */
   
   return RetStatus;
   
} /* End JMSG_UDP_SubscribeToTopicPlugin() */


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
   int32  Status;
   CFE_SB_Buffer_t  *SbBufPtr;
   const char *Topic;
   const char *Payload;
   
   while (true)
   {
      memset(JMsgUdp->Tx.Buffer, 0, JMSG_UDP_BUF_LEN);
      Status = CFE_SB_ReceiveBuffer(&SbBufPtr, JMsgUdp->JMsgPipe, CFE_SB_PEND_FOREVER);

      if (Status == CFE_SUCCESS)
      {
         if (JMSG_TRANS_ProcessSbMsg(&SbBufPtr->Msg, &Topic, &Payload))
         {
            //TODO: Create buffer & compute message size
            strcpy(JMsgUdp->Tx.Buffer,Topic);
            strcat(JMsgUdp->Tx.Buffer,":");
            strcat(JMsgUdp->Tx.Buffer,Payload);            
            Status = OS_SocketSendTo(JMsgUdp->Tx.SocketId, JMsgUdp->Tx.Buffer, sizeof(JMsgUdp->Tx.Buffer), &JMsgUdp->Tx.SocketAddr);
            JMsgUdp->Tx.MsgCnt++;         
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
** Notes:
**   1. A SB duplicate subscription event message will be sent if two 
**      subscription requests are made without an unsubscribe requests
**      between them.
**
*/
static bool ConfigSubscription(const JMSG_TOPIC_TBL_Topic_t *Topic, 
                               JMSG_TOPIC_TBL_SubscriptionOptEnum_t ConfigOpt)
{

   bool RetStatus = true;
   int32 SbStatus;
   CFE_SB_Qos_t Qos;
   
   switch (ConfigOpt)
   {

      case JMSG_TOPIC_TBL_SUB_SB:
         Qos.Priority    = 0;
         Qos.Reliability = 0;
         SbStatus = CFE_SB_SubscribeEx(CFE_SB_ValueToMsgId(Topic->Cfe), JMsgUdp->JMsgPipe, Qos, 20);
         if (SbStatus == CFE_SUCCESS)
         {
            CFE_EVS_SendEvent(JMSG_UDP_CONFIG_SUBSCRIPTIONS_EID, CFE_EVS_EventType_INFORMATION, 
                              "Subscribed to SB for topic 0x%04X(%d)", Topic->Cfe, Topic->Cfe);
         }
         else
         {
            RetStatus = false;
            CFE_EVS_SendEvent(JMSG_UDP_CONFIG_SUBSCRIPTIONS_EID, CFE_EVS_EventType_ERROR, 
                              "Error subscribing to SB for topic 0x%04X(%d)", Topic->Cfe, Topic->Cfe);
         }
         break;
         
      case JMSG_TOPIC_TBL_SUB_JMSG:
         CFE_EVS_SendEvent(JMSG_UDP_CONFIG_SUBSCRIPTIONS_EID, CFE_EVS_EventType_INFORMATION, 
                           "Listening for topic %s", Topic->Name);
         break;
         
      case JMSG_TOPIC_TBL_UNSUB_SB:
         SbStatus = CFE_SB_Unsubscribe(CFE_SB_ValueToMsgId(Topic->Cfe), JMsgUdp->JMsgPipe);
         if(SbStatus == CFE_SUCCESS)
         {
            CFE_EVS_SendEvent(JMSG_UDP_CONFIG_SUBSCRIPTIONS_EID, CFE_EVS_EventType_INFORMATION, 
                              "Unsubscribed from SB for topic %s", Topic->Name);
         }
         else
         {
            RetStatus = false;
            CFE_EVS_SendEvent(JMSG_UDP_CONFIG_SUBSCRIPTIONS_EID, CFE_EVS_EventType_ERROR, 
                              "Error unsubscribing from SB for topic %s, status = %d", Topic->Name, SbStatus);            
         }
         break;
      
      case JMSG_TOPIC_TBL_UNSUB_JMSG:
         CFE_EVS_SendEvent(JMSG_UDP_CONFIG_SUBSCRIPTIONS_EID, CFE_EVS_EventType_INFORMATION, 
                           "Nolonger expecting topic %s", Topic->Name);
         break;
      
      default:
         RetStatus = false;
         CFE_EVS_SendEvent(JMSG_UDP_CONFIG_SUBSCRIPTIONS_EID, CFE_EVS_EventType_ERROR, 
                           "Invalid subscription option for topic %s", Topic->Name);
         break;

   } /** End switch */

   return RetStatus;
   
} /* End ConfigSubscription() */
