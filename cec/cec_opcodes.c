/*
 * 
 * (c) 2010 konfetti, schischu
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */


#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/delay.h>

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include <linux/platform_device.h>

#include <asm/system.h>
#include <asm/io.h>

#include "cec_worker.h"
#include "cec_opcodes.h"
#include "cec_opcodes_def.h"
#include "cec_internal.h"
#include "cec_proc.h"

extern long stmhdmiio_get_cec_address(unsigned int * arg);

static unsigned char isFirstKiss = 0;

static unsigned char logicalDeviceTypeChoicesIndex = 0;
static const unsigned char logicalDeviceTypeChoices[] =  { 
DEVICE_TYPE_DVD1, 
DEVICE_TYPE_DVD2, 
DEVICE_TYPE_DVD3, 
DEVICE_TYPE_STB1, 
DEVICE_TYPE_STB2, 
DEVICE_TYPE_STB3,
DEVICE_TYPE_UNREG };

static unsigned char logicalDeviceType = DEVICE_TYPE_DVD1;
static unsigned char deviceType = DEVICE_TYPE_DVD; // cause we want deck control

static unsigned short ActiveSource = 0x0000;

////////////////////////////////////

unsigned char getIsFirstKiss(void) {
  return isFirstKiss;
}

void setIsFirstKiss(unsigned char kiss) {
  isFirstKiss = kiss;
  if(isFirstKiss == 0)
    sendReportPhysicalAddress();
}

unsigned char getLogicalDeviceType(void) {
  return logicalDeviceType;
}

unsigned char getDeviceType(void) {
  return deviceType;
}

unsigned short getPhysicalAddress(void) {
  unsigned int value = 0;
  stmhdmiio_get_cec_address(&value);
  
  return value & 0xffff;
}


//=================


unsigned short getActiveSource(void) {
  return ActiveSource;
}

void setActiveSource(unsigned short addr) {
  printk("FROM: %04x TO: %04x\n", ActiveSource, addr);
  if(ActiveSource != addr) {
    ActiveSource = addr;
    setUpdatedActiveSource();
  }
}

//-----------------------------------------

