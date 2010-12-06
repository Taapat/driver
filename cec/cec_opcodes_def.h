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

#ifndef CEC_OPCODES_DEF_H_
#define CEC_OPCODES_DEF_H_

#define BROADCAST 0xf
#define SCREEN 0x0

#define ABORT_REASON_UNRECOGNIZED_OPCODE    0x00
#define ABORT_REASON_NOT_IN_CORRECT_MODE    0x01
#define ABORT_REASON_CANNOT_PROVIDE_SOURCE  0x02
#define ABORT_REASON_INVALID_OPERAND        0x03
#define ABORT_REASON_REFUSED                0x04

#define CEC_VERSION_V11   0x00
#define CEC_VERSION_V12   0x01
#define CEC_VERSION_V12A  0x02
#define CEC_VERSION_V13   0x03
#define CEC_VERSION_V13A  0x04
#define CEC_VERSION_V14   0x05

#define DEVICE_TYPE_TV     0x00
#define DEVICE_TYPE_REC    0x01
#define DEVICE_TYPE_STB    0x03
#define DEVICE_TYPE_DVD    0x04
#define DEVICE_TYPE_AUDIO  0x05

#define DEVICE_TYPE_REC1   0x01
#define DEVICE_TYPE_REC2   0x02
#define DEVICE_TYPE_STB1   0x03
#define DEVICE_TYPE_DVD1   0x04
#define DEVICE_TYPE_AUDIO  0x05
#define DEVICE_TYPE_STB2   0x06
#define DEVICE_TYPE_STB3   0x07
#define DEVICE_TYPE_DVD2   0x08
#define DEVICE_TYPE_REC3   0x09
#define DEVICE_TYPE_STB4   0x0A
#define DEVICE_TYPE_DVD3   0x0B

#define DEVICE_TYPE_RES1    0x0C
#define DEVICE_TYPE_RES2    0x0D
#define DEVICE_TYPE_FREEUSE 0x0E
#define DEVICE_TYPE_UNREG   0x0F


#define DISPLAY_CONTROL_DEFAULT_TIME    0x00
#define DISPLAY_CONTROL_UNTIL_CLEARED   0x01
#define DISPLAY_CONTROL_CLEAR           0x02
#define DISPLAY_CONTROL_RESERVED        0x03

#define MENU_REQUEST_TYPE_ACTIVATE      0x00
#define MENU_REQUEST_TYPE_DEACTIVATE    0x01
#define MENU_REQUEST_TYPE_QUERY         0x02

#define MENU_STATE_ACTIVATE             0x00
#define MENU_STATE_DEACTIVATE           0x01

#define PHYSICAL_ADDRESS_MASK  0xFFFF

#define POWER_STATUS_ON                 0x00
#define POWER_STATUS_STANDBY            0x01
#define POWER_STATUS_GOING_TO_ON        0x02
#define POWER_STATUS_GOING_TO_STANDBY   0x03

typedef unsigned char  tAbortReason;
typedef unsigned char  tDeviceType;
typedef unsigned char  tDisplayControl;
typedef unsigned char  tMenuRequestType;
typedef unsigned char  tMenuState;
typedef unsigned short tPhysicalAddress;
typedef unsigned char  tPowerStatus;
typedef unsigned char  tUiCommand;

#define FEATURE_ABORT           0x00
struct sFEATURE_ABORT  {
  unsigned char FeatureOpcode;
  tAbortReason AbortReason;
};

#define ABORT_MESSAGE           0xFF
struct sABORT_MESSAGE  {
};

#define IMAGE_VIEW_ON           0x04
struct sIMAGE_VIEW_ON {
};

#define RECORD_ON               0x09
struct sRECORD_ON {
  unsigned char RecordSource; //todo
};

#define RECORD_STATUS           0x0A
struct sRECORD_STATUS {
  unsigned char RecordStatusInfo; //todo
};

