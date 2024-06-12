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
**   Manage JMSG UDP Raspberry Pi Demo plugin topic
**
** Notes:
**   None
**
*/

/*
** Includes
*/

#include "udp_topic_rpi_demo.h"

/************************************/
/** Local File Function Prototypes **/
/************************************/

static bool CfeToJson(const char **JsonMsgPayload, const CFE_MSG_Message_t *CfeMsg);
static bool JsonToCfe(CFE_MSG_Message_t **CfeMsg, const char *JsonMsgPayload, uint16 PayloadLen);
static bool LoadJsonData(const char *JsonMsgPayload, uint16 PayloadLen);
static void PluginTest(bool Init, int16 Param);


/**********************/
/** Global File Data **/
/**********************/

static UDP_TOPIC_RPI_DEMO_Class_t* UdpTopicRpiDemo = NULL;

static JMSG_UDP_PLUGIN_RpiDemoTlm_Payload_t  RpiDemoTlm; /* Working buffer for loads */

static CJSON_Obj_t JsonTblObjs[] = 
{

   /* Data                 Data                                   core-json         length of query      */
   /* Address,            Length,  Updated, Data Type,  Float,  query string,       string(exclude '\0') */
   
   { &RpiDemoTlm.Rate_X,   4,     false,   JSONNumber, false,  { "rpi-demo.rate-x", (sizeof("rpi-demo.rate-x")-1)} },
   { &RpiDemoTlm.Rate_Y,   4,     false,   JSONNumber, false,  { "rpi-demo.rate-y", (sizeof("rpi-demo.rate-y")-1)} },
   { &RpiDemoTlm.Rate_Z,   4,     false,   JSONNumber, false,  { "rpi-demo.rate-z", (sizeof("rpi-demo.rate-z")-1)} },
   { &RpiDemoTlm.Lux,      4,     false,   JSONNumber, true,   { "rpi-demo.lux",    (sizeof("rpi-demo.lux")-1)} }
   
};

static const char *NullRpiDemoMsg = "{\"rpi-demo\":{\"rate-x\": 0.0,\"rate-y\": 0.0,\"rate-z\": 0.0,\"lux\": 0}}";


/******************************************************************************
** Function: UDP_TOPIC_RPI_DEMO_Constructor
**
** Initialize the UDP RPI Demo topic
**
** Notes:
**   None
**
*/
void UDP_TOPIC_RPI_DEMO_Constructor(UDP_TOPIC_RPI_DEMO_Class_t *UdpTopicRpiDemoPtr,
                                    JMSG_USR_TopicPlugin_Enum_t TopicPlugin)
{

   UdpTopicRpiDemo = UdpTopicRpiDemoPtr;
   memset(UdpTopicRpiDemo, 0, sizeof(UDP_TOPIC_RPI_DEMO_Class_t));

   UdpTopicRpiDemo->JsonObjCnt = (sizeof(JsonTblObjs)/sizeof(CJSON_Obj_t));

   /* TODO: Confirm no race ocndition if JSON being produced while this initialization is taking place becuase message init occurs after the callback subscription */ 
   
   UdpTopicRpiDemo->SbMsgId = JMSG_TOPIC_TBL_RegisterPlugin(TopicPlugin, CfeToJson, JsonToCfe, PluginTest);

   CFE_MSG_Init(CFE_MSG_PTR(UdpTopicRpiDemo->TlmMsg), UdpTopicRpiDemo->SbMsgId, sizeof(JMSG_UDP_PLUGIN_RpiDemoTlmMsg_t));
      
} /* End UDP_TOPIC_RPI_DEMO_Constructor() */


/******************************************************************************
** Function: CfeToJson
**
** Convert a cFE RPI Demo message to a UDP JSON topic message 
**
** Notes:
**   1. Signature must match MQTT_TOPIC_TBL_CfeToJson_t
**
*/
static bool CfeToJson(const char **JsonMsgPayload, const CFE_MSG_Message_t *CfeMsg)
{

   bool  RetStatus = false;
   int   PayloadLen; 
   const JMSG_UDP_PLUGIN_RpiDemoTlm_Payload_t *RpiDemoMsg = CMDMGR_PAYLOAD_PTR(CfeMsg, JMSG_UDP_PLUGIN_RpiDemoTlmMsg_t);

   *JsonMsgPayload = NullRpiDemoMsg;
   
   PayloadLen = sprintf(UdpTopicRpiDemo->JsonMsgPayload,
                "{\"rpi-demo\":{\"rate-x\": %f,\"rate-y\": %f,\"rate-z\": %f,\"lux\": %1d}}",
                RpiDemoMsg->Rate_X, RpiDemoMsg->Rate_Y, RpiDemoMsg->Rate_Z, RpiDemoMsg->Lux);

   if (PayloadLen > 0)
   {
      *JsonMsgPayload = UdpTopicRpiDemo->JsonMsgPayload;
   
      ++UdpTopicRpiDemo->CfeToJsonCnt;
      RetStatus = true;
   }
   
   return RetStatus;
   
} /* End CfeToJson() */