void parseMessage(unsigned char src, unsigned char dst, unsigned int len, unsigned char buf[])
{
#define NAME_SIZE 100
  char name[NAME_SIZE];
  unsigned char responseBuffer[SEND_BUF_SIZE];

  memset(name, 0, NAME_SIZE);
  memset(responseBuffer, 0, SEND_BUF_SIZE);

  switch(buf[0])
  {
    case FEATURE_ABORT: 
      strcpy(name, "FEATURE_ABORT");
      strcat(name,": ");
      switch(buf[2])
      {
        case ABORT_REASON_UNRECOGNIZED_OPCODE:   strcat(name, "UNRECOGNIZED_OPCODE");   break;
        case ABORT_REASON_NOT_IN_CORRECT_MODE:   strcat(name, "NOT_IN_CORRECT_MODE");   break;
        case ABORT_REASON_CANNOT_PROVIDE_SOURCE: strcat(name, "CANNOT_PROVIDE_SOURCE"); break;
        case ABORT_REASON_INVALID_OPERAND:       strcat(name, "INVALID_OPERAND");       break;
        case ABORT_REASON_REFUSED:               strcat(name, "REFUSED");               break;
        default: break;
      }
      break;

    case ABORT_MESSAGE: 
      strcpy(name, "ABORT_MESSAGE");
      break;


    case IMAGE_VIEW_ON: 
      strcpy(name, "IMAGE_VIEW_ON");
      break;

    case RECORD_ON: 
      strcpy(name, "RECORD_ON");
      break;

    case RECORD_STATUS: 
      strcpy(name, "RECORD_STATUS");
      break;

    case RECORD_OFF: 
      strcpy(name, "RECORD_OFF");
      break;

    case TEXT_VIEW_ON: 
      strcpy(name, "TEXT_VIEW_ON");
      break;

    case RECORD_TV_SCREEN: 
      strcpy(name, "RECORD_TV_SCREEN");
      break;

    case SET_MENU_LANGUAGE: 
      strcpy(name, "SET_MENU_LANGUAGE");
      strcat(name,": ");
      strcat(name, (buf+1));
      break;

    case STANDBY: 
      strcpy(name, "STANDBY");
      setUpdatedStandby();
      break;

    case USER_CONTROL_PRESSED : 
      strcpy(name, "USER_CONTROL_PRESSED ");
      break;

    case USER_CONTROL_RELEASED: 
      strcpy(name, "USER_CONTROL_RELEASED");
      break;

    case GIVE_OSD_NAME: 
      strcpy(name, "GIVE_OSD_NAME");
      responseBuffer[0] = (getLogicalDeviceType() << 4) + (src & 0xF);
      responseBuffer[1] = SET_OSD_NAME;
      responseBuffer[2] = 'D';
      responseBuffer[3] = 'U';
      responseBuffer[4] = 'C'; 
      responseBuffer[5] = 'K'; 
      responseBuffer[6] = 'B'; 
      responseBuffer[7] = 'O'; 
      responseBuffer[8] = 'X'; 
      sendMessage(9, responseBuffer);
      break;

    case SET_OSD_NAME: 
      strcpy(name, "SET_OSD_NAME");
      strcat(name,": ");
      strcat(name, (buf+1));
      break;

    case SET_OSD_STRING: 
      strcpy(name, "SET_OSD_STRING");
      strcat(name,": ");
      strcat(name, (buf+2));
      break;

    case ROUTING_CHANGE: 
      strcpy(name, "ROUTING_CHANGE");
      setActiveSource((buf[3]<<8) + buf[4]);
      break;

    case ROUTING_INFORMATION: 
      strcpy(name, "ROUTING_INFORMATION");
      setActiveSource((buf[1]<<8) + buf[2]);
      break;

    case ACTIVE_SOURCE: 
      strcpy(name, "ACTIVE_SOURCE");
      setActiveSource((buf[1]<<8) + buf[2]);
      break;

    case GIVE_PHYSICAL_ADDRESS: 
      strcpy(name, "GIVE_PHYSICAL_ADDRESS");
      sendReportPhysicalAddress();
      break;

    case REPORT_PHYSICAL_ADDRESS: 
      strcpy(name, "REPORT_PHYSICAL_ADDRESS");
      break;

    case REQUEST_ACTIVE_SOURCE: 
      strcpy(name, "REQUEST_ACTIVE_SOURCE");
      // if we are the current active source, than we have to answer here
      break;

    case SET_STREAM_PATH: 
      strcpy(name, "SET_STREAM_PATH");
      if(((buf[1]<<8) + buf[2]) == getPhysicalAddress()) // If we are the active source
      {
        responseBuffer[0] = (getLogicalDeviceType() << 4) + (BROADCAST & 0xF);
        responseBuffer[1] = ACTIVE_SOURCE;
        responseBuffer[2] = (((getPhysicalAddress() >> 12)&0xf) << 4) + ((getPhysicalAddress() >> 8)&0xf);
        responseBuffer[3] = (((getPhysicalAddress() >> 4)&0xf) << 4) + ((getPhysicalAddress() >> 0)&0xf);
        sendMessage(4, responseBuffer);
      }
      break;

    case DEVICE_VENDOR_ID: 
      strcpy(name, "DEVICE_VENDOR_ID");
      break;

    case VENDOR_COMMAND: 
      strcpy(name, "VENDOR_COMMAND");
      responseBuffer[0] = (getLogicalDeviceType() << 4) + (src & 0xF);
      responseBuffer[1] = FEATURE_ABORT;
      responseBuffer[2] = VENDOR_COMMAND;
      responseBuffer[3] = ABORT_REASON_UNRECOGNIZED_OPCODE;
      sendMessage(4, responseBuffer);
      break;

    case GIVE_DEVICE_VENDOR_ID: 
      strcpy(name, "GIVE_DEVICE_VENDOR_ID");
      responseBuffer[0] = (getLogicalDeviceType() << 4) + (BROADCAST & 0xF);
      responseBuffer[1] = DEVICE_VENDOR_ID;
      responseBuffer[2] = 'D';
      responseBuffer[3] = 'B';
      responseBuffer[4] = 'X';
      sendMessage(5, responseBuffer);
      break;

    case MENU_REQUEST: 
      strcpy(name, "MENU_REQUEST");
      strcat(name,": ");
      switch(buf[1])
      {
        case MENU_REQUEST_TYPE_ACTIVATE:   strcat(name, "ACTIVATE");   break;
        case MENU_REQUEST_TYPE_DEACTIVATE: strcat(name, "DEACTIVATE"); break;
        case MENU_REQUEST_TYPE_QUERY:      strcat(name, "QUERY");      break;
        default: break;
      }
      break;

    case MENU_STATUS: 
      strcpy(name, "MENU_STATUS");
      strcat(name,": ");
      switch(buf[1])
      {
        case MENU_STATE_ACTIVATE:   strcat(name, "ACTIVATE");   break;
        case MENU_STATE_DEACTIVATE: strcat(name, "DEACTIVATE"); break;
        default: break;
      }
      break;

    case GIVE_DEVICE_POWER_STATUS: 
      strcpy(name, "GIVE_DEVICE_POWER_STATUS");
      responseBuffer[0] = (getLogicalDeviceType() << 4) + (src & 0xF);
      responseBuffer[1] = REPORT_POWER_STATUS;
      responseBuffer[2] = POWER_STATUS_ON;
      sendMessage(3, responseBuffer);
      break;

    case REPORT_POWER_STATUS: 
      strcpy(name, "REPORT_POWER_STATUS");
      break;

    case GIVE_MENU_LANGUAGE: 
      strcpy(name, "SET_STREAM_PATH");
      break;

    case CEC_VERSION: 
      strcpy(name, "CEC_VERSION");
      strcat(name,": ");
      switch(buf[1])
      {
        case CEC_VERSION_V11:  strcat(name, "1.1");   break;
        case CEC_VERSION_V12:  strcat(name, "1.2"); break;
        case CEC_VERSION_V12A: strcat(name, "1.2a"); break;
        case CEC_VERSION_V13:  strcat(name, "1.3"); break;
        case CEC_VERSION_V13A: strcat(name, "1.3a"); break;
        case CEC_VERSION_V14:  strcat(name, "1.4"); break;
        default: break;
      }
      break;

    case GET_CEC_VERSION: 
      strcpy(name, "CEC_VERSION");
      responseBuffer[0] = (getLogicalDeviceType() << 4) + (src & 0xF);
      responseBuffer[1] = CEC_VERSION;
      responseBuffer[2] = CEC_VERSION_V13A;
      sendMessage(3, responseBuffer);
      break;

    case VENDOR_COMMAND_WITH_ID: 
      strcpy(name, "VENDOR_COMMAND_WITH_ID");
      responseBuffer[0] = (getLogicalDeviceType() << 4) + (src & 0xF);
      responseBuffer[1] = FEATURE_ABORT;
      responseBuffer[2] = VENDOR_COMMAND_WITH_ID;
      responseBuffer[3] = ABORT_REASON_UNRECOGNIZED_OPCODE;
      sendMessage(4, responseBuffer);
      break;

    default:
      strcpy(name, "UNKNOWN");
      break;
  }

  printk("\tis %s\n", name);
}


