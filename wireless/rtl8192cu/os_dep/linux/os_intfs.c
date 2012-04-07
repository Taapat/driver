/******************************************************************************
* os_intfs.c                                                                                                                                 *
*                                                                                                                                          *
* Description :                      
*		drv_entry
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
#define _OS_INTFS_C_

#include <drv_conf.h>

#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif
 

#include <linux/module.h>
#include <linux/init.h>

#include <osdep_service.h>
#include <drv_types.h>
#include <xmit_osdep.h>
#include <recv_osdep.h>
#include <hal_init.h>
#include <rtw_ioctl.h>

#ifdef CONFIG_SDIO_HCI
#include <sdio_osintf.h>
#include <linux/mmc/sdio_func.h> 
#include <linux/mmc/sdio_ids.h>
#endif

#ifdef CONFIG_USB_HCI
#include <usb_osintf.h>
#endif

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("realtek wireless lan driver");
MODULE_AUTHOR("...");

/* module param defaults */
int chip_version = VERSION_TEST_CHIP_88C;
int rfintfs = HWPI;
int lbkmode = 0;//RTL8712_AIR_TRX;
#ifdef CONFIG_SDIO_HCI
int hci = RTL8192C_SDIO;
#endif
#ifdef CONFIG_USB_HCI
int hci = RTL8192C_USB;
#endif

int network_mode = Ndis802_11IBSS;//Ndis802_11Infrastructure;//infra, ad-hoc, auto	  
//NDIS_802_11_SSID	ssid;
int channel = 1;//ad-hoc support requirement 
int wireless_mode = WIRELESS_11BG;
int vrtl_carrier_sense = AUTO_VCS;
int vcs_type = RTS_CTS;//*
int rts_thresh = 2347;//*
int frag_thresh = 2346;//*
int preamble = PREAMBLE_LONG;//long, short, auto
int scan_mode = 1;//active, passive
int adhoc_tx_pwr = 1;
int soft_ap = 0;
//int smart_ps = 1;  
int power_mgnt = PS_MODE_ACTIVE;
int radio_enable = 1;
int long_retry_lmt = 7;
int short_retry_lmt = 7;
int busy_thresh = 40;
//int qos_enable = 0; //*
int ack_policy = NORMAL_ACK;
int mp_mode = 0;	
int software_encrypt = 0;
int software_decrypt = 0;	  
 
int wmm_enable = 0;// default is set to disable the wmm.
int uapsd_enable = 0;	  
int uapsd_max_sp = NO_LIMIT;
int uapsd_acbk_en = 0;
int uapsd_acbe_en = 0;
int uapsd_acvi_en = 0;
int uapsd_acvo_en = 0;
	
#ifdef CONFIG_80211N_HT
int ht_enable = 1;
int cbw40_enable = 1;
int ampdu_enable = 1;//for enable tx_ampdu
#endif
//int rf_config = RF_1T2R;  // 1T2R	
int rf_config = RF_819X_MAX_TYPE;  //auto
int low_power = 0;
int wifi_spec = 0;//for wifi test
int channel_plan = RT_CHANNEL_DOMAIN_MAX;

#ifdef CONFIG_BT_COEXIST
int bt_iso = 2;// 0:Low, 1:High, 2:From Efuse
int bt_sco = 3;// 0:Idle, 1:None-SCO, 2:SCO, 3:From Counter, 4.Busy, 5.OtherBusy
int bt_ampdu =1 ;// 0:Disable BT control A-MPDU, 1:Enable BT control A-MPDU.
#endif
int AcceptAddbaReq = _TRUE;// 0:Reject AP's Add BA req, 1:Accept AP's Add BA req.

#ifdef CONFIG_ANTENNA_DIVERSITY
int  antdiv_cfg = 0; // 0:OFF , 1:ON, 2:decide by Efuse config
#endif

char* initmac = 0;  // temp mac address if users want to use instead of the mac address in Efuse

