#ifndef __USB_OPS_H_
#define __USB_OPS_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <osdep_intf.h>


void rtl8192cu_set_intf_ops(struct _io_ops *pops);

void rtl8192cu_trigger_gpio_0(_adapter *padapter);

void rtl8192cu_recv_tasklet(void *priv);

#endif
