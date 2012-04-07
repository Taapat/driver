/******************************************************************************
* rtl871x_io.c                                                                                                                                 *
*                                                                                                                                          *
* Description :                                                                                                                       *
*                                                                                                                                           *
* Author :                                                                                                                       *
*                                                                                                                                         *
* History :                                                          
*
*                                        
*                                                                                                                                       *
* Copyright 2007, Realtek Corp.                                                                                                  *
*                                                                                                                                        *
* The contents of this file is the sole property of Realtek Corp.  It can not be                                     *
* be used, copied or modified without written permission from Realtek Corp.                                         *
*                                                                                                                                          *
*******************************************************************************/
/*

The purpose of rtl871x_io.c

a. provides the API 

b. provides the protocol engine

c. provides the software interface between caller and the hardware interface


Compiler Flag Option:

1. CONFIG_SDIO_HCI:
    a. USE_SYNC_IRP:  Only sync operations are provided.
    b. USE_ASYNC_IRP:Both sync/async operations are provided.

2. CONFIG_USB_HCI:
   a. USE_ASYNC_IRP: Both sync/async operations are provided.

3. CONFIG_CFIO_HCI:
   b. USE_SYNC_IRP: Only sync operations are provided.


Only sync read/write_mem operations are provided.

jackson@realtek.com.tw

*/

#define _RTL871X_IO_C_
#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <rtw_io.h>
#include <osdep_intf.h>

#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)
#error "Shall be Linux or Windows, but not both!\n"
#endif

#ifdef PLATFORM_LINUX
#include <linux/compiler.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/smp_lock.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/usb.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
#include <linux/usb_ch9.h>
#else
#include <linux/usb/ch9.h>
#endif
#include <linux/circ_buf.h>
#include <asm/uaccess.h>
#include <asm/byteorder.h>
#include <asm/atomic.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26))
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif
#endif

#ifdef CONFIG_SDIO_HCI
#include <sdio_ops.h>
#endif

#ifdef CONFIG_USB_HCI
#include <usb_ops.h>
#endif


u8 read8(_adapter *adapter, u32 addr)
{
	u8 r_val;
	//struct	io_queue  	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_priv *pio_priv = &adapter->iopriv;
	struct	intf_hdl		*pintfhdl = &(pio_priv->intf);
	u8 (*_read8)(struct intf_hdl *pintfhdl, u32 addr);
	_func_enter_;
	_read8 = pintfhdl->io_ops._read8;
	
	r_val = _read8(pintfhdl, addr);
	_func_exit_;
	return r_val;
}

u16 read16(_adapter *adapter, u32 addr)
{
	u16 r_val;
	//struct	io_queue  	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_priv *pio_priv = &adapter->iopriv;
	struct	intf_hdl		*pintfhdl = &(pio_priv->intf);
	u16 	(*_read16)(struct intf_hdl *pintfhdl, u32 addr);
	_func_enter_;
	_read16 = pintfhdl->io_ops._read16;

	r_val = _read16(pintfhdl, addr);
	_func_exit_;
	return r_val;
}
	
u32 read32(_adapter *adapter, u32 addr)
{
	u32 r_val;
	//struct	io_queue  	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_priv *pio_priv = &adapter->iopriv;
	struct	intf_hdl		*pintfhdl = &(pio_priv->intf);
	u32 	(*_read32)(struct intf_hdl *pintfhdl, u32 addr);
	_func_enter_;
	_read32 = pintfhdl->io_ops._read32;
	
	r_val = _read32(pintfhdl, addr);
	_func_exit_;
	return r_val;	

}