module_param(initmac, charp, 0644);
module_param(chip_version, int, 0644);
module_param(rfintfs, int, 0644);
module_param(lbkmode, int, 0644);
module_param(hci, int, 0644);
module_param(network_mode, int, 0644);
module_param(channel, int, 0644);
module_param(mp_mode, int, 0644);
module_param(wmm_enable, int, 0644);
module_param(vrtl_carrier_sense, int, 0644);
module_param(vcs_type, int, 0644);
module_param(busy_thresh, int, 0644);
#ifdef CONFIG_80211N_HT
module_param(ht_enable, int, 0644);
module_param(cbw40_enable, int, 0644);
module_param(ampdu_enable, int, 0644);
#endif
module_param(rf_config, int, 0644);
module_param(power_mgnt, int, 0644);
module_param(low_power, int, 0644);
module_param(wifi_spec, int, 0644);

#ifdef CONFIG_ANTENNA_DIVERSITY
module_param(antdiv_cfg, int, 0644);
#endif

#ifdef CONFIG_R871X_TEST
int start_pseudo_adhoc(_adapter *padapter);
int stop_pseudo_adhoc(_adapter *padapter);
#endif

extern void rtw_dev_unload(_adapter *padapter);


u32 start_drv_threads(_adapter *padapter);
void stop_drv_threads (_adapter *padapter);
u8 init_drv_sw(_adapter *padapter);
u8 free_drv_sw(_adapter *padapter);

struct net_device *init_netdev(void);

static uint loadparam( _adapter *padapter,  _nic_hdl	pnetdev);
static int netdev_open (struct net_device *pnetdev);
static int netdev_close (struct net_device *pnetdev);

uint loadparam( _adapter *padapter,  _nic_hdl	pnetdev)
{
       
	uint status = _SUCCESS;
	struct registry_priv  *registry_par = &padapter->registrypriv;

_func_enter_;

	registry_par->chip_version = (u8)chip_version;
	registry_par->rfintfs = (u8)rfintfs;
	registry_par->lbkmode = (u8)lbkmode;	
	registry_par->hci = (u8)hci;
	registry_par->network_mode  = (u8)network_mode;	

     	_memcpy(registry_par->ssid.Ssid, "ANY", 3);
	registry_par->ssid.SsidLength = 3;
	
	registry_par->channel = (u8)channel;
	registry_par->wireless_mode = (u8)wireless_mode;
	registry_par->vrtl_carrier_sense = (u8)vrtl_carrier_sense ;
	registry_par->vcs_type = (u8)vcs_type;
	registry_par->frag_thresh=(u16)frag_thresh;
	registry_par->preamble = (u8)preamble;
	registry_par->scan_mode = (u8)scan_mode;
	registry_par->adhoc_tx_pwr = (u8)adhoc_tx_pwr;
	registry_par->soft_ap=  (u8)soft_ap;
	//registry_par->smart_ps =  (u8)smart_ps;  
	registry_par->power_mgnt = (u8)power_mgnt;
	registry_par->radio_enable = (u8)radio_enable;
	registry_par->long_retry_lmt = (u8)long_retry_lmt;
	registry_par->short_retry_lmt = (u8)short_retry_lmt;
  	registry_par->busy_thresh = (u16)busy_thresh;
  	//registry_par->qos_enable = (u8)qos_enable;
    	registry_par->ack_policy = (u8)ack_policy;
	registry_par->mp_mode = (u8)mp_mode;	
	registry_par->software_encrypt = (u8)software_encrypt;
	registry_par->software_decrypt = (u8)software_decrypt;	  

	 //UAPSD
	registry_par->wmm_enable = (u8)wmm_enable;
	registry_par->uapsd_enable = (u8)uapsd_enable;	  
	registry_par->uapsd_max_sp = (u8)uapsd_max_sp;
	registry_par->uapsd_acbk_en = (u8)uapsd_acbk_en;
	registry_par->uapsd_acbe_en = (u8)uapsd_acbe_en;
	registry_par->uapsd_acvi_en = (u8)uapsd_acvi_en;
	registry_par->uapsd_acvo_en = (u8)uapsd_acvo_en;

#ifdef CONFIG_80211N_HT
	registry_par->ht_enable = (u8)ht_enable;
	registry_par->cbw40_enable = (u8)cbw40_enable;
	registry_par->ampdu_enable = (u8)ampdu_enable;
#endif

	registry_par->rf_config = (u8)rf_config;
	registry_par->low_power = (u8)low_power;

	
	registry_par->wifi_spec = (u8)wifi_spec;

	registry_par->channel_plan = (u8)channel_plan;

#ifdef CONFIG_BT_COEXIST
	registry_par->bt_iso = (u8)bt_iso;
	registry_par->bt_sco = (u8)bt_sco;
	registry_par->bt_ampdu = (u8)bt_ampdu;
#endif
	registry_par->bAcceptAddbaReq = (u8)AcceptAddbaReq;

#ifdef CONFIG_ANTENNA_DIVERSITY
	registry_par->antdiv_cfg = (u8)antdiv_cfg;
#endif
	
_func_exit_;

	return status;


}

