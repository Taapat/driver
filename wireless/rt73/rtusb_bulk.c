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
	rtusb_bulk.c

	Abstract:

	Revision History:
	Who			When		What
	--------	----------	----------------------------------------------
	Name		Date		Modification logs
	Paul Lin	06-25-2004	created
	
*/

#include	"rt_config.h"


void RTusb_fill_bulk_urb (struct urb *pUrb,
	struct usb_device *pUsb_Dev,
	unsigned int bulkpipe,
	void *pTransferBuf,
	int BufSize,
	usb_complete_t Complete,
	void *pContext)
{

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	usb_fill_bulk_urb(pUrb, pUsb_Dev, bulkpipe, pTransferBuf, BufSize, Complete, pContext);	
#else
	FILL_BULK_URB(pUrb, pUsb_Dev, bulkpipe, pTransferBuf, BufSize, Complete, pContext);
#endif

}

// ************************ Completion Func ************************ //
VOID RTUSBBulkOutDataPacketComplete(purbb_t pUrb, struct pt_regs *pt_regs)
{
	PTX_CONTEXT 	pTxContext;
	PRTMP_ADAPTER	pAd;
	NTSTATUS		status;
	UCHAR			BulkOutPipeId;
	unsigned long	IrqFlags;
	
	DBGPRINT_RAW(RT_DEBUG_INFO, "--->RTUSBBulkOutDataPacketComplete\n");
	pTxContext= (PTX_CONTEXT)pUrb->context;
	pAd = pTxContext->pAd;
	status = pUrb->status;

	// Store BulkOut PipeId
	BulkOutPipeId = pTxContext->BulkOutPipeId;
	pAd->BulkOutDataOneSecCount++;
	
	if (status == USB_ST_NOERROR)
	{	
		DBGPRINT_RAW(RT_DEBUG_INFO, "BulkOutDataPacketComplete %d (STATUS_SUCCESS)\n", BulkOutPipeId);

		if (pTxContext->LastOne == TRUE)
		{
			pAd->Counters.GoodTransmits++;
			FREE_TX_RING(pAd, BulkOutPipeId, pTxContext);
            pAd->TxRingTotalNumber[BulkOutPipeId]--;    // sync. to TxCount

			if (pAd->SendTxWaitQueue[BulkOutPipeId].Number > 0)
			{
				RTMPDeQueuePacket(pAd, BulkOutPipeId);
			}
		}
		else
		{
			if ((!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS)) &&
				(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS)) &&
				(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)) &&
				(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BULKOUT_RESET)))
			{
				FREE_TX_RING(pAd, BulkOutPipeId, pTxContext);
				pAd->TxRingTotalNumber[BulkOutPipeId]--;    // sync. to TxCount
				// Indicate next one is frag data which has highest priority
				RTUSB_SET_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_FRAG << BulkOutPipeId));
			}
			else
			{
				while (pTxContext->LastOne != TRUE)
				{
					FREE_TX_RING(pAd, BulkOutPipeId, pTxContext);
					pAd->TxRingTotalNumber[BulkOutPipeId]--;    // sync. to TxCount
					pTxContext = &(pAd->TxContext[BulkOutPipeId][pAd->NextBulkOutIndex[BulkOutPipeId]]);
				}
				
				FREE_TX_RING(pAd, BulkOutPipeId, pTxContext);
				pAd->TxRingTotalNumber[BulkOutPipeId]--;    // sync. to TxCount
			}
		}
	}
#if 1	// STATUS_OTHER
	else
	{
		DBGPRINT_RAW(RT_DEBUG_ERROR, "BulkOutDataPacketComplete %d (STATUS_OTHER)\n", BulkOutPipeId);

		while (pTxContext->LastOne != TRUE)
		{
			FREE_TX_RING(pAd, BulkOutPipeId, pTxContext);
			pAd->TxRingTotalNumber[BulkOutPipeId]--;    // sync. to TxCount
			pTxContext = &(pAd->TxContext[BulkOutPipeId][pAd->NextBulkOutIndex[BulkOutPipeId]]);
		}
		FREE_TX_RING(pAd, BulkOutPipeId, pTxContext);
		pAd->TxRingTotalNumber[BulkOutPipeId]--;    // sync. to TxCount

		if ((!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS)) &&
			(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS)) &&
			(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)) &&
			(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BULKOUT_RESET)))
		{
			RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_BULKOUT_RESET);

			RTUSBEnqueueInternalCmd(pAd, RT_OID_USB_RESET_BULK_OUT);
		}

	}
#endif


	pTxContext = &(pAd->TxContext[BulkOutPipeId][pAd->NextBulkOutIndex[BulkOutPipeId]]);
	//
	// bInUse = TRUE, means some process are filling TX data, after that must turn on bWaitingBulkOut
	// bWaitingBulkOut = TRUE, means the TX data are waiting for bulk out. 
	//
	if ((pTxContext->bWaitingBulkOut == TRUE) && !RTUSB_TEST_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_FRAG << BulkOutPipeId)))
	{
		// Indicate There is data avaliable
		RTUSB_SET_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_NORMAL << BulkOutPipeId));
	}


	NdisAcquireSpinLock(&pAd->BulkOutLock[BulkOutPipeId], IrqFlags);
	pAd->BulkOutPending[BulkOutPipeId] = FALSE;
	NdisReleaseSpinLock(&pAd->BulkOutLock[BulkOutPipeId], IrqFlags);
	// Always call Bulk routine, even reset bulk.
	// The protectioon of rest bulk should be in BulkOut routine
	RTUSBKickBulkOut(pAd);
}

// NULL frame use BulkOutPipeId = 0
VOID RTUSBBulkOutNullFrameComplete(purbb_t pUrb, struct pt_regs *pt_regs)
{
	PRTMP_ADAPTER	pAd;
	PTX_CONTEXT		pNullContext;
	NTSTATUS		status;
	unsigned long	IrqFlags;
	
	pNullContext= (PTX_CONTEXT)pUrb->context;
	pAd = pNullContext->pAd;

	DBGPRINT_RAW(RT_DEBUG_INFO, "--->RTUSBBulkOutNullFrameComplete\n");

	// Reset Null frame context flags
	pNullContext->IRPPending = FALSE;
	pNullContext->InUse = FALSE;
	
	status = pUrb->status;
	
	if (status == USB_ST_NOERROR)
	{
		// Don't worry about the queue is empty or not, this function will check itself
		RTMPDeQueuePacket(pAd, 0);
	}
#if 1	// STATUS_OTHER
	else
	{
		if ((!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS)) &&
			(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS)) &&
			(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)) &&
			(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BULKOUT_RESET)))
		{
			DBGPRINT_RAW(RT_DEBUG_ERROR, "Bulk Out Null Frame Failed\n");
			RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_BULKOUT_RESET);
			RTUSBEnqueueInternalCmd(pAd, RT_OID_USB_RESET_BULK_OUT);
		}
	}
