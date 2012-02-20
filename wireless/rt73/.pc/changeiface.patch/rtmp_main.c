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
    rtmp_main.c

    Abstract:
    main initialization routines

    Revision History:
    Who         When            What
    --------    ----------      ----------------------------------------------
    Name        Date            Modification logs
    Jan Lee		01-10-2005	    modified
*/

#include "rt_config.h"

ULONG	RTDebugLevel = RT_DEBUG_TRACE;

char *hostname = "";		   // default CMPC
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,12)
MODULE_PARM (hostname, "s");
#else
module_param (hostname, charp, 0);
#endif
MODULE_PARM_DESC (hostname, "Host Name");


// Following information will be show when you run 'modinfo'
// *** If you have a solution for the bug in current version of driver, please mail to me.
// Otherwise post to forum in ralinktech's web site(www.ralinktech.com) and let all users help you. ***
MODULE_AUTHOR("Paul Lin <paul_lin@ralinktech.com>");
MODULE_DESCRIPTION("RT73 Wireless Lan Linux Driver");

// *** open source release
MODULE_LICENSE("GPL");

/* Kernel thread and vars, which handles packets that are completed. Only
 * packets that have a "complete" function are sent here. This way, the
 * completion is run out of kernel context, and doesn't block the rest of
 * the stack. */
static int mlme_kill;
static int RTUSBCmd_kill;
//static dma_addr_t  dma_adapter;

extern	const struct iw_handler_def rt73_iw_handler_def;


/* module table */
struct usb_device_id    rtusb_usb_id[] = RT73_USB_DEVICES;
INT const               rtusb_usb_id_len = sizeof(rtusb_usb_id) / sizeof(struct usb_device_id);   
MODULE_DEVICE_TABLE(usb, rtusb_usb_id);


#ifndef PF_NOFREEZE
#define PF_NOFREEZE  0
#endif

static int usb_rtusb_init_device(
	struct net_device *net_dev);

static int usb_rtusb_close_device(
	IN PRTMP_ADAPTER pAd);

/**************************************************************************/
/**************************************************************************/
//tested for kernel 2.4 series
/**************************************************************************/
/**************************************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)


static void usb_rtusb_disconnect(struct usb_device *dev, void *ptr);
static void *usb_rtusb_probe(struct usb_device *dev, UINT interface,
				const struct usb_device_id *id_table);

struct usb_driver rtusb_driver = {
		name:"rt73",
		probe:usb_rtusb_probe,
		disconnect:usb_rtusb_disconnect,
		id_table:rtusb_usb_id,
	};

#else
/**************************************************************************/
/**************************************************************************/
//tested for kernel 2.6series
/**************************************************************************/
/**************************************************************************/
static int usb_rtusb_probe (struct usb_interface *intf,
					  const struct usb_device_id *id);

static void usb_rtusb_disconnect(struct usb_interface *intf);

#ifdef CONFIG_PM
static int rt73_suspend(struct usb_interface *intf, pm_message_t state);
static int rt73_resume(struct usb_interface *intf);
#endif

struct usb_driver rtusb_driver = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15)
	.owner = THIS_MODULE,
#endif
	.name="rt73",
	.probe=usb_rtusb_probe,
	.disconnect=usb_rtusb_disconnect,
	.id_table=rtusb_usb_id,
#ifdef CONFIG_PM
	.suspend = rt73_suspend,
	.resume = rt73_resume,
#endif
	};


/**************************
	Add for ethtool support. HAL/NetworkManager will use it.
***************************/
#define CSR_REG_BASE                    MAC_CSR0
#define CSR_REG_SIZE                    0x04b0
#define EEPROM_BASE                     0x0000
#define EEPROM_SIZE                     0x0100

#ifdef CONFIG_PM
static int 
rt73_suspend(struct usb_interface *intf, pm_message_t state)
{
	PRTMP_ADAPTER       pAd = (PRTMP_ADAPTER)NULL;
	
	DBGPRINT(RT_DEBUG_TRACE,"---> rt73_suspend()\n");
	
	pAd = usb_get_intfdata(intf);

	// need to send it first before USB go susped.
	// without it system unable to reume back.
	RTUSB_VendorRequest(pAd,
                                0,
                                DEVICE_VENDOR_REQUEST_OUT,
                                0x0C,
                                0x0,
                                0x0,
                                NULL,
                                0);
 

	RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS);
	MlmeRadioOff(pAd);
	
	DBGPRINT(RT_DEBUG_TRACE,"<--- rt73_suspend()\n");
	return 0;
}

static int
rt73_resume(struct usb_interface *intf)
{
	PRTMP_ADAPTER       pAd = (PRTMP_ADAPTER)NULL;
	
	DBGPRINT(RT_DEBUG_TRACE,"---> rt73_resume()\n");
	
	pAd = usb_get_intfdata(intf);

	MlmeRadioOn(pAd);
	RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS);

	DBGPRINT(RT_DEBUG_TRACE,"<--- rt73_resume()\n");

	return 0;
}
#endif

static void
rt73_get_drvinfo(struct net_device *net_dev,
        struct ethtool_drvinfo *drvinfo)
{
        PRTMP_ADAPTER   pAd = net_dev->priv;

        strcpy(drvinfo->driver, NIC_DEVICE_NAME);
        strcpy(drvinfo->version, DRIVER_VERSION);
		
		snprintf(drvinfo->fw_version, sizeof(drvinfo->fw_version), "%d(%d)", pAd->FirmwareVersion, pAd->EepromVersion);
		usb_make_path(pAd->pUsb_Dev, drvinfo->bus_info, sizeof(drvinfo->bus_info));
}

