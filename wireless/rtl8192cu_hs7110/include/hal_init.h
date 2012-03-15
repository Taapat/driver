
#ifndef __HAL_INIT_H__
#define __HAL_INIT_H__

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>



enum _CHIP_TYPE {

	NULL_CHIP_TYPE,
	RTL8712_8188S_8191S_8192S,
	RTL8188C_8192C,
	RTL8192D,
	MAX_CHIP_TYPE
};

uint rtw_hal_init(_adapter *padapter);
uint rtw_hal_deinit(_adapter *padapter);
void rtw_hal_stop(_adapter *padapter);

#ifdef CONFIG_RTL8711
#include "rtl8711_hal.h"
#endif

#ifdef CONFIG_RTL8712
#include "rtl8712_hal.h"
#endif

#ifdef CONFIG_RTL8192C
#include "rtl8192c_hal.h"
#endif

#endif //__HAL_INIT_H__