#define RECORD_OFF              0x0B
struct sRECORD_OFF {
};

#define TEXT_VIEW_ON            0x0D
struct sTEXT_VIEW_ON {
};

#define RECORD_TV_SCREEN        0x0F
struct sRECORD_TV_SCREEN {
};

#define SET_MENU_LANGUAGE       0x32
struct sSET_MENU_LANGUAGE {
  char Language[3];
};

#define STANDBY                 0x36
struct sSTANDBY {
};

#define USER_CONTROL_PRESSED    0x44
// Directly
struct sUSER_CONTROL_PRESSED {
  tUiCommand UiCommand;
};

#define USER_CONTROL_RELEASED   0x45
// Directly
struct sUSER_CONTROL_RELEASED {
};

#define GIVE_OSD_NAME           0x46
struct sGIVE_OSD_NAME {
};

#define SET_OSD_NAME            0x47
struct sSET_OSD_NAME {
  char OsdName[8];
};

#define SET_OSD_STRING          0x64
struct sSET_OSD_STRING {
  tDisplayControl DisplayControl;
  char OsdString[13];
};

#define ROUTING_CHANGE          0x80
struct sROUTING_CHANGE {
  tPhysicalAddress  OriginalAddress;
  tPhysicalAddress  NewAddress;
};

#define ROUTING_INFORMATION     0x81
struct sROUTING_INFORMATION {
  tPhysicalAddress  PhysicalAddress;
};

#define ACTIVE_SOURCE           0x82
// Broadcast
struct sACTIVE_SOURCE {
  tPhysicalAddress  PhysicalAddress;
};

#define GIVE_PHYSICAL_ADDRESS   0x83
struct sGIVE_PHYSICAL_ADDRESS {
};

#define REPORT_PHYSICAL_ADDRESS 0x84
struct sREPORT_PHYSICAL_ADDRESS { // 2f 84 20 00 03
  tPhysicalAddress  PhysicalAddress;
  tDeviceType       DeviceType;
};

#define REQUEST_ACTIVE_SOURCE   0x85
struct sREQUEST_ACTIVE_SOURCE {
};

#define SET_STREAM_PATH         0x86
// Broadcast
// -> ACTIVE_SOURCE
struct sSET_STREAM_PATH {
  tPhysicalAddress  PhysicalAddress;
};

#define DEVICE_VENDOR_ID        0x87

//SONY BRAVIA: 08 00 46
//PANASONIC:   00 80 45

struct sDEVICE_VENDOR_ID {
  unsigned char  VendorId[3];
};

#define VENDOR_COMMAND          0x89
struct sVENDOR_COMMAND {
  unsigned char  VendorSpecificData[14];
};

#define GIVE_DEVICE_VENDOR_ID   0x8C
struct sGIVE_DEVICE_VENDOR_ID {
};

#define MENU_REQUEST   0x8D
// Directly
struct sMENU_REQUEST {
  tMenuRequestType MenuRequestType;
};

#define MENU_STATUS   0x8E
// Directly
struct sMENU_STATUS {
  tMenuState MenuState;
};

#define GIVE_DEVICE_POWER_STATUS    0x8F
// Directly
// -> REPORT_POWER_STATUS
struct sGIVE_DEVICE_POWER_STATUS {
};

#define REPORT_POWER_STATUS     0x90
// Directly
struct sEPORT_POWER_STATUS {
  tPowerStatus PowerStatus;
};

#define GIVE_MENU_LANGUAGE      0x91
struct sGIVE_MENU_LANGUAGE {
};

#define CEC_VERSION      0x9e
struct sCEC_VERSION {
};

#define GET_CEC_VERSION      0x9f
struct sGET_CEC_VERSION {
};

#define VENDOR_COMMAND_WITH_ID          0xa0
struct sVENDOR_COMMAND_WITH_ID {
  //unsigned char  VendorSpecificData[14];
};

#endif
