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

#ifndef _udp_topic_rpi_demo_
#define _udp_topic_rpi_demo_

/*
** Includes
*/

#include "app_cfg.h"
#include "jmsg_topic_plugin.h"
#include "jmsg_udp_plugin_eds_typedefs.h"

/***********************/
/** Macro Definitions **/
/***********************/


/*
** Event Message IDs
*/

#define UDP_TOPIC_RPI_DEMO_BASE_EID   JMSG_USR_TopicPluginBaseEid_USR_1 // Create local define in case USR assignment changes

#define UDP_TOPIC_RPI_DEMO_INIT_PLUGIN_TEST_EID  (UDP_TOPIC_RPI_DEMO_BASE_EID + 0)
#define UDP_TOPIC_RPI_DEMO_PLUGIN_TEST_EID       (UDP_TOPIC_RPI_DEMO_BASE_EID + 1)
#define UDP_TOPIC_RPI_DEMO_LOAD_JSON_DATA_EID    (UDP_TOPIC_RPI_DEMO_BASE_EID + 2)
#define UDP_TOPIC_RPI_DEMO_JSON_TO_CCSDS_ERR_EID (UDP_TOPIC_RPI_DEMO_BASE_EID + 3)

/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Telemetry
** 
** JMSG_MQTT_PLUGIN_RpiDemoTlmMsg_t & JMSG_MQTT_PLUGIN_RpiDemoTlmMsg_Payload_t are defined in EDS
*/


/******************************************************************************
** Class
** 
*/

typedef struct
{
    float   RateX;
    float   RateY;
    float   RateZ;
    int32   Lux;

} UDP_TOPIC_RPI_DEMO_TestData_t;

typedef struct
{

   /*
   ** Discrete Telemetry
   */
   
   char            JsonMsgPayload[1024];
   CFE_SB_MsgId_t  SbMsgId;
   JMSG_UDP_PLUGIN_RpiDemoTlmMsg_t  TlmMsg;

   /*
   ** The plugin test treats the 4 integers as a 4-bit integer that is
   ** incremented from 0..15 
   */
   
   UDP_TOPIC_RPI_DEMO_TestData_t TestData;
   
   /*
   ** Subset of the standard CJSON table data because this isn't using the
   ** app_c_fw table manager service, but is using core-json in the same way
   ** as an app_c_fw managed table.
   */
   size_t  JsonObjCnt;

   uint32  CfeToJsonCnt;
   uint32  JsonToCfeCnt;
   
   
} UDP_TOPIC_RPI_DEMO_Class_t;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: UDP_TOPIC_RPI_DEMO_Constructor
**
** Initialize the JMSG MQTT discrete plugin topic

** Notes:
**   None
**
*/
void UDP_TOPIC_RPI_DEMO_Constructor(UDP_TOPIC_RPI_DEMO_Class_t *MqttTopicDiscretePtr,
                                     JMSG_USR_TopicPlugin_Enum_t TopicPlugin);


#endif /* _udp_topic_rpi_demo_ */
