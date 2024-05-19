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
**   Manage a JSON gateway between the cFS Software Bus and a UDP socket.
**
** Notes:
**   None
**
*/

/*
** Includes
*/

#include <string.h>
#include "jmsg_udp_app.h"
#include "jmsg_udp_eds_cc.h"

/***********************/
/** Macro Definitions **/
/***********************/

/* Convenience macros */
#define  INITBL_OBJ      (&(JMsgUdp.IniTbl))
#define  CMDMGR_OBJ      (&(JMsgUdp.CmdMgr))
#define  RX_CHILDMGR_OBJ (&(JMsgUdp.RxChildMgr))
#define  TX_CHILDMGR_OBJ (&(JMsgUdp.TxChildMgr))
#define  UDP_COMM_OBJ    (&(JMsgUdp.UdpComm))

/*******************************/
/** Local Function Prototypes **/
/*******************************/

static int32 InitApp(void);
static int32 ProcessCommands(void);
static void SendStatusPkt(void);


/**********************/
/** File Global Data **/
/**********************/

/* 
** Must match DECLARE ENUM() declaration in app_cfg.h
** Defines "static INILIB_CfgEnum IniCfgEnum"
*/
DEFINE_ENUM(Config,APP_CONFIG)  

static CFE_EVS_BinFilter_t  EventFilters[] =
{  
   /* Event ID                           Mask */
   {UDP_COMM_RX_CHILD_TASK_EID, CFE_EVS_NO_FILTER}
};

/*****************/
/** Global Data **/
/*****************/

JMSG_UDP_Class_t   JMsgUdp;


/******************************************************************************
** Function: JMSG_UDP_AppMain
**
*/
void JMSG_UDP_AppMain(void)
{

   uint32 RunStatus = CFE_ES_RunStatus_APP_ERROR;
   
   CFE_EVS_Register(EventFilters, sizeof(EventFilters)/sizeof(CFE_EVS_BinFilter_t),
                    CFE_EVS_EventFilter_BINARY);

   if (InitApp() == CFE_SUCCESS)      /* Performs initial CFE_ES_PerfLogEntry() call */
   {
      RunStatus = CFE_ES_RunStatus_APP_RUN; 
   }
   
   /*
   ** Main process loop
   */
   while (CFE_ES_RunLoop(&RunStatus))
   {
      
      /*
      ** The Rx and Tx child tasks manage translating and transferring the JSON
      ** messages. This loop only needs to service commands.
      */ 
      
      RunStatus = ProcessCommands();
      
   } /* End CFE_ES_RunLoop */

   CFE_ES_WriteToSysLog("JSON Gateway App terminating, run status = 0x%08X\n", RunStatus);   /* Use SysLog, events may not be working */

   CFE_EVS_SendEvent(JMSG_UDP_EXIT_EID, CFE_EVS_EventType_CRITICAL, "JMSG UDP Gateway App terminating, run status = 0x%08X", RunStatus);

   CFE_ES_ExitApp(RunStatus);  /* Let cFE kill the task (and any child tasks) */


} /* End of JMSG_UDP_AppMain() */



/******************************************************************************
** Function: JMSG_UDP_NoOpCmd
**
*/

bool JMSG_UDP_NoOpCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   CFE_EVS_SendEvent (JMSG_UDP_NOOP_EID, CFE_EVS_EventType_INFORMATION,
                      "No operation command received for JMSG UDP Gateway App version %d.%d.%d",
                      JMSG_UDP_MAJOR_VER, JMSG_UDP_MINOR_VER, JMSG_UDP_PLATFORM_REV);

   return true;


} /* End JMSG_UDP_NoOpCmd() */


/******************************************************************************
** Function: JMSG_UDP_ResetAppCmd
**
*/

