/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 5F, No. 36 Taiyuan St.
 * Jhubei City
 * Hsinchu County 302, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2008, Ralink Technology, Inc.
 *
 * This program is free software; you can redistribute it and/or modify  * 
 * it under the terms of the GNU General Public License as published by  * 
 * the Free Software Foundation; either version 2 of the License, or     * 
 * (at your option) any later version.                                   * 
 *                                                                       * 
 * This program is distributed in the hope that it will be useful,       * 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        * 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         * 
 * GNU General Public License for more details.                          * 
 *                                                                       * 
 * You should have received a copy of the GNU General Public License     * 
 * along with this program; if not, write to the                         * 
 * Free Software Foundation, Inc.,                                       * 
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             * 
 *                                                                       * 
 *************************************************************************

	Module Name:
	rtmp_init.c

	Abstract:
	Miniport generic portion header file

	Revision History:
	Who 		When		  What
	--------	----------	  ----------------------------------------------
	Paul Lin	2002-08-01	  created
	John Chang	2004-08-20	  RT2561/2661 use scatter-gather scheme
*/

#include	"rt_config.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
#define RT_USB_ALLOC_URB(iso)	usb_alloc_urb(iso, GFP_ATOMIC);
#else
#define RT_USB_ALLOC_URB(iso)	usb_alloc_urb(iso);
#endif

UCHAR	 BIT8[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
ULONG	 BIT32[] = {0x00000001, 0x00000002, 0x00000004, 0x00000008,
					0x00000010, 0x00000020, 0x00000040, 0x00000080,
					0x00000100, 0x00000200, 0x00000400, 0x00000800,
					0x00001000, 0x00002000, 0x00004000, 0x00008000,
					0x00010000, 0x00020000, 0x00040000, 0x00080000,
					0x00100000, 0x00200000, 0x00400000, 0x00800000,
					0x01000000, 0x02000000, 0x04000000, 0x08000000,
					0x10000000, 0x20000000, 0x40000000, 0x80000000};

char*	CipherName[] = {"none","wep64","wep128","TKIP","AES","CKIP64","CKIP128"};

const unsigned short ccitt_16Table[] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
	0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
	0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
	0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
	0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
	0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
	0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
	0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
	0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
	0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};
#define ByteCRC16(v, crc) \
	(unsigned short)((crc << 8) ^  ccitt_16Table[((crc >> 8) ^ (v)) & 255])


//
// BBP register initialization set
//
BBP_REG_PAIR   BBPRegTable[] = {
	{3, 	0x80},
	{15,	0x30},
	{17,	0x20},
	{21,	0xc8},
	{22,	0x38},
	{23,	0x06},
	{24,	0xfe},
	{25,	0x0a},
	{26,	0x0d},
	{32,	0x0b},
	{34,	0x12},
	{37,	0x07},
	{39,	0xf8}, // 2005-09-02 by Gary, Atheros 11b issue 
	{41,	0x60}, // 03-09 gary
	{53,	0x10}, // 03-09 gary
	{54,	0x18}, // 03-09 gary
	{60,	0x10},
	{61,	0x04},
	{62,	0x04},
	{75,	0xfe},
	{86,	0xfe},
	{88,	0xfe},
	{90,	0x0f},
	{99,	0x00},
	{102,	0x16},
	{107,	0x04},
};
#define	NUM_BBP_REG_PARMS	(sizeof(BBPRegTable) / sizeof(BBP_REG_PAIR))


//
// ASIC register initialization sets
//
RTMP_REG_PAIR	MACRegTable[] =	{
	{TXRX_CSR0, 	0x025fb032}, // 0x3040, RX control, default Disable RX
	{TXRX_CSR1, 	0x9eaa9eaf}, // 0x3044, BBP 30:Ant-A RSSI, R51:Ant-B RSSI, R42:OFDM rate, R47:CCK SIGNAL
	{TXRX_CSR2, 	0x8a8b8c8d}, // 0x3048, CCK TXD BBP registers
	{TXRX_CSR3, 	0x00858687}, // 0x304c, OFDM TXD BBP registers
	{TXRX_CSR7, 	0x2E31353B}, // 0x305c, ACK/CTS payload consume time for 18/12/9/6 mbps    
	{TXRX_CSR8, 	0x2a2a2a2c}, // 0x3060, ACK/CTS payload consume time for 54/48/36/24 mbps
	{TXRX_CSR15,	0x0000000f}, // 0x307c, TKIP MIC priority byte "AND" mask
	{MAC_CSR6,		0x00000fff}, // 0x3018, MAX frame length
	{MAC_CSR8,		0x016c030a}, // 0x3020, SIFS/EIFS time, set SIFS delay time.	
	{MAC_CSR10, 	0x00000718}, // 0x3028, ASIC PIN control in various power states
	{MAC_CSR12, 	0x00000004}, // 0x3030, power state control, set to AWAKE state
	{MAC_CSR13, 	0x00007f00}, // 0x3034, GPIO pin#7 as bHwRadio (input:0), otherwise (output:1)
	{SEC_CSR0,		0x00000000}, // 0x30a0, invalidate all shared key entries
	{SEC_CSR1,		0x00000000}, // 0x30a4, reset all shared key algorithm to "none"
	{SEC_CSR5,		0x00000000}, // 0x30b4, reset all shared key algorithm to "none"
	{PHY_CSR1,		0x000023b0}, // 0x3084, BBP Register R/W mode set to "Parallel mode"	
	{PHY_CSR5,		0x00040a06}, //  0x060a100c
	{PHY_CSR6,		0x00080606},
	{PHY_CSR7,		0x00000408},
	{AIFSN_CSR, 	0x00002273},
	{CWMIN_CSR, 	0x00002344},
	{CWMAX_CSR, 	0x000034aa},
};
#define	NUM_MAC_REG_PARMS	(sizeof(MACRegTable) / sizeof(RTMP_REG_PAIR))


UCHAR FirmwareImage[] = 
{
	//2005/07/22 Suport LED mode #0,#1,#2
	//2005/07/28 add Version control V1.0
	//2005/09/14 Update firmware code to prevent buffer not page out while aggregate.
	//2005/10/04 Firmware support Windows Power Saving.
	//2005/11/03 V1.3 not release, V1.4 improve Aggregation throughput
	//			 V1.4 will cause USB1.0 RX Stuck.
	//			 V1.5 remove RX checking(Special case, fixed on USB1.X Stuck issue)
	//			 V1.6 High throughput & WMM support (base on V1.4) not release
	//2005/11/24 V1.7 prevent USB1.0 Stuck issue. (base on V1.5)
        //2007/11/01 V2.2 Fix 2 bytes single write command failed issue.
    0x02, 0x13, 0x16, 0x12, 0x10, 0xd9, 0x02, 0x12, 0x58, 0x02, 0x13, 0x49, 0x02, 0x13, 0x4b, 0xc0, 

    0xd0, 0x75, 0xd0, 0x18, 0x12, 0x13, 0x4d, 0xd0, 0xd0, 0x22, 0x02, 0x14, 0x7c, 0x02, 0x15, 0x11, 

    0xed, 0x4c, 0x70, 0x44, 0x90, 0x01, 0xa8, 0x74, 0x80, 0xf0, 0xef, 0x30, 0xe5, 0x07, 0xe4, 0x90, 

    0x00, 0x0f, 0xf0, 0x80, 0x2c, 0xe5, 0x40, 0x24, 0xc0, 0x60, 0x13, 0x24, 0xc0, 0x60, 0x16, 0x24, 

    0xc0, 0x60, 0x19, 0x24, 0xc0, 0x70, 0x1a, 0xe4, 0x90, 0x00, 0x0b, 0xf0, 0x80, 0x13, 0xe4, 0x90, 

    0x00, 0x13, 0xf0, 0x80, 0x0c, 0xe4, 0x90, 0x00, 0x1b, 0xf0, 0x80, 0x05, 0xe4, 0x90, 0x00, 0x23, 

    0xf0, 0xe4, 0x90, 0x01, 0xa8, 0xf0, 0xd3, 0x22, 0x90, 0x02, 0x02, 0xed, 0xf0, 0x90, 0x02, 0x01, 

    0xef, 0xf0, 0xd3, 0x22, 0xef, 0x24, 0xc0, 0x60, 0x1f, 0x24, 0xc0, 0x60, 0x2e, 0x24, 0xc0, 0x60, 

    0x3d, 0x24, 0xc0, 0x70, 0x53, 0x90, 0x00, 0x0b, 0xe0, 0x30, 0xe1, 0x02, 0xc3, 0x22, 0x90, 0x00, 

    0x09, 0xe0, 0xfe, 0x90, 0x00, 0x08, 0x80, 0x37, 0x90, 0x00, 0x13, 0xe0, 0x30, 0xe1, 0x02, 0xc3, 

    0x22, 0x90, 0x00, 0x11, 0xe0, 0xfe, 0x90, 0x00, 0x10, 0x80, 0x24, 0x90, 0x00, 0x1b, 0xe0, 0x30, 

    0xe1, 0x02, 0xc3, 0x22, 0x90, 0x00, 0x19, 0xe0, 0xfe, 0x90, 0x00, 0x18, 0x80, 0x11, 0x90, 0x00, 

    0x23, 0xe0, 0x30, 0xe1, 0x02, 0xc3, 0x22, 0x90, 0x00, 0x21, 0xe0, 0xfe, 0x90, 0x00, 0x20, 0xe0, 

    0xfd, 0xee, 0xf5, 0x37, 0xed, 0xf5, 0x38, 0xd3, 0x22, 0x30, 0x09, 0x20, 0x20, 0x04, 0x0b, 0x90, 

    0x02, 0x08, 0xe0, 0x54, 0x0f, 0x70, 0x03, 0x02, 0x12, 0x57, 0xc2, 0x09, 0x90, 0x02, 0x00, 0xe0, 

    0x44, 0x04, 0xf0, 0x74, 0x04, 0x12, 0x0c, 0x3a, 0xc2, 0x04, 0xc2, 0x07, 0x90, 0x02, 0x01, 0xe0, 

    0x30, 0xe0, 0x03, 0x00, 0x80, 0xf6, 0x90, 0x03, 0x26, 0xe0, 0x20, 0xe2, 0x03, 0x02, 0x12, 0x57, 

    0x90, 0x02, 0x08, 0xe0, 0x70, 0x1b, 0x20, 0x07, 0x03, 0x02, 0x12, 0x57, 0x90, 0x03, 0x12, 0xe0, 

    0x64, 0x22, 0x60, 0x03, 0x02, 0x12, 0x57, 0xd2, 0x09, 0xc2, 0x07, 0x74, 0x02, 0x12, 0x0c, 0x3a, 

    0x22, 0x90, 0x02, 0x03, 0xe0, 0x30, 0xe4, 0x47, 0x20, 0x06, 0x44, 0xe5, 0x3c, 0x60, 0x34, 0xe5, 

    0x40, 0x24, 0xc0, 0x60, 0x14, 0x24, 0xc0, 0x60, 0x18, 0x24, 0xc0, 0x60, 0x1c, 0x24, 0xc0, 0x70, 

    0x22, 0x90, 0x00, 0x0b, 0xe0, 0x30, 0xe1, 0x1b, 0x22, 0x90, 0x00, 0x13, 0xe0, 0x30, 0xe1, 0x13, 

    0x22, 0x90, 0x00, 0x1b, 0xe0, 0x30, 0xe1, 0x0b, 0x22, 0x90, 0x00, 0x23, 0xe0, 0x30, 0xe1, 0x03, 

    0x02, 0x12, 0x57, 0x90, 0x02, 0x03, 0x74, 0x01, 0xf0, 0x00, 0xe0, 0x54, 0xc0, 0xf5, 0x40, 0xe5, 

    0x40, 0x24, 0xc0, 0x60, 0x20, 0x24, 0xc0, 0x60, 0x30, 0x24, 0xc0, 0x60, 0x40, 0x24, 0xc0, 0x70, 

    0x56, 0x90, 0x00, 0x0b, 0xe0, 0x30, 0xe1, 0x03, 0x02, 0x12, 0x57, 0x90, 0x00, 0x09, 0xe0, 0xfe, 

    0x90, 0x00, 0x08, 0x80, 0x3a, 0x90, 0x00, 0x13, 0xe0, 0x30, 0xe1, 0x03, 0x02, 0x12, 0x57, 0x90, 

    0x00, 0x11, 0xe0, 0xfe, 0x90, 0x00, 0x10, 0x80, 0x26, 0x90, 0x00, 0x1b, 0xe0, 0x30, 0xe1, 0x03, 

    0x02, 0x12, 0x57, 0x90, 0x00, 0x19, 0xe0, 0xfe, 0x90, 0x00, 0x18, 0x80, 0x12, 0x90, 0x00, 0x23, 

    0xe0, 0x30, 0xe1, 0x03, 0x02, 0x12, 0x57, 0x90, 0x00, 0x21, 0xe0, 0xfe, 0x90, 0x00, 0x20, 0xe0, 

    0xfd, 0xee, 0xf5, 0x37, 0xed, 0xf5, 0x38, 0x90, 0x03, 0x27, 0x74, 0x82, 0xf0, 0x90, 0x02, 0x01, 

    0xe5, 0x40, 0xf0, 0x90, 0x02, 0x06, 0xe0, 0xf5, 0x3c, 0xc3, 0xe5, 0x38, 0x95, 0x3a, 0xe5, 0x37, 

    0x95, 0x39, 0x50, 0x21, 0xe5, 0x40, 0x44, 0x05, 0xff, 0xe5, 0x37, 0xa2, 0xe7, 0x13, 0xfc, 0xe5, 

    0x38, 0x13, 0xfd, 0x12, 0x10, 0x20, 0xe5, 0x3c, 0x30, 0xe2, 0x04, 0xd2, 0x06, 0x80, 0x02, 0xc2, 

    0x06, 0x53, 0x3c, 0x01, 0x22, 0x30, 0x0b, 0x07, 0xe4, 0x90, 0x02, 0x02, 0xf0, 0x80, 0x06, 0x90, 

    0x02, 0x02, 0x74, 0x20, 0xf0, 0xe5, 0x40, 0x44, 0x01, 0x90, 0x02, 0x01, 0xf0, 0x90, 0x02, 0x01, 

    0xe0, 0x30, 0xe0, 0x03, 0x00, 0x80, 0xf6, 0x90, 0x03, 0x27, 0x74, 0x02, 0xf0, 0xaf, 0x40, 0x12, 

    0x10, 0x74, 0x40, 0xa5, 0x00, 0x80, 0xf6, 0x22, 0x90, 0x02, 0x01, 0xe0, 0x30, 0xe0, 0x03, 0x00, 

    0x80, 0xf6, 0x90, 0x03, 0x26, 0xe0, 0x20, 0xe1, 0x07, 0xe5, 0x3b, 0x70, 0x03, 0x02, 0x13, 0x15, 

    0xe5, 0x3b, 0x70, 0x14, 0x90, 0x03, 0x24, 0xe0, 0x75, 0xf0, 0x40, 0xa4, 0xf5, 0x36, 0x85, 0xf0, 

    0x35, 0x7b, 0x83, 0x75, 0x3b, 0x01, 0x80, 0x02, 0x7b, 0x03, 0xd3, 0xe5, 0x36, 0x95, 0x3a, 0xe5, 

    0x35, 0x95, 0x39, 0x40, 0x34, 0x90, 0x02, 0x01, 0xe0, 0x30, 0xe0, 0x03, 0x00, 0x80, 0xf6, 0x90, 

    0x03, 0x27, 0xeb, 0xf0, 0x90, 0x00, 0x0f, 0xe0, 0x30, 0xe1, 0x04, 0x30, 0x0e, 0xf6, 0x22, 0x30, 

    0x0b, 0x07, 0xe4, 0x90, 0x02, 0x02, 0xf0, 0x80, 0x06, 0x90, 0x02, 0x02, 0x74, 0x20, 0xf0, 0x90, 

    0x02, 0x01, 0x74, 0x21, 0xf0, 0x7b, 0x03, 0x80, 0x3c, 0xe5, 0x35, 0xa2, 0xe7, 0x13, 0xfe, 0xe5, 

    0x36, 0x13, 0xfd, 0xac, 0x06, 0x90, 0x02, 0x01, 0xe0, 0x30, 0xe0, 0x03, 0x00, 0x80, 0xf6, 0x90, 

    0x03, 0x27, 0xeb, 0xf0, 0x90, 0x00, 0x0f, 0xe0, 0x30, 0xe1, 0x04, 0x30, 0x0e, 0xf6, 0x22, 0x7f, 

    0x25, 0x12, 0x10, 0x20, 0xe5, 0x36, 0xb5, 0x3a, 0x08, 0xe5, 0x35, 0xb5, 0x39, 0x03, 0x00, 0x80, 

    0x04, 0xe4, 0xf5, 0x3b, 0x22, 0xc3, 0xe5, 0x36, 0x95, 0x3a, 0xf5, 0x36, 0xe5, 0x35, 0x95, 0x39, 

    0xf5, 0x35, 0x02, 0x12, 0x8a, 0x22, 0x75, 0xa8, 0x0f, 0x90, 0x03, 0x06, 0x74, 0x01, 0xf0, 0x90, 

    0x03, 0x07, 0xf0, 0x90, 0x03, 0x08, 0x04, 0xf0, 0x90, 0x03, 0x09, 0x74, 0x6c, 0xf0, 0x90, 0x03, 

    0x0a, 0x74, 0xff, 0xf0, 0x90, 0x03, 0x02, 0x74, 0x1f, 0xf0, 0x90, 0x03, 0x00, 0x74, 0x04, 0xf0, 

    0x90, 0x03, 0x25, 0x74, 0x31, 0xf0, 0xd2, 0xaf, 0x22, 0x00, 0x22, 0x00, 0x22, 0x90, 0x03, 0x05, 

    0xe0, 0x30, 0xe0, 0x0b, 0xe0, 0x44, 0x01, 0xf0, 0x30, 0x09, 0x02, 0xd2, 0x04, 0xc2, 0x07, 0x22, 

    0x8f, 0x24, 0xa9, 0x05, 0x90, 0x7f, 0xfc, 0xe0, 0x75, 0x25, 0x00, 0xf5, 0x26, 0xa3, 0xe0, 0x75, 

    0x27, 0x00, 0xf5, 0x28, 0xa3, 0xe0, 0xff, 0xa3, 0xe0, 0xfd, 0xe5, 0x24, 0x30, 0xe5, 0x19, 0x54, 

    0xc0, 0x60, 0x05, 0x43, 0x05, 0x03, 0x80, 0x08, 0xe4, 0x90, 0x7f, 0xf0, 0xf0, 0x53, 0x05, 0xfc, 

    0xef, 0x54, 0x3f, 0x44, 0x40, 0xff, 0x80, 0x17, 0xe4, 0x90, 0x7f, 0xf0, 0xf0, 0xe9, 0x30, 0xe6, 

    0x08, 0x43, 0x07, 0xc0, 0x43, 0x05, 0x0f, 0x80, 0x06, 0x53, 0x07, 0x3f, 0x53, 0x05, 0xf0, 0xe9, 

    0x30, 0xe6, 0x05, 0x43, 0x05, 0x10, 0x80, 0x03, 0x53, 0x05, 0xef, 0x90, 0x7f, 0xfc, 0xe5, 0x26, 

    0xf0, 0xa3, 0xe5, 0x28, 0xf0, 0xa3, 0xef, 0xf0, 0xa3, 0xed, 0xf0, 0x22, 0x8d, 0x24, 0xa9, 0x07, 

    0x90, 0x7f, 0xfc, 0xe0, 0x75, 0x25, 0x00, 0xf5, 0x26, 0xa3, 0xe0, 0x75, 0x27, 0x00, 0xf5, 0x28, 

    0xa3, 0xe0, 0xff, 0xa3, 0xe0, 0xfd, 0xe9, 0x30, 0xe5, 0x05, 0x43, 0x05, 0x0f, 0x80, 0x03, 0x53, 

    0x05, 0xf0, 0xe9, 0x30, 0xe6, 0x08, 0xef, 0x54, 0x3f, 0x44, 0x40, 0xff, 0x80, 0x03, 0x53, 0x07, 

    0x3f, 0xe5, 0x24, 0x30, 0xe6, 0x05, 0x43, 0x05, 0x10, 0x80, 0x03, 0x53, 0x05, 0xef, 0x90, 0x7f, 

    0xfc, 0xe5, 0x26, 0xf0, 0xa3, 0xe5, 0x28, 0xf0, 0xa3, 0xef, 0xf0, 0xa3, 0xed, 0xf0, 0x22, 0xa9, 

    0x05, 0x90, 0x7f, 0xfc, 0xa3, 0xe0, 0xfd, 0xe9, 0x30, 0xe6, 0x17, 0xef, 0x54, 0xc0, 0x60, 0x0a, 

    0x7e, 0x46, 0x7d, 0x46, 0x7c, 0x45, 0x7b, 0x01, 0x80, 0x1d, 0x7e, 0xfa, 0x7c, 0x85, 0x7b, 0x12, 

    0x80, 0x15, 0xef, 0x54, 0xc0, 0x60, 0x0a, 0x7e, 0x46, 0x7d, 0x46, 0x7c, 0x45, 0x7b, 0x11, 0x80, 

    0x06, 0x7e, 0xfa, 0x7c, 0x85, 0x7b, 0x02, 0xef, 0x20, 0xe5, 0x12, 0xe9, 0x30, 0xe6, 0x08, 0x43, 

    0x04, 0xc0, 0x43, 0x03, 0x0f, 0x80, 0x06, 0x53, 0x04, 0x3f, 0x53, 0x03, 0xf0, 0x90, 0x7f, 0xfc, 

    0xee, 0xf0, 0xa3, 0xed, 0xf0, 0xa3, 0xec, 0xf0, 0xa3, 0xeb, 0xf0, 0x22, 0xe5, 0x4b, 0xfd, 0x54, 

    0x1f, 0x90, 0x7f, 0xf8, 0xf0, 0xe5, 0x4a, 0xf5, 0x09, 0x90, 0x30, 0x38, 0xe0, 0x90, 0x7f, 0xfc, 

    0xf0, 0x90, 0x30, 0x39, 0xe0, 0x90, 0x7f, 0xfd, 0xf0, 0x90, 0x30, 0x3a, 0xe0, 0x90, 0x7f, 0xfe, 

    0xf0, 0x90, 0x30, 0x3b, 0xe0, 0x90, 0x7f, 0xff, 0xf0, 0xed, 0x30, 0xe5, 0x0c, 0x54, 0xc0, 0x60, 

    0x0d, 0x90, 0x7f, 0xf0, 0xe5, 0x47, 0xf0, 0x80, 0x05, 0xe4, 0x90, 0x7f, 0xf0, 0xf0, 0x90, 0x7f, 

    0xf8, 0xe0, 0x14, 0x60, 0x08, 0x24, 0xfe, 0x60, 0x0d, 0x24, 0x03, 0x80, 0x12, 0xaf, 0x05, 0xad, 

    0x09, 0x12, 0x13, 0xcc, 0x80, 0x10, 0xaf, 0x05, 0xad, 0x09, 0x12, 0x14, 0x1f, 0x80, 0x07, 0xaf, 

    0x05, 0xad, 0x09, 0x12, 0x13, 0x60, 0x90, 0x7f, 0xfc, 0xe0, 0x90, 0x30, 0x38, 0xf0, 0x90, 0x7f, 

    0xfd, 0xe0, 0x90, 0x30, 0x39, 0xf0, 0x90, 0x7f, 0xfe, 0xe0, 0x90, 0x30, 0x3a, 0xf0, 0x90, 0x7f, 

    0xff, 0xe0, 0x90, 0x30, 0x3b, 0xf0, 0x90, 0x7f, 0xf8, 0xe0, 0xb4, 0x02, 0x03, 0x12, 0x16, 0x09, 

    0x22, 0xe5, 0x4b, 0x64, 0x01, 0x60, 0x03, 0x02, 0x15, 0x9b, 0xf5, 0x4b, 0xe5, 0x44, 0x45, 0x43, 

    0x70, 0x03, 0x02, 0x15, 0xca, 0x12, 0x0c, 0x14, 0x12, 0x0b, 0x86, 0x50, 0xfb, 0x90, 0x00, 0x00, 

    0xe0, 0xf5, 0x25, 0x12, 0x15, 0xde, 0xc2, 0x92, 0xe4, 0xf5, 0x24, 0xe5, 0x24, 0xc3, 0x95, 0x25, 

    0x50, 0x49, 0x7e, 0x00, 0x7f, 0x4c, 0x74, 0x40, 0x25, 0x24, 0xf5, 0x82, 0xe4, 0x34, 0x01, 0xad, 

    0x82, 0xfc, 0x75, 0x2b, 0x02, 0x7b, 0x10, 0x12, 0x07, 0x1e, 0xc2, 0x93, 0x12, 0x15, 0xcb, 0x7d, 

    0xa0, 0x12, 0x15, 0xfa, 0xe5, 0x24, 0x54, 0x0f, 0x24, 0x4c, 0xf8, 0xe6, 0xfd, 0xaf, 0x4b, 0xae, 

    0x4a, 0x12, 0x16, 0x02, 0x05, 0x4b, 0xe5, 0x4b, 0x70, 0x02, 0x05, 0x4a, 0x12, 0x0a, 0x5f, 0x05, 

    0x24, 0xe5, 0x24, 0x54, 0x0f, 0x70, 0xd5, 0xd2, 0x93, 0x80, 0xb0, 0xc3, 0xe5, 0x44, 0x95, 0x25, 

    0xf5, 0x44, 0xe5, 0x43, 0x94, 0x00, 0xf5, 0x43, 0x02, 0x15, 0x1c, 0x12, 0x15, 0xde, 0xc2, 0x93, 

    0xc2, 0x92, 0x12, 0x15, 0xcb, 0x7d, 0x80, 0x12, 0x15, 0xfa, 0x7d, 0xaa, 0x74, 0x55, 0xff, 0xfe, 

    0x12, 0x16, 0x02, 0x7d, 0x55, 0x7f, 0xaa, 0x7e, 0x2a, 0x12, 0x16, 0x02, 0x7d, 0x30, 0xaf, 0x4b, 

    0xae, 0x4a, 0x12, 0x16, 0x02, 0x12, 0x0a, 0x5f, 0xd2, 0x93, 0x22, 0x7d, 0xaa, 0x74, 0x55, 0xff, 

    0xfe, 0x12, 0x16, 0x02, 0x7d, 0x55, 0x7f, 0xaa, 0x7e, 0x2a, 0x12, 0x16, 0x02, 0x22, 0xad, 0x47, 

    0x7f, 0x34, 0x7e, 0x30, 0x12, 0x16, 0x02, 0x7d, 0xff, 0x7f, 0x35, 0x7e, 0x30, 0x12, 0x16, 0x02, 

    0xe4, 0xfd, 0x7f, 0x37, 0x7e, 0x30, 0x12, 0x16, 0x02, 0x22, 0x74, 0x55, 0xff, 0xfe, 0x12, 0x16, 

    0x02, 0x22, 0x8f, 0x82, 0x8e, 0x83, 0xed, 0xf0, 0x22, 0xe4, 0xfc, 0x90, 0x7f, 0xf0, 0xe0, 0xaf, 

    0x09, 0x14, 0x60, 0x14, 0x14, 0x60, 0x15, 0x14, 0x60, 0x16, 0x14, 0x60, 0x17, 0x14, 0x60, 0x18, 

    0x24, 0x05, 0x70, 0x16, 0xe4, 0xfc, 0x80, 0x12, 0x7c, 0x01, 0x80, 0x0e, 0x7c, 0x03, 0x80, 0x0a, 

    0x7c, 0x07, 0x80, 0x06, 0x7c, 0x0f, 0x80, 0x02, 0x7c, 0x1f, 0xec, 0x6f, 0xf4, 0x54, 0x1f, 0xfc, 

    0x90, 0x30, 0x34, 0xe0, 0x54, 0xe0, 0x4c, 0xfd, 0xa3, 0xe0, 0xfc, 0x43, 0x04, 0x1f, 0x7f, 0x34, 

    0x7e, 0x30, 0x12, 0x16, 0x02, 0xad, 0x04, 0x0f, 0x12, 0x16, 0x02, 0xe4, 0xfd, 0x7f, 0x37, 0x02, 

    0x16, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 

};

