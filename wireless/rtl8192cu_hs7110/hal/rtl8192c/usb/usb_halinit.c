/******************************************************************************
* usb_halinit.c                                                                                                                                 *
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
#define _HCI_HAL_INIT_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <hal_init.h>
#include <rtl8712_efuse.h>

#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif

#ifndef CONFIG_USB_HCI

#error "CONFIG_USB_HCI shall be on!\n"

#endif

#include <usb_ops.h>
#include <usb_hal.h>
#include <usb_osintf.h>

//endpoint number 1,2,3,4,5
// bult in : 1
// bult out: 2 (High)
// bult out: 3 (Normal) for 3 out_ep, (Low) for 2 out_ep
// interrupt in: 4
// bult out: 5 (Low) for 3 out_ep


static VOID
_OneOutEpMapping(
	IN	HAL_DATA_TYPE	*pHalData
	)
{
	//only endpoint number 0x02

	pHalData->Queue2EPNum[0] = 0x02;//VO
	pHalData->Queue2EPNum[1] = 0x02;//VI
	pHalData->Queue2EPNum[2] = 0x02;//BE
	pHalData->Queue2EPNum[3] = 0x02;//BK
	
	pHalData->Queue2EPNum[4] = 0x02;//TS
	pHalData->Queue2EPNum[5] = 0x02;//MGT
	pHalData->Queue2EPNum[6] = 0x02;//BMC
	pHalData->Queue2EPNum[7] = 0x02;//BCN
}


static VOID
_TwoOutEpMapping(
	IN	BOOLEAN			IsTestChip,
	IN	HAL_DATA_TYPE	*pHalData,
	IN	BOOLEAN	 		bWIFICfg
	)
{

/*
#define VO_QUEUE_INX	0
#define VI_QUEUE_INX		1
#define BE_QUEUE_INX		2
#define BK_QUEUE_INX		3
#define TS_QUEUE_INX		4
#define MGT_QUEUE_INX	5
#define BMC_QUEUE_INX	6
#define BCN_QUEUE_INX	7
*/

	if(IsTestChip && bWIFICfg){ // test chip && wmm
	
		
		//	BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA 
		//{  1, 	0, 	1, 	0, 	0, 	0, 	0, 	0, 		0	};			
		//0:H(end_number=0x02), 1:L (end_number=0x03)

		pHalData->Queue2EPNum[0] = 0x02;//VO
		pHalData->Queue2EPNum[1] = 0x03;//VI
		pHalData->Queue2EPNum[2] = 0x02;//BE
		pHalData->Queue2EPNum[3] = 0x03;//BK
		
		pHalData->Queue2EPNum[4] = 0x02;//TS
		pHalData->Queue2EPNum[5] = 0x02;//MGT
		pHalData->Queue2EPNum[6] = 0x02;//BMC
		pHalData->Queue2EPNum[7] = 0x02;//BCN
	
	}
	else if(!IsTestChip && bWIFICfg){ // Normal chip && wmm
		
		//	BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA 
		//{  0, 	1, 	0, 	1, 	0, 	0, 	0, 	0, 		0	};
		//0:H(end_number=0x02), 1:L (end_number=0x03)
		
		pHalData->Queue2EPNum[0] = 0x02;//VO
		pHalData->Queue2EPNum[1] = 0x03;//VI
		pHalData->Queue2EPNum[2] = 0x02;//BE
		pHalData->Queue2EPNum[3] = 0x03;//BK
		
		pHalData->Queue2EPNum[4] = 0x02;//TS
		pHalData->Queue2EPNum[5] = 0x02;//MGT
		pHalData->Queue2EPNum[6] = 0x02;//BMC
		pHalData->Queue2EPNum[7] = 0x02;//BCN
		
	}
	else{//typical setting

		
		//BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA 
		//{  1, 	1, 	0, 	0, 	0, 	0, 	0, 	0, 		0	};			
		//0:H(end_number=0x02), 1:L (end_number=0x03)
		
		pHalData->Queue2EPNum[0] = 0x02;//VO
		pHalData->Queue2EPNum[1] = 0x02;//VI
		pHalData->Queue2EPNum[2] = 0x03;//BE
		pHalData->Queue2EPNum[3] = 0x03;//BK
		
		pHalData->Queue2EPNum[4] = 0x02;//TS
		pHalData->Queue2EPNum[5] = 0x02;//MGT
		pHalData->Queue2EPNum[6] = 0x02;//BMC
		pHalData->Queue2EPNum[7] = 0x02;//BCN	
		
	}
	
}


static VOID _ThreeOutEpMapping(
	IN	HAL_DATA_TYPE	*pHalData,
	IN	BOOLEAN	 		bWIFICfg
	)
{
	if(bWIFICfg){//for WMM
		
		//	BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA 
		//{  1, 	2, 	1, 	0, 	0, 	0, 	0, 	0, 		0	};
		//0:H(end_number=0x02), 1:N(end_number=0x03), 2:L (end_number=0x05)
		
		pHalData->Queue2EPNum[0] = 0x02;//VO
		pHalData->Queue2EPNum[1] = 0x03;//VI
		pHalData->Queue2EPNum[2] = 0x05;//BE
		pHalData->Queue2EPNum[3] = 0x03;//BK
		
		pHalData->Queue2EPNum[4] = 0x02;//TS
		pHalData->Queue2EPNum[5] = 0x02;//MGT
		pHalData->Queue2EPNum[6] = 0x02;//BMC
		pHalData->Queue2EPNum[7] = 0x02;//BCN
		
	}
	else{//typical setting

		
		//	BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA 
		//{  2, 	2, 	1, 	0, 	0, 	0, 	0, 	0, 		0	};			
		//0:H(end_number=0x02), 1:N(end_number=0x03), 2:L (end_number=0x05)
		
		pHalData->Queue2EPNum[0] = 0x02;//VO
		pHalData->Queue2EPNum[1] = 0x03;//VI
		pHalData->Queue2EPNum[2] = 0x05;//BE
		pHalData->Queue2EPNum[3] = 0x05;//BK
		
		pHalData->Queue2EPNum[4] = 0x02;//TS
		pHalData->Queue2EPNum[5] = 0x02;//MGT
		pHalData->Queue2EPNum[6] = 0x02;//BMC
		pHalData->Queue2EPNum[7] = 0x02;//BCN	
	}

}

static BOOLEAN
_MappingOutEP(
	IN	PADAPTER	pAdapter,
	IN	u8		NumOutPipe,
	IN	BOOLEAN		IsTestChip
	)
{		
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct registry_priv *pregistrypriv = &pAdapter->registrypriv;

	BOOLEAN	 bWIFICfg = (pregistrypriv->wifi_spec) ?_TRUE:_FALSE;
	
	BOOLEAN result = _TRUE;

	switch(NumOutPipe)
	{
		case 2:
			_TwoOutEpMapping(IsTestChip, pHalData, bWIFICfg);
			break;
		case 3:
			// Test chip doesn't support three out EPs.
			if(IsTestChip){
				return _FALSE;
			}			
			_ThreeOutEpMapping(pHalData, bWIFICfg);
			break;
		case 1:
			_OneOutEpMapping(pHalData);
			break;
		default:
			result = _FALSE;
			break;
	}

	return result;
	
}

static VOID
_ConfigTestChipOutEP(
	IN	PADAPTER	pAdapter,
	IN	u8		NumOutPipe
	)
{
	u8			value8,txqsele;
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(pAdapter);

	pHalData->OutEpQueueSel = 0;
	pHalData->OutEpNumber	= 0;

	value8 = read8(pAdapter, REG_TEST_SIE_OPTIONAL);
	value8 = (value8 & USB_TEST_EP_MASK) >> USB_TEST_EP_SHIFT;
	
	switch(value8)
	{
		case 0:		// 2 bulk OUT, 1 bulk IN
		case 3:		
			pHalData->OutEpQueueSel = TX_SELE_HQ | TX_SELE_LQ;
			pHalData->OutEpNumber	= 2;
			//RT_TRACE(COMP_INIT,  DBG_LOUD, ("EP Config: 2 bulk OUT, 1 bulk IN\n"));
			break;
		case 1:		// 1 bulk IN/OUT => map all endpoint to Low queue
		case 2:		// 1 bulk IN, 1 bulk OUT => map all endpoint to High queue
			txqsele = read8(pAdapter, REG_TEST_USB_TXQS);
			if(txqsele & 0x0F){//map all endpoint to High queue
				pHalData->OutEpQueueSel  = TX_SELE_HQ;
			}
			else if(txqsele&0xF0){//map all endpoint to Low queue
				pHalData->OutEpQueueSel  =  TX_SELE_LQ;
			}
			pHalData->OutEpNumber	= 1;
			//RT_TRACE(COMP_INIT,  DBG_LOUD, ("%s\n", ((1 == value8) ? "1 bulk IN/OUT" : "1 bulk IN, 1 bulk OUT")));
			break;
		default:
			break;
	}

	// TODO: Error recovery for this case
	//RT_ASSERT((NumOutPipe == pHalData->OutEpNumber), ("Out EP number isn't match! %d(Descriptor) != %d (SIE reg)\n", (u4Byte)NumOutPipe, (u4Byte)pHalData->OutEpNumber));

}



static VOID
_ConfigNormalChipOutEP(
	IN	PADAPTER	pAdapter,
	IN	u8		NumOutPipe
	)
{
	u8			value8;
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(pAdapter);

	pHalData->OutEpQueueSel = 0;
	pHalData->OutEpNumber	= 0;
		
	// Normal and High queue
	value8 = read8(pAdapter, (REG_NORMAL_SIE_EP + 1));
	
	if(value8 & USB_NORMAL_SIE_EP_MASK){
		pHalData->OutEpQueueSel |= TX_SELE_HQ;
		pHalData->OutEpNumber++;
	}
	
	if((value8 >> USB_NORMAL_SIE_EP_SHIFT) & USB_NORMAL_SIE_EP_MASK){
		pHalData->OutEpQueueSel |= TX_SELE_NQ;
		pHalData->OutEpNumber++;
	}
	
	// Low queue
	value8 = read8(pAdapter, (REG_NORMAL_SIE_EP + 2));
	if(value8 & USB_NORMAL_SIE_EP_MASK){
		pHalData->OutEpQueueSel |= TX_SELE_LQ;
		pHalData->OutEpNumber++;
	}

	// TODO: Error recovery for this case
	//RT_ASSERT((NumOutPipe == pHalData->OutEpNumber), ("Out EP number isn't match! %d(Descriptor) != %d (SIE reg)\n", (u4Byte)NumOutPipe, (u4Byte)pHalData->OutEpNumber));

}

