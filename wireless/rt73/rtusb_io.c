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
	rtusb_io.c

	Abstract:

	Revision History:
	Who			When	    What
	--------	----------  ----------------------------------------------
	Name		Date	    Modification logs
	Paul Lin    06-25-2004  created
*/

#include	"rt_config.h"

/*
	========================================================================
	
	Routine Description: NIC initialization complete

	Arguments:

	Return Value:

	Note:
	
	========================================================================
*/
NTSTATUS	RTUSBFirmwareRun(
	IN	PRTMP_ADAPTER	pAd)
{
	NTSTATUS	Status;

	Status = RTUSB_VendorRequest(
		pAd,
		0,
		DEVICE_VENDOR_REQUEST_OUT,
		0x01,
		0x8,
		0,
		NULL,
		0);
	
	return Status;
}

/*
	========================================================================
	
	Routine Description: Read various length data from RT2573

	Arguments:

	Return Value:

	Note:
	
	========================================================================
*/
NTSTATUS	RTUSBMultiRead(
	IN	PRTMP_ADAPTER	pAd,
	IN	USHORT			Offset,
	OUT	PUCHAR			pData,
	IN	USHORT			length)
{
	NTSTATUS	Status;
//2008/01/07:KH add to solve the racing condition of Mac Registers
	wait_queue_head_t wait;
	init_waitqueue_head(&wait);
		
	for(;pAd->MacRegWrite_Processing==1;)
		wait_event_interruptible_timeout(wait,0,1);
	RTUSBMacRegDown(pAd);


	Status = RTUSB_VendorRequest(
		pAd,
		0,
		DEVICE_VENDOR_REQUEST_IN,
		0x7,
		0,
		Offset,
		pData,
		length);


	RTUSBMacRegUp(pAd);
	
	return Status;
}
//iverson 2007 1109
NTSTATUS RTUSBSingleWrite(
    IN     PRTMP_ADAPTER   pAd,
    IN  USHORT                   Offset,
    IN  USHORT                   Value)
{


        NTSTATUS        status;

        status = RTUSB_VendorRequest(pAd,
                0,
                DEVICE_VENDOR_REQUEST_OUT,
                0x2,
                Value,
                Offset,
                NULL,
                0);


        return status;
}


/*
	========================================================================
	
	Routine Description: Write various length data to RT2573

	Arguments:

	Return Value:

	Note:
	
	========================================================================
*/
NTSTATUS	RTUSBMultiWrite(
	IN	PRTMP_ADAPTER	pAd,
	IN	USHORT			Offset,
	IN	PUCHAR			pData,
	IN	USHORT			length)
{
	NTSTATUS	Status;
        USHORT          index = 0,Value;
        PUCHAR          pSrc = pData;
        USHORT          resude = 0;
//2008/01/07:KH add to solve the racing condition of Mac Registers
 	wait_queue_head_t wait;
	init_waitqueue_head(&wait);

	for(;pAd->MacRegWrite_Processing==1;)
		wait_event_interruptible_timeout(wait,0,HZ/1000);
 
	RTUSBMacRegDown(pAd);
        resude = length % 2;
        length  += resude ;

        do
        {
                Value =(USHORT)( *pSrc  | (*(pSrc + 1) << 8));
                Status = RTUSBSingleWrite(pAd,Offset + index,Value);
                index +=2;
                length -= 2;
                pSrc = pSrc + 2;
        }while(length > 0);
		
	RTUSBMacRegUp(pAd);
	return Status;
}

/*
	========================================================================
	
	Routine Description: Read 32-bit MAC register

	Arguments:

	Return Value:

	Note:
	
	========================================================================
*/
NTSTATUS	RTUSBReadMACRegister(
	IN	PRTMP_ADAPTER	pAd,
	IN	USHORT			Offset,
	OUT	PULONG			pValue)
{
	NTSTATUS	Status;
#ifdef BIG_ENDIAN		
	ULONG		Value_Big;
#endif
//2008/01/07:KH add to solve the racing condition of Mac Registers
        wait_queue_head_t wait;
	init_waitqueue_head(&wait);
		
	for(;pAd->MacRegWrite_Processing==1;)
		wait_event_interruptible_timeout(wait,0,1);
	
	RTUSBMacRegDown(pAd);

#ifdef BIG_ENDIAN
	Status = RTUSB_VendorRequest(
		pAd,
		0,
		DEVICE_VENDOR_REQUEST_IN,
		0x7,
		0,
		Offset,
		&Value_Big,
		4);
	
	*pValue = SWAP32(Value_Big);
#else
	Status = RTUSB_VendorRequest(
		pAd,
		0,
		DEVICE_VENDOR_REQUEST_IN,
		0x7,
		0,
		Offset,
		pValue,
		4);
#endif

RTUSBMacRegUp(pAd);

	return Status;
}

/*
	========================================================================
	
	Routine Description: Write 32-bit MAC register

	Arguments:

	Return Value:

	Note:
	
	========================================================================
*/
NTSTATUS	RTUSBWriteMACRegister(
	IN	PRTMP_ADAPTER	pAd,
	IN	USHORT			Offset,
	IN	ULONG			Value)
{
	NTSTATUS	Status;

	USHORT	va1;
	USHORT	va2;
//2008/01/07:KH add to solve the racing condition of Mac Registers
	wait_queue_head_t wait;

	init_waitqueue_head(&wait);
		
	for(;pAd->MacRegWrite_Processing==1;)
		wait_event_interruptible_timeout(wait,0,1);
	
	RTUSBMacRegDown(pAd);
	
	va1=(USHORT)(Value & 0xffff);
	va2=(USHORT)((Value & 0xffff0000) >> 16);
	Status = RTUSBSingleWrite(pAd, Offset, va1);
	Status = RTUSBSingleWrite(pAd, Offset + 2, va2);
        RTUSBMacRegUp(pAd);
	return Status;


}

/*
	========================================================================
	
	Routine Description: Write 32-bit MAC register

	Arguments:

	Return Value:

	Note:
	
	========================================================================
*/
NTSTATUS    RTUSBSetLED(
	IN	PRTMP_ADAPTER		pAd,
	IN	MCU_LEDCS_STRUC		LedStatus,
	IN	USHORT				LedIndicatorStrength)
{
	NTSTATUS	Status;
//2008/01/07:KH add to solve the racing condition of Mac Registers
        wait_queue_head_t wait;
	init_waitqueue_head(&wait);

	for(;pAd->MacRegWrite_Processing==1;)
		wait_event_interruptible_timeout(wait,0,1);
	
	RTUSBMacRegDown(pAd);
	
	Status = RTUSB_VendorRequest(
		pAd,
		0,
		DEVICE_VENDOR_REQUEST_OUT,
		0x0a,
		LedStatus.word,
		LedIndicatorStrength,
		NULL,
		0);
	RTUSBMacRegUp(pAd);
	return Status;
}

/*
	========================================================================
	
	Routine Description: Read 8-bit BBP register

	Arguments:

	Return Value:

	Note:
	
	========================================================================
*/
NTSTATUS	RTUSBReadBBPRegister(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			Id,
	IN	PUCHAR			pValue)
{
	PHY_CSR3_STRUC	PhyCsr3;
	UINT			i = 0;

	// Verify the busy condition
	do
	{
		RTUSBReadMACRegister(pAd, PHY_CSR3, &PhyCsr3.word);
		if (!(PhyCsr3.field.Busy == BUSY))
			break;
		i++;
	}
	while ((i < RETRY_LIMIT) && (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)));
	
	if ((i == RETRY_LIMIT) || (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)))
	{
		//
		// Read failed then Return Default value.
		//
		*pValue = pAd->BbpWriteLatch[Id];
	
		DBGPRINT_RAW(RT_DEBUG_ERROR, "Retry count exhausted or device removed!!!\n");
		return STATUS_UNSUCCESSFUL;
	}

	// Prepare for write material
	PhyCsr3.word 				= 0;
	PhyCsr3.field.fRead			= 1;
	PhyCsr3.field.Busy			= 1;
	PhyCsr3.field.RegNum 		= Id;
	RTUSBWriteMACRegister(pAd, PHY_CSR3, PhyCsr3.word);

	i = 0;	
	// Verify the busy condition
	do
	{
		RTUSBReadMACRegister(pAd, PHY_CSR3, &PhyCsr3.word);
		if (!(PhyCsr3.field.Busy == BUSY))
		{
			*pValue = (UCHAR)PhyCsr3.field.Value;
			break;
		}
		i++;
	}
	while ((i < RETRY_LIMIT) && (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)));
	
	if ((i == RETRY_LIMIT) || (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)))
	{
		//
		// Read failed then Return Default value.
		//
		*pValue = pAd->BbpWriteLatch[Id];

		DBGPRINT_RAW(RT_DEBUG_ERROR, "Retry count exhausted or device removed!!!\n");
		return STATUS_UNSUCCESSFUL;
	}
	
	return STATUS_SUCCESS;
}

