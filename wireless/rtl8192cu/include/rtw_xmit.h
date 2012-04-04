#ifndef _RTL871X_XMIT_H_
#define _RTL871X_XMIT_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <xmit_osdep.h>

#ifdef CONFIG_SDIO_HCI
#define MAX_XMITBUF_SZ (30720)//	(2048)
#define NR_XMITBUFF	(16)
#else //USB
#if USB_TX_AGGREGATION_92C
//#define MAX_XMITBUF_SZ	16384	// 32k
#define MAX_XMITBUF_SZ	20480	// 20k
#else
#define MAX_XMITBUF_SZ	(2048)
#endif
#define NR_XMITBUFF	(4)
#endif

#ifdef PLATFORM_OS_CE
#define XMITBUF_ALIGN_SZ 4
#else
#define XMITBUF_ALIGN_SZ 512
#endif

#define MAX_NUMBLKS		(1)


#define WEP_IV(pattrib_iv, dot11txpn, keyidx)\
do{\
	pattrib_iv[0] = dot11txpn._byte_.TSC0;\
	pattrib_iv[1] = dot11txpn._byte_.TSC1;\
	pattrib_iv[2] = dot11txpn._byte_.TSC2;\
	pattrib_iv[3] = ((keyidx & 0x3)<<6);\
	dot11txpn.val = (dot11txpn.val == 0xffffff) ? 0: (dot11txpn.val+1);\
}while(0)


#define TKIP_IV(pattrib_iv, dot11txpn, keyidx)\
do{\
	pattrib_iv[0] = dot11txpn._byte_.TSC1;\
	pattrib_iv[1] = (dot11txpn._byte_.TSC1 | 0x20) & 0x7f;\
	pattrib_iv[2] = dot11txpn._byte_.TSC0;\
	pattrib_iv[3] = BIT(5) | ((keyidx & 0x3)<<6);\
	pattrib_iv[4] = dot11txpn._byte_.TSC2;\
	pattrib_iv[5] = dot11txpn._byte_.TSC3;\
	pattrib_iv[6] = dot11txpn._byte_.TSC4;\
	pattrib_iv[7] = dot11txpn._byte_.TSC5;\
	dot11txpn.val = dot11txpn.val == 0xffffffffffffULL ? 0: (dot11txpn.val+1);\
}while(0)

#define AES_IV(pattrib_iv, dot11txpn, keyidx)\
do{\
	pattrib_iv[0] = dot11txpn._byte_.TSC0;\
	pattrib_iv[1] = dot11txpn._byte_.TSC1;\
	pattrib_iv[2] = 0;\
	pattrib_iv[3] = BIT(5) | ((keyidx & 0x3)<<6);\
	pattrib_iv[4] = dot11txpn._byte_.TSC2;\
	pattrib_iv[5] = dot11txpn._byte_.TSC3;\
	pattrib_iv[6] = dot11txpn._byte_.TSC4;\
	pattrib_iv[7] = dot11txpn._byte_.TSC5;\
	dot11txpn.val = dot11txpn.val == 0xffffffffffffULL ? 0: (dot11txpn.val+1);\
}while(0)



struct	hw_xmit	{
	_lock xmit_lock;
	_list	pending;	
	_queue *sta_queue;
	struct hw_txqueue *phwtxqueue;
	sint	txcmdcnt;		
	int	accnt;		
};

struct pkt_attrib
{	
	u8	type;
	u8   subtype;
	u8	bswenc;
	u8   dhcp_pkt;
	u16	ether_type;	
	int	pktlen;		//the original 802.3 pkt raw_data len (not include ether_hdr data)
	int	pkt_hdrlen;	//the original 802.3 pkt header len
	int	hdrlen;		//the WLAN Header Len	
	int	nr_frags;
	int	last_txcmdsz;
	int	encrypt;	//when 0 indicate no encrypt. when non-zero, indicate the encrypt algorith
	unsigned char iv[8];
	int	iv_len;
	unsigned char icv[8];	
	int	icv_len;
	int	priority;
	int	ack_policy;
	int	mac_id;
	int	vcs_mode;	//virtual carrier sense method	
	
	u8 	dst[ETH_ALEN];
	u8	src[ETH_ALEN];
	u8	ta[ETH_ALEN];
	u8 	ra[ETH_ALEN];