void parseRawMessage(unsigned int len, unsigned char buf[])
{
  int ic;

  // Header
  unsigned char src = buf[0] >> 4;
  unsigned char dst = buf[0] & 0x0F;

  unsigned char dataLen = len - 1;

  if(dataLen > CEC_MAX_DATA_LEN) {
    printk("Incoming Message was too long! (%u)\n", dataLen);
    return;
  }

  printk("\tFROM 0x%02x TO 0x%02x : %3u : ", src, dst, dataLen);
  for(ic = 0; ic < dataLen; ic++)
    printk("%02x ", buf[ic+1]);
  printk("\n");

  if (dataLen > 0)
    parseMessage(src, dst, dataLen, buf + 1);
  else
  {
    printk("\tis PING\n");
    //Lets check if the ping was send from or id, if so this means that the deviceId pinged ist already been taken
    if(src == dst && src == getLogicalDeviceType())
    {
      if(getLogicalDeviceType() != DEVICE_TYPE_UNREG)
        sendPingWithAutoIncrement();
    }
  }
}

//================
// Higher Level Functions

void sendReportPhysicalAddress(void) 
{
  unsigned char responseBuffer[SEND_BUF_SIZE];
  memset(responseBuffer, 0, SEND_BUF_SIZE);

  responseBuffer[0] = (getLogicalDeviceType() << 4) + (BROADCAST & 0xF);
  responseBuffer[1] = REPORT_PHYSICAL_ADDRESS;
  responseBuffer[2] = (((getPhysicalAddress() >> 12)&0xf) << 4) + ((getPhysicalAddress() >> 8)&0xf);
  responseBuffer[3] = (((getPhysicalAddress() >> 4)&0xf) << 4) + ((getPhysicalAddress() >> 0)&0xf);
  responseBuffer[4] = getDeviceType(); 
  sendMessage(5, responseBuffer);
}

