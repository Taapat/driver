/*****************************************************************************
*    Copyright (C)2008 Ali Corporation. All Rights Reserved.
*
*    File:    nim_vz7903.c
*
*    Description:    This file contains SHARP BS2S7VZ7903 tuner basic function.
*    History:
*           Date            		Athor        			Version          	Reason
*	    ============	=============		=========	=================
*	1.  2011-10-16		   	Russell Luo		 	Ver 0.1		Create file.
*
*****************************************************************************/

#include "ywdefs.h"
#include "ywos.h"
#include "ywlib.h"
#include "ywhal_assert.h"
#include "ywtuner_ext.h"
#include "tuner_def.h"

//#include "../../../ioreg.h"
//#include "../../../tuner_interface.h"
//#include "../../tuner/tunsdrv.h"
#include "ywtuner_def.h"
//#include "ywgpio_ext.h"
//#include "stddefs.h"
//#include "porting.h"

#include "vz7903.h"

#include "dvb_frontend.h"
#include "stv090x_reg.h"
#include "stv090x.h"
#include "stv090x_priv.h"


#ifndef I2C_FOR_VZ7903
#define I2C_FOR_VZ7903 	I2C_TYPE_SCB0
#endif

//#define VZ7903_PRINTF(...)
#define VZ7903_PRINTF  libc_printf

//static UINT8 QM1D1C0045_d_reg[QM1D1C0045_INI_REG_MAX];
#define	DEF_XTAL_FREQ				16000
#define INIT_DUMMY_RESET	0x0C
#define LPF_CLK_16000kHz	16000

#define I2cWriteRegs(Addr, rAddr, lpbData, bLen) i2c_write(I2C_FOR_VZ7903, Addr, lpbData, bLen)
#define I2cReadReg(Addr, rAddr, lpbData) i2c_read(I2C_FOR_VZ7903, Addr, lpbData, 1)

//struct QPSK_TUNER_CONFIG_EXT * vz7903_dev_id[MAX_TUNER_SUPPORT_NUM] = {NULL};

static UINT32 vz7903_tuner_cnt = 0;

typedef enum _QM1D1C0045_INIT_CFG_DATA{
	QM1D1C0045_LOCAL_FREQ,
	QM1D1C0045_XTAL_FREQ,
	QM1D1C0045_CHIP_ID,
	QM1D1C0045_LPF_WAIT_TIME,
	QM1D1C0045_FAST_SEARCH_WAIT_TIME,
	QM1D1C0045_NORMAL_SEARCH_WAIT_TIME,
	QM1D1C0045_INI_CGF_MAX
}QM1D1C0045_INIT_CFG_DATA, *PQM1D1C0045_INIT_CFG_DATA ;

typedef enum _QM1D1C0045_INIT_REG_DATA{
	QM1D1C0045_REG_00 = 0,
	QM1D1C0045_REG_01,
	QM1D1C0045_REG_02,
	QM1D1C0045_REG_03,
	QM1D1C0045_REG_04,
	QM1D1C0045_REG_05,
	QM1D1C0045_REG_06,
	QM1D1C0045_REG_07,
	QM1D1C0045_REG_08,
	QM1D1C0045_REG_09,
	QM1D1C0045_REG_0A,
	QM1D1C0045_REG_0B,
	QM1D1C0045_REG_0C,
	QM1D1C0045_REG_0D,
	QM1D1C0045_REG_0E,
	QM1D1C0045_REG_0F,
	QM1D1C0045_REG_10,
	QM1D1C0045_REG_11,
	QM1D1C0045_REG_12,
	QM1D1C0045_REG_13,
	QM1D1C0045_REG_14,
	QM1D1C0045_REG_15,
	QM1D1C0045_REG_16,
	QM1D1C0045_REG_17,
	QM1D1C0045_REG_18,
	QM1D1C0045_REG_19,
	QM1D1C0045_REG_1A,
	QM1D1C0045_REG_1B,
	QM1D1C0045_REG_1C,
	QM1D1C0045_REG_1D,
	QM1D1C0045_REG_1E,
	QM1D1C0045_REG_1F,
	QM1D1C0045_INI_REG_MAX
}QM1D1C0045_INIT_REG_DATA, *PQM1D1C0045_INIT_REG_DATA ;

typedef enum _QM1D1C0045_LPF_FC{
	QM1D1C0045_LPF_FC_4MHz=0,	//0000:4MHz
	QM1D1C0045_LPF_FC_6MHz, 	//0001:6MHz
	QM1D1C0045_LPF_FC_8MHz, 	//0010:8MHz
	QM1D1C0045_LPF_FC_10MHz,	//0011:10MHz
	QM1D1C0045_LPF_FC_12MHz,	//0100:12MHz
	QM1D1C0045_LPF_FC_14MHz,	//0101:14MHz
	QM1D1C0045_LPF_FC_16MHz,	//0110:16MHz
	QM1D1C0045_LPF_FC_18MHz,	//0111:18MHz
	QM1D1C0045_LPF_FC_20MHz,	//1000:20MHz
	QM1D1C0045_LPF_FC_22MHz,	//1001:22MHz
	QM1D1C0045_LPF_FC_24MHz,	//1010:24MHz
	QM1D1C0045_LPF_FC_26MHz,	//1011:26MHz
	QM1D1C0045_LPF_FC_28MHz,	//1100:28MHz
	QM1D1C0045_LPF_FC_30MHz,	//1101:30MHz
	QM1D1C0045_LPF_FC_32MHz,	//1110:32MHz
	QM1D1C0045_LPF_FC_34MHz,	//1111:34MHz
	QM1D1C0045_LPF_FC_MAX,
}QM1D1C0045_LPF_FC;

typedef enum _QM1D1C0045_LPF_ADJUSTMENT_CURRENT{
	QM1D1C0045_LPF_ADJUSTMENT_CURRENT_25UA=0,	//0x1B b[1:0] = b00
	QM1D1C0045_LPF_ADJUSTMENT_CURRENT_DUMMY1,	//0x1B b[1:0] = b01
	QM1D1C0045_LPF_ADJUSTMENT_CURRENT_37R5UA,	//0x1B b[1:0] = b10
	QM1D1C0045_LPF_ADJUSTMENT_CURRENT_DUMMY2,	//0x1B b[1:0] = b11
}QM1D1C0045_LPF_ADJUSTMENT_CURRENT;

typedef struct _QM1D1C0045_CONFIG_STRUCT {
	unsigned int		ui_QM1D1C0045_RFChannelkHz;	/* direct channel */
	unsigned int		ui_QM1D1C0045_XtalFreqKHz;
	BOOL				b_QM1D1C0045_fast_search_mode;
	BOOL				b_QM1D1C0045_loop_through;
	BOOL				b_QM1D1C0045_tuner_standby;
	BOOL				b_QM1D1C0045_head_amp;
	QM1D1C0045_LPF_FC	QM1D1C0045_lpf;
	unsigned int		ui_QM1D1C0045_LpfWaitTime;
	unsigned int		ui_QM1D1C0045_FastSearchWaitTime;
	unsigned int		ui_QM1D1C0045_NormalSearchWaitTime;
	BOOL				b_QM1D1C0045_iq_output;
} QM1D1C0045_CONFIG_STRUCT, *PQM1D1C0045_CONFIG_STRUCT;