static BOOLEAN HalUsbSetQueuePipeMapping8192CUsb(
	IN	PADAPTER	pAdapter,
	IN	u8		NumInPipe,
	IN	u8		NumOutPipe
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(pAdapter);
	BOOLEAN			result		= _FALSE;
	BOOLEAN			isNormalChip;

	// ReadAdapterInfo8192C also call _ReadChipVersion too.
	// Since we need dynamic config EP mapping, so we call this function to get chip version.
	// We can remove _ReadChipVersion from ReadAdapterInfo8192C later.
	rtl8192c_ReadChipVersion(pAdapter);

	isNormalChip = IS_NORMAL_CHIP(pHalData->VersionID);

	if(isNormalChip){
		_ConfigNormalChipOutEP(pAdapter, NumOutPipe);
	}
	else{
		_ConfigTestChipOutEP(pAdapter, NumOutPipe);
	}

	// Normal chip with one IN and one OUT doesn't have interrupt IN EP.
	if(isNormalChip && (1 == pHalData->OutEpNumber)){
		if(1 != NumInPipe){
			return result;
		}
	}

	// All config other than above support one Bulk IN and one Interrupt IN.
	//if(2 != NumInPipe){
	//	return result;
	//}

	result = _MappingOutEP(pAdapter, NumOutPipe, !isNormalChip);
	
	return result;

}

void rtl8192cu_interface_configure(_adapter *padapter)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(padapter);

#if USB_TX_AGGREGATION_92C
	pHalData->UsbTxAggMode		= 1;
	pHalData->UsbTxAggDescNum	= 0x6;	// only 4 bits
#endif

#if USB_RX_AGGREGATION_92C
	pHalData->UsbRxAggMode		= USB_RX_AGG_DMA_USB;// USB_RX_AGG_DMA;
	pHalData->UsbRxAggBlockCount	= 8; //unit : 512b
	pHalData->UsbRxAggBlockTimeout	= 0x6;
	pHalData->UsbRxAggPageCount	= 48; //uint :128 b //0x0A;	// 10 = MAX_RX_DMA_BUFFER_SIZE/2/pHalData->UsbBulkOutSize
	pHalData->UsbRxAggPageTimeout	= 0x4; //6, absolute time = 34ms/(2^6)
#endif

	HalUsbSetQueuePipeMapping8192CUsb(padapter,
				(u8)pHalData->RtNumInPipes, (u8)pHalData->RtNumOutPipes);

}

static u8 _InitPowerOn(_adapter *padapter)
{
	u8	ret = _SUCCESS;
	u32	value32;
	u16	value16=0;
	u8	value8 = 0;

	// polling autoload done.
	u32	pollingCount = 0;

	do
	{
		if(read8(padapter, REG_APS_FSMCO) & PFM_ALDN){
			//RT_TRACE(COMP_INIT,DBG_LOUD,("Autoload Done!\n"));
			break;
		}

		if(pollingCount++ > POLLING_READY_TIMEOUT_COUNT){
			//RT_TRACE(COMP_INIT,DBG_SERIOUS,("Failed to polling REG_APS_FSMCO[PFM_ALDN] done!\n"));
			return _FAIL;
		}
				
	}while(_TRUE);


//	For hardware power on sequence.

	//0.	RSV_CTRL 0x1C[7:0] = 0x00			// unlock ISO/CLK/Power control register
	write8(padapter, REG_RSV_CTRL, 0x0);	
	// Power on when re-enter from IPS/Radio off/card disable
	write8(padapter, REG_SPS0_CTRL, 0x2b);//enable SPS into PWM mode
/*
	value16 = PlatformIORead2Byte(Adapter, REG_AFE_XTAL_CTRL);//enable AFE clock
	value16 &=  (~XTAL_GATE_AFE);
	PlatformIOWrite2Byte(Adapter,REG_AFE_XTAL_CTRL, value16 );		
*/
	
	udelay_os(100);//PlatformSleepUs(150);//this is not necessary when initially power on

	value8 = read8(padapter, REG_LDOV12D_CTRL);	
	if(0== (value8 & LDV12_EN) ){
		value8 |= LDV12_EN;
		write8(padapter, REG_LDOV12D_CTRL, value8);	
		//RT_TRACE(COMP_INIT, DBG_LOUD, (" power-on :REG_LDOV12D_CTRL Reg0x21:0x%02x.\n",value8));
		udelay_os(100);//PlatformSleepUs(100);//this is not necessary when initially power on
		value8 = read8(padapter, REG_SYS_ISO_CTRL);
		value8 &= ~ISO_MD2PP;
		write8(padapter, REG_SYS_ISO_CTRL, value8);			
	}	
	
	// auto enable WLAN
	pollingCount = 0;
	value16 = read16(padapter, REG_APS_FSMCO);
	value16 |= APFM_ONMAC;
	write16(padapter, REG_APS_FSMCO, value16);

	do
	{
		if(0 == (read16(padapter, REG_APS_FSMCO) & APFM_ONMAC)){
			//RT_TRACE(COMP_INIT,DBG_LOUD,("MAC auto ON okay!\n"));
			break;
		}

		if(pollingCount++ > POLLING_READY_TIMEOUT_COUNT){
			//RT_TRACE(COMP_INIT,DBG_SERIOUS,("Failed to polling REG_APS_FSMCO[APFM_ONMAC] done!\n"));
			return _FAIL;
		}
				
	}while(_TRUE);

	//Enable Radio ,GPIO ,and LED function
	write16(padapter,REG_APS_FSMCO,0x0812);

	// release RF digital isolation
	value16 = read16(padapter, REG_SYS_ISO_CTRL);
	value16 &= ~ISO_DIOR;
	write16(padapter, REG_SYS_ISO_CTRL, value16);

	// Enable MAC DMA/WMAC/SCHEDULE/SEC block
	value16 = read16(padapter, REG_CR);
	value16 |= (HCI_TXDMA_EN | HCI_RXDMA_EN | TXDMA_EN | RXDMA_EN
				| PROTOCOL_EN | SCHEDULE_EN | MACTXEN | MACRXEN | ENSEC);
	write16(padapter, REG_CR, value16);
	
	//tynli_test for suspend mode.
	{
		write8(padapter,  0xfe10, 0x19);
	}
	


	return ret;

}


static void _dbg_dump_macreg(_adapter *padapter)
{
	u32 offset = 0;
	u32 val32 = 0;
	u32 index =0 ;
	for(index=0;index<64;index++)
	{
		offset = index*4;
		val32 = read32(padapter,offset);
		printk("offset : 0x%02x ,val:0x%08x\n",offset,val32);
	}
}


static void _InitPABias(_adapter *padapter)
{
	HAL_DATA_TYPE		*pHalData	= GET_HAL_DATA(padapter);
	u8			pa_setting;
	BOOLEAN		isNormal = IS_NORMAL_CHIP(pHalData->VersionID);
	BOOLEAN		is92C = IS_92C_SERIAL(pHalData->VersionID);
	
	//FIXED PA current issue	
	efuse_one_byte_read(padapter, 0x1FA, &pa_setting);

	//RT_TRACE(COMP_INIT, DBG_LOUD, ("_InitPABias 0x1FA 0x%x \n",pa_setting));

	if(!(pa_setting & BIT0))
	{
		PHY_SetRFReg(padapter, RF90_PATH_A, 0x15, 0x0FFFFF, 0x0F406);
		PHY_SetRFReg(padapter, RF90_PATH_A, 0x15, 0x0FFFFF, 0x4F406);		
		PHY_SetRFReg(padapter, RF90_PATH_A, 0x15, 0x0FFFFF, 0x8F406);		
		PHY_SetRFReg(padapter, RF90_PATH_A, 0x15, 0x0FFFFF, 0xCF406);		
		//RT_TRACE(COMP_INIT, DBG_LOUD, ("PA BIAS path A\n"));
	}	

	if(!(pa_setting & BIT1) && isNormal && is92C)
	{
		PHY_SetRFReg(padapter,RF90_PATH_B, 0x15, 0x0FFFFF, 0x0F406);
		PHY_SetRFReg(padapter,RF90_PATH_B, 0x15, 0x0FFFFF, 0x4F406);		
		PHY_SetRFReg(padapter,RF90_PATH_B, 0x15, 0x0FFFFF, 0x8F406);		
		PHY_SetRFReg(padapter,RF90_PATH_B, 0x15, 0x0FFFFF, 0xCF406);
		//RT_TRACE(COMP_INIT, DBG_LOUD, ("PA BIAS path B\n"));	
	}

	if(!(pa_setting & BIT4))
	{
		pa_setting = read8(padapter, 0x16);
		pa_setting &= 0x0F; 
		write8(padapter, 0x16, pa_setting | 0x90);		
	}
}
#ifdef CONFIG_BT_COEXIST
static void _InitBTCoexist(_adapter *padapter)
{
	struct btcoexist_priv	 *pbtpriv = &(padapter->bt_coexist);
	u8 u1Tmp;

	if(pbtpriv->BT_Coexist && pbtpriv->BT_CoexistType == BT_CSR)
	{

#if MP_DRIVER != 1
		if(pbtpriv->BT_Ant_isolation)
		{
			write8( padapter,REG_GPIO_MUXCFG, 0xa0);
			printk("BT write 0x%x = 0x%x\n", REG_GPIO_MUXCFG, 0xa0);
		}
#endif		

		u1Tmp = read8(padapter, 0x4fd) & BIT0;
		u1Tmp = u1Tmp | 
				((pbtpriv->BT_Ant_isolation==1)?0:BIT1) | 
				((pbtpriv->BT_Service==BT_SCO)?0:BIT2);
		write8( padapter, 0x4fd, u1Tmp);
		printk("BT write 0x%x = 0x%x for non-isolation\n", 0x4fd, u1Tmp);
		
		
		write32(padapter, REG_BT_COEX_TABLE+4, 0xaaaa9aaa);
		printk("BT write 0x%x = 0x%x\n", REG_BT_COEX_TABLE+4, 0xaaaa9aaa);
		
		write32(padapter, REG_BT_COEX_TABLE+8, 0xffbd0040);
		printk("BT write 0x%x = 0x%x\n", REG_BT_COEX_TABLE+8, 0xffbd0040);

		write32(padapter,  REG_BT_COEX_TABLE+0xc, 0x40000010);
		printk("BT write 0x%x = 0x%x\n", REG_BT_COEX_TABLE+0xc, 0x40000010);

		//Config to 1T1R
		u1Tmp =  read8(padapter,rOFDM0_TRxPathEnable);
		u1Tmp &= ~(BIT1);
		write8( padapter, rOFDM0_TRxPathEnable, u1Tmp);
		printk("BT write 0xC04 = 0x%x\n", u1Tmp);
			
		u1Tmp = read8(padapter, rOFDM1_TRxPathEnable);
		u1Tmp &= ~(BIT1);
		write8( padapter, rOFDM1_TRxPathEnable, u1Tmp);
		printk("BT write 0xD04 = 0x%x\n", u1Tmp);

	}
}
#endif