static int
rt73_get_regs_len(struct net_device *net_dev)
{
        return CSR_REG_SIZE;
}

static void
rt73_get_regs(struct net_device *net_dev,
        struct ethtool_regs *regs, void *data)
{
	PRTMP_ADAPTER   pAd = net_dev->priv;
	unsigned int counter;

	regs->len = CSR_REG_SIZE;

	for (counter = 0; counter < CSR_REG_SIZE; counter += sizeof(UINT32)) {
		RTUSBReadMACRegister(pAd, CSR_REG_BASE + counter, (PULONG)data);
		data += sizeof(UINT32);
	}
}


static uint32 rt73_ethtool_get_link(struct net_device *dev)
{
	RTMP_ADAPTER *pAd;

	ASSERT((dev));
	pAd = (PRTMP_ADAPTER) dev->priv;

	//We return true if we already associated to some AP.
	return(OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED));
}

static int
rt73_get_eeprom_len(struct net_device *net_dev)
{
        return EEPROM_SIZE;
}

static int
rt73_get_eeprom(struct net_device *net_dev,
        struct ethtool_eeprom *eeprom, u8 *data)
{
        PRTMP_ADAPTER   pAd = net_dev->priv;
        unsigned int counter;

        for (counter = eeprom->offset; counter < eeprom->len; counter += sizeof(USHORT)) {
                USHORT value = 0;
				RTUSBReadEEPROM(pAd, CSR_REG_BASE + counter, (PUCHAR)&value, sizeof(USHORT));
                memcpy(data, &value, sizeof(USHORT));
                data += sizeof(USHORT);
        }

        return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
struct ethtool_ops rt73_ethtool_ops = {
#else
static const struct ethtool_ops rt73_ethtool_ops = {
#endif
        .get_drvinfo    = rt73_get_drvinfo,
        .get_regs_len   = rt73_get_regs_len,
        .get_regs       = rt73_get_regs,
        .get_link       = rt73_ethtool_get_link,
        .get_eeprom_len = rt73_get_eeprom_len,
        .get_eeprom     = rt73_get_eeprom,
};

#endif


struct net_device_stats *rt73_get_ether_stats(
    IN  struct net_device *net_dev)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER) net_dev->priv;

	DBGPRINT(RT_DEBUG_INFO, "rt73_get_ether_stats --->\n");

	pAd->stats.rx_packets = pAd->WlanCounters.ReceivedFragmentCount.vv.LowPart;        // total packets received
	pAd->stats.tx_packets = pAd->WlanCounters.TransmittedFragmentCount.vv.LowPart;     // total packets transmitted

	pAd->stats.rx_bytes= pAd->RalinkCounters.ReceivedByteCount;             // total bytes received
	pAd->stats.tx_bytes = pAd->RalinkCounters.TransmittedByteCount;         // total bytes transmitted

	pAd->stats.rx_errors = pAd->Counters8023.RxErrors;                      // bad packets received
	pAd->stats.tx_errors = pAd->Counters8023.TxErrors;                      // packet transmit problems

	pAd->stats.rx_dropped = pAd->Counters8023.RxNoBuffer;                   // no space in linux buffers
	pAd->stats.tx_dropped = pAd->WlanCounters.FailedCount.vv.LowPart;                  // no space available in linux

	pAd->stats.multicast = pAd->WlanCounters.MulticastReceivedFrameCount.vv.LowPart;   // multicast packets received
	pAd->stats.collisions = pAd->Counters8023.OneCollision + pAd->Counters8023.MoreCollisions;  // Collision packets

	pAd->stats.rx_length_errors = 0;
	pAd->stats.rx_over_errors = pAd->Counters8023.RxNoBuffer;               // receiver ring buff overflow
	pAd->stats.rx_crc_errors = 0;//pAd->WlanCounters.FCSErrorCount;         // recved pkt with crc error
	pAd->stats.rx_frame_errors = pAd->Counters8023.RcvAlignmentErrors;      // recv'd frame alignment error
	pAd->stats.rx_fifo_errors = pAd->Counters8023.RxNoBuffer;               // recv'r fifo overrun
	pAd->stats.rx_missed_errors = 0;                                        // receiver missed packet

	// detailed tx_errors
	pAd->stats.tx_aborted_errors = 0;
	pAd->stats.tx_carrier_errors = 0;
	pAd->stats.tx_fifo_errors = 0;
	pAd->stats.tx_heartbeat_errors = 0;
	pAd->stats.tx_window_errors = 0;

	// for cslip etc
	pAd->stats.rx_compressed = 0;
	pAd->stats.tx_compressed = 0;
   
	return &pAd->stats;
}
#if (WIRELESS_EXT >= 12)
/*
	========================================================================

	Routine Description:
		get wireless statistics

	Arguments:
		net_dev                     Pointer to net_device

	Return Value:
		struct iw_statistics

	Note:
		This function will be called when query /proc

	========================================================================
*/
long rt_abs(long arg)	{	return (arg<0)? -arg : arg;}
struct iw_statistics *rt73_get_wireless_stats(
	IN  struct net_device *net_dev)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER) net_dev->priv;

	DBGPRINT(RT_DEBUG_TRACE, "rt73_get_wireless_stats --->\n");

	// TODO: All elements are zero before be implemented

	pAd->iw_stats.status = 0;   // Status - device dependent for now

	pAd->iw_stats.qual.qual = pAd->Mlme.ChannelQuality; // link quality (%retries, SNR, %missed beacons or better...)