/*
	========================================================================
	
	Routine Description: Write 8-bit BBP register

	Arguments:

	Return Value:
	
	Note:
	
	========================================================================
*/
NTSTATUS	RTUSBWriteBBPRegister(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			Id,
	IN	UCHAR			Value)
{
	PHY_CSR3_STRUC	PhyCsr3;
	UINT			i = 0;

	// Verify the busy condition
	do
	{
		RTUSBReadMACRegister(pAd, PHY_CSR3, &PhyCsr3.word);
		if (!(PhyCsr3.field.Busy == BUSY))
			break;
		i++;
	}
	while ((i < RETRY_LIMIT) && (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)));
	
	if ((i == RETRY_LIMIT) || (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)))
	{
		DBGPRINT_RAW(RT_DEBUG_ERROR, "Retry count exhausted or device removed!!!\n");
		return STATUS_UNSUCCESSFUL;
	}

	// Prepare for write material
	PhyCsr3.word 				= 0;
	PhyCsr3.field.fRead			= 0;
	PhyCsr3.field.Value			= Value;
	PhyCsr3.field.Busy			= 1;
	PhyCsr3.field.RegNum 		= Id;
	RTUSBWriteMACRegister(pAd, PHY_CSR3, PhyCsr3.word);
	
	pAd->BbpWriteLatch[Id] = Value;

	return STATUS_SUCCESS;
}

/*
	========================================================================
	
	Routine Description: Write RF register through MAC

	Arguments:

	Return Value:
	
	Note:
	
	========================================================================
*/
NTSTATUS	RTUSBWriteRFRegister(
	IN	PRTMP_ADAPTER	pAd,
	IN	ULONG			Value)
{
	PHY_CSR4_STRUC	PhyCsr4;
	UINT			i = 0;

	do
	{
		RTUSBReadMACRegister(pAd, PHY_CSR4, &PhyCsr4.word);
		if (!(PhyCsr4.field.Busy))
			break;
		i++;
	}
	while ((i < RETRY_LIMIT) && (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)));

	if ((i == RETRY_LIMIT) || (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)))
	{
		DBGPRINT_RAW(RT_DEBUG_ERROR, "Retry count exhausted or device removed!!!\n");
		return STATUS_UNSUCCESSFUL;
	}

	RTUSBWriteMACRegister(pAd, PHY_CSR4, Value);
	
	return STATUS_SUCCESS;
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:

	Note:
	
	========================================================================