#endif

	NdisAcquireSpinLock(&pAd->BulkOutLock[0], IrqFlags);
	pAd->BulkOutPending[0] = FALSE;
	NdisReleaseSpinLock(&pAd->BulkOutLock[0], IrqFlags);

	// Always call Bulk routine, even reset bulk.
	// The protectioon of rest bulk should be in BulkOut routine
	RTUSBKickBulkOut(pAd);

	DBGPRINT_RAW(RT_DEBUG_INFO, "<---RTUSBBulkOutNullFrameComplete\n");

}

// RTS frame use BulkOutPipeId = PipeID
VOID RTUSBBulkOutRTSFrameComplete(purbb_t pUrb, struct pt_regs *pt_regs)
{
	PRTMP_ADAPTER	pAd;
	PTX_CONTEXT		pRTSContext;
	NTSTATUS		status;
	unsigned long	IrqFlags;
	
	pRTSContext= (PTX_CONTEXT)pUrb->context;
	pAd = pRTSContext->pAd;
	
	
	DBGPRINT_RAW(RT_DEBUG_INFO, "--->RTUSBBulkOutRTSFrameComplete\n");

	// Reset RTS frame context flags
	pRTSContext->IRPPending = FALSE;
	pRTSContext->InUse = FALSE;
		
	status = pUrb->status;
	
	if (status == USB_ST_NOERROR)
	{
		// Don't worry about the queue is empty or not, this function will check itself
		RTMPDeQueuePacket(pAd, pRTSContext->BulkOutPipeId);
	}
#if 1	// STATUS_OTHER
	else
	{
		if ((!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS)) &&
			(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS)) &&
			(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)) &&
			(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BULKOUT_RESET)))
		{
			DBGPRINT_RAW(RT_DEBUG_ERROR, "Bulk Out RTS Frame Failed\n");
			RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_BULKOUT_RESET);
			RTUSBEnqueueInternalCmd(pAd, RT_OID_USB_RESET_BULK_OUT);
		}
	}
#endif

	NdisAcquireSpinLock(&pAd->BulkOutLock[pRTSContext->BulkOutPipeId], IrqFlags);
	pAd->BulkOutPending[pRTSContext->BulkOutPipeId] = FALSE;
	NdisReleaseSpinLock(&pAd->BulkOutLock[pRTSContext->BulkOutPipeId], IrqFlags);

	// Always call Bulk routine, even reset bulk.
	// The protectioon of rest bulk should be in BulkOut routine
	RTUSBKickBulkOut(pAd);

	DBGPRINT_RAW(RT_DEBUG_INFO, "<---RTUSBBulkOutRTSFrameComplete\n");
	
}

// MLME use BulkOutPipeId = 0
VOID RTUSBBulkOutMLMEPacketComplete(purbb_t pUrb, struct pt_regs *pt_regs)
{
	PTX_CONTEXT			pMLMEContext;
	PRTMP_ADAPTER		pAd;
	NTSTATUS			status;
	unsigned long	IrqFlags;
	
	pMLMEContext= (PTX_CONTEXT)pUrb->context;
	pAd = pMLMEContext->pAd;
	status = pUrb->status;

	pAd->PrioRingTxCnt--;
	if (pAd->PrioRingTxCnt < 0)
		pAd->PrioRingTxCnt = 0;

	pAd->PrioRingFirstIndex++;
	if (pAd->PrioRingFirstIndex >= PRIO_RING_SIZE)
	{
		pAd->PrioRingFirstIndex = 0;
	}	

	DBGPRINT(RT_DEBUG_INFO, "RTUSBBulkOutMLMEPacketComplete::PrioRingFirstIndex = %d, PrioRingTxCnt = %d, PopMgmtIndex = %d, PushMgmtIndex = %d, NextMLMEIndex = %d\n", 
			pAd->PrioRingFirstIndex, 
			pAd->PrioRingTxCnt, pAd->PopMgmtIndex, pAd->PushMgmtIndex, pAd->NextMLMEIndex);

	DBGPRINT_RAW(RT_DEBUG_INFO, "--->RTUSBBulkOutMLMEPacketComplete\n");
	
	// Reset MLME context flags
	pMLMEContext->IRPPending	= FALSE;
	pMLMEContext->InUse 		= FALSE;
	pMLMEContext->bWaitingBulkOut =FALSE;
	
	if (status == USB_ST_NOERROR)
	{
		// Don't worry about the queue is empty or not, this function will check itself
		RTUSBDequeueMLMEPacket(pAd);
	}
#if 1	// STATUS_OTHER
	else
	{
		if ((!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS)) &&
			(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS)) &&
			(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)) &&
			(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BULKOUT_RESET)))
		{
			DBGPRINT_RAW(RT_DEBUG_ERROR, "Bulk Out MLME Failed\n");
			RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_BULKOUT_RESET);
			RTUSBEnqueueInternalCmd(pAd, RT_OID_USB_RESET_BULK_OUT);
		}
	}
#endif
	pMLMEContext = &pAd->MLMEContext[pAd->PrioRingFirstIndex];

	if ( (pAd->PrioRingTxCnt >= 1) && (pMLMEContext->bWaitingBulkOut == TRUE))
		RTUSB_SET_BULK_FLAG(pAd, fRTUSB_BULK_OUT_MLME);

	NdisAcquireSpinLock(&pAd->BulkOutLock[0], IrqFlags);
	pAd->BulkOutPending[0] = FALSE;
	NdisReleaseSpinLock(&pAd->BulkOutLock[0], IrqFlags);

	// Always call Bulk routine, even reset bulk.
	// The protectioon of rest bulk should be in BulkOut routine
	RTUSBKickBulkOut(pAd);

	DBGPRINT_RAW(RT_DEBUG_INFO, "<---RTUSBBulkOutMLMEPacketComplete\n");

}

// PS-Poll frame use BulkOutPipeId = 0
VOID RTUSBBulkOutPsPollComplete(purbb_t pUrb,struct pt_regs *pt_regs)
{
	PRTMP_ADAPTER	pAd;
	PTX_CONTEXT		pPsPollContext;
	NTSTATUS		status;
	unsigned long	IrqFlags;
	
	pPsPollContext= (PTX_CONTEXT)pUrb->context;
	pAd = pPsPollContext->pAd;

	DBGPRINT_RAW(RT_DEBUG_INFO, "--->RTUSBBulkOutPsPollComplete\n");
		
	// Reset PsPoll context flags
	pPsPollContext->IRPPending	= FALSE;
	pPsPollContext->InUse		= FALSE;
	
	status = pUrb->status;
	if (status == USB_ST_NOERROR)
	{
		// Don't worry about the queue is empty or not, this function will check itself
		RTMPDeQueuePacket(pAd, 0);
	}
#if 1	// STATUS_OTHER
	else
	{
		if ((!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS)) &&
			(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS)) &&
			(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)) &&
			(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BULKOUT_RESET)))
		{
			DBGPRINT_RAW(RT_DEBUG_ERROR, "Bulk Out PSPoll Failed\n");
			RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_BULKOUT_RESET);
			RTUSBEnqueueInternalCmd(pAd, RT_OID_USB_RESET_BULK_OUT);
		}
	}