#ifdef RTMP_EMBEDDED
	pAd->iw_stats.qual.level = rt_abs(pAd->PortCfg.LastRssi);   // signal level (dBm)
#else
	pAd->iw_stats.qual.level = abs(pAd->PortCfg.LastRssi);      // signal level (dBm)
#endif
	pAd->iw_stats.qual.level += 256 - pAd->BbpRssiToDbmDelta;

	pAd->iw_stats.qual.noise = (pAd->BbpWriteLatch[17] > pAd->BbpTuning.R17UpperBoundG) ? pAd->BbpTuning.R17UpperBoundG : ((ULONG) pAd->BbpWriteLatch[17]); // noise level (dBm)
	pAd->iw_stats.qual.noise += 256 - 143;
	pAd->iw_stats.qual.updated = 1;     // Flags to know if updated

	pAd->iw_stats.discard.nwid = 0;     // Rx : Wrong nwid/essid
	pAd->iw_stats.miss.beacon = 0;      // Missed beacons/superframe

	// pAd->iw_stats.discard.code, discard.fragment, discard.retries, discard.misc has counted in other place

	return &pAd->iw_stats;
}
#endif

VOID RTUSBHalt(
	IN	PRTMP_ADAPTER	pAd, 
	IN  BOOLEAN         IsFree)
{
	BOOLEAN					 TimerCancelled;
	
	DBGPRINT(RT_DEBUG_TRACE, "====> RTUSBHalt\n");

	//
	// before set flag fRTMP_ADAPTER_HALT_IN_PROGRESS, 
	// we should send a disassoc frame to our AP.
	//

	RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS);

	RTUSBCleanUpMLMEWaitQueue(pAd);
	RTUSBCleanUpMLMEBulkOutQueue(pAd);

	RTMPCancelTimer(&pAd->PortCfg.QuickResponeForRateUpTimer,&TimerCancelled);
	RTMPCancelTimer(&pAd->RxAnt.RxAntDiversityTimer,&TimerCancelled);

	// Free MLME stuff
	MlmeHalt(pAd);

	// Sleep 50 milliseconds so pending io might finish normally
	RTMPusecDelay(50000);

	// We want to wait until all pending receives and sends to the
	// device object. We cancel any
	// irps. Wait until sends and receives have stopped.
	//
	RTUSBCancelPendingIRPs(pAd);

	// Free the entire adapter object
	ReleaseAdapter(pAd, IsFree, FALSE);
  		
	RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS);
}

static int usb_rtusb_close_device(
	IN	PRTMP_ADAPTER	pAd)
{
	int i = 0;
	int ret;
	
	DECLARE_WAIT_QUEUE_HEAD (unlink_wakeup); 
	DECLARE_WAITQUEUE (wait, current);

	DBGPRINT(RT_DEBUG_TRACE, "-->usb_rtusb_close_device \n");

	// ensure there are no more active urbs. 
	add_wait_queue (&unlink_wakeup, &wait);
	pAd->wait = &unlink_wakeup;

	// maybe wait for deletions to finish.
	while ((i < 25) && atomic_read(&pAd->PendingRx) > 0)
	{
#if LINUX_VERSION_CODE >KERNEL_VERSION(2,6,9)
		msleep(UNLINK_TIMEOUT_MS);
#endif
		i++;
	}

	pAd->wait = NULL;
	remove_wait_queue (&unlink_wakeup, &wait); 

	if (pAd->MLMEThr_pid >= 0) 
	{
		mlme_kill = 1;
		RTUSBMlmeUp(pAd);
		wmb(); // need to check
		ret = kill_proc (pAd->MLMEThr_pid, SIGTERM, 1);
		if (ret) 
		{
			printk (KERN_WARNING "%s: unable to Mlme thread, pid=%d\n", pAd->net_dev->name, pAd->MLMEThr_pid);
		}
		wait_for_completion (&pAd->MlmeThreadNotify);
		// reset mlme thread
		pAd->MLMEThr_pid = -1;
	}

	if (pAd->RTUSBCmdThr_pid>= 0) 
	{
		RTUSBCmd_kill = 1;
		RTUSBCMDUp(pAd);
		wmb(); // need to check
		ret = kill_proc (pAd->RTUSBCmdThr_pid, SIGTERM, 1);
		if (ret) 
		{
			printk (KERN_WARNING "%s: unable to RTUSBCmd thread, pid=%d\n", pAd->net_dev->name, pAd->RTUSBCmdThr_pid);
		}
		wait_for_completion (&pAd->CmdThreadNotify);
		// reset cmd thread
		pAd->RTUSBCmdThr_pid= -1;
	}

	RTUSBHalt(pAd, TRUE);
	RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_REMOVE_IN_PROGRESS);
	tasklet_kill(&pAd->rx_bh);



	DBGPRINT(RT_DEBUG_TRACE,"<--usb_rtusb_close_device \n");

	return 0;
}