//=========================================================================
// GLOBAL VARIALBLES
//=========================================================================

const unsigned long QM1D1C0045_d[QM1D1C0045_INI_CGF_MAX]={
	0x0017a6b0,	//	LOCAL_FREQ,
	0x00003e80,	//	XTAL_FREQ,
	0x00000068,	//	CHIP_ID,
	0x00000014,	//	LPF_WAIT_TIME,
	0x00000004,	//	FAST_SEARCH_WAIT_TIME,
	0x0000000f,	//	NORMAL_SEARCH_WAIT_TIME,
};
UINT8	QM1D1C0045_d_reg[QM1D1C0045_INI_REG_MAX]={
	0x68,	//0x00
	0x1c,	//0x01
	0xc0,	//0x02
	0x10,	//0x03
	0xbc,	//0x04
	0xc1,	//0x05
	0x15,	//0x06
	0x34,	//0x07
	0x06,	//0x08
	0x3e,	//0x09
	0x00,	//0x0a
	0x00,	//0x0b
	0x43,	//0x0c
	0x00,	//0x0d
	0x00,	//0x0e
	0x00,	//0x0f
	0x00,	//0x10
	0xff,	//0x11
	0xf3,	//0x12
	0x00,	//0x13
//	0x3f,	//0x14
	0x3e,	//0x14
	0x25,	//0x15
	0x5c,	//0x16
	0xd6,	//0x17
	0x55,	//0x18
//	0xcf,	//0x19
	0x8f,	//0x19
	0x95,	//0x1a
//	0xf6,	//0x1b
	0xfe,	//0x1b
	0x36,	//0x1c
	0xf2,	//0x1d
	0x09,	//0x1e
	0x00,	//0x1f
};
const UINT8	QM1D1C0045_d_flg[QM1D1C0045_INI_REG_MAX]=	/* 0:R, 1:R/W */
{
	0,	// 0x0
	1,	// 0x1
	1,	// 0x2
	1,	// 0x3
	1,	// 0x4
	1,	// 0x5
	1,	// 0x6
	1,	// 0x7
	1,	// 0x8
	1,	// 0x9
	1,	// 0xA
	1,	// 0xB
	1,	// 0xC
	0,	// 0xD
	0,	// 0xE
	0,	// 0xF
	0,	// 0x10
	1,	// 0x11
	1,	// 0x12
	1,	// 0x13
	1,	// 0x14
	1,	// 0x15
	1,	// 0x16
	1,	// 0x17
	1,	// 0x18
	1,	// 0x19
	1,	// 0x1A
	1,	// 0x1B
	1,	// 0x1C
	1,	// 0x1D
	1,	// 0x1E
	1,	// 0x1F
};
const unsigned long QM1D1C0045_local_f[]={//kHz
//	2151000,
	2551000,
	1950000,
	1800000,
	1600000,
	1450000,
	1250000,
	1200000,
	975000,
	950000,
	0,
	0,
	0,
	0,
	0,
	0,
};
const unsigned long QM1D1C0045_div2[]={
	1,
	1,
	1,
	1,
	1,
	1,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};
const unsigned long QM1D1C0045_vco_band[]={
	7,
	6,
	5,
	4,
	3,
	2,
	7,
	6,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};
const unsigned long QM1D1C0045_div45_lband[]={
	0,
	0,
	0,
	0,
	0,
	0,
	1,
	1,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};


/*====================================================*
    QM1D1C0045_register_real_write
   --------------------------------------------------
    Description     register write
    Argument        RegAddr
					RegData
    Return Value	BOOL (TRUE:success, FALSE:error)
 *====================================================*/
/*BOOL QM1D1C0045_register_real_write(UINT32 tuner_id, UINT8 RegAddr, UINT8 RegData){
	UINT8 slvAddr;
	UINT8 i2c_tmp_buf[2];
	UINT16 i2c_access_size = 2;
	i2c_tmp_buf[0] = RegAddr;
	i2c_tmp_buf[1] = RegData;
	slvAddr = 0xc0;
	i2c_write(tuner_id, slvAddr, i2c_tmp_buf, i2c_access_size);
	return TRUE;
}*/


#define vz7903_i2c_addr 0x60 //add by yanbinL

struct vz7903_state *pVz7903_Config;
struct vz7903_config  *pVz7903_I2cConfig;

INT32 Vz7903_Read(struct i2c_adapter *i2c_adap, UINT8 I2cAddr, UINT8 *pData, UINT8 bLen)
{

//   	int i;
	int ret;
	UINT8 RegAddr[] = {pData[0]};

	struct i2c_msg msg[] = {
		{ .addr	= I2cAddr, .flags	= I2C_M_RD,	.buf = pData, .len = bLen}
	};



   // printk("[information]len:%d MemAddr:%x addr%x\n",bLen,pData[0],I2cAddr);

	ret = i2c_transfer(i2c_adap, msg, 1);

#if 0

	for(i=0;i<bLen;i++)
	{
		printk("[DATA]%x ",pData[i]);
	}
	printk("\n");
#endif
	if (ret != 1)
	{
		if (ret != -ERESTARTSYS)
			printk(	"[vz7903 Read error], Reg=[0x%02x], Status=%d\n",RegAddr[0], ret);

		return ret < 0 ? ret : -EREMOTEIO;
	}

	return 0;


}

INT32 Vz7903_Write(struct i2c_adapter *i2c_adap, UINT8 I2CAddr, UINT8 *pData, UINT8 bLen)
{
	int ret;

	struct i2c_msg i2c_msg = {.addr = I2CAddr, .flags = 0, .buf = pData, .len = bLen };
//	printk("pVz7903_Config->i2c = 0x%x i2cAddr=%x\n", (int)i2c_adap,I2CAddr);

	//int i;

#if 0
	for (i = 0; i < bLen; i++)
	{
		printk("%02x ", pData[i]);
	}
	printk("  write Data,Addr: %x\n",I2CAddr);
#endif

	ret = i2c_transfer(i2c_adap, &i2c_msg, 1);
	if (ret != 1) {
		if (ret != -ERESTARTSYS)
			printk("[vz7903 write error]Reg=[0x%04x], Data=[0x%02x ...], Count=%u, Status=%d\n",
				pData[0], pData[1], bLen, ret);
		return ret < 0 ? ret : -EREMOTEIO;
	}
    return 0;
}