#endif

	NdisAcquireSpinLock(&pAd->BulkOutLock[0], IrqFlags);
	pAd->BulkOutPending[0] = FALSE;
	NdisReleaseSpinLock(&pAd->BulkOutLock[0], IrqFlags);

	// Always call Bulk routine, even reset bulk.
	// The protectioon of rest bulk should be in BulkOut routine
	RTUSBKickBulkOut(pAd);

	DBGPRINT_RAW(RT_DEBUG_INFO, "<---RTUSBBulkOutPsPollComplete\n");

}

/*
	========================================================================

	Routine Description:
		This routine process Rx Irp and call rx complete function.
		
	Arguments:
		DeviceObject	Pointer to the device object for next lower
						device. DeviceObject passed in here belongs to
						the next lower driver in the stack because we
						were invoked via IoCallDriver in USB_RxPacket
						AND it is not OUR device object
	  Irp				Ptr to completed IRP
	  Context			Ptr to our Adapter object (context specified
						in IoSetCompletionRoutine
		
	Return Value:
		Always returns STATUS_MORE_PROCESSING_REQUIRED

	Note:
		Always returns STATUS_MORE_PROCESSING_REQUIRED
	========================================================================
*/
VOID RTUSBBulkRxComplete(purbb_t pUrb, struct pt_regs *pt_regs)
{
	PRX_CONTEXT 	pRxContext;
	PRTMP_ADAPTER	pAd;
	NTSTATUS		status;
	
	pRxContext= (PRX_CONTEXT)pUrb->context;
	pAd = pRxContext->pAd;

	//
	// We have a number of cases:
	//		1) The USB read timed out and we received no data.
	//		2) The USB read timed out and we received some data.
	//		3) The USB read was successful and fully filled our irp buffer.
	//		4) The irp was cancelled.
	//		5) Some other failure from the USB device object.
	//
	
	//
	// Free the IRP  and its mdl because they were	alloced by us
	//
#if 0
	if ( (atomread = (atomic_read(&pRxContext->IrpLock))) == IRPLOCK_CANCE_START)
	{
		atomic_dec(&pAd->PendingRx);
		atomic_set(&pRxContext->IrpLock, IRPLOCK_CANCE_COMPLETE);	
	}
#endif
	status = pUrb->status;
	atomic_set(&pRxContext->IrpLock, IRPLOCK_COMPLETED);
	if( atomic_read(&pAd->PendingRx) > 0)
		atomic_dec(&pAd->PendingRx);

	switch (status)
	{
		case 0:
			break;

		case -ECONNRESET:		// async unlink
		case -ESHUTDOWN:		// hardware gone = -108
			//pUrb = NULL;
			DBGPRINT(RT_DEBUG_ERROR, "==> RTUSBBulkRxComplete Error code = %d\n", status);
			break;

		default:
			DBGPRINT(RT_DEBUG_ERROR, "==> RTUSBBulkRxComplete UnKnown Error code = %d\n", status);
			break;
	}

	if (atomic_read(&pRxContext->IrpLock) != IRPLOCK_CANCE_START)
	{
		pAd->rx_bh.data = (unsigned long)pUrb;
		if(!RTMP_TEST_FLAG(pAd,fRTMP_ADAPTER_HALT_IN_PROGRESS)) //Add by Zero:Jan09.2008 Fix IF down crash
		tasklet_schedule(&pAd->rx_bh);
//iverson patch usb 1.1 or 2.0 2007 1109
		if(pAd->BulkOutMaxPacketSize == 512)
		{ 
			DBGPRINT(RT_DEBUG_INFO, "In USB 2.0 Mode \n");
		}
		else{
 			RTUSBBulkReceive(pAd);
		}


	}
	else
		DBGPRINT(RT_DEBUG_INFO, "==> RTUSBBulkRxComplete  (IrpLock) = %d\n", atomic_read(&pRxContext->IrpLock));
#if 0
	 if ((status == USB_ST_NOERROR) && (atomic_read(&pRxContext->IrpLock) != IRPLOCK_CANCE_START))
	{
		RTUSBRxPacket(pUrb);
		//tasklet_schedule(&pAd->rx_bh);
		
	}// STATUS_SUCCESS
	else
	{
		DBGPRINT(RT_DEBUG_TEMP,"==> RTUSBBulkRxComplete Error code = %d\n", status);
		pRxContext->InUse = FALSE;

		if ((!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS)) &&
			(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BULKIN_RESET)) &&
			(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS)) &&
			(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)))
		{
			DBGPRINT_RAW(RT_DEBUG_ERROR, "Bulk In Failed\n");
			RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_BULKIN_RESET);
			RTUSBEnqueueInternalCmd(pAd, RT_OID_USB_RESET_BULK_IN);
		}
	}

#endif
}

VOID	RTUSBInitTxDesc(
	IN	PRTMP_ADAPTER	pAd,
	IN	PTX_CONTEXT 	pTxContext,
	IN	UCHAR			BulkOutPipeId,
	IN	usb_complete_t	Func)
{
	PURB				pUrb;
	PUCHAR				pSrc = NULL;

	pUrb = pTxContext->pUrb;
	ASSERT(pUrb);

	// Store BulkOut PipeId
	pTxContext->BulkOutPipeId = BulkOutPipeId;
	
    pSrc = (PUCHAR) &pTxContext->TransferBuffer->TxDesc;


	//Initialize a tx bulk urb
	RTusb_fill_bulk_urb(pUrb,
						pAd->pUsb_Dev,
						usb_sndbulkpipe(pAd->pUsb_Dev, 1),
						pSrc,
						pTxContext->BulkOutSize,
						Func,
						pTxContext);
		
}