static int rtw_net_set_mac_address(struct net_device *pnetdev, void *p)
{
	_adapter *padapter = (_adapter *)netdev_priv(pnetdev);
	struct sockaddr *addr = p;
	
	if(padapter->bup == _FALSE)
	{
		//printk("r8711_net_set_mac_address(), MAC=%x:%x:%x:%x:%x:%x\n", addr->sa_data[0], addr->sa_data[1], addr->sa_data[2], addr->sa_data[3],
		//addr->sa_data[4], addr->sa_data[5]);
		//_memcpy(padapter->eeprompriv.mac_addr, addr->sa_data, ETH_ALEN);
		_memcpy(pnetdev->dev_addr, addr->sa_data, ETH_ALEN);
		//padapter->bset_hwaddr = _TRUE;
	}

	return 0;
}

static struct net_device_stats *rtw_net_get_stats(struct net_device *pnetdev)
{
	_adapter *padapter = (_adapter *)netdev_priv(pnetdev);
	struct xmit_priv *pxmitpriv = &(padapter->xmitpriv);
	struct recv_priv *precvpriv = &(padapter->recvpriv);

	padapter->stats.tx_packets = pxmitpriv->tx_pkts;//pxmitpriv->tx_pkts++;
	padapter->stats.rx_packets = precvpriv->rx_pkts;//precvpriv->rx_pkts++; 		
	padapter->stats.tx_dropped = pxmitpriv->tx_drop;
	padapter->stats.rx_dropped = precvpriv->rx_drop;
	padapter->stats.tx_bytes = pxmitpriv->tx_bytes;
	padapter->stats.rx_bytes = precvpriv->rx_bytes;
	
	return &padapter->stats;	
}

#if (LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,29))
static const struct net_device_ops rtw_netdev_ops = {
	.ndo_open = netdev_open,
        .ndo_stop = netdev_close,
        .ndo_start_xmit = xmit_entry,
        .ndo_set_mac_address = rtw_net_set_mac_address,
        .ndo_get_stats = rtw_net_get_stats,
        .ndo_do_ioctl = rtw_ioctl,
};
#endif

struct net_device *init_netdev(void)	
{
	_adapter *padapter;
	struct net_device *pnetdev;

	RT_TRACE(_module_os_intfs_c_,_drv_info_,("+init_net_dev\n"));

	//pnetdev = alloc_netdev(sizeof(_adapter), "wlan%d", ether_setup);
	pnetdev = alloc_etherdev(sizeof(_adapter));	
	if (!pnetdev)
	   return NULL;

	//SET_MODULE_OWNER(pnetdev);
       ether_setup(pnetdev);
	
	padapter = netdev_priv(pnetdev);
	padapter->pnetdev = pnetdev;	
	
	//pnetdev->init = NULL;
#if (LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,29))

	printk("register rtl8712_netdev_ops to netdev_ops\n");
	pnetdev->netdev_ops = &rtw_netdev_ops;

#else
	pnetdev->open = netdev_open;
	pnetdev->stop = netdev_close;	
	
	pnetdev->hard_start_xmit = xmit_entry;

	pnetdev->set_mac_address = rtw_net_set_mac_address;
	pnetdev->get_stats = rtw_net_get_stats;

	pnetdev->do_ioctl = rtw_ioctl;

#endif


#ifdef CONFIG_RTL8712_TCP_CSUM_OFFLOAD_TX
	pnetdev->features |= NETIF_F_IP_CSUM;
#endif	
	//pnetdev->tx_timeout = NULL;
	pnetdev->watchdog_timeo = HZ; /* 1 second timeout */	
	
	pnetdev->wireless_handlers = (struct iw_handler_def *)&rtw_handlers_def;  
	
#ifdef WIRELESS_SPY
	//priv->wireless_data.spy_data = &priv->spy_data;
	//pnetdev->wireless_data = &priv->wireless_data;
#endif
	