*/
NTSTATUS	RTUSBReadEEPROM(
	IN	PRTMP_ADAPTER	pAd,
	IN	USHORT			Offset,
	OUT	PUCHAR			pData,
	IN	USHORT			length)
{
	NTSTATUS	Status;

	Status = RTUSB_VendorRequest(
		pAd,
		0,
		DEVICE_VENDOR_REQUEST_IN,
		0x9,
		0,
		Offset,
		pData,
		length);
	
	return Status;
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:
	
	Note:
	
	========================================================================
*/
NTSTATUS	RTUSBWriteEEPROM(
	IN	PRTMP_ADAPTER	pAd,
	IN	USHORT			Offset,
	IN	PUCHAR			pData,
	IN	USHORT			length)
{
	NTSTATUS	Status;
//2008/01/07:KH add to solve the racing condition of Mac Registers
	wait_queue_head_t wait;

	init_waitqueue_head(&wait);
		
	for(;pAd->MacRegWrite_Processing==1;)
		wait_event_interruptible_timeout(wait,0,1);
	
	RTUSBMacRegDown(pAd);
	
	Status = RTUSB_VendorRequest(
		pAd,
		0,
		DEVICE_VENDOR_REQUEST_OUT,
		0x8,
		0,
		Offset,
		pData,
		length);
	RTUSBMacRegUp(pAd);
	return Status;
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:
	
	Note:
	
	========================================================================
*/
NTSTATUS RTUSBPutToSleep(
	IN	PRTMP_ADAPTER	pAd)
{
	NTSTATUS	Status;
//2008/01/07:KH add to solve the racing condition of Mac Registers
	wait_queue_head_t wait;

	init_waitqueue_head(&wait);
		
	for(;pAd->MacRegWrite_Processing==1;)
		wait_event_interruptible_timeout(wait,0,1);
	
	RTUSBMacRegDown(pAd);
	
	Status = RTUSB_VendorRequest(
		pAd,
		0,
		DEVICE_VENDOR_REQUEST_OUT,
		0x01,
		0x07,
		0,
		NULL,
		0);
	RTUSBMacRegUp(pAd);
	return Status;
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:
	
	Note:
	
	========================================================================
*/
NTSTATUS RTUSBWakeUp(
	IN	PRTMP_ADAPTER	pAd)
{
	NTSTATUS	Status;
	//2008/01/07:KH add to solve the racing condition of Mac Registers
	wait_queue_head_t wait;

	init_waitqueue_head(&wait);
		
	for(;pAd->MacRegWrite_Processing==1;)
		wait_event_interruptible_timeout(wait,0,1);
	
	RTUSBMacRegDown(pAd);
	
	Status = RTUSB_VendorRequest(
		pAd,
		0,
		DEVICE_VENDOR_REQUEST_OUT,
		0x01,
		0x09,
		0,
		NULL,
		0);
	RTUSBMacRegUp(pAd);
	return Status;
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:
	
	Note:
	
	========================================================================
*/
VOID	RTUSBInitializeCmdQ(
	IN	PCmdQ	cmdq)
{
	cmdq->head = NULL;
	cmdq->tail = NULL;
	cmdq->size = 0;
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:
	
	Note:
	
	========================================================================
*/
NDIS_STATUS	RTUSBEnqueueCmdFromNdis(
	IN	PRTMP_ADAPTER	pAd,
	IN	NDIS_OID		Oid,
	IN	BOOLEAN			SetInformation,
	IN	PVOID			pInformationBuffer,
	IN	ULONG			InformationBufferLength)
{
	PCmdQElmt	cmdqelmt = NULL;
	unsigned long	IrqFlags;
	
	if (pAd->RTUSBCmdThr_pid < 0) 
		return (NDIS_STATUS_RESOURCES);
        
    cmdqelmt = (PCmdQElmt) kmalloc(sizeof(CmdQElmt), MEM_ALLOC_FLAG);
	if (!cmdqelmt) 
	{
		DBGPRINT(RT_DEBUG_ERROR,"Not enough memory\n");
		kfree((PCmdQElmt)cmdqelmt);
		return NDIS_STATUS_RESOURCES;
	}

	if ((Oid == RT_OID_MULTI_READ_MAC) ||
		(Oid == RT_OID_VENDOR_READ_BBP) ||
#ifdef DBG		
		(Oid == RT_OID_802_11_QUERY_HARDWARE_REGISTER) ||
#endif		
		(Oid == RT_OID_USB_VENDOR_EEPROM_READ))
	{
		cmdqelmt->buffer = pInformationBuffer;
	}
	else
	{
		cmdqelmt->buffer = NULL;
		if (pInformationBuffer != NULL)
		{
			cmdqelmt->buffer =	kmalloc(InformationBufferLength, MEM_ALLOC_FLAG);
			if ((!cmdqelmt->buffer) )
			{       
                kfree((PVOID)cmdqelmt->buffer);
				kfree((PCmdQElmt)cmdqelmt);
				return (NDIS_STATUS_RESOURCES);
			}
			else
			{
				NdisMoveMemory(cmdqelmt->buffer, pInformationBuffer, InformationBufferLength);
				cmdqelmt->bufferlength = InformationBufferLength;
			}
		}
		else
			cmdqelmt->bufferlength = 0;
	}
	
	cmdqelmt->command = Oid;
	cmdqelmt->CmdFromNdis = TRUE;
	if (SetInformation == TRUE)
		cmdqelmt->SetOperation = TRUE;
	else
		cmdqelmt->SetOperation = FALSE;

	NdisAcquireSpinLock(&pAd->CmdQLock,  IrqFlags);
	EnqueueCmd((&pAd->CmdQ), cmdqelmt);
	NdisReleaseSpinLock(&pAd->CmdQLock,  IrqFlags);
	
    RTUSBCMDUp(pAd);
	
	if ((Oid == OID_802_11_BSSID_LIST_SCAN) ||
		(Oid == RT_OID_802_11_BSSID) ||
		(Oid == OID_802_11_SSID) ||
		(Oid == OID_802_11_DISASSOCIATE))
	{
		return(NDIS_STATUS_SUCCESS);
	}
	
    return(NDIS_STATUS_SUCCESS);
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:
	
	Note:
	
	========================================================================
*/
VOID	RTUSBEnqueueInternalCmd(
	IN	PRTMP_ADAPTER	pAd,
	IN	NDIS_OID		Oid)
{
	PCmdQElmt	cmdqelmt = NULL;
	unsigned long	IrqFlags;
	
    if (pAd->RTUSBCmdThr_pid < 0) 
		return;
		
	switch (Oid)
	{
		case RT_OID_CHECK_GPIO:
			cmdqelmt = &(pAd->CmdQElements[CMD_CHECK_GPIO]);
			break;
			
		case RT_OID_PERIODIC_EXECUT:
			cmdqelmt = &(pAd->CmdQElements[CMD_PERIODIC_EXECUT]);
			break;
			
		//For Alpha only
		case RT_OID_ASICLED_EXECUT:
			cmdqelmt = &(pAd->CmdQElements[CMD_ASICLED_EXECUT]);
			break;

		case RT_OID_UPDATE_TX_RATE:
			cmdqelmt = &(pAd->CmdQElements[CMD_UPDATE_TX_RATE]);
			break;
			
		case RT_OID_SET_PSM_BIT_SAVE:
			cmdqelmt = &(pAd->CmdQElements[CMD_SET_PSM_SAVE]);
			break;
			
		case RT_OID_LINK_DOWN:
			cmdqelmt = &(pAd->CmdQElements[CMD_LINK_DOWN]);
			break;
			
		case RT_OID_USB_RESET_BULK_IN:
			cmdqelmt = &(pAd->CmdQElements[CMD_RESET_BULKIN]);
			break;
			
		case RT_OID_USB_RESET_BULK_OUT:
			cmdqelmt = &(pAd->CmdQElements[CMD_RESET_BULKOUT]);
			break;
			
		case RT_OID_RESET_FROM_ERROR:
			cmdqelmt = &(pAd->CmdQElements[CMD_RESET_FROM_ERROR]);
			break;
			
		case RT_OID_RESET_FROM_NDIS:
			cmdqelmt = &(pAd->CmdQElements[CMD_RESET_FROM_NDIS]);
			break;

		case RT_PERFORM_SOFT_DIVERSITY:
			cmdqelmt = &(pAd->CmdQElements[CMD_SOFT_DIVERSITY]);
			break;

        case RT_OID_FORCE_WAKE_UP:
            cmdqelmt = &(pAd->CmdQElements[CMD_FORCE_WAKEUP]);
            break;
 
        case RT_OID_SET_PSM_BIT_ACTIVE:
            cmdqelmt = &(pAd->CmdQElements[CMD_SET_PSM_ACTIVE]);
        break;
 
		default:
			break;
	}

	if ((cmdqelmt != NULL) && (cmdqelmt->InUse == FALSE) && (pAd->RTUSBCmdThr_pid > 0))
	{
		cmdqelmt->InUse = TRUE;
		cmdqelmt->command = Oid;

		NdisAcquireSpinLock(&pAd->CmdQLock, IrqFlags);
		EnqueueCmd((&pAd->CmdQ), cmdqelmt);
		NdisReleaseSpinLock(&pAd->CmdQLock, IrqFlags);
		
		RTUSBCMDUp(pAd);
	}
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:
	
	Note:
	
	========================================================================
*/
VOID	RTUSBDequeueCmd(
	IN	PCmdQ		cmdq,
	OUT	PCmdQElmt	*pcmdqelmt)
{
	*pcmdqelmt = cmdq->head;
	
	if (*pcmdqelmt != NULL)
	{
		cmdq->head = cmdq->head->next;
		cmdq->size--;
		if (cmdq->size == 0)
			cmdq->tail = NULL;
	}
}

/*
    ========================================================================
	  usb_control_msg - Builds a control urb, sends it off and waits for completion
	  @dev: pointer to the usb device to send the message to
	  @pipe: endpoint "pipe" to send the message to
	  @request: USB message request value
	  @requesttype: USB message request type value
	  @value: USB message value
	  @index: USB message index value
	  @data: pointer to the data to send
	  @size: length in bytes of the data to send
	  @timeout: time in jiffies to wait for the message to complete before
			  timing out (if 0 the wait is forever)
	  Context: !in_interrupt ()

	  This function sends a simple control message to a specified endpoint
	  and waits for the message to complete, or timeout.
	  If successful, it returns the number of bytes transferred, otherwise a negative error number.

	 Don't use this function from within an interrupt context, like a
	  bottom half handler.	If you need an asynchronous message, or need to send
	  a message from within interrupt context, use usb_submit_urb()
	  If a thread in your driver uses this call, make sure your disconnect()
	  method can wait for it to complete.  Since you don't have a handle on
	  the URB used, you can't cancel the request.
  
	
	Routine Description:

	Arguments:

	Return Value:
	
	Note:
	
	========================================================================
*/
INT	    RTUSB_VendorRequest(
	IN	PRTMP_ADAPTER	pAd,
	IN	ULONG			TransferFlags,
	IN	UCHAR			RequestType,
	IN	UCHAR			Request,
	IN	USHORT			Value,
	IN	USHORT			Index,
	IN	PVOID			TransferBuffer,
	IN	ULONG			TransferBufferLength)
{
	int ret;
	int RetryCnt;

	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST))
	{
		DBGPRINT(RT_DEBUG_ERROR,"device disconnected\n");
		return -1;
	}
	else if (in_interrupt())	
	{
		DBGPRINT(RT_DEBUG_ERROR,"in_interrupt, return RTUSB_VendorRequest\n");

		return -1;
	}
	else
	{
		RetryCnt = 0;
		do
		{
			if( RequestType == DEVICE_VENDOR_REQUEST_OUT)
				ret=usb_control_msg(pAd->pUsb_Dev, usb_sndctrlpipe( pAd->pUsb_Dev, 0 ), Request, RequestType, Value,Index, TransferBuffer, TransferBufferLength, CONTROL_TIMEOUT_JIFFIES);
			else if(RequestType == DEVICE_VENDOR_REQUEST_IN)
				ret=usb_control_msg(pAd->pUsb_Dev, usb_rcvctrlpipe( pAd->pUsb_Dev, 0 ), Request, RequestType, Value,Index, TransferBuffer, TransferBufferLength, CONTROL_TIMEOUT_JIFFIES);
			else
			{
				DBGPRINT(RT_DEBUG_ERROR,"vendor request direction is failed\n");
				ret = -1;
				break;
			}
			DBGPRINT(RT_DEBUG_INFO,"%x\t%x\t%x\t%x\n",Request,RequestType,Value,Index);

			if( ret >= 0)
				break;
			else
				RTMPusecDelay(50 * 1000); /* pause for 50 ms. */
		} while (RetryCnt++ < 5);

        if (ret < 0)
			DBGPRINT(RT_DEBUG_ERROR,"USBVendorRequest failed ret=%d \n",ret);
	}
	return ret;
}

/*
	========================================================================
	
	Routine Description:
	  Creates an IRP to submite an IOCTL_INTERNAL_USB_RESET_PORT
	  synchronously. Callers of this function must be running at
	  PASSIVE LEVEL.

	Arguments:

	Return Value:

	Note:
	
	========================================================================
*/
NTSTATUS	RTUSB_ResetDevice(
	IN	PRTMP_ADAPTER	pAd)
{
	NTSTATUS		Status = TRUE;

	DBGPRINT_RAW(RT_DEBUG_TRACE, "--->USB_ResetDevice\n");
	//RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS);
	return Status;
}

VOID CMDHandler(
    IN PRTMP_ADAPTER pAd) 
{
	PCmdQElmt	cmdqelmt;
	PUCHAR	    pData;
	NDIS_STATUS	NdisStatus = NDIS_STATUS_SUCCESS;
	unsigned long	IrqFlags;
	ULONG       Now;
    
	while (pAd->CmdQ.size > 0)
	{
		NdisStatus = NDIS_STATUS_SUCCESS;
		NdisAcquireSpinLock(&pAd->CmdQLock, IrqFlags);
		RTUSBDequeueCmd(&pAd->CmdQ, &cmdqelmt);
		NdisReleaseSpinLock(&pAd->CmdQLock, IrqFlags);
		if (cmdqelmt == NULL)
			break;
		pData = cmdqelmt->buffer;

        //DBGPRINT_RAW(RT_DEBUG_INFO, "Cmd = %x\n", cmdqelmt->command);
		switch (cmdqelmt->command)
		{
			case RT_OID_CHECK_GPIO:
			{
				ULONG data;
				// Read GPIO pin7 as Hardware controlled radio state
				RTUSBReadMACRegister(pAd, MAC_CSR13, &data);
				if (data & 0x80)
				{
					pAd->PortCfg.bHwRadio = TRUE;
				}
				else
				{
					pAd->PortCfg.bHwRadio = FALSE;
				}
				if (pAd->PortCfg.bRadio != (pAd->PortCfg.bHwRadio && pAd->PortCfg.bSwRadio))
				{
					pAd->PortCfg.bRadio = (pAd->PortCfg.bHwRadio && pAd->PortCfg.bSwRadio);
					if (pAd->PortCfg.bRadio == TRUE)
					{
						MlmeRadioOn(pAd);
						// Update extra information
						pAd->ExtraInfo = EXTRA_INFO_CLEAR;
					}
					else
					{
						MlmeRadioOff(pAd);
						// Update extra information
						pAd->ExtraInfo = HW_RADIO_OFF;
					}
				}		
			}
			break;

			case RT_OID_PERIODIC_EXECUT:
				//printk("RT_OID_PERIODIC_EXECUT\n");
			    STAMlmePeriodicExec(pAd);
			break;

			case OID_802_11_BSSID_LIST_SCAN:
			{
				if (pAd->Mlme.CntlMachine.CurrState != CNTL_IDLE)
				{
					MlmeEnqueue(pAd, 
					            MLME_CNTL_STATE_MACHINE,
					            RT_CMD_RESET_MLME,
					            0,
					            NULL);
				}

				Now = jiffies;
				// Reset Missed scan number
				pAd->PortCfg.ScanCnt = 0;
				pAd->PortCfg.LastScanTime = Now;
				MlmeEnqueue(pAd,
							MLME_CNTL_STATE_MACHINE,
							OID_802_11_BSSID_LIST_SCAN,
							0,
							NULL);
				RTUSBMlmeUp(pAd);
			}
			break;
			
			case RT_OID_802_11_BSSID:
			{

				if (pAd->Mlme.CntlMachine.CurrState != CNTL_IDLE)
				{
					MlmeEnqueue(pAd, 
					            MLME_CNTL_STATE_MACHINE,
					            RT_CMD_RESET_MLME,
					            0,
					            NULL);
				}

				// Reset allowed scan retries
				pAd->PortCfg.ScanCnt = 0;

				MlmeEnqueue(pAd,
							MLME_CNTL_STATE_MACHINE,
							OID_802_11_BSSID,
							cmdqelmt->bufferlength,
							cmdqelmt->buffer);
				RTUSBMlmeUp(pAd);
			}
			break;
			
			case OID_802_11_SSID:
			{
				if (pAd->Mlme.CntlMachine.CurrState != CNTL_IDLE)
				{				
					MlmeEnqueue(pAd, 
					            MLME_CNTL_STATE_MACHINE,
					            RT_CMD_RESET_MLME,
					            0,
					            NULL);	

				}

				// Reset allowed scan retries
				pAd->PortCfg.ScanCnt = 0;
				pAd->bConfigChanged = TRUE;

				MlmeEnqueue(pAd,
							MLME_CNTL_STATE_MACHINE,
							OID_802_11_SSID,
							cmdqelmt->bufferlength, 
							pData);
				RTUSBMlmeUp(pAd);
			}
			break;

			case OID_802_11_DISASSOCIATE:
			{
				if (pAd->Mlme.CntlMachine.CurrState != CNTL_IDLE)
				{
					MlmeEnqueue(pAd, 
					            MLME_CNTL_STATE_MACHINE,
					            RT_CMD_RESET_MLME,
					            0,
					            NULL);
				}

				// Set to immediately send the media disconnect event
				pAd->MlmeAux.CurrReqIsFromNdis = TRUE;

				MlmeEnqueue(pAd,
							MLME_CNTL_STATE_MACHINE,
							OID_802_11_DISASSOCIATE,
							0,
							NULL);
				RTUSBMlmeUp(pAd);
			}
			break;

			case OID_802_11_RX_ANTENNA_SELECTED:
			{

		        NDIS_802_11_ANTENNA	Antenna = *(NDIS_802_11_ANTENNA *)pData;

				    if (Antenna == 0) 
					    pAd->Antenna.field.RxDefaultAntenna = 1;    // ant-A
				    else if(Antenna == 1)
					    pAd->Antenna.field.RxDefaultAntenna = 2;    // ant-B
				    else
					    pAd->Antenna.field.RxDefaultAntenna = 0;    // diversity

			    pAd->PortCfg.BandState = UNKNOWN_BAND;
			    AsicAntennaSelect(pAd, pAd->LatchRfRegs.Channel);
			    DBGPRINT(RT_DEBUG_TRACE, "CMDHandler::OID_802_11_RX_ANTENNA_SELECTED (=%d)\n", Antenna);
            }
		    break;

			case OID_802_11_TX_ANTENNA_SELECTED:
		    {
			    NDIS_802_11_ANTENNA	Antenna = *(NDIS_802_11_ANTENNA *)pData;

			    if (Antenna == 0) 
				    pAd->Antenna.field.TxDefaultAntenna = 1;    // ant-A
			    else if(Antenna == 1)
				    pAd->Antenna.field.TxDefaultAntenna = 2;    // ant-B
			    else
				    pAd->Antenna.field.TxDefaultAntenna = 0;    // diversity

			    pAd->PortCfg.BandState = UNKNOWN_BAND;
			    AsicAntennaSelect(pAd, pAd->LatchRfRegs.Channel);
			    DBGPRINT(RT_DEBUG_TRACE, "CMDHandler::OID_802_11_TX_ANTENNA_SELECTED (=%d)\n", Antenna);
            }
		    break;
#if 0
	        case RT_OID_802_11_QUERY_HARDWARE_REGISTER:
		        NdisStatus = RTUSBQueryHardWareRegister(pAd, pData);
		    break;

		    case RT_OID_802_11_SET_HARDWARE_REGISTER:
		        NdisStatus = RTUSBSetHardWareRegister(pAd, pData);
			break;
#endif
			case RT_OID_MULTI_READ_MAC:
	        {
			    USHORT	Offset = *((PUSHORT)pData);
			    USHORT	Length = *((PUSHORT)(pData + 2));
		        RTUSBMultiRead(pAd, Offset, pData + 4, Length);
		    }
		    break;

			case RT_OID_MULTI_WRITE_MAC:
	        {
		        USHORT	Offset = *((PUSHORT)pData);
			    USHORT	Length = *((PUSHORT)(pData + 2));
			    RTUSBMultiWrite(pAd, Offset, pData + 4, Length);
		    }
		    break;

			case RT_OID_USB_VENDOR_EEPROM_READ:
			{
				USHORT	Offset = *((PUSHORT)pData);
				USHORT	Length = *((PUSHORT)(pData + 2));
				RTUSBReadEEPROM(pAd, Offset, pData + 4, Length);
			}
			break;
				    
			case RT_OID_USB_VENDOR_EEPROM_WRITE:
			{
				USHORT	Offset = *((PUSHORT)pData);
#if 0
				USHORT	Length = *((PUSHORT)(pData + 2));
				RTUSBWriteEEPROM(pAd, Offset, pData + 4, Length);
#else//F/W restricts the max EEPROM write size to 62 bytes.
				USHORT	Residual = *((PUSHORT)(pData + 2));
				pData += 4;
				while (Residual > 62)
				{
				RTUSBWriteEEPROM(pAd, Offset, pData, 62);
				Offset += 62;
				Residual -= 62;
				pData += 62;
				}
				RTUSBWriteEEPROM(pAd, Offset, pData, Residual);
#endif
			}
			break;

			case RT_OID_USB_VENDOR_ENTER_TESTMODE:
			    RTUSB_VendorRequest(pAd,
					0,
					DEVICE_VENDOR_REQUEST_OUT,
					0x1,
					0x4,
					0x1,
					NULL,
					0);
					break;

			case RT_OID_USB_VENDOR_EXIT_TESTMODE:
				RTUSB_VendorRequest(pAd,
					0,
					DEVICE_VENDOR_REQUEST_OUT,
					0x1,
					0x4,
					0x0,
					NULL,
					0);
			break;
			case RT_OID_USB_RESET_BULK_OUT:
			{
				INT 	Index;
				
		        DBGPRINT_RAW(RT_DEBUG_ERROR, "RT_OID_USB_RESET_BULK_OUT\n");
				
				RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_RESET_PIPE_IN_PROGRESS);

				RTUSBRejectPendingPackets(pAd); //reject all NDIS packets waiting in TX queue						
				RTUSBCancelPendingBulkOutIRP(pAd);
				RTUSBCleanUpDataBulkOutQueue(pAd);

				NICInitializeAsic(pAd);
				ReleaseAdapter(pAd, FALSE, TRUE);   // unlink urb releated tx context
				NICInitTransmit(pAd);
				
				RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_RESET_PIPE_IN_PROGRESS); 
				
				if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BULKOUT_RESET))
				{
					RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_BULKOUT_RESET);
				}
				
				if (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF))
				{
					for (Index = 0; Index < 4; Index++)
					{
						if(pAd->SendTxWaitQueue[Index].Number > 0)
						{
							RTMPDeQueuePacket(pAd, Index);
						}
					}

					RTUSBKickBulkOut(pAd);
				}		
			}	 

    	    break;

			case RT_OID_USB_RESET_BULK_IN:
		    {
			    int	i;
				DBGPRINT_RAW(RT_DEBUG_ERROR, "!!!!!RT_OID_USB_RESET_BULK_IN\n");
				RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_RESET_PIPE_IN_PROGRESS);
				NICInitializeAsic(pAd);
				//RTUSBWriteMACRegister(pAd, TXRX_CSR0, 0x025eb032); // ??
				for (i = 0; i < RX_RING_SIZE; i++)
				{
					PRX_CONTEXT  pRxContext = &(pAd->RxContext[i]);

					if (pRxContext->pUrb != NULL)
					{
						RTUSB_UNLINK_URB(pRxContext->pUrb);
						usb_free_urb(pRxContext->pUrb);
						pRxContext->pUrb = NULL;
					}
					if (pRxContext->TransferBuffer != NULL)
					{
						kfree(pRxContext->TransferBuffer); 
						pRxContext->TransferBuffer = NULL;
					}
				}
				NICInitRecv(pAd);
				RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_RESET_PIPE_IN_PROGRESS);
				if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BULKIN_RESET))
				{
					RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_BULKIN_RESET);
				}

				if (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF))
				{
					RTUSBBulkReceive(pAd);
					RTUSBWriteMACRegister(pAd, TXRX_CSR0, 0x0276b032);  // enable RX of MAC block
				}
		    }
			break;

			case RT_OID_802_11_STA_CONFIG:
			{
				RT_802_11_STA_CONFIG *pStaConfig = (RT_802_11_STA_CONFIG *)pData;
				if (pStaConfig->EnableTxBurst != pAd->PortCfg.bEnableTxBurst)
				{
					pAd->PortCfg.bEnableTxBurst = (pStaConfig->EnableTxBurst == 1);
					//Currently Tx burst mode is only implemented in infrastructure mode.
					if (INFRA_ON(pAd))
					{
						if (pAd->PortCfg.bEnableTxBurst)
						{
							//Extend slot time if any encryption method is used to give ASIC more time to do encryption/decryption during Tx burst mode.
							if (pAd->PortCfg.WepStatus != Ndis802_11EncryptionDisabled)
							{
							// Nemo  RT2573USBWriteMACRegister_old(pAd, MAC_CSR10, 0x20);
							}
							//Set CWmin/CWmax to 0.
							// Nemo 2004    RT2573USBWriteMACRegister_old(pAd, MAC_CSR22, 0x100);
						}
						else
						{
							if (pAd->PortCfg.WepStatus != Ndis802_11EncryptionDisabled)
								AsicSetSlotTime(pAd, (BOOLEAN)pAd->PortCfg.UseShortSlotTime);
						// Nemo 2004    RT2573USBWriteMACRegister_old(pAd, MAC_CSR22, 0x53);
						}
					}
				}
				//pAd->PortCfg.EnableTurboRate = pStaConfig->EnableTurboRate;
				pAd->PortCfg.UseBGProtection = pStaConfig->UseBGProtection;
				//pAd->PortCfg.UseShortSlotTime = pStaConfig->UseShortSlotTime;
				pAd->PortCfg.UseShortSlotTime = 1; // 2003-10-30 always SHORT SLOT capable
				if (pAd->PortCfg.AdhocMode != pStaConfig->AdhocMode)
				{
					// allow dynamic change of "USE OFDM rate or not" in ADHOC mode
					// if setting changed, need to reset current TX rate as well as BEACON frame format
					pAd->PortCfg.AdhocMode = pStaConfig->AdhocMode;
					if (pAd->PortCfg.BssType == BSS_ADHOC)
					{
						MlmeUpdateTxRates(pAd, FALSE);
						MakeIbssBeacon(pAd);
						AsicEnableIbssSync(pAd);
					}
				}
				DBGPRINT(RT_DEBUG_TRACE, "CmdThread::RT_OID_802_11_SET_STA_CONFIG (Burst=%d,BGprot=%d,ShortSlot=%d,Adhoc=%d,Protection=%d\n",
					pStaConfig->EnableTxBurst,
					pStaConfig->UseBGProtection,
					pStaConfig->UseShortSlotTime,
					pStaConfig->AdhocMode,
					pAd->PortCfg.UseBGProtection);
			}
		    break;

			case RT_OID_SET_PSM_BIT_SAVE:
				MlmeSetPsmBit(pAd, PWR_SAVE);
				RTMPSendNullFrame(pAd, pAd->PortCfg.TxRate);
		    break;

		    case RT_OID_SET_RADIO:
			    if (pAd->PortCfg.bRadio == TRUE)
                {
				    MlmeRadioOn(pAd);
				    // Update extra information
				    pAd->ExtraInfo = EXTRA_INFO_CLEAR;
			    }
			    else
                {
			        MlmeRadioOff(pAd);
				    // Update extra information
    			    pAd->ExtraInfo = SW_RADIO_OFF;
    		    }
		    break;

			case RT_OID_RESET_FROM_ERROR:
			case RT_OID_RESET_FROM_NDIS:
			{
				UINT	i = 0;

				RTUSBRejectPendingPackets(pAd);//reject all NDIS packets waiting in TX queue
				RTUSBCleanUpDataBulkOutQueue(pAd);
				MlmeSuspend(pAd, FALSE);

				//Add code to access necessary registers here.
				//disable Rx
				RTUSBWriteMACRegister(pAd, TXRX_CSR2, 1);
				//Ask our device to complete any pending bulk in IRP.
				while ((atomic_read(&pAd->PendingRx) > 0) || 
                       (pAd->BulkOutPending[0] == TRUE) ||
					   (pAd->BulkOutPending[1] == TRUE) || 
					   (pAd->BulkOutPending[2] == TRUE) ||
					   (pAd->BulkOutPending[3] == TRUE))

				{
				    if (atomic_read(&pAd->PendingRx) > 0)
					{
						DBGPRINT_RAW(RT_DEBUG_TRACE, "BulkIn IRP Pending!!!\n");
						RTUSB_VendorRequest(pAd,
											0,
											DEVICE_VENDOR_REQUEST_OUT,
											0x0C,
											0x0,
											0x0,
											NULL,
											0);
					}

					if ((pAd->BulkOutPending[0] == TRUE) ||
						(pAd->BulkOutPending[1] == TRUE) || 
						(pAd->BulkOutPending[2] == TRUE) ||
						(pAd->BulkOutPending[3] == TRUE))
					{
						DBGPRINT_RAW(RT_DEBUG_TRACE, "BulkOut IRP Pending!!!\n");
						if (i == 0)
						{
							RTUSBCancelPendingBulkOutIRP(pAd);
							i++;
						}
					}

					RTMPusecDelay(500000);
				}

				NICResetFromError(pAd);            
				if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HARDWARE_ERROR))
				{
					RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_HARDWARE_ERROR);
				}
				if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BULKIN_RESET))
				{
					RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_BULKIN_RESET);
				}
				if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BULKOUT_RESET))
				{
					RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_BULKOUT_RESET);
				}

				RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS);

				if ((!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF)) &&
					(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS)) &&
					(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)))
				{
					MlmeResume(pAd);
					RTUSBBulkReceive(pAd);
					RTUSBWriteMACRegister(pAd, TXRX_CSR2, 0x7e);
				}
			}
			break;

			case RT_OID_LINK_DOWN:
				DBGPRINT_RAW(RT_DEBUG_TRACE, "LinkDown(RT_OID_LINK_DOWN)\n");
				LinkDown(pAd, TRUE);
			break;

			case RT_OID_VENDOR_WRITE_BBP:
			{
				UCHAR	Offset, Value;
				Offset = *((PUCHAR)pData);
				Value = *((PUCHAR)(pData + 1));
				DBGPRINT_RAW(RT_DEBUG_INFO, "offset = 0x%02x	value = 0x%02x\n", Offset, Value);
				RTUSBWriteBBPRegister(pAd, Offset, Value);
			}
			break;

			case RT_OID_VENDOR_READ_BBP:
			{
				UCHAR	Offset = *((PUCHAR)pData);
				PUCHAR	pValue = (PUCHAR)(pData + 1);

				DBGPRINT_RAW(RT_DEBUG_INFO, "offset = 0x%02x\n", Offset);
				RTUSBReadBBPRegister(pAd, Offset, pValue);
				DBGPRINT_RAW(RT_DEBUG_INFO, "value = 0x%02x\n", *pValue);
			}
			break;

			case RT_OID_VENDOR_WRITE_RF:
			{
				ULONG	Value = *((PULONG)pData);
        	
				DBGPRINT_RAW(RT_DEBUG_INFO, "value = 0x%08x\n", Value);
				RTUSBWriteRFRegister(pAd, Value);
			}
			break;
			    
			case RT_OID_802_11_RESET_COUNTERS:
			{
				UCHAR	Value[22];

				RTUSBMultiRead(pAd, STA_CSR0, Value, 24);
			}
			break;

			case RT_OID_USB_VENDOR_RESET:
				RTUSB_VendorRequest(pAd,
									0,
									DEVICE_VENDOR_REQUEST_OUT,
									1,
									1,
									0,
									NULL,
									0);
			break;

			case RT_OID_USB_VENDOR_UNPLUG:
				RTUSB_VendorRequest(pAd,
									0,
									DEVICE_VENDOR_REQUEST_OUT,
									1,
									2,
									0,
									NULL,
									0);
			break;