//-------------------------------------------------------------------------
//
// LLT R/W/Init function
//
//-------------------------------------------------------------------------
static u8 _LLTWrite(
	IN  PADAPTER	Adapter,
	IN	u32		address,
	IN	u32		data
	)
{
	u8	status = _SUCCESS;
	int 		count = 0;
	u32 		value = _LLT_INIT_ADDR(address) | _LLT_INIT_DATA(data) | _LLT_OP(_LLT_WRITE_ACCESS);

	write32(Adapter, REG_LLT_INIT, value);
	
	//polling
	do{
		
		value = read32(Adapter, REG_LLT_INIT);
		if(_LLT_NO_ACTIVE == _LLT_OP_VALUE(value)){
			break;
		}
		
		if(count > POLLING_LLT_THRESHOLD){
			//RT_TRACE(COMP_INIT,DBG_SERIOUS,("Failed to polling write LLT done at address %d!\n", address));
			status = _FAIL;
			break;
		}
	}while(count++);

	return status;
	
}


static u8 _LLTRead(
	IN  PADAPTER	Adapter,
	IN	u32		address
	)
{
	int		count = 0;
	u32		value = _LLT_INIT_ADDR(address) | _LLT_OP(_LLT_READ_ACCESS);

	write32(Adapter, REG_LLT_INIT, value);

	//polling and get value
	do{
		
		value = read32(Adapter, REG_LLT_INIT);
		if(_LLT_NO_ACTIVE == _LLT_OP_VALUE(value)){
			return (u8)value;
		}
		
		if(count > POLLING_LLT_THRESHOLD){
			//RT_TRACE(COMP_INIT,DBG_SERIOUS,("Failed to polling read LLT done at address %d!\n", address));
			break;
		}
	}while(count++);

	return 0xFF;

}


static u8 InitLLTTable(
	IN  PADAPTER	Adapter,
	IN	u32		boundary
	)
{
	u8	status = _SUCCESS;
	u32		i;

	for(i = 0 ; i < (boundary - 1) ; i++){
		status = _LLTWrite(Adapter, i , i + 1);
		if(_SUCCESS != status){
			return status;
		}
	}

	// end of list
	status = _LLTWrite(Adapter, (boundary - 1), 0xFF); 
	if(_SUCCESS != status){
		return status;
	}

	// Make the other pages as ring buffer
	// This ring buffer is used as beacon buffer if we config this MAC as two MAC transfer.
	// Otherwise used as local loopback buffer. 
	for(i = boundary ; i < LAST_ENTRY_OF_TX_PKT_BUFFER ; i++){
		status = _LLTWrite(Adapter, i, (i + 1)); 
		if(_SUCCESS != status){
			return status;
		}
	}
	
	// Let last entry point to the start entry of ring buffer
	status = _LLTWrite(Adapter, LAST_ENTRY_OF_TX_PKT_BUFFER, boundary);
	if(_SUCCESS != status){
		return status;
	}

	return status;
	
}


//---------------------------------------------------------------
//
//	MAC init functions
//
//---------------------------------------------------------------
static VOID
_SetMacID(
	IN  PADAPTER Adapter, u8* MacID
	)
{
	u32 i;
	for(i=0 ; i< MAC_ADDR_LEN ; i++){
		write32(Adapter, REG_MACID+i, MacID[i]);
	}
}

static VOID
_SetBSSID(
	IN  PADAPTER Adapter, u8* BSSID
	)
{
	u32 i;
	for(i=0 ; i< MAC_ADDR_LEN ; i++){
		write32(Adapter, REG_BSSID+i, BSSID[i]);
	}
}


// Shall USB interface init this?
static VOID
_InitInterrupt(
	IN  PADAPTER Adapter
	)
{
	u32	value32;

	// HISR - turn all on
	value32 = 0xFFFFFFFF;
	write32(Adapter, REG_HISR, value32);

	// HIMR - turn all on
	write32(Adapter, REG_HIMR, value32);
}


static VOID
_InitQueueReservedPage(
	IN  PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);
	struct registry_priv *pregistrypriv = &Adapter->registrypriv;
	BOOLEAN			isNormalChip = IS_NORMAL_CHIP(pHalData->VersionID);
	
	u32			outEPNum	= (u32)pHalData->OutEpNumber;
	u32			numHQ		= 0;
	u32			numLQ		= 0;
	u32			numNQ		= 0;
	u32			numPubQ;
	u32			value32;
	u8			value8;
	u32			txQPageNum, txQPageUnit,txQRemainPage;

	if(!pregistrypriv->wifi_spec){		
		numPubQ = (isNormalChip) ? NORMAL_PAGE_NUM_PUBQ : TEST_PAGE_NUM_PUBQ;
		//RT_ASSERT((numPubQ < TX_TOTAL_PAGE_NUMBER), ("Public queue page number is great than total tx page number.\n"));
		txQPageNum = TX_TOTAL_PAGE_NUMBER - numPubQ;

		//RT_ASSERT((0 == txQPageNum%txQPageNum), ("Total tx page number is not dividable!\n"));
		
		txQPageUnit = txQPageNum/outEPNum;
		txQRemainPage = txQPageNum % outEPNum;

		if(pHalData->OutEpQueueSel & TX_SELE_HQ){
			numHQ = txQPageUnit;
		}
		if(pHalData->OutEpQueueSel & TX_SELE_LQ){
			numLQ = txQPageUnit;
		}
		// HIGH priority queue always present in the configuration of 2 or 3 out-ep 
		// so ,remainder pages have assigned to High queue
		if((outEPNum>1) && (txQRemainPage)){			
			numHQ += txQRemainPage;
		}

		// NOTE: This step shall be proceed before writting REG_RQPN.
		if(isNormalChip){
			if(pHalData->OutEpQueueSel & TX_SELE_NQ){
				numNQ = txQPageUnit;
			}
			value8 = (u8)_NPQ(numNQ);
			write8(Adapter, REG_RQPN_NPQ, value8);
		}
		//RT_ASSERT(((numHQ + numLQ + numNQ + numPubQ) < TX_PAGE_BOUNDARY), ("Total tx page number is greater than tx boundary!\n"));
	}
	else{ //for WMM 
		//RT_ASSERT((outEPNum>=2), ("for WMM ,number of out-ep must more than or equal to 2!\n"));
		
		numPubQ = (isNormalChip) 	?WMM_NORMAL_PAGE_NUM_PUBQ
								:WMM_TEST_PAGE_NUM_PUBQ;		
		
		if(pHalData->OutEpQueueSel & TX_SELE_HQ){
			numHQ = (isNormalChip)?WMM_NORMAL_PAGE_NUM_HPQ
								:WMM_TEST_PAGE_NUM_HPQ;
		}

		if(pHalData->OutEpQueueSel & TX_SELE_LQ){
			numLQ = (isNormalChip)?WMM_NORMAL_PAGE_NUM_LPQ
								:WMM_TEST_PAGE_NUM_LPQ;
		}
		// NOTE: This step shall be proceed before writting REG_RQPN.
		if(isNormalChip){			
			if(pHalData->OutEpQueueSel & TX_SELE_NQ){
				numNQ = WMM_NORMAL_PAGE_NUM_NPQ;
			}
			value8 = (u8)_NPQ(numNQ);
			write8(Adapter, REG_RQPN_NPQ, value8);
		}
	}

	// TX DMA
	value32 = _HPQ(numHQ) | _LPQ(numLQ) | _PUBQ(numPubQ) | LD_RQPN;	
	write32(Adapter, REG_RQPN, value32);	
}

static void _InitID(IN  PADAPTER Adapter)
{
	int i;	 
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	
	for(i=0; i<6; i++)
	{
		write8(Adapter, (REG_MACID+i), pEEPROM->mac_addr[i]);		 	
	}

/*
	NicIFSetMacAddress(Adapter, Adapter->PermanentAddress);
	//Ziv test
#if 1
	{
		u1Byte sMacAddr[6] = {0};
		u4Byte i;
		
		for(i = 0 ; i < MAC_ADDR_LEN ; i++){
			sMacAddr[i] = PlatformIORead1Byte(Adapter, (REG_MACID + i));
		}
		RT_PRINT_ADDR(COMP_INIT|COMP_EFUSE, DBG_LOUD, "Read back MAC Addr: ", sMacAddr);
	}
#endif

#if 0
	u4Byte nMAR 	= 0xFFFFFFFF;
	u8 m_MacID[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	u8 m_BSSID[] = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	int i;
	
	_SetMacID(Adapter, Adapter->PermanentAddress);
	_SetBSSID(Adapter, m_BSSID);

	//set MAR
	PlatformIOWrite4Byte(Adapter, REG_MAR, nMAR);
	PlatformIOWrite4Byte(Adapter, REG_MAR+4, nMAR);
#endif
*/
}


static VOID
_InitTxBufferBoundary(
	IN  PADAPTER Adapter
	)
{	
	struct registry_priv *pregistrypriv = &Adapter->registrypriv;
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);

	u16	txdmactrl;	
	u8	txpktbuf_bndy; 

	if(!pregistrypriv->wifi_spec){
		txpktbuf_bndy = TX_PAGE_BOUNDARY;
	}
	else{//for WMM
		txpktbuf_bndy = ( IS_NORMAL_CHIP( pHalData->VersionID))?WMM_NORMAL_TX_PAGE_BOUNDARY
															:WMM_TEST_TX_PAGE_BOUNDARY;
	}

	write8(Adapter, REG_TXPKTBUF_BCNQ_BDNY, txpktbuf_bndy);
	write8(Adapter, REG_TXPKTBUF_MGQ_BDNY, txpktbuf_bndy);
	write8(Adapter, REG_TXPKTBUF_WMAC_LBK_BF_HD, txpktbuf_bndy);
	write8(Adapter, REG_TRXFF_BNDY, txpktbuf_bndy);	
#if 1
	write8(Adapter, REG_TDECTRL+1, txpktbuf_bndy);
#else
	txdmactrl = PlatformIORead2Byte(Adapter, REG_TDECTRL);
	txdmactrl &= ~BCN_HEAD_MASK;
	txdmactrl |= BCN_HEAD(txpktbuf_bndy);
	PlatformIOWrite2Byte(Adapter, REG_TDECTRL, txdmactrl);
#endif
}

static VOID
_InitPageBoundary(
	IN  PADAPTER Adapter
	)
{
	// RX Page Boundary
	//srand(static_cast<unsigned int>(time(NULL)) );
	u16 rxff_bndy = 0x27FF;//(rand() % 1) ? 0x27FF : 0x23FF;

	write16(Adapter, (REG_TRXFF_BNDY + 2), rxff_bndy);

	// TODO: ?? shall we set tx boundary?
}


