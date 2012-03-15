#ifndef __XMIT_OSDEP_H_
#define __XMIT_OSDEP_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

struct pkt_file {
	_pkt *pkt;
	u32	pkt_len;	 //the remainder length of the open_file
	_buffer *cur_buffer;
	u8 *buf_start;
	u8 *cur_addr;
	u32 buf_len;
};

#ifdef PLATFORM_WINDOWS

#ifdef PLATFORM_OS_XP
#ifdef CONFIG_USB_HCI
#include <usb.h>
#include <usbdlib.h>
#include <usbioctl.h>
#endif
#endif

#define NR_XMITFRAME     128

#define ETH_ALEN	6

extern NDIS_STATUS xmit_entry(
IN _nic_hdl		cnxt,
IN NDIS_PACKET		*pkt,
IN UINT				flags
);

#endif


#ifdef PLATFORM_LINUX

#define NR_XMITFRAME	256

struct xmit_priv;
struct pkt_attrib;
struct sta_xmit_priv;
struct xmit_frame;
struct xmit_buf;

extern int xmit_entry(_pkt *pkt, _nic_hdl pnetdev);

#ifdef CONFIG_USB_HCI
extern void xmit_tasklet(void *priv);
#endif

#endif


int os_xmit_resource_alloc(_adapter *padapter, struct xmit_buf *pxmitbuf);
void os_xmit_resource_free(_adapter *padapter, struct xmit_buf *pxmitbuf);

extern void set_tx_chksum_offload(_pkt *pkt, struct pkt_attrib *pattrib);

extern uint remainder_len(struct pkt_file *pfile);
extern void _open_pktfile(_pkt *pkt, struct pkt_file *pfile);
extern uint _pktfile_read (struct pkt_file *pfile, u8 *rmem, uint rlen);
extern sint endofpktfile (struct pkt_file *pfile);

extern void os_xmit_complete(_adapter *padapter, struct xmit_frame *pxframe);

#endif //