static int usb_rtusb_open(struct net_device *net_dev)
{
	PRTMP_ADAPTER   pAd = (PRTMP_ADAPTER) net_dev->priv;
	UCHAR	TmpPhy;
	NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
//	if (!pAd->ProbeFinish)
//		return -1;

	DBGPRINT(RT_DEBUG_TRACE, "%s-->\n", __FUNCTION__);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	if (!try_module_get(THIS_MODULE))
	{
		DBGPRINT(RT_DEBUG_ERROR, "%s: cannot reserve module\n", __FUNCTION__);
		return -1;
	}
#else
	MOD_INC_USE_COUNT;
#endif
//iverson
        PortCfgInit(pAd);

	// Wait for hardware stable
	{
		ULONG MacCsr0 = 0, Index = 0;
		
		do
		{
			Status = RTUSBReadMACRegister(pAd, MAC_CSR0, &MacCsr0);

			if (MacCsr0 != 0)
				break;
			
			RTMPusecDelay(1000);
		} while (Index++ < 1000);
		DBGPRINT(RT_DEBUG_TRACE, "Init: MAC_CSR0=0x%08x, Status=0x%08x\n", MacCsr0, Status);
	}

	// Load 8051 firmware
	// Read RaConfig profile parameters 
#ifdef  READ_PROFILE_FROM_FILE 
 	RTMPReadParametersFromFile(pAd);
#endif
 
 //
 // Read additional info from NIC such as MAC address
 // This function must called after register CSR base address
 //
#ifdef INIT_FROM_EEPROM
	 NICReadEEPROMParameters(pAd);
	 NICInitAsicFromEEPROM(pAd);
#endif
 	RTUSBWriteHWMACAddress(pAd);
 
	 // external LNA has different R17 base
	 if (pAd->NicConfig2.field.ExternalLNA)
	 {
	  pAd->BbpTuning.R17LowerBoundA += 0x10;
	  pAd->BbpTuning.R17UpperBoundA += 0x10;
	  pAd->BbpTuning.R17LowerBoundG += 0x10;
	  pAd->BbpTuning.R17UpperBoundG += 0x10;
	 }
 
	 // hardware initialization after all parameters are acquired from
	 // Registry or E2PROM
#if 1
	 TmpPhy = pAd->PortCfg.PhyMode;
//	pAd->PortCfg.PhyMode = 0xff;
	 RTMPSetPhyMode(pAd, TmpPhy);
#endif
	 // max desired rate might be reset as call phymode setup, so set TxRate again
	 if (pAd->PortCfg.DefaultMaxDesiredRate == 0)
	  RTMPSetDesiredRates(pAd, -1);
	 else
	  RTMPSetDesiredRates(pAd, (LONG) (RateIdToMbps[pAd->PortCfg.DefaultMaxDesiredRate - 1] * 1000000));

	printk("=> usb_rtusb_open\n");
	if (VIRTUAL_IF_NUM(pAd) == 0)
	{
		RTUSBWriteMACRegister(pAd, MAC_CSR10, 0x0718);

		// Clear Reset Flag before starting receiving/transmitting
		RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS);
		RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_REMOVE_IN_PROGRESS);

		if (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF))
		{
			RTUSBBulkReceive(pAd);
			RTUSBWriteMACRegister(pAd, TXRX_CSR0, 0x025eb032);    // enable RX of MAC block, Staion not drop control frame
		}
		AsicSwitchChannel(pAd, pAd->PortCfg.Channel);
		AsicLockChannel(pAd, pAd->PortCfg.Channel);
	}
	VIRTUAL_IF_INC(pAd);

	netif_start_queue(net_dev);
	netif_carrier_on(net_dev);
	netif_wake_queue(net_dev);

	DBGPRINT(RT_DEBUG_TRACE, "%s<--\n", __FUNCTION__);

	return 0;
}

static int usb_rtusb_close(struct net_device *net_dev)
{
	PRTMP_ADAPTER   pAd = (PRTMP_ADAPTER) net_dev->priv;
	int	            i = 0;
	MLME_DISASSOC_REQ_STRUCT DisReq;
	MLME_QUEUE_ELEM          MsgElem;
	wait_queue_head_t wait;

//	if (!pAd->ProbeFinish)
//		return -1;

	init_waitqueue_head(&wait);

	DBGPRINT(RT_DEBUG_TRACE, "%s-->\n", __FUNCTION__);

	netif_carrier_off(pAd->net_dev);
	netif_stop_queue(pAd->net_dev);

	if (INFRA_ON(pAd))
	{
		COPY_MAC_ADDR(DisReq.Addr, pAd->PortCfg.Bssid);
		DisReq.Reason =  REASON_DISASSOC_STA_LEAVING;

		MsgElem.Machine = ASSOC_STATE_MACHINE;
		MsgElem.MsgType = MT2_MLME_DISASSOC_REQ;
		MsgElem.MsgLen = sizeof(MLME_DISASSOC_REQ_STRUCT);
		NdisMoveMemory(MsgElem.Msg, &DisReq, sizeof(MLME_DISASSOC_REQ_STRUCT));

		MlmeDisassocReqAction(pAd, &MsgElem);
		wait_event_interruptible_timeout(wait,0,HZ/1000);
	}

	VIRTUAL_IF_DEC(pAd);
	if (VIRTUAL_IF_NUM(pAd) == 0)
	{
		for(i=0;i<10 || pAd->CmdHandlerIsRunning==TRUE;i++)
			wait_event_interruptible_timeout(wait,0,HZ/1000);
		
		for(i=0;i<10 || pAd->Mlme.bRunning==TRUE;i++)
			wait_event_interruptible_timeout(wait,0,HZ/1000);

		//
		// Patch to fully turn off BBP, need to send a fake NULL frame.
		//
		RTUSBWriteMACRegister(pAd, MAC_CSR10, 0x0018);
		for (i=0; i<10; i++)
		{
			RTMPSendNullFrame(pAd, RATE_6);
			RTMPusecDelay(1000);
		}
			

		AsicDisableSync(pAd);
		
		RTMPSetLED(pAd, LED_HALT);
		atomic_set(&pAd->PendingRx, 0);
	}

#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT
{
	// TODO: Shall we send wireless event to notfiy user space that if-down??
}
#endif // NATIVE_WPA_SUPPLICANT_SUPPORT //

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	module_put(THIS_MODULE);
#else
	MOD_DEC_USE_COUNT;
#endif

	DBGPRINT(RT_DEBUG_TRACE, "%s<--\n", __FUNCTION__);
	return 0;
}