#define	FIRMAREIMAGE_LENGTH		(sizeof (FirmwareImage) / sizeof(UCHAR))
#define FIRMWARE_MAJOR_VERSION	2
#define FIRMWARE_MINOR_VERSION	2

VOID CreateThreads( struct net_device *net_dev )
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER) RTMP_OS_NETDEV_GET_PRIV(net_dev);
	pid_t pid_number = -1;
	
	// Creat MLME Thread
	pAd->MLMEThr_pid= THREAD_PID_INIT_VALUE;

	pid_number = kernel_thread(MlmeThread, pAd, CLONE_VM);
	if (pid_number < 0) {
		printk (KERN_WARNING "%s: unable to start mlme thread\n",pAd->net_dev->name);
	}

	pAd->MLMEThr_pid=GET_PID(pid_number);

	/* Wait for the thread to start */
	wait_for_completion(&(pAd->MlmeThreadNotify));

	// Creat Command Thread
	pAd->RTUSBCmdThr_pid= THREAD_PID_INIT_VALUE;

	 pid_number = kernel_thread(RTUSBCmdThread, pAd, CLONE_VM);
	if (pid_number < 0) {
		printk (KERN_WARNING "%s: unable to start RTUSBCmd thread\n",pAd->net_dev->name);
	}

	pAd->RTUSBCmdThr_pid=GET_PID(pid_number);

	/* Wait for the thread to start */
	wait_for_completion(&(pAd->CmdThreadNotify));
}

NDIS_STATUS NICInitTransmit(
	IN	PRTMP_ADAPTER	 pAd )
{
	UCHAR			i, acidx;
	NDIS_STATUS 	Status = NDIS_STATUS_SUCCESS;
	PTX_CONTEXT		pPsPollContext = &(pAd->PsPollContext);
	PTX_CONTEXT		pNullContext   = &(pAd->NullContext);
	PTX_CONTEXT		pRTSContext    = &(pAd->RTSContext);

	DBGPRINT(RT_DEBUG_TRACE,"--> NICInitTransmit\n");
	
	// Init 4 set of Tx parameters
	for (i = 0; i < 4; i++)
	{
		// Initialize all Transmit releated queues
		InitializeQueueHeader(&pAd->SendTxWaitQueue[i]);
		
		pAd->NextTxIndex[i]			= 0;		// Next Free local Tx ring pointer	
		pAd->TxRingTotalNumber[i]	= 0;
		pAd->NextBulkOutIndex[i]	= 0;		// Next Local tx ring pointer waiting for buck out
		pAd->BulkOutPending[i]		= FALSE;	// Buck Out control flag	
	}
	
	pAd->PrivateInfo.TxRingFullCnt = 0;
	
	pAd->NextRxBulkInIndex	   = 0;	// Rx Bulk pointer
	pAd->NextMLMEIndex		   = 0;
	pAd->PushMgmtIndex		   = 0;
	pAd->PopMgmtIndex		   = 0;
	atomic_set(&pAd->MgmtQueueSize, 0);
	atomic_set(&pAd->TxCount, 0);
	
	pAd->PrioRingFirstIndex    = 0;
	pAd->PrioRingTxCnt		   = 0;
	
	do
	{		
		//
		// TX_RING_SIZE
		//
		for (acidx = 0; acidx < 4; acidx++)
		{
			for ( i= 0; i < TX_RING_SIZE; i++ )
			{
				PTX_CONTEXT pTxContext = &(pAd->TxContext[acidx][i]);
			
				//Allocate URB
				pTxContext->pUrb = RT_USB_ALLOC_URB(0);
				if(pTxContext->pUrb == NULL){
					Status = NDIS_STATUS_RESOURCES;
					goto done;
				}
				pTxContext->TransferBuffer= (PTX_BUFFER) kmalloc(sizeof(TX_BUFFER), GFP_KERNEL);
				Status = NDIS_STATUS_SUCCESS;
				if(!pTxContext->TransferBuffer){
					DBGPRINT(RT_DEBUG_ERROR,"Not enough memory\n");
					Status = NDIS_STATUS_RESOURCES;
					goto out1;
				}

				NdisZeroMemory(pTxContext->TransferBuffer, sizeof(TX_BUFFER));

				pTxContext->pAd = pAd;
				pTxContext->InUse = FALSE;
				pTxContext->IRPPending = FALSE;
			}
		}

		
		//
		// PRIO_RING_SIZE
		//
		for ( i= 0; i < PRIO_RING_SIZE; i++ )
		{
			PTX_CONTEXT	pMLMEContext = &(pAd->MLMEContext[i]);
			
			pMLMEContext->pUrb = RT_USB_ALLOC_URB(0);
			if(pMLMEContext->pUrb == NULL){
				Status = NDIS_STATUS_RESOURCES;
				goto out1;
			}
			
			pMLMEContext->TransferBuffer= (PTX_BUFFER) kmalloc(sizeof(TX_BUFFER), GFP_KERNEL);
			if(!pMLMEContext->TransferBuffer){
				DBGPRINT(RT_DEBUG_ERROR,"Not enough memory\n");
				Status = NDIS_STATUS_RESOURCES;
				goto out2;
			}
			
			NdisZeroMemory(pMLMEContext->TransferBuffer, sizeof(TX_BUFFER));
			pMLMEContext->pAd = pAd ;
			pMLMEContext->InUse = FALSE;
			pMLMEContext->IRPPending = FALSE;
		}

				
		//
		// BEACON_RING_SIZE
		//
		for (i = 0; i < BEACON_RING_SIZE; i++)
		{
			PTX_CONTEXT	pBeaconContext = &(pAd->BeaconContext[i]);
			pBeaconContext->pUrb = RT_USB_ALLOC_URB(0);
			if(pBeaconContext->pUrb == NULL){
				Status = NDIS_STATUS_RESOURCES;
				goto out2;
			}
			
			pBeaconContext->TransferBuffer= (PTX_BUFFER) kmalloc(sizeof(TX_BUFFER), GFP_KERNEL);
			if(!pBeaconContext->TransferBuffer){
				DBGPRINT(RT_DEBUG_ERROR,"Not enough memory\n");
				Status = NDIS_STATUS_RESOURCES;
				goto out3;
			}
			NdisZeroMemory(pBeaconContext->TransferBuffer, sizeof(TX_BUFFER));

			pBeaconContext->pAd = pAd;
			pBeaconContext->InUse = FALSE;
			pBeaconContext->IRPPending = FALSE;
		}

				
		//
		// NullContext
		//
		pNullContext->pUrb = RT_USB_ALLOC_URB(0);
		if(pNullContext->pUrb == NULL){
			Status = NDIS_STATUS_RESOURCES;
			goto out3;
		}		 
		
		pNullContext->TransferBuffer= (PTX_BUFFER) kmalloc(sizeof(TX_BUFFER), GFP_KERNEL);
		if(!pNullContext->TransferBuffer){
			DBGPRINT(RT_DEBUG_ERROR,"Not enough memory\n");
			Status = NDIS_STATUS_RESOURCES;
			goto out4;
		}

		NdisZeroMemory(pNullContext->TransferBuffer, sizeof(TX_BUFFER));
		pNullContext->pAd = pAd;
		pNullContext->InUse = FALSE;
		pNullContext->IRPPending = FALSE;
				
		//
		// RTSContext
		//
		pRTSContext->pUrb = RT_USB_ALLOC_URB(0);
		if(pRTSContext->pUrb == NULL){
			Status = NDIS_STATUS_RESOURCES;
			goto out4;
		}		 
		
		pRTSContext->TransferBuffer= (PTX_BUFFER) kmalloc(sizeof(TX_BUFFER), GFP_KERNEL);
		if(!pRTSContext->TransferBuffer){
			DBGPRINT(RT_DEBUG_ERROR,"Not enough memory\n");
			Status = NDIS_STATUS_RESOURCES;
			goto out5;
		}

		NdisZeroMemory(pRTSContext->TransferBuffer, sizeof(TX_BUFFER));
		pRTSContext->pAd = pAd;
		pRTSContext->InUse = FALSE;
		pRTSContext->IRPPending = FALSE; 


		//
		// PsPollContext
		// 
		pPsPollContext->pUrb = RT_USB_ALLOC_URB(0);
		if(pPsPollContext->pUrb == NULL){
			Status = NDIS_STATUS_RESOURCES;
			goto out5;
		}		 
		
		pPsPollContext->TransferBuffer= (PTX_BUFFER) kmalloc(sizeof(TX_BUFFER), GFP_KERNEL);
		if(!pPsPollContext->TransferBuffer){
			DBGPRINT(RT_DEBUG_ERROR,"Not enough memory\n");
			Status = NDIS_STATUS_RESOURCES;
			goto out6;
		}

		NdisZeroMemory(pPsPollContext->TransferBuffer, sizeof(TX_BUFFER));
		pPsPollContext->pAd = pAd;
		pPsPollContext->InUse = FALSE;
		pPsPollContext->IRPPending = FALSE;

	}  while (FALSE);
	
	return Status;


out6:	
	if (NULL != pPsPollContext->pUrb)
	{
		RTUSB_UNLINK_URB(pPsPollContext->pUrb);
		usb_free_urb(pPsPollContext->pUrb);
		pPsPollContext->pUrb = NULL;
	}
	if (NULL != pPsPollContext->TransferBuffer)
	{
		kfree(pPsPollContext->TransferBuffer);
		pPsPollContext->TransferBuffer = NULL;
	}
out5:
	if (NULL != pRTSContext->pUrb)
	{
		RTUSB_UNLINK_URB(pRTSContext->pUrb);
		usb_free_urb(pRTSContext->pUrb);
		pRTSContext->pUrb = NULL;
	}
	if (NULL != pRTSContext->TransferBuffer)
	{
		kfree(pRTSContext->TransferBuffer);
		pRTSContext->TransferBuffer = NULL;
	}	
out4:	
	if (NULL != pNullContext->pUrb)
	{
		RTUSB_UNLINK_URB(pNullContext->pUrb);
		usb_free_urb(pNullContext->pUrb);
		pNullContext->pUrb = NULL;
	}
	if (NULL != pNullContext->TransferBuffer)
	{
		kfree(pNullContext->TransferBuffer);
		pNullContext->TransferBuffer = NULL;
	}
out3:	
	for (i = 0; i < BEACON_RING_SIZE; i++)
	{
		PTX_CONTEXT	pBeaconContext = &(pAd->BeaconContext[i]);
		if ( NULL != pBeaconContext->pUrb )
		{
			RTUSB_UNLINK_URB(pBeaconContext->pUrb);
			usb_free_urb(pBeaconContext->pUrb);
			pBeaconContext->pUrb = NULL;
		}
		
		if ( NULL != pBeaconContext->TransferBuffer )
		{
			kfree( pBeaconContext->TransferBuffer);
			pBeaconContext->TransferBuffer = NULL;
		}
	}
out2:	
	for ( i= 0; i < PRIO_RING_SIZE; i++ )
	{
		PTX_CONTEXT pMLMEContext = &(pAd->MLMEContext[i]);
		
		if ( NULL != pMLMEContext->pUrb )
		{
			RTUSB_UNLINK_URB(pMLMEContext->pUrb);
			usb_free_urb(pMLMEContext->pUrb);
			pMLMEContext->pUrb = NULL;
		}
		
		if ( NULL != pMLMEContext->TransferBuffer )
		{
			kfree( pMLMEContext->TransferBuffer);
			pMLMEContext->TransferBuffer = NULL;
		}
	}
out1:
	for (acidx = 0; acidx < 4; acidx++)
	{
		for ( i= 0; i < TX_RING_SIZE; i++ )
		{
			PTX_CONTEXT pTxContext = &(pAd->TxContext[acidx][i]);

			if ( NULL != pTxContext->pUrb )
			{
				RTUSB_UNLINK_URB(pTxContext->pUrb);
				usb_free_urb(pTxContext->pUrb);
				pTxContext->pUrb = NULL;
			}
			if ( NULL != pTxContext->TransferBuffer )
			{
				kfree( pTxContext->TransferBuffer);
				pTxContext->TransferBuffer = NULL;
			}
		}
	}
	
done:
	return Status;
}