	if(dev_alloc_name(pnetdev,"wlan%d") < 0)
	{
		RT_TRACE(_module_os_intfs_c_,_drv_err_,("dev_alloc_name, fail! \n"));
	}

	//step 2.
   	loadparam(padapter, pnetdev);	   

	netif_carrier_off(pnetdev);
	//netif_stop_queue(pnetdev);
	
	return pnetdev;

}

u32 start_drv_threads(_adapter *padapter)
{

    u32 _status = _SUCCESS;

    RT_TRACE(_module_os_intfs_c_,_drv_info_,("+start_drv_threads\n"));

#ifdef CONFIG_SDIO_HCI
    padapter->xmitThread = kernel_thread(xmit_thread, padapter, CLONE_FS|CLONE_FILES);
    if(padapter->xmitThread < 0)
		_status = _FAIL;
#endif

#ifdef CONFIG_RECV_THREAD_MODE
    padapter->recvThread = kernel_thread(recv_thread, padapter, CLONE_FS|CLONE_FILES);
    if(padapter->recvThread < 0)
		_status = _FAIL;	
#endif

    padapter->cmdThread = kernel_thread(cmd_thread, padapter, CLONE_FS|CLONE_FILES);
    if(padapter->cmdThread < 0)
		_status = _FAIL;		

#ifdef CONFIG_EVENT_THREAD_MODE
    padapter->evtThread = kernel_thread(event_thread, padapter, CLONE_FS|CLONE_FILES);
    if(padapter->evtThread < 0)
		_status = _FAIL;		
#endif
  
    return _status;

}

void stop_drv_threads (_adapter *padapter)
{
	RT_TRACE(_module_os_intfs_c_,_drv_info_,("+stop_drv_threads\n"));	

	//Below is to termindate cmd_thread & event_thread...
	_up_sema(&padapter->cmdpriv.cmd_queue_sema);
	//_up_sema(&padapter->cmdpriv.cmd_done_sema);
	if(padapter->cmdThread){
		_down_sema(&padapter->cmdpriv.terminate_cmdthread_sema);
	}

#ifdef CONFIG_EVENT_THREAD_MODE
        _up_sema(&padapter->evtpriv.evt_notify);
	if(padapter->evtThread){
		_down_sema(&padapter->evtpriv.terminate_evtthread_sema);
	}
#endif

#ifdef CONFIG_XMIT_THREAD_MODE
	// Below is to termindate tx_thread...
	_up_sema(&padapter->xmitpriv.xmit_sema);	
	_down_sema(&padapter->xmitpriv.terminate_xmitthread_sema);
	RT_TRACE(_module_os_intfs_c_,_drv_info_,("\n drv_halt: xmit_thread can be terminated ! \n"));
#endif
	 
#ifdef CONFIG_RECV_THREAD_MODE	
	// Below is to termindate rx_thread...
	_up_sema(&padapter->recvpriv.recv_sema);
	_down_sema(&padapter->recvpriv.terminate_recvthread_sema);
	RT_TRACE(_module_os_intfs_c_,_drv_info_,("\n drv_halt:recv_thread can be terminated! \n"));
#endif


}