VOID	RTUSBInitRxDesc(
	IN	PRTMP_ADAPTER	pAd,
	IN	PRX_CONTEXT		pRxContext)
{
	PURB				pUrb;
	
	pUrb = pRxContext->pUrb;
	ASSERT(pUrb);

	//Initialize a rx bulk urb
	RTusb_fill_bulk_urb(pUrb,
						pAd->pUsb_Dev,
						usb_rcvbulkpipe(pAd->pUsb_Dev, 1),
						pRxContext->TransferBuffer,
						BUFFER_SIZE,
						RTUSBBulkRxComplete,
						pRxContext);
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:

	Note:
	
	========================================================================
*/
VOID	RTUSBBulkOutDataPacket(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			BulkOutPipeId,
	IN	UCHAR			Index)
{
	PTX_CONTEXT	pTxContext;
	PURB		pUrb;
	int 		ret = 0;
	unsigned long	IrqFlags;
	
	NdisAcquireSpinLock(&pAd->BulkOutLock[BulkOutPipeId], IrqFlags);
	if (pAd->BulkOutPending[BulkOutPipeId] == TRUE)
	{
		NdisReleaseSpinLock(&pAd->BulkOutLock[BulkOutPipeId], IrqFlags);
		return;
	}
	pAd->BulkOutPending[BulkOutPipeId] = TRUE;
	NdisReleaseSpinLock(&pAd->BulkOutLock[BulkOutPipeId], IrqFlags);

	pTxContext = &(pAd->TxContext[BulkOutPipeId][Index]);

	// Increase Total transmit byte counter
	pAd->RalinkCounters.TransmittedByteCount +=  pTxContext->BulkOutSize;


	// Clear Data flag
	RTUSB_CLEAR_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_FRAG << BulkOutPipeId));
	RTUSB_CLEAR_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_NORMAL << BulkOutPipeId));

	if (pTxContext->bWaitingBulkOut	!= TRUE)
	{
		DBGPRINT(RT_DEBUG_ERROR, "RTUSBBulkOutDataPacket failed, pTxContext->bWaitingBulkOut != TRUE, Index %d, NextBulkOutIndex %d\n", 
			Index, pAd->NextBulkOutIndex[BulkOutPipeId]);
		NdisAcquireSpinLock(&pAd->BulkOutLock[BulkOutPipeId], IrqFlags);
		pAd->BulkOutPending[BulkOutPipeId] = FALSE;
		NdisReleaseSpinLock(&pAd->BulkOutLock[BulkOutPipeId], IrqFlags);
		return;
	}
	else if (pTxContext->BulkOutSize == 0)
	{
		//
		// This may happen on CCX Leap Ckip or Cmic
		// When the Key was been set not on time.
		// We will break it when the Key was Zero on RTUSBHardTransmit
		// And this will cause deadlock that the TxContext always InUse.
		//
		NdisAcquireSpinLock(&pAd->BulkOutLock[BulkOutPipeId], IrqFlags);
		
		pTxContext->InUse	   = FALSE;
		pTxContext->LastOne    = FALSE;
		pTxContext->IRPPending = FALSE;
		pTxContext->bWaitingBulkOut = FALSE;
		pTxContext->BulkOutSize= 0;
		pAd->NextBulkOutIndex[BulkOutPipeId] = (pAd->NextBulkOutIndex[BulkOutPipeId] + 1) % TX_RING_SIZE;
		pAd->BulkOutPending[BulkOutPipeId] = FALSE;
		NdisReleaseSpinLock(&pAd->BulkOutLock[BulkOutPipeId], IrqFlags);

		return;		
	}
	else if (!OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
	{
		//
		// Since there is no connection, so we need to empty the Tx Bulk out Ring.
		//
		while (atomic_read(&pAd->TxCount) > 0)
		{
			DBGPRINT(RT_DEBUG_ERROR, "RTUSBBulkOutDataPacket failed, snice NdisMediaStateDisconnected discard NextBulkOutIndex %d, NextIndex = %d\n", 
				pAd->NextBulkOutIndex[BulkOutPipeId], pAd->NextTxIndex[BulkOutPipeId]);
				
			FREE_TX_RING(pAd, BulkOutPipeId, pTxContext);
			pAd->TxRingTotalNumber[BulkOutPipeId]--;    // sync. to TxCount
			pTxContext = &(pAd->TxContext[BulkOutPipeId][pAd->NextBulkOutIndex[BulkOutPipeId]]);
		}

		NdisAcquireSpinLock(&pAd->BulkOutLock[BulkOutPipeId], IrqFlags);
		pAd->BulkOutPending[BulkOutPipeId] = FALSE;
		NdisReleaseSpinLock(&pAd->BulkOutLock[BulkOutPipeId], IrqFlags);

		return;
	}
	

	// Init Tx context descriptor
	RTUSBInitTxDesc(pAd, pTxContext, BulkOutPipeId, RTUSBBulkOutDataPacketComplete);

	
	pTxContext->IRPPending = TRUE;

	pUrb = pTxContext->pUrb;
	if((ret = rtusb_submit_urb(pUrb))!=0)
	{
		DBGPRINT(RT_DEBUG_ERROR, "Submit Tx URB failed %d\n", ret);
		return;
	}

	DBGPRINT_RAW(RT_DEBUG_INFO, "<---RTUSBBulkOutDataPacket \n");
	return;
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:

	Note: NULL frame use BulkOutPipeId = 0
	
	========================================================================
*/
VOID	RTUSBBulkOutNullFrame(
	IN	PRTMP_ADAPTER	pAd)
{
	PTX_CONTEXT	pNullContext = &(pAd->NullContext);
	PURB		pUrb;
	int 		ret = 0;
	unsigned long	IrqFlags;
	
	NdisAcquireSpinLock(&pAd->BulkOutLock[0], IrqFlags);
	if (pAd->BulkOutPending[0] == TRUE)
	{
		NdisReleaseSpinLock(&pAd->BulkOutLock[0], IrqFlags);
		return;
	}
	pAd->BulkOutPending[0] = TRUE;
	NdisReleaseSpinLock(&pAd->BulkOutLock[0], IrqFlags);

	// Increase Total transmit byte counter
	pAd->RalinkCounters.TransmittedByteCount +=  pNullContext->BulkOutSize;

	DBGPRINT_RAW(RT_DEBUG_INFO, "--->RTUSBBulkOutNullFrame \n");
	
	// Clear Null frame bulk flag
	RTUSB_CLEAR_BULK_FLAG(pAd, fRTUSB_BULK_OUT_DATA_NULL);


	// Init Tx context descriptor
	RTUSBInitTxDesc(pAd, pNullContext, 0, RTUSBBulkOutNullFrameComplete);
	pNullContext->IRPPending = TRUE;

	pUrb = pNullContext->pUrb;
	if((ret = rtusb_submit_urb(pUrb))!=0)
	{
		DBGPRINT(RT_DEBUG_ERROR,"Submit Tx URB failed %d\n", ret);
		return;
	}	
	
	DBGPRINT_RAW(RT_DEBUG_INFO, "<---RTUSBBulkOutNullFrame \n");
	return;
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:

	Note: 
	
	========================================================================
*/
VOID	RTUSBBulkOutRTSFrame(
	IN	PRTMP_ADAPTER	pAd)
{
	PTX_CONTEXT	pRTSContext = &(pAd->RTSContext);
	PURB		pUrb;
	int 		ret = 0;
	unsigned long	IrqFlags;
	UCHAR		PipeID=0;
	
	if (RTUSB_TEST_BULK_FLAG(pAd, fRTUSB_BULK_OUT_DATA_NORMAL_4))
		PipeID= 3;
	else if (RTUSB_TEST_BULK_FLAG(pAd, fRTUSB_BULK_OUT_DATA_NORMAL_3))
		PipeID= 2;
	else if (RTUSB_TEST_BULK_FLAG(pAd, fRTUSB_BULK_OUT_DATA_NORMAL_2))
		PipeID= 1;
	else if (RTUSB_TEST_BULK_FLAG(pAd, fRTUSB_BULK_OUT_DATA_NORMAL))
		PipeID= 0;


	NdisAcquireSpinLock(&pAd->BulkOutLock[PipeID], IrqFlags);
	if (pAd->BulkOutPending[PipeID] == TRUE)
	{
		NdisReleaseSpinLock(&pAd->BulkOutLock[PipeID], IrqFlags);
		return;
	}
	pAd->BulkOutPending[PipeID] = TRUE;
	NdisReleaseSpinLock(&pAd->BulkOutLock[PipeID], IrqFlags);

	// Increase Total transmit byte counter
	pAd->RalinkCounters.TransmittedByteCount +=  pRTSContext->BulkOutSize;

	DBGPRINT_RAW(RT_DEBUG_INFO, "--->RTUSBBulkOutRTSFrame \n");
	
	// Clear RTS frame bulk flag
	RTUSB_CLEAR_BULK_FLAG(pAd, fRTUSB_BULK_OUT_RTS);

	// Init Tx context descriptor
	RTUSBInitTxDesc(pAd, pRTSContext, PipeID, RTUSBBulkOutRTSFrameComplete);
	pRTSContext->IRPPending = TRUE;
	
	pUrb = pRTSContext->pUrb;
	if((ret = rtusb_submit_urb(pUrb))!=0)
	{
		DBGPRINT(RT_DEBUG_ERROR,"Submit Tx URB failed %d\n", ret);
		return;
	}	
	
	DBGPRINT_RAW(RT_DEBUG_INFO, "<---RTUSBBulkOutRTSFrame \n");
	return;
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:

	Note: MLME use BulkOutPipeId = 0
	
	========================================================================
*/
VOID	RTUSBBulkOutMLMEPacket(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			Index)
{
	PTX_CONTEXT		pMLMEContext;
	PURB			pUrb;
	int 			ret = 0;
	unsigned long	IrqFlags;
	
	pMLMEContext = &pAd->MLMEContext[Index];
	
	NdisAcquireSpinLock(&pAd->BulkOutLock[0], IrqFlags);
	if (pAd->BulkOutPending[0] == TRUE)
	{
		NdisReleaseSpinLock(&pAd->BulkOutLock[0], IrqFlags);
		return;
	}
	pAd->BulkOutPending[0] = TRUE;
	NdisReleaseSpinLock(&pAd->BulkOutLock[0], IrqFlags);

	// Increase Total transmit byte counter
	pAd->RalinkCounters.TransmittedByteCount +=  pMLMEContext->BulkOutSize;

	// Clear MLME bulk flag
	RTUSB_CLEAR_BULK_FLAG(pAd, fRTUSB_BULK_OUT_MLME);

	DBGPRINT_RAW(RT_DEBUG_INFO, "RTUSBBulkOutMLMEPacket::PrioRingFirstIndex = %d, PrioRingTxCnt = %d, PopMgmtIndex = %d, PushMgmtIndex = %d, NextMLMEIndex = %d\n", 
			pAd->PrioRingFirstIndex, 
			pAd->PrioRingTxCnt, pAd->PopMgmtIndex, pAd->PushMgmtIndex, pAd->NextMLMEIndex);

	DBGPRINT_RAW(RT_DEBUG_INFO, "--->RTUSBBulkOutMLMEPacket\n");


	// Init Tx context descriptor
	RTUSBInitTxDesc(pAd, pMLMEContext, 0, RTUSBBulkOutMLMEPacketComplete);
	pMLMEContext->IRPPending = TRUE;


	pUrb = pMLMEContext->pUrb;
	if((ret = rtusb_submit_urb(pUrb))!=0)
	{
		DBGPRINT(RT_DEBUG_ERROR,"Submit MLME URB failed %d\n", ret);
		return;
	}
	
	DBGPRINT_RAW(RT_DEBUG_INFO, "<---RTUSBBulkOutMLMEPacket \n");
	return;
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:

	Note: PsPoll use BulkOutPipeId = 0
	
	========================================================================
*/
VOID	RTUSBBulkOutPsPoll(
	IN	PRTMP_ADAPTER	pAd)
{
	PTX_CONTEXT		pPsPollContext = &(pAd->PsPollContext);
	PURB			pUrb;
	int 			ret = 0;
	unsigned long	IrqFlags;
	
	NdisAcquireSpinLock(&pAd->BulkOutLock[0], IrqFlags);
	if (pAd->BulkOutPending[0] == TRUE)
	{
		NdisReleaseSpinLock(&pAd->BulkOutLock[0], IrqFlags);
		return;
	}
	pAd->BulkOutPending[0] = TRUE;
	NdisReleaseSpinLock(&pAd->BulkOutLock[0], IrqFlags);

	DBGPRINT_RAW(RT_DEBUG_INFO, "--->RTUSBBulkOutPsPoll \n");
	
	// Clear PS-Poll bulk flag
	RTUSB_CLEAR_BULK_FLAG(pAd, fRTUSB_BULK_OUT_PSPOLL);


	// Init Tx context descriptor
	RTUSBInitTxDesc(pAd, pPsPollContext, 0, RTUSBBulkOutPsPollComplete);
	pPsPollContext->IRPPending = TRUE;
	
	pUrb = pPsPollContext->pUrb;
	if((ret = rtusb_submit_urb(pUrb))!=0)
	{
		DBGPRINT(RT_DEBUG_ERROR,"Submit Tx URB failed %d\n", ret);
		return;
	}
	
	DBGPRINT_RAW(RT_DEBUG_INFO, "<---RTUSBBulkOutPsPoll \n");
	return;
}
void dump_urb(struct urb* purb)
{
	printk("urb                  :0x%08lx\n", (unsigned long)purb);
	printk("\tdev                   :0x%08lx\n", (unsigned long)purb->dev);
//Benson 20080505 add
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	printk("\t\tdev->state          :0x%d\n", purb->dev->state);
#endif
//Benson 20080505 end
	printk("\tpipe                  :0x%08x\n", purb->pipe);
	printk("\tstatus                :%d\n", purb->status);
	printk("\ttransfer_flags        :0x%08x\n", purb->transfer_flags);
	printk("\ttransfer_buffer       :0x%08lx\n", (unsigned long)purb->transfer_buffer);
	printk("\ttransfer_buffer_length:%d\n", purb->transfer_buffer_length);
	printk("\tactual_length         :%d\n", purb->actual_length);
	printk("\tsetup_packet          :0x%08lx\n", (unsigned long)purb->setup_packet);
	printk("\tstart_frame           :%d\n", purb->start_frame);
	printk("\tnumber_of_packets     :%d\n", purb->number_of_packets);
	printk("\tinterval              :%d\n", purb->interval);
	printk("\terror_count           :%d\n", purb->error_count);
	printk("\tcontext               :0x%08lx\n", (unsigned long)purb->context);
	printk("\tcomplete              :0x%08lx\n", (unsigned long)purb->complete);
	//printk("\tuse_count			:0x%d\n\n",purb->use_count);
}


/*
	========================================================================

	Routine Description:
	USB_RxPacket initializes a URB and uses the Rx IRP to submit it
	to USB. It checks if an Rx Descriptor is available and passes the
	the coresponding buffer to be filled. If no descriptor is available
	fails the request. When setting the completion routine we pass our
	Adapter Object as Context.
		
	Arguments:
		
	Return Value:
		TRUE			found matched tuple cache
		FALSE			no matched found

	Note:
	
	========================================================================
*/
VOID	RTUSBBulkReceive(
	IN	PRTMP_ADAPTER	pAd)
{
	PRX_CONTEXT pRxContext;
	PURB		pUrb;
	int 		ret = 0;

	if ((RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST))||
		(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF))||
		(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_PIPE_IN_PROGRESS)))
	{
		return;
	}

	DBGPRINT(RT_DEBUG_INFO,"RTUSBBulkReceive:: pAd->NextRxBulkInIndex = %d\n",pAd->NextRxBulkInIndex);
 
 
	pRxContext = &(pAd->RxContext[pAd->NextRxBulkInIndex]);
	pRxContext->InUse = TRUE;
	pAd->NextRxBulkInIndex = (pAd->NextRxBulkInIndex + 1) % RX_RING_SIZE;
	
	atomic_set(&pRxContext->IrpLock, IRPLOCK_CANCELABLE);	

	// Init Rx context descriptor
	NdisZeroMemory(pRxContext->TransferBuffer, BUFFER_SIZE);
	RTUSBInitRxDesc(pAd, pRxContext);
	
	pUrb = pRxContext->pUrb;
	if((ret = rtusb_submit_urb(pUrb))!=0)
	{
		DBGPRINT(RT_DEBUG_ERROR,"Submit Rx URB failed %d\n", ret);
		return;
	}

	atomic_inc(&pAd->PendingRx);

	return;
}

VOID RTUSBBulkRxHandle(
	IN unsigned long data)
{
	purbb_t 		pUrb = (purbb_t)data;
	PRTMP_ADAPTER	pAd;
	PRX_CONTEXT 	pRxContext;

	pRxContext = (PRX_CONTEXT)pUrb->context;
	pAd = pRxContext->pAd;

	/* device had been closed */
	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_REMOVE_IN_PROGRESS)) 
		return;

	if(pUrb->status != 0)
	{
		RTUSBBulkReceive(pAd);
		return;
	}

	RTUSBRxPacket(data);
	return;
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:

	Note:
	
	========================================================================