/*
	========================================================================
	
	Routine Description:
		Initialize receive data structures

	Arguments:
		Adapter 					Pointer to our adapter

	Return Value:
		NDIS_STATUS_SUCCESS
		NDIS_STATUS_RESOURCES

	Note:
		Initialize all receive releated private buffer, include those define
		in RTMP_ADAPTER structure and all private data structures. The mahor
		work is to allocate buffer for each packet and chain buffer to 
		NDIS packet descriptor.
		
	========================================================================
*/
NDIS_STATUS NICInitRecv(
	IN	PRTMP_ADAPTER	pAd)
{
	UCHAR	i;
	NDIS_STATUS 	Status = NDIS_STATUS_SUCCESS;


	DBGPRINT(RT_DEBUG_TRACE,"--> NICInitRecv\n");
	pAd->NextRxBulkInIndex = 0;
	atomic_set( &pAd->PendingRx, 0);
	for (i = 0; i < RX_RING_SIZE; i++)
	{
		PRX_CONTEXT  pRxContext = &(pAd->RxContext[i]);
		pRxContext->pUrb = RT_USB_ALLOC_URB(0);
		if(pRxContext->pUrb == NULL)
		{
			Status = NDIS_STATUS_RESOURCES;
			DBGPRINT(RT_DEBUG_TRACE,"--> pRxContext->pUrb == NULL\n");
			break;
		}
						
		pRxContext->TransferBuffer= (PUCHAR) kmalloc(BUFFER_SIZE, MEM_ALLOC_FLAG);
		if(!pRxContext->TransferBuffer)
		{
			DBGPRINT(RT_DEBUG_ERROR,"Not enough memory\n");
			Status = NDIS_STATUS_RESOURCES;
			break;
		}	

		NdisZeroMemory(pRxContext->TransferBuffer, BUFFER_SIZE);
		
		pRxContext->pAd	= pAd;
		pRxContext->InUse = FALSE;
		pRxContext->IRPPending	= FALSE;
	}

	DBGPRINT(RT_DEBUG_TRACE,"<-- NICInitRecv\n");	
	return Status;
}

////////////////////////////////////////////////////////////////////////////
//
//	FUNCTION
//		ReleaseAdapter
//
//	DESCRIPTION
//		Calls USB_InterfaceStop and frees memory allocated for the URBs
//		calls NdisMDeregisterDevice and frees the memory
//		allocated in VNetInitialize for the Adapter Object
//		
//	INPUT
//		Adapter 	Pointer to RTMP_ADAPTER structure
//
//	OUTPUT
//		-
//		
////////////////////////////////////////////////////////////////////////////
VOID ReleaseAdapter(
	IN	PRTMP_ADAPTER pAd, 
    IN  BOOLEAN         IsFree,
	IN  BOOLEAN         IsOnlyTx) 
{
	UINT			i, acidx;
	PTX_CONTEXT		pNullContext   = &pAd->NullContext;
	PTX_CONTEXT		pPsPollContext = &pAd->PsPollContext;
	PTX_CONTEXT 	pRTSContext    = &pAd->RTSContext;

	DBGPRINT(RT_DEBUG_TRACE, "---> ReleaseAdapter\n");
	
    if (!IsOnlyTx)
    {
	    // Free all resources for the RECEIVE buffer queue.
	    for (i = 0; i < RX_RING_SIZE; i++)
	    {
		    PRX_CONTEXT  pRxContext = &(pAd->RxContext[i]);

		    if (pRxContext->pUrb != NULL)
		    {
			    RTUSB_UNLINK_URB(pRxContext->pUrb);
			    if (IsFree)
			    usb_free_urb(pRxContext->pUrb);
			    pRxContext->pUrb = NULL;
		    }
		    if (pRxContext->TransferBuffer != NULL)
		    {
			    kfree(pRxContext->TransferBuffer); 
			    pRxContext->TransferBuffer = NULL;
		    }

	    }
    }

	// Free PsPoll frame resource
	if (NULL != pPsPollContext->pUrb)
	{
		RTUSB_UNLINK_URB(pPsPollContext->pUrb);
		if (IsFree)
		usb_free_urb(pPsPollContext->pUrb);
		pPsPollContext->pUrb = NULL;
	}
	if (NULL != pPsPollContext->TransferBuffer)
	{
		kfree(pPsPollContext->TransferBuffer);
		pPsPollContext->TransferBuffer = NULL;
	}
	
	// Free NULL frame resource
	if (NULL != pNullContext->pUrb)
	{
		RTUSB_UNLINK_URB(pNullContext->pUrb);
		if (IsFree)
		usb_free_urb(pNullContext->pUrb);
		pNullContext->pUrb = NULL;
	}
	if (NULL != pNullContext->TransferBuffer)
	{
		kfree(pNullContext->TransferBuffer);
		pNullContext->TransferBuffer = NULL;
	}
	
	// Free RTS frame resource
	if (NULL != pRTSContext->pUrb)
	{
		RTUSB_UNLINK_URB(pRTSContext->pUrb);
		if (IsFree)
		usb_free_urb(pRTSContext->pUrb);
		pRTSContext->pUrb = NULL;
	}
	if (NULL != pRTSContext->TransferBuffer)
	{
		kfree(pRTSContext->TransferBuffer);
		pRTSContext->TransferBuffer = NULL;
	}

	// Free beacon frame resource
	for (i = 0; i < BEACON_RING_SIZE; i++)
	{
		PTX_CONTEXT	pBeaconContext = &(pAd->BeaconContext[i]);
		if ( NULL != pBeaconContext->pUrb )
		{
			RTUSB_UNLINK_URB(pBeaconContext->pUrb);
			if (IsFree)
			usb_free_urb(pBeaconContext->pUrb);
			pBeaconContext->pUrb = NULL;
		}
		
		if ( NULL != pBeaconContext->TransferBuffer )
		{
			kfree( pBeaconContext->TransferBuffer);
			pBeaconContext->TransferBuffer = NULL;
		}
	}

	// Free Prio frame resource
	for ( i= 0; i < PRIO_RING_SIZE; i++ )
	{
		PTX_CONTEXT pMLMEContext = &(pAd->MLMEContext[i]);
		
		if ( NULL != pMLMEContext->pUrb )
		{
			RTUSB_UNLINK_URB(pMLMEContext->pUrb);
			if (IsFree)
			usb_free_urb(pMLMEContext->pUrb);
			pMLMEContext->pUrb = NULL;
		}
		
		if ( NULL != pMLMEContext->TransferBuffer )
		{
			kfree( pMLMEContext->TransferBuffer);
			pMLMEContext->TransferBuffer = NULL;
		}
	}
	
	// Free Tx frame resource
	for (acidx = 0; acidx < 4; acidx++)
	{
		for ( i= 0; i < TX_RING_SIZE; i++ )
		{
			PTX_CONTEXT pTxContext = &(pAd->TxContext[acidx][i]);

			if ( NULL != pTxContext->pUrb )
			{
				RTUSB_UNLINK_URB(pTxContext->pUrb);
				if (IsFree)
				usb_free_urb(pTxContext->pUrb);
				pTxContext->pUrb = NULL;
			}
		
			if ( NULL != pTxContext->TransferBuffer )
			{
				kfree( pTxContext->TransferBuffer);
				pTxContext->TransferBuffer = NULL;
			}
		}
	}

	DBGPRINT(RT_DEBUG_TRACE, "<--- ReleaseAdapter\n");

}

/*
	========================================================================
	
	Routine Description:
		Allocate DMA memory blocks for send, receive

	Arguments:
		Adapter		Pointer to our adapter

	Return Value:
		NDIS_STATUS_SUCCESS
		NDIS_STATUS_FAILURE
		NDIS_STATUS_RESOURCES

	Note:
	
	========================================================================
*/
NDIS_STATUS	RTMPInitAdapterBlock(
	IN	PRTMP_ADAPTER	pAd)
{
	NDIS_STATUS		Status=NDIS_STATUS_SUCCESS;
	UINT			i;
	PCmdQElmt		cmdqelmt;

	DBGPRINT(RT_DEBUG_TRACE, "--> RTMPInitAdapterBlock\n");

	// init counter
	pAd->WlanCounters.TransmittedFragmentCount.vv.LowPart =  0;
	pAd->WlanCounters.MulticastTransmittedFrameCount.vv.LowPart =0;
	pAd->WlanCounters.FailedCount.vv.LowPart =0;
	pAd->WlanCounters.NoRetryCount.vv.LowPart =0;
	pAd->WlanCounters.RetryCount.vv.LowPart =0;
	pAd->WlanCounters.MultipleRetryCount.vv.LowPart =0;
	pAd->WlanCounters.RTSSuccessCount.vv.LowPart =0;
	pAd->WlanCounters.RTSFailureCount.vv.LowPart =0;
	pAd->WlanCounters.ACKFailureCount.vv.LowPart =0;
	pAd->WlanCounters.FrameDuplicateCount.vv.LowPart =0;
	pAd->WlanCounters.ReceivedFragmentCount.vv.LowPart =0;
	pAd->WlanCounters.MulticastReceivedFrameCount.vv.LowPart =0;
	pAd->WlanCounters.FCSErrorCount.vv.LowPart =0;

	pAd->WlanCounters.TransmittedFragmentCount.vv.HighPart =  0;
	pAd->WlanCounters.MulticastTransmittedFrameCount.vv.HighPart =0;
	pAd->WlanCounters.FailedCount.vv.HighPart =0;
	pAd->WlanCounters.NoRetryCount.vv.HighPart =0;
	pAd->WlanCounters.RetryCount.vv.HighPart =0;
	pAd->WlanCounters.MultipleRetryCount.vv.HighPart =0;
	pAd->WlanCounters.RTSSuccessCount.vv.HighPart =0;
	pAd->WlanCounters.RTSFailureCount.vv.HighPart =0;
	pAd->WlanCounters.ACKFailureCount.vv.HighPart =0;
	pAd->WlanCounters.FrameDuplicateCount.vv.HighPart =0;
	pAd->WlanCounters.ReceivedFragmentCount.vv.HighPart =0;
	pAd->WlanCounters.MulticastReceivedFrameCount.vv.HighPart =0;
	pAd->WlanCounters.FCSErrorCount.vv.HighPart =0;
	//2008/01/07:KH add to solve the racing condition of Mac Registers
	pAd->MacRegWrite_Processing=0;
	do
	{
		for (i = 0; i < COMMAND_QUEUE_SIZE; i++)
		{
			cmdqelmt = &(pAd->CmdQElements[i]);
			NdisZeroMemory(cmdqelmt, sizeof(CmdQElmt));
			cmdqelmt->buffer = NULL;
			cmdqelmt->CmdFromNdis = FALSE;
			cmdqelmt->InUse = FALSE;
		}
		RTUSBInitializeCmdQ(&pAd->CmdQ);
		//Initial semaphores and locked it at start.
		init_MUTEX(&(pAd->usbdev_semaphore));
		init_MUTEX_LOCKED(&(pAd->mlme_semaphore));
		init_MUTEX_LOCKED(&(pAd->RTUSBCmd_semaphore));
		//2008/01/07:KH add to solve the racing condition of Mac Registers
		init_MUTEX_LOCKED(&(pAd->MaCRegWrite_semaphore));
		init_completion (&pAd->MlmeThreadNotify); 	// event initially non-signalled
		init_completion (&pAd->CmdThreadNotify); 	// event initially non-signalled
		//2008/01/07:KH add to solve the racing condition of Mac Registers
		
		RTUSBMacRegUp(pAd);
		////////////////////////
		// Spinlock
		NdisAllocateSpinLock(&pAd->MLMEQLock);
		NdisAllocateSpinLock(&pAd->BulkOutLock[0]);
		NdisAllocateSpinLock(&pAd->BulkOutLock[1]);
		NdisAllocateSpinLock(&pAd->BulkOutLock[2]);
		NdisAllocateSpinLock(&pAd->BulkOutLock[3]);
		NdisAllocateSpinLock(&pAd->CmdQLock);
		NdisAllocateSpinLock(&pAd->SendTxWaitQueueLock[0]);
		NdisAllocateSpinLock(&pAd->SendTxWaitQueueLock[1]);
		NdisAllocateSpinLock(&pAd->SendTxWaitQueueLock[2]);
		NdisAllocateSpinLock(&pAd->SendTxWaitQueueLock[3]);
		NdisAllocateSpinLock(&pAd->DeQueueLock[0]);
		NdisAllocateSpinLock(&pAd->DeQueueLock[1]);
		NdisAllocateSpinLock(&pAd->DeQueueLock[2]);
		NdisAllocateSpinLock(&pAd->DeQueueLock[3]);
		NdisAllocateSpinLock(&pAd->DataQLock[0]);
		NdisAllocateSpinLock(&pAd->DataQLock[1]);
		NdisAllocateSpinLock(&pAd->DataQLock[2]);
		NdisAllocateSpinLock(&pAd->DataQLock[3]);
		
		NdisAllocateSpinLock(&pAd->MLMEWaitQueueLock);
		NdisAllocateSpinLock(&pAd->DeMGMTQueueLock);

		NdisAllocateSpinLock(&pAd->TxRingLock);//BensonLiu modify

	}	while (FALSE);

	DBGPRINT(RT_DEBUG_TRACE, "<-- RTMPInitAdapterBlock\n");
	
	return Status;
}

NDIS_STATUS	RTUSBWriteHWMACAddress(
	IN	PRTMP_ADAPTER		pAd)
{
	MAC_CSR2_STRUC		StaMacReg0;
	MAC_CSR3_STRUC		StaMacReg1;
	NDIS_STATUS			Status = NDIS_STATUS_SUCCESS;

	if (pAd->bLocalAdminMAC != TRUE)
	{
		pAd->CurrentAddress[0] = pAd->PermanentAddress[0];
		pAd->CurrentAddress[1] = pAd->PermanentAddress[1];
		pAd->CurrentAddress[2] = pAd->PermanentAddress[2];
		pAd->CurrentAddress[3] = pAd->PermanentAddress[3];
		pAd->CurrentAddress[4] = pAd->PermanentAddress[4];
		pAd->CurrentAddress[5] = pAd->PermanentAddress[5];
	}
	// Write New MAC address to MAC_CSR2 & MAC_CSR3 & let ASIC know our new MAC
	StaMacReg0.field.Byte0 = pAd->CurrentAddress[0];
	StaMacReg0.field.Byte1 = pAd->CurrentAddress[1];
	StaMacReg0.field.Byte2 = pAd->CurrentAddress[2];
	StaMacReg0.field.Byte3 = pAd->CurrentAddress[3];
	StaMacReg1.field.Byte4 = pAd->CurrentAddress[4];
	StaMacReg1.field.Byte5 = pAd->CurrentAddress[5];
	StaMacReg1.field.U2MeMask = 0xff;

	DBGPRINT_RAW(RT_DEBUG_TRACE, "Local MAC = %02x:%02x:%02x:%02x:%02x:%02x\n",
			pAd->CurrentAddress[0], pAd->CurrentAddress[1], pAd->CurrentAddress[2],
			pAd->CurrentAddress[3], pAd->CurrentAddress[4], pAd->CurrentAddress[5]);

	RTUSBWriteMACRegister(pAd, MAC_CSR2, StaMacReg0.word);
	RTUSBWriteMACRegister(pAd, MAC_CSR3, StaMacReg1.word);

	return Status;
}

/*
	========================================================================
	
	Routine Description:
		Read initial parameters from EEPROM
		
	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		None

	Note:
		
	========================================================================
*/
VOID NICReadEEPROMParameters(
	IN	PRTMP_ADAPTER	pAd)
{
	USHORT					i, value, value2;
	EEPROM_ANTENNA_STRUC	Antenna;
	EEPROM_VERSION_STRUC	Version;
	CHAR					ChannelTxPower[MAX_NUM_OF_CHANNELS];
	EEPROM_LED_STRUC		LedSetting;
	UCHAR					MacAddr[MAC_ADDR_LEN];
	UCHAR					DefaultValue[2*NUM_EEPROM_BBP_PARMS];

	DBGPRINT(RT_DEBUG_TRACE, "--> NICReadEEPROMParameters\n");

	//Read MAC address.
	RTUSBReadEEPROM(pAd, EEPROM_MAC_ADDRESS_BASE_OFFSET, MacAddr, MAC_ADDR_LEN);
	NdisMoveMemory( pAd->PermanentAddress, MacAddr, MAC_ADDR_LEN);
	DBGPRINT_RAW(RT_DEBUG_TRACE, "Local MAC = %02x:%02x:%02x:%02x:%02x:%02x\n",
			pAd->PermanentAddress[0], pAd->PermanentAddress[1], pAd->PermanentAddress[2],
			pAd->PermanentAddress[3], pAd->PermanentAddress[4], pAd->PermanentAddress[5]);

	// Init the channel number for TX channel power
	// 0. 11b/g
	for (i = 0; i < 14; i++)
		pAd->TxPower[i].Channel = i + 1;
	// 1. UNI 36 - 64
	for (i = 0; i < 8; i++)
		pAd->TxPower[i + 14].Channel = 36 + i * 4;
	// 2. HipperLAN 2 100 - 140
	for (i = 0; i < 11; i++)
		pAd->TxPower[i + 22].Channel = 100 + i * 4;
	// 3. UNI 140 - 165
	for (i = 0; i < 5; i++)
		pAd->TxPower[i + 33].Channel = 149 + i * 4; 	   

	// 34/38/42/46
	for (i = 0; i < 4; i++)
		pAd->TxPower[i + 38].Channel = 34 + i * 4;

	// if E2PROM version mismatch with driver's expectation, then skip
	// all subsequent E2RPOM retieval and set a system error bit to notify GUI
	RTUSBReadEEPROM(pAd, EEPROM_VERSION_OFFSET, (PUCHAR)&Version.word, 2);
	pAd->EepromVersion = Version.field.Version + Version.field.FaeReleaseNumber * 256;
	DBGPRINT(RT_DEBUG_TRACE, "E2PROM: Version = %d, FAE release #%d\n", Version.field.Version, Version.field.FaeReleaseNumber);

	// Read BBP default value from EEPROM and store to array(EEPROMDefaultValue) in pAd
	RTUSBReadEEPROM(pAd, EEPROM_BBP_BASE_OFFSET, DefaultValue, 2 * NUM_EEPROM_BBP_PARMS);
	NdisMoveMemory((PUCHAR)(pAd->EEPROMDefaultValue), DefaultValue, 2 * NUM_EEPROM_BBP_PARMS);

	// We have to parse NIC configuration 0 at here.
	// If TSSI did not have preloaded value, it should reset the TxAutoAgc to false
	// Therefore, we have to read TxAutoAgc control beforehand.
	// Read Tx AGC control bit

#ifdef BIG_ENDIAN
	Antenna.word = SWAP16(pAd->EEPROMDefaultValue[0]);
#else
	Antenna.word = pAd->EEPROMDefaultValue[0];
#endif

	if (Antenna.field.DynamicTxAgcControl == 1)
		pAd->bAutoTxAgcA = pAd->bAutoTxAgcG = TRUE;
	else
		pAd->bAutoTxAgcA = pAd->bAutoTxAgcG = FALSE;		

	//
	// Reset PhyMode if we don't support 802.11a
	//
	if ((pAd->PortCfg.PhyMode == PHY_11ABG_MIXED) || (pAd->PortCfg.PhyMode == PHY_11A))
	{
		//
		// Only RFIC_5226, RFIC_5225 suport 11a
		//
		if ((Antenna.field.RfIcType == RFIC_2528) || (Antenna.field.RfIcType == RFIC_2527) ||
		    (Antenna.field.RfIcType == RFIC_3020))
			pAd->PortCfg.PhyMode = PHY_11BG_MIXED;

		//
		// Reset Adhoc Mode if we don't support 802.11a
		//
		if ((pAd->PortCfg.AdhocMode == ADHOC_11A) || (pAd->PortCfg.AdhocMode == ADHOC_11ABG_MIXED))
		{
			//
			// Only RFIC_5226, RFIC_5225 suport 11a
			//
			if ((Antenna.field.RfIcType == RFIC_2528) || (Antenna.field.RfIcType == RFIC_2527) ||
			    (Antenna.field.RfIcType == RFIC_3020))
				pAd->PortCfg.AdhocMode = ADHOC_11BG_MIXED;
		}

    }

	
	// Read Tx power value for all 14 channels
	// Value from 1 - 0x7f. Default value is 24.
	// 0. 11b/g
	// Power value 0xFA (-6) ~ 0x24 (36)
	RTUSBReadEEPROM(pAd, EEPROM_G_TX_PWR_OFFSET, ChannelTxPower, 2 * NUM_EEPROM_TX_G_PARMS);
	for (i = 0; i < 2 * NUM_EEPROM_TX_G_PARMS; i++)
	{
		if ((ChannelTxPower[i] > 36) || (ChannelTxPower[i] < -6))
			pAd->TxPower[i].Power = 24;			
		else
			pAd->TxPower[i].Power = ChannelTxPower[i];

		DBGPRINT_RAW(RT_DEBUG_INFO, "Tx power for channel %d : %0x\n", pAd->TxPower[i].Channel, pAd->TxPower[i].Power);
	}

	// 1. UNI 36 - 64, HipperLAN 2 100 - 140, UNI 140 - 165
	// Power value 0xFA (-6) ~ 0x24 (36)
	RTUSBReadEEPROM(pAd, EEPROM_A_TX_PWR_OFFSET, ChannelTxPower, MAX_NUM_OF_A_CHANNELS);
	for (i = 0; i < MAX_NUM_OF_A_CHANNELS; i++)
	{
		if ((ChannelTxPower[i] > 36) || (ChannelTxPower[i] < -6))
			pAd->TxPower[i + 14].Power = 24;
		else			
			pAd->TxPower[i + 14].Power = ChannelTxPower[i];
		DBGPRINT_RAW(RT_DEBUG_INFO, "Tx power for channel %d : %0x\n", pAd->TxPower[i + 14].Channel, pAd->TxPower[i + 14].Power);
	}

	//
	// Please note, we must skip frist value, so we get TxPower as ChannelTxPower[i + 1];
	// because the TxPower was stored from 0x7D, but we need to read EEPROM from 0x7C. (Word alignment)
	//
	// for J52, 34/38/42/46
	RTUSBReadEEPROM(pAd, EEPROM_J52_TX_PWR_OFFSET, ChannelTxPower, 6); //must Read even valuse

	for (i = 0; i < 4; i++)
	{
		ASSERT(pAd->TxPower[J52_CHANNEL_START_OFFSET + i].Channel == 34 + i * 4);
		if ((ChannelTxPower[i] > 36) || (ChannelTxPower[i] < -6))
			pAd->TxPower[J52_CHANNEL_START_OFFSET + i].Power = 24;
		else			
			pAd->TxPower[J52_CHANNEL_START_OFFSET + i].Power = ChannelTxPower[i + 1];

		DBGPRINT_RAW(RT_DEBUG_INFO, "Tx power for channel %d : %0x\n", pAd->TxPower[J52_CHANNEL_START_OFFSET + i].Channel, pAd->TxPower[J52_CHANNEL_START_OFFSET + i].Power);
	}

	// Read TSSI reference and TSSI boundary for temperature compensation.
	// 0. 11b/g
	{
		RTUSBReadEEPROM(pAd, EEPROM_BG_TSSI_CALIBRAION, ChannelTxPower, 10);
		pAd->TssiMinusBoundaryG[4] = ChannelTxPower[0];
		pAd->TssiMinusBoundaryG[3] = ChannelTxPower[1];
		pAd->TssiMinusBoundaryG[2] = ChannelTxPower[2];
		pAd->TssiMinusBoundaryG[1] = ChannelTxPower[3];
		pAd->TssiPlusBoundaryG[1] = ChannelTxPower[4];
		pAd->TssiPlusBoundaryG[2] = ChannelTxPower[5];
		pAd->TssiPlusBoundaryG[3] = ChannelTxPower[6];
		pAd->TssiPlusBoundaryG[4] = ChannelTxPower[7];
		pAd->TssiRefG	= ChannelTxPower[8];
		pAd->TxAgcStepG = ChannelTxPower[9];  
		pAd->TxAgcCompensateG = 0;
		pAd->TssiMinusBoundaryG[0] = pAd->TssiRefG;
		pAd->TssiPlusBoundaryG[0]  = pAd->TssiRefG;

		// Disable TxAgc if the based value is not right
		if (pAd->TssiRefG == 0xff)
			pAd->bAutoTxAgcG = FALSE;

		DBGPRINT(RT_DEBUG_TRACE,"E2PROM: G Tssi[-4 .. +4] = %d %d %d %d - %d -%d %d %d %d, step=%d, tuning=%d\n",
			pAd->TssiMinusBoundaryG[4], pAd->TssiMinusBoundaryG[3], pAd->TssiMinusBoundaryG[2], pAd->TssiMinusBoundaryG[1],
			pAd->TssiRefG,
			pAd->TssiPlusBoundaryG[1], pAd->TssiPlusBoundaryG[2], pAd->TssiPlusBoundaryG[3], pAd->TssiPlusBoundaryG[4],
			pAd->TxAgcStepG, pAd->bAutoTxAgcG);
	}	
	// 1. 11a
	{
		RTUSBReadEEPROM(pAd, EEPROM_A_TSSI_CALIBRAION, ChannelTxPower, 10);
		pAd->TssiMinusBoundaryA[4] = ChannelTxPower[0];
		pAd->TssiMinusBoundaryA[3] = ChannelTxPower[1];
		pAd->TssiMinusBoundaryA[2] = ChannelTxPower[2];
		pAd->TssiMinusBoundaryA[1] = ChannelTxPower[3];
		pAd->TssiPlusBoundaryA[1] = ChannelTxPower[4];
		pAd->TssiPlusBoundaryA[2] = ChannelTxPower[5];
		pAd->TssiPlusBoundaryA[3] = ChannelTxPower[6];
		pAd->TssiPlusBoundaryA[4] = ChannelTxPower[7];
		pAd->TssiRefA	= ChannelTxPower[8];
		pAd->TxAgcStepA = ChannelTxPower[9]; 
		pAd->TxAgcCompensateA = 0;
		pAd->TssiMinusBoundaryA[0] = pAd->TssiRefA;
		pAd->TssiPlusBoundaryA[0]  = pAd->TssiRefA;

		// Disable TxAgc if the based value is not right
		if (pAd->TssiRefA == 0xff)
			pAd->bAutoTxAgcA = FALSE;

		DBGPRINT(RT_DEBUG_TRACE,"E2PROM: A Tssi[-4 .. +4] = %d %d %d %d - %d -%d %d %d %d, step=%d, tuning=%d\n",
			pAd->TssiMinusBoundaryA[4], pAd->TssiMinusBoundaryA[3], pAd->TssiMinusBoundaryA[2], pAd->TssiMinusBoundaryA[1],
			pAd->TssiRefA,
			pAd->TssiPlusBoundaryA[1], pAd->TssiPlusBoundaryA[2], pAd->TssiPlusBoundaryA[3], pAd->TssiPlusBoundaryA[4],
			pAd->TxAgcStepA, pAd->bAutoTxAgcA);
	}	
	pAd->BbpRssiToDbmDelta = 0x79;

	RTUSBReadEEPROM(pAd, EEPROM_FREQ_OFFSET, (PUCHAR) &value, 2);
	if ((value & 0xFF00) == 0xFF00)
	{
		pAd->RFProgSeq = 0;
	}
	else
	{
		pAd->RFProgSeq = (value & 0x0300) >> 8;	// bit 8,9
	}

	value &= 0x00FF;
	if (value != 0x00FF)
		pAd->RfFreqOffset = (ULONG) value;
	else
		pAd->RfFreqOffset = 0;
	DBGPRINT(RT_DEBUG_TRACE, "E2PROM: RF freq offset=0x%x\n", pAd->RfFreqOffset);

	//CountryRegion byte offset = 0x25
	value = pAd->EEPROMDefaultValue[2] >> 8;
	value2 = pAd->EEPROMDefaultValue[2] & 0x00FF;
    if ((value <= REGION_MAXIMUM_BG_BAND) && (value2 <= REGION_MAXIMUM_A_BAND))
	{
		pAd->PortCfg.CountryRegion = ((UCHAR) value) | 0x80;
		pAd->PortCfg.CountryRegionForABand = ((UCHAR) value2) | 0x80;
	}

	//
	// Get RSSI Offset on EEPROM 0x9Ah & 0x9Ch.
	// The valid value are (-10 ~ 10) 
	// 
	RTUSBReadEEPROM(pAd, EEPROM_RSSI_BG_OFFSET, (PUCHAR) &value, 2);
	pAd->BGRssiOffset1 = value & 0x00ff;
	pAd->BGRssiOffset2 = (value >> 8);

	// Validate 11b/g RSSI_1 offset.
	if ((pAd->BGRssiOffset1 < -10) || (pAd->BGRssiOffset1 > 10))
		pAd->BGRssiOffset1 = 0;

	// Validate 11b/g RSSI_2 offset.
	if ((pAd->BGRssiOffset2 < -10) || (pAd->BGRssiOffset2 > 10))
		pAd->BGRssiOffset2 = 0;
		
	RTUSBReadEEPROM(pAd, EEPROM_RSSI_A_OFFSET, (PUCHAR) &value, 2);
	pAd->ARssiOffset1 = value & 0x00ff;
	pAd->ARssiOffset2 = (value >> 8);

	// Validate 11a RSSI_1 offset.
	if ((pAd->ARssiOffset1 < -10) || (pAd->ARssiOffset1 > 10))
		pAd->ARssiOffset1 = 0;

	//Validate 11a RSSI_2 offset.
	if ((pAd->ARssiOffset2 < -10) || (pAd->ARssiOffset2 > 10))
		pAd->ARssiOffset2 = 0;

	//
	// Get LED Setting.
	//
	RTUSBReadEEPROM(pAd, EEPROM_LED_OFFSET, (PUCHAR)&LedSetting.word, 2);
	if (LedSetting.word == 0xFFFF)
	{
		//
		// Set it to Default.
		//
		LedSetting.field.PolarityRDY_G = 0;   // Active High.
		LedSetting.field.PolarityRDY_A = 0;   // Active High.
		LedSetting.field.PolarityACT = 0;	 // Active High.
		LedSetting.field.PolarityGPIO_0 = 0; // Active High.
		LedSetting.field.PolarityGPIO_1 = 0; // Active High.
		LedSetting.field.PolarityGPIO_2 = 0; // Active High.
		LedSetting.field.PolarityGPIO_3 = 0; // Active High.
		LedSetting.field.PolarityGPIO_4 = 0; // Active High.
		LedSetting.field.LedMode = LED_MODE_DEFAULT;		
	}
	pAd->LedCntl.word = 0;
	pAd->LedCntl.field.LedMode = LedSetting.field.LedMode;
	pAd->LedCntl.field.PolarityRDY_G = LedSetting.field.PolarityRDY_G;
	pAd->LedCntl.field.PolarityRDY_A = LedSetting.field.PolarityRDY_A;
	pAd->LedCntl.field.PolarityACT = LedSetting.field.PolarityACT;
	pAd->LedCntl.field.PolarityGPIO_0 = LedSetting.field.PolarityGPIO_0;
	pAd->LedCntl.field.PolarityGPIO_1 = LedSetting.field.PolarityGPIO_1;
	pAd->LedCntl.field.PolarityGPIO_2 = LedSetting.field.PolarityGPIO_2;
	pAd->LedCntl.field.PolarityGPIO_3 = LedSetting.field.PolarityGPIO_3;
	pAd->LedCntl.field.PolarityGPIO_4 = LedSetting.field.PolarityGPIO_4;

	RTUSBReadEEPROM(pAd, EEPROM_TXPOWER_DELTA_OFFSET, (PUCHAR)&value, 2);
	value = value & 0x00ff;
	if (value != 0xff)
	{
		pAd->TxPowerDeltaConfig.value = (UCHAR) value;
		if (pAd->TxPowerDeltaConfig.field.DeltaValue > 0x04)
			pAd->TxPowerDeltaConfig.field.DeltaValue = 0x04;
	}
	else
		pAd->TxPowerDeltaConfig.field.TxPowerEnable = FALSE;
		
	DBGPRINT(RT_DEBUG_TRACE, "<-- NICReadEEPROMParameters\n");
}

/*
	========================================================================
	
	Routine Description:
		Set default value from EEPROM
		
	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		None

	Note:
		
	========================================================================
*/
VOID NICInitAsicFromEEPROM(
	IN	PRTMP_ADAPTER	pAd)
{
	ULONG					data;
	USHORT					i;
	ULONG					MiscMode;
	EEPROM_ANTENNA_STRUC	Antenna;
	EEPROM_NIC_CONFIG2_STRUC	NicConfig2;

	DBGPRINT(RT_DEBUG_TRACE, "--> NICInitAsicFromEEPROM\n");

	for(i = 3; i < NUM_EEPROM_BBP_PARMS; i++)
	{
		UCHAR BbpRegIdx, BbpValue;
	
		if ((pAd->EEPROMDefaultValue[i] != 0xFFFF) && (pAd->EEPROMDefaultValue[i] != 0))
		{
			BbpRegIdx = (UCHAR)(pAd->EEPROMDefaultValue[i] >> 8);
			BbpValue  = (UCHAR)(pAd->EEPROMDefaultValue[i] & 0xff);
			RTUSBWriteBBPRegister(pAd, BbpRegIdx, BbpValue);
		}
	}
	
#ifdef BIG_ENDIAN
	Antenna.word = SWAP16(pAd->EEPROMDefaultValue[0]);
#else
	Antenna.word = pAd->EEPROMDefaultValue[0];
#endif

	if (Antenna.word == 0xFFFF)
	{
		Antenna.word = 0;
		Antenna.field.RfIcType = RFIC_5226;
		Antenna.field.HardwareRadioControl = 0; 	// no hardware control
		Antenna.field.DynamicTxAgcControl = 0;
		Antenna.field.FrameType = 0;
		Antenna.field.RxDefaultAntenna = 2; 		// Ant-B
		Antenna.field.TxDefaultAntenna = 2; 		// Ant-B
		Antenna.field.NumOfAntenna = 2;
		DBGPRINT(RT_DEBUG_WARN, "E2PROM error, hard code as 0x%04x\n", Antenna.word);
	}

	pAd->RfIcType = (UCHAR) Antenna.field.RfIcType;
	DBGPRINT(RT_DEBUG_WARN, "pAd->RfIcType = %d\n", pAd->RfIcType);

	//
	// For RFIC RFIC_5225 & RFIC_2527
	// Must enable RF RPI mode on PHY_CSR1 bit 16.
	//
	if ((pAd->RfIcType == RFIC_5225) || (pAd->RfIcType == RFIC_2527) || (pAd->RfIcType == RFIC_3020))
	{
		RTUSBReadMACRegister(pAd, PHY_CSR1, &MiscMode);
		MiscMode |= 0x10000;
		RTUSBWriteMACRegister(pAd, PHY_CSR1, MiscMode);
	}
	
	// Save the antenna for future use
	pAd->Antenna.word = Antenna.word;
	
	// Read Hardware controlled Radio state enable bit
	if (Antenna.field.HardwareRadioControl == 1)
	{
		pAd->PortCfg.bHardwareRadio = TRUE;
		
		// Read GPIO pin7 as Hardware controlled radio state
		RTUSBReadMACRegister(pAd, MAC_CSR13, &data);

		//
		// The GPIO pin7 default is 1:Pull-High, means HW Radio Enable.
		// When the value is 0, means HW Radio disable.
		//
		if ((data & 0x80) == 0)
		{
			pAd->PortCfg.bHwRadio = FALSE;
			// Update extra information to link is up
			pAd->ExtraInfo = HW_RADIO_OFF;
		}
	}
	else
		pAd->PortCfg.bHardwareRadio = FALSE;		
	
	pAd->PortCfg.bRadio = pAd->PortCfg.bSwRadio && pAd->PortCfg.bHwRadio;
	
	if (pAd->PortCfg.bRadio == FALSE)
	{
		RTUSBWriteMACRegister(pAd, MAC_CSR10, 0x00001818);
		RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF);
	
		RTMPSetLED(pAd, LED_RADIO_OFF);
	}
	else
	{
		RTMPSetLED(pAd, LED_RADIO_ON);
	}
	
#ifdef BIG_ENDIAN
	NicConfig2.word = SWAP16(pAd->EEPROMDefaultValue[1]);
#else
	NicConfig2.word = pAd->EEPROMDefaultValue[1];
#endif
	if (NicConfig2.word == 0xffff)
	{
		NicConfig2.word = 0;
	}
	// Save the antenna for future use
	pAd->NicConfig2.word = NicConfig2.word;

	DBGPRINT(RT_DEBUG_TRACE, "Use Hw Radio Control Pin=%d; if used Pin=%d;\n",
		pAd->PortCfg.bHardwareRadio, pAd->PortCfg.bHardwareRadio);
	
	DBGPRINT(RT_DEBUG_TRACE, "RFIC=%d, LED mode=%d\n", pAd->RfIcType, pAd->LedCntl.field.LedMode);

	pAd->PortCfg.BandState = UNKNOWN_BAND;

	DBGPRINT(RT_DEBUG_TRACE, "<-- NICInitAsicFromEEPROM\n");
}

/*
	========================================================================
	
	Routine Description:
		Initialize NIC hardware

	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		None

	Note:
		
	========================================================================
*/
NDIS_STATUS	NICInitializeAsic(
	IN	PRTMP_ADAPTER	pAd)
{
	ULONG			Index, Counter;
	UCHAR			Value = 0xff;
	ULONG			Version;
	MAC_CSR12_STRUC	MacCsr12;

	DBGPRINT(RT_DEBUG_TRACE, "--> NICInitializeAsic\n");

	RTUSBReadMACRegister(pAd, MAC_CSR0, &Version);
	
	// Initialize MAC register to default value
	for (Index = 0; Index < NUM_MAC_REG_PARMS; Index++)
	{
		RTUSBWriteMACRegister(pAd, (USHORT)MACRegTable[Index].Register, MACRegTable[Index].Value);
	}
	
	// Set Host ready before kicking Rx
	RTUSBWriteMACRegister(pAd, MAC_CSR1, 0x3);
	RTUSBWriteMACRegister(pAd, MAC_CSR1, 0x0);		

	//
	// Before program BBP, we need to wait BBP/RF get wake up.
	//
	Index = 0;
	do
	{
		RTUSBReadMACRegister(pAd, MAC_CSR12, &MacCsr12.word);

		if (MacCsr12.field.BbpRfStatus == 1)
			break;

		RTUSBWriteMACRegister(pAd, MAC_CSR12, 0x4); //Force wake up.
		RTMPusecDelay(1000);
	} while (Index++ < 1000);

	// Read BBP register, make sure BBP is up and running before write new data
	Index = 0;
	do 
	{
		RTUSBReadBBPRegister(pAd, BBP_R0, &Value);
		DBGPRINT(RT_DEBUG_TRACE, "BBP version = %d\n", Value);
	} while ((++Index < 100) && ((Value == 0xff) || (Value == 0x00)));
		  
	// Initialize BBP register to default value
	for (Index = 0; Index < NUM_BBP_REG_PARMS; Index++)
	{
		RTUSBWriteBBPRegister(pAd, BBPRegTable[Index].Register, BBPRegTable[Index].Value);
	}

	// Clear raw counters
	RTUSBReadMACRegister(pAd, STA_CSR0, &Counter);
	RTUSBReadMACRegister(pAd, STA_CSR1, &Counter);
	RTUSBReadMACRegister(pAd, STA_CSR2, &Counter);

	// assert HOST ready bit
	RTUSBWriteMACRegister(pAd, MAC_CSR1, 0x4);
	
	DBGPRINT(RT_DEBUG_TRACE, "<-- NICInitializeAsic\n");

	return NDIS_STATUS_SUCCESS;
}

/*
	========================================================================
	
	Routine Description:
		Reset NIC Asics

	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		None

	Note:
		Reset NIC to initial state AS IS system boot up time.
		
	========================================================================
*/
VOID NICIssueReset(
	IN	PRTMP_ADAPTER	pAd)
{

}

/*
	========================================================================
	
	Routine Description:
		Check ASIC registers and find any reason the system might hang

	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		None
	
	========================================================================
*/
BOOLEAN	NICCheckForHang(
	IN	PRTMP_ADAPTER	pAd)
{
	return (FALSE);
}

/*
	========================================================================
	
	Routine Description:
		Read statistical counters from hardware registers and record them
		in software variables for later on query

	Arguments:
		pAd					Pointer to our adapter

	Return Value:
		None


	========================================================================
*/
VOID NICUpdateRawCounters(
	IN PRTMP_ADAPTER pAd)
{
	ULONG	OldValue;
	STA_CSR0_STRUC StaCsr0;
	STA_CSR1_STRUC StaCsr1;
	STA_CSR2_STRUC StaCsr2;
	STA_CSR3_STRUC StaCsr3;
	STA_CSR4_STRUC StaCsr4;
	STA_CSR5_STRUC StaCsr5;

	RTUSBReadMACRegister(pAd, STA_CSR0, &StaCsr0.word);

	// Update RX PLCP error counter
	pAd->PrivateInfo.PhyRxErrCnt += StaCsr0.field.PlcpErr;

	// Update FCS counters
	OldValue= pAd->WlanCounters.FCSErrorCount.vv.LowPart;
	pAd->WlanCounters.FCSErrorCount.vv.LowPart += (StaCsr0.field.CrcErr); // >> 7);
	if (pAd->WlanCounters.FCSErrorCount.vv.LowPart < OldValue)
		pAd->WlanCounters.FCSErrorCount.vv.HighPart++;
		
	// Add FCS error count to private counters	 
	OldValue = pAd->RalinkCounters.RealFcsErrCount.vv.LowPart;
	pAd->RalinkCounters.RealFcsErrCount.vv.LowPart += StaCsr0.field.CrcErr;
	if (pAd->RalinkCounters.RealFcsErrCount.vv.LowPart < OldValue)
		pAd->RalinkCounters.RealFcsErrCount.vv.HighPart++;


	// Update False CCA counter
	RTUSBReadMACRegister(pAd, STA_CSR1, &StaCsr1.word);
	pAd->RalinkCounters.OneSecFalseCCACnt += StaCsr1.field.FalseCca;

	// Update RX Overflow counter
	RTUSBReadMACRegister(pAd, STA_CSR2, &StaCsr2.word);
	pAd->Counters8023.RxNoBuffer += (StaCsr2.field.RxOverflowCount + StaCsr2.field.RxFifoOverflowCount);

	// Update BEACON sent count
	RTUSBReadMACRegister(pAd, STA_CSR3, &StaCsr3.word);
	pAd->RalinkCounters.OneSecBeaconSentCnt += StaCsr3.field.TxBeaconCount;

	RTUSBReadMACRegister(pAd, STA_CSR4, &StaCsr4.word);
	RTUSBReadMACRegister(pAd, STA_CSR5, &StaCsr5.word);

	// 1st - Transmit Success
	OldValue = pAd->WlanCounters.TransmittedFragmentCount.vv.LowPart;
	pAd->WlanCounters.TransmittedFragmentCount.vv.LowPart += (StaCsr4.field.TxOneRetryCount + StaCsr4.field.TxNoRetryCount + StaCsr5.field.TxMultiRetryCount);
	if (pAd->WlanCounters.TransmittedFragmentCount.vv.LowPart < OldValue)
	{
		pAd->WlanCounters.TransmittedFragmentCount.vv.HighPart++;
	}

	// 2rd	-success and no retry
	OldValue = pAd->WlanCounters.RetryCount.vv.LowPart;
	pAd->WlanCounters.NoRetryCount.vv.LowPart += StaCsr4.field.TxNoRetryCount;
	if (pAd->WlanCounters.NoRetryCount.vv.LowPart < OldValue)
	{
		pAd->WlanCounters.NoRetryCount.vv.HighPart++;
	}

	// 3rd	-success and retry
	OldValue = pAd->WlanCounters.RetryCount.vv.LowPart;
	pAd->WlanCounters.RetryCount.vv.LowPart += (StaCsr4.field.TxOneRetryCount  +StaCsr5.field.TxMultiRetryCount);
	if (pAd->WlanCounters.RetryCount.vv.LowPart < OldValue)
	{
		pAd->WlanCounters.RetryCount.vv.HighPart++;
	}
	// 4th - fail
	OldValue = pAd->WlanCounters.FailedCount.vv.LowPart;
	pAd->WlanCounters.FailedCount.vv.LowPart += StaCsr5.field.TxRetryFailCount;
	if (pAd->WlanCounters.FailedCount.vv.LowPart < OldValue)
	{
		pAd->WlanCounters.FailedCount.vv.HighPart++;
	}	

	
	pAd->RalinkCounters.OneSecTxNoRetryOkCount = StaCsr4.field.TxNoRetryCount;
	pAd->RalinkCounters.OneSecTxRetryOkCount = StaCsr4.field.TxOneRetryCount + StaCsr5.field.TxMultiRetryCount;
	pAd->RalinkCounters.OneSecTxFailCount = StaCsr5.field.TxRetryFailCount;
	pAd->RalinkCounters.OneSecFalseCCACnt = StaCsr1.field.FalseCca;
	pAd->RalinkCounters.OneSecRxOkCnt = pAd->RalinkCounters.RxCount;
	pAd->RalinkCounters.RxCount = 0; //Reset RxCount
	pAd->RalinkCounters.OneSecRxFcsErrCnt = StaCsr0.field.CrcErr;
	pAd->RalinkCounters.OneSecBeaconSentCnt = StaCsr3.field.TxBeaconCount;
}

/*
	========================================================================
	
	Routine Description:
		Reset NIC from error

	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		None

	Note:
		Reset NIC from error state
		
	========================================================================
*/
VOID NICResetFromError(
	IN	PRTMP_ADAPTER	pAd)
{
	NICInitializeAsic(pAd);
#ifdef	INIT_FROM_EEPROM
	NICInitAsicFromEEPROM(pAd);
#endif
	RTUSBWriteHWMACAddress(pAd);
	
}

/*******************************************************************************

	File open/close related functions.
	
 *******************************************************************************/
RTMP_OS_FD RtmpOSFileOpen(char *pPath,  int flag, int mode)
{
	struct file	*filePtr;
		
	filePtr = filp_open(pPath, flag, 0);
	if (IS_ERR(filePtr))
	{
		DBGPRINT(RT_DEBUG_ERROR, "%s(): Error %ld opening %s\n", __FUNCTION__, -PTR_ERR(filePtr), pPath);
	}

	return (RTMP_OS_FD)filePtr;
}

int RtmpOSFileClose(RTMP_OS_FD osfd)
{
	filp_close(osfd, NULL);
	return 0;
}


void RtmpOSFileSeek(RTMP_OS_FD osfd, int offset)
{
	osfd->f_pos = offset;
}


int RtmpOSFileRead(RTMP_OS_FD osfd, char *pDataPtr, int readLen)
{
	// The object must have a read method
	if (osfd->f_op && osfd->f_op->read)
	{
		return osfd->f_op->read(osfd,  pDataPtr, readLen, &osfd->f_pos);
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR, "no file read method\n");
		return -1;
	}
}


int RtmpOSFileWrite(RTMP_OS_FD osfd, char *pDataPtr, int writeLen)
{
	return osfd->f_op->write(osfd, pDataPtr, (size_t)writeLen, &osfd->f_pos);
}

void RtmpOSFSInfoChange(RTMP_OS_FS_INFO *pOSFSInfo, BOOLEAN bSet)
{
	if (bSet)
	{
		// Save uid and gid used for filesystem access.
		// Set user and group to 0 (root)	
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
		pOSFSInfo->fsuid= current->fsuid;
		pOSFSInfo->fsgid = current->fsgid;
		current->fsuid = current->fsgid = 0;
#else
		pOSFSInfo->fsuid = current_fsuid();
		pOSFSInfo->fsgid = current_fsgid();		
#endif
		pOSFSInfo->fs = get_fs();
		set_fs(KERNEL_DS);
	}
	else
	{
		set_fs(pOSFSInfo->fs);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
		current->fsuid = pOSFSInfo->fsuid;
		current->fsgid = pOSFSInfo->fsgid;
#endif
	}
}

/*
	========================================================================
	
	Routine Description:
		Load 8051 firmware RT2561.BIN file into MAC ASIC

	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		NDIS_STATUS_SUCCESS 		firmware image load ok
		NDIS_STATUS_FAILURE 		image not found

	========================================================================
*/
NDIS_STATUS NICLoadFirmware(
	IN PRTMP_ADAPTER pAd)
{
	NDIS_STATUS				Status = NDIS_STATUS_SUCCESS;
	PUCHAR					src = NULL;
	struct file				*srcf;
	INT 					retval = 0,i;

	PUCHAR					pFirmwareImage;
	UINT					FileLength = 0;
	INT						ret;					

	RTMP_OS_FS_INFO	osFSInfo;
	
	DBGPRINT(RT_DEBUG_TRACE,"--> NICLoadFirmware\n");
	pAd->FirmwareVersion = (FIRMWARE_MAJOR_VERSION << 8) + FIRMWARE_MINOR_VERSION; //default version.

	src = RT2573_IMAGE_FILE_NAME;


	RtmpOSFSInfoChange(&osFSInfo, TRUE);

	pFirmwareImage = kmalloc(MAX_FIRMWARE_IMAGE_SIZE, MEM_ALLOC_FLAG);
	if (pFirmwareImage == NULL) 
	{
		DBGPRINT(RT_DEBUG_ERROR, "NICLoadFirmware-Memory allocate fail\n");
		Status = NDIS_STATUS_FAILURE;
		goto out;
	}


	/* open the bin file */
	srcf = RtmpOSFileOpen(src, O_RDONLY, 0);

	//srcf = filp_open(src, O_RDONLY, 0);
	
	if (IS_ERR(srcf)) 
	{
		Status = NDIS_STATUS_FAILURE;
		DBGPRINT(RT_DEBUG_ERROR, "--> Error %ld opening %s\n", -PTR_ERR(srcf),src);    
	}
	else 
	{

		memset(pFirmwareImage, 0x00, MAX_FIRMWARE_IMAGE_SIZE);

		/* read the firmware from the file *.bin */
		FileLength = RtmpOSFileRead(srcf, pFirmwareImage, MAX_FIRMWARE_IMAGE_SIZE);

		if (FileLength != MAX_FIRMWARE_IMAGE_SIZE)
		{
			DBGPRINT_ERR("NICLoadFirmware: error file length (=%d) in rt73.bin\n",FileLength);
			Status = NDIS_STATUS_FAILURE;
		}
		else
		{  //FileLength == MAX_FIRMWARE_IMAGE_SIZE
			PUCHAR ptr = pFirmwareImage;
			USHORT crc = 0;
			
			for (i=0; i<(MAX_FIRMWARE_IMAGE_SIZE-2); i++, ptr++)
				crc = ByteCRC16(*ptr, crc);
			crc = ByteCRC16(0x00, crc);
			crc = ByteCRC16(0x00, crc);
			
			if ((pFirmwareImage[MAX_FIRMWARE_IMAGE_SIZE-2] != (UCHAR)(crc>>8)) ||
				(pFirmwareImage[MAX_FIRMWARE_IMAGE_SIZE-1] != (UCHAR)(crc)))
			{
				DBGPRINT_ERR("NICLoadFirmware: CRC = 0x%02x 0x%02x error, should be 0x%02x 0x%02x\n",
					pFirmwareImage[MAX_FIRMWARE_IMAGE_SIZE-2], pFirmwareImage[MAX_FIRMWARE_IMAGE_SIZE-1],
					(UCHAR)(crc>>8), (UCHAR)(crc) );

				if (retval)
				{
					DBGPRINT(RT_DEBUG_ERROR, "--> Error %d closing %s\n", -retval, src);
				}

				Status = NDIS_STATUS_FAILURE;
			}
			else
			{

				if ((pAd->FirmwareVersion) > ((pFirmwareImage[MAX_FIRMWARE_IMAGE_SIZE-4] << 8) + pFirmwareImage[MAX_FIRMWARE_IMAGE_SIZE-3]))
				{
					DBGPRINT_ERR("NICLoadFirmware: Ver=%d.%d, local Ver=%d.%d, used FirmwareImage talbe instead\n",
						pFirmwareImage[MAX_FIRMWARE_IMAGE_SIZE-4], pFirmwareImage[MAX_FIRMWARE_IMAGE_SIZE-3],
						FIRMWARE_MAJOR_VERSION, FIRMWARE_MINOR_VERSION);

					Status = NDIS_STATUS_FAILURE;
				}
				else
				{
					pAd->FirmwareVersion = (pFirmwareImage[MAX_FIRMWARE_IMAGE_SIZE-4] << 8) + pFirmwareImage[MAX_FIRMWARE_IMAGE_SIZE-3];
					DBGPRINT(RT_DEBUG_TRACE,"NICLoadFirmware OK: CRC = 0x%04x ver=%d.%d\n", crc,
						pFirmwareImage[MAX_FIRMWARE_IMAGE_SIZE-4], pFirmwareImage[MAX_FIRMWARE_IMAGE_SIZE-3]);
				}

			}
		}

		

		/* close firmware file */
		if (IS_FILE_OPEN_ERR(srcf))
			;
		else
		{
			retval = RtmpOSFileClose(srcf);
			if (retval)
			{
				DBGPRINT(RT_DEBUG_ERROR, "--> Error %d closing %s\n", -retval, src);
			}
		}
		
	}



	if (Status != NDIS_STATUS_SUCCESS)
	{	
		FileLength = FIRMAREIMAGE_LENGTH;
		memset(pFirmwareImage, 0x00, FileLength);
		NdisMoveMemory(pFirmwareImage, &FirmwareImage[0], FileLength);
		Status = NDIS_STATUS_SUCCESS; // change to success
		
		DBGPRINT(RT_DEBUG_ERROR, "NICLoadFirmware failed, used local Firmware(v %d.%d) instead\n", 
				FIRMWARE_MAJOR_VERSION, FIRMWARE_MINOR_VERSION);		
	}

	// select 8051 program bank; write entire firmware image
	for (i = 0; i < FileLength; i = i + 4)
	{
		ret = RTUSBMultiWrite(pAd, FIRMWARE_IMAGE_BASE + i, pFirmwareImage + i, 4);

		if (ret < 0)
		{
			Status = NDIS_STATUS_FAILURE;
			break;
		}
	}


out:	
	if (pFirmwareImage != NULL)
		kfree(pFirmwareImage);

	RtmpOSFSInfoChange(&osFSInfo, FALSE);

	if (Status == NDIS_STATUS_SUCCESS)
	{
		RTUSBFirmwareRun(pAd);
		
		//
		// Send LED command to Firmare after RTUSBFirmwareRun;
		//
		RTMPSetLED(pAd, LED_LINK_DOWN);

	}		

	DBGPRINT(RT_DEBUG_TRACE,"<-- NICLoadFirmware (src=%s)\n", src);  
	

	return Status;
}

/*
	========================================================================

	Routine Description:
		Find key section for Get key parameter.

	Arguments:
		buffer						Pointer to the buffer to start find the key section
		section 					the key of the secion to be find

	Return Value:
		NULL						Fail
		Others						Success
	========================================================================
*/
PUCHAR RTMPFindSection(
	IN	PCHAR	buffer)
{
	CHAR temp_buf[255];
	PUCHAR	ptr;

	strcpy(temp_buf, "[");					/*	and the opening bracket [  */
	strcat(temp_buf, "Default");
	strcat(temp_buf, "]");

  
	if((ptr = rtstrstr(buffer, temp_buf)) != NULL)
			return (ptr+strlen("\n"));
		else
			return NULL;
}

 /**
  * strstr - Find the first substring in a %NUL terminated string
  * @s1: The string to be searched
  * @s2: The string to search for
  */
char * rtstrstr(const char * s1,const char * s2)
{
	INT l1, l2;

	l2 = strlen(s2);
	if (!l2)
		return (char *) s1;
	
	l1 = strlen(s1);
	
	while (l1 >= l2)
	{
		l1--;
		if (!memcmp(s1,s2,l2))
			return (char *) s1;
		s1++;
	}
	
	return NULL;
}

/**
 * rstrtok - Split a string into tokens
 * @s: The string to be searched
 * @ct: The characters to search for
 * * WARNING: strtok is deprecated, use strsep instead. However strsep is not compatible with old architecture.
 */
char * __rstrtok;
char * rstrtok(char * s,const char * ct)
{
	char *sbegin, *send;

	sbegin  = s ? s : __rstrtok;
	if (!sbegin)
	{
		return NULL;
	}

	sbegin += strspn(sbegin,ct);
	if (*sbegin == '\0')
	{
		__rstrtok = NULL;
		return( NULL );
	}

	send = strpbrk( sbegin, ct);
	if (send && *send != '\0')
		*send++ = '\0';

	__rstrtok = send;

	return (sbegin);
}

/*
	========================================================================

	Routine Description:
		Get key parameter.

	Arguments:
		key 						Pointer to key string
		dest						Pointer to destination		
		destsize					The datasize of the destination
		buffer						Pointer to the buffer to start find the key

	Return Value:
		TRUE						Success
		FALSE						Fail

	Note:
		This routine get the value with the matched key (case case-sensitive)
	========================================================================
*/
INT RTMPGetKeyParameter(
	IN	PCHAR	key,
	OUT PCHAR	dest,	
	IN	INT 	destsize,
	IN	PCHAR	buffer)
{
    CHAR *temp_buf1;
    CHAR *temp_buf2;
	CHAR *start_ptr;
	CHAR *end_ptr;
	CHAR *ptr;
	CHAR *offset = 0;
	INT  len;

#define MAX_PARAM_BUFFER_SIZE 2048

	temp_buf1 = kmalloc(MAX_PARAM_BUFFER_SIZE, MEM_ALLOC_FLAG);
	if(temp_buf1 == NULL)
        return (FALSE);

	temp_buf2 = kmalloc(MAX_PARAM_BUFFER_SIZE, MEM_ALLOC_FLAG);
	if(temp_buf2 == NULL)
	{
		kfree(temp_buf1);
        return (FALSE);
	}

	//find section
	if((offset = RTMPFindSection(buffer)) == NULL)
    {
    	kfree(temp_buf1);
    	kfree(temp_buf2);
		return (FALSE);
    }

	strcpy(temp_buf1, "\n");
	strcat(temp_buf1, key);
	strcat(temp_buf1, "=");

	//search key
	if((start_ptr=rtstrstr(offset, temp_buf1))==NULL)
    {
    	kfree(temp_buf1);
    	kfree(temp_buf2);
        return (FALSE);
    }

	start_ptr+=strlen("\n");
   
	if((end_ptr=rtstrstr(start_ptr, "\n"))==NULL)
	   end_ptr=start_ptr+strlen(start_ptr);

	if (end_ptr<start_ptr)
    {
    	kfree(temp_buf1);
    	kfree(temp_buf2);
        return (FALSE);
    }

	memcpy(temp_buf2, start_ptr, end_ptr-start_ptr);
	temp_buf2[end_ptr-start_ptr]='\0';
	len = strlen(temp_buf2);
	strcpy(temp_buf1, temp_buf2);
	if((start_ptr=rtstrstr(temp_buf1, "=")) == NULL)
    {
    	kfree(temp_buf1);
    	kfree(temp_buf2);
        return (FALSE);
    }

	strcpy(temp_buf2, start_ptr+1);
	ptr = temp_buf2;
	//trim space or tab
	while(*ptr != 0x00)
	{
		if( (*ptr == ' ') || (*ptr == '\t') )
			ptr++;
		else
		   break;
	}

	len = strlen(ptr);	  
	memset(dest, 0x00, destsize);
	strncpy(dest, ptr, len >= destsize ?  destsize: len);

	kfree(temp_buf1);
   	kfree(temp_buf2);
	return TRUE;
}

static void rtmp_read_key_parms_from_file(IN  PRTMP_ADAPTER pAd, char *tmpbuf, char *buffer)
{
	char		tok_str[16];					
	INT			idx;
	ULONG		KeyType;
	ULONG		KeyLen;
	ULONG		KeyIdx;
	UCHAR		CipherAlg = CIPHER_WEP64;

	//DefaultKeyID
	if(RTMPGetKeyParameter("DefaultKeyID", tmpbuf, 25, buffer))
	{
		KeyIdx = simple_strtol(tmpbuf, 0, 10);
		if((KeyIdx >= 1 ) && (KeyIdx <= 4))
			pAd->PortCfg.DefaultKeyId = (UCHAR) (KeyIdx - 1 );
		else
			pAd->PortCfg.DefaultKeyId = 0;

		DBGPRINT(RT_DEBUG_TRACE, "  DefaultKeyID(0~3)=%d\n", pAd->PortCfg.DefaultKeyId);
	}	   

	for (idx=0; idx<4; idx++) 
	{
	
		sprintf(tok_str, "Key%dType", idx+1);
		//Key%dType
		if(RTMPGetKeyParameter(tok_str, tmpbuf, 32, buffer))
		{		   
			KeyType = simple_strtol(tmpbuf, 0, 10);

			sprintf(tok_str, "Key%dStr", idx+1);
			//Key%dStr
			if(RTMPGetKeyParameter(tok_str, tmpbuf, 128, buffer))
			{
				KeyLen = strlen(tmpbuf);
				if(KeyType == 0)
				{//Hex type
					if( (KeyLen == 10) || (KeyLen == 26))
					{
						pAd->SharedKey[idx].KeyLen = KeyLen / 2;
						AtoH(tmpbuf, pAd->SharedKey[idx].Key, KeyLen / 2);
						if (KeyLen == 10)
							CipherAlg = CIPHER_WEP64;
						else
							CipherAlg = CIPHER_WEP128;
						pAd->SharedKey[idx].CipherAlg = CipherAlg;

						DBGPRINT(RT_DEBUG_TRACE, "  Key%dStr=%s and type=%s\n", idx+1, tmpbuf, (KeyType==0) ? "Hex":"Ascii");
					}
					else
					{ //Invalid key length
						DBGPRINT(RT_DEBUG_ERROR, "  Key%dStr is Invalid key length!\n", idx+1);
					}								
				}
				else
				{//Ascii								
					if( (KeyLen == 5) || (KeyLen == 13))
					{
						pAd->SharedKey[idx].KeyLen = KeyLen;
						NdisMoveMemory(pAd->SharedKey[idx].Key, tmpbuf, KeyLen);
						if (KeyLen == 5)
							CipherAlg = CIPHER_WEP64;
						else
							CipherAlg = CIPHER_WEP128;
						pAd->SharedKey[idx].CipherAlg = CipherAlg;
					
						DBGPRINT(RT_DEBUG_TRACE, "  Key%dStr=%s and type=%s\n", idx+1, tmpbuf, (KeyType==0) ? "Hex":"Ascii");		
					}
					else
					{ //Invalid key length
						DBGPRINT(RT_DEBUG_ERROR, "  Key%dStr is Invalid key length!\n", idx+1);
					}
				}
			}        
		}
	}
}

/*
	========================================================================

	Routine Description:
		In kernel mode read parameters from file

	Arguments:
		src 					the location of the file.
		dest						put the parameters to the destination.
		Length					size to read.

	Return Value:
		None

	Note:

	========================================================================
*/
VOID RTMPReadParametersFromFile(
	IN	PRTMP_ADAPTER pAd)
{
	NDIS_STATUS								Status = NDIS_STATUS_SUCCESS;
	PUCHAR									src;
	struct file 							*srcf;
	INT 									retval;
	RTMP_OS_FS_INFO	osFSInfo;
	UCHAR									*buffer, *mpool;
	CHAR									*tmpbuf; //[640];
	UCHAR									keyMaterial[40];
	UCHAR									Channel;
	ULONG									ulInfo;
	UCHAR									ucInfo;
	RT_802_11_PREAMBLE						Preamble;

	
	src = PROFILE_PATH;

	

	// allocate memory
	mpool = kmalloc(sizeof(CHAR)*(2*MAX_INI_BUFFER_SIZE+10), MEM_ALLOC_FLAG);  
	if (mpool == NULL)	
	{
		DBGPRINT(RT_DEBUG_ERROR, "Not enough memory\n");
		goto done;
	}
	

	RtmpOSFSInfoChange(&osFSInfo, TRUE);
			   
	if (src && *src)
	{
		srcf = RtmpOSFileOpen(src, O_RDONLY, 0);
		if (IS_FILE_OPEN_ERR(srcf)) 
		{
			Status = NDIS_STATUS_FAILURE;
			DBGPRINT(RT_DEBUG_TRACE, "--> Error %ld opening %s\n", -PTR_ERR(srcf),src);
		}		
		else
		{		 
			pAd->PortCfg.bGetAPConfig = FALSE;
			/* The object must have a read method */
			if (srcf->f_op && srcf->f_op->read) 
			{		   
				buffer = (UCHAR *) (((unsigned long)(mpool+3)) & ((unsigned long)~(0x03)));
				tmpbuf = (UCHAR *) (((unsigned long)(mpool+ MAX_INI_BUFFER_SIZE +3)) & ((unsigned long)~(0x03)));  
				memset(buffer, 0x00, MAX_INI_BUFFER_SIZE);
				memset(tmpbuf, 0x00, MAX_INI_BUFFER_SIZE);		

				/* read the firmware from the file *.bin */
				retval = RtmpOSFileRead(srcf, mpool, MAX_INI_BUFFER_SIZE);				
				if (retval < 0)
				{
					Status = NDIS_STATUS_FAILURE;
					DBGPRINT(RT_DEBUG_TRACE, "--> Read %s error %d\n", src, -retval);
				}				 
				else
				{					
					// set file parameter to portcfg
					//CountryRegion
					if (RTMPGetKeyParameter("CountryRegion", tmpbuf, 255, buffer))
					{
						ucInfo = (UCHAR) simple_strtol(tmpbuf, 0, 10);
						if (ucInfo <= REGION_MAXIMUM_BG_BAND)
							pAd->PortCfg.CountryRegion = ucInfo; 
						else
						{	
							ucInfo = 0; // default
							pAd->PortCfg.CountryRegion = ucInfo;	
						}
						DBGPRINT(RT_DEBUG_TRACE, "%s::(CountryRegion=%d)\n", __FUNCTION__, pAd->PortCfg.CountryRegion);
					} 
					//CountryRegionABand
					if(RTMPGetKeyParameter("CountryRegionABand", tmpbuf, 25, buffer))
					{
						ucInfo = (UCHAR) simple_strtol(tmpbuf, 0, 10);
						if (ucInfo <= REGION_MAXIMUM_A_BAND)
							pAd->PortCfg.CountryRegionForABand = ucInfo;
						else
						{
							ucInfo = 7; // default
							pAd->PortCfg.CountryRegionForABand = ucInfo;
						}
						DBGPRINT(RT_DEBUG_TRACE, "%s::(CountryRegionABand=%d)\n", __FUNCTION__, pAd->PortCfg.CountryRegionForABand);
					}					
					//SSID
					memset(tmpbuf, 0x00, 255);
					if (RTMPGetKeyParameter("SSID", tmpbuf, 64, buffer))
					{
						if (strlen(tmpbuf) <= 32)
						{
							pAd->PortCfg.SsidLen = (UCHAR) strlen(tmpbuf);
							memcpy(pAd->PortCfg.Ssid, tmpbuf, pAd->PortCfg.SsidLen);

							pAd->MlmeAux.AutoReconnectSsidLen = pAd->PortCfg.SsidLen;
							memcpy(pAd->MlmeAux.AutoReconnectSsid, tmpbuf, pAd->MlmeAux.AutoReconnectSsidLen);
							pAd->MlmeAux.SsidLen = pAd->PortCfg.SsidLen;
							NdisZeroMemory(pAd->MlmeAux.Ssid, MAX_LEN_OF_SSID);
							memcpy(pAd->MlmeAux.Ssid, tmpbuf, pAd->MlmeAux.SsidLen);

							DBGPRINT(RT_DEBUG_TRACE, "%s::(SSID=%s)\n", __FUNCTION__, tmpbuf);

						}
					}					
					//NetworkType
					if (RTMPGetKeyParameter("NetworkType", tmpbuf, 25, buffer))
					{
						pAd->bConfigChanged = TRUE;
						if (strcmp(tmpbuf, "Adhoc") == 0)
							pAd->PortCfg.BssType = BSS_ADHOC;
						else //Default Infrastructure mode
							pAd->PortCfg.BssType = BSS_INFRA;
						// Reset Ralink supplicant to not use, it will be set to start when UI set PMK key
						//pAd->PortCfg.WpaState = SS_NOTUSE;
						DBGPRINT(RT_DEBUG_TRACE, "%s::(NetworkType=%d)\n", __FUNCTION__, pAd->PortCfg.BssType);
					}					 
					//WirelessMode
					if (RTMPGetKeyParameter("WirelessMode", tmpbuf, 10, buffer))
					{	
						int i=0;

						ulInfo = (ULONG)simple_strtol(tmpbuf, 0, 10);
						// Init the channel number for TX channel power 0. 11b/g
						// fix bug that can not read channel from file
						for (i = 0; i < 14; i++)
						pAd->TxPower[i].Channel = i + 1;
						// 1. UNI 36 - 64
						for (i = 0; i < 8; i++)
						pAd->TxPower[i + 14].Channel = 36 + i * 4;
						// 2. HipperLAN 2 100 - 140
						for (i = 0; i < 11; i++)
						pAd->TxPower[i + 22].Channel = 100 + i * 4;
						// 3. UNI 140 - 165
						for (i = 0; i < 5; i++)
						pAd->TxPower[i + 33].Channel = 149 + i * 4; 	   

						// 34/38/42/46
						for (i = 0; i < 4; i++)
						pAd->TxPower[i + 38].Channel = 34 + i * 4;
	
						if ((ulInfo == PHY_11BG_MIXED) || (ulInfo == PHY_11B) ||
							(ulInfo == PHY_11A) || (ulInfo == PHY_11ABG_MIXED) ||
							(ulInfo == PHY_11G))
						{
							pAd->PortCfg.PhyMode = 0xff;
							RTMPSetPhyMode(pAd, ulInfo);
						 
							DBGPRINT(RT_DEBUG_TRACE, "%s::(WirelessMode=%d)\n", __FUNCTION__, ulInfo);
						}
					}										 
					//TxRate
					if (RTMPGetKeyParameter("TxRate", tmpbuf, 10, buffer))
					{
						ulInfo = (ULONG)simple_strtol(tmpbuf, 0, 10);

						if ((pAd->PortCfg.PhyMode == PHY_11B && ulInfo <= 4) ||\
							((pAd->PortCfg.PhyMode == PHY_11BG_MIXED || pAd->PortCfg.PhyMode == PHY_11ABG_MIXED) && ulInfo <= 12) ||\
							((pAd->PortCfg.PhyMode == PHY_11A || pAd->PortCfg.PhyMode == PHY_11G) && (ulInfo == 0 ||(ulInfo > 4 && ulInfo <= 12))))
						{
							if (ulInfo == 0)
								RTMPSetDesiredRates(pAd, -1);
							else
								RTMPSetDesiredRates(pAd, (LONG) (RateIdToMbps[ulInfo-1] * 1000000));
						}
						else
						{
							ulInfo = 0; // default, TxRate=Auto
							RTMPSetDesiredRates(pAd, -1);
						}
						pAd->PortCfg.DefaultMaxDesiredRate = (UCHAR)ulInfo;
						DBGPRINT(RT_DEBUG_TRACE, "%s::(TxRate=%d)\n", __FUNCTION__, ulInfo);
					}					 
					//Channel
					if (RTMPGetKeyParameter("Channel", tmpbuf, 10, buffer))
					{
						Channel = (UCHAR) simple_strtol(tmpbuf, 0, 10);
						pAd->PortCfg.AdhocChannel = Channel;		
						//BuildChannelList(pAd);
						if (ChannelSanity(pAd, Channel) == TRUE)
						{

							pAd->PortCfg.Channel = Channel;
							
							DBGPRINT(RT_DEBUG_TRACE, "%s::(Channel=%d)\n", __FUNCTION__, Channel);
						}
					}					
					//BGProtection
					if (RTMPGetKeyParameter("BGProtection", tmpbuf, 10, buffer))
					{
						switch (simple_strtol(tmpbuf, 0, 10))
						{
							case 1: //Always On
								pAd->PortCfg.UseBGProtection = 1;
								break;
							case 2: //Always OFF
								pAd->PortCfg.UseBGProtection = 2;
								break;
							case 0: //AUTO
							default:
								pAd->PortCfg.UseBGProtection = 0;
								break;
						}
						DBGPRINT(RT_DEBUG_TRACE, "%s::(BGProtection=%d)\n", __FUNCTION__, pAd->PortCfg.UseBGProtection);
					}				   
					//TxPreamble
					if (RTMPGetKeyParameter("TxPreamble", tmpbuf, 10, buffer))
					{
						Preamble = simple_strtol(tmpbuf, 0, 10);
						switch (Preamble)
						{
							case Rt802_11PreambleShort:
								pAd->PortCfg.TxPreamble = Preamble;
								MlmeSetTxPreamble(pAd, Rt802_11PreambleShort);
								break;
							case Rt802_11PreambleLong:
							case Rt802_11PreambleAuto:
							default:
								// if user wants AUTO, initialize to LONG here, then change according to AP's
								// capability upon association.
								pAd->PortCfg.TxPreamble = Preamble;
								MlmeSetTxPreamble(pAd, Rt802_11PreambleLong);
						}
						DBGPRINT(RT_DEBUG_TRACE, "%s::(TxPreamble=%d)\n", __FUNCTION__, Preamble);
					}
					//RTSThreshold
					if (RTMPGetKeyParameter("RTSThreshold", tmpbuf, 10, buffer))
					{
						ulInfo = simple_strtol(tmpbuf, 0, 10);

						if((ulInfo > 0) && (ulInfo <= MAX_RTS_THRESHOLD))
							pAd->PortCfg.RtsThreshold = (USHORT)ulInfo;
						else 
							pAd->PortCfg.RtsThreshold = MAX_RTS_THRESHOLD;

						DBGPRINT(RT_DEBUG_TRACE, "%s::(RTSThreshold=%d)\n", __FUNCTION__, pAd->PortCfg.RtsThreshold);
					}
					//FragThreshold
					if (RTMPGetKeyParameter("FragThreshold", tmpbuf, 10, buffer))
					{
						ulInfo = simple_strtol(tmpbuf, 0, 10);

						if ( (ulInfo >= MIN_FRAG_THRESHOLD) && (ulInfo <= MAX_FRAG_THRESHOLD))
							pAd->PortCfg.FragmentThreshold = (USHORT)ulInfo;
						else
							pAd->PortCfg.FragmentThreshold = MAX_FRAG_THRESHOLD;

						if (pAd->PortCfg.FragmentThreshold == MAX_FRAG_THRESHOLD)
							pAd->PortCfg.bFragmentZeroDisable = TRUE;
						else
							pAd->PortCfg.bFragmentZeroDisable = FALSE;

						DBGPRINT(RT_DEBUG_TRACE, "%s::(FragThreshold=%d)\n", __FUNCTION__, ulInfo);
					}				   
					//TxBurst
					if (RTMPGetKeyParameter("TxBurst", tmpbuf, 10, buffer))
					{
						ulInfo = simple_strtol(tmpbuf, 0, 10);

						if (ulInfo == 1)
							pAd->PortCfg.bEnableTxBurst = TRUE;
						else
							pAd->PortCfg.bEnableTxBurst = FALSE;

						DBGPRINT(RT_DEBUG_TRACE, "%s::(TxBurst=%d)\n", __FUNCTION__, pAd->PortCfg.bEnableTxBurst);
					} 
					
#ifdef AGGREGATION_SUPPORT
					//PktAggregate
					if(RTMPGetKeyParameter("PktAggregate", tmpbuf, 10, buffer))
					{
						if(simple_strtol(tmpbuf, 0, 10) != 0)  //Enable
							pAd->PortCfg.bAggregationCapable = TRUE;
						else //Disable
							pAd->PortCfg.bAggregationCapable = FALSE;

						DBGPRINT(RT_DEBUG_TRACE, "%s::(PktAggregate=%d)\n", __FUNCTION__, pAd->PortCfg.bAggregationCapable);
					}
#else
					pAd->PortCfg.bAggregationCapable = FALSE;
#endif /* !AGGREGATION_SUPPORT */

					//TurboRate
					if (RTMPGetKeyParameter("TurboRate", tmpbuf, 10, buffer))
					{
						ulInfo = simple_strtol(tmpbuf, 0, 10);

						if (ulInfo == 1)
							pAd->PortCfg.EnableTurboRate = TRUE;
						else
							pAd->PortCfg.EnableTurboRate = FALSE;

						DBGPRINT(RT_DEBUG_TRACE, "%s::(TurboRate=%d)\n", __FUNCTION__, pAd->PortCfg.EnableTurboRate);
					}
					
#if 0 
					//WmmCapable
					if(RTMPGetKeyParameter("WmmCapable", tmpbuf, 32, buffer))
					{
						if(simple_strtol(tmpbuf, 0, 10) != 0)  //Enable
							pAd->PortCfg.bWmmCapable = TRUE;
						else //Disable
							pAd->PortCfg.bWmmCapable = FALSE;
							
						DBGPRINT(RT_DEBUG_TRACE, "%s::(WmmCapable=%d)\n", __FUNCTION__, pAd->PortCfg.bWmmCapable); 
					}
					//AckPolicy for AC_BK, AC_BE, AC_VI, AC_VO
					if(RTMPGetKeyParameter("AckPolicy", tmpbuf, 32, buffer))
					{
						INT 	i;
						PUCHAR	macptr;
						
					    for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
					    {
				            pAd->PortCfg.AckPolicy[i] = (UCHAR)simple_strtol(macptr, 0, 10);

				            DBGPRINT(RT_DEBUG_TRACE, "%s::(AckPolicy[%d]=%d)\n", __FUNCTION__, i, pAd->PortCfg.AckPolicy[i]);
					    }
					}
#else	 
					pAd->PortCfg.bWmmCapable = FALSE;
#endif

					//IEEE80211H
					if(RTMPGetKeyParameter("IEEE80211H", tmpbuf, 10, buffer))
					{
						if(simple_strtol(tmpbuf, 0, 10) != 0)  //Enable
							pAd->PortCfg.bIEEE80211H = TRUE;
						else //Disable
							pAd->PortCfg.bIEEE80211H = FALSE;

						DBGPRINT(RT_DEBUG_TRACE, "%s::(IEEE80211H=%d)\n", __FUNCTION__, pAd->PortCfg.bIEEE80211H);
					}
					//POWER_MODE
					if (RTMPGetKeyParameter("PSMode", tmpbuf, 10, buffer))
					{
						if (pAd->PortCfg.BssType == BSS_INFRA)
						{
							if ((strcmp(tmpbuf, "MAX_PSP") == 0) || (strcmp(tmpbuf, "max_psp") == 0))
							{
								// do NOT turn on PSM bit here, wait until MlmeCheckForPsmChange()
								// to exclude certain situations.
								//	   MlmeSetPsmBit(pAd, PWR_SAVE);
								if (pAd->PortCfg.bWindowsACCAMEnable == FALSE)
									pAd->PortCfg.WindowsPowerMode = Ndis802_11PowerModeMAX_PSP;
								pAd->PortCfg.WindowsBatteryPowerMode = Ndis802_11PowerModeMAX_PSP;
								OPSTATUS_SET_FLAG(pAd, fOP_STATUS_RECEIVE_DTIM);
								pAd->PortCfg.DefaultListenCount = 5;
							}
							else if ((strcmp(tmpbuf, "Fast_PSP") == 0) || (strcmp(tmpbuf, "fast_psp") == 0) 
								|| (strcmp(tmpbuf, "FAST_PSP") == 0))
							{
								// do NOT turn on PSM bit here, wait until MlmeCheckForPsmChange()
								// to exclude certain situations.
								//	   MlmeSetPsmBit(pAd, PWR_SAVE);
								OPSTATUS_SET_FLAG(pAd, fOP_STATUS_RECEIVE_DTIM);
								if (pAd->PortCfg.bWindowsACCAMEnable == FALSE)
									pAd->PortCfg.WindowsPowerMode = Ndis802_11PowerModeFast_PSP;
								pAd->PortCfg.WindowsBatteryPowerMode = Ndis802_11PowerModeFast_PSP;
								pAd->PortCfg.DefaultListenCount = 3;
							}
							else
							{ //Default Ndis802_11PowerModeCAM
								// clear PSM bit immediately
								MlmeSetPsmBit(pAd, PWR_ACTIVE);
								OPSTATUS_SET_FLAG(pAd, fOP_STATUS_RECEIVE_DTIM);
								if (pAd->PortCfg.bWindowsACCAMEnable == FALSE)
									pAd->PortCfg.WindowsPowerMode = Ndis802_11PowerModeCAM;
								pAd->PortCfg.WindowsBatteryPowerMode = Ndis802_11PowerModeCAM;
							}
							DBGPRINT(RT_DEBUG_TRACE, "%s::(PSMode=%d)\n", __FUNCTION__, pAd->PortCfg.WindowsPowerMode);
						}
					}
					//AdhocOfdm
					if (RTMPGetKeyParameter("AdhocOfdm", tmpbuf, 32, buffer))
					{
					    ulInfo = simple_strtol(tmpbuf, 0, 10);

					    if (ulInfo <= ADHOC_11ABG_MIXED)
					        pAd->PortCfg.AdhocMode = ulInfo;
                        else
                            pAd->PortCfg.AdhocMode = 0;

                        DBGPRINT(RT_DEBUG_TRACE, "%s::(AdhocOfdm=%d)\n", __FUNCTION__, pAd->PortCfg.AdhocMode);
					}
			        // FastRoaming
					if (RTMPGetKeyParameter("FastRoaming", tmpbuf, 32, buffer))
					{
                   	    ulInfo = simple_strtol(tmpbuf, 0, 10);

					    if (ulInfo == 1)
					        pAd->PortCfg.bFastRoaming = TRUE;
                        else
                            pAd->PortCfg.bFastRoaming = FALSE;

                        DBGPRINT(RT_DEBUG_TRACE, "%s::(FastRoaming=%d)\n", __FUNCTION__, pAd->PortCfg.bFastRoaming);
					}
					// RoamThreshold
					if (RTMPGetKeyParameter("RoamThreshold", tmpbuf, 32, buffer))
					{
                        ulInfo = simple_strtol(tmpbuf, 0, 10);
                            
                        if (ulInfo > 90 || ulInfo < 60)
                            pAd->PortCfg.dBmToRoam = 70;
                        else    
                            pAd->PortCfg.dBmToRoam = ulInfo;

                        DBGPRINT(RT_DEBUG_TRACE, "%s::(RoamThreshold=%d)\n", __FUNCTION__, (-1)*(pAd->PortCfg.dBmToRoam));
					}    
					//AuthMode
// add by johnli
#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT
				if (pAd->PortCfg.bNativeWpa == FALSE)
				{
#endif // NATIVE_WPA_SUPPLICANT_SUPPORT //
// end johnli
					if (RTMPGetKeyParameter("AuthMode", tmpbuf, 32, buffer))
					{ 
						if ((strcmp(tmpbuf, "WEPAUTO") == 0) || (strcmp(tmpbuf, "wepauto") == 0))
							pAd->PortCfg.AuthMode = Ndis802_11AuthModeAutoSwitch;
						else if ((strcmp(tmpbuf, "SHARED") == 0) || (strcmp(tmpbuf, "shared") == 0))
							pAd->PortCfg.AuthMode = Ndis802_11AuthModeShared;
						else if ((strcmp(tmpbuf, "WPAPSK") == 0) || (strcmp(tmpbuf, "wpapsk") == 0))
							pAd->PortCfg.AuthMode = Ndis802_11AuthModeWPAPSK;
						else if ((strcmp(tmpbuf, "WPANONE") == 0) || (strcmp(tmpbuf, "wpanone") == 0))
							pAd->PortCfg.AuthMode = Ndis802_11AuthModeWPANone;
						else if ((strcmp(tmpbuf, "WPA2PSK") == 0) || (strcmp(tmpbuf, "wpa2psk") == 0))
							pAd->PortCfg.AuthMode = Ndis802_11AuthModeWPA2PSK;		
#ifdef RALINK_WPA_SUPPLICANT_SUPPORT
                        else if ((strcmp(tmpbuf, "WPA") == 0) || (strcmp(tmpbuf, "wpa") == 0))
                            pAd->PortCfg.AuthMode = Ndis802_11AuthModeWPA;  
                        else if ((strcmp(tmpbuf, "WPA2") == 0) || (strcmp(tmpbuf, "wpa2") == 0))
                            pAd->PortCfg.AuthMode = Ndis802_11AuthModeWPA2;
#endif
						else
							pAd->PortCfg.AuthMode = Ndis802_11AuthModeOpen;

						pAd->PortCfg.PortSecured = WPA_802_1X_PORT_NOT_SECURED;
						DBGPRINT(RT_DEBUG_TRACE, "%s::(AuthMode=%d)\n", __FUNCTION__, pAd->PortCfg.AuthMode);
					}
					//EncrypType
					if (RTMPGetKeyParameter("EncrypType", tmpbuf, 32, buffer))
					{
						if ((strcmp(tmpbuf, "WEP") == 0) || (strcmp(tmpbuf, "wep") == 0))
						{
							pAd->PortCfg.WepStatus	 = Ndis802_11WEPEnabled;
							pAd->PortCfg.PairCipher  = Ndis802_11WEPEnabled;
							pAd->PortCfg.GroupCipher = Ndis802_11WEPEnabled;
						}
						else if ((strcmp(tmpbuf, "TKIP") == 0) || (strcmp(tmpbuf, "tkip") == 0))
						{
							pAd->PortCfg.WepStatus	 = Ndis802_11Encryption2Enabled;
							pAd->PortCfg.PairCipher  = Ndis802_11Encryption2Enabled;
							pAd->PortCfg.GroupCipher = Ndis802_11Encryption2Enabled;
						}
						else if ((strcmp(tmpbuf, "AES") == 0) || (strcmp(tmpbuf, "aes") == 0))
						{
							pAd->PortCfg.WepStatus	 = Ndis802_11Encryption3Enabled;
							pAd->PortCfg.PairCipher  = Ndis802_11Encryption3Enabled;
							pAd->PortCfg.GroupCipher = Ndis802_11Encryption3Enabled;   
						}	 
						else
						{
							pAd->PortCfg.WepStatus	 = Ndis802_11WEPDisabled;
							pAd->PortCfg.PairCipher  = Ndis802_11WEPDisabled;
							pAd->PortCfg.GroupCipher = Ndis802_11WEPDisabled;
						}
						RTMPMakeRSNIE(pAd, pAd->PortCfg.GroupCipher);
						DBGPRINT(RT_DEBUG_TRACE, "%s::(EncrypType=%d)\n", __FUNCTION__, pAd->PortCfg.WepStatus);
					}	  
					//WPAPSK_KEY
					if (RTMPGetKeyParameter("WPAPSK", tmpbuf, 255, buffer))
					{
						int 	err=0;

                        // the end of this string
                        tmpbuf[strlen(tmpbuf)]='\0';
                        
						if ((pAd->PortCfg.AuthMode != Ndis802_11AuthModeWPAPSK) &&\
							(pAd->PortCfg.AuthMode != Ndis802_11AuthModeWPA2PSK) &&\
							(pAd->PortCfg.AuthMode != Ndis802_11AuthModeWPANone) )
						{
							err = 1;
						}
						else if ((strlen(tmpbuf) >= 8) && (strlen(tmpbuf) < 64))
						{
							PasswordHash((char *)tmpbuf, pAd->PortCfg.Ssid, pAd->PortCfg.SsidLen, keyMaterial);
							NdisMoveMemory(pAd->PortCfg.PskKey.Key, keyMaterial, 32);
							
						}
						else if (strlen(tmpbuf) == 64)
						{							 
							AtoH(tmpbuf, keyMaterial, 32);
							NdisMoveMemory(pAd->PortCfg.PskKey.Key, keyMaterial, 32);
						}
						else
						{	
							err = 1;
							DBGPRINT(RT_DEBUG_ERROR, "%s::(WPAPSK key-string required 8 ~ 64 characters!)\n", __FUNCTION__);
						}
						
						if (err == 0)
						{  
							RTMPMakeRSNIE(pAd, pAd->PortCfg.GroupCipher);
							
							if ((pAd->PortCfg.AuthMode == Ndis802_11AuthModeWPAPSK) ||\
								(pAd->PortCfg.AuthMode == Ndis802_11AuthModeWPA2PSK))
							{
								// Use RaConfig as PSK agent.
								// Start STA supplicant state machine
								pAd->PortCfg.WpaState = SS_START;
							}
							else if (pAd->PortCfg.AuthMode == Ndis802_11AuthModeWPANone)
							{							 
								// Set wpapsk key later
								// set the flag 
								pAd->PortCfg.WpaState = SS_START;
							}
							DBGPRINT(RT_DEBUG_TRACE, "%s::(WPAPSK=%s)\n", __FUNCTION__, tmpbuf);
						}	  
					}
                    //DefaultKeyID, KeyType, KeyStr
                    DBGPRINT(RT_DEBUG_TRACE, "%s::(DefaultKeyID, KeyType, KeyStr !!!!!!!!!)\n", __FUNCTION__);
					rtmp_read_key_parms_from_file(pAd, tmpbuf, buffer);

                    //
					// Set wep keys into ASIC
                    //
					if ((pAd->PortCfg.WepStatus == Ndis802_11WEPEnabled) &&\
					    ((pAd->PortCfg.AuthMode == Ndis802_11AuthModeOpen) ||\
					     (pAd->PortCfg.AuthMode == Ndis802_11AuthModeShared) ||\
					     (pAd->PortCfg.AuthMode == Ndis802_11AuthModeAutoSwitch)))
					{	
                        int idx;
#ifdef RALINK_WPA_SUPPLICANT_SUPPORT
		                int KeyIdx = pAd->PortCfg.DefaultKeyId;

					    // Record the user desired wep setting
				        pAd->PortCfg.DesireSharedKey[KeyIdx].KeyLen = pAd->SharedKey[KeyIdx].KeyLen;
					    NdisMoveMemory(pAd->PortCfg.DesireSharedKey[KeyIdx].Key, pAd->SharedKey[KeyIdx].Key, pAd->SharedKey[KeyIdx].KeyLen);						                        
					    pAd->PortCfg.DesireSharedKey[KeyIdx].CipherAlg = pAd->SharedKey[KeyIdx].CipherAlg;												
#endif						

                        for (idx=0; idx<4; idx++)
                        {
						    if (pAd->SharedKey[idx].KeyLen > 0)
							    AsicAddSharedKeyEntry(pAd, 
												    0, 
												    idx, 
												    pAd->SharedKey[idx].CipherAlg, 
												    pAd->SharedKey[idx].Key, 
												    NULL,
												    NULL);
	                    }											    						    
					}				
// add by johnli
#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT
					if (RTMPGetKeyParameter("NativeWpa", tmpbuf, 10, buffer))
					{
						int i;

						if(simple_strtol(tmpbuf, 0, 10) == 1)  //Enable
						{
							pAd->PortCfg.AuthMode = Ndis802_11AuthModeOpen;
							pAd->PortCfg.WepStatus = Ndis802_11EncryptionDisabled;
							pAd->PortCfg.PairCipher = Ndis802_11EncryptionDisabled;
							pAd->PortCfg.GroupCipher = Ndis802_11EncryptionDisabled;
							pAd->PortCfg.bMixCipher = FALSE;
							pAd->PortCfg.DefaultKeyId = 0;

							pAd->PortCfg.PortSecured = WPA_802_1X_PORT_NOT_SECURED;
							pAd->PortCfg.WpaState         = SS_NOTUSE;

							RTMPWPARemoveAllKeys(pAd);

							pAd->PortCfg.bAutoReconnect = FALSE;
							pAd->PortCfg.WPA_Supplicant = TRUE;
							pAd->PortCfg.bNativeWpa = TRUE;
						}
						else //Disable
						{
							pAd->PortCfg.bNativeWpa = FALSE;
						}        
						DBGPRINT(RT_DEBUG_TRACE, "%s::(NativeWpa=%d)\n", __FUNCTION__, pAd->PortCfg.bNativeWpa); 
					}
				}
#endif // NATIVE_WPA_SUPPLICANT_SUPPORT //
// end johnli
				}   /* end of reading parameters */
			}
			else
			{
				Status = NDIS_STATUS_FAILURE;
				DBGPRINT(RT_DEBUG_TRACE, "--> %s does not have a write method\n", src);
			}

			/* close firmware file */
			if (IS_FILE_OPEN_ERR(srcf))
				;
			else
			{
				retval = RtmpOSFileClose(srcf);
				if (retval)
				{
					DBGPRINT(RT_DEBUG_ERROR, "--> Error %d closing %s\n", -retval, src);
				}
			} 
		} // else (IS_ERR(srcf))	  
	} //if (src && *src)

  
done:
	// Error handling
	if (mpool != NULL)
		kfree(mpool);
	
	RtmpOSFSInfoChange(&osFSInfo, FALSE);

}

#ifndef BIG_ENDIAN
/*
	========================================================================

	Routine Description:
		Compare two memory block

	Arguments:
		Adapter 					Pointer to our adapter

	Return Value:
		1:			memory are equal
		0:			memory are not equal

	Note:

	========================================================================
*/
ULONG	RTMPEqualMemory(
	IN	PVOID	pSrc1,
	IN	PVOID	pSrc2,
	IN	ULONG	Length)
{
	PUCHAR	pMem1;
	PUCHAR	pMem2;
	ULONG	Index = 0;

	pMem1 = (PUCHAR) pSrc1;
	pMem2 = (PUCHAR) pSrc2;

	for (Index = 0; Index < Length; Index++)
	{
		if (pMem1[Index] != pMem2[Index])
		{
			break;
		}
	}

	if (Index == Length)
	{
		return (1);
	}
	else
	{
		return (0);
	}
}
#endif
/*
	========================================================================

	Routine Description:
		Compare two memory block

	Arguments:
		pSrc1		Pointer to first memory address
		pSrc2		Pointer to second memory addres

	Return Value:
		0:			memory is equal
		1:			pSrc1 memory is larger
		2:			pSrc2 memory is larger

	Note:

	========================================================================
*/
ULONG	RTMPCompareMemory(
	IN	PVOID	pSrc1,
	IN	PVOID	pSrc2,
	IN	ULONG	Length)
{
	PUCHAR	pMem1;
	PUCHAR	pMem2;
	ULONG	Index = 0;

	pMem1 = (PUCHAR) pSrc1;
	pMem2 = (PUCHAR) pSrc2;

	for (Index = 0; Index < Length; Index++)
	{
		if (pMem1[Index] > pMem2[Index])
			return (1);
		else if (pMem1[Index] < pMem2[Index])
			return (2);
	}

	// Equal
	return (0);
}

/*
	========================================================================
	
	Routine Description:
		Zero out memory block

	Arguments:
		pSrc1		Pointer to memory address
		Length		Size

	Return Value:
		None
		
	Note:
		
	========================================================================
*/
VOID	RTMPZeroMemory(
	IN	PVOID	pSrc,
	IN	ULONG	Length)
{
	memset(pSrc, 0, Length);
}

VOID	RTMPFillMemory(
	IN	PVOID	pSrc,
	IN	ULONG	Length,
	IN	UCHAR	Fill)
{
	memset(pSrc, Fill, Length);
}

/*
	========================================================================
	
	Routine Description:
		Copy data from memory block 1 to memory block 2

	Arguments:
		pDest		Pointer to destination memory address
		pSrc		Pointer to source memory address
		Length		Copy size
		
	Return Value:
		None

	Note:
		
	========================================================================
*/
VOID	RTMPMoveMemory(
	OUT PVOID	pDest,
	IN	PVOID	pSrc,
	IN	ULONG	Length)
{
#ifdef RTMP_EMBEDDED
	if(Length <= 8)
	{
		*(((PUCHAR)pDest)++) = *(((PUCHAR)pSrc)++);
		if(--Length == 0)	return;
		*(((PUCHAR)pDest)++) = *(((PUCHAR)pSrc)++);
		if(--Length == 0)	return;
		*(((PUCHAR)pDest)++) = *(((PUCHAR)pSrc)++);
		if(--Length == 0)	return;
		*(((PUCHAR)pDest)++) = *(((PUCHAR)pSrc)++);
		if(--Length == 0)	return;
		*(((PUCHAR)pDest)++) = *(((PUCHAR)pSrc)++);
		if(--Length == 0)	return;
		*(((PUCHAR)pDest)++) = *(((PUCHAR)pSrc)++);
		if(--Length == 0)	return;
		*(((PUCHAR)pDest)++) = *(((PUCHAR)pSrc)++);
		if(--Length == 0)	return;
		*(((PUCHAR)pDest)++) = *(((PUCHAR)pSrc)++);
		if(--Length == 0)	return;
	}
	else
		memcpy(pDest, pSrc, Length);
#else
	memcpy(pDest, pSrc, Length);
#endif
}

/*
	========================================================================
	
	Routine Description:
		Initialize port configuration structure

	Arguments:
		Adapter			Pointer to our adapter

	Return Value:
		None

	Note:
		
	========================================================================
*/
VOID	PortCfgInit(
	IN	PRTMP_ADAPTER pAd)
{
	UINT i;

	DBGPRINT(RT_DEBUG_TRACE, "--> PortCfgInit\n");	  
	
	//
	//	part I. intialize common configuration
	//
	for(i = 0; i < SHARE_KEY_NUM; i++) 
	{
		pAd->SharedKey[i].KeyLen = 0;  
		pAd->SharedKey[i].CipherAlg = CIPHER_NONE;	
	}

	pAd->Antenna.field.TxDefaultAntenna = 2;	// Ant-B
	pAd->Antenna.field.RxDefaultAntenna = 2;	// Ant-B
	pAd->Antenna.field.NumOfAntenna = 2;

	pAd->LedCntl.field.LedMode = LED_MODE_DEFAULT;
	pAd->LedIndicatorStrength = 0;
	pAd->bAutoTxAgcA = FALSE;			// Default is OFF
	pAd->bAutoTxAgcG = FALSE;			// Default is OFF
	pAd->RfIcType = RFIC_5226;

	pAd->PortCfg.Dsifs = 10;	  // in units of usec 
	pAd->PortCfg.PrivacyFilter = Ndis802_11PrivFilterAcceptAll;
	pAd->PortCfg.TxPower = 100; //mW
	pAd->PortCfg.TxPowerPercentage = 0xffffffff; // AUTO
	pAd->PortCfg.TxPowerDefault = 0xffffffff; // AUTO
	pAd->PortCfg.TxPreamble = Rt802_11PreambleAuto; // use Long preamble on TX by defaut
	pAd->PortCfg.bUseZeroToDisableFragment = FALSE;
	pAd->PortCfg.RtsThreshold = 2347;
	pAd->PortCfg.FragmentThreshold = 2346;
    pAd->PortCfg.dBmToRoam = 70;    // default threshold used
	pAd->PortCfg.UseBGProtection = 0;	 // 0: AUTO
	pAd->PortCfg.bEnableTxBurst = 0;
	pAd->PortCfg.PhyMode = 0xff;	 // unknown
	pAd->PortCfg.BandState = UNKNOWN_BAND;
	pAd->PortCfg.UseShortSlotTime = TRUE;   // default short slot used, it depends on AP's capability
	
	pAd->bAcceptDirect = TRUE;
	pAd->bAcceptMulticast = FALSE;
	pAd->bAcceptBroadcast = TRUE;
	pAd->bAcceptAllMulticast = TRUE;

	pAd->bLocalAdminMAC = FALSE; //TRUE;


    pAd->PortCfg.RadarDetect.CSPeriod = 10;
	pAd->PortCfg.RadarDetect.CSCount = 0;
	pAd->PortCfg.RadarDetect.RDMode = RD_NORMAL_MODE;


	//
	// part II. intialize STA specific configuration
	//
	pAd->PortCfg.Psm = PWR_ACTIVE;
	pAd->PortCfg.BeaconPeriod = 100;	 // in mSec

	pAd->PortCfg.ScanCnt = 0;

	pAd->PortCfg.AuthMode = Ndis802_11AuthModeOpen;
	pAd->PortCfg.WepStatus = Ndis802_11EncryptionDisabled;
	pAd->PortCfg.PairCipher = Ndis802_11EncryptionDisabled;
	pAd->PortCfg.GroupCipher = Ndis802_11EncryptionDisabled;
	pAd->PortCfg.bMixCipher = FALSE;
	pAd->PortCfg.DefaultKeyId = 0;

	// 802.1x port control
	pAd->PortCfg.PortSecured = WPA_802_1X_PORT_NOT_SECURED;
	pAd->PortCfg.LastMicErrorTime = 0;
	pAd->PortCfg.MicErrCnt		  = 0;
	pAd->PortCfg.bBlockAssoc	  = FALSE;
	pAd->PortCfg.WpaState		  = SS_NOTUSE;		// Handle by microsoft unless RaConfig changed it.

#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT
	pAd->PortCfg.wx_auth_alg = 0;
//	pAd->PortCfg.wx_eapol_flags = 0;
	pAd->PortCfg.wx_need_sync = 0;
	pAd->PortCfg.wx_groupCipher = 0;
	pAd->PortCfg.wx_key_mgmt = 0;
	pAd->PortCfg.wx_pairwise = 0;
	pAd->PortCfg.wx_wpa_version = 0;
	pAd->PortCfg.bNativeWpa = FALSE;  // add  by johnli
#endif

	pAd->PortCfg.RssiTrigger = 0;
	pAd->PortCfg.LastRssi = 0;
	pAd->PortCfg.LastRssi2 = 0;
	pAd->PortCfg.AvgRssi  = 0;
	pAd->PortCfg.AvgRssiX8 = 0;
	pAd->PortCfg.RssiTriggerMode = RSSI_TRIGGERED_UPON_BELOW_THRESHOLD;
	pAd->PortCfg.AtimWin = 0;
	pAd->PortCfg.DefaultListenCount = 3;//default listen count;
	pAd->PortCfg.BssType = BSS_INFRA;  // BSS_INFRA or BSS_ADHOC
	pAd->PortCfg.AdhocMode = 0;

	// global variables mXXXX used in MAC protocol state machines
	OPSTATUS_SET_FLAG(pAd, fOP_STATUS_RECEIVE_DTIM);
	OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_ADHOC_ON);
	OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_INFRA_ON);

	// PHY specification
	pAd->PortCfg.PhyMode = PHY_11ABG_MIXED; 	// default PHY mode
	OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_SHORT_PREAMBLE_INUSED);  // CCK use LONG preamble

	// user desired power mode
	pAd->PortCfg.WindowsPowerMode = Ndis802_11PowerModeCAM;
	pAd->PortCfg.WindowsBatteryPowerMode = Ndis802_11PowerModeCAM;
	pAd->PortCfg.bWindowsACCAMEnable = FALSE;
	
    RTMPInitTimer(pAd, &pAd->PortCfg.QuickResponeForRateUpTimer, GET_TIMER_FUNCTION(StaQuickResponeForRateUpExec), pAd, FALSE);
	pAd->PortCfg.QuickResponeForRateUpTimerRunning = FALSE;


	pAd->PortCfg.bHwRadio  = TRUE; // Default Hardware Radio status is On
	pAd->PortCfg.bSwRadio  = TRUE; // Default Software Radio status is On
	pAd->PortCfg.bRadio    = TRUE; // bHwRadio && bSwRadio
	pAd->PortCfg.bHardwareRadio = FALSE;		// Default is OFF
	pAd->PortCfg.bShowHiddenSSID = FALSE;		// Default no show 
	
	// Nitro mode control
	pAd->PortCfg.bAutoReconnect = TRUE;
/* remvoe by johnli
#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT
	// If the device controlled by native wpa_supplicant wext dirver, we should not enable AutoReconnect.
	pAd->PortCfg.bAutoReconnect = FALSE;
#endif // NATIVE_WPA_SUPPLICANT_SUPPORT //
*/

	// Save the init time as last scan time, the system should do scan after 2 seconds.
	// This patch is for driver wake up from standby mode, system will do scan right away.
	pAd->PortCfg.LastScanTime = 0;

	// Default for extra information is not valid
	pAd->ExtraInfo = EXTRA_INFO_CLEAR;
	
	// Default Config change flag
	pAd->bConfigChanged = FALSE;

	
	//
	// part III. others
	//
	// dynamic BBP R17:sensibity tuning to overcome background noise
	pAd->BbpTuning.bEnable				  = TRUE;  
	pAd->BbpTuning.R17LowerBoundG		  = 0x20; // for best RX sensibility
	pAd->BbpTuning.R17UpperBoundG		  = 0x40; // for best RX noise isolation to prevent false CCA
	pAd->BbpTuning.R17LowerBoundA		  = 0x28; // for best RX sensibility
	pAd->BbpTuning.R17UpperBoundA		  = 0x48; // for best RX noise isolation to prevent false CCA
	pAd->BbpTuning.R17LowerUpperSelect	  = 0;	  // Default used LowerBound.
	pAd->BbpTuning.FalseCcaLowerThreshold = 100;
	pAd->BbpTuning.FalseCcaUpperThreshold = 512;   
	pAd->BbpTuning.R17Delta 			  = 4;

    pAd->Bbp94 = BBPR94_DEFAULT;
	pAd->BbpForCCK = FALSE;

//#ifdef RALINK_WPA_SUPPLICANT_SUPPORT
	pAd->PortCfg.IEEE8021X = 0;
	pAd->PortCfg.IEEE8021x_required_keys = 0;
	pAd->PortCfg.WPA_Supplicant = FALSE;
//#endif

	pAd->MaxTxQueueSize = MAX_TX_QUEUE_SIZE;


/* remove by johnli			
#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT
	pAd->PortCfg.WPA_Supplicant = TRUE;
#endif
*/

#ifdef RALINK_ATE
	memset(&pAd->ate, 0, sizeof(ATE_INFO));
	pAd->ate.Mode = ATE_STASTART;
	pAd->ate.TxCount = TX_RING_SIZE;
	pAd->ate.TxLength = 1024;
	pAd->ate.TxRate = RATE_11;
	pAd->ate.Channel = 1;
	pAd->ate.RFFreqOffset = 0;
	pAd->ate.Addr1[0] = 0x00;
	pAd->ate.Addr1[1] = 0x11;
	pAd->ate.Addr1[2] = 0x22;
	pAd->ate.Addr1[3] = 0xAA;
	pAd->ate.Addr1[4] = 0xBB;
	pAd->ate.Addr1[5] = 0xCC;
	memcpy(pAd->ate.Addr2, pAd->ate.Addr1, MAC_ADDR_LEN);
	memcpy(pAd->ate.Addr3, pAd->ate.Addr1, MAC_ADDR_LEN);
#endif	//RALINK_ATE

	RTMPInitTimer(pAd, &pAd->PortCfg.WpaDisassocAndBlockAssocTimer, &WpaDisassocApAndBlockAssoc, pAd, FALSE);//BensonLiu 07-11-22 add for countermeasure
	DBGPRINT(RT_DEBUG_TRACE, "<-- PortCfgInit\n");

}

UCHAR BtoH(
	IN CHAR		ch)
{
	if (ch >= '0' && ch <= '9') return (ch - '0');		  // Handle numerals
	if (ch >= 'A' && ch <= 'F') return (ch - 'A' + 0xA);  // Handle capitol hex digits
	if (ch >= 'a' && ch <= 'f') return (ch - 'a' + 0xA);  // Handle small hex digits
	return(255);
}

//
//	PURPOSE:  Converts ascii string to network order hex
//
//	PARAMETERS:
//	  src	 - pointer to input ascii string
//	  dest	 - pointer to output hex
//	  destlen - size of dest
//
//	COMMENTS:
//
//	  2 ascii bytes make a hex byte so must put 1st ascii byte of pair
//	  into upper nibble and 2nd ascii byte of pair into lower nibble.
//
VOID AtoH(
	IN CHAR		*src,
	OUT UCHAR	*dest,
	IN INT		destlen)
{
	CHAR *srcptr;
	PUCHAR destTemp;

	srcptr = src;	
	destTemp = (PUCHAR) dest; 

	while(destlen--)
	{
		*destTemp = BtoH(*srcptr++) << 4;	 // Put 1st ascii byte in upper nibble.
		*destTemp += BtoH(*srcptr++);	   // Add 2nd ascii byte to above.
		destTemp++;
	}
}

VOID	RTMPPatchMacBbpBug(
	IN	PRTMP_ADAPTER	pAd)
{
#if 0
	ULONG	Index;

	// Initialize BBP register to default value
	for (Index = 0; Index < NUM_BBP_REG_PARMS; Index++)
	{
		RTUSBWriteBBPRegister(pAd, BBPRegTable[Index].Register, (UCHAR)BBPRegTable[Index].Value);
	}
	
	// Initialize RF register to default value
	AsicSwitchChannel(pAd, pAd->PortCfg.Channel);
	AsicLockChannel(pAd, pAd->PortCfg.Channel);

	// Re-init BBP register from EEPROM value
	NICInitAsicFromEEPROM(pAd);
#endif	
}

// Unify all delay routine by using udelay
VOID	RTMPusecDelay(
	IN	ULONG	usec)
{
	ULONG	i;

	for (i = 0; i < (usec / 50); i++)
		udelay(50);

	if (usec % 50)
		udelay(usec % 50);
}

/*
	========================================================================
	
	Routine Description:
		Set LED Status

	Arguments:
		pAd						Pointer to our adapter
		Status					LED Status

	Return Value:
		None

	Note:
		
	========================================================================
*/
VOID	RTMPSetLED(
	IN PRTMP_ADAPTER	pAd, 
	IN UCHAR			Status)
{
	switch (Status)
	{
		case LED_LINK_DOWN:
			pAd->LedCntl.field.LinkGStatus = 0;
			pAd->LedCntl.field.LinkAStatus = 0;
			pAd->LedIndicatorStrength = 0;			
			RTUSBSetLED(pAd, pAd->LedCntl, pAd->LedIndicatorStrength);
			break;
		case LED_LINK_UP:
			if (pAd->PortCfg.Channel <= 14)
			{
				// 11 G mode
				pAd->LedCntl.field.LinkGStatus = 1;
				pAd->LedCntl.field.LinkAStatus = 0;
			}
			else
			{ 
				//11 A mode
				pAd->LedCntl.field.LinkGStatus = 0;
				pAd->LedCntl.field.LinkAStatus = 1;
			}
			
			RTUSBSetLED(pAd, pAd->LedCntl, pAd->LedIndicatorStrength);			
			break;
		case LED_RADIO_ON:
			pAd->LedCntl.field.RadioStatus = 1;
			RTUSBSetLED(pAd, pAd->LedCntl, pAd->LedIndicatorStrength);			
			break;
		case LED_HALT: 
			//Same as Radio Off.
		case LED_RADIO_OFF:
			pAd->LedCntl.field.RadioStatus = 0;
			pAd->LedCntl.field.LinkGStatus = 0;
			pAd->LedCntl.field.LinkAStatus = 0;
			pAd->LedIndicatorStrength = 0;
			RTUSBSetLED(pAd, pAd->LedCntl, pAd->LedIndicatorStrength);			
			break;
		default:
			DBGPRINT(RT_DEBUG_WARN, "RTMPSetLED::Unknown Status %d\n", Status);
			break;
	}
}

/*
	========================================================================
	
	Routine Description:
		Set LED Signal Stregth 

	Arguments:
		pAd						Pointer to our adapter
		Dbm						Signal Stregth

	Return Value:
		None

	Note:
		Can be run on any IRQL level. 

		According to Microsoft Zero Config Wireless Signal Stregth definition as belows.
		<= -90	No Signal
		<= -81	Very Low
		<= -71	Low
		<= -67	Good
		<= -57	Very Good
		 > -57	Excellent		
	========================================================================
*/
VOID RTMPSetSignalLED(
	IN PRTMP_ADAPTER	pAd, 
	IN NDIS_802_11_RSSI Dbm)
{
	USHORT		nLed = 0;

	if (Dbm <= -90)
		nLed = 0;
	else if (Dbm <= -81)
		nLed = 1;
	else if (Dbm <= -71)
		nLed = 2;
	else if (Dbm <= -67)
		nLed = 3;
	else if (Dbm <= -57)
		nLed = 4;
	else 
		nLed = 5;

	//
	// Update Signal Stregth to if changed.
	//
	if ((pAd->LedIndicatorStrength != nLed) && 
		(pAd->LedCntl.field.LedMode == LED_MODE_SIGNAL_STREGTH))
	{
		pAd->LedIndicatorStrength = nLed;
		RTUSBSetLED(pAd, pAd->LedCntl, pAd->LedIndicatorStrength);
	}
}

VOID RTMPCckBbpTuning(
	IN	PRTMP_ADAPTER	pAd, 
	IN	UINT			TxRate)
{
	CHAR		Bbp94 = 0xFF;
	USHORT		Value = 0;

	//
	// Do nothing if TxPowerEnable == FALSE
	//
	if (pAd->TxPowerDeltaConfig.field.TxPowerEnable == FALSE)
		return;
		
	if ((TxRate < RATE_FIRST_OFDM_RATE) && 
		(pAd->BbpForCCK == FALSE))
	{
		Bbp94 = pAd->Bbp94;
	
		if (pAd->TxPowerDeltaConfig.field.Type == 1)
		{
			Bbp94 += pAd->TxPowerDeltaConfig.field.DeltaValue;
		}
		else
		{
			Bbp94 -= pAd->TxPowerDeltaConfig.field.DeltaValue;
		}
		pAd->BbpForCCK = TRUE;
	}
	else if ((TxRate >= RATE_FIRST_OFDM_RATE) &&
		(pAd->BbpForCCK == TRUE))
	{
		Bbp94 = pAd->Bbp94;
		pAd->BbpForCCK = FALSE;
	}

	if ((Bbp94 >= 0) && (Bbp94 <= 0x0C))
	{
		Value = (Bbp94 << 8) + BBP_R94;
        RTUSBEnqueueCmdFromNdis(pAd, RT_OID_VENDOR_WRITE_BBP, TRUE, (PVOID) &Value, sizeof(Value));
	}
}


/*
	========================================================================

	Routine Description:
		Init timer objects

	Arguments:
		pAd			Pointer to our adapter
		pTimer				Timer structure
		pTimerFunc			Function to execute when timer expired

	Return Value:
		None

	Note:

	========================================================================
*/
VOID	RTMPInitTimer(
	IN	PRTMP_ADAPTER			pAd,
	IN	PRALINK_TIMER_STRUCT	pTimer,
	IN	PVOID					pTimerFunc,
	IN	PVOID					pData,
	IN  BOOLEAN					Repeat)
{
	pTimer->Valid = TRUE;
	pTimer->Repeat = Repeat;
	pTimer->State = FALSE;
	pTimer->cookie = (ULONG)pData;
	//RTMP_OS_Init_Timer(pAd, &pTimer->TimerObj, pTimerFunc, (PVOID) pTimer);
	
	init_timer(&pTimer->TimerObj);
    pTimer->TimerObj.data = (unsigned long)pTimer;
    pTimer->TimerObj.function = pTimerFunc;

}

/*
	========================================================================

	Routine Description:
		Init timer objects

	Arguments:
		pAd			Pointer to our adapter
		pTimer				Timer structure
		Value				Timer value in milliseconds

	Return Value:
		None

	Note:

	========================================================================
*/
VOID	RTMPSetTimer(
	IN	PRALINK_TIMER_STRUCT	pTimer,
	IN	ULONG					Value)
{
	if (pTimer->Valid)
	{
		//Benson 07-11-22 add for countermeasure
		if (timer_pending(&pTimer->TimerObj))
		{	
			del_timer_sync(&pTimer->TimerObj);	
		}
		//Benson 07-11-22 end for countermeasure
		pTimer->TimerValue = Value;
		pTimer->State = FALSE;
		Value = ((Value*HZ) / 1000);
		pTimer->TimerObj.expires = jiffies + Value;
		add_timer(&pTimer->TimerObj);
	}
	else
	{
	 	DBGPRINT_ERR("RTMPSetTimer failed, Timer hasn't been initialize!\n");
	}
}

/*
VOID RTMP_SetPeriodicTimer(
	IN	NDIS_MINIPORT_TIMER *pTimer,
	IN	unsigned long timeout)
{
	timeout = ((timeout*HZ) / 1000);
	pTimer->expires = jiffies + timeout;
	add_timer(pTimer);
}
*/
/*
	========================================================================

	Routine Description:
		Modify timer objects

	Arguments:
		pAd			Pointer to our adapter
		pTimer				Timer structure
		Value				Timer value in milliseconds

	Return Value:
		None

	Note:

	========================================================================
*/
VOID	RTMPModTimer(
	IN	PRALINK_TIMER_STRUCT	pTimer,
	IN	ULONG					Value)
{
	if (pTimer->Valid)
	{
		pTimer->TimerValue = Value;
		pTimer->State = FALSE;
		//RTMP_OS_Mod_Timer(&pTimer->TimerObj,Value);
	}
	else
	{
	 	DBGPRINT_ERR("RTMPSetTimer failed, Timer hasn't been initialize!\n");
	}
}

/*
	========================================================================

	Routine Description:
		Cancel timer objects

	Arguments:
		pTimer				Timer structure

	Return Value:
		None

	Note:
		Reset NIC to initial state AS IS system boot up time.

	========================================================================
*/
VOID    RTMPCancelTimer(
	IN  PRALINK_TIMER_STRUCT    pTimer,
	OUT BOOLEAN                 *pCancelled)
{
	if (pTimer->Valid)
	{
    	if (pTimer->State == FALSE)
    	{
			//RTMP_OS_Del_Timer(&pTimer->TimerObj, pCancelled);
			if(timer_pending(&pTimer->TimerObj))
				*pCancelled = del_timer_sync(&pTimer->TimerObj);
			else
				*pCancelled = TRUE;			
    	}
		else
		  *pCancelled=pTimer->State;	//added by wy
		
        if (*pCancelled == TRUE)
			pTimer->State = TRUE;
		
		if(*pCancelled==FALSE)
		DBGPRINT_ERR("CancelTimer failed ...............!\n");
	}
	else
		DBGPRINT_ERR("NdisMCancelTimer failed, Timer hasn't been initialize!\n");
}