u8 init_default_value(_adapter *padapter)
{
	u8 ret  = _SUCCESS;
	struct registry_priv* pregistrypriv = &padapter->registrypriv;
	struct xmit_priv	*pxmitpriv = &padapter->xmitpriv;
	struct recv_priv	*precvpriv = &padapter->recvpriv;
	struct mlme_priv *pmlmepriv= &padapter->mlmepriv;
	struct security_priv *psecuritypriv = &padapter->securitypriv;

	//xmit_priv
	pxmitpriv->vcs_setting = pregistrypriv->vrtl_carrier_sense;
	pxmitpriv->vcs = pregistrypriv->vcs_type;
	pxmitpriv->vcs_type = pregistrypriv->vcs_type;
	pxmitpriv->rts_thresh = pregistrypriv->rts_thresh;
	pxmitpriv->frag_len = pregistrypriv->frag_thresh;
	
		

	//recv_priv
	

	//mlme_priv
	pmlmepriv->scan_interval = 0;
	
	//qos_priv
	//pmlmepriv->qospriv.qos_option = pregistrypriv->wmm_enable;
	
	//ht_priv
#ifdef CONFIG_80211N_HT		
	{
		int i;
		struct ht_priv	 *phtpriv = &pmlmepriv->htpriv;
		
		phtpriv->ampdu_enable = _FALSE;//set to disabled

		for(i=0; i<16; i++)
		{
			phtpriv->baddbareq_issued[i] = _FALSE;
		}
		
	}	
#endif	

	//security_priv
	//get_encrypt_decrypt_from_registrypriv(padapter);
	psecuritypriv->binstallGrpkey = _FAIL;
	psecuritypriv->sw_encrypt=pregistrypriv->software_encrypt;
	psecuritypriv->sw_decrypt=pregistrypriv->software_decrypt;
	
	psecuritypriv->dot11AuthAlgrthm = dot11AuthAlgrthm_Open; //open system
	psecuritypriv->dot11PrivacyAlgrthm = _NO_PRIVACY_;

	psecuritypriv->dot11PrivacyKeyIndex = 0;

	psecuritypriv->dot118021XGrpPrivacy = _NO_PRIVACY_;
	psecuritypriv->dot118021XGrpKeyid = 1;

	psecuritypriv->ndisauthtype = Ndis802_11AuthModeOpen;
	psecuritypriv->ndisencryptstatus = Ndis802_11WEPDisabled;
	

	//pwrctrl_priv


	//registry_priv
	init_registrypriv_dev_network(padapter);		
	update_registrypriv_dev_network(padapter);


	//hal_priv
	padapter->halpriv.fw_ractrl = _FALSE;
	padapter->halpriv.LastHMEBoxNum = 0;

	//misc.
	
	padapter->bReadPortCancel = _FALSE;	
	padapter->bWritePortCancel = _FALSE;		
	return ret;
}

u8 reset_drv_sw(_adapter *padapter)
{
	u8	ret8=_SUCCESS;	
	struct mlme_priv *pmlmepriv= &padapter->mlmepriv;
	struct pwrctrl_priv *pwrctrlpriv = &padapter->pwrctrlpriv;
	struct sitesurvey_ctrl *psitesurveyctrl=&pmlmepriv->sitesurveyctrl;
	//hal_priv	
	padapter->halpriv.fw_ractrl = _FALSE;	
	padapter->halpriv.LastHMEBoxNum = 0;	//misc.		
	padapter->bReadPortCancel = _FALSE;		
	padapter->bWritePortCancel = _FALSE;	
	pmlmepriv->scan_interval = 0;

	pwrctrlpriv->inactive_pwrstate = rf_on;
	pwrctrlpriv->bips_processing = _FALSE;		

	padapter->xmitpriv.tx_pkts = psitesurveyctrl->last_tx_pkts = 0;
	padapter->recvpriv.rx_pkts = psitesurveyctrl->last_rx_pkts = 0;
	psitesurveyctrl->traffic_busy = _FALSE;	
	return ret8;
}


u8 init_drv_sw(_adapter *padapter)
{

	u8	ret8=_SUCCESS;

_func_enter_;

	RT_TRACE(_module_os_intfs_c_,_drv_info_,("+init_drv_sw\n"));

	if ((init_cmd_priv(&padapter->cmdpriv)) == _FAIL)
	{
		RT_TRACE(_module_os_intfs_c_,_drv_err_,("\n Can't init cmd_priv\n"));
		ret8=_FAIL;
		goto exit;
	}
	
	padapter->cmdpriv.padapter=padapter;
	
	if ((init_evt_priv(&padapter->evtpriv)) == _FAIL)
	{
		RT_TRACE(_module_os_intfs_c_,_drv_err_,("\n Can't init evt_priv\n"));
		ret8=_FAIL;
		goto exit;
	}
	
	
	if (init_mlme_priv(padapter) == _FAIL)
	{
		RT_TRACE(_module_os_intfs_c_,_drv_err_,("\n Can't init mlme_priv\n"));
		ret8=_FAIL;
		goto exit;
	}
		
	if(_init_xmit_priv(&padapter->xmitpriv, padapter) == _FAIL)
	{
		DBG_871X("Can't _init_xmit_priv\n");
		ret8=_FAIL;
		goto exit;
	}
		
	if(_init_recv_priv(&padapter->recvpriv, padapter) == _FAIL)
	{
		DBG_871X("Can't _init_recv_priv\n");
		ret8=_FAIL;
		goto exit;
	}

	_memset((unsigned char *)&padapter->securitypriv, 0, sizeof (struct security_priv));	
	_init_timer(&(padapter->securitypriv.tkip_timer), padapter->pnetdev, use_tkipkey_handler, padapter);

	if(_init_sta_priv(&padapter->stapriv) == _FAIL)
	{
		DBG_871X("Can't _init_sta_priv\n");
		ret8=_FAIL;
		goto exit;
	}
	
	padapter->stapriv.padapter = padapter;	

	init_bcmc_stainfo(padapter);

	init_pwrctrl_priv(padapter);	

	//_memset((u8 *)&padapter->qospriv, 0, sizeof (struct qos_priv));//move to mlme_priv

	//_set_timer(&padapter->mlmepriv.sitesurveyctrl.sitesurvey_ctrl_timer, 5000);	 	

	_set_timer(&padapter->mlmepriv.dynamic_chk_timer, 5000);
	
#ifdef CONFIG_MP_INCLUDED
        mp871xinit(padapter); 
#endif

	ret8 = init_default_value(padapter);	
	init_dm_priv(padapter);

	InitSwLeds(padapter);

exit:
	
	RT_TRACE(_module_os_intfs_c_,_drv_info_,("-init_drv_sw\n"));

	_func_exit_;	
	
	return ret8;
	
}

