/*
** Copyright 2022 bitValence, Inc.
** All Rights Reserved.
**
** This program is free software; you can modify and/or redistribute it
** under the terms of the GNU Affero General Public License
** as published by the Free Software Foundation; version 3 with
** attribution addendums as found in the LICENSE.txt.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Affero General Public License for more details.
**
** Purpose:
**   Define platform configurations for the JMSG UDP Gateway application
**
** Notes:
**   1. These macros can only be build with the application and can't
**      have a platform scope because the same app_cfg.h file name is used for
**      all applications following the object-based application design.
**
*/

#ifndef _app_cfg_
#define _app_cfg_

/*
** Includes
*/

#include "jmsg_udp_eds_typedefs.h"
#include "jmsg_udp_platform_cfg.h"
#include "app_c_fw.h"


/******************************************************************************
** Application Macros
*/

/*
** Versions:
**
** 1.0 - Initial release tested with jmsg_lib 1.0 and jmsg_mqtt 2.0
*/

#define  JMSG_UDP_APP_MAJOR_VER   1
#define  JMSG_UDP_APP_MINOR_VER   0


/******************************************************************************
** Init File declarations create:
**
**  typedef enum {
**     CMD_PIPE_DEPTH,
**     CMD_PIPE_NAME
**  } INITBL_ConfigEnum;
**    
**  typedef struct {
**     CMD_PIPE_DEPTH,
**     CMD_PIPE_NAME
**  } INITBL_ConfigStruct;
**
**   const char *GetConfigStr(value);
**   ConfigEnum GetConfigVal(const char *str);
**
** XX(name,type)
*/

#define CFG_APP_CFE_NAME       APP_CFE_NAME
#define CFG_APP_MAIN_PERF_ID   APP_MAIN_PERF_ID

#define CFG_JMSG_UDP_CMD_TOPICID                  JMSG_UDP_CMD_TOPICID
#define CFG_JMSG_UDP_STATUS_TLM_TOPICID           JMSG_UDP_STATUS_TLM_TOPICID
#define CFG_SEND_STATUS_TLM_TOPICID               BC_SCH_2_SEC_TOPICID
#define CFG_JMSG_LIB_TOPIC_SUBSCRIBE_TLM_TOPICID  JMSG_LIB_TOPIC_SUBSCRIBE_TLM_TOPICID

      
#define CFG_CMD_PIPE_NAME   CMD_PIPE_NAME
#define CFG_CMD_PIPE_DEPTH  CMD_PIPE_DEPTH

#define CFG_JMSG_PIPE_NAME  JMSG_PIPE_NAME
#define CFG_JMSG_PIPE_DEPTH JMSG_PIPE_DEPTH

#define CFG_RX_UDP_PORT          RX_UDP_PORT
#define CFG_RX_CHILD_NAME        RX_CHILD_NAME
#define CFG_RX_CHILD_STACK_SIZE  RX_CHILD_STACK_SIZE
#define CFG_RX_CHILD_PRIORITY    RX_CHILD_PRIORITY
#define CFG_RX_CHILD_PERF_ID     RX_CHILD_PERF_ID

#define CFG_TX_UDP_ADDR          TX_UDP_ADDR
#define CFG_TX_UDP_PORT          TX_UDP_PORT
#define CFG_TX_CHILD_NAME        TX_CHILD_NAME
#define CFG_TX_CHILD_STACK_SIZE  TX_CHILD_STACK_SIZE
#define CFG_TX_CHILD_PRIORITY    TX_CHILD_PRIORITY
#define CFG_TX_CHILD_PERF_ID     TX_CHILD_PERF_ID
#define CFG_TX_SB_PIPE_NAME      TX_SB_PIPE_NAME
#define CFG_TX_SB_PIPE_DEPTH     TX_SB_PIPE_DEPTH


#define APP_CONFIG(XX) \
   XX(APP_CFE_NAME,char*) \
   XX(APP_MAIN_PERF_ID,uint32) \
   XX(JMSG_UDP_CMD_TOPICID,uint32) \
   XX(JMSG_UDP_STATUS_TLM_TOPICID,uint32) \
   XX(BC_SCH_2_SEC_TOPICID,uint32) \
   XX(JMSG_LIB_TOPIC_SUBSCRIBE_TLM_TOPICID,uint32) \
   XX(CMD_PIPE_NAME,char*) \
   XX(CMD_PIPE_DEPTH,uint32) \
   XX(JMSG_PIPE_NAME,char*) \
   XX(JMSG_PIPE_DEPTH,uint32) \
   XX(RX_UDP_PORT,uint32) \
   XX(RX_CHILD_NAME,char*) \
   XX(RX_CHILD_STACK_SIZE,uint32) \
   XX(RX_CHILD_PRIORITY,uint32) \
   XX(RX_CHILD_PERF_ID,uint32) \
   XX(TX_UDP_ADDR,char*) \
   XX(TX_UDP_PORT,uint32) \
   XX(TX_CHILD_NAME,char*) \
   XX(TX_CHILD_STACK_SIZE,uint32) \
   XX(TX_CHILD_PRIORITY,uint32) \
   XX(TX_CHILD_PERF_ID,uint32) \
   XX(TX_SB_PIPE_NAME,char*) \
   XX(TX_SB_PIPE_DEPTH,uint32)

DECLARE_ENUM(Config,APP_CONFIG)


/******************************************************************************
** Event Macros
**
** Define the base event message IDs used by each object/component used by the
** application. There are no automated checks to ensure an ID range is not
** exceeded so it is the developer's responsibility to verify the ranges. 
*/

#define JMSG_UDP_APP_BASE_EID  (APP_C_FW_APP_BASE_EID +  0)
#define JMSG_UDP_BASE_EID      (APP_C_FW_APP_BASE_EID + 20)
#define JMSG_TRANS_BASE_EID    (APP_C_FW_APP_BASE_EID + 30)

// Topic plugin macros are defined in jmsg_lib/eds/jmsg_usr.xml


/******************************************************************************
** JMSG_UDP
**
*/

#define JMSG_UDP_BUF_LEN   4096  /* Must accomodate Rx and Tx maximum message length */



#endif /* _app_cfg_ */