static int  I2C_ReadWrite(void *I2CHandle, TUNER_IOARCH_Operation_t Operation, unsigned short SubAddr, U8 *Data, U32 TransferSize, U32 i2ctype)
	{
		int  Ret = 0;
		U8	SubAddress, SubAddress16bit[2]={0};
		U8	nsbuffer[50];
		BOOL ADR16_FLAG=FALSE;
		U8 i2cAddr;
		 struct i2c_adapter *i2c_adap =(struct i2c_adapter	*)I2CHandle;
		if(i2ctype == 0)
		{
			i2cAddr = pVz7903_I2cConfig->I2cAddr;
		}
		else
		{
			i2cAddr = pVz7903_I2cConfig->DemodI2cAddr;
		}
		if(SubAddr & 0xFF00)
			ADR16_FLAG = TRUE;
		else
			ADR16_FLAG=FALSE;

		if(ADR16_FLAG == FALSE)
		{
			SubAddress = (U8)(SubAddr & 0xFF);
		}
		else
		{
			SubAddress16bit[0]=(U8)((SubAddr & 0xFF00)>>8);
			SubAddress16bit[1]=(U8)(SubAddr & 0xFF);
		}

		switch (Operation)
		{
			/* ---------- Read ---------- */

			case TUNER_IO_SA_READ:
				if(ADR16_FLAG == FALSE)
				{
					 Ret = Vz7903_Write( i2c_adap,i2cAddr,&SubAddress, 1); /* fix for cable (297 chip) */
				}
				else
				{

					 Ret = Vz7903_Write( i2c_adap,i2cAddr, SubAddress16bit, 2);
				}

				/* fallthrough (no break;) */
			case TUNER_IO_READ:
					Ret = Vz7903_Read( i2c_adap,i2cAddr,Data,TransferSize);

				break;

			case TUNER_IO_SA_WRITE:


				if (TransferSize >= 50)
				{
					return(YWHAL_ERROR_NO_MEMORY);
				}
				if(ADR16_FLAG == FALSE)
				{
					nsbuffer[0] = SubAddress;
					YWLIB_Memcpy( (nsbuffer + 1), Data, TransferSize);
					Ret = Vz7903_Write( i2c_adap,i2cAddr, nsbuffer, TransferSize+1);
				}
				else
				{
					nsbuffer[0] = SubAddress16bit[0];
					nsbuffer[1] = SubAddress16bit[1];
					YWLIB_Memcpy( (nsbuffer + 2), Data, TransferSize);
					Ret = Vz7903_Write( i2c_adap, i2cAddr, nsbuffer, TransferSize+2);
				}

				break;
			case TUNER_IO_WRITE:
	            Ret = Vz7903_Write(i2c_adap,i2cAddr, Data, TransferSize);
	            break;

			/* ---------- Error ---------- */
			default:
				break;
		}

	   // STI2C_Unlock(I2C_HANDLE(I2CHandle));	//lwj remove

		return Ret;

	}

BOOL QM1D1C0045_register_real_write(UINT32 tuner_id, UINT8 RegAddr, UINT8 RegData){

    YW_ErrorType_T   Err =0;
    U16 reg_index= 0xf12a;
	U8 data = 0;

	/*将转发开关打开*/
	Err = I2C_ReadWrite((void*)pVz7903_Config->i2c, TUNER_IO_SA_READ, reg_index, &data, 1, 1);
	data |= 0x80;
	Err |= I2C_ReadWrite((void*)pVz7903_Config->i2c, TUNER_IO_SA_WRITE, reg_index, &data, 1, 1);

    Err = I2C_ReadWrite((void*)pVz7903_Config->i2c, TUNER_IO_SA_WRITE, RegAddr, &RegData, 1,0);
	//printf("[%s]Err=0x%x,IOHandle=%d\n",__FUNCTION__,Err,IOHandle);

	return TRUE;
}

/*====================================================*
    QM1D1C0045_register_real_read
   --------------------------------------------------
    Description     register read
    Argument        RegAddr (Register Address)
					apData (Read data)
    Return Value	BOOL (TRUE:success, FALSE:error)
 *====================================================*/
/*BOOL QM1D1C0045_register_real_read(UINT32 tuner_id, UINT8 RegAddr, UINT8 *apData)
{
	BOOL bRetVal;
	UINT8 slvAddr;

	slvAddr = 0xc0; //QM1D1C0045_i2c_slave_addr_set(QM1D1C0045_ILLEAGAL_SLAVE_ADDR);

	{
		UINT8 i2c_tmp_buf[1];
		UINT16 i2c_access_size = 1;
		i2c_tmp_buf[0] = RegAddr;
		bRetVal = i2c_write(tuner_id, slvAddr, i2c_tmp_buf, i2c_access_size);
	}
	if(bRetVal != TRUE){
		//VZ7903_PRINTF("  write error --\n");
		//return bRetVal;

	}

	{
		UINT16 i2c_access_size = 1;
		bRetVal = i2c_read(tuner_id, slvAddr, apData, i2c_access_size);

	}
	if(bRetVal != TRUE){

		//VZ7903_PRINTF("  write error --\n");
		//return bRetVal;

	}

	return TRUE;
}
*/

/*====================================================*
	QM1D1C0045_register_real_read
   --------------------------------------------------
	Description 	register read
	Argument		RegAddr (Register Address)
					apData (Read data)
	Return Value	BOOL (TRUE:success, FALSE:error)
 *====================================================*/
 #if 1
 BOOL QM1D1C0045_register_real_read1(void * I2cHandle, UINT8 RegAddr, UINT8 *apData)
{
	YW_ErrorType_T		   Err = 0;
	U16 reg_index = 0xf12a;
	U8 data = 0,i_data =0;

	/*将转发开关打开*/
	Err = I2C_ReadWrite(I2cHandle, TUNER_IO_SA_READ, reg_index, &data, 1, 1);
	data |= 0x80;
	Err |= I2C_ReadWrite(I2cHandle, TUNER_IO_SA_WRITE, reg_index, &data, 1, 1);
	i_data = RegAddr;
	Err |= I2C_ReadWrite(I2cHandle, TUNER_IO_WRITE, 1, &i_data, 1, 0);
	data = 0;
	Err = I2C_ReadWrite(I2cHandle, TUNER_IO_SA_READ, reg_index, &data, 1, 1);
	data |= 0x80;
	Err |= I2C_ReadWrite(I2cHandle, TUNER_IO_SA_WRITE, reg_index, &data, 1, 1);

	Err |= I2C_ReadWrite(I2cHandle, TUNER_IO_READ, 1, &i_data, 1, 0);
	*apData = i_data;
	//printf("[%s]%d,Err=0x%x,i_data = 0x%x\n",__FUNCTION__,__LINE__,Err,i_data);


	return TRUE;
}
#endif

BOOL QM1D1C0045_register_real_read(UINT32 tuner_id, UINT8 RegAddr, UINT8 *apData)
{
	YW_ErrorType_T         Err = 0;
    U16 reg_index = 0xf12a;
	U8 data = 0,i_data =0;


	/*将转发开关打开*/
	Err = I2C_ReadWrite((void*)pVz7903_Config->i2c, TUNER_IO_SA_READ, reg_index, &data, 1, 1);
	data |= 0x80;
	Err |= I2C_ReadWrite((void*)pVz7903_Config->i2c, TUNER_IO_SA_WRITE, reg_index, &data, 1, 1);
	i_data = RegAddr;
	Err |= I2C_ReadWrite((void*)pVz7903_Config->i2c, TUNER_IO_WRITE, 1, &i_data, 1, 0);

	data = 0;
	Err = I2C_ReadWrite((void*)pVz7903_Config->i2c, TUNER_IO_SA_READ, reg_index, &data, 1, 1);
	data |= 0x80;
	Err |= I2C_ReadWrite((void*)pVz7903_Config->i2c, TUNER_IO_SA_WRITE, reg_index, &data, 1, 1);

	Err |= I2C_ReadWrite((void*)pVz7903_Config->i2c, TUNER_IO_READ, 1, &i_data, 1, 0);
	*apData = i_data;
	//printf("[%s]%d,Err=0x%x,i_data = 0x%x\n",__FUNCTION__,__LINE__,Err,i_data);


	return TRUE;
}