void cancel_all_timer(_adapter *padapter)
{
	u8 bool;
	_set_timer(&padapter->mlmepriv.assoc_timer, 10000);
	while(1)
	{
		_cancel_timer(&padapter->mlmepriv.assoc_timer, &bool);
		if (bool == _TRUE)
			break;
	}
	RT_TRACE(_module_os_intfs_c_,_drv_info_,("\n drv_halt:cancel association timer complete! \n"));

/*
	while(1)
	{
		_cancel_timer(&padapter->mlmepriv.sitesurveyctrl.sitesurvey_ctrl_timer, &bool);
		if (bool == _TRUE)
			break;
	}
	RT_TRACE(_module_os_intfs_c_,_drv_info_,("\n drv_halt:cancel sitesurvey_ctrl_timer! \n"));
*/
	_set_timer(&padapter->securitypriv.tkip_timer, 10000);
	while(1)
	{
		_cancel_timer(&padapter->securitypriv.tkip_timer, &bool);
		if (bool == _TRUE)
			break;
	}
	RT_TRACE(_module_os_intfs_c_,_drv_info_,("\n drv_halt:cancel tkip_timer! \n"));
	
	
	 _set_timer(&padapter->mlmepriv.scan_to_timer, 10000);
   	 while(1)
   	 {
       	 _cancel_timer(&padapter->mlmepriv.scan_to_timer, &bool);
       	 if (bool == _TRUE)
           	 break;
	}
	
	 _set_timer(&padapter->mlmepriv.dynamic_chk_timer, 10000);
   	 while(1)
   	 {
       	 _cancel_timer(&padapter->mlmepriv.dynamic_chk_timer, &bool);
       	 if (bool == _TRUE)
           	 break;
	}

	DeInitSwLeds(padapter);

#ifdef CONFIG_IPS
	_set_timer(&padapter->pwrctrlpriv.pwr_state_check_timer, 10000);
	 while(1)
   	 {
	       	 _cancel_timer(&padapter->pwrctrlpriv.pwr_state_check_timer, &bool);
	       	 if (bool == _TRUE)
	           	 break;
	}	
#endif	

#ifdef CONFIG_ANTENNA_DIVERSITY

	_set_timer(&padapter->dmpriv.SwAntennaSwitchTimer, 10000);	
	 while(1)
   	 {
	       	 _cancel_timer(&padapter->dmpriv.SwAntennaSwitchTimer, &bool);
	       	 if (bool == _TRUE)
	           	 break;
	}
	
#endif
}

