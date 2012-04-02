#ifndef __USB_HAL_H__
#define __USB_HAL_H__

//u32 rtl8192cu_hal_init(_adapter * adapter);
//u32 rtl8192cu_hal_deinit(_adapter * adapter);

//unsigned int rtl8192cu_inirp_init(_adapter * padapter);
//unsigned int rtl8192cu_inirp_deinit(_adapter * padapter);

//void rtl8192cu_interface_configure(_adapter *padapter);

void rtl8192cu_set_hal_ops(_adapter * padapter);

#endif //__USB_HAL_H__