static VOID
_InitNormalChipRegPriority(
	IN	PADAPTER	Adapter,
	IN	u16		beQ,
	IN	u16		bkQ,
	IN	u16		viQ,
	IN	u16		voQ,
	IN	u16		mgtQ,
	IN	u16		hiQ
	)
{
	u16 value16		= (read16(Adapter, REG_TRXDMA_CTRL) & 0x7);

	value16 |=	_TXDMA_BEQ_MAP(beQ) 	| _TXDMA_BKQ_MAP(bkQ) |
				_TXDMA_VIQ_MAP(viQ) 	| _TXDMA_VOQ_MAP(voQ) |
				_TXDMA_MGQ_MAP(mgtQ)| _TXDMA_HIQ_MAP(hiQ);
	
	write16(Adapter, REG_TRXDMA_CTRL, value16);
}

static VOID
_InitNormalChipOneOutEpPriority(
	IN	PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);

	u16	value = 0;
	switch(pHalData->OutEpQueueSel)
	{
		case TX_SELE_HQ:
			value = QUEUE_HIGH;
			break;
		case TX_SELE_LQ:
			value = QUEUE_LOW;
			break;
		case TX_SELE_NQ:
			value = QUEUE_NORMAL;
			break;
		default:
			//RT_ASSERT(FALSE,("Shall not reach here!\n"));
			break;
	}
	
	_InitNormalChipRegPriority(Adapter,
								value,
								value,
								value,
								value,
								value,
								value
								);

}

static VOID
_InitNormalChipTwoOutEpPriority(
	IN	PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);
	struct registry_priv *pregistrypriv = &Adapter->registrypriv;
	u16			beQ,bkQ,viQ,voQ,mgtQ,hiQ;
	

	u16	valueHi = 0;
	u16	valueLow = 0;
	
	switch(pHalData->OutEpQueueSel)
	{
		case (TX_SELE_HQ | TX_SELE_LQ):
			valueHi = QUEUE_HIGH;
			valueLow = QUEUE_LOW;
			break;
		case (TX_SELE_NQ | TX_SELE_LQ):
			valueHi = QUEUE_NORMAL;
			valueLow = QUEUE_LOW;
			break;
		case (TX_SELE_HQ | TX_SELE_NQ):
			valueHi = QUEUE_HIGH;
			valueLow = QUEUE_NORMAL;
			break;
		default:
			//RT_ASSERT(FALSE,("Shall not reach here!\n"));
			break;
	}

	if(!pregistrypriv->wifi_spec ){
		beQ 		= valueLow;
		bkQ 		= valueLow;
		viQ		= valueHi;
		voQ 		= valueHi;
		mgtQ 	= valueHi; 
		hiQ 		= valueHi;								
	}
	else{//for WMM ,CONFIG_OUT_EP_WIFI_MODE
		beQ		= valueHi;
		bkQ 		= valueLow;		
		viQ 		= valueLow;
		voQ 		= valueHi;
		mgtQ 	= valueHi;
		hiQ 		= valueHi;							
	}
	
	_InitNormalChipRegPriority(Adapter,beQ,bkQ,viQ,voQ,mgtQ,hiQ);

}

static VOID
_InitNormalChipThreeOutEpPriority(
	IN	PADAPTER Adapter
	)
{
	struct registry_priv *pregistrypriv = &Adapter->registrypriv;
	u16			beQ,bkQ,viQ,voQ,mgtQ,hiQ;

	if(!pregistrypriv->wifi_spec ){// typical setting
		beQ		= QUEUE_LOW;
		bkQ 		= QUEUE_LOW;
		viQ 		= QUEUE_NORMAL;
		voQ 		= QUEUE_HIGH;
		mgtQ 	= QUEUE_HIGH;
		hiQ 		= QUEUE_HIGH;			
	}
	else{// for WMM
		beQ		= QUEUE_LOW;
		bkQ 		= QUEUE_NORMAL;
		viQ 		= QUEUE_NORMAL;
		voQ 		= QUEUE_HIGH;
		mgtQ 	= QUEUE_HIGH;
		hiQ 		= QUEUE_HIGH;			
	}
	_InitNormalChipRegPriority(Adapter,beQ,bkQ,viQ,voQ,mgtQ,hiQ);
}

static VOID
_InitNormalChipQueuePriority(
	IN	PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);

	switch(pHalData->OutEpNumber)
	{
		case 1:
			_InitNormalChipOneOutEpPriority(Adapter);
			break;
		case 2:
			_InitNormalChipTwoOutEpPriority(Adapter);
			break;
		case 3:
			_InitNormalChipThreeOutEpPriority(Adapter);
			break;
		default:
			//RT_ASSERT(FALSE,("Shall not reach here!\n"));
			break;
	}


}

static VOID
_InitTestChipQueuePriority(
	IN	PADAPTER Adapter
	)
{
	u8	hq_sele ;
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);
	struct registry_priv *pregistrypriv = &Adapter->registrypriv;
	
	switch(pHalData->OutEpNumber)
	{
		case 2:	// (TX_SELE_HQ|TX_SELE_LQ)
			if(!pregistrypriv->wifi_spec)//typical setting			
				hq_sele =  HQSEL_VOQ | HQSEL_VIQ | HQSEL_MGTQ | HQSEL_HIQ ;
			else	//for WMM
				hq_sele = HQSEL_VOQ | HQSEL_BEQ | HQSEL_MGTQ | HQSEL_HIQ ;
			break;
		case 1:
			if(TX_SELE_LQ == pHalData->OutEpQueueSel ){//map all endpoint to Low queue
				 hq_sele = 0;
			}
			else if(TX_SELE_HQ == pHalData->OutEpQueueSel){//map all endpoint to High queue
				hq_sele =  HQSEL_VOQ | HQSEL_VIQ | HQSEL_BEQ | HQSEL_BKQ | HQSEL_MGTQ | HQSEL_HIQ ;
			}		
			break;
		default:
			//RT_ASSERT(FALSE,("Shall not reach here!\n"));
			break;
	}
	write8(Adapter, (REG_TRXDMA_CTRL+1), hq_sele);
}


static VOID
_InitQueuePriority(
	IN  PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);

	if(IS_NORMAL_CHIP( pHalData->VersionID)){
		_InitNormalChipQueuePriority(Adapter);
	}
	else{
		_InitTestChipQueuePriority(Adapter);
	}
}

static VOID
_InitHardwareDropIncorrectBulkOut(
	IN  PADAPTER Adapter
	)
{
	u32	value32 = read32(Adapter, REG_TXDMA_OFFSET_CHK);
	value32 |= DROP_DATA_EN;
	write32(Adapter, REG_TXDMA_OFFSET_CHK, value32);
}

static VOID
_InitNetworkType(
	IN  PADAPTER Adapter
	)
{
	u32	value32;

	value32 = read32(Adapter, REG_CR);

	// TODO: use the other function to set network type
#if RTL8191C_FPGA_NETWORKTYPE_ADHOC
	value32 = (value32 & ~MASK_NETTYPE) | _NETTYPE(NT_LINK_AD_HOC);
#else
	value32 = (value32 & ~MASK_NETTYPE) | _NETTYPE(NT_LINK_AP);
#endif
	write32(Adapter, REG_CR, value32);
//	RASSERT(pIoBase->read8(REG_CR + 2) == 0x2);
}

static VOID
_InitTransferPageSize(
	IN  PADAPTER Adapter
	)
{
	// Tx page size is always 128.
	
	u8	value8;
	value8 = _PSRX(PBP_128) | _PSTX(PBP_128);
	write8(Adapter, REG_PBP, value8);
}

static VOID
_InitDriverInfoSize(
	IN  PADAPTER	Adapter,
	IN	u8		drvInfoSize
	)
{
	write8(Adapter,REG_RX_DRVINFO_SZ, drvInfoSize);
}

static VOID
_InitWMACSetting(
	IN  PADAPTER Adapter
	)
{
	//u4Byte			value32;
	u16			value16;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	//pHalData->ReceiveConfig = AAP | APM | AM | AB | APP_ICV | ADF | AMF | APP_FCS | HTC_LOC_CTRL | APP_MIC | APP_PHYSTS;
	pHalData->ReceiveConfig = RCR_AAP | RCR_APM | RCR_AM | RCR_AB | RCR_APP_ICV | RCR_AMF | RCR_HTC_LOC_CTRL | RCR_APP_MIC | RCR_APP_PHYSTS;
#if (0 == RTL8192C_RX_PACKET_NO_INCLUDE_CRC)
	pHalData->ReceiveConfig |= ACRC32;
#endif

	// some REG_RCR will be modified later by phy_ConfigMACWithHeaderFile()
	write32(Adapter, REG_RCR, pHalData->ReceiveConfig);

	// Accept all multicast address
	write32(Adapter, REG_MAR, 0xFFFFFFFF);
	write32(Adapter, REG_MAR + 4, 0xFFFFFFFF);

	// Accept all management frames
	value16 = 0xFFFF;
	write16(Adapter, REG_RXFLTMAP0, value16);

	//Reject all control frame - default value is 0
	write16(Adapter,REG_RXFLTMAP1,0x0);

	// Accept all data frames
	value16 = 0xFFFF;
	write16(Adapter, REG_RXFLTMAP2, value16);
	
	//enable RX_SHIFT bits
	//write8(Adapter, REG_TRXDMA_CTRL, read8(Adapter, REG_TRXDMA_CTRL)|BIT(1));
	
}

static VOID
_InitAdaptiveCtrl(
	IN  PADAPTER Adapter
	)
{
	u16	value16;
	u32	value32;

	// Response Rate Set
	value32 = read32(Adapter, REG_RRSR);
	value32 &= ~RATE_BITMAP_ALL;
	value32 |= RATE_RRSR_CCK_ONLY_1M;
	write32(Adapter, REG_RRSR, value32);

	// CF-END Threshold
	//m_spIoBase->write8(REG_CFEND_TH, 0x1);

	// SIFS (used in NAV)
	value16 = _SPEC_SIFS_CCK(0x10) | _SPEC_SIFS_OFDM(0x10);
	write16(Adapter, REG_SPEC_SIFS, value16);

	// Retry Limit
	value16 = _LRL(0x30) | _SRL(0x30);
	write16(Adapter, REG_RL, value16);
	
}

static VOID
_InitRateFallback(
	IN  PADAPTER Adapter
	)
{
	// Set Data Auto Rate Fallback Retry Count register.
	write32(Adapter, REG_DARFRC, 0x00000000);
	write32(Adapter, REG_DARFRC+4, 0x10080404);
	write32(Adapter, REG_RARFRC, 0x04030201);
	write32(Adapter, REG_RARFRC+4, 0x08070605);

}