static INT usb_rtusb_sendpkt(
	IN	struct sk_buff		*pSkb,
	IN	struct net_device	*net_dev)
{

	/* transmit the packet */
	return RTMPSendPackets(pSkb, net_dev);
}

static int usb_rtusb_init_device(struct net_device *net_dev)
{
	PRTMP_ADAPTER   pAd = (PRTMP_ADAPTER) net_dev->priv;
	NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
	UCHAR           TmpPhy;
	
	DBGPRINT(RT_DEBUG_TRACE, "%s-->\n", __FUNCTION__);

	// init mediastate to disconnected
	OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED);
	
	//bottom half data is assign at  each task_scheduler
	tasklet_init(&pAd->rx_bh, RTUSBBulkRxHandle, (unsigned long)pAd);
	
	// Initialize pAd->PortCfg to manufacture default
	PortCfgInit(pAd);


	// Init  RTMP_ADAPTER CmdQElements
	Status = RTMPInitAdapterBlock(pAd); //always success
	if (Status != NDIS_STATUS_SUCCESS)
	{
		return Status;
	}

	//
	// Init send data structures and related parameters
	//
	Status = NICInitTransmit(pAd);      //if fail clean itself
	if (Status != NDIS_STATUS_SUCCESS)
	{
		return Status;
	}

	//
	// Init receive data structures and related parameters
	//
	Status = NICInitRecv(pAd);
	if (Status != NDIS_STATUS_SUCCESS)
	{
		goto out;
	}
	

	// Wait for hardware stable
	{
		ULONG MacCsr0 = 0, Index = 0;
		
		do
		{
			Status = RTUSBReadMACRegister(pAd, MAC_CSR0, &MacCsr0);

			if (MacCsr0 != 0)
				break;
			
			RTMPusecDelay(1000);
		} while (Index++ < 1000);
		DBGPRINT(RT_DEBUG_TRACE, "Init: MAC_CSR0=0x%08x, Status=0x%08x\n", MacCsr0, Status);
	}

	// Load 8051 firmware
	Status = NICLoadFirmware(pAd);
	if(Status != NDIS_STATUS_SUCCESS)
	{
		goto out;
	}

	// Initialize Asics
	NICInitializeAsic(pAd);

#ifdef BLOCK_NET_IF
	initblockQueueTab(pAd);
#endif // BLOCK_NET_IF //

	// Read RaConfig profile parameters 
#ifdef  READ_PROFILE_FROM_FILE 
	RTMPReadParametersFromFile(pAd);
#endif

	//
	// Read additional info from NIC such as MAC address
	// This function must called after register CSR base address
	//
#ifdef	INIT_FROM_EEPROM
	NICReadEEPROMParameters(pAd);
	NICInitAsicFromEEPROM(pAd);
#endif
	RTUSBWriteHWMACAddress(pAd);

	// external LNA has different R17 base
	if (pAd->NicConfig2.field.ExternalLNA)
	{
		pAd->BbpTuning.R17LowerBoundA += 0x10;
		pAd->BbpTuning.R17UpperBoundA += 0x10;
		pAd->BbpTuning.R17LowerBoundG += 0x10;
		pAd->BbpTuning.R17UpperBoundG += 0x10;
	}

	// hardware initialization after all parameters are acquired from
	// Registry or E2PROM
	TmpPhy = pAd->PortCfg.PhyMode;
	pAd->PortCfg.PhyMode = 0xff;
	RTMPSetPhyMode(pAd, TmpPhy);
	// max desired rate might be reset as call phymode setup, so set TxRate again
	if (pAd->PortCfg.DefaultMaxDesiredRate == 0)
		RTMPSetDesiredRates(pAd, -1);
	else
		RTMPSetDesiredRates(pAd, (LONG) (RateIdToMbps[pAd->PortCfg.DefaultMaxDesiredRate - 1] * 1000000));

	//
	// initialize MLME
	//
	Status = MlmeInit(pAd);
	if(Status != NDIS_STATUS_SUCCESS)
	{
		goto out;
	}

	// mlmethread & RTUSBCmd flag restart
	mlme_kill = 0;
	RTUSBCmd_kill =0;
	CreateThreads(net_dev);

	// at every open handler, copy mac address.
	COPY_MAC_ADDR(pAd->net_dev->dev_addr, pAd->CurrentAddress);

	// USB_ID info for UI
	pAd->VendorDesc = 0x148F2573;

	DBGPRINT(RT_DEBUG_TRACE, "%s<--\n", __FUNCTION__);

	return 0;

out:
	ReleaseAdapter(pAd, TRUE, FALSE);
	return Status;

}