int tuner_Sharp7903_Identify(void * I2cHandle,struct vz7903_config  *I2cConfig)
{
	YW_ErrorType_T Err = YW_NO_ERROR;
    U8		data = 0;
	pVz7903_I2cConfig = I2cConfig;
	QM1D1C0045_register_real_read1(I2cHandle, 0, &data);
	if( 0x68!= data)
	{
		return YWHAL_ERROR_UNKNOWN_DEVICE;
	}
	return 0;
}


void QM1D1C0045_set_lpf(QM1D1C0045_LPF_FC lpf)
{
	QM1D1C0045_d_reg[QM1D1C0045_REG_08] &= 0xF0;
	QM1D1C0045_d_reg[QM1D1C0045_REG_08] |= (lpf&0x0F);
}

BOOL QM1D1C0045_pll_setdata_once(UINT32 tuner_id, QM1D1C0045_INIT_REG_DATA RegAddr, UINT8 RegData){
	BOOL bRetValue;
	bRetValue = QM1D1C0045_register_real_write( tuner_id,RegAddr, RegData);
	return bRetValue;
}

UINT8 QM1D1C0045_pll_getdata_once(UINT32 tuner_id,QM1D1C0045_INIT_REG_DATA RegAddr){
	UINT8 data;
	QM1D1C0045_register_real_read(tuner_id,RegAddr, &data);
	return data;
}



void QM1D1C0045_get_lock_status(UINT32 tuner_id,BOOL* pbLock)
{
	QM1D1C0045_d_reg[QM1D1C0045_REG_0D] = QM1D1C0045_pll_getdata_once(tuner_id,QM1D1C0045_REG_0D);
	if(QM1D1C0045_d_reg[QM1D1C0045_REG_0D]&0x40){
		*pbLock = TRUE;
	}else{
		*pbLock = FALSE;
	}
}

BOOL QM1D1C0045_set_searchmode(UINT32 tuner_id, PQM1D1C0045_CONFIG_STRUCT	apConfig)
{
	BOOL bRetValue;

	if(apConfig->b_QM1D1C0045_fast_search_mode){
		QM1D1C0045_d_reg[QM1D1C0045_REG_03] = QM1D1C0045_pll_getdata_once(tuner_id, QM1D1C0045_REG_03);
		QM1D1C0045_d_reg[QM1D1C0045_REG_03] |= 0x01;
		bRetValue = QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_03 , QM1D1C0045_d_reg[QM1D1C0045_REG_03]);
		if(!bRetValue){
			return bRetValue;
		}
	}else{
		QM1D1C0045_d_reg[QM1D1C0045_REG_03] = QM1D1C0045_pll_getdata_once(tuner_id, QM1D1C0045_REG_03);
		QM1D1C0045_d_reg[QM1D1C0045_REG_03] &= 0xFE;
		bRetValue = QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_03 , QM1D1C0045_d_reg[QM1D1C0045_REG_03]);
		if(!bRetValue){
			return bRetValue;
		}
	}
	return TRUE;
}


BOOL QM1D1C0045_Set_Operation_Param(UINT32 tuner_id, PQM1D1C0045_CONFIG_STRUCT	apConfig)
{
	UINT8 u8TmpData;
	BOOL bRetValue;

	QM1D1C0045_d_reg[QM1D1C0045_REG_01] = QM1D1C0045_pll_getdata_once(tuner_id, QM1D1C0045_REG_01);//Volatile
	QM1D1C0045_d_reg[QM1D1C0045_REG_05] = QM1D1C0045_pll_getdata_once(tuner_id, QM1D1C0045_REG_05);//Volatile

	if(apConfig->b_QM1D1C0045_loop_through && apConfig->b_QM1D1C0045_tuner_standby){
		//LoopThrough = Enable , TunerMode = Standby
		QM1D1C0045_d_reg[QM1D1C0045_REG_01] |= (0x01<<3);//BB_REG_enable
		QM1D1C0045_d_reg[QM1D1C0045_REG_01] |= (0x01<<2);//HA_LT_enable
		QM1D1C0045_d_reg[QM1D1C0045_REG_01] |= (0x01<<1);//LT_enable
		QM1D1C0045_d_reg[QM1D1C0045_REG_01] |= 0x01;//STDBY
		QM1D1C0045_d_reg[QM1D1C0045_REG_05] |= (0x01<<3);//pfd_rst STANDBY

		bRetValue = QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_05 , QM1D1C0045_d_reg[QM1D1C0045_REG_05]);
		if(!bRetValue){
			return bRetValue;
		}
		bRetValue = QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_01 , QM1D1C0045_d_reg[QM1D1C0045_REG_01]);
		if(!bRetValue){
			return bRetValue;
		}
	}else if(apConfig->b_QM1D1C0045_loop_through && !(apConfig->b_QM1D1C0045_tuner_standby)){
		//LoopThrough = Enable , TunerMode = Normal
		QM1D1C0045_d_reg[QM1D1C0045_REG_01] |= (0x01<<3);//BB_REG_enable
		QM1D1C0045_d_reg[QM1D1C0045_REG_01] |= (0x01<<2);//HA_LT_enable
		QM1D1C0045_d_reg[QM1D1C0045_REG_01] |= (0x01<<1);//LT_enable
		QM1D1C0045_d_reg[QM1D1C0045_REG_01] &= (~(0x01))&0xFF;//NORMAL
		QM1D1C0045_d_reg[QM1D1C0045_REG_05] &= (~(0x01<<3))&0xFF;//pfd_rst NORMAL

		bRetValue = QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_01 , QM1D1C0045_d_reg[QM1D1C0045_REG_01]);
		if(!bRetValue){
			return bRetValue;
		}
		bRetValue = QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_05 , QM1D1C0045_d_reg[QM1D1C0045_REG_05]);
		if(!bRetValue){
			return bRetValue;
		}
	}else if(!(apConfig->b_QM1D1C0045_loop_through) && apConfig->b_QM1D1C0045_tuner_standby){
		//LoopThrough = Disable , TunerMode = Standby
		QM1D1C0045_d_reg[QM1D1C0045_REG_01] &= (~(0x01<<3))&0xFF;//BB_REG_disable
		QM1D1C0045_d_reg[QM1D1C0045_REG_01] &= (~(0x01<<2))&0xFF;//HA_LT_disable
		QM1D1C0045_d_reg[QM1D1C0045_REG_01] &= (~(0x01<<1))&0xFF;//LT_disable
		QM1D1C0045_d_reg[QM1D1C0045_REG_01] |= 0x01;//STDBY
		QM1D1C0045_d_reg[QM1D1C0045_REG_05] |= (0x01<<3);//pfd_rst STANDBY

		bRetValue = QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_05 , QM1D1C0045_d_reg[QM1D1C0045_REG_05]);
		if(!bRetValue){
			return bRetValue;
		}
		bRetValue = QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_01 , QM1D1C0045_d_reg[QM1D1C0045_REG_01]);
		if(!bRetValue){
			return bRetValue;
		}
	}else{//!(iLoopThrough) && !(iTunerMode)
		//LoopThrough = Disable , TunerMode = Normal
		QM1D1C0045_d_reg[QM1D1C0045_REG_01] |= (0x01<<3);//BB_REG_enable
		QM1D1C0045_d_reg[QM1D1C0045_REG_01] |= (0x01<<2);//HA_LT_enable
		QM1D1C0045_d_reg[QM1D1C0045_REG_01] &= (~(0x01<<1))&0xFF;//LT_disable
		QM1D1C0045_d_reg[QM1D1C0045_REG_01] &= (~(0x01))&0xFF;//NORMAL
		QM1D1C0045_d_reg[QM1D1C0045_REG_05] &= (~(0x01<<3))&0xFF;//pfd_rst NORMAL

		bRetValue = QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_01 , QM1D1C0045_d_reg[QM1D1C0045_REG_01]);
		if(!bRetValue){
			return bRetValue;
		}
		bRetValue = QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_05 , QM1D1C0045_d_reg[QM1D1C0045_REG_05]);
		if(!bRetValue){
			return bRetValue;
		}
	}

	/*Head Amp*/
	u8TmpData = QM1D1C0045_pll_getdata_once(tuner_id, QM1D1C0045_REG_01);
	if(u8TmpData&0x04){
		apConfig->b_QM1D1C0045_head_amp = TRUE;
	}else{
		apConfig->b_QM1D1C0045_head_amp = FALSE;
	}
	return TRUE;
}