*/
VOID	RTUSBKickBulkOut(
	IN	PRTMP_ADAPTER pAd)
{
	
	DBGPRINT_RAW(RT_DEBUG_INFO, "--->RTUSBKickBulkOut\n");
	
	
	if (!(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)) &&
		!(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS)) &&
		!(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS)) &&
		!(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_PIPE_IN_PROGRESS)) &&
#ifdef RALINK_ATE			
		!(pAd->ate.Mode != ATE_STASTART) &&
#endif					
		!(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF)))
	{

		// 1. Data Fragment has highest priority
		if (RTUSB_TEST_BULK_FLAG(pAd, fRTUSB_BULK_OUT_DATA_FRAG))
		{
			if ((!LOCAL_TX_RING_EMPTY(pAd, 0)) && 
				(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS)))
			{				
				RTUSBBulkOutDataPacket(pAd, 0, pAd->NextBulkOutIndex[0]);
			}
		}
		
		if (RTUSB_TEST_BULK_FLAG(pAd, fRTUSB_BULK_OUT_DATA_FRAG_2))
		{
			if ((!LOCAL_TX_RING_EMPTY(pAd, 1)) && 
				(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS)))
			{
				RTUSBBulkOutDataPacket(pAd, 1, pAd->NextBulkOutIndex[1]);
			}
		}
		
		if (RTUSB_TEST_BULK_FLAG(pAd, fRTUSB_BULK_OUT_DATA_FRAG_3))
		{
			if ((!LOCAL_TX_RING_EMPTY(pAd, 2)) && 
				(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS)))
			{
				RTUSBBulkOutDataPacket(pAd, 2, pAd->NextBulkOutIndex[2]);
			}
		}
		
		if (RTUSB_TEST_BULK_FLAG(pAd, fRTUSB_BULK_OUT_DATA_FRAG_4))
		{		
			if ((!LOCAL_TX_RING_EMPTY(pAd, 3)) && 
				(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS)))
			{
				RTUSBBulkOutDataPacket(pAd, 3, pAd->NextBulkOutIndex[3]);
			}
		}
		
		// 2. PS-Poll frame is next
		if (RTUSB_TEST_BULK_FLAG(pAd, fRTUSB_BULK_OUT_PSPOLL))
		{
			RTUSBBulkOutPsPoll(pAd);
		}		
		// 5. Mlme frame is next
		else if (RTUSB_TEST_BULK_FLAG(pAd, fRTUSB_BULK_OUT_MLME))
		{
			RTUSBBulkOutMLMEPacket(pAd, pAd->PrioRingFirstIndex);
		}

		// 6. Data frame normal is next
		if (RTUSB_TEST_BULK_FLAG(pAd, fRTUSB_BULK_OUT_DATA_NORMAL))
		{
			if ((!LOCAL_TX_RING_EMPTY(pAd, 0)) && 
				(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS)))
			{
				RTUSBBulkOutDataPacket(pAd, 0, pAd->NextBulkOutIndex[0]);			
			}
		}
				
		if (RTUSB_TEST_BULK_FLAG(pAd, fRTUSB_BULK_OUT_DATA_NORMAL_2))
		{		
			if ((!LOCAL_TX_RING_EMPTY(pAd, 1)) && 
				(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS)))			
			{
				RTUSBBulkOutDataPacket(pAd, 1, pAd->NextBulkOutIndex[1]);
			}
		}

		if (RTUSB_TEST_BULK_FLAG(pAd, fRTUSB_BULK_OUT_DATA_NORMAL_3))
		{
			if ((!LOCAL_TX_RING_EMPTY(pAd, 2)) && 
				(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS)))			
			{
				RTUSBBulkOutDataPacket(pAd, 2, pAd->NextBulkOutIndex[2]);
			}
		}
		
		if (RTUSB_TEST_BULK_FLAG(pAd, fRTUSB_BULK_OUT_DATA_NORMAL_4))
		{
			if ((!LOCAL_TX_RING_EMPTY(pAd, 3)) && 
				(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS)))			
			{
				RTUSBBulkOutDataPacket(pAd, 3, pAd->NextBulkOutIndex[3]);
			}
		}

		// 7. Null frame is the last
		else if (RTUSB_TEST_BULK_FLAG(pAd, fRTUSB_BULK_OUT_DATA_NULL))
		{
			if (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS))
			{
				RTUSBBulkOutNullFrame(pAd);
			}
		}
		
		// 8. No data avaliable
		else
		{
			
		}
	}
	