	u8 key_idx;

	u8 qos_en;
	u8 ht_en;	
	u8 raid;//rate adpative id
	u8 bwmode;
	u8 ch_offset;//PRIME_CHNL_OFFSET
	u8 sgi;//short GI

	u8  pctrl;//per packet txdesc control enable
	
	u32 qsel;
	u16 seqnum;

	struct sta_info * psta;
#ifdef CONFIG_RTL8712_TCP_CSUM_OFFLOAD_TX
	u8 hw_tcp_csum;
#endif	
};


#define WLANHDR_OFFSET	64

#define NULL_FRAMETAG		(0x0)
#define DATA_FRAMETAG		0x01
#define L2_FRAMETAG		0x02
#define MGNT_FRAMETAG		0x03
#define AMSDU_FRAMETAG	0x04

#define EII_FRAMETAG		0x05
#define IEEE8023_FRAMETAG  0x06

#define MP_FRAMETAG		0x07


#define TXAGG_FRAMETAG 	0x08


struct xmit_buf
{
	_list	list;
		
	_adapter *padapter;

	u8 *pallocated_buf;
	
       u8 *pbuf;

	void *priv_data;

#ifdef CONFIG_USB_HCI	
       
       u32 sz[8];	   

#if defined(PLATFORM_OS_XP)||defined(PLATFORM_LINUX)
	PURB	pxmit_urb[8];
#endif

#ifdef PLATFORM_OS_XP
	PIRP		pxmit_irp[8];
#endif
	u8 bpending[8];
	
	sint last[8];
 
#endif

#ifdef CONFIG_SDIO_HCI
	u32  len;	
	u8 *phead;
	u8 *pdata;
	u8 *ptail;
	u8 *pend;
	u32 ff_hwaddr;
#ifdef PLATFORM_OS_XP
	PMDL pxmitbuf_mdl;
	PIRP  pxmitbuf_irp; 
	PSDBUS_REQUEST_PACKET pxmitbuf_sdrp;
#endif	
#endif
	

};

struct xmit_frame
{
	_list	list;

	struct pkt_attrib attrib;
	
	_pkt *pkt;
	
	int frame_tag;
	
	 _adapter *padapter;

	 u8 *buf_addr;

	 struct xmit_buf *pxmitbuf;


#ifdef CONFIG_SDIO_HCI
	u8 pg_num;
#endif

#if USB_TX_AGGREGATION_92C
	u8 agg_num;
	u8 pkt_offset;
#endif
};

struct tx_servq {
	_list	tx_pending;
	_queue	sta_pending;	
	int qcnt;		
};



struct sta_xmit_priv
{
	_lock	lock;
	sint	option;
	sint	apsd_setting;	//When bit mask is on, the associated edca queue supports APSD.


	//struct tx_servq blk_q[MAX_NUMBLKS];
	struct tx_servq	be_q;			//priority == 0,3 
	struct tx_servq	bk_q;			//priority == 1,2
	struct tx_servq	vi_q;			//priority == 4,5
	struct tx_servq	vo_q;			//priority == 6,7
	_list 	legacy_dz;
	_list  apsd;

	u16 txseq_tid[16];

	//uint	sta_tx_bytes;
	//u64	sta_tx_pkts;
	//uint	sta_tx_fail;

};


struct	hw_txqueue	{
	volatile sint	head;
	volatile sint	tail;
	volatile sint 	free_sz;	//in units of 64 bytes	
	volatile sint      free_cmdsz;
	volatile sint	 txsz[8];
	uint	ff_hwaddr;
	uint	cmd_hwaddr;
	sint	ac_tag;
};


struct	xmit_priv	{
	
	_lock	lock;

	_sema	xmit_sema;
	_sema	terminate_xmitthread_sema;
	
	//_queue	blk_strms[MAX_NUMBLKS];
	_queue	be_pending;
	_queue	bk_pending;
	_queue	vi_pending;
	_queue	vo_pending;
	_queue	bm_pending;
	
	//_queue	legacy_dz_queue;
	//_queue	apsd_queue;
	
	u8 *pallocated_frame_buf;
	u8 *pxmit_frame_buf;
	uint free_xmitframe_cnt;

	//uint mapping_addr;
	//uint pkt_sz;	
	