void setSourceAsActive(void) 
{
  unsigned char responseBuffer[SEND_BUF_SIZE];
  memset(responseBuffer, 0, SEND_BUF_SIZE);

  responseBuffer[0] = (getLogicalDeviceType() << 4) + (BROADCAST & 0xF);
  responseBuffer[1] = ACTIVE_SOURCE;
  responseBuffer[2] = (((getPhysicalAddress() >> 12)&0xf) << 4) + ((getPhysicalAddress() >> 8)&0xf);
  responseBuffer[3] = (((getPhysicalAddress() >> 4)&0xf) << 4) + ((getPhysicalAddress() >> 0)&0xf);
  sendMessage(4, responseBuffer);
}

void wakeTV(void) {
  unsigned char responseBuffer[SEND_BUF_SIZE];
  memset(responseBuffer, 0, SEND_BUF_SIZE);

  responseBuffer[0] = (getLogicalDeviceType() << 4) + (DEVICE_TYPE_TV & 0xF);
  responseBuffer[1] = IMAGE_VIEW_ON;
  sendMessage(2, responseBuffer);
}

void sendInitMessage(void)
{
  unsigned char responseBuffer[SEND_BUF_SIZE];
  memset(responseBuffer, 0, SEND_BUF_SIZE);
  // Determine if TV is On
  
  responseBuffer[0] = (getLogicalDeviceType() << 4) + (DEVICE_TYPE_TV & 0xF);
  responseBuffer[1] = GIVE_DEVICE_POWER_STATUS;
  sendMessage(2, responseBuffer);
}

void sendPingWithAutoIncrement(void)
{
  unsigned char responseBuffer[1];

  setIsFirstKiss(1);

  logicalDeviceType = logicalDeviceTypeChoices[logicalDeviceTypeChoicesIndex++];
  responseBuffer[0] = (logicalDeviceType << 4) + (logicalDeviceType & 0xF);
  sendMessage(1, responseBuffer);
}

void sendOneTouchPlay(void)
{
  unsigned char responseBuffer[SEND_BUF_SIZE];
  memset(responseBuffer, 0, SEND_BUF_SIZE);

  responseBuffer[0] = (getLogicalDeviceType() << 4) + (DEVICE_TYPE_TV & 0xF);
  responseBuffer[1] = IMAGE_VIEW_ON;
  sendMessage(2, responseBuffer);

  msleep(100);

  memset(responseBuffer, 0, SEND_BUF_SIZE);

  responseBuffer[0] = (getLogicalDeviceType() << 4) + (BROADCAST & 0xF);
  responseBuffer[1] = ACTIVE_SOURCE;
  responseBuffer[2] = (((getPhysicalAddress() >> 12)&0xf) << 4) + ((getPhysicalAddress() >> 8)&0xf);
  responseBuffer[3] = (((getPhysicalAddress() >> 4)&0xf) << 4) + ((getPhysicalAddress() >> 0)&0xf);
  sendMessage(4, responseBuffer);
}

void sendSystemStandby(int deviceId)
{
  unsigned char responseBuffer[SEND_BUF_SIZE];
  memset(responseBuffer, 0, SEND_BUF_SIZE);

  responseBuffer[0] = (getLogicalDeviceType() << 4) + (deviceId & 0xF);
  responseBuffer[1] = STANDBY;
  sendMessage(2, responseBuffer);
}

//================

