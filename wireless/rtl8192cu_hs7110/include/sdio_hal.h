#ifndef __SDIO_HAL_H__

#define __SDIO_HAL_H__

extern u8 sd_hal_bus_init(_adapter * adapter);
extern u8 sd_hal_bus_deinit(_adapter * adapter);


u8  sd_int_isr (IN PADAPTER	padapter);
void sd_int_dpc(PADAPTER padapter);


#endif //__SDIO_HAL_H__