INT MlmeThread(
    IN void * Context)
{
	PRTMP_ADAPTER	pAd = (PRTMP_ADAPTER)Context;
//2007/12/11:KH modified to fix compiled failed
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	daemonize();
#else
	daemonize("rt73");
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	allow_signal(SIGTERM);
#endif
	current->flags |= PF_NOFREEZE;
	/* signal that we've started the thread */
	complete(&(pAd->MlmeThreadNotify));

	while (1)
	{
		int status;
		
		status = down_interruptible(&pAd->mlme_semaphore);
		if (status != 0)
			continue;

		if (mlme_kill)
			break;

		MlmeHandler(pAd);

	}

	/* notify the exit routine that we're actually exiting now 
	 *
	 * complete()/wait_for_completion() is similar to up()/down(),
	 * except that complete() is safe in the case where the structure
	 * is getting deleted in a parallel mode of execution (i.e. just
	 * after the down() -- that's necessary for the thread-shutdown
	 * case.
	 *
	 * complete_and_exit() goes even further than this -- it is safe in
	 * the case that the thread of the caller is going away (not just
	 * the structure) -- this is necessary for the module-remove case.
	 * This is important in preemption kernels, which transfer the flow
	 * of execution immediately upon a complete().
	 */
	complete_and_exit (&pAd->MlmeThreadNotify, 0);
	DBGPRINT(RT_DEBUG_TRACE, "<---MlmeThread\n");

}

INT RTUSBCmdThread(
    IN void * Context)
{
	PRTMP_ADAPTER	pAd = (PRTMP_ADAPTER)Context;

//2007/12/11:KH modified to fix compiled failed
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	daemonize();
#else
	daemonize("rt73");
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	allow_signal(SIGTERM);
#endif
	current->flags |= PF_NOFREEZE;
	/* signal that we've started the thread */
	complete(&(pAd->CmdThreadNotify));

	while (1)	
	{		
		int status;				
		status = down_interruptible(&(pAd->RTUSBCmd_semaphore));		
		if (status != 0)			
			continue;				

		if (RTUSBCmd_kill)
			break;
		pAd->CmdHandlerIsRunning=TRUE;
		CMDHandler(pAd);	
		pAd->CmdHandlerIsRunning=FALSE;
	}

	/* notify the exit routine that we're actually exiting now 
	 *
	 * complete()/wait_for_completion() is similar to up()/down(),
	 * except that complete() is safe in the case where the structure
	 * is getting deleted in a parallel mode of execution (i.e. just
	 * after the down() -- that's necessary for the thread-shutdown
	 * case.
	 *
	 * complete_and_exit() goes even further than this -- it is safe in
	 * the case that the thread of the caller is going away (not just
	 * the structure) -- this is necessary for the module-remove case.
	 * This is important in preemption kernels, which transfer the flow
	 * of execution immediately upon a complete().
	 */
	complete_and_exit (&pAd->CmdThreadNotify, 0);
	DBGPRINT(RT_DEBUG_TRACE, "<---RTUSBCmdThread\n");

}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
static void *usb_rtusb_probe(struct usb_device *dev, UINT interface,
				const struct usb_device_id *id_table)
{	
	PRTMP_ADAPTER       pAd = (PRTMP_ADAPTER)NULL;
	int                 i;
	struct net_device   *netdev;
	int                 res = -ENOMEM;

	DBGPRINT(RT_DEBUG_TRACE, "%s-->\n", __FUNCTION__);

	for (i = 0; i < rtusb_usb_id_len; i++)
	{
		if (dev->descriptor.idVendor == rtusb_usb_id[i].idVendor &&
			dev->descriptor.idProduct == rtusb_usb_id[i].idProduct)
		{
			printk("idVendor = 0x%x, idProduct = 0x%x \n",dev->descriptor.idVendor, dev->descriptor.idProduct);
			break;
		}
	}
	if (i == rtusb_usb_id_len) {
		printk("Device Descriptor not matching\n");
		return NULL;
	}

	netdev = alloc_etherdev(sizeof(PRTMP_ADAPTER));
	if(!netdev)
	{
		printk("alloc_etherdev failed\n");
			
		MOD_DEC_USE_COUNT;
		usb_dec_dev_use(dev);
		return NULL;
	}
	
	netdev->priv = (PVOID)vmalloc(sizeof(RTMP_ADAPTER));
	pAd = netdev->priv;
	if(pAd == NULL)
	{
		kfree(pAd->net_dev);
		printk("vmalloc failed\n");
		return NULL;
	}

	NdisZeroMemory(pAd, sizeof(RTMP_ADAPTER));
	pAd->net_dev = netdev;
	netif_stop_queue(netdev);
	pAd->config = dev->config;
	pAd->pUsb_Dev= dev;
	SET_MODULE_OWNER(pAd->net_dev);
	ether_setup(pAd->net_dev);
	
	netdev->priv_flags = INT_MAIN;
	netdev->open = usb_rtusb_open;
	netdev->hard_start_xmit = usb_rtusb_sendpkt;
	netdev->stop = usb_rtusb_close;
	netdev->priv = pAd;
	netdev->get_stats = rt73_get_ether_stats;
#if (WIRELESS_EXT >= 12)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
	netdev->get_wireless_stats = rt73_get_wireless_stats;
#endif
	netdev->wireless_handlers = (struct iw_handler_def *) &rt73_iw_handler_def;
#endif
	netdev->do_ioctl = rt73_ioctl;
	pAd->net_dev->hard_header_len = 14;
	pAd->net_dev->mtu = 1500;
	pAd->net_dev->addr_len = 6;

	pAd->net_dev->weight = 64;

	OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED);

	{// find available 
		int 	i=0;
		char	slot_name[IFNAMSIZ];
		struct  net_device	*device;
		struct  usb_interface *ifp= &dev->actconfig->interface[interface];  // get interface from system           
		struct  usb_interface_descriptor *as;
		struct  usb_endpoint_descriptor *ep;             

		for (i = 0; i < 8; i++)
		{
			sprintf(slot_name, "rausb%d", i);
			
			read_lock_bh(&dev_base_lock); // avoid multiple init
//Benson modify -->
    			for (device = dev_base; device != NULL; device = device->next)
//<-- Benson modify
			{
				if (strncmp(device->name, slot_name, 4) == 0)
				{
					break;
				}
			}
			read_unlock_bh(&dev_base_lock);

			if(device == NULL)	break;
		}

		if(i == 8)
		{
			DBGPRINT(RT_DEBUG_ERROR, "No available slot name\n");
			return NULL;
		}

		sprintf(pAd->net_dev->name, "rausb%d", i);
		DBGPRINT(RT_DEBUG_ERROR, "usb device name %s\n",pAd->net_dev->name);

        /* get Max Packet Size from usb_dev endpoint */
//        ifp = dev->actconfig->interface + i;
        as = ifp->altsetting + ifp->act_altsetting;
        ep = as->endpoint;

        pAd->BulkOutMaxPacketSize = (USHORT)ep[i].wMaxPacketSize;
        DBGPRINT(RT_DEBUG_TRACE, "BulkOutMaxPacketSize  %d\n", pAd->BulkOutMaxPacketSize);
	}
	
	res = register_netdev(pAd->net_dev);
	if (res)
		goto out;

	RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST);

	if (usb_rtusb_init_device(pAd->net_dev)!=NDIS_STATUS_SUCCESS)
		goto out;


	DBGPRINT(RT_DEBUG_TRACE, "%s<--\n", __FUNCTION__);

	return pAd;
	