void write8(_adapter *adapter, u32 addr, u8 val)
{
	//struct	io_queue  	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_priv *pio_priv = &adapter->iopriv;
	struct	intf_hdl		*pintfhdl = &(pio_priv->intf);
	void (*_write8)(struct intf_hdl *pintfhdl, u32 addr, u8 val);
	_func_enter_;
	_write8 = pintfhdl->io_ops._write8;
	
	_write8(pintfhdl, addr, val);
}
void write16(_adapter *adapter, u32 addr, u16 val)
{
	//struct	io_queue  	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_priv *pio_priv = &adapter->iopriv;
	struct	intf_hdl		*pintfhdl = &(pio_priv->intf);
	void (*_write16)(struct intf_hdl *pintfhdl, u32 addr, u16 val);
	_func_enter_;
	_write16 = pintfhdl->io_ops._write16;
	
	_write16(pintfhdl, addr, val);
	_func_exit_;

}
void write32(_adapter *adapter, u32 addr, u32 val)
{
	//struct	io_queue  	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_priv *pio_priv = &adapter->iopriv;
	struct	intf_hdl		*pintfhdl = &(pio_priv->intf);
	void (*_write32)(struct intf_hdl *pintfhdl, u32 addr, u32 val);
	_func_enter_;
	_write32 = pintfhdl->io_ops._write32;
	
	_write32(pintfhdl, addr, val);	
	_func_exit_;

}
void writeN(_adapter *adapter, u32 addr ,u32 length , u8 *pdata)
{
	//struct	io_queue  	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_priv *pio_priv = &adapter->iopriv;
        struct	intf_hdl	*pintfhdl = (struct intf_hdl*)(&(pio_priv->intf));
	void (*_writeN)(struct intf_hdl *pintfhdl, u32 addr,u32 length, u8 *pdata);
	_func_enter_;
	_writeN = pintfhdl->io_ops._writeN;
	
	_writeN(pintfhdl, addr,length,pdata);	
	_func_exit_;

}
void write8_async(_adapter *adapter, u32 addr, u8 val)
{
	//struct	io_queue  	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_priv *pio_priv = &adapter->iopriv;
	struct	intf_hdl		*pintfhdl = &(pio_priv->intf);
	void (*_write8_async)(struct intf_hdl *pintfhdl, u32 addr, u8 val);
	_func_enter_;
	_write8_async = pintfhdl->io_ops._write8_async;
	
	_write8_async(pintfhdl, addr, val);	
	_func_exit_;

}
void write16_async(_adapter *adapter, u32 addr, u16 val)
{
	//struct	io_queue  	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_priv *pio_priv = &adapter->iopriv;
	struct	intf_hdl		*pintfhdl = &(pio_priv->intf);
	void (*_write16_async)(struct intf_hdl *pintfhdl, u32 addr, u16 val);
	_func_enter_;
	_write16_async = pintfhdl->io_ops._write16_async;
	
	_write16_async(pintfhdl, addr, val);	
	_func_exit_;

}
void write32_async(_adapter *adapter, u32 addr, u32 val)
{
	//struct	io_queue  	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_priv *pio_priv = &adapter->iopriv;
	struct	intf_hdl		*pintfhdl = &(pio_priv->intf);
	void (*_write32_async)(struct intf_hdl *pintfhdl, u32 addr, u32 val);
	_func_enter_;
	_write32_async = pintfhdl->io_ops._write32_async;
	
	_write32_async(pintfhdl, addr, val);	
	_func_exit_;

}
void read_mem(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem)
{
	void (*_read_mem)(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *pmem);	
	//struct	io_queue  	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_priv *pio_priv = &adapter->iopriv;
	struct	intf_hdl		*pintfhdl = &(pio_priv->intf);
	
	_func_enter_;

	if( (adapter->bDriverStopped ==_TRUE) || (adapter->bSurpriseRemoved == _TRUE))
	{
	     RT_TRACE(_module_rtl871x_io_c_, _drv_info_, ("read_mem:bDriverStopped(%d) OR bSurpriseRemoved(%d)", adapter->bDriverStopped, adapter->bSurpriseRemoved));	    
	     return;
	}	
	
	_read_mem = pintfhdl->io_ops._read_mem;
	
	_read_mem(pintfhdl, addr, cnt, pmem);
	
	_func_exit_;
	
}

void write_mem(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem)
{	
	void (*_write_mem)(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *pmem);	
	//struct	io_queue  	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_priv *pio_priv = &adapter->iopriv;
	struct	intf_hdl		*pintfhdl = &(pio_priv->intf);

	_func_enter_;
	
	_write_mem = pintfhdl->io_ops._write_mem;
	
	_write_mem(pintfhdl, addr, cnt, pmem);
	
	_func_exit_;	
	
}