BOOL QM1D1C0045_Initialize(UINT32 tuner_id,PQM1D1C0045_CONFIG_STRUCT	apConfig)
{
	UINT8 i_data , i;
	BOOL bRetValue;

	if(apConfig == NULL){
		return FALSE;
	}

	/*Soft Reaet ON*/
	QM1D1C0045_pll_setdata_once(tuner_id,QM1D1C0045_REG_01 , INIT_DUMMY_RESET);
	//msleep(100);
	/*Soft Reaet OFF*/
	i_data = QM1D1C0045_pll_getdata_once(tuner_id,QM1D1C0045_REG_01);
	i_data |= 0x10;
	QM1D1C0045_pll_setdata_once(tuner_id,QM1D1C0045_REG_01 , i_data);

	/*ID Check*/
	i_data = QM1D1C0045_pll_getdata_once(tuner_id,QM1D1C0045_REG_00);
	//printf("[%s]i_data=0x%x\n",__FUNCTION__,i_data);

	if( QM1D1C0045_d[QM1D1C0045_CHIP_ID] != i_data){
		return FALSE;	//"I2C Comm Error", NULL, MB_ICONWARNING);
	}

	/*LPF Tuning On*/
	//msleep(100);
	QM1D1C0045_d_reg[QM1D1C0045_REG_0C] |= 0x40;
	QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_0C , QM1D1C0045_d_reg[QM1D1C0045_REG_0C]);
	msleep(apConfig->ui_QM1D1C0045_LpfWaitTime);

	/*LPF_CLK Setting Addr:0x08 b6*/
	if(apConfig->ui_QM1D1C0045_XtalFreqKHz == LPF_CLK_16000kHz){
		QM1D1C0045_d_reg[QM1D1C0045_REG_08] &= ~(0x40);
	}else{
		QM1D1C0045_d_reg[QM1D1C0045_REG_08] |= 0x40;
	}

	/*Timeset Setting Addr:0x12 b[2-0]*/
	if(apConfig->ui_QM1D1C0045_XtalFreqKHz == LPF_CLK_16000kHz){
		QM1D1C0045_d_reg[QM1D1C0045_REG_12] &= 0xF8;
		QM1D1C0045_d_reg[QM1D1C0045_REG_12] |= 0x03;
	}else{
		QM1D1C0045_d_reg[QM1D1C0045_REG_12] &= 0xF8;
		QM1D1C0045_d_reg[QM1D1C0045_REG_12] |= 0x04;
	}

	/*IQ Output Setting Addr:0x17 b5 , 0x1B b3*/
	if(apConfig->b_QM1D1C0045_iq_output){//Differential
		QM1D1C0045_d_reg[QM1D1C0045_REG_17] &= 0xDF;
	}else{//SinglEnd
		QM1D1C0045_d_reg[QM1D1C0045_REG_17] |= 0x20;
	}

	for(i=0 ; i<QM1D1C0045_INI_REG_MAX ; i++){
		if(QM1D1C0045_d_flg[i] == TRUE){
			QM1D1C0045_pll_setdata_once(tuner_id, (QM1D1C0045_INIT_REG_DATA)i , QM1D1C0045_d_reg[i]);
		}
	}