#if 0
			case RT_OID_USB_VENDOR_SWITCH_FUNCTION:
				RTUSBWriteMACRegister(pAd, MAC_CSR13, 0x2121);
				RTUSBWriteMACRegister(pAd, MAC_CSR14, 0x1e1e);
				RTUSBWriteMACRegister(pAd, MAC_CSR1, 3);
				RTUSBWriteMACRegister(pAd, PHY_CSR4, 0xf);

				RTUSB_VendorRequest(pAd,
									0,
									DEVICE_VENDOR_REQUEST_OUT,
									1,
									3,
									0,
									NULL,
									0);
			break;
#endif			
			case RT_OID_VENDOR_FLIP_IQ:
			{
				ULONG	Value1, Value2;
				RTUSBReadMACRegister(pAd, PHY_CSR5, &Value1);
				RTUSBReadMACRegister(pAd, PHY_CSR6, &Value2);
				if (*pData == 1)
				{
					DBGPRINT_RAW(RT_DEBUG_INFO, "I/Q Flip\n");
					Value1 = Value1 | 0x0004;
					Value2 = Value2 | 0x0004;
				}
				else
				{
					DBGPRINT_RAW(RT_DEBUG_INFO, "I/Q Not Flip\n");
					Value1 = Value1 & 0xFFFB;
					Value2 = Value2 & 0xFFFB;
				}
				RTUSBWriteMACRegister(pAd, PHY_CSR5, Value1);
				RTUSBWriteMACRegister(pAd, PHY_CSR6, Value2);
			}
			break;

			case RT_OID_UPDATE_TX_RATE:
				MlmeUpdateTxRates(pAd, FALSE);
				if (ADHOC_ON(pAd))
					MakeIbssBeacon(pAd);
			break;

			case RT_OID_802_11_PREAMBLE:
			{
				ULONG	Preamble = *((PULONG)(cmdqelmt->buffer));
				if (Preamble == Rt802_11PreambleShort)
				{
					pAd->PortCfg.TxPreamble = Preamble;
					MlmeSetTxPreamble(pAd, Rt802_11PreambleShort);
				}
				else if ((Preamble == Rt802_11PreambleLong) || (Preamble == Rt802_11PreambleAuto))
				{
					// if user wants AUTO, initialize to LONG here, then change according to AP's
					// capability upon association.
					pAd->PortCfg.TxPreamble = Preamble;
					MlmeSetTxPreamble(pAd, Rt802_11PreambleLong);
				}
				else
					NdisStatus = NDIS_STATUS_FAILURE;
				DBGPRINT(RT_DEBUG_ERROR, "CMDHandler::RT_OID_802_11_SET_PREAMBLE (=%d)\n", Preamble);
			}
			break;
			case OID_802_11_NETWORK_TYPE_IN_USE:
			{
				NDIS_802_11_NETWORK_TYPE	NetType = *(PNDIS_802_11_NETWORK_TYPE)(cmdqelmt->buffer);
				if (NetType == Ndis802_11DS)
					RTMPSetPhyMode(pAd, PHY_11B);
				else if (NetType == Ndis802_11OFDM24)
					RTMPSetPhyMode(pAd, PHY_11BG_MIXED);
				else if (NetType == Ndis802_11OFDM5)
					RTMPSetPhyMode(pAd, PHY_11A);
				else
					NdisStatus = NDIS_STATUS_FAILURE;
				DBGPRINT(RT_DEBUG_ERROR, "CMDHandler::OID_802_11_NETWORK_TYPE_IN_USE (=%d)\n",NetType);
			
            }
            break;
			case RT_OID_802_11_PHY_MODE:
				{
					ULONG	phymode = *(ULONG *)(cmdqelmt->buffer);
					RTMPSetPhyMode(pAd, phymode);
					DBGPRINT(RT_DEBUG_ERROR, "CMDHandler::RT_OID_802_11_SET_PHY_MODE (=%d)\n", phymode);
				}
			break;