static VOID
_InitEDCA(
	IN  PADAPTER Adapter
	)
{
	//PHAL_DATA_8192CUSB	pHalData = GetHalData8192CUsb(Adapter);
	u16				value16;

#if 1
	//disable EDCCA count down, to reduce collison and retry
	value16 = read16(Adapter, REG_RD_CTRL);
	value16 |= DIS_EDCA_CNT_DWN;
	write16(Adapter, REG_RD_CTRL, value16);	


	// Update SIFS timing.  ??????????
	//pHalData->SifsTime = 0x0e0e0a0a;
	//Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_SIFS,  (pu1Byte)&pHalData->SifsTime);

	// Set CCK/OFDM SIFS
	write16(Adapter, REG_SIFS_CCK, 0x0a0a); // CCK SIFS shall always be 10us.
	write16(Adapter, REG_SIFS_OFDM, 0x1010);
#endif

	write16(Adapter, REG_PROT_MODE_CTRL, 0x0204);

	write32(Adapter, REG_BAR_MODE_CTRL, 0x014004);


	// TXOP
	write32(Adapter, REG_EDCA_BE_PARAM, 0x005EA42B);
	write32(Adapter, REG_EDCA_BK_PARAM, 0x0000A44F);
	write32(Adapter, REG_EDCA_VI_PARAM, 0x005EA324);
	write32(Adapter, REG_EDCA_VO_PARAM, 0x002FA226);

	// PIFS
	write8(Adapter, REG_PIFS, 0x1C);
		
	//AGGR BREAK TIME Register
	write8(Adapter, REG_AGGR_BREAK_TIME, 0x16);

	write16(Adapter, REG_NAV_PROT_LEN, 0x0040);
	
	write8(Adapter, REG_BCNDMATIM, 0x02);

	write8(Adapter, REG_ATIMWND, 0x02);
	
}


static VOID
_InitAMPDUAggregation(
	IN  PADAPTER Adapter
	)
{
	write32(Adapter, REG_AGGLEN_LMT, 0x99997631);
	write8(Adapter, REG_AGGR_BREAK_TIME, 0x16);

	// init AMPDU aggregation number, tuning for Tx's TP, suggested by Scott.
	write16(Adapter, 0x4CA, 0x0708);	
}

static VOID
_InitBeaconMaxError(
	IN  PADAPTER	Adapter,
	IN	BOOLEAN		InfraMode
	)
{
	//write8(Adapter, REG_BCN_MAX_ERR, (InfraMode ? 0xFF : 0x10));	
	write8(Adapter, REG_BCN_MAX_ERR,  0xFF );
}

static VOID
_InitRDGSetting(
	IN	PADAPTER Adapter
	)
{
	write8(Adapter,REG_RD_CTRL,0xFF);
	write16(Adapter, REG_RD_NAV_NXT, 0x200);
	write8(Adapter,REG_RD_RESP_PKT_TH,0x05);
}



static VOID
_InitRetryFunction(
	IN  PADAPTER Adapter
	)
{
	u8	value8;
	
	value8 = read8(Adapter, REG_FWHW_TXQ_CTRL);
	value8 |= EN_AMPDU_RTY_NEW;
	write8(Adapter, REG_FWHW_TXQ_CTRL, value8);

	// Set ACK timeout
	write8(Adapter, REG_ACKTO, 0x40);
}


static VOID
_InitUsbAggregationSetting(
	IN  PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

#if USB_TX_AGGREGATION_92C
{
	u32			value32;

	if(pHalData->UsbTxAggMode){
		value32 = read32(Adapter, REG_TDECTRL);
		value32 = value32 & ~(BLK_DESC_NUM_MASK << BLK_DESC_NUM_SHIFT);
		value32 |= ((pHalData->UsbTxAggDescNum & BLK_DESC_NUM_MASK) << BLK_DESC_NUM_SHIFT);
		
		write32(Adapter, REG_TDECTRL, value32);
	}
}
#endif

	// Rx aggregation setting
#if USB_RX_AGGREGATION_92C
{
	u8		valueDMA;
	u8		valueUSB;

	valueDMA = read8(Adapter, REG_TRXDMA_CTRL);
	valueUSB = read8(Adapter, REG_USB_SPECIAL_OPTION);

	switch(pHalData->UsbRxAggMode)
	{
		case USB_RX_AGG_DMA:
			valueDMA |= RXDMA_AGG_EN;
			valueUSB &= ~USB_AGG_EN;
			break;
		case USB_RX_AGG_USB:
			valueDMA &= ~RXDMA_AGG_EN;
			valueUSB |= USB_AGG_EN;
			break;
		case USB_RX_AGG_DMA_USB:
			valueDMA |= RXDMA_AGG_EN;
			valueUSB |= USB_AGG_EN;
			break;
		case USB_RX_AGG_DISABLE:
		default:
			valueDMA &= ~RXDMA_AGG_EN;
			valueUSB &= ~USB_AGG_EN;
			break;
	}

	write8(Adapter, REG_TRXDMA_CTRL, valueDMA);
	write8(Adapter, REG_USB_SPECIAL_OPTION, valueUSB);

	switch(pHalData->UsbRxAggMode)
	{
		case USB_RX_AGG_DMA:
			write8(Adapter, REG_RXDMA_AGG_PG_TH, pHalData->UsbRxAggPageCount);
			write8(Adapter, REG_USB_DMA_AGG_TO, pHalData->UsbRxAggPageTimeout);
			break;
		case USB_RX_AGG_USB:
			write8(Adapter, REG_USB_AGG_TH, pHalData->UsbRxAggBlockCount);
			write8(Adapter, REG_USB_AGG_TO, pHalData->UsbRxAggBlockTimeout);
			break;
		case USB_RX_AGG_DMA_USB:
			write8(Adapter, REG_RXDMA_AGG_PG_TH, pHalData->UsbRxAggPageCount);
			write8(Adapter, REG_USB_DMA_AGG_TO, pHalData->UsbRxAggPageTimeout);
			write8(Adapter, REG_USB_AGG_TH, pHalData->UsbRxAggBlockCount);
			write8(Adapter, REG_USB_AGG_TO, pHalData->UsbRxAggBlockTimeout);
			break;
		case USB_RX_AGG_DISABLE:
		default:
			// TODO: 
			break;
	}

	switch(PBP_128)
	{
		case PBP_128:
			pHalData->HwRxPageSize = 128;
			break;
		case PBP_64:
			pHalData->HwRxPageSize = 64;
			break;
		case PBP_256:
			pHalData->HwRxPageSize = 256;
			break;
		case PBP_512:
			pHalData->HwRxPageSize = 512;
			break;
		case PBP_1024:
			pHalData->HwRxPageSize = 1024;
			break;
		default:
			//RT_ASSERT(FALSE, ("RX_PAGE_SIZE_REG_VALUE definition is incorrect!\n"));
			break;
	}

}
#endif

}


static VOID
_InitOperationMode(
	IN	PADAPTER			Adapter
	)
{
#if 0//gtest
	PHAL_DATA_8192CUSB	pHalData = GetHalData8192CUsb(Adapter);
	u1Byte				regBwOpMode = 0;
	u4Byte				regRATR = 0, regRRSR = 0;


	//1 This part need to modified according to the rate set we filtered!!
	//
	// Set RRSR, RATR, and REG_BWOPMODE registers
	//
	switch(Adapter->RegWirelessMode)
	{
		case WIRELESS_MODE_B:
			regBwOpMode = BW_OPMODE_20MHZ;
			regRATR = RATE_ALL_CCK;
			regRRSR = RATE_ALL_CCK;
			break;
		case WIRELESS_MODE_A:
			ASSERT(FALSE);
#if 0
			regBwOpMode = BW_OPMODE_5G |BW_OPMODE_20MHZ;
			regRATR = RATE_ALL_OFDM_AG;
			regRRSR = RATE_ALL_OFDM_AG;
#endif
			break;
		case WIRELESS_MODE_G:
			regBwOpMode = BW_OPMODE_20MHZ;
			regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
			regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
			break;
		case WIRELESS_MODE_AUTO:
			if (Adapter->bInHctTest)
			{
			    regBwOpMode = BW_OPMODE_20MHZ;
			    regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
			    regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
			}
			else
			{
			    regBwOpMode = BW_OPMODE_20MHZ;
			    regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
			    regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
			}
			break;
		case WIRELESS_MODE_N_24G:
			// It support CCK rate by default.
			// CCK rate will be filtered out only when associated AP does not support it.
			regBwOpMode = BW_OPMODE_20MHZ;
				regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
				regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
			break;
		case WIRELESS_MODE_N_5G:
			ASSERT(FALSE);
#if 0
			regBwOpMode = BW_OPMODE_5G;
			regRATR = RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
			regRRSR = RATE_ALL_OFDM_AG;
#endif
			break;
	}

	// Ziv ????????
	//PlatformEFIOWrite4Byte(Adapter, REG_INIRTS_RATE_SEL, regRRSR);
	PlatformEFIOWrite1Byte(Adapter, REG_BWOPMODE, regBwOpMode);

	// For Min Spacing configuration.
	switch(pHalData->RF_Type)
	{
		case RF_1T2R:
		case RF_1T1R:
			RT_TRACE(COMP_INIT, DBG_LOUD, ("Initializeadapter: RF_Type%s\n", (pHalData->RF_Type==RF_1T1R? "(1T1R)":"(1T2R)")));
			Adapter->MgntInfo.MinSpaceCfg = (MAX_MSS_DENSITY_1T<<3);						
			break;
		case RF_2T2R:
		case RF_2T2R_GREEN:
			RT_TRACE(COMP_INIT, DBG_LOUD, ("Initializeadapter:RF_Type(2T2R)\n"));
			Adapter->MgntInfo.MinSpaceCfg = (MAX_MSS_DENSITY_2T<<3);			
			break;
	}
	
	PlatformEFIOWrite1Byte(Adapter, REG_AMPDU_MIN_SPACE, Adapter->MgntInfo.MinSpaceCfg);
#endif
}


static VOID
_InitSecuritySetting(
	IN  PADAPTER Adapter
	)
{
#if 0
	//Security related.
	//-----------------------------------------------------------------------------
	// Set up security related. 070106, by rcnjko:
	// 1. Clear all H/W keys.
	// 2. Enable H/W encryption/decryption.
	//-----------------------------------------------------------------------------
	if(Adapter->ResetProgress == RESET_TYPE_NORESET && Adapter->bInSetPower == FALSE)
	{
		SecClearAllKeys(Adapter);	
		CamResetAllEntry(Adapter);
		SecInit(Adapter);    
	}
#else

	u8 ucIndex;

	// 1. Clear all H/W keys.
	for(ucIndex=0;ucIndex<TOTAL_CAM_ENTRY;ucIndex++)
		CAM_mark_invalid(Adapter, ucIndex);
	
	for(ucIndex=0;ucIndex<TOTAL_CAM_ENTRY;ucIndex++)
		CAM_empty_entry(Adapter, ucIndex);

	//
	invalidate_cam_all(Adapter);


#endif	
}

 static VOID