#ifdef RALINK_ATE		//2006/06/11	
	//If the mode is in ATE mode.
	else if((pAd->ate.Mode != ATE_STASTART) &&
		!(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)) &&
		!(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS)) &&
		!(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS)) &&
		!(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_PIPE_IN_PROGRESS)) &&						
		!(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF)))
	{
		if (RTUSB_TEST_BULK_FLAG(pAd, fRTUSB_BULK_OUT_DATA_ATE))
		{
			ATE_RTUSBBulkOutDataPacket(pAd);
		}
	}	
#endif	
	
	DBGPRINT_RAW(RT_DEBUG_INFO, "<---RTUSBKickBulkOut\n");
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:

	Note:
	
	========================================================================
*/
VOID	RTUSBCleanUpDataBulkOutQueue(
	IN	PRTMP_ADAPTER	pAd)
{
	UCHAR			Idx;
	PTX_CONTEXT 	pTxContext;			
	unsigned long	IrqFlags;
	
	DBGPRINT(RT_DEBUG_TRACE, "--->CleanUpDataBulkOutQueue\n");

	for (Idx = 0; Idx < 4; Idx++)
	{
		while (!LOCAL_TX_RING_EMPTY(pAd, Idx))
		{			
			pTxContext						= &(pAd->TxContext[Idx][pAd->NextBulkOutIndex[Idx]]);
			pTxContext->LastOne 			= FALSE;
			pTxContext->InUse				= FALSE;
			pTxContext->bWaitingBulkOut		= FALSE;
			pAd->NextBulkOutIndex[Idx] = (pAd->NextBulkOutIndex[Idx] + 1) % TX_RING_SIZE;
		}
		NdisAcquireSpinLock(&pAd->BulkOutLock[Idx], IrqFlags);
		pAd->BulkOutPending[Idx] = FALSE;
		NdisReleaseSpinLock(&pAd->BulkOutLock[Idx], IrqFlags);
	}
	
	DBGPRINT(RT_DEBUG_TRACE, "<---CleanUpDataBulkOutQueue\n");
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:
	
	Note:
	
	========================================================================
*/
VOID	RTUSBCleanUpMLMEBulkOutQueue(
	IN	PRTMP_ADAPTER	pAd)
{
	unsigned long	IrqFlags;
	
	DBGPRINT(RT_DEBUG_TRACE, "--->CleanUpMLMEBulkOutQueue\n");

	NdisAcquireSpinLock(&pAd->MLMEQLock, IrqFlags);
	while (pAd->PrioRingTxCnt > 0)
	{
		pAd->MLMEContext[pAd->PrioRingFirstIndex].InUse = FALSE;
			
		pAd->PrioRingFirstIndex++;
		if (pAd->PrioRingFirstIndex >= PRIO_RING_SIZE)
		{
			pAd->PrioRingFirstIndex = 0;
		}

		pAd->PrioRingTxCnt--;
	}
	NdisReleaseSpinLock(&pAd->MLMEQLock, IrqFlags);

	DBGPRINT(RT_DEBUG_TRACE, "<---CleanUpMLMEBulkOutQueue\n");
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:

	Note:
	
	========================================================================
*/
VOID	RTUSBCancelPendingBulkInIRP(
	IN	PRTMP_ADAPTER	pAd)
{
	PRX_CONTEXT	pRxContext;
	UINT		i;

	DBGPRINT_RAW(RT_DEBUG_TRACE,"--->RTUSBCancelPendingBulkInIRP\n");
	for ( i = 0; i < RX_RING_SIZE; i++)
	{

		pRxContext = &(pAd->RxContext[i]);
		if(atomic_read(&pRxContext->IrpLock) == IRPLOCK_CANCELABLE)
		{
			RTUSB_UNLINK_URB(pRxContext->pUrb);
		}
		atomic_set(&pRxContext->IrpLock, IRPLOCK_CANCE_START);
	}
	DBGPRINT_RAW(RT_DEBUG_TRACE,"<---RTUSBCancelPendingBulkInIRP\n");
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:

	Note:
	
	========================================================================
*/
VOID	RTUSBCancelPendingBulkOutIRP(
	IN	PRTMP_ADAPTER	pAd)
{
	PTX_CONTEXT		pTxContext;
	PTX_CONTEXT		pMLMEContext;
	PTX_CONTEXT		pBeaconContext;
	PTX_CONTEXT		pNullContext;
	PTX_CONTEXT		pPsPollContext;
	PTX_CONTEXT		pRTSContext;
	UINT			i, Idx;
	unsigned long	IrqFlags;
	
	for (Idx = 0; Idx < 4; Idx++)
	{
		for (i = 0; i < TX_RING_SIZE; i++)
		{
			pTxContext = &(pAd->TxContext[Idx][i]);

			if (pTxContext->IRPPending == TRUE)
			{

				// Get the USB_CONTEXT and cancel it's IRP; the completion routine will itself
				// remove it from the HeadPendingSendList and NULL out HeadPendingSendList
				//	when the last IRP on the list has been	cancelled; that's how we exit this loop
				//

				RTUSB_UNLINK_URB(pTxContext->pUrb);

				// Sleep 200 microseconds to give cancellation time to work
				RTMPusecDelay(200);
			}
		}
	}

	for (i = 0; i < PRIO_RING_SIZE; i++)
	{
		pMLMEContext = &(pAd->MLMEContext[i]);

		if(pMLMEContext->IRPPending == TRUE)
		{

			// Get the USB_CONTEXT and cancel it's IRP; the completion routine will itself
			// remove it from the HeadPendingSendList and NULL out HeadPendingSendList
			//	when the last IRP on the list has been	cancelled; that's how we exit this loop
			//

			RTUSB_UNLINK_URB(pMLMEContext->pUrb);

			// Sleep 200 microsecs to give cancellation time to work
			RTMPusecDelay(200);
		}
	}

	for (i = 0; i < BEACON_RING_SIZE; i++)
	{
		pBeaconContext = &(pAd->BeaconContext[i]);

		if(pBeaconContext->IRPPending == TRUE)
		{

			// Get the USB_CONTEXT and cancel it's IRP; the completion routine will itself
			// remove it from the HeadPendingSendList and NULL out HeadPendingSendList
			//	when the last IRP on the list has been	cancelled; that's how we exit this loop
			//

			RTUSB_UNLINK_URB(pBeaconContext->pUrb);

			// Sleep 200 microsecs to give cancellation time to work
			RTMPusecDelay(200);
		}
	}

	pNullContext = &(pAd->NullContext);
	if (pNullContext->IRPPending == TRUE)
		RTUSB_UNLINK_URB(pNullContext->pUrb);

	pRTSContext = &(pAd->RTSContext);
	if (pRTSContext->IRPPending == TRUE)
		RTUSB_UNLINK_URB(pRTSContext->pUrb);
		
	pPsPollContext = &(pAd->PsPollContext);
	if (pPsPollContext->IRPPending == TRUE)
		RTUSB_UNLINK_URB(pPsPollContext->pUrb);

	for (Idx = 0; Idx < 4; Idx++)
	{
		NdisAcquireSpinLock(&pAd->BulkOutLock[Idx], IrqFlags);
		pAd->BulkOutPending[Idx] = FALSE;
		NdisReleaseSpinLock(&pAd->BulkOutLock[Idx], IrqFlags);
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
VOID	RTUSBCancelPendingIRPs(
	IN	PRTMP_ADAPTER	pAd)
{
	RTUSBCancelPendingBulkInIRP(pAd);
	RTUSBCancelPendingBulkOutIRP(pAd);
}

#ifdef RALINK_ATE
VOID ATE_RTUSBBulkOutDataPacketComplete(purbb_t pUrb, struct pt_regs *pt_regs)
{
	PRTMP_ADAPTER	pAd;
	PTX_CONTEXT		pNullContext;
	NTSTATUS		status;
	unsigned long	IrqFlags;
	ULONG			OldValue;
	
	pNullContext= (PTX_CONTEXT)pUrb->context;
	pAd = pNullContext->pAd;

	DBGPRINT_RAW(RT_DEBUG_INFO, "--->ATE_RTUSBBulkOutDataPacketComplete\n");

	// Reset Null frame context flags
	pNullContext->IRPPending = FALSE;
	pNullContext->InUse = FALSE;
	
	status = pUrb->status;
	
	if (status == USB_ST_NOERROR)
	{
		// Don't worry about the queue is empty or not, this function will check itself
		RTMPDeQueuePacket(pAd, 0);
	}
#if 1	// STATUS_OTHER
	else
	{
		if ((!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS)) &&
			(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS)) &&
			(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)) &&
			(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BULKOUT_RESET)))
		{
			DBGPRINT_RAW(RT_DEBUG_ERROR, "Bulk Out Null Frame Failed\n");
			RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_BULKOUT_RESET);
			RTUSBEnqueueInternalCmd(pAd, RT_OID_USB_RESET_BULK_OUT);
		}
	}
#endif	

	if (atomic_read(&pAd->BulkOutRemained) > 0)
	{			
		atomic_dec(&pAd->BulkOutRemained);
		DBGPRINT(RT_DEBUG_INFO, "Bulk Out Remained = %d\n", atomic_read(&pAd->BulkOutRemained));
	}
	
	// 1st - Transmit Success
	OldValue = pAd->WlanCounters.TransmittedFragmentCount.vv.LowPart;
	pAd->WlanCounters.TransmittedFragmentCount.vv.LowPart++;
	if (pAd->WlanCounters.TransmittedFragmentCount.vv.LowPart < OldValue)
	{
		pAd->WlanCounters.TransmittedFragmentCount.vv.HighPart++;
	}
	
	if(((pAd->ContinBulkOut == TRUE ) ||(atomic_read(&pAd->BulkOutRemained) > 0)) && (pAd->ate.Mode != ATE_STASTART))
	{	
	
		DBGPRINT(RT_DEBUG_INFO, "ContinBulkOut = TRUE \n");
		RTUSB_SET_BULK_FLAG(pAd, fRTUSB_BULK_OUT_DATA_ATE);
	}	
	else
	{
		RTUSB_CLEAR_BULK_FLAG(pAd, fRTUSB_BULK_OUT_DATA_ATE);
	}

	NdisAcquireSpinLock(&pAd->BulkOutLock[0], IrqFlags);
	pAd->BulkOutPending[0] = FALSE;
	NdisReleaseSpinLock(&pAd->BulkOutLock[0], IrqFlags);	
	// Always call Bulk routine, even reset bulk.
	// The protectioon of rest bulk should be in BulkOut routine
	RTUSBKickBulkOut(pAd);

	DBGPRINT_RAW(RT_DEBUG_INFO, "<---ATE_RTUSBBulkOutDataPacketComplete\n");

}