#if 0
			case OID_802_11_WEP_STATUS:
				{
					USHORT	Value;
					NDIS_802_11_WEP_STATUS	WepStatus = *(PNDIS_802_11_WEP_STATUS)pData;

					DBGPRINT(RT_DEBUG_ERROR, "CMDHandler::66- OID_802_11_WEP_STATUS  \n");
			break;

					if (pAd->PortCfg.WepStatus != WepStatus)
					{
						DBGPRINT(RT_DEBUG_ERROR, "Config Changed !!status= %x  \n", WepStatus);

						// Config has changed
						pAd->bConfigChanged = TRUE;
					}
					pAd->PortCfg.WepStatus   = WepStatus;
					pAd->PortCfg.PairCipher  = WepStatus;
				    pAd->PortCfg.GroupCipher = WepStatus;

#if 1
					if ((WepStatus == Ndis802_11Encryption1Enabled) && 
						(pAd->SharedKey[pAd->PortCfg.DefaultKeyId].KeyLen != 0))
					{
						if (pAd->SharedKey[pAd->PortCfg.DefaultKeyId].KeyLen <= 5)
						{
							DBGPRINT(RT_DEBUG_ERROR, "WEP64!  \n");

							pAd->SharedKey[pAd->PortCfg.DefaultKeyId].CipherAlg = CIPHER_WEP64;
						}
						else
						{
							DBGPRINT(RT_DEBUG_ERROR, "WEP128!  \n");

							pAd->SharedKey[pAd->PortCfg.DefaultKeyId].CipherAlg = CIPHER_WEP128;
						}
#if 0	    
						RTUSBReadMACRegister_old(pAd, TXRX_CSR0, &Value);
						Value &= 0xfe00;
						Value |= ((LENGTH_802_11 << 3) | (pAd->PortCfg.CipherAlg));
						RTUSBWriteMACRegister_old(pAd, TXRX_CSR0, Value);
#endif
					}
					else if (WepStatus == Ndis802_11Encryption2Enabled)
					{
						DBGPRINT(RT_DEBUG_ERROR, " TKIP !!!  \n");

						pAd->SharedKey[pAd->PortCfg.DefaultKeyId].CipherAlg = CIPHER_TKIP;
#if 0
						RTUSBReadMACRegister_old(pAd, TXRX_CSR0, &Value);
						Value &= 0xfe00;
						Value |= ((LENGTH_802_11 << 3) | (pAd->PortCfg.CipherAlg));
						RTUSBWriteMACRegister_old(pAd, TXRX_CSR0, Value);
#endif
					}
					else if (WepStatus == Ndis802_11Encryption3Enabled)
					{
						DBGPRINT(RT_DEBUG_ERROR, " AES  !!!  \n");
						pAd->SharedKey[pAd->PortCfg.DefaultKeyId].CipherAlg = CIPHER_AES;
#if 0
						RTUSBReadMACRegister_old(pAd, TXRX_CSR0, &Value);
						Value &= 0xfe00;
						Value |= ((LENGTH_802_11 << 3) | (pAd->PortCfg.CipherAlg));
						RTUSBWriteMACRegister_old(pAd, TXRX_CSR0, Value);
#endif
					}
					else if (WepStatus == Ndis802_11EncryptionDisabled)
					{
						DBGPRINT(RT_DEBUG_ERROR, " CIPHER_NONE  !!!  \n");

						pAd->SharedKey[pAd->PortCfg.DefaultKeyId].CipherAlg = CIPHER_NONE;
#if 0
						RTUSBReadMACRegister_old(pAd, TXRX_CSR0, &Value);
						Value &= 0xfe00;
						RTUSBWriteMACRegister_old(pAd, TXRX_CSR0, Value);
#endif
					}else 
					{
						DBGPRINT(RT_DEBUG_ERROR, " ERROR Cipher   !!!  \n");
					}
#endif
				}
			break;