	_queue	free_xmit_queue;	

	struct	hw_txqueue	be_txqueue;
	struct	hw_txqueue	bk_txqueue;
	struct	hw_txqueue	vi_txqueue;
	struct	hw_txqueue	vo_txqueue;
	struct	hw_txqueue	bmc_txqueue;

	uint	frag_len;

	_adapter	*adapter;
	
	u8   vcs_setting;
	u8	vcs;
	u8	vcs_type;
	u16  rts_thresh;
	
	u32	NumTxOkInPeriod;
	u64	tx_bytes;
	u64	tx_pkts;
	u64	tx_drop;
	
	struct hw_xmit *hwxmits;
	u8	hwxmit_entry;

#ifdef CONFIG_USB_HCI
	_sema	tx_retevt;//all tx return event;
	u8		txirp_cnt;//
	
#ifdef PLATFORM_OS_CE
	USB_TRANSFER	usb_transfer_write_port;
//	USB_TRANSFER	usb_transfer_write_mem;
#endif
#ifdef PLATFORM_LINUX
	struct tasklet_struct xmit_tasklet;
#endif
	//per AC pending irp
	int beq_cnt;
	int bkq_cnt;
	int viq_cnt;
	int voq_cnt;
	
#endif

#ifdef CONFIG_SDIO_HCI
	u8 free_pg[8];
	u8	public_pgsz;
	u8	required_pgsz;
	u8	used_pgsz;
	u8	init_pgsz;
#ifdef PLATFORM_OS_XP
	PMDL prd_freesz_mdl[2];
	u8 brd_freesz_pending[2];
	PIRP  prd_freesz_irp[2]; 
	PSDBUS_REQUEST_PACKET prd_freesz_sdrp[2];
	u8 rd_freesz_irp_idx;
#endif

#endif

	_queue free_xmitbuf_queue;
	_queue pending_xmitbuf_queue;
	u8 *pallocated_xmitbuf;
	u8 *pxmitbuf;
	uint free_xmitbuf_cnt;	

};


extern s32 free_xmitbuf(struct xmit_priv *pxmitpriv, struct xmit_buf *pxmitbuf);
extern struct xmit_buf *alloc_xmitbuf(struct xmit_priv *pxmitpriv);

extern void update_protection(_adapter *padapter, u8 *ie, uint ie_len);
extern struct xmit_frame *alloc_xmitframe(struct xmit_priv *pxmitpriv);
extern s32 make_wlanhdr (_adapter *padapter, unsigned char *hdr, struct pkt_attrib *pattrib);
extern s32 rtw_put_snap(u8 *data, u16 h_proto);
extern s32 free_xmitframe(struct xmit_priv *pxmitpriv, struct xmit_frame *pxmitframe);
extern void free_xmitframe_queue(struct xmit_priv *pxmitpriv, _queue *pframequeue );
extern s32 xmit_classifier(_adapter *padapter, struct xmit_frame *pxmitframe);
extern thread_return xmit_thread(thread_context context);
extern s32 xmitframe_coalesce(_adapter *padapter, _pkt *pkt, struct xmit_frame *pxmitframe);

s32 _init_hw_txqueue(struct hw_txqueue* phw_txqueue, u8 ac_tag);
void	_init_sta_xmit_priv(struct sta_xmit_priv *psta_xmitpriv);


s32 txframes_pending(_adapter *padapter);
s32 txframes_sta_ac_pending(_adapter *padapter, struct pkt_attrib *pattrib);
void init_hwxmits(struct hw_xmit *phwxmit, sint entry);


s32 _init_xmit_priv(struct xmit_priv *pxmitpriv, _adapter *padapter);
void _free_xmit_priv (struct xmit_priv *pxmitpriv);


void alloc_hwxmits(_adapter *padapter);
void free_hwxmits(_adapter *padapter);

s32 free_xmitframe_ex(struct xmit_priv *pxmitpriv, struct xmit_frame *pxmitframe);

s32 rtw_xmit(_adapter *padapter, _pkt *pkt);

#ifdef CONFIG_RTL8712
#include "rtl8712_xmit.h"
#endif

#ifdef CONFIG_RTL8192C
#include "rtl8192c_xmit.h"
#endif

#endif	//_RTL871X_XMIT_H_