bool JMSG_UDP_ResetAppCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   CFE_EVS_ResetAllFilters();

   CMDMGR_ResetStatus(CMDMGR_OBJ);
   CHILDMGR_ResetStatus(RX_CHILDMGR_OBJ);
   CHILDMGR_ResetStatus(TX_CHILDMGR_OBJ);
   
   UDP_COMM_ResetStatus();
	  
   return true;

} /* End JMSG_UDP_ResetAppCmd() */


/******************************************************************************
** Function: InitApp
**
*/
static int32 InitApp(void)
{

   int32 RetStatus = APP_C_FW_CFS_ERROR;
   
   CHILDMGR_TaskInit_t ChildTaskInit;


   /*
   ** Read JSON INI Table & class variable defaults defined in JSON  
   */

   if (INITBL_Constructor(INITBL_OBJ, JMSG_UDP_INI_FILENAME, &IniCfgEnum))
   {
   
      JMsgUdp.PerfId = INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_MAIN_PERF_ID);
      CFE_ES_PerfLogEntry(JMsgUdp.PerfId);

      UDP_COMM_Constructor(UDP_COMM_OBJ, INITBL_OBJ);

      JMsgUdp.CmdMid        = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_JMSG_UDP_CMD_TOPICID));
      JMsgUdp.SendStatusMid = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_SEND_STATUS_TLM_TOPICID));
   
      /* Child Manager constructor sends error events */

      ChildTaskInit.TaskName  = INITBL_GetStrConfig(INITBL_OBJ, CFG_RX_CHILD_NAME);
      ChildTaskInit.StackSize = INITBL_GetIntConfig(INITBL_OBJ, CFG_RX_CHILD_STACK_SIZE);
      ChildTaskInit.Priority  = INITBL_GetIntConfig(INITBL_OBJ, CFG_RX_CHILD_PRIORITY);
      ChildTaskInit.PerfId    = INITBL_GetIntConfig(INITBL_OBJ, CFG_RX_CHILD_PERF_ID);
      RetStatus = CHILDMGR_Constructor(RX_CHILDMGR_OBJ, ChildMgr_TaskMainCallback,
                                       UDP_COMM_RxChildTask, &ChildTaskInit); 

      ChildTaskInit.TaskName  = INITBL_GetStrConfig(INITBL_OBJ, CFG_TX_CHILD_NAME);
      ChildTaskInit.StackSize = INITBL_GetIntConfig(INITBL_OBJ, CFG_TX_CHILD_STACK_SIZE);
      ChildTaskInit.Priority  = INITBL_GetIntConfig(INITBL_OBJ, CFG_TX_CHILD_PRIORITY);
      ChildTaskInit.PerfId    = INITBL_GetIntConfig(INITBL_OBJ, CFG_TX_CHILD_PERF_ID);
      RetStatus = CHILDMGR_Constructor(TX_CHILDMGR_OBJ, ChildMgr_TaskMainCallback,
                                       UDP_COMM_TxChildTask, &ChildTaskInit); 
      
      /*
      ** Initialize app level interfaces
      */
 
      CFE_SB_CreatePipe(&JMsgUdp.CmdPipe, INITBL_GetIntConfig(INITBL_OBJ, CFG_CMD_PIPE_DEPTH), INITBL_GetStrConfig(INITBL_OBJ, CFG_CMD_PIPE_NAME));  
      CFE_SB_Subscribe(JMsgUdp.CmdMid, JMsgUdp.CmdPipe);
      CFE_SB_Subscribe(JMsgUdp.SendStatusMid, JMsgUdp.CmdPipe);

      CMDMGR_Constructor(CMDMGR_OBJ);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, JMSG_UDP_NOOP_CC,   NULL, JMSG_UDP_NoOpCmd,     0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, JMSG_UDP_RESET_CC,  NULL, JMSG_UDP_ResetAppCmd, 0);
         
      CFE_MSG_Init(CFE_MSG_PTR(JMsgUdp.StatusTlm.TelemetryHeader), CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_JMSG_UDP_STATUS_TLM_TOPICID)), sizeof(JMSG_UDP_StatusTlm_t));

      /*
      ** Application startup event message
      */
      CFE_EVS_SendEvent(JMSG_UDP_INIT_APP_EID, CFE_EVS_EventType_INFORMATION,
                        "JMSG UDP Gateway App Initialized. Version %d.%d.%d",
                        JMSG_UDP_MAJOR_VER, JMSG_UDP_MINOR_VER, JMSG_UDP_PLATFORM_REV);
                        
   } /* End if INITBL Constructed */
   
   return RetStatus;

} /* End of InitApp() */