#endif
			case OID_802_11_ADD_WEP:
			{
				ULONG	KeyIdx;
				PNDIS_802_11_WEP	pWepKey;

				DBGPRINT(RT_DEBUG_TRACE, "CMDHandler::OID_802_11_ADD_WEP  \n");
				
				pWepKey = (PNDIS_802_11_WEP)pData;
				KeyIdx = pWepKey->KeyIndex & 0x0fffffff;

				// it is a shared key
				if ((KeyIdx >= 4) || ((pWepKey->KeyLength != 5) && (pWepKey->KeyLength != 13)))
				{
					NdisStatus = NDIS_STATUS_FAILURE;
					DBGPRINT(RT_DEBUG_ERROR, "CMDHandler::OID_802_11_ADD_WEP, INVALID_DATA!!\n");
				}
				else 
				{
					UCHAR CipherAlg;
					pAd->SharedKey[KeyIdx].KeyLen = (UCHAR) pWepKey->KeyLength;
					NdisMoveMemory(pAd->SharedKey[KeyIdx].Key, &pWepKey->KeyMaterial, pWepKey->KeyLength);
					CipherAlg = (pAd->SharedKey[KeyIdx].KeyLen == 5)? CIPHER_WEP64 : CIPHER_WEP128;
					pAd->SharedKey[KeyIdx].CipherAlg = CipherAlg;
					if (pWepKey->KeyIndex & 0x80000000)
					{
						// Default key for tx (shared key)
						pAd->PortCfg.DefaultKeyId = (UCHAR) KeyIdx;
					}							
					AsicAddSharedKeyEntry(pAd, 0, (UCHAR)KeyIdx, CipherAlg, pWepKey->KeyMaterial, NULL, NULL);
					DBGPRINT(RT_DEBUG_TRACE, "CMDHandler::OID_802_11_ADD_WEP (KeyIdx=%d, Len=%d-byte)\n", KeyIdx, pWepKey->KeyLength);
				}
			}
			break;
			    
			case OID_802_11_REMOVE_WEP:
			{
				ULONG		KeyIdx;

				
				KeyIdx = *(NDIS_802_11_KEY_INDEX *) pData;
				if (KeyIdx & 0x80000000)
				{
					NdisStatus = NDIS_STATUS_FAILURE;
					DBGPRINT(RT_DEBUG_ERROR, "CMDHandler::OID_802_11_REMOVE_WEP, INVALID_DATA!!\n");
				}
				else
				{
					KeyIdx = KeyIdx & 0x0fffffff;
					if (KeyIdx >= 4)
					{
						NdisStatus = NDIS_STATUS_FAILURE;
						DBGPRINT(RT_DEBUG_ERROR, "CMDHandler::OID_802_11_REMOVE_WEP, Invalid KeyIdx[=%d]!!\n", KeyIdx);
					}
					else
					{
						pAd->SharedKey[KeyIdx].KeyLen = 0;
						pAd->SharedKey[KeyIdx].CipherAlg = CIPHER_NONE;
						AsicRemoveSharedKeyEntry(pAd, 0, (UCHAR)KeyIdx);
						DBGPRINT(RT_DEBUG_TRACE, "CMDHandler::OID_802_11_REMOVE_WEP (KeyIdx=%d)\n", KeyIdx);
					}
				}	
			}
			break;

			case OID_802_11_ADD_KEY_WEP:
			{
				PNDIS_802_11_KEY		pKey;
				ULONG					i, KeyIdx;						

				pKey = (PNDIS_802_11_KEY) pData;
				KeyIdx = pKey->KeyIndex & 0x0fffffff;

				// it is a shared key
			    if (KeyIdx >= 4)
				{
			        NdisStatus = NDIS_STATUS_FAILURE;
			        DBGPRINT(RT_DEBUG_ERROR, "CMDHandler::OID_802_11_ADD_KEY_WEP, Invalid KeyIdx[=%d]!!\n", KeyIdx);
			    }
			    else 
			    {
			        UCHAR CipherAlg;
					 
			        pAd->SharedKey[KeyIdx].KeyLen = (UCHAR) pKey->KeyLength;
			        NdisMoveMemory(pAd->SharedKey[KeyIdx].Key, &pKey->KeyMaterial, pKey->KeyLength);

			        if (pKey->KeyLength == 5)
				        CipherAlg = CIPHER_WEP64;
				    else
				        CipherAlg = CIPHER_WEP128;

				    // always expand the KEY to 16-byte here for efficiency sake. so that in case CKIP is used
				    // sometime later we don't have to do key expansion for each TX in RTUSBHardTransmit().
				    // However, we shouldn't change pAd->SharedKey[BSS0][KeyIdx].KeyLen
				    if (pKey->KeyLength < 16)
				    {
				        for(i = 1; i < (16 / pKey->KeyLength); i++)
				        {
				            NdisMoveMemory(&pAd->SharedKey[KeyIdx].Key[i * pKey->KeyLength], 
										   &pKey->KeyMaterial[0], 
										   pKey->KeyLength);
				        }
					    NdisMoveMemory(&pAd->SharedKey[KeyIdx].Key[i * pKey->KeyLength], 
									   &pKey->KeyMaterial[0], 
									   16 - (i * pKey->KeyLength));
				    }

				    pAd->SharedKey[KeyIdx].CipherAlg = CipherAlg;
				    if (pKey->KeyIndex & 0x80000000)
					{
				        // Default key for tx (shared key)
					    pAd->PortCfg.DefaultKeyId = (UCHAR) KeyIdx;						
				    }

				    AsicAddSharedKeyEntry(pAd, 0, (UCHAR)KeyIdx, CipherAlg, pAd->SharedKey[KeyIdx].Key, NULL, NULL);
				    DBGPRINT(RT_DEBUG_TRACE, "CMDHandler::OID_802_11_ADD_KEY_WEP (KeyIdx=%d, KeyLen=%d, CipherAlg=%d)\n", 
				        pAd->PortCfg.DefaultKeyId, pAd->SharedKey[KeyIdx].KeyLen, pAd->SharedKey[KeyIdx].CipherAlg);
				}
			}
			break;

			case OID_802_11_ADD_KEY:
			{  
                PNDIS_802_11_KEY	pkey = (PNDIS_802_11_KEY)pData;
                
				NdisStatus = RTMPWPAAddKeyProc(pAd, pkey);
				RTUSBBulkReceive(pAd);
				DBGPRINT(RT_DEBUG_TRACE, "CMDHandler::OID_802_11_ADD_KEY\n");
			}
			break;