u8 free_drv_sw(_adapter *padapter)
{


	struct net_device *pnetdev = (struct net_device*)padapter->pnetdev;

	RT_TRACE(_module_os_intfs_c_,_drv_info_,("==>free_drv_sw"));	

	free_mlme_ext_priv(&padapter->mlmeextpriv);
	
	free_cmd_priv(&padapter->cmdpriv);
	
	free_evt_priv(&padapter->evtpriv);
	
	free_mlme_priv(&padapter->mlmepriv);
	
	//free_io_queue(padapter);
	
	_free_xmit_priv(&padapter->xmitpriv);
	
	_free_sta_priv(&padapter->stapriv); //will free bcmc_stainfo here
	
	_free_recv_priv(&padapter->recvpriv);	

	//_mfree((void *)padapter, sizeof (padapter));

#ifdef CONFIG_DRVEXT_MODULE
	free_drvext(&padapter->drvextpriv);
#endif	


	RT_TRACE(_module_os_intfs_c_,_drv_info_,("<==free_drv_sw\n"));

	if(pnetdev)
	{
		free_netdev(pnetdev);
	}

	RT_TRACE(_module_os_intfs_c_,_drv_info_,("-free_drv_sw\n"));

	return _SUCCESS;
	
}


static int netdev_open(struct net_device *pnetdev)
{
	uint status;	
	_adapter *padapter = (_adapter *)netdev_priv(pnetdev);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct pwrctrl_priv *pwrctrlpriv = &padapter->pwrctrlpriv;

	RT_TRACE(_module_os_intfs_c_,_drv_info_,("+871x_drv - dev_open\n"));
	printk("+871x_drv - drv_open, bup=%d\n", padapter->bup);

       if(padapter->bup == _FALSE)
    	{    
		padapter->bDriverStopped = _FALSE;
	 	padapter->bSurpriseRemoved = _FALSE;	 
		padapter->bCardDisableWOHSM = _FALSE;        	
	
		status = rtw_hal_init(padapter);		
		if (status ==_FAIL)
		{			
			RT_TRACE(_module_os_intfs_c_,_drv_err_,("rtl871x_hal_init(): Can't init h/w!\n"));
			goto netdev_open_error;
		}

		if ( initmac == NULL )	//	Use the mac address stored in the Efuse
		{
			_memcpy(pnetdev->dev_addr, padapter->eeprompriv.mac_addr, ETH_ALEN);
		}
		else
		{	//	Use the user specifiy mac address.

			//	Commented by Albert 2010/07/19
			//	The "myid" function will get the wifi mac address from eeprompriv structure instead of netdev structure.			
			//	So, we have to overwrite the mac_addr stored in the eeprompriv structure.
			//	In this case, the real mac address won't be used anymore.
			//	So that, the eeprompriv.mac_addr should store the mac which users specify.
			_memcpy( padapter->eeprompriv.mac_addr, pnetdev->dev_addr, ETH_ALEN );
		}		

		printk("MAC Address = %x-%x-%x-%x-%x-%x\n", 
				 pnetdev->dev_addr[0], pnetdev->dev_addr[1], pnetdev->dev_addr[2], pnetdev->dev_addr[3], pnetdev->dev_addr[4], pnetdev->dev_addr[5]);		

		
		status=start_drv_threads(padapter);
		if(status ==_FAIL)
		{			
			RT_TRACE(_module_os_intfs_c_,_drv_err_,("Initialize driver software resource Failed!\n"));			
			goto netdev_open_error;			
		}


		if (init_mlme_ext_priv(padapter) == _FAIL)
		{
			RT_TRACE(_module_os_intfs_c_,_drv_err_,("can't init mlme_ext_priv\n"));
			goto netdev_open_error;
		}

		
#ifdef CONFIG_DRVEXT_MODULE
		init_drvext(padapter);
#endif	   		

#ifdef CONFIG_USB_HCI	
		if(pHalData->hal_ops.inirp_init == NULL)
		{
			RT_TRACE(_module_os_intfs_c_,_drv_err_,("Initialize dvobjpriv.inirp_init error!!!\n"));
			goto netdev_open_error;	
		}
		else
		{	
			pHalData->hal_ops.inirp_init(padapter);
		}			
#endif
                padapter->bup = _TRUE;
	}		
	padapter->net_closed = _FALSE;	
		
#ifdef CONFIG_IPS
	if( pwrctrlpriv->power_mgnt != PS_MODE_ACTIVE )
	{
		//if(pwrctrlpriv->ips_enable){
			padapter->pwrctrlpriv.bips_processing = _FALSE;		
			_set_timer(&padapter->pwrctrlpriv.pwr_state_check_timer, 2000);
		//}
	}
 #endif      	
		
	//netif_carrier_on(pnetdev);//call this func when joinbss_event_callback return success       
 	if(!netif_queue_stopped(pnetdev))
      		netif_start_queue(pnetdev);
	else
		netif_wake_queue(pnetdev);
		
        RT_TRACE(_module_os_intfs_c_,_drv_info_,("-871x_drv - dev_open\n"));
	printk("-871x_drv - drv_open, bup=%d\n", padapter->bup);
		
	 return 0;
	
netdev_open_error:

	padapter->bup = _FALSE;
	
	netif_carrier_off(pnetdev);	
	netif_stop_queue(pnetdev);
	
	RT_TRACE(_module_os_intfs_c_,_drv_err_,("-871x_drv - dev_open, fail!\n"));
	printk("-871x_drv - drv_open fail, bup=%d\n", padapter->bup);
	
	return (-1);
	
}