VOID ATE_RTUSBBulkOutDataPacket(	
	IN	PRTMP_ADAPTER	pAd)
{
	PTX_CONTEXT		pNullContext = &(pAd->NullContext);
	PURB			pUrb;
	int 			ret = 0;
	unsigned long	IrqFlags;


	ATE_RTMPSendNullFrame(pAd);
	
	DBGPRINT_RAW(RT_DEBUG_INFO, "--->ATE_RTUSBBulkOutDataPacket \n");	
	NdisAcquireSpinLock(&pAd->BulkOutLock[0], IrqFlags);
	if (pAd->BulkOutPending[0] == TRUE)
	{
		DBGPRINT_RAW(RT_DEBUG_INFO, "--->ATE_RTUSBBulkOutDataPacket BulkOutPending \n");
		NdisReleaseSpinLock(&pAd->BulkOutLock[0], IrqFlags);
		return;
	}
	pAd->BulkOutPending[0] = TRUE;
	NdisReleaseSpinLock(&pAd->BulkOutLock[0], IrqFlags);

	// Increase Total transmit byte counter
	pAd->RalinkCounters.TransmittedByteCount +=  pNullContext->BulkOutSize;

	DBGPRINT_RAW(RT_DEBUG_INFO, "--->ATE_RTUSBBulkOutDataPacket \n");

	// Clear Null frame bulk flag
	RTUSB_CLEAR_BULK_FLAG(pAd, fRTUSB_BULK_OUT_DATA_ATE);	

	// Init Tx context descriptor
	RTUSBInitTxDesc(pAd, pNullContext, 0, ATE_RTUSBBulkOutDataPacketComplete);
	pNullContext->IRPPending = TRUE;

	pUrb = pNullContext->pUrb;
	if((ret = rtusb_submit_urb(pUrb))!=0)
	{
		DBGPRINT(RT_DEBUG_ERROR,"Submit Tx URB failed %d\n", ret);
		return;
	}	
	
	DBGPRINT_RAW(RT_DEBUG_INFO, "<---ATE_RTUSBBulkOutDataPacket \n");
	return;
}
#endif