//	apConfig->b_QM1D1C0045_fast_search_mode = FALSE;//Normal Search
//	apConfig->b_QM1D1C0045_loop_through = TRUE;//LoopThrough Enable
//	apConfig->b_QM1D1C0045_tuner_standby = FALSE;//Normal Mode

	bRetValue = QM1D1C0045_Set_Operation_Param( tuner_id,apConfig);
	if(!bRetValue){
		return FALSE;
	}
	bRetValue = QM1D1C0045_set_searchmode(tuner_id, apConfig);
	if(!bRetValue){
		return FALSE;
	}
	return TRUE;
}
BOOL QM1D1C0045_LocalLpfTuning(UINT32 tuner_id, PQM1D1C0045_CONFIG_STRUCT	apConfig)
{
	UINT8 i_data , i_data1 , i;
	unsigned int COMP_CTRL;

	if(apConfig == NULL){
		return FALSE;
	}

	/*LPF*/
	QM1D1C0045_set_lpf(apConfig->QM1D1C0045_lpf);

	/*div2/vco_band*/
	for(i=0;i<15;i++){
		if(QM1D1C0045_local_f[i]==0){
			continue;
		}
		if((QM1D1C0045_local_f[i+1]<=apConfig->ui_QM1D1C0045_RFChannelkHz) && (QM1D1C0045_local_f[i]>apConfig->ui_QM1D1C0045_RFChannelkHz)){
			i_data = QM1D1C0045_pll_getdata_once(tuner_id,QM1D1C0045_REG_02);
			i_data &= 0x0F;
			i_data |= ((QM1D1C0045_div2[i]<<7)&0x80);
			i_data |= ((QM1D1C0045_vco_band[i]<<4)&0x70);
			QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_02 , i_data);

			i_data = QM1D1C0045_pll_getdata_once(tuner_id, QM1D1C0045_REG_03);
			i_data &= 0xBF;
			i_data |= ((QM1D1C0045_div45_lband[i]<<6)&0x40);
			QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_03 , i_data);
		}
	}

	/*PLL Counter*/
	{
		int F_ref , pll_ref_div , alpha , N , A , sd;
		int M , beta;
		if(apConfig->ui_QM1D1C0045_XtalFreqKHz==DEF_XTAL_FREQ){
			F_ref = apConfig->ui_QM1D1C0045_XtalFreqKHz;
			pll_ref_div = 0;
		}else{
			F_ref = (apConfig->ui_QM1D1C0045_XtalFreqKHz>>1);
			pll_ref_div = 1;
		}
		M = ( (apConfig->ui_QM1D1C0045_RFChannelkHz * 1000)/(F_ref) );
		alpha = (int)(M + 500)/1000*1000;
		beta = (M - alpha);
		N = (int)((alpha-12000)/4000);
		A = alpha/1000 - 4*(N + 1)-5;  //???
		if(beta>=0){
			sd = (int)(1048576*beta/1000);//sd = (int)(pow(2.,20.)*beta);
		}else{
			sd = (int)(0x400000 + 1048576*beta/1000);//sd = (int)(0x400000 + pow(2.,20.)*beta)
		}

		printk("[tuner vz7903]beta=%d alpha=%d M=%d A=%d N=%d sd=%x\n",beta,alpha,M,A,N,sd);
		QM1D1C0045_d_reg[QM1D1C0045_REG_06] &= 0x40;
		QM1D1C0045_d_reg[QM1D1C0045_REG_06] |= ((pll_ref_div<<7)|(N));
		QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_06 , QM1D1C0045_d_reg[QM1D1C0045_REG_06]);

		QM1D1C0045_d_reg[QM1D1C0045_REG_07] &= 0xF0;
		QM1D1C0045_d_reg[QM1D1C0045_REG_07] |= (A&0x0F);
		QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_07 , QM1D1C0045_d_reg[QM1D1C0045_REG_07]);

		/*LPF_CLK , LPF_FC*/
		i_data = QM1D1C0045_d_reg[QM1D1C0045_REG_08]&0xF0;
		if(apConfig->ui_QM1D1C0045_XtalFreqKHz>=6000 && apConfig->ui_QM1D1C0045_XtalFreqKHz<66000){
			i_data1 = ((unsigned char)((apConfig->ui_QM1D1C0045_XtalFreqKHz+2000)/4000)*2 - 4)>>1;
		}else if(apConfig->ui_QM1D1C0045_XtalFreqKHz<6000){
			i_data1 = 0x00;
		}else{
			i_data1 = 0x0F;
		}

		printk("i_data1 = %x XtalFreqKHz=%d\n",i_data1,apConfig->ui_QM1D1C0045_XtalFreqKHz);

		i_data |= i_data1;
		QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_08 , i_data);

		QM1D1C0045_d_reg[QM1D1C0045_REG_09] &= 0xC0;
		QM1D1C0045_d_reg[QM1D1C0045_REG_09] |= ((sd>>16)&0x3F);
		QM1D1C0045_d_reg[QM1D1C0045_REG_0A] = (UINT8)((sd>>8)&0xFF);
		QM1D1C0045_d_reg[QM1D1C0045_REG_0B] = (UINT8)(sd&0xFF);
		QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_09 , QM1D1C0045_d_reg[QM1D1C0045_REG_09]);
		QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_0A , QM1D1C0045_d_reg[QM1D1C0045_REG_0A]);
		QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_0B , QM1D1C0045_d_reg[QM1D1C0045_REG_0B]);
	}

	/*COMP_CTRL*/

	if(apConfig->ui_QM1D1C0045_XtalFreqKHz == 17000){
		COMP_CTRL = 0x03;
	}else if(apConfig->ui_QM1D1C0045_XtalFreqKHz == 18000){
		COMP_CTRL = 0x00;
	}else if(apConfig->ui_QM1D1C0045_XtalFreqKHz == 19000){
		COMP_CTRL = 0x01;
	}else if(apConfig->ui_QM1D1C0045_XtalFreqKHz == 20000){
		COMP_CTRL = 0x03;
	}else if(apConfig->ui_QM1D1C0045_XtalFreqKHz == 21000){
		COMP_CTRL = 0x04;
	}else if(apConfig->ui_QM1D1C0045_XtalFreqKHz == 22000){
		COMP_CTRL = 0x01;
	}else if(apConfig->ui_QM1D1C0045_XtalFreqKHz == 23000){
		COMP_CTRL = 0x02;
	}else if(apConfig->ui_QM1D1C0045_XtalFreqKHz == 24000){
		COMP_CTRL = 0x03;
	}else if(apConfig->ui_QM1D1C0045_XtalFreqKHz == 25000){
		COMP_CTRL = 0x04;
	}else if(apConfig->ui_QM1D1C0045_XtalFreqKHz == 26000){
		COMP_CTRL = 0x02;
	}else if(apConfig->ui_QM1D1C0045_XtalFreqKHz == 27000){
		COMP_CTRL = 0x03;
	}else if(apConfig->ui_QM1D1C0045_XtalFreqKHz == 28000){
		COMP_CTRL = 0x04;
	}else if(apConfig->ui_QM1D1C0045_XtalFreqKHz == 29000){
		COMP_CTRL = 0x04;
	}else if(apConfig->ui_QM1D1C0045_XtalFreqKHz == 30000){
		COMP_CTRL = 0x03;
	}else if(apConfig->ui_QM1D1C0045_XtalFreqKHz == 31000){
		COMP_CTRL = 0x04;
	}else if(apConfig->ui_QM1D1C0045_XtalFreqKHz == 32000){
		COMP_CTRL = 0x04;
	}else{
		COMP_CTRL = 0x02;
	}

	QM1D1C0045_d_reg[QM1D1C0045_REG_1D] &= 0xF8;
	QM1D1C0045_d_reg[QM1D1C0045_REG_1D] |= COMP_CTRL;
	QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_1D , QM1D1C0045_d_reg[QM1D1C0045_REG_1D]);

	/*VCO_TM , LPF_TM*/
	i_data = QM1D1C0045_pll_getdata_once(tuner_id, QM1D1C0045_REG_0C);
	i_data &= 0x3F;
	QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_0C , i_data);
	YWOS_TaskSleep(100);;//1024usec

	/*VCO_TM , LPF_TM*/
	i_data = QM1D1C0045_pll_getdata_once(tuner_id, QM1D1C0045_REG_0C);
	i_data |= 0xC0;
	QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_0C , i_data);

	//YWOS_TaskSleep(apConfig->ui_QM1D1C0045_LpfWaitTime*1000);//jhy modified,wait too long
	YWOS_TaskSleep(100);//jhy modified,wait too long

	/*LPF_FC*/
	QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_08 , QM1D1C0045_d_reg[QM1D1C0045_REG_08]);

	/*CSEL_Offset*/
	QM1D1C0045_d_reg[QM1D1C0045_REG_13] &= 0x9F;
	QM1D1C0045_pll_setdata_once(tuner_id, QM1D1C0045_REG_13 , QM1D1C0045_d_reg[QM1D1C0045_REG_13]);

	/*PLL Lock*/
	{
		BOOL bLock;
		QM1D1C0045_get_lock_status(tuner_id, &bLock);
		//printf("[%s]bLock=%d\n",__FUNCTION__,bLock);
		if(bLock!=TRUE)
		{
			return FALSE;
		}
	}
	return TRUE;
}

