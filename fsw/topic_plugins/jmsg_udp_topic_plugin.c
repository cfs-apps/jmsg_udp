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
**  Purpose:
**    Configure JMSG topic plugins for the JMSG_UDP app
**
**  Notes:
**    1. This file encapsulates all of the topic plugin configurations required
**       by the JMSG_UDP. It should be the only file that is changed to add and
**       topic plugins used by JMSG_UDP.
**    2. Each JMSG_UDP plugin requires the following steps:
**       A. In the "Topic Plugin" section include the plugin header and define
**          data
**       B. In the JMSG_UDP_TOPIC_PLUGIN_Constructor(), construct the plugin
**          and subscribe to the message  
**    3. JMSG_LIB plugins used by JMSG_UDP only need to be subscribed to because
**       they've already been constructed.
*/

/*
** Include Files:
*/

#include "jmsg_udp_topic_plugin.h"

/*******************/
/** Topic Plugins **/
/*******************/

#include "udp_topic_rpi_demo.h"
static UDP_TOPIC_RPI_DEMO_Class_t UdpTopicRpiDemo;


/******************************************************************************
** Function: JMSG_UDP_TOPIC_PLUGIN_Constructor
**
*/
void JMSG_UDP_TOPIC_PLUGIN_Constructor(JMSG_TOPIC_TBL_ConfigSubscription_t ConfigSubscription)
{

   // The configure subscription callback must be registered prior to topic message subscriptions
   JMSG_TOPIC_TBL_RegisterConfigSubscriptionCallback(ConfigSubscription);

   // Construct JMSG_UDP plugins
   UDP_TOPIC_RPI_DEMO_Constructor(&UdpTopicRpiDemo, JMSG_USR_TopicPlugin_USR_1);

   // Register all plugins used by JMSG_UDP
   JMSG_TOPIC_TBL_SubscribeToTopicMsg(JMSG_USR_TopicPlugin_CMD, JMSG_TOPIC_TBL_SUB_TO_ROLE);
   JMSG_TOPIC_TBL_SubscribeToTopicMsg(JMSG_USR_TopicPlugin_TLM, JMSG_TOPIC_TBL_SUB_TO_ROLE);
   JMSG_TOPIC_TBL_SubscribeToTopicMsg(JMSG_USR_TopicPlugin_TEST,JMSG_TOPIC_TBL_SUB_TO_ROLE);  // Test configured as sub
   JMSG_TOPIC_TBL_SubscribeToTopicMsg(JMSG_USR_TopicPlugin_USR_4,JMSG_TOPIC_TBL_SUB_TO_ROLE); // Test configured as pub
   JMSG_TOPIC_TBL_SubscribeToTopicMsg(JMSG_UDP_PLUGIN_TopicPlugin_RpiDemo,JMSG_TOPIC_TBL_SUB_TO_ROLE);
    
} /* JMSG_TOPIC_PLUGIN_Constructor() */