_InitBeaconParameters(
	IN  PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	// TODO: Remove these magic number
	write16(Adapter, REG_TBTT_PROHIBIT,0x6404);// ms
	write8(Adapter, REG_DRVERLYINT, DRIVER_EARLY_INT_TIME);//ms
	write8(Adapter, REG_BCNDMATIM, BCN_DMA_ATIME_INT_TIME);

	// Suggested by designer timchen. Change beacon AIFS to the largest number
	// beacause test chip does not contension before sending beacon. by tynli. 2009.11.03
	if(IS_NORMAL_CHIP( pHalData->VersionID)){
		write16(Adapter, REG_BCNTCFG, 0x660F);
	}
	else{		
		write16(Adapter, REG_BCNTCFG, 0x66FF);
	}

}

static VOID
_InitRFType(
	IN	PADAPTER Adapter
	)
{
	struct registry_priv	 *pregpriv = &Adapter->registrypriv;
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);
	BOOLEAN			is92CU		= IS_92C_SERIAL(pHalData->VersionID);

#if	DISABLE_BB_RF
	pHalData->rf_chip	= RF_PSEUDO_11N;
	return;
#endif

	pHalData->rf_chip	= RF_6052;

	if(pregpriv->rf_config != RF_819X_MAX_TYPE)
	{
		pHalData->rf_type = pregpriv->rf_config;
		DBG_8192C("Set RF Chip ID to RF_6052 and RF type to %d.\n", pHalData->rf_type);
		return;
	}	

	if(_TRUE == is92CU)
	{
		pHalData->rf_type = RF_2T2R;	
		DBG_8192C("Set RF Chip ID to RF_6052 and RF type to 2T2R.\n");
		//return;
	}
	else
	{
		pHalData->rf_type = RF_1T1R;
		DBG_8192C("Set RF Chip ID to RF_6052 and RF type to 1T1R.\n");
	}


	// TODO: Consider that EEPROM set 92CU to 1T1R later.
	// Force to overwrite setting according to chip version. Ignore EEPROM setting.
	//pHalData->RF_Type = is92CU ? RF_2T2R : RF_1T1R;
	//RT_TRACE(COMP_INIT,DBG_TRACE,("Set RF Chip ID to RF_6052 and RF type to %d.\n", pHalData->RF_Type));


	MSG_8192C("rf_chip=0x%x, rf_type=0x%x\n",  pHalData->rf_chip, pHalData->rf_type);

}

static VOID _InitAdhocWorkaroundParams(IN PADAPTER Adapter)
{
#if RTL8192CU_ADHOC_WORKAROUND_SETTING
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);	
	pHalData->RegBcnCtrlVal 	= read8(Adapter, REG_BCN_CTRL);
	pHalData->RegTxPause = read8(Adapter, REG_TXPAUSE); 
	pHalData->RegFwHwTxQCtrl = read8(Adapter, REG_FWHW_TXQ_CTRL+2);
	pHalData->RegReg542 = read8(Adapter, REG_TBTT_PROHIBIT+2);
#endif	
}

static VOID
_BeaconFunctionEnable(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN			Enable,
	IN	BOOLEAN			Linked
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8			value8 = 0;

	//value8 = Enable ? (EN_BCN_FUNCTION | EN_TXBCN_RPT) : EN_BCN_FUNCTION;

	if(_FALSE == Linked){		
		if(IS_NORMAL_CHIP( pHalData->VersionID)){
			value8 |= DIS_TSF_UDT0_NORMAL_CHIP;
		}
		else{
			value8 |= DIS_TSF_UDT0_TEST_CHIP;
		}
	}

	write8(Adapter, REG_BCN_CTRL, value8);
}


// Set CCK and OFDM Block "ON"
static VOID _BBTurnOnBlock(
	IN	PADAPTER		Adapter
	)
{
#if (DISABLE_BB_RF)
	return;
#endif

	PHY_SetBBReg(Adapter, rFPGA0_RFMOD, bCCKEn, 0x1);
	PHY_SetBBReg(Adapter, rFPGA0_RFMOD, bOFDMEn, 0x1);
}

static VOID _RfPowerSave(
	IN	PADAPTER		Adapter
	)
{
#if 0
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);
	PMGNT_INFO		pMgntInfo	= &(Adapter->MgntInfo);
	u1Byte			eRFPath;

#if (DISABLE_BB_RF)
	return;
#endif

	if(pMgntInfo->RegRfOff == TRUE){ // User disable RF via registry.
		RT_TRACE((COMP_INIT|COMP_RF), DBG_LOUD, ("InitializeAdapter8192CUsb(): Turn off RF for RegRfOff.\n"));
		MgntActSet_RF_State(Adapter, eRfOff, RF_CHANGE_BY_SW);
		// Those action will be discard in MgntActSet_RF_State because off the same state
		for(eRFPath = 0; eRFPath <pHalData->NumTotalRFPath; eRFPath++)
			PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, 0x4, 0xC00, 0x0);
	}
	else if(pMgntInfo->RfOffReason > RF_CHANGE_BY_PS){ // H/W or S/W RF OFF before sleep.
		RT_TRACE((COMP_INIT|COMP_RF), DBG_LOUD, ("InitializeAdapter8192CUsb(): Turn off RF for RfOffReason(%ld).\n", pMgntInfo->RfOffReason));
		MgntActSet_RF_State(Adapter, eRfOff, pMgntInfo->RfOffReason);
	}
	else{
		pHalData->eRFPowerState = eRfOn;
		pMgntInfo->RfOffReason = 0; 
		if(Adapter->bInSetPower || Adapter->bResetInProgress)
			PlatformUsbEnableInPipes(Adapter);
		RT_TRACE((COMP_INIT|COMP_RF), DBG_LOUD, ("InitializeAdapter8192CUsb(): RF is on.\n"));
	}
#endif
}


static VOID
_InitAntenna_Selection(IN PADAPTER Adapter)
{
#ifdef CONFIG_ANTENNA_DIVERSITY
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);

	if(pHalData->AntDivCfg==0)
		return;
	printk("==>  %s ....\n",__FUNCTION__);
	
	if((RF_1T1R == pHalData->rf_type)||(RF_1T2R == pHalData->rf_type))
	{
		// Force use left antenna by default for 88C.set BIT23 to enable func of antenna diversity
		write32(Adapter, REG_LEDCFG0, read32(Adapter, REG_LEDCFG0)|BIT23);	
		PHY_SetBBReg(Adapter, rFPGA0_XAB_RFParameter, BIT13, 0x01);
		
		if(PHY_QueryBBReg(Adapter, rFPGA0_XA_RFInterfaceOE, 0x300) == Antenna_A)
			pHalData->CurAntenna = Antenna_A;
		else
			pHalData->CurAntenna = Antenna_B;
		printk("%s,Cur_ant:(%x)%s\n",__FUNCTION__,pHalData->CurAntenna,(pHalData->CurAntenna == Antenna_A)?"Antenna_A":"Antenna_B");
			
	}
	
#endif
}