#if 0
			case RT_OID_802_11_REMOVE_WEP:
			case OID_802_11_REMOVE_WEP:
			{
				ULONG  KeyIdx;

				
				KeyIdx = *(NDIS_802_11_KEY_INDEX *) pData;
				if (KeyIdx & 0x80000000)
				{
					NdisStatus = NDIS_STATUS_FAILURE;
					DBGPRINT(RT_DEBUG_ERROR, "CMDHandler::OID_802_11_REMOVE_WEP, INVALID_DATA!!\n");
				}
				else
				{
					KeyIdx = KeyIdx & 0x0fffffff;
					if (KeyIdx >= 4)
					{
						NdisStatus = NDIS_STATUS_FAILURE;
						DBGPRINT(RT_DEBUG_ERROR, "CMDHandler::OID_802_11_REMOVE_WEP, INVALID_DATA!!\n");
					}
						else
					{

					}
				}
			}
			break;
#if 0				
			{
				//PNDIS_802_11_REMOVE_KEY  pRemoveKey;
				ULONG  KeyIdx;
				//pRemoveKey = (PNDIS_802_11_REMOVE_KEY) pData;
				//KeyIdx = pRemoveKey->KeyIndex;


				DBGPRINT(RT_DEBUG_ERROR, "CMDHandler::OID_802_11_REMOVE_WEP\n");
				//if (InformationBufferLength != sizeof(NDIS_802_11_KEY_INDEX))
				//	Status = NDIS_STATUS_INVALID_LENGTH;
				//else 
				{
					KeyIdx = *(NDIS_802_11_KEY_INDEX *) pData;

					if (KeyIdx & 0x80000000)
					{
						// Should never set default bit when remove key
						//Status = NDIS_STATUS_INVALID_DATA;
					}
					else
					{
						KeyIdx = KeyIdx & 0x0fffffff;
						if (KeyIdx >= 4)
						{
							//Status = NDIS_STATUS_INVALID_DATA;
						}
						else
						{
							pAd->SharedKey[KeyIdx].KeyLen = 0;
							//Status = RT2573USBEnqueueCmdFromNdis(pAd, OID_802_11_REMOVE_WEP, TRUE, pInformationBuffer, InformationBufferLength);

							AsicRemoveSharedKeyEntry(pAd, 0, (UCHAR)KeyIdx);
						}
					}
				}
			}
			break;
#endif
#endif
			case OID_802_11_REMOVE_KEY:
			{
				PNDIS_802_11_REMOVE_KEY  pRemoveKey;
				ULONG  KeyIdx;
				
				pRemoveKey = (PNDIS_802_11_REMOVE_KEY) pData;
#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT
				if (pAd->PortCfg.bNativeWpa == TRUE)  // add by johnli
				{
					if(pRemoveKey->KeyIndex == 0xffffffff)
					{
						// Do remove all sharekey table.
						//printk("Remove all AsicShareKey table only. Keep PortCfg.keyInfo!\n");
						for (KeyIdx =0 ; KeyIdx < SHARE_KEY_NUM; KeyIdx++)
							AsicRemoveSharedKeyEntry(pAd, 0, (UCHAR)KeyIdx);	
					}
				}
#endif
				if (pAd->PortCfg.AuthMode >= Ndis802_11AuthModeWPA)
				{
					NdisStatus = RTMPWPARemoveKeyProc(pAd, pData);
					DBGPRINT(RT_DEBUG_TRACE, "CMDHandler::RTMPWPARemoveKeyProc\n");
				}
				else 
				{
					KeyIdx = pRemoveKey->KeyIndex;
						
					if (KeyIdx & 0x80000000)
					{
						// Should never set default bit when remove key
						NdisStatus = NDIS_STATUS_FAILURE;
						DBGPRINT(RT_DEBUG_ERROR, "CMDHandler::OID_802_11_REMOVE_KEY, Invalid KeyIdx[=%d]!!\n", KeyIdx);
					}
					else
					{
						KeyIdx = KeyIdx & 0x0fffffff;
						if (KeyIdx >= 4)
						{
							NdisStatus = NDIS_STATUS_FAILURE;
							DBGPRINT(RT_DEBUG_ERROR, "CMDHandler::OID_802_11_REMOVE_KEY, Invalid KeyIdx[=%d]!!\n", KeyIdx);										
						}
						else
						{
							pAd->SharedKey[KeyIdx].KeyLen = 0;
							pAd->SharedKey[KeyIdx].CipherAlg = CIPHER_NONE;
							AsicRemoveSharedKeyEntry(pAd, 0, (UCHAR)KeyIdx);
							DBGPRINT(RT_DEBUG_TRACE, "CMDHandler::AsicRemoveSharedKeyEntry(KeyIdx=%d)\n", KeyIdx);
						}
					}
				}

			}
			break;
				
			case OID_802_11_POWER_MODE:
			{
				NDIS_802_11_POWER_MODE PowerMode = *(PNDIS_802_11_POWER_MODE) pData;
				DBGPRINT(RT_DEBUG_TRACE, "CMDHandler::OID_802_11_POWER_MODE (=%d)\n",PowerMode);
				
				// save user's policy here, but not change PortCfg.Psm immediately
				if (PowerMode == Ndis802_11PowerModeCAM) 
				{
					// clear PSM bit immediately
					MlmeSetPsmBit(pAd, PWR_ACTIVE);
		        
					OPSTATUS_SET_FLAG(pAd, fOP_STATUS_RECEIVE_DTIM); 
					if (pAd->PortCfg.bWindowsACCAMEnable == FALSE)
						pAd->PortCfg.WindowsPowerMode = PowerMode;
					pAd->PortCfg.WindowsBatteryPowerMode = PowerMode;
				} 
				else if (PowerMode == Ndis802_11PowerModeMAX_PSP) 
				{
					// do NOT turn on PSM bit here, wait until MlmeCheckPsmChange()
					// to exclude certain situations.
					//     MlmeSetPsmBit(pAd, PWR_SAVE);
					if (pAd->PortCfg.bWindowsACCAMEnable == FALSE)
						pAd->PortCfg.WindowsPowerMode = PowerMode;
					pAd->PortCfg.WindowsBatteryPowerMode = PowerMode;
					OPSTATUS_SET_FLAG(pAd, fOP_STATUS_RECEIVE_DTIM); 
					pAd->PortCfg.DefaultListenCount = 5;
				} 
				else if (PowerMode == Ndis802_11PowerModeFast_PSP) 
				{
					// do NOT turn on PSM bit here, wait until MlmeCheckPsmChange()
					// to exclude certain situations.
					//     MlmeSetPsmBit(pAd, PWR_SAVE);
					OPSTATUS_SET_FLAG(pAd, fOP_STATUS_RECEIVE_DTIM);
					if (pAd->PortCfg.bWindowsACCAMEnable == FALSE)
						pAd->PortCfg.WindowsPowerMode = PowerMode;
					pAd->PortCfg.WindowsBatteryPowerMode = PowerMode;
					pAd->PortCfg.DefaultListenCount = 3;
				} 
			}					
			break;

			case RT_PERFORM_SOFT_DIVERSITY:
				AsicRxAntEvalAction(pAd);
			break;

		    case RT_OID_FORCE_WAKE_UP:
			    AsicForceWakeup(pAd);
			break;

		    case RT_OID_SET_PSM_BIT_ACTIVE:
			    MlmeSetPsmBit(pAd, PWR_ACTIVE);
		    break;

			default:
			break;
		}

    
		if (cmdqelmt->CmdFromNdis == TRUE)
		{
			if ((cmdqelmt->command != OID_802_11_BSSID_LIST_SCAN) &&
				(cmdqelmt->command != RT_OID_802_11_BSSID) &&
				(cmdqelmt->command != OID_802_11_SSID) &&
				(cmdqelmt->command != OID_802_11_DISASSOCIATE))
			{
			}

			if ((cmdqelmt->command != RT_OID_MULTI_READ_MAC) &&
				(cmdqelmt->command != RT_OID_VENDOR_READ_BBP) &&
#ifdef DBG					
				(cmdqelmt->command != RT_OID_802_11_QUERY_HARDWARE_REGISTER) &&
#endif					
				(cmdqelmt->command != RT_OID_USB_VENDOR_EEPROM_READ))
			{
				if (cmdqelmt->buffer != NULL)
			        kfree(cmdqelmt->buffer);
			}
			
			kfree((PCmdQElmt)cmdqelmt);
		}
		else
            cmdqelmt->InUse = FALSE;
            
	}
}