/******************************************************************************
** Function: ProcessCommands
**
** 
*/
static int32 ProcessCommands(void)
{
   
   int32  RetStatus = CFE_ES_RunStatus_APP_RUN;
   int32  SysStatus;

   CFE_SB_Buffer_t  *SbBufPtr;
   CFE_SB_MsgId_t   MsgId = CFE_SB_INVALID_MSG_ID;


   CFE_ES_PerfLogExit(JMsgUdp.PerfId);
   SysStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, JMsgUdp.CmdPipe, CFE_SB_PEND_FOREVER);
   CFE_ES_PerfLogEntry(JMsgUdp.PerfId);

   if (SysStatus == CFE_SUCCESS)
   {
      SysStatus = CFE_MSG_GetMsgId(&SbBufPtr->Msg, &MsgId);

      if (SysStatus == CFE_SUCCESS)
      {

         if (CFE_SB_MsgId_Equal(MsgId, JMsgUdp.CmdMid))
         {
            CMDMGR_DispatchFunc(CMDMGR_OBJ, &SbBufPtr->Msg);
         } 
         else if (CFE_SB_MsgId_Equal(MsgId, JMsgUdp.SendStatusMid))
         {   
            SendStatusPkt();
         }
         else
         {   
            CFE_EVS_SendEvent(JMSG_UDP_INVALID_MID_EID, CFE_EVS_EventType_ERROR,
                              "Received invalid command packet, MID = 0x%04X(%d)", 
                              CFE_SB_MsgIdToValue(MsgId), CFE_SB_MsgIdToValue(MsgId));
         }

      } /* End if got message ID */
   } /* End if received buffer */
   else
   {
      if (SysStatus != CFE_SB_NO_MESSAGE)
      {
         RetStatus = CFE_ES_RunStatus_APP_ERROR;
      }
   } 

   return RetStatus;
   
} /* End ProcessCommands() */


/******************************************************************************
** Function: SendStatusPkt
**
*/
void SendStatusPkt(void)
{
   
   JMSG_UDP_StatusTlm_Payload_t *Payload = &JMsgUdp.StatusTlm.Payload;

   /*
   ** Framework Data
   */

   Payload->ValidCmdCnt    = JMsgUdp.CmdMgr.ValidCmdCnt;
   Payload->InvalidCmdCnt  = JMsgUdp.CmdMgr.InvalidCmdCnt;

   /*
   ** UDP Comm Data
   */

   Payload->RxUdpConnected = JMsgUdp.UdpComm.Rx.Connected;
   Payload->RxUdpMsgCnt    = JMsgUdp.UdpComm.Rx.MsgCnt;
   Payload->RxUdpMsgErrCnt = JMsgUdp.UdpComm.Rx.MsgErrCnt;
   
   Payload->TxUdpConnected = JMsgUdp.UdpComm.Tx.Connected;
   Payload->TxUdpMsgCnt    = JMsgUdp.UdpComm.Tx.MsgCnt;
   Payload->TxUdpMsgErrCnt = JMsgUdp.UdpComm.Tx.MsgErrCnt;

   CFE_SB_TimeStampMsg(CFE_MSG_PTR(JMsgUdp.StatusTlm.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(JMsgUdp.StatusTlm.TelemetryHeader), true);

} /* End SendStatusPkt() */