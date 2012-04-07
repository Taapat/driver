#ifndef	__RTL8712_RF_H_ 
#define __RTL8712_RF_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

#include <rtw_mp_phy_regdef.h>

#define WIFI_20MHZ_MODE			0x0000
#define WIFI_40MHZ_MODE			0x8000

// Bandwidth Offset
#define HAL_PRIME_CHNL_OFFSET_DONT_CARE	0
#define HAL_PRIME_CHNL_OFFSET_LOWER		1
#define HAL_PRIME_CHNL_OFFSET_UPPER		2

/*
u32 get_bbreg(PADAPTER pAdapter ,u16 offset ,u32 bitmask);
u8 set_bbreg(PADAPTER pAdapter ,u16 offset ,u32 bitmask, u32 value);
u32 get_rfreg(PADAPTER pAdapter ,u8 path,u8 offset,u32 bitmask);
u8 set_rfreg(PADAPTER pAdapter ,u8 path,u8 offset,u32 bitmask,u32 value);
*/

u32 get_efuse_content(_adapter *padapter, u16 offset);
void dump_efuse_content(_adapter *padapter, unsigned int *pbuf, int sz);



void set_channel_and_txpower(_adapter *padapter, u8 level, u8 ch);
void switchBWOPMODE(_adapter *padapter, unsigned short bwmode, unsigned char channel_offset);


//int set_init_ra_tbl(PADAPTER pAdapter, unsigned short param, unsigned int bitmap);

//u8 get_rf_mimo_mode(_adapter *padapter);

#endif //_RTL8712_RF_H_