int  nim_vz7903_init(UINT8* ptuner_id)
{
	//INT32 result;
	//UINT8 i;
	//struct _QM1D1C0045_CONFIG_STRUCT *vz7903_cfg = NULL;
	struct _QM1D1C0045_CONFIG_STRUCT vz7903_cfg;
	BOOL rval;
	UINT32  tuner_id = 0;
	//UINT8 rbuf;
	if (ptuner_id)
	{
		tuner_id = *ptuner_id;
	}

	if (vz7903_tuner_cnt>=YWTUNERi_MAX_TUNER_NUM)
	{
		return ERR_FAILUE;
	}

	//vz7903_dev_id[vz7903_tuner_cnt] = vz7903_ptr;
	//*tuner_id = vz7903_tuner_cnt;
	//vz7903_tuner_cnt++;
	//jhy modified
	vz7903_tuner_cnt++;

	/*VZ7903_PRINTF("  VZ7903 initial --------------- ");
	 for(i=0;i<16;i++)
	{
		rbuf = 0xff;
		QM1D1C0045_register_real_write(vz7903_ptr->i2c_type_id, i, rbuf);
		VZ7903_PRINTF(" read reg %d is: ",i);
		QM1D1C0045_register_real_read(vz7903_ptr->i2c_type_id, i, &rbuf);
		// result = i2c_read(vz7903_ptr->i2c_type_id, 0xc0, &rbuf, 1);
		VZ7903_PRINTF(" : %d \n",rbuf);
	}	*/

	//vz7903_cfg = ( struct _QM1D1C0045_CONFIG_STRUCT*) YWOS_Malloc ( sizeof (struct _QM1D1C0045_CONFIG_STRUCT)) ;
	YWLIB_Memset(&vz7903_cfg,0,sizeof(struct _QM1D1C0045_CONFIG_STRUCT));

	vz7903_cfg.ui_QM1D1C0045_RFChannelkHz = 1550000 ;					/*XtalFreqKHz*/
	vz7903_cfg.ui_QM1D1C0045_XtalFreqKHz = 16000 ;						/*XtalFreqKHz*/
	vz7903_cfg.b_QM1D1C0045_fast_search_mode = FALSE ;						/*b_fast_search_mode*/
	vz7903_cfg.b_QM1D1C0045_loop_through = FALSE ;						/*b_loop_through*/
	vz7903_cfg.b_QM1D1C0045_tuner_standby = FALSE ;						/*b_tuner_standby*/
	vz7903_cfg.b_QM1D1C0045_head_amp = TRUE ;						/*b_head_amp*/
	vz7903_cfg.QM1D1C0045_lpf = QM1D1C0045_LPF_FC_10MHz ;	/*lpf*/
	vz7903_cfg.ui_QM1D1C0045_LpfWaitTime = 1;//20 ;//jhy change						/*QM1D1C0045_LpfWaitTime*/
	vz7903_cfg.ui_QM1D1C0045_FastSearchWaitTime = 4 ;							/*QM1D1C0045_FastSearchWaitTime*/
	vz7903_cfg.ui_QM1D1C0045_NormalSearchWaitTime = 15 ;						/*QM1D1C0045_NormalSearchWaitTime*/

	//BOOL				b_QM1D1C0045_iq_output;


	//rval = QM1D1C0045_Initialize(vz7903_ptr->i2c_type_id,vz7903_cfg ) ;
	rval = QM1D1C0045_Initialize(tuner_id,&vz7903_cfg ) ;
	//printf("[%s]rval=%d\n",__FUNCTION__,rval);


	/*
	if ((result = i2c_write(vz7903_ptr->i2c_type_id, vz7903_ptr->cTuner_Base_Addr, init_data1, sizeof(init_data1))) != SUCCESS)

	{
		VZ7903_PRINTF("nim_vz7903_init: I2C write error\n");
		return result;
	}

	if ((result = i2c_write(vz7903_ptr->i2c_type_id, vz7903_ptr->cTuner_Base_Addr, init_data2, sizeof(init_data2))) != SUCCESS)
		return result;


	osal_delay(10000);

	if ((result = i2c_write(vz7903_ptr->i2c_type_id, vz7903_ptr->cTuner_Base_Addr, init_data3, sizeof(init_data3))) != SUCCESS)
		return result;

	*/
	return SUCCESS;
}

