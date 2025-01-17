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
**   Translate between JSON Messages and Software Bus messages
**
** Notes:
**   1. Each supported JMSG topic is listed in a JSON file and each
**      topic has a JSON file that defines the topic's content.
**   2. The Basecamp JSON table coding idiom is to use a separate object to manage 
**      the table. Since UDP_COMM has very little functionality beyond
**      processing the table, a single object is used for management functions
**      and table processing.
**
*/

/*
** Include Files:
*/

#include <string.h>

#include "jmsg_trans.h"
#include "jmsg_topic_tbl.h"

/********************************** **/
/** Local File Function Prototypes **/
/************************************/


/**********************/
/** Global File Data **/
/**********************/

static JMSG_TRANS_Class_t *JMsgTrans = NULL;


/******************************************************************************
** Function: JMSG_TRANS_Constructor
**
*/
void JMSG_TRANS_Constructor(JMSG_TRANS_Class_t *JMsgTransPtr)
{
 
   JMsgTrans = JMsgTransPtr;

   CFE_PSP_MemSet((void*)JMsgTransPtr, 0, sizeof(JMSG_TRANS_Class_t));


} /* End JMSG_TRANS_Constructor() */


/******************************************************************************
** Function: JMSG_TRANS_ProcessJMsg
**
** Notes:
**   1. Assumes caller has ensured a null terminated string
**   2. Topic string uses MQTT path style topics with a colon appended to the end 
**   3. Test strings that can be pasted in console:
**      echo -n 'hello' >  /dev/udp/localhost/8888   # Error: Null message length since no colon
**      echo -n 'hello:' >  /dev/udp/localhost/8888  # Error: Can't find topic in table
**      echo -n 'basecamp/test:' >  /dev/udp/localhost/8888  # Error: JSON query error since no JSON text
**      echo -n 'basecamp/test:{"int32": 1,"float": 2.3}' >  /dev/udp/localhost/8888  # Successfully send SB test message
**      echo -n 'basecamp/rpi/demo:{"rpi-demo":{"rate-x": 1.0, "rate-y": 2.0, "rate-z": 3.0, "lux": 456}}' >  /dev/udp/localhost/8888
*/
bool JMSG_TRANS_ProcessJMsg(const char *MsgData)
{
   char    *MsgPayload;
   char    MsgTopicName[JMSG_PLATFORM_TOPIC_NAME_MAX_LEN]; 
   uint16  MsgTopicNameLen;
   uint16  MsgPayloadLen;
   uint16  TblTopicNameLen;
   bool    MsgFound = false;
   const JMSG_TOPIC_TBL_Topic_t *TopicTblEntry;
   JMSG_PLATFORM_TopicPlugin_Enum_t TopicPluginId = JMSG_PLATFORM_TopicPlugin_Enum_t_MIN;

   JMSG_TOPIC_TBL_JsonToCfe_t JsonToCfe;
   CFE_MSG_Message_t *CfeMsg;
   CFE_SB_MsgId_t    MsgId = CFE_SB_INVALID_MSG_ID;
   CFE_MSG_Size_t    MsgSize;
   CFE_MSG_Type_t    MsgType;
   
   
   CFE_EVS_SendEvent(JMSG_TRANS_PROCESS_JMSG_EID, CFE_EVS_EventType_DEBUG,
                     "JMSG_TRANS_ProcessJMsg: Received JMSG %s", MsgData);
                    
   MsgPayload = strchr(MsgData, ':');
                    
   if(MsgPayload != NULL)
   {
      MsgPayload++;  // Move past colon
      MsgTopicNameLen = MsgPayload - MsgData;
      strncpy(MsgTopicName, MsgData, MsgTopicNameLen);
      MsgTopicName[MsgTopicNameLen] = '\0';
      MsgPayloadLen = strlen(MsgData) - MsgTopicNameLen;
      
      CFE_EVS_SendEvent(JMSG_TRANS_PROCESS_JMSG_EID, CFE_EVS_EventType_DEBUG,
                        "Message topic name len %d, text: %s", MsgTopicNameLen, MsgTopicName);

      while (!MsgFound && TopicPluginId <= JMSG_PLATFORM_TopicPlugin_Enum_t_MAX)
      {
         TopicTblEntry = JMSG_TOPIC_TBL_GetTopic(TopicPluginId);
         if (TopicTblEntry != NULL)
         {
            TblTopicNameLen = strlen(TopicTblEntry->Name);
            
            if (TblTopicNameLen < JMSG_PLATFORM_TOPIC_NAME_MAX_LEN)
            {
               
               CFE_EVS_SendEvent(JMSG_TRANS_PROCESS_JMSG_EID, CFE_EVS_EventType_DEBUG,
                                 "Table topic name=%s, length=%d", TopicTblEntry->Name, TblTopicNameLen);
               
               if (strncmp(TopicTblEntry->Name, MsgTopicName, TblTopicNameLen) == 0)
               {
                  
                  MsgFound = true;
                  CFE_EVS_SendEvent(JMSG_TRANS_PROCESS_JMSG_EID, CFE_EVS_EventType_DEBUG,
                                   "JMSG_TRANS_ProcessJMsg: Topic=%s, TopicLen=%d, Payload=%s, PayloadLen=%d", 
                                    MsgTopicName, MsgTopicNameLen, MsgPayload, MsgPayloadLen);
               }
            } /* End if valid length */
            else
            {
               CFE_EVS_SendEvent(JMSG_TRANS_PROCESS_JMSG_EID, CFE_EVS_EventType_ERROR,
                                 "Table topic name %s with length %d exceeds maximum length %d", 
                                 TopicTblEntry->Name, TblTopicNameLen, JMSG_PLATFORM_TOPIC_NAME_MAX_LEN);               
            }
         } /* End while loop */
         
         if (!MsgFound)
         {
            TopicPluginId++;
         }
      } /* End while loop */

      if (MsgFound)
      {
            
         CFE_EVS_SendEvent(JMSG_TRANS_PROCESS_JMSG_EID, CFE_EVS_EventType_DEBUG,
                           "JMSG_TRANS_ProcessJMsg: Found message at index %d", TopicPluginId); 
                              
         JsonToCfe = JMSG_TOPIC_TBL_GetJsonToCfe(TopicPluginId);    
       
         if (JsonToCfe(&CfeMsg, MsgPayload, MsgPayloadLen))
         {         
      
            CFE_MSG_GetMsgId(CfeMsg, &MsgId);
            CFE_MSG_GetSize(CfeMsg, &MsgSize);
            
            CFE_MSG_GetType(CfeMsg,&MsgType);
            CFE_MSG_GetTypeFromMsgId(MsgId, &MsgType);
            if (MsgType == CFE_MSG_Type_Cmd)
            {
               CFE_MSG_GenerateChecksum(CFE_MSG_PTR(*CfeMsg));
            }
            else
            {
               CFE_SB_TimeStampMsg(CFE_MSG_PTR(*CfeMsg));
            }
            
            CFE_EVS_SendEvent(JMSG_TRANS_PROCESS_JMSG_EID, CFE_EVS_EventType_DEBUG,
                              "MSG_TRANS_ProcessJMsg: Sending SB message 0x%04X(%d), len %d, type %d", 
                              CFE_SB_MsgIdToValue(MsgId), CFE_SB_MsgIdToValue(MsgId), (int)MsgSize, (int)MsgType); 
            CFE_SB_TransmitMsg(CFE_MSG_PTR(*CfeMsg), true);               
            JMsgTrans->ValidJMsgCnt++;
            
         }
         else
         {
            CFE_EVS_SendEvent(JMSG_TRANS_PROCESS_JMSG_EID, CFE_EVS_EventType_ERROR,
                              "MSG_TRANS_ProcessJMsg: Error creating SB message from JSON topic %s, Id %d",
                              MsgTopicName, TopicPluginId); 
         }
         
      } /* End if message found */
      else 
      {      
         CFE_EVS_SendEvent(JMSG_TRANS_PROCESS_JMSG_EID, CFE_EVS_EventType_ERROR, 
                           "JMSG_TRANS_ProcessJMsg: Could not find a topic match for %s", 
                           MsgTopicName);      
      }
   
   } /* End null message len */
   else 
   {
      
      CFE_EVS_SendEvent(JMSG_TRANS_PROCESS_JMSG_EID, CFE_EVS_EventType_ERROR,
                        "Null JSON message data length for %s", MsgData);
   }

   if (!MsgFound)
   {
      JMsgTrans->InvalidJMsgCnt++;
   }   
   
   return MsgFound;

} /* End JMSG_TRANS_ProcessJMsg() */


