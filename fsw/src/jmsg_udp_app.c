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
#include "jmsg_lib_eds_cc.h"

/***********************/
/** Macro Definitions **/
/***********************/

/* Convenience macros */
#define  INITBL_OBJ      (&(JMsgUdpApp.IniTbl))
#define  CMDMGR_OBJ      (&(JMsgUdpApp.CmdMgr))
#define  TBLMGR_OBJ      (&(JMsgUdpApp.TblMgr))  
#define  RX_CHILDMGR_OBJ (&(JMsgUdpApp.RxChildMgr))
#define  TX_CHILDMGR_OBJ (&(JMsgUdpApp.TxChildMgr))
#define  JMSG_UDP_OBJ    (&(JMsgUdpApp.JMsgUdp))

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
   {JMSG_UDP_RX_CHILD_TASK_EID, CFE_EVS_NO_FILTER}
};

/*****************/
/** Global Data **/
/*****************/

JMSG_UDP_APP_Class_t   JMsgUdpApp;


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

   CFE_ES_WriteToSysLog("JMSG UDP Gateway App terminating, run status = 0x%08X\n", RunStatus);   /* Use SysLog, events may not be working */

   CFE_EVS_SendEvent(JMSG_UDP_APP_EXIT_EID, CFE_EVS_EventType_CRITICAL, "JMSG UDP Gateway App terminating, run status = 0x%08X", RunStatus);

   CFE_ES_ExitApp(RunStatus);  /* Let cFE kill the task (and any child tasks) */


} /* End of JMSG_UDP_AppMain() */



/******************************************************************************
** Function: JMSG_UDP_APP_NoOpCmd
**
*/

bool JMSG_UDP_APP_NoOpCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   CFE_EVS_SendEvent (JMSG_UDP_APP_NOOP_EID, CFE_EVS_EventType_INFORMATION,
                      "No operation command received for JMSG UDP Gateway App version %d.%d.%d",
                      JMSG_UDP_APP_MAJOR_VER, JMSG_UDP_APP_MINOR_VER, JMSG_UDP_APP_PLATFORM_REV);

   return true;


} /* End JMSG_UDP_APP_NoOpCmd() */


/******************************************************************************
** Function: JMSG_UDP_APP_ResetAppCmd
**
*/