/*****************************************************************************
* INT32 nim_vz7903_control(UINT32 freq, UINT8 sym, UINT8 cp)
*
* Tuner write operation
*
* Arguments:
*  Parameter1: UINT32 freq		: Synthesiser programmable divider(MHz)
*  Parameter2: UINT32 sym		: symbol rate (KHz)
*
* Return Value: INT32			: Result
*****************************************************************************/
int nim_vz7903_control(UINT8 tuner_id,UINT32 freq, UINT32 sym)
{
	//UINT8 data[4];
	//UINT16 Npro,tmp;
	UINT32 Rs, BW;
	//UINT8 Nswa;
	UINT8 LPF = 15;
	//UINT8 BA = 1;
	//UINT8 DIV = 0;
	//INT32 result;
    BOOL rval;
	struct _QM1D1C0045_CONFIG_STRUCT vz7903_cfg ;

	if(tuner_id>=vz7903_tuner_cnt||tuner_id>=YWTUNERi_MAX_TUNER_NUM)
		return ERR_FAILUE;
	//vz7903_ptr = vz7903_dev_id[tuner_id];

	 //vz7903_cfg = ( struct _QM1D1C0045_CONFIG_STRUCT*) YWOS_Malloc ( sizeof (struct _QM1D1C0045_CONFIG_STRUCT)) ;
	YWLIB_Memset(&vz7903_cfg,0,sizeof(struct _QM1D1C0045_CONFIG_STRUCT));
	vz7903_cfg.ui_QM1D1C0045_RFChannelkHz = 1550000 ;					/*XtalFreqKHz*/
	vz7903_cfg.ui_QM1D1C0045_XtalFreqKHz = 16000 ;						/*XtalFreqKHz*/
	vz7903_cfg.b_QM1D1C0045_fast_search_mode = FALSE ;						/*b_fast_search_mode*/
	vz7903_cfg.b_QM1D1C0045_loop_through = FALSE ;						/*b_loop_through*/
	vz7903_cfg.b_QM1D1C0045_tuner_standby = FALSE ;						/*b_tuner_standby*/
	vz7903_cfg.b_QM1D1C0045_head_amp = TRUE ;						/*b_head_amp*/
	vz7903_cfg.QM1D1C0045_lpf = QM1D1C0045_LPF_FC_10MHz ;	/*lpf*/
	vz7903_cfg.ui_QM1D1C0045_LpfWaitTime = 1;//20 ;	 jhy change					/*QM1D1C0045_LpfWaitTime*/
	vz7903_cfg.ui_QM1D1C0045_FastSearchWaitTime = 4 ;							/*QM1D1C0045_FastSearchWaitTime*/
	vz7903_cfg.ui_QM1D1C0045_NormalSearchWaitTime = 15 ;						/*QM1D1C0045_NormalSearchWaitTime*/

    /* LPF cut_off */
    Rs = sym;
    if (freq>2200)
        freq = 2200;
    if (Rs==0)
        Rs = 45000;
    BW = Rs*135/200;                // rolloff factor is 35%
    if (Rs<6500)  BW = BW + 4000;   // add 3M when Rs<5M, since we need shift 3M to avoid DC
    BW = BW + 2000;                 // add 2M for LNB frequency shifting
//ZCY: the cutoff freq of IX2410 is not 3dB point, it more like 0.1dB, so need not 30%
//  BW = BW*130/100;                // 0.1dB to 3dB need about 30% transition band for 7th order LPF
    BW = BW*108/100;                // add 8% margin since fc is not very accurate

    if (BW< 4000)   BW =  4000;     // Sharp2410 LPF can be tuned form 10M to 30M, step is 2.0M
    if (BW>34000)   BW = 34000;     // the range can be 6M~~34M actually, 4M is not good

    if (BW<=4000)  LPF = 0;
    else if (BW<=6000 )  LPF = 1;
    else if (BW<=8000 )  LPF = 2;
    else if (BW<=10000)  LPF = 3;
    else if (BW<=12000)  LPF = 4;
    else if (BW<=14000)  LPF = 5;
    else if (BW<=16000)  LPF = 6;
    else if (BW<=18000)  LPF = 7;
    else if (BW<=20000)  LPF = 8;
    else if (BW<=22000)  LPF = 9;
    else if (BW<=24000)  LPF = 10;
    else if (BW<=26000)  LPF = 11;
    else if (BW<=28000)  LPF = 12;
    else if (BW<=30000)  LPF = 13;
    else if (BW<=32000)  LPF = 14;
    else                 LPF = 15;


	vz7903_cfg.QM1D1C0045_lpf = (QM1D1C0045_LPF_FC) LPF;
	vz7903_cfg.ui_QM1D1C0045_RFChannelkHz = freq * 1000;
	//rval = QM1D1C0045_LocalLpfTuning(vz7903_ptr->i2c_type_id, vz7903_cfg) ;
	rval = QM1D1C0045_LocalLpfTuning(tuner_id, &vz7903_cfg) ;
	//printf("[%s]rval=%d\n",__FUNCTION__,rval);


	return SUCCESS;
}

/*****************************************************************************
* INT32 nim_vz7903_status(UINT8 *lock)
*
* Tuner read operation
*
* Arguments:
*  Parameter1: UINT8 *lock		: Phase lock status
*
* Return Value: INT32			: Result
*****************************************************************************/
int  nim_vz7903_status(UINT8 tuner_id,UINT8 *lock)
{
	//INT32 result;
	//UINT8 data;

	printk("[nim_vz7903_status]\n");
	//struct QPSK_TUNER_CONFIG_EXT * tuner_dev_ptr = NULL;
	if(tuner_id>=vz7903_tuner_cnt||tuner_id>=YWTUNERi_MAX_TUNER_NUM)
	{
		*lock = 0;
		return ERR_FAILUE;
	}
	*lock = 1;
	//tuner_dev_ptr = vz7903_dev_id[tuner_id];
/*
	if ((result = i2c_read(tuner_dev_ptr->i2c_type_id, tuner_dev_ptr->cTuner_Base_Addr, &data, 1)) == SUCCESS)
	{
		VZ7903_PRINTF("nim_vz7903_status: data = 0x%x\n", data);
		*lock = ((data & 0x40) >> 6);
	}
	*/
	return SUCCESS;
}


/*---add for Dvb-------*/

int vz7903_get_frequency(struct dvb_frontend *fe, u32 *frequency)
{
	struct dvb_frontend_ops	*frontend_ops = NULL;
	struct vz7903_state *vz7903_state = fe->tuner_priv;
	int err = 0;

	*frequency = vz7903_state->frequency;
		printk("%s: Frequency=%d\n", __func__, vz7903_state->frequency);
	return 0;
}

int vz7903_set_frequency(struct dvb_frontend *fe, u32 frequency)
{
	struct vz7903_state *vz7903_state = fe->tuner_priv;
    struct stv090x_state *state = fe->demodulator_priv;
	struct dvb_frontend_ops	*frontend_ops = NULL;
	struct dvb_tuner_ops	*tuner_ops = NULL;
	int err = 0;
	vz7903_state->frequency = frequency;
	err = nim_vz7903_control(0,frequency/1000,state->srate/1000);
	if (err != 0)
	{
		printk("nim_vz7903_control error!\n");
	}
	printk("%s: Frequency=%d symbolrate=%d\n", __func__,frequency/1000,state->srate/1000);
	return 0;
}

int vz7903_set_bandwidth(struct dvb_frontend *fe, u32 bandwidth)
{
	struct vz7903_state *state = fe->tuner_priv;

	state->bandwidth = bandwidth;

	return 0;
}
int vz7903_get_bandwidth(struct dvb_frontend *fe, u32 *bandwidth)
{
	struct vz7903_state *state = fe->tuner_priv;

	*bandwidth = state->bandwidth;

	printk("%s: Bandwidth=%d\n", __func__,state->bandwidth);
	return 0;
}
int vz7903_get_status(struct dvb_frontend *fe, u32 *status)
{
	struct vz7903_state *state = fe->tuner_priv;
	u8 lock;
	nim_vz7903_status(0,&lock);
	*status = (u32)lock;
	return 0;
}

static struct dvb_tuner_ops vz7903_ops = {

	.info = {
		.name		= "VZ7903",
		.frequency_min	=  950000,
		.frequency_max	= 2150000,
		.frequency_step = 0
	},
	.get_status	= vz7903_get_status,
	.release	= NULL
};

struct dvb_frontend *vz7903_attach(struct dvb_frontend *fe,
											struct vz7903_config *i2cConfig,
											struct i2c_adapter *i2c)
{
	struct vz7903_state *state = NULL;
	if ((state = kzalloc(sizeof (struct vz7903_state), GFP_KERNEL)) == NULL)
		goto exit;

	state->i2c		= i2c;
	state->fe		= fe;
	state->config =  i2cConfig;
	state->bandwidth	= 125000;
	state->symbolrate   = 0;
	fe->tuner_priv		= state;
	fe->ops.tuner_ops	= vz7903_ops;

	pVz7903_Config = state;

	UINT8 tunerID = 0;
	nim_vz7903_init(&tunerID); //tuner init

	printk("%s: Attaching vz7903  tuner\n",__func__);

	return fe;

exit:
	kfree(state);
	return NULL;
}