out:
	printk("register_netdev failed err=%d\n",res);
	return NULL;
}

//Disconnect function is called within exit routine
static void usb_rtusb_disconnect(struct usb_device *dev, void *ptr)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER) ptr;
	struct net_device   *net_dev = NULL;

	DBGPRINT(RT_DEBUG_TRACE, "%s-->\n", __FUNCTION__);
	
	RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST);

	if (!pAd)
		return;

	if (pAd)
		usb_rtusb_close_device(pAd);
	
	// for debug, wait to show some messages to /proc system
	udelay(1);
	//After Add Thread implementation, Upon exec there, pAd->net_dev seems becomes NULL, 
	//need to check why???
	//assert(pAd->net_dev != NULL)

	net_dev = pAd->net_dev;
	if(pAd->net_dev != NULL)
	{
		printk("unregister_netdev()\n");
		unregister_netdev (pAd->net_dev);
	}
	udelay(1);
	udelay(1);

	// free adapter
	vfree(pAd);

	if (net_dev != NULL)
		kfree(net_dev);

	while (MOD_IN_USE > 0) {
		MOD_DEC_USE_COUNT;
	}
	udelay(1);

	DBGPRINT(RT_DEBUG_TRACE, "%s<--\n", __FUNCTION__);

}

#else //LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
static int usb_rtusb_probe (struct usb_interface *intf,
					  const struct usb_device_id *id)
{	
	struct usb_device   *dev = interface_to_usbdev(intf);
	PRTMP_ADAPTER       pAd = (PRTMP_ADAPTER)NULL;
	int                 i;
	struct net_device   *netdev;
	int                 res = -ENOMEM;

	DBGPRINT(RT_DEBUG_TRACE, "%s-->\n", __FUNCTION__);

	usb_get_dev(dev);
	for (i = 0; i < rtusb_usb_id_len; i++)
	{
		if (dev->descriptor.idVendor == rtusb_usb_id[i].idVendor &&
			dev->descriptor.idProduct == rtusb_usb_id[i].idProduct)
		{
			printk("idVendor = 0x%x, idProduct = 0x%x \n",dev->descriptor.idVendor, dev->descriptor.idProduct);
			break;
		}
	}
	if (i == rtusb_usb_id_len) {
		printk("Device Descriptor not matching\n");
		return res;
	}

	netdev = alloc_etherdev(sizeof(PRTMP_ADAPTER));
	if(!netdev)
	{
		printk("alloc_etherdev failed\n");
		return res;
	}
	
	netdev->priv = (PVOID)vmalloc(sizeof(RTMP_ADAPTER));
	pAd = netdev->priv;

	if(!pAd)
	{
		free_netdev(netdev);
		printk("vmalloc failed\n");
		return res;
	}

	NdisZeroMemory(pAd, sizeof(RTMP_ADAPTER));
	pAd->net_dev = netdev;
	netif_stop_queue(netdev);
	pAd->config = &dev->config->desc;
	pAd->pUsb_Dev = dev;
//--> Benson add 20080505, for Linux_kernel > 2,6,24 
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
	SET_MODULE_OWNER(pAd->net_dev);
#endif
//<-- Benson end 20080505

	ether_setup(pAd->net_dev);
	
	netdev->priv_flags = INT_MAIN;
	netdev->open = usb_rtusb_open;
	netdev->stop = usb_rtusb_close;
	netdev->priv = pAd;
	netdev->hard_start_xmit = usb_rtusb_sendpkt;
	netdev->get_stats = rt73_get_ether_stats;

#if (WIRELESS_EXT >= 12)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
	netdev->get_wireless_stats = rt73_get_wireless_stats;
#endif
	netdev->wireless_handlers = (struct iw_handler_def *) &rt73_iw_handler_def;
#endif
	netdev->do_ioctl = rt73_ioctl;

	pAd->net_dev->hard_header_len = 14;
	pAd->net_dev->mtu = 1500;
	pAd->net_dev->addr_len = 6;

//--> Benson add 20080505, for Linux_kernel > 2,6,24
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
	pAd->net_dev->weight = 64;
#endif
//<-- Benson end 20080505

	OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED);
	
	pAd->MLMEThr_pid= -1;
	pAd->RTUSBCmdThr_pid= -1;

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
    SET_ETHTOOL_OPS(pAd->net_dev, &rt73_ethtool_ops);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
    SET_NETDEV_DEV(pAd->net_dev, &(intf->dev));