#ifdef CONFIG_PM
int pm_netdev_open(struct net_device *pnetdev)
{
	return netdev_open(pnetdev);
}
#endif

#ifdef CONFIG_IPS
int  ips_netdrv_open(_adapter *padapter)
{
	int status;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	padapter->net_closed = _FALSE;
	//printk("%s\n",__FUNCTION__);
	//if(padapter->bup == _FALSE)
    	{    
		padapter->bDriverStopped = _FALSE;
	 	padapter->bSurpriseRemoved = _FALSE;	 
		padapter->bCardDisableWOHSM = _FALSE;
		padapter->bup = _TRUE;        	
	
		status = rtw_hal_init(padapter);		
		if (status ==_FAIL)
		{			
			RT_TRACE(_module_os_intfs_c_,_drv_err_,("_r871xu_netdrv_open(): Can't init h/w!\n"));
			goto netdev_open_error;
		}
  		

#ifdef CONFIG_USB_HCI	
		if(pHalData->hal_ops.inirp_init == NULL)
		{
			RT_TRACE(_module_os_intfs_c_,_drv_err_,("Initialize dvobjpriv.inirp_init error!!!\n"));
			goto netdev_open_error;	
		}
		else
		{	
			pHalData->hal_ops.inirp_init(padapter);
		}			
#endif

		_set_timer(&padapter->pwrctrlpriv.pwr_state_check_timer, 2000);
  		_set_timer(&padapter->mlmepriv.dynamic_chk_timer,5000);

	}		
		
        RT_TRACE(_module_os_intfs_c_,_drv_info_,("-871x_drv - dev_open\n"));
	//printk("-ips_netdrv_open - drv_open, bup=%d\n", padapter->bup);
		
	 return _SUCCESS;

netdev_open_error:

	//padapter->bup = _FALSE;
	
	RT_TRACE(_module_os_intfs_c_,_drv_err_,("-871x_drv - dev_open, fail!\n"));
	//printk("-ips_netdrv_open - drv_open fail, bup=%d\n", padapter->bup);
	
	return _FAIL;
}
#endif
static int netdev_close(struct net_device *pnetdev)
{
	_adapter *padapter = (_adapter *)netdev_priv(pnetdev);
		
	RT_TRACE(_module_os_intfs_c_,_drv_info_,("+871x_drv - drv_close\n"));	


	padapter->net_closed = _TRUE;

/*	if(!padapter->hw_init_completed)
	{
		printk("(1)871x_drv - drv_close, bup=%d, hw_init_completed=%d\n", padapter->bup, padapter->hw_init_completed);

	padapter->bDriverStopped = _TRUE;   

	rtw_dev_unload(padapter);	
	}
	else*/
	{
		printk("(2)871x_drv - drv_close, bup=%d, hw_init_completed=%d\n", padapter->bup, padapter->hw_init_completed);

		//s1.
		if(pnetdev)   
     		{
			if (!netif_queue_stopped(pnetdev))
				netif_stop_queue(pnetdev);
     		}
		
		//s2.	
		//s2-1.  issue disassoc_cmd to fw
		disassoc_cmd(padapter);	
		//s2-2.  indicate disconnect to os
		indicate_disconnect(padapter);
		//s2-3. 
	       free_assoc_resources(padapter);	
		//s2-4.
		free_network_queue(padapter);

	}

	// Close LED
	padapter->ledpriv.LedControlHandler(padapter, LED_CTL_POWER_OFF);

	RT_TRACE(_module_os_intfs_c_,_drv_info_,("-871x_drv - drv_close\n"));
	printk("-871x_drv - drv_close, bup=%d\n", padapter->bup);
	   
	return 0;
	
}