void read_port(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem)
{	
	u32 (*_read_port)(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *pmem);	
	//struct	io_queue  	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_priv *pio_priv = &adapter->iopriv;
	struct	intf_hdl		*pintfhdl = &(pio_priv->intf);
	
	_func_enter_;	

	if( (adapter->bDriverStopped ==_TRUE) || (adapter->bSurpriseRemoved == _TRUE))
	{
	     RT_TRACE(_module_rtl871x_io_c_, _drv_info_, ("read_port:bDriverStopped(%d) OR bSurpriseRemoved(%d)", adapter->bDriverStopped, adapter->bSurpriseRemoved));	    
	     return;
	}	

	_read_port = pintfhdl->io_ops._read_port;
	
	_read_port(pintfhdl, addr, cnt, pmem);
	 
	_func_exit_;

}

void read_port_cancel(_adapter *adapter)
{
	void (*_read_port_cancel)(struct intf_hdl *pintfhdl);
	struct io_priv *pio_priv = &adapter->iopriv;
	struct intf_hdl *pintfhdl = &(pio_priv->intf);
	
	_read_port_cancel = pintfhdl->io_ops._read_port_cancel;

	if(_read_port_cancel)
		_read_port_cancel(pintfhdl);	
			
}

void write_port(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem)
{	
	u32 (*_write_port)(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *pmem);
	//struct	io_queue  	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_priv *pio_priv = &adapter->iopriv;
	struct	intf_hdl		*pintfhdl = &(pio_priv->intf);
	
	_func_enter_;	
	
	_write_port = pintfhdl->io_ops._write_port;
	
	_write_port(pintfhdl, addr, cnt, pmem);
	
	 _func_exit_;
	 
}

void write_port_cancel(_adapter *adapter)
{
	void (*_write_port_cancel)(struct intf_hdl *pintfhdl);
	struct io_priv *pio_priv = &adapter->iopriv;
	struct intf_hdl *pintfhdl = &(pio_priv->intf);
	
	_write_port_cancel = pintfhdl->io_ops._read_port_cancel;

	if(_write_port_cancel)
		_write_port_cancel(pintfhdl);	

}


void attrib_read(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem){
#ifdef CONFIG_SDIO_HCI
	void (*_attrib_read)(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *pmem);
	
	//struct	io_queue  	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_priv *pio_priv = &adapter->iopriv;
	struct	intf_hdl		*pintfhdl = &(pio_priv->intf);
	
	_func_enter_;	
	
	_attrib_read= pintfhdl->io_ops._attrib_read;
	
	_attrib_read(pintfhdl, addr, cnt, pmem);
	
	 _func_exit_;
#endif
}

void attrib_write(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem){
#ifdef CONFIG_SDIO_HCI
	void (*_attrib_write)(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *pmem);
	
	//struct	io_queue  	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_priv *pio_priv = &adapter->iopriv;
	struct	intf_hdl		*pintfhdl = &(pio_priv->intf);
	
	_func_enter_;	
	
	_attrib_write= pintfhdl->io_ops._attrib_write;
	
	_attrib_write(pintfhdl, addr, cnt, pmem);
	
	 _func_exit_;

#endif
}

int init_io_priv(_adapter *padapter)
{	
	void (*set_intf_ops)(struct _io_ops	*pops);
	struct io_priv	*piopriv = &padapter->iopriv;
	struct intf_hdl *pintf = &piopriv->intf;

	piopriv->padapter = padapter;
	pintf->padapter = padapter;
	pintf->pintf_dev = &padapter->dvobjpriv;
	
	
#ifdef CONFIG_SDIO_HCI	
	set_intf_ops = &sdio_set_intf_ops;	
#endif //END OF CONFIG_SDIO_HCI


#ifdef CONFIG_USB_HCI	

	if(padapter->chip_type == RTL8188C_8192C)
	{
#ifdef CONFIG_RTL8192C
		set_intf_ops = &rtl8192cu_set_intf_ops;
#endif
	}
	else if(padapter->chip_type == RTL8192D)
	{
#ifdef CONFIG_RTL8192D
		set_intf_ops = &rtl8192cu_set_intf_ops;
#endif		
	}
	else
	{
		set_intf_ops = NULL;		
	}
#endif //END OF CONFIG_USB_HCI


	if(set_intf_ops==NULL)
		return _FAIL;


	set_intf_ops(&pintf->io_ops);

	return _SUCCESS;

}