u32 rtl8192cu_hal_init(_adapter *padapter)
{
	u8	val8 = 0;
	u32	val32, boundary, status = _SUCCESS;
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(padapter);
	struct registry_priv *pregistrypriv = &padapter->registrypriv;
	u8	isNormal = IS_NORMAL_CHIP(pHalData->VersionID);
	u8	is92C = IS_92C_SERIAL(pHalData->VersionID);
#ifdef CONFIG_BT_COEXIST
	struct btcoexist_priv	 *pbtpriv = &(padapter->bt_coexist);
#endif
_func_enter_;

	status = _InitPowerOn(padapter);
	if(status == _FAIL){
		RT_TRACE(_module_hci_hal_init_c_, _drv_err_, ("Failed to init power on!\n"));
		goto exit;
	}

	if(!pregistrypriv->wifi_spec){
		boundary = TX_PAGE_BOUNDARY;
	}
	else{// for WMM
		boundary = (IS_NORMAL_CHIP(pHalData->VersionID))	?WMM_NORMAL_TX_PAGE_BOUNDARY
													:WMM_TEST_TX_PAGE_BOUNDARY;
	}															

	status =  InitLLTTable(padapter, boundary);
	if(status == _FAIL){
		//RT_TRACE(COMP_INIT,DBG_SERIOUS,("Failed to init power on!\n"));
		return status;
	}		
	
	_InitQueueReservedPage(padapter);
	_InitTxBufferBoundary(padapter);		
	_InitQueuePriority(padapter);
	_InitPageBoundary(padapter);	
	_InitTransferPageSize(padapter);	
	_InitDriverInfoSize(padapter, 4);// Get Rx PHY status in order to report RSSI and others.
	_InitInterrupt(padapter);	
	_InitID(padapter);//set mac_address
	_InitNetworkType(padapter);//set msr	
	_InitWMACSetting(padapter);
	_InitAdaptiveCtrl(padapter);
	_InitEDCA(padapter);
	_InitRateFallback(padapter);
	_InitRetryFunction(padapter);
	_InitUsbAggregationSetting(padapter);
	_InitOperationMode(padapter);//todo
	_InitBeaconParameters(padapter);
	_InitAMPDUAggregation(padapter);
	_InitBeaconMaxError(padapter, _TRUE);
	_BeaconFunctionEnable(padapter, _FALSE, _FALSE);
	

#if ENABLE_USB_DROP_INCORRECT_OUT
	_InitHardwareDropIncorrectBulkOut(padapter);
#endif

	if(pHalData->bRDGEnable){
		_InitRDGSetting(padapter);
	}

#if ((0 == MP_DRIVER) && RTL8192CU_FW_DOWNLOAD_ENABLE)
	status = FirmwareDownload92C(padapter);
	if(status == _FAIL)
	{

		padapter->bFWReady = _FALSE;

		pHalData->fw_ractrl = _FALSE;

		DBG_8192C("fw download fail!\n");

		goto exit;
	}	
	else
	{

		padapter->bFWReady = _TRUE;

		pHalData->fw_ractrl = _TRUE;

		DBG_8192C("fw download ok!\n");	
	}
#endif

	//if(pMgntInfo->RegRfOff == TRUE){
	//	pHalData->eRFPowerState = eRfOff;
	//}

	// Set RF type for BB/RF configuration	
	_InitRFType(padapter);//->_ReadRFType()

	// Save target channel
	// <Roger_Notes> Current Channel will be updated again later.
	pHalData->CurrentChannel = 6;//default set to 6

	status = PHY_MACConfig8192C(padapter);
	if(status == _FAIL)
	{
		goto exit;
	}

	//
	//d. Initialize BB related configurations.
	//
	status = PHY_BBConfig8192C(padapter);
	if(status == _FAIL)
	{
		goto exit;
	}

	// 92CU use 3-wire to r/w RF
	//pHalData->Rf_Mode = RF_OP_By_SW_3wire;

	status = PHY_RFConfig8192C(padapter);	
	if(status == _FAIL)
	{
		goto exit;
	}

	//
	// Joseph Note: Keep RfRegChnlVal for later use.
	//
	pHalData->RfRegChnlVal[0] = PHY_QueryRFReg(padapter, (RF90_RADIO_PATH_E)0, RF_CHNLBW, bRFRegOffsetMask);
	pHalData->RfRegChnlVal[1] = PHY_QueryRFReg(padapter, (RF90_RADIO_PATH_E)1, RF_CHNLBW, bRFRegOffsetMask);

	_BBTurnOnBlock(padapter);
	//NicIFSetMacAddress(padapter, padapter->PermanentAddress);

	_InitSecuritySetting(padapter);
	_RfPowerSave(padapter);

	//
	// f. Start to BulkIn transfer.
	//


#ifdef CONFIG_BT_COEXIST
	if(	(pbtpriv->BT_Coexist) &&
		(pbtpriv->BT_CoexistType == BT_CSR) )
	{
		write32(padapter, REG_FAST_EDCA_CTRL, 0x03086666);
		printk("BT write 0x%x = 0x03086666\n", REG_FAST_EDCA_CTRL);
	}
	else
#endif
	{
	if(pregistrypriv->wifi_spec)
		write16(padapter,REG_FAST_EDCA_CTRL ,0);
	}
	


#if (MP_DRIVER == 1)
	//MPT_InitializeAdapter(padapter, Channel);
#endif
	
	PHY_IQCalibrate(padapter);
	dm_CheckTXPowerTracking(padapter);
	PHY_LCCalibrate(padapter);


#if RTL8192CU_ADHOC_WORKAROUND_SETTING
	_InitAdhocWorkaroundParams(padapter);
#endif

#ifdef CONFIG_USB_HCI //fixed USB interface interference issue
	write8(padapter, 0xfe40, 0xe0);
	write8(padapter, 0xfe41, 0x8d);
	write8(padapter, 0xfe42, 0x80);
#endif

	//misc
	{
		int i;		
		u8 mac_addr[6];
		for(i=0; i<6; i++)
		{			
			mac_addr[i] = read8(padapter, REG_MACID+i);		
		}
		
		DBG_8192C("MAC Address from REG = %x-%x-%x-%x-%x-%x\n", 
			mac_addr[0],	mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
	}

	_InitPABias(padapter);

#ifdef CONFIG_BT_COEXIST
	_InitBTCoexist(padapter);
#endif
	_InitAntenna_Selection(padapter);

	rtl8192c_InitHalDm(padapter);

	write8(padapter, 0x15, 0xe9);//suggest by Johnny for lower temperature
	//_dbg_dump_macreg(padapter);

exit:

_func_exit_;

	return status;
}


static VOID 
_DisableGPIO(
	IN	PADAPTER	Adapter
	)
{
/***************************************
j. GPIO_PIN_CTRL 0x44[31:0]=0x000		// 
k. Value = GPIO_PIN_CTRL[7:0]
l.  GPIO_PIN_CTRL 0x44[31:0] = 0x00FF0000 | (value <<8); //write external PIN level
m. GPIO_MUXCFG 0x42 [15:0] = 0x0780
n. LEDCFG 0x4C[15:0] = 0x8080
***************************************/
	u8	value8;
	u16	value16;
	u32	value32;

	//1. Disable GPIO[7:0]
	write16(Adapter, REG_GPIO_PIN_CTRL+2, 0x0000);
    	value32 = read32(Adapter, REG_GPIO_PIN_CTRL) & 0xFFFF00FF;  
	value8 = (u8) (value32&0x000000FF);
	value32 |= ((value8<<8) | 0x00FF0000);
	write32(Adapter, REG_GPIO_PIN_CTRL, value32);
	      
	//2. Disable GPIO[10:8]          
	write8(Adapter, REG_GPIO_MUXCFG+3, 0x00);
	    value16 = read16(Adapter, REG_GPIO_MUXCFG+2) & 0xFF0F;  
	value8 = (u8) (value16&0x000F);
	value16 |= ((value8<<4) | 0x0780);
	write16(Adapter, REG_GPIO_PIN_CTRL+2, value16);

	//3. Disable LED0 & 1
	write16(Adapter, REG_LEDCFG0, 0x8080);

	//RT_TRACE(COMP_INIT, DBG_LOUD, ("======> Disable GPIO and LED.\n"));
 
} //end of _DisableGPIO()

static VOID
_ResetFWDownloadRegister(
	IN PADAPTER			Adapter
	)
{
	u32	value32;

	value32 = read32(Adapter, REG_MCUFWDL);
	value32 &= ~(MCUFWDL_EN | MCUFWDL_RDY);
	write32(Adapter, REG_MCUFWDL, value32);
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Reset FW download register.\n"));
}


static int
_DisableRF_AFE(
	IN PADAPTER			Adapter
	)
{
	int		rtStatus = _SUCCESS;
	u32			pollingCount = 0;
	u8			value8;
	
	//disable RF/ AFE AD/DA
	value8 = APSDOFF;
	write8(Adapter, REG_APSD_CTRL, value8);


#if (RTL8192CU_ASIC_VERIFICATION)

	do
	{
		if(read8(Adapter, REG_APSD_CTRL) & APSDOFF_STATUS){
			//RT_TRACE(COMP_INIT, DBG_LOUD, ("Disable RF, AFE, AD, DA Done!\n"));
			break;
		}

		if(pollingCount++ > POLLING_READY_TIMEOUT_COUNT){
			//RT_TRACE(COMP_INIT, DBG_SERIOUS, ("Failed to polling APSDOFF_STATUS done!\n"));
			return _FAIL;
		}
				
	}while(_TRUE);
	
#endif

	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Disable RF, AFE,AD, DA.\n"));
	return rtStatus;

}

static VOID
_ResetBB(
	IN PADAPTER			Adapter
	)
{
	u16	value16;

	//reset BB
	value16 = read16(Adapter, REG_SYS_FUNC_EN);
	value16 &= ~(FEN_BBRSTB | FEN_BB_GLB_RSTn);
	write16(Adapter, REG_SYS_FUNC_EN, value16);
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Reset BB.\n"));
}

static VOID
_ResetMCU(
	IN PADAPTER			Adapter
	)
{
	u16	value16;
	
	// reset MCU
	value16 = read16(Adapter, REG_SYS_FUNC_EN);
	value16 &= ~FEN_CPUEN;
	write16(Adapter, REG_SYS_FUNC_EN, value16);
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Reset MCU.\n"));
}

static VOID
_DisableMAC_AFE_PLL(
	IN PADAPTER			Adapter
	)
{
	u32	value32;
	
	//disable MAC/ AFE PLL
	value32 = read32(Adapter, REG_APS_FSMCO);
	value32 |= APDM_MAC;
	write32(Adapter, REG_APS_FSMCO, value32);
	
	value32 |= APFM_OFF;
	write32(Adapter, REG_APS_FSMCO, value32);
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Disable MAC, AFE PLL.\n"));
}

static VOID
_AutoPowerDownToHostOff(
	IN	PADAPTER		Adapter
	)
{
	u32			value32;
	write8(Adapter, REG_SPS0_CTRL, 0x22);

	value32 = read32(Adapter, REG_APS_FSMCO);	
	
	value32 |= APDM_HOST;//card disable
	write32(Adapter, REG_APS_FSMCO, value32);
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Auto Power Down to Host-off state.\n"));

	// set USB suspend
	value32 = read32(Adapter, REG_APS_FSMCO);
	value32 &= ~AFSM_PCIE;
	write32(Adapter, REG_APS_FSMCO, value32);

}

static VOID
_SetUsbSuspend(
	IN PADAPTER			Adapter
	)
{
	u32			value32;

	value32 = read32(Adapter, REG_APS_FSMCO);
	
	// set USB suspend
	value32 |= AFSM_HSUS;
	write32(Adapter, REG_APS_FSMCO, value32);

	//RT_ASSERT(0 == (read32(Adapter, REG_APS_FSMCO) & BIT(12)),(""));
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Set USB suspend.\n"));
	
}

static VOID
_DisableRFAFEAndResetBB(
	IN PADAPTER			Adapter
	)
{
/**************************************
a.	TXPAUSE 0x522[7:0] = 0xFF             //Pause MAC TX queue
b.	RF path 0 offset 0x00 = 0x00            // disable RF
c. 	APSD_CTRL 0x600[7:0] = 0x40
d.	SYS_FUNC_EN 0x02[7:0] = 0x16		//reset BB state machine
e.	SYS_FUNC_EN 0x02[7:0] = 0x14		//reset BB state machine
***************************************/
	u8 eRFPath = 0,value8 = 0;
	write8(Adapter, REG_TXPAUSE, 0xFF);
	PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, 0x0, bMaskByte0, 0x0);

	value8 |= APSDOFF;
	write8(Adapter, REG_APSD_CTRL, value8);//0x40
	
	value8 = 0 ; 
	value8 |=( FEN_USBD | FEN_USBA | FEN_BB_GLB_RSTn);
	write8(Adapter, REG_SYS_FUNC_EN,value8 );//0x16		
	
	value8 &=( ~FEN_BB_GLB_RSTn );
	write8(Adapter, REG_SYS_FUNC_EN, value8); //0x14		
	
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("======> RF off and reset BB.\n"));
}