bool JMSG_UDP_APP_ResetAppCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   CFE_EVS_ResetAllFilters();

   CMDMGR_ResetStatus(CMDMGR_OBJ);
   CHILDMGR_ResetStatus(RX_CHILDMGR_OBJ);
   CHILDMGR_ResetStatus(TX_CHILDMGR_OBJ);
   
   JMSG_UDP_ResetStatus();
	  
   return true;

} /* End JMSG_UDP_APP_ResetAppCmd() */


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

   if (INITBL_Constructor(INITBL_OBJ, JMSG_UDP_APP_INI_FILENAME, &IniCfgEnum))
   {
   
      JMsgUdpApp.PerfId = INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_MAIN_PERF_ID);
      CFE_ES_PerfLogEntry(JMsgUdpApp.PerfId);

      /* Must construct table manager prior to table objects */
      TBLMGR_Constructor(TBLMGR_OBJ, INITBL_GetStrConfig(INITBL_OBJ, CFG_APP_CFE_NAME));
      JMSG_UDP_Constructor(JMSG_UDP_OBJ, INITBL_OBJ, TBLMGR_OBJ);

      JMsgUdpApp.JMsgLibMid    = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_JMSG_LIB_CMD_TOPICID));
      JMsgUdpApp.CmdMid        = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_JMSG_UDP_CMD_TOPICID));
      JMsgUdpApp.SendStatusMid = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_SEND_STATUS_TLM_TOPICID));
   
      /* Child Manager constructor sends error events */

      ChildTaskInit.TaskName  = INITBL_GetStrConfig(INITBL_OBJ, CFG_RX_CHILD_NAME);
      ChildTaskInit.StackSize = INITBL_GetIntConfig(INITBL_OBJ, CFG_RX_CHILD_STACK_SIZE);
      ChildTaskInit.Priority  = INITBL_GetIntConfig(INITBL_OBJ, CFG_RX_CHILD_PRIORITY);
      ChildTaskInit.PerfId    = INITBL_GetIntConfig(INITBL_OBJ, CFG_RX_CHILD_PERF_ID);
      RetStatus = CHILDMGR_Constructor(RX_CHILDMGR_OBJ, ChildMgr_TaskMainCallback,
                                       JMSG_UDP_RxChildTask, &ChildTaskInit); 

      ChildTaskInit.TaskName  = INITBL_GetStrConfig(INITBL_OBJ, CFG_TX_CHILD_NAME);
      ChildTaskInit.StackSize = INITBL_GetIntConfig(INITBL_OBJ, CFG_TX_CHILD_STACK_SIZE);
      ChildTaskInit.Priority  = INITBL_GetIntConfig(INITBL_OBJ, CFG_TX_CHILD_PRIORITY);
      ChildTaskInit.PerfId    = INITBL_GetIntConfig(INITBL_OBJ, CFG_TX_CHILD_PERF_ID);
      RetStatus = CHILDMGR_Constructor(TX_CHILDMGR_OBJ, ChildMgr_TaskMainCallback,
                                       JMSG_UDP_TxChildTask, &ChildTaskInit); 
      
      /*
      ** Initialize app level interfaces
      */
 
      CFE_SB_CreatePipe(&JMsgUdpApp.CmdPipe, INITBL_GetIntConfig(INITBL_OBJ, CFG_CMD_PIPE_DEPTH), INITBL_GetStrConfig(INITBL_OBJ, CFG_CMD_PIPE_NAME));  
      CFE_SB_Subscribe(JMsgUdpApp.JMsgLibMid, JMsgUdpApp.CmdPipe);
      CFE_SB_Subscribe(JMsgUdpApp.CmdMid, JMsgUdpApp.CmdPipe);
      CFE_SB_Subscribe(JMsgUdpApp.SendStatusMid, JMsgUdpApp.CmdPipe);

      CMDMGR_Constructor(CMDMGR_OBJ);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, JMSG_UDP_NOOP_CC,  NULL, JMSG_UDP_APP_NoOpCmd,     0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, JMSG_UDP_RESET_CC, NULL, JMSG_UDP_APP_ResetAppCmd, 0);

      CMDMGR_RegisterFunc(CMDMGR_OBJ, JMSG_UDP_LOAD_TBL_CC, TBLMGR_OBJ, TBLMGR_LoadTblCmd, sizeof(JMSG_UDP_LoadTbl_CmdPayload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, JMSG_UDP_DUMP_TBL_CC, TBLMGR_OBJ, TBLMGR_DumpTblCmd, sizeof(JMSG_UDP_DumpTbl_CmdPayload_t));
 
      CMDMGR_RegisterFunc(CMDMGR_OBJ, JMSG_LIB_CONFIG_TOPIC_PLUGIN_CC,    NULL, JMSG_TOPIC_TBL_ConfigTopicPluginCmd,     sizeof(JMSG_LIB_ConfigTopicPlugin_CmdPayload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, JMSG_LIB_SEND_TOPIC_PLUGIN_TLM_CC,  NULL, JMSG_TOPIC_TBL_SendTopicTPluginTlmCmd,   0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, JMSG_LIB_CONFIG_SB_TOPIC_TEST_CC,   NULL, JMSG_TOPIC_TBL_ConfigSbTopicTestCmd,     sizeof(JMSG_LIB_ConfigSbTopicTest_CmdPayload_t));

      CMDMGR_RegisterFunc(CMDMGR_OBJ, JMSG_UDP_START_TEST_CC, NULL, JMSG_UDP_StartTestCmd, 0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, JMSG_UDP_STOP_TEST_CC,  NULL, JMSG_UDP_StopTestCmd, 0);

         
      CFE_MSG_Init(CFE_MSG_PTR(JMsgUdpApp.StatusTlm.TelemetryHeader), CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_JMSG_UDP_STATUS_TLM_TOPICID)), sizeof(JMSG_UDP_StatusTlm_t));

      /*
      ** Application startup event message
      */
      CFE_EVS_SendEvent(JMSG_UDP_APP_INIT_APP_EID, CFE_EVS_EventType_INFORMATION,
                        "JMSG UDP Gateway App Initialized. Version %d.%d.%d",
                        JMSG_UDP_APP_MAJOR_VER, JMSG_UDP_APP_MINOR_VER, JMSG_UDP_APP_PLATFORM_REV);
                        
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


   CFE_ES_PerfLogExit(JMsgUdpApp.PerfId);
   SysStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, JMsgUdpApp.CmdPipe, CFE_SB_PEND_FOREVER);
   CFE_ES_PerfLogEntry(JMsgUdpApp.PerfId);

   if (SysStatus == CFE_SUCCESS)
   {
      SysStatus = CFE_MSG_GetMsgId(&SbBufPtr->Msg, &MsgId);

      if (SysStatus == CFE_SUCCESS)
      {

         if (CFE_SB_MsgId_Equal(MsgId, JMsgUdpApp.JMsgLibMid) || CFE_SB_MsgId_Equal(MsgId, JMsgUdpApp.CmdMid))
         {
            CMDMGR_DispatchFunc(CMDMGR_OBJ, &SbBufPtr->Msg);
         } 
         else if (CFE_SB_MsgId_Equal(MsgId, JMsgUdpApp.SendStatusMid))
         {   
            SendStatusPkt();
         }
         else
         {   
            CFE_EVS_SendEvent(JMSG_UDP_APP_INVALID_MID_EID, CFE_EVS_EventType_ERROR,
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
   
   JMSG_UDP_StatusTlm_Payload_t *Payload = &JMsgUdpApp.StatusTlm.Payload;

   /*
   ** Framework Data
   */

   Payload->ValidCmdCnt    = JMsgUdpApp.CmdMgr.ValidCmdCnt;
   Payload->InvalidCmdCnt  = JMsgUdpApp.CmdMgr.InvalidCmdCnt;

   /*
   ** UDP Manager Data
   */

   Payload->RxUdpConnected = JMsgUdpApp.JMsgUdp.Rx.Connected;
   Payload->RxUdpMsgCnt    = JMsgUdpApp.JMsgUdp.Rx.MsgCnt;
   Payload->RxUdpMsgErrCnt = JMsgUdpApp.JMsgUdp.Rx.MsgErrCnt;
   
   Payload->TxUdpConnected = JMsgUdpApp.JMsgUdp.Tx.Connected;
   Payload->TxUdpMsgCnt    = JMsgUdpApp.JMsgUdp.Tx.MsgCnt;
   Payload->TxUdpMsgErrCnt = JMsgUdpApp.JMsgUdp.Tx.MsgErrCnt;

   CFE_SB_TimeStampMsg(CFE_MSG_PTR(JMsgUdpApp.StatusTlm.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(JMsgUdpApp.StatusTlm.TelemetryHeader), true);

} /* End SendStatusPkt() */