#endif

    
	{// find available 
		int 	i=0;
		char	slot_name[IFNAMSIZ];
		struct  usb_host_interface *iface_desc;
		struct  usb_endpoint_descriptor *endpoint;
		struct  net_device   *device;
        
		for (i = 0; i < 8; i++)
		{
			sprintf(slot_name, "rausb%d", i);

#if 1          
//--> Benson add 20080505, for Linux_kernel > 2,6,24 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
      		if((device=dev_get_by_name(&init_net, slot_name))==NULL)
				break;
#else
			if((device=dev_get_by_name(slot_name))==NULL)                
				break;
#endif
			else
				dev_put(device);
//<-- Benson end 20080505
#else			
			read_lock_bh(&dev_base_lock); // avoid multiple init
//Benson modify -->
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,21)
                        for (device = dev_base; device != NULL; device = device->next)
#else
                        for (device = first_net_device(); device != NULL; device = next_net_device(device))
#endif
//<-- Benson modify
			{
				if (strncmp(device->name, slot_name, 4) == 0)
				{
					break;
				}
			}
			read_unlock_bh(&dev_base_lock);
			
			if(device == NULL)	break;
#endif			
		}
		if(i == 8)
		{
			DBGPRINT(RT_DEBUG_ERROR, "No available slot name\n");
			return res;
		}

		sprintf(pAd->net_dev->name, "rausb%d", i);
		DBGPRINT(RT_DEBUG_ERROR, "usb device name %s\n", pAd->net_dev->name);


		/* get the active interface descriptor */
		iface_desc = intf->cur_altsetting;

		/* check out the endpoint: it has to be Interrupt & IN */
		endpoint = &iface_desc->endpoint[i].desc;
        
		/* get Max Packet Size from endpoint */
		pAd->BulkOutMaxPacketSize = (USHORT)endpoint->wMaxPacketSize;
		DBGPRINT(RT_DEBUG_TRACE, "BulkOutMaxPacketSize  %d\n", pAd->BulkOutMaxPacketSize);

	}

	res = register_netdev(pAd->net_dev);
	if (res)
		goto out;

	usb_set_intfdata(intf, pAd);

	RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST);

	if (usb_rtusb_init_device(pAd->net_dev)!=NDIS_STATUS_SUCCESS)
		goto out;


	pAd->ProbeFinish = TRUE;	

	//RTUSBBulkReceive(pAd);
	
	DBGPRINT(RT_DEBUG_TRACE, "%s<--\n", __FUNCTION__);

	return 0;
	
out:
	printk("register_netdev failed err=%d\n",res);
	free_netdev(netdev);

	return -1;
}

static void usb_rtusb_disconnect(struct usb_interface *intf)
{
	struct net_device   *net_dev = NULL;
	struct usb_device   *dev = interface_to_usbdev(intf);
	PRTMP_ADAPTER       pAd = (PRTMP_ADAPTER)NULL;
	pAd = usb_get_intfdata(intf);

	DBGPRINT(RT_DEBUG_TRACE, "%s-->\n", __FUNCTION__);

	usb_set_intfdata(intf, NULL);
	RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST);
	DBGPRINT(RT_DEBUG_ERROR, "unregister usbnet usb-%s-%s\n",
			dev->bus->bus_name, dev->devpath);

	if (!pAd)
		return;

	if (pAd)
		usb_rtusb_close_device(pAd);
	
	// for debug, wait to show some messages to /proc system
	udelay(1);
	//After Add Thread implementation, Upon exec there, pAd->net_dev seems becomes NULL, 
	//need to check why???
	//assert(pAd->net_dev != NULL)
	net_dev = pAd->net_dev;
	if(pAd->net_dev!= NULL)
	{
		printk("unregister_netdev()\n");
		unregister_netdev (pAd->net_dev);
	}
	udelay(1);
	flush_scheduled_work ();
	udelay(1);

	// free adapter
	vfree(pAd);

	if (net_dev != NULL)
		free_netdev(net_dev);

	usb_put_dev(dev);
	udelay(1);

	DBGPRINT(RT_DEBUG_TRACE, "%s<--\n", __FUNCTION__);

}
#endif


//
// Driver module load function
//
INT __init usb_rtusb_init(void)
{
	DBGPRINT(RT_DEBUG_TRACE, "%s-->\n", __FUNCTION__);
    
	return usb_register(&rtusb_driver);
}

//
// Driver module unload function
//
VOID __exit usb_rtusb_exit(void)
{
	udelay(1);
	udelay(1);
	usb_deregister(&rtusb_driver);
	
	DBGPRINT(RT_DEBUG_TRACE, "%s<--\n", __FUNCTION__);
}
/**************************************/
module_init(usb_rtusb_init);
module_exit(usb_rtusb_exit);
/**************************************/