static VOID
_ResetDigitalProcedure1(
	IN 	PADAPTER			Adapter,
	IN	BOOLEAN				bWithoutHWSM	
	)
{

	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(Adapter);

	if(pHalData->FirmwareVersion <=  0x20){
		#if 0
		/*****************************
		f.	SYS_FUNC_EN 0x03[7:0]=0x54		// reset MAC register, DCORE
		g.	MCUFWDL 0x80[7:0]=0				// reset MCU ready status
		******************************/
		u4Byte	value32 = 0;
		PlatformIOWrite1Byte(Adapter, REG_SYS_FUNC_EN+1, 0x54);
		PlatformIOWrite1Byte(Adapter, REG_MCUFWDL, 0);	
		#else
		/*****************************
		f.	MCUFWDL 0x80[7:0]=0				// reset MCU ready status
		g.	SYS_FUNC_EN 0x02[10]= 0			// reset MCU register, (8051 reset)
		h.	SYS_FUNC_EN 0x02[15-12]= 5		// reset MAC register, DCORE
		i.     SYS_FUNC_EN 0x02[10]= 1			// enable MCU register, (8051 enable)
		******************************/
			u16 valu16 = 0;
			write8(Adapter, REG_MCUFWDL, 0);

			valu16 = read16(Adapter, REG_SYS_FUNC_EN);	
			write16(Adapter, REG_SYS_FUNC_EN, (valu16 & (~FEN_CPUEN)));//reset MCU ,8051

			valu16 = read16(Adapter, REG_SYS_FUNC_EN)&0x0FFF;	
			write16(Adapter, REG_SYS_FUNC_EN, (valu16 |(FEN_HWPDN|FEN_ELDR)));//reset MAC
			
			valu16 = read16(Adapter, REG_SYS_FUNC_EN);	
			write16(Adapter, REG_SYS_FUNC_EN, (valu16 | FEN_CPUEN));//enable MCU ,8051	

		
		#endif
	}
	else{
		u8 retry_cnts = 0;	
		
		if(read8(Adapter, REG_MCUFWDL) & BIT1){ //IF fw in RAM code, do reset 

			write8(Adapter, REG_MCUFWDL, 0);
			if(Adapter->bFWReady){
				write8(Adapter, REG_HMETFR+3, 0x20);//8051 reset by self
				while( (retry_cnts++ <100) && (FEN_CPUEN &read16(Adapter, REG_SYS_FUNC_EN)))
				{					
					mdelay_os(50);//PlatformStallExecution(50);//us
				}
				if(retry_cnts >= 100){				
					//printk("#####=> 8051 reset failed!.........................\n");
				}
				
				//RT_ASSERT((retry_cnts < 100), );			
				//RT_TRACE(COMP_INIT, DBG_LOUD, ("=====> 8051 reset success (%d) .\n",retry_cnts));
			}
		}
		else{
			//RT_TRACE(COMP_INIT, DBG_LOUD, ("=====> 8051 in ROM.\n"));
		}	
		
		write8(Adapter, REG_SYS_FUNC_EN+1, 0x54);	//Reset MAC and Enable 8051
	}			

	if(bWithoutHWSM){
	/*****************************
		Without HW auto state machine
	g.	SYS_CLKR 0x08[15:0] = 0x30A3			//disable MAC clock
	h.	AFE_PLL_CTRL 0x28[7:0] = 0x80			//disable AFE PLL
	i.	AFE_XTAL_CTRL 0x24[15:0] = 0x880F		//gated AFE DIG_CLOCK
	j.	SYS_ISO_CTRL 0x00[7:0] = 0xF9			// isolated digital to PON
	******************************/	
		//write16(Adapter, REG_SYS_CLKR, 0x30A3);
		write16(Adapter, REG_SYS_CLKR, 0x70A3);//modify to 0x70A3 by Scott.
		write8(Adapter, REG_AFE_PLL_CTRL, 0x80);		
		write16(Adapter, REG_AFE_XTAL_CTRL, 0x880F);
		write8(Adapter, REG_SYS_ISO_CTRL, 0xF9);		
	}
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("======> Reset Digital.\n"));

}

static VOID
_ResetDigitalProcedure2(
	IN 	PADAPTER			Adapter
)
{
/*****************************
k.	SYS_FUNC_EN 0x03[7:0] = 0x44			// disable ELDR runction
l.	SYS_CLKR 0x08[15:0] = 0x3083			// disable ELDR clock
m.	SYS_ISO_CTRL 0x01[7:0] = 0x83			// isolated ELDR to PON
******************************/
	//write8(Adapter, REG_SYS_FUNC_EN+1, 0x44);//marked by Scott.
	//write16(Adapter, REG_SYS_CLKR, 0x3083);
	//write8(Adapter, REG_SYS_ISO_CTRL+1, 0x83);

 	write16(Adapter, REG_SYS_CLKR, 0x70a3); //modify to 0x70a3 by Scott.
 	write8(Adapter, REG_SYS_ISO_CTRL+1, 0x82); //modify to 0x82 by Scott.
}

static VOID
_DisableAnalog(
	IN PADAPTER			Adapter,
	IN BOOLEAN			bWithoutHWSM	
	)
{
	u32 value16 = 0;
	u8 value8=0;
	
	if(bWithoutHWSM){
	/*****************************
	n.	LDOA15_CTRL 0x20[7:0] = 0x04		// disable A15 power
	o.	LDOV12D_CTRL 0x21[7:0] = 0x54		// disable digital core power
	r.	When driver call disable, the ASIC will turn off remaining clock automatically 
	******************************/
	
		write8(Adapter, REG_LDOA15_CTRL, 0x04);
		//PlatformIOWrite1Byte(Adapter, REG_LDOV12D_CTRL, 0x54);		
		
		value8 = read8(Adapter, REG_LDOV12D_CTRL);		
		value8 &= (~LDV12_EN);
		write8(Adapter, REG_LDOV12D_CTRL, value8);	
		//RT_TRACE(COMP_INIT, DBG_LOUD, (" REG_LDOV12D_CTRL Reg0x21:0x%02x.\n",value8));
	}
	
/*****************************
h.	SPS0_CTRL 0x11[7:0] = 0x23			//enter PFM mode
i.	APS_FSMCO 0x04[15:0] = 0x4802		// set USB suspend 
******************************/	
	write8(Adapter, REG_SPS0_CTRL, 0x23);
	
	value16 |= (APDM_HOST | AFSM_HSUS |PFM_ALDN);
	write16(Adapter, REG_APS_FSMCO,value16 );//0x4802 


	write8(Adapter, REG_RSV_CTRL, 0x0e);
 #if 0
 	//tynli_test for suspend mode.
	if(!bWithoutHWSM){
 		write8(Adapter, 0xfe10, 0x19);
	} 
#endif
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("======> Disable Analog Reg0x04:0x%04x.\n",value16));
}

static int	
CardDisableHWSM( // HW Auto state machine
	IN	PADAPTER		Adapter,
	IN	BOOLEAN			resetMCU
	)
{
	int		rtStatus = _SUCCESS;
	if(Adapter->bSurpriseRemoved){
		return rtStatus;
	}
#if 1
	//==== RF Off Sequence ====
	_DisableRFAFEAndResetBB(Adapter);

	//  ==== Reset digital sequence   ======
	_ResetDigitalProcedure1(Adapter, _FALSE);
	
	//  ==== Pull GPIO PIN to balance level and LED control ======
	_DisableGPIO(Adapter);

	//  ==== Disable analog sequence ===
	_DisableAnalog(Adapter, _FALSE);

	RT_TRACE(_module_hci_hal_init_c_, _drv_info_, ("======> Card disable finished.\n"));
#else
	_DisableGPIO(Adapter);
	
	//reset FW download register
	_ResetFWDownloadRegister(Adapter);


	//disable RF/ AFE AD/DA
	rtStatus = _DisableRF_AFE(Adapter);
	if(RT_STATUS_SUCCESS != rtStatus){
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("_DisableRF_AFE failed!\n"));
		goto Exit;
	}
	_ResetBB(Adapter);

	if(resetMCU){
		_ResetMCU(Adapter);
	}

	_AutoPowerDownToHostOff(Adapter);
	//_DisableMAC_AFE_PLL(Adapter);
	
	_SetUsbSuspend(Adapter);
Exit:
#endif
	return rtStatus;
	
}

static int	
CardDisableWithoutHWSM( // without HW Auto state machine
	IN	PADAPTER		Adapter	
	)
{
	int		rtStatus = _SUCCESS;

	if(Adapter->bSurpriseRemoved){
		return rtStatus;
	}
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("======> Card Disable Without HWSM .\n"));
	//==== RF Off Sequence ====
	_DisableRFAFEAndResetBB(Adapter);

	//  ==== Reset digital sequence   ======
	_ResetDigitalProcedure1(Adapter, _TRUE);

	//  ==== Pull GPIO PIN to balance level and LED control ======
	_DisableGPIO(Adapter);

	//  ==== Reset digital sequence   ======
	_ResetDigitalProcedure2(Adapter);

	//  ==== Disable analog sequence ===
	_DisableAnalog(Adapter, _TRUE);
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("<====== Card Disable Without HWSM .\n"));
	return rtStatus;
}


u32 rtl8192cu_hal_deinit(_adapter *padapter)
 {

_func_enter_;

	if( padapter->bCardDisableWOHSM == _FALSE)
	{
		CardDisableHWSM(padapter, _FALSE);
	}
	else
	{
		//printk("card disble without HWSM...........\n");
		CardDisableWithoutHWSM(padapter); // without HW Auto state machine		
	}

_func_exit_;
	
	return _SUCCESS;
 }


unsigned int rtl8192cu_inirp_init(_adapter * padapter)
{	
	u8 i;	
	struct recv_buf *precvbuf;
	uint	status;
	struct dvobj_priv *pdev=&padapter->dvobjpriv;
	struct intf_hdl * pintfhdl=&padapter->iopriv.intf;
	struct recv_priv *precvpriv = &(padapter->recvpriv);
	u32 (*_read_port)(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *pmem);

_func_enter_;

	_read_port = pintfhdl->io_ops._read_port;

	status = _SUCCESS;

	RT_TRACE(_module_hci_hal_init_c_,_drv_info_,("===> usb_inirp_init \n"));	
		
	precvpriv->ff_hwaddr = RECV_BULK_IN_ADDR;

	//issue Rx irp to receive data	
	precvbuf = (struct recv_buf *)precvpriv->precv_buf;	
	for(i=0; i<NR_RECVBUFF; i++)
	{
		if(_read_port(pintfhdl, precvpriv->ff_hwaddr, 0, (unsigned char *)precvbuf) == _FALSE )
		{
			RT_TRACE(_module_hci_hal_init_c_,_drv_err_,("usb_rx_init: usb_read_port error \n"));
			status = _FAIL;
			goto exit;
		}
		
		precvbuf++;		
		precvpriv->free_recv_buf_queue_cnt--;
	}
		
exit:
	
	RT_TRACE(_module_hci_hal_init_c_,_drv_info_,("<=== usb_inirp_init \n"));

_func_exit_;

	return status;

}

unsigned int rtl8192cu_inirp_deinit(_adapter * padapter)
{	
	RT_TRACE(_module_hci_hal_init_c_,_drv_info_,("\n ===> usb_rx_deinit \n"));
	
	read_port_cancel(padapter);


	RT_TRACE(_module_hci_hal_init_c_,_drv_info_,("\n <=== usb_rx_deinit \n"));

	return _SUCCESS;
}


void rtl8192cu_set_hal_ops(_adapter * padapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);

_func_enter_;

	pHalData->hal_ops.hal_init = &rtl8192cu_hal_init;
	pHalData->hal_ops.hal_deinit = &rtl8192cu_hal_deinit;

	pHalData->hal_ops.inirp_init = &rtl8192cu_inirp_init;
	pHalData->hal_ops.inirp_deinit = &rtl8192cu_inirp_deinit;

	pHalData->hal_ops.intf_chip_configure = &rtl8192cu_interface_configure;
	pHalData->hal_ops.read_adapter_info = &NicIFReadAdapterInfo8192C;

	pHalData->hal_ops.set_bwmode_handler = &PHY_SetBWMode8192C;
	pHalData->hal_ops.set_channel_handler = &PHY_SwChnl8192C;

	pHalData->hal_ops.process_phy_info = &rtl8192c_process_phy_info;
	pHalData->hal_ops.hal_dm_watchdog = &rtl8192c_HalDmWatchDog;

_func_exit_;
}