/******************************************************************************
** Function: JMSG_TRANS_ProcessSbMsg
**
** Notes:
**   None
**
*/
bool JMSG_TRANS_ProcessSbMsg(const CFE_MSG_Message_t *CfeMsgPtr,
                             const char **Topic, const char **Payload)
{
   
   bool RetStatus = false;
   int32 TopicIndex;
   int32 SbStatus;
   CFE_SB_MsgId_t  MsgId = CFE_SB_INVALID_MSG_ID;
   JMSG_TOPIC_TBL_CfeToJson_t CfeToJson;
   const char *JsonMsgTopic;
   const char *JsonMsgPayload;

   *Topic   = NULL; 
   *Payload = NULL;
   
   SbStatus = CFE_MSG_GetMsgId(CfeMsgPtr, &MsgId);
   if (SbStatus == CFE_SUCCESS)
   {
   
      CFE_EVS_SendEvent(JMSG_TRANS_PROCESS_SB_MSG_EID, CFE_EVS_EventType_DEBUG, 
                        "JMSG_TRANS_ProcessSbMsg: Received SB message ID 0x%04X(%d)", 
                        CFE_SB_MsgIdToValue(MsgId), CFE_SB_MsgIdToValue(MsgId)); 
      
      if ((TopicIndex = JMSG_TOPIC_TBL_MsgIdToTopicPlugin(MsgId)) != JMSG_PLATFORM_TOPIC_PLUGIN_UNDEF)
      {
         
         CfeToJson = JMSG_TOPIC_TBL_GetCfeToJson(TopicIndex, &JsonMsgTopic);    
         
         if (CfeToJson(&JsonMsgPayload, CfeMsgPtr))
         {
            *Topic   = JsonMsgTopic; 
            *Payload = JsonMsgPayload;
            RetStatus = true;
            CFE_EVS_SendEvent(JMSG_TRANS_PROCESS_SB_MSG_EID, CFE_EVS_EventType_INFORMATION,
                              "Created JMSG plugin topic %s message %s",
                              JsonMsgTopic, JsonMsgPayload);             
            JMsgTrans->ValidSbMsgCnt++;

         }
         else
         {
            CFE_EVS_SendEvent(JMSG_TRANS_PROCESS_SB_MSG_EID, CFE_EVS_EventType_ERROR,
                              "Error creating JSON message from SB for plugin topic %d", TopicIndex); 
         
         }        
      }
      else
      {
         CFE_EVS_SendEvent(JMSG_TRANS_PROCESS_SB_MSG_EID, CFE_EVS_EventType_ERROR, 
                           "Unable to locate SB message 0x%04X(%d) in JMSG plugin topic table", 
                           CFE_SB_MsgIdToValue(MsgId), CFE_SB_MsgIdToValue(MsgId));
      }

   } /* End message Id */
   else
   {
      CFE_EVS_SendEvent(JMSG_TRANS_PROCESS_SB_MSG_EID, CFE_EVS_EventType_ERROR, 
                        "Error reading SB message, return status = 0x%04X", SbStatus); 
   }

   return RetStatus;
   
} /* End JMSG_TRANS_ProcessSbMsg() */


/******************************************************************************
** Function: JMSG_TRANS_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
*/
void JMSG_TRANS_ResetStatus(void)
{

   JMsgTrans->ValidJMsgCnt    = 0;
   JMsgTrans->InvalidJMsgCnt  = 0;
   JMsgTrans->ValidSbMsgCnt   = 0;
   JMsgTrans->InvalidSbMsgCnt = 0;

} /* JMSG_TRANS_ResetStatus() */