#ifdef DBG
#define HARDWARE_MAC	0
#define HARDWARE_BBP	1
#define HARDWARE_RF		2
NDIS_STATUS     RTUSBQueryHardWareRegister(
	IN	PRTMP_ADAPTER	pAd,
	IN	PVOID			pBuf)
{
	PRT_802_11_HARDWARE_REGISTER	pHardwareRegister;
	ULONG							Value;
	USHORT							Offset;
	UCHAR							bbpValue;
	UCHAR							bbpID;
	NDIS_STATUS						Status = NDIS_STATUS_SUCCESS;	

	pHardwareRegister = (PRT_802_11_HARDWARE_REGISTER) pBuf;

	if (pHardwareRegister->HardwareType == HARDWARE_MAC)
	{
		//Check Offset is valid?
		if (pHardwareRegister->Offset > 0xF4)
			Status = NDIS_STATUS_FAILURE;
		
		Offset = (USHORT) pHardwareRegister->Offset;
		RTUSBReadMACRegister(pAd, Offset, &Value);
		pHardwareRegister->Data = Value;
		DBGPRINT(RT_DEBUG_TRACE, "MAC:Offset[0x%04x]=[0x%04x]\n", Offset, Value);
	}
	else if (pHardwareRegister->HardwareType == HARDWARE_BBP)
	{
		bbpID = (UCHAR) pHardwareRegister->Offset;
		
		RTUSBReadBBPRegister(pAd, bbpID, &bbpValue);		
		pHardwareRegister->Data = bbpValue;
		DBGPRINT(RT_DEBUG_TRACE, "BBP:ID[0x%02x]=[0x%02x]\n", bbpID, bbpValue);		
	}
	else
		Status = NDIS_STATUS_FAILURE;
	
	return Status;
}

NDIS_STATUS     RTUSBSetHardWareRegister(
	IN	PRTMP_ADAPTER	pAd,
	IN	PVOID			pBuf)
{
	PRT_802_11_HARDWARE_REGISTER	pHardwareRegister;
	ULONG							Value;
	USHORT							Offset;
	UCHAR							bbpValue;
	UCHAR							bbpID;
	NDIS_STATUS						Status = NDIS_STATUS_SUCCESS;	

	pHardwareRegister = (PRT_802_11_HARDWARE_REGISTER) pBuf;

	if (pHardwareRegister->HardwareType == HARDWARE_MAC)
	{
		//Check Offset is valid?
		if (pHardwareRegister->Offset > 0xF4)
			Status = NDIS_STATUS_FAILURE;

		Offset = (USHORT) pHardwareRegister->Offset;
		Value = (ULONG) pHardwareRegister->Data;
		RTUSBWriteMACRegister(pAd, Offset, Value);		
		DBGPRINT(RT_DEBUG_TRACE, "RT_OID_802_11_SET_HARDWARE_REGISTER (MAC offset=0x%08x, data=0x%08x)\n", pHardwareRegister->Offset, pHardwareRegister->Data);

		// 2004-11-08 a special 16-byte on-chip memory is used for RaConfig to pass debugging parameters to driver
		// for debug-tuning only
		if ((pHardwareRegister->Offset >= HW_DEBUG_SETTING_BASE) && 
			(pHardwareRegister->Offset <= HW_DEBUG_SETTING_END))
		{
			// 0x2bf0: test power-saving feature
			if (pHardwareRegister->Offset == HW_DEBUG_SETTING_BASE)
			{
#if 0			
				ULONG isr, imr, gimr;
				USHORT tbtt = 3;

				RTMP_IO_READ32(pAd, MCU_INT_SOURCE_CSR, &isr);
				RTMP_IO_READ32(pAd, MCU_INT_MASK_CSR, &imr);
				RTMP_IO_READ32(pAd, INT_MASK_CSR, &gimr);
				DBGPRINT(RT_DEBUG_TRACE, "Sleep %d TBTT, 8051 IMR=%08x, ISR=%08x, MAC IMR=%08x\n", tbtt, imr, isr, gimr);
				AsicSleepThenAutoWakeup(pAd, tbtt);	
#endif				
			}
			// 0x2bf4: test H2M_MAILBOX. byte3: Host command, byte2: token, byte1-0: arguments
			else if (pHardwareRegister->Offset == (HW_DEBUG_SETTING_BASE + 4))
			{
				// 0x2bf4: byte0 non-zero: enable R17 tuning, 0: disable R17 tuning
				if (pHardwareRegister->Data & 0x000000ff) 
				{
					pAd->BbpTuning.bEnable = TRUE;
					DBGPRINT(RT_DEBUG_TRACE,"turn on R17 tuning\n");
				}
				else
				{
					UCHAR R17;

					pAd->BbpTuning.bEnable = FALSE;
					if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
					{
						if (pAd->PortCfg.Channel > 14)
							R17 = pAd->BbpTuning.R17LowerBoundA;
						else
							R17 = pAd->BbpTuning.R17LowerBoundG;
						RTUSBWriteBBPRegister(pAd, 17, R17);
						DBGPRINT(RT_DEBUG_TRACE,"turn off R17 tuning, restore to 0x%02x\n", R17);
					}
				}
			}
			// 0x2bf8: test ACK policy and QOS format in ADHOC mode
			else if (pHardwareRegister->Offset == (HW_DEBUG_SETTING_BASE + 8))
			{
				PUCHAR pAckStr[4] = {"NORMAL", "NO-ACK", "NO-EXPLICIT-ACK", "BLOCK-ACK"};
				EDCA_PARM DefaultEdcaParm;

				// byte0 b1-0 means ACK POLICY - 0: normal ACK, 1: no ACK, 2:no explicit ACK, 3:BA
				pAd->PortCfg.AckPolicy[0] = ((UCHAR)pHardwareRegister->Data & 0x02) << 5;
				pAd->PortCfg.AckPolicy[1] = ((UCHAR)pHardwareRegister->Data & 0x02) << 5;
				pAd->PortCfg.AckPolicy[2] = ((UCHAR)pHardwareRegister->Data & 0x02) << 5;
				pAd->PortCfg.AckPolicy[3] = ((UCHAR)pHardwareRegister->Data & 0x02) << 5;
				DBGPRINT(RT_DEBUG_TRACE, "ACK policy = %s\n", pAckStr[(UCHAR)pHardwareRegister->Data & 0x02]);

				// any non-ZERO value in byte1 turn on EDCA & QOS format
				if (pHardwareRegister->Data & 0x0000ff00) 
				{
					NdisZeroMemory(&DefaultEdcaParm, sizeof(EDCA_PARM));
					DefaultEdcaParm.bValid = TRUE;
					DefaultEdcaParm.Aifsn[0] = 3;
					DefaultEdcaParm.Aifsn[1] = 7;
					DefaultEdcaParm.Aifsn[2] = 2;
					DefaultEdcaParm.Aifsn[3] = 2;

					DefaultEdcaParm.Cwmin[0] = 4;
					DefaultEdcaParm.Cwmin[1] = 4;
					DefaultEdcaParm.Cwmin[2] = 3;
					DefaultEdcaParm.Cwmin[3] = 2;

					DefaultEdcaParm.Cwmax[0] = 10;
					DefaultEdcaParm.Cwmax[1] = 10;
					DefaultEdcaParm.Cwmax[2] = 4;
					DefaultEdcaParm.Cwmax[3] = 3;

					DefaultEdcaParm.Txop[0]  = 0;
					DefaultEdcaParm.Txop[1]  = 0;
					DefaultEdcaParm.Txop[2]  = 96;
					DefaultEdcaParm.Txop[3]  = 48;
					AsicSetEdcaParm(pAd, &DefaultEdcaParm);
				}
				else
					AsicSetEdcaParm(pAd, NULL);
			}
			// 0x2bfc: turn ON/OFF TX aggregation
			else if (pHardwareRegister->Offset == (HW_DEBUG_SETTING_BASE + 12))
			{
				if (pHardwareRegister->Data)
					OPSTATUS_SET_FLAG(pAd, fOP_STATUS_AGGREGATION_INUSED);
				else
					OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_AGGREGATION_INUSED);
				DBGPRINT(RT_DEBUG_TRACE, "AGGREGATION = %d\n", OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_AGGREGATION_INUSED));
			}
			else
				Status = NDIS_STATUS_FAILURE;				
		}
	}
	else if (pHardwareRegister->HardwareType == HARDWARE_BBP)
	{
		bbpID = (UCHAR) pHardwareRegister->Offset;
		bbpValue = (UCHAR) pHardwareRegister->Data;
		RTUSBWriteBBPRegister(pAd, bbpID, bbpValue);
		DBGPRINT(RT_DEBUG_TRACE, "BBP:ID[0x%02x]=[0x%02x]\n", bbpID, bbpValue);		
	}

	return Status;
}
#endif