/******************************************************************************
** Function: JsonToCfe
**
** Convert a UDP JSON RPI Demo topic message to a cFE SB RPI Demo message 
**
** Notes:
**   1. Signature must match MQTT_TOPIC_TBL_JsonToCfe_t
**   2. Messages that can be sent over UDP for testing
**      {"rpi-demo":{"rate-x": 1.0, "rate-y": 2.0, "rate-z": 3.0, "lux": 456}}
**
*/
static bool JsonToCfe(CFE_MSG_Message_t **CfeMsg, const char *JsonMsgPayload, uint16 PayloadLen)
{
   
   bool RetStatus = false;
   
   *CfeMsg = NULL;
   
   if (LoadJsonData(JsonMsgPayload, PayloadLen))
   {
      *CfeMsg = (CFE_MSG_Message_t *)&UdpTopicRpiDemo->TlmMsg;

      ++UdpTopicRpiDemo->JsonToCfeCnt;
      RetStatus = true;
   }

   return RetStatus;
   
} /* End JsonToCfe() */


/******************************************************************************
** Function: PluginTest
**
** Generate and send SB RPI Demo topic messages on SB that are read back by
** JMSG_UDP and cause JMSG's to be generated from the SB messages.  
**
** Notes:
**   1. Param is not used
**
*/
static void PluginTest(bool Init, int16 Param)
{

   if (Init)
   {
      
      UdpTopicRpiDemo->TestData.RateX = 1.0;
      UdpTopicRpiDemo->TestData.RateY = 2.0;
      UdpTopicRpiDemo->TestData.RateZ = 3.0;
      UdpTopicRpiDemo->TestData.Lux   = 100;

      CFE_EVS_SendEvent(UDP_TOPIC_RPI_DEMO_INIT_PLUGIN_TEST_EID, CFE_EVS_EventType_INFORMATION,
                        "UDP RPI Demo plugin topic test started");

   }
   else
   {
                    
      UdpTopicRpiDemo->TestData.RateX += 1.0;
      UdpTopicRpiDemo->TestData.RateY += 1.0;
      UdpTopicRpiDemo->TestData.RateZ += 1.0;
      UdpTopicRpiDemo->TestData.Lux++;
      
   }

   
   CFE_EVS_SendEvent(UDP_TOPIC_RPI_DEMO_PLUGIN_TEST_EID, CFE_EVS_EventType_DEBUG,
                        "UDP RPI Demo plugin topic test sending: %.4f, %.4f, %.4f, %d ", 
                        UdpTopicRpiDemo->TestData.RateX, UdpTopicRpiDemo->TestData.RateY,
                        UdpTopicRpiDemo->TestData.RateZ, UdpTopicRpiDemo->TestData.Lux);
                        
   CFE_SB_TimeStampMsg(CFE_MSG_PTR(UdpTopicRpiDemo->TlmMsg.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(UdpTopicRpiDemo->TlmMsg.TelemetryHeader), true);
   
} /* End SbMsgTest() */


/******************************************************************************
** Function: LoadJsonData
**
** Notes:
**  1. See file prologue for full/partial table load scenarios
*/
static bool LoadJsonData(const char *JsonMsgPayload, uint16 PayloadLen)
{

   bool      RetStatus = false;
   size_t    ObjLoadCnt;

   memset(&UdpTopicRpiDemo->TlmMsg.Payload, 0, sizeof(JMSG_UDP_PLUGIN_RpiDemoTlm_Payload_t));
   ObjLoadCnt = CJSON_LoadObjArray(JsonTblObjs, UdpTopicRpiDemo->JsonObjCnt, 
                                   JsonMsgPayload, PayloadLen);
   CFE_EVS_SendEvent(UDP_TOPIC_RPI_DEMO_LOAD_JSON_DATA_EID, CFE_EVS_EventType_DEBUG,
                     "UDP RPI Demo Plugin LoadJsonData() process %d JSON objects", (uint16)ObjLoadCnt);

   if (ObjLoadCnt == UdpTopicRpiDemo->JsonObjCnt)
   {
      memcpy(&UdpTopicRpiDemo->TlmMsg.Payload, &RpiDemoTlm, sizeof(JMSG_UDP_PLUGIN_RpiDemoTlm_Payload_t));      
      RetStatus = true;
   }
   else
   {
      CFE_EVS_SendEvent(UDP_TOPIC_RPI_DEMO_JSON_TO_CCSDS_ERR_EID, CFE_EVS_EventType_ERROR, 
                        "Error processing UDP RPI Demo plugin topic, payload contained %d of %d data objects",
                        (unsigned int)ObjLoadCnt, (unsigned int)UdpTopicRpiDemo->JsonObjCnt);
   }
   
   return RetStatus;
   
} /* End LoadJsonData() */

