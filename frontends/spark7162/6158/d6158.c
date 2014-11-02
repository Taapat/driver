/*$Source$*/
/*****************************文件头部注释*************************************/
//
//			Copyright (C), 2013-2018, AV Frontier Tech. Co., Ltd.
//
//
// 文 件 名：	$RCSfile$
//
// 创 建 者：	D26LF
//
// 创建时间：	2013.12.11
//
// 最后更新：	$Date$
//
//				$Author$
//
//				$Revision$
//
//				$State$
//
// 文件描述：	ter 6158
//
/******************************************************************************/

/********************************  文件包含************************************/
#include <linux/kernel.h>  /* Kernel support */
#include <linux/delay.h>
#include <linux/i2c.h>

#include <linux/dvb/version.h>


#include "ywdefs.h"
#include "ywos.h"
#include "ywlib.h"
#include "ywhal_assert.h"
#include "ywtuner_ext.h"
#include "tuner_def.h"
#include "ioarch.h"

#include "ioreg.h"
#include "tuner_interface.h"
#include "ywtuner_def.h"

#include "mxl_common.h"
#include "mxL_user_define.h"
#include "mxl301rf.h"
#include "tun_mxl301.h"
#include "dvb_frontend.h"
#include "d6158.h"

#include "stv0367ofdm_init.h"

#include "nim_panic6158_ext.h"

/********************************  常量定义************************************/

/********************************  数据结构************************************/

/********************************  宏 定 义************************************/

/********************************  变量定义************************************/

/********************************  变量引用************************************/

/********************************  函数声明************************************/


/********************************  函数定义************************************/

YW_ErrorType_T demod_d6158_Close(U8 Index)
{
	YW_ErrorType_T Error = YW_NO_ERROR;
	TUNER_ScanTaskParam_T *Inst;
	struct nim_device			*dev;
	TUNER_IOARCH_CloseParams_t CloseParams;

	Inst = TUNER_GetScanInfo(Index);

    dev = (struct nim_device *)Inst->userdata;

	TUNER_IOARCH_Close(dev->DemodIOHandle[2],&CloseParams);
	nim_panic6158_close(dev);
	if(YWTUNER_DELIVER_TER == Inst->Device)
	{
		Inst->DriverParam.Ter.DemodDriver.Demod_GetSignalInfo = NULL;
		Inst->DriverParam.Ter.DemodDriver.Demod_IsLocked = NULL;
		Inst->DriverParam.Ter.DemodDriver.Demod_repeat = NULL;
		Inst->DriverParam.Ter.DemodDriver.Demod_reset = NULL;
		Inst->DriverParam.Ter.DemodDriver.Demod_ScanFreq = NULL;

		Inst->DriverParam.Ter.Demod_DeviceMap.Error = YW_NO_ERROR;
	}
	else
	{
		Inst->DriverParam.Cab.DemodDriver.Demod_GetSignalInfo = NULL;
		Inst->DriverParam.Cab.DemodDriver.Demod_IsLocked = NULL;
		Inst->DriverParam.Cab.DemodDriver.Demod_repeat = NULL;
		Inst->DriverParam.Cab.DemodDriver.Demod_reset = NULL;
		Inst->DriverParam.Cab.DemodDriver.Demod_ScanFreq = NULL;

		Inst->DriverParam.Cab.Demod_DeviceMap.Error = YW_NO_ERROR;
	}
	return(Error);
}


/***********************************************************************
	函数名称:	demod_d6158_IsLocked

	函数说明:	读取信号是否锁定

************************************************************************/
YW_ErrorType_T demod_d6158_IsLocked(U8 Handle, BOOL *IsLocked)
{
    struct nim_device *dev = NULL;
	TUNER_ScanTaskParam_T   *Inst = NULL;
	UINT8 lock = 0;

    *IsLocked = FALSE;
	Inst = TUNER_GetScanInfo(Handle);
    if (Inst == NULL)
    {
        return 1;
    }
    dev = (struct nim_device *)Inst->userdata;
    if (dev == NULL)
    {
        return 1;
    }
	nim_panic6158_get_lock(dev,&lock);
	*IsLocked = (BOOL )lock;

	return YW_NO_ERROR;
}


/***********************************************************************
	函数名称:	demod_d6158_Identify

	函数说明:	检测硬件是否6158

************************************************************************/
int   demod_d6158_Identify(struct i2c_adapter* i2c_adap,U8 ucID)
{
	int ret;
	U8 pucActualID = 0;
	u8 b0[] = { DMD_CHIPRD };

	struct i2c_msg msg[] = {
		{ .addr	= PANIC6158_T2_ADDR >>1 , .flags	= 0, 		.buf = b0,   .len = 1 },
		{ .addr	= PANIC6158_T2_ADDR >>1 , .flags	= I2C_M_RD,	.buf = &pucActualID, .len = 1}
	};
	printk("pI2c = 0x%08x\n", (int)i2c_adap);
	ret = i2c_transfer(i2c_adap, msg, 2);
	if (ret == 2)
	{
		printf("pucActualID = %d\n", pucActualID);
    	if (pucActualID != 0x2)
        {
           // YWOSTRACE(( YWOS_TRACE_INFO, "[%s] YWHAL_ERROR_UNKNOWN_DEVICE!\n",__FUNCTION__) );
            printk("[%s] YWHAL_ERROR_UNKNOWN_DEVICE!\n",__FUNCTION__);
        	return YWHAL_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            printk("[%s] Got Demode 88472!\n",__FUNCTION__);
            return YW_NO_ERROR;
        }
    }
    else
    {
        YWOSTRACE(( YWOS_TRACE_ERROR, "[ERROR][%s]Can not find the MN88472,Check your Hardware!\n",__FUNCTION__) );
    	return YWHAL_ERROR_UNKNOWN_DEVICE;
    }
    return YWHAL_ERROR_UNKNOWN_DEVICE;
}
YW_ErrorType_T  demod_d6158_Repeat(IOARCH_Handle_t	    DemodIOHandle, /*demod io ??±ú*/
									IOARCH_Handle_t				TunerIOHandle, /*?°?? io ??±ú*/
									TUNER_IOARCH_Operation_t Operation,
									unsigned short SubAddr,
									unsigned char *Data,
									unsigned int TransferSize,
									unsigned int Timeout)
{
	(void)DemodIOHandle;
	(void)TunerIOHandle;
	(void)Operation;
	(void)SubAddr;
	(void)Data;
	(void)TransferSize;
	(void)Timeout;
    return 0;
}




YW_ErrorType_T demod_d6158_ScanFreqDVB(struct dvb_frontend_parameters *p,
	                                            struct nim_device *dev,UINT8   System,UINT8   plp_id)
{
   // struct nim_device           *dev;
    INT32 ret = 0;
    struct NIM_Channel_Change param;

	memset(&param, 0, sizeof(struct NIM_Channel_Change));

	if(dev == NULL)
	{

		return YWHAL_ERROR_BAD_PARAMETER;
	}

	if ((DEMO_BANK_T2 == System) || (DEMO_BANK_T == System))
	{

		//printf("TuneMode=%d\n", Inst->DriverParam.Ter.Param.TuneMode);
		param.priv_param = DEMO_UNKNOWN;//T2  T
		//param.priv_param = DEMO_DVBT;//T2  T
		//param.priv_param = DEMO_DVBT2;//T2  T

		//printk("p->frequency:%dKHz, bw:%dMHz\n",
		//		p->frequency, p->u.ofdm.bandwidth);
		param.freq = p->frequency;
		param.PLP_id = plp_id;
		switch(p->u.ofdm.bandwidth)
		{
		case BANDWIDTH_6_MHZ:
			param.bandwidth = MxL_BW_6MHz;
			break;
		case BANDWIDTH_7_MHZ:
			param.bandwidth = MxL_BW_7MHz;
			break;
		case BANDWIDTH_8_MHZ:
			param.bandwidth = MxL_BW_8MHz;
			break;
		default:
			return YWHAL_ERROR_BAD_PARAMETER;

		}
	}
	else if(DEMO_BANK_C == System)
	{
		param.priv_param = DEMO_DVBC;

		param.bandwidth = MxL_BW_8MHz;
		param.freq = p->frequency;
		switch(p->u.qam.modulation)
		{
		case QAM_16:
			param.modulation = QAM16;
			break;
		case QAM_32:
			param.modulation = QAM32;
			break;
		case QAM_64:
			param.modulation = QAM64;
			break;
		case QAM_128:
			param.modulation = QAM128;
			break;
		case QAM_256:
			param.modulation = QAM256;
			break;
		default:
			return YWHAL_ERROR_BAD_PARAMETER;
		}
	}

	//printf("[%s]%d,dev=0x%x\n",__FUNCTION__,__LINE__,dev);

    ret = nim_panic6158_channel_change(dev, &param);


    return ret;

}

YW_ErrorType_T demod_d6158earda_ScanFreq(struct dvb_frontend_parameters *p,
	                                            struct nim_device *dev,UINT8   System, UINT8   plp_id)
{
   // struct nim_device           *dev;
    INT32 ret = 0;
    struct NIM_Channel_Change param;

	memset(&param, 0, sizeof(struct NIM_Channel_Change));

	if(dev == NULL)
	{

		return YWHAL_ERROR_BAD_PARAMETER;
	}

	if ((DEMO_BANK_T2 == System) || (DEMO_BANK_T == System))
	{

		//printf("TuneMode=%d\n", Inst->DriverParam.Ter.Param.TuneMode);
		param.priv_param = DEMO_UNKNOWN;//T2  T
		//param.priv_param = DEMO_DVBT;//T2  T
		//param.priv_param = DEMO_DVBT2;//T2  T

		//printk("p->frequency:%dKHz, bw:%dMHz\n",
		//		p->frequency, p->u.ofdm.bandwidth);
		param.freq = p->frequency / 1000;
		param.PLP_id = plp_id;
		switch(p->u.ofdm.bandwidth)
		{
		case BANDWIDTH_6_MHZ:
			param.bandwidth = MxL_BW_6MHz;
			break;
		case BANDWIDTH_7_MHZ:
			param.bandwidth = MxL_BW_7MHz;
			break;
		case BANDWIDTH_8_MHZ:
			param.bandwidth = MxL_BW_8MHz;
			break;
		default:
			return YWHAL_ERROR_BAD_PARAMETER;

		}
	}
	else if(DEMO_BANK_C == System)
	{
		param.priv_param = DEMO_DVBC;

		param.bandwidth = MxL_BW_8MHz;
		param.freq = p->frequency / 1000;
		switch(p->u.qam.modulation)
		{
		case QAM_16:
			param.modulation = QAM16;
			break;
		case QAM_32:
			param.modulation = QAM32;
			break;
		case QAM_64:
			param.modulation = QAM64;
			break;
		case QAM_128:
			param.modulation = QAM128;
			break;
		case QAM_256:
			param.modulation = QAM256;
			break;
		default:
			return YWHAL_ERROR_BAD_PARAMETER;
		}
	}

	//printf("[%s]%d,dev=0x%x\n",__FUNCTION__,__LINE__,dev);

    ret = nim_panic6158_channel_change_earda(dev, &param);


    return ret;

}

static YW_ErrorType_T demod_d6158_ScanFreq(U8 Handle,
													TUNER_TunerType_T TunerType)
{
	TUNER_ScanTaskParam_T       *Inst = NULL;
    struct nim_device           *dev;
    INT32 ret = 0;
    struct NIM_Channel_Change param;

	YWLIB_Memset((void*)&param,0,sizeof(struct NIM_Channel_Change));

	//printf("Handle = %d\n", Handle);

	Inst = TUNER_GetScanInfo(Handle);
	Inst->Status = TUNER_INNER_STATUS_SCANNING;
	UINT8 *pPlpNum = &(Inst->DriverParam.Ter.Param.T2PlpNum);

	dev = (struct nim_device *)Inst->userdata;
	if(dev == NULL)
	{
		return YWHAL_ERROR_BAD_PARAMETER;
	}

	if(YWTUNER_DELIVER_TER == Inst->Device)
	{
		//printf("TuneMode=%d\n", Inst->DriverParam.Ter.Param.TuneMode);
		param.priv_param = Inst->DriverParam.Ter.Param.TuneMode;//T2  T
		//param.priv_param = DEMO_DVBT2;//T2  T

		param.freq = Inst->DriverParam.Ter.Param.FreqKHz;
		switch(Inst->DriverParam.Ter.Param.ChannelBW)
		{
		case YWTUNER_TER_BANDWIDTH_6_MHZ:
			param.bandwidth = MxL_BW_6MHz;
			break;
		case YWTUNER_TER_BANDWIDTH_7_MHZ:
			param.bandwidth = MxL_BW_7MHz;
			break;
		case YWTUNER_TER_BANDWIDTH_8_MHZ:
			param.bandwidth = MxL_BW_8MHz;
			break;
		default:
			return YWHAL_ERROR_BAD_PARAMETER;

		}
	}
	else if(YWTUNER_DELIVER_CAB == Inst->Device)
	{
		param.priv_param = DEMO_DVBC;

		param.bandwidth = MxL_BW_8MHz;
		param.freq = Inst->DriverParam.Cab.Param.FreqKHz;
		switch(Inst->DriverParam.Cab.Param.Modulation)
		{
		case YWTUNER_MOD_QAM_16:
			param.modulation = QAM16;
			break;
		case YWTUNER_MOD_QAM_32:
			param.modulation = QAM32;
			break;
		case YWTUNER_MOD_QAM_64:
			param.modulation = QAM64;
			break;
		case YWTUNER_MOD_QAM_128:
			param.modulation = QAM128;
			break;
		case YWTUNER_MOD_QAM_256:
			param.modulation = QAM256;
			break;
		default:
			return YWHAL_ERROR_BAD_PARAMETER;
		}
	}
	//printf("[%s]%d,dev=0x%x\n",__FUNCTION__,__LINE__,dev);

	if (TUNER_TUNER_MXL301 == TunerType)
	{
		ret = nim_panic6158_channel_change(dev, &param);
	}
	else if (TUNER_TUNER_MXL603 == TunerType)
	{
		struct nim_panic6158_private *priv;
		priv = (struct nim_panic6158_private *) dev->priv;
		priv->PLP_id = Inst->DriverParam.Ter.Param.T2PlpID;
		ret = nim_panic6158_channel_change_earda(dev, &param);
		*pPlpNum = priv->PLP_num;
	}

	return ret;
}

static YW_ErrorType_T demod_d6158_ScanFreq_301(U8 Handle)
{
	return demod_d6158_ScanFreq(Handle, TUNER_TUNER_MXL301);
}

static YW_ErrorType_T demod_d6158_ScanFreq_603(U8 Handle)
{
	return demod_d6158_ScanFreq(Handle, TUNER_TUNER_MXL603);
}

YW_ErrorType_T demod_d6158_Reset(U8 Index)
{
	YW_ErrorType_T Error = YW_NO_ERROR;

	(void)Index;

	/*TUNER_ScanTaskParam_T *Inst = NULL;
        struct nim_device *dev;

	Inst = TUNER_GetScanInfo(Index);
        dev = (struct nim_device *)Inst->userdata;*/

    return Error;
}

YW_ErrorType_T demod_d6158_GetSignalInfo(U8 Handle,
										unsigned int  *Quality,
										unsigned int  *Intensity,
										unsigned int  *Ber)
{
	YW_ErrorType_T              Error = YW_NO_ERROR;
	TUNER_ScanTaskParam_T       *Inst = NULL;
    struct nim_device *dev;

    Inst = TUNER_GetScanInfo(Handle);
    dev = (struct nim_device *)Inst->userdata;

	*Quality = 0;
	*Intensity = 0;
	*Ber = 0;

	/* Read noise estimations for C/N and BER */
	nim_panic6158_get_BER(dev, Ber);

    nim_panic6158_get_AGC(dev, (UINT8*)Intensity);  //level
	//printf("[%s]Intensity=%d\n",__FUNCTION__,*Intensity);
	if(*Intensity>90)
        *Intensity = 90;
    if (*Intensity < 10)
        *Intensity = *Intensity * 11/2;
    else
        *Intensity = *Intensity / 2 + 50;
    if(*Intensity>90)
        *Intensity = 90;

    nim_panic6158_get_SNR(dev, (UINT8*)Quality); //quality
	//printf("[%s]QualityValue=%d\n",__FUNCTION__,*Quality);
    if (*Quality < 30)
        *Quality = *Quality * 7/ 3;
    else
        *Quality = *Quality / 3 + 60;
	if(*Quality>90)
		*Quality = 90;


	return(Error);
}

void nim_config_EARDATEK11658(struct COFDM_TUNER_CONFIG_API *Tuner_API_T, UINT32 i2c_id, UINT8 idx)
{
	MEMSET(Tuner_API_T, 0x0, sizeof(struct COFDM_TUNER_CONFIG_API));

	Tuner_API_T->nim_Tuner_Init =  tun_mxl603_init;
	Tuner_API_T->nim_Tuner_Control =  tun_mxl603_control;
	Tuner_API_T->nim_Tuner_Status =  tun_mxl603_status;
	Tuner_API_T->nim_Tuner_Command  =  tun_mxl603_command;
	Tuner_API_T->tune_t2_first = 0;

	//Tuner_API_T->tuner_config.Tuner_Write = i2c_scb_write; //can be setted stb directly.
	//Tuner_API_T->tuner_config.Tuner_Read = i2c_scb_read; //can be setted stb directly.
	Tuner_API_T->tuner_config.cTuner_Base_Addr = 0xC0;
	Tuner_API_T->tuner_config.demo_type = PANASONIC_DEMODULATOR;
	Tuner_API_T->tuner_config.cChip = Tuner_Chip_MAXLINEAR;   //Tuner_Chip_PHILIPS
	Tuner_API_T->tuner_config.demo_addr = 0x38;
	Tuner_API_T->tuner_config.i2c_mutex_id = OSAL_INVALID_ID;

	Tuner_API_T->tuner_config.i2c_type_id = i2c_id;
	Tuner_API_T->ext_dm_config.i2c_type_id = i2c_id;
	Tuner_API_T->ext_dm_config.i2c_base_addr = 0xd8;

	Tuner_API_T->work_mode = NIM_COFDM_ONLY_MODE;
	Tuner_API_T->ts_mode = NIM_COFDM_TS_SSI;
	Tuner_API_T->ext_dm_config.nim_idx = idx;

	Tuner_API_T->config_data.Connection_config = 0x00;  //no I/Q swap.
	Tuner_API_T->config_data.Cofdm_Config = 0x03;        //low-if, dirct , if-enable.
	Tuner_API_T->config_data.AGC_REF = 0x63;
	Tuner_API_T->config_data.IF_AGC_MAX = 0xff;
	Tuner_API_T->config_data.IF_AGC_MIN = 0x00;
	Tuner_API_T->tuner_config.cTuner_Crystal      = 24;//16;       // !!! Unit:KHz	//88472 use 603 crystal
	Tuner_API_T->tuner_config.wTuner_IF_Freq      = 5000;
	Tuner_API_T->tuner_config.cTuner_AGC_TOP      = 252;          //Special for MaxLiner

}

static void nim_config_d61558(struct COFDM_TUNER_CONFIG_API *Tuner_API_T, UINT32 i2c_id, UINT8 idx)
{
	(void)i2c_id;
	(void)idx;

    YWLIB_Memset(Tuner_API_T, 0, sizeof(struct COFDM_TUNER_CONFIG_API));

	Tuner_API_T->nim_Tuner_Init = tun_mxl301_init;
	Tuner_API_T->nim_Tuner_Status = tun_mxl301_status;
	Tuner_API_T->nim_Tuner_Control = tun_mxl301_control;
	Tuner_API_T->tune_t2_first = 0;//when demod_d6158_ScanFreq,param.priv_param >DEMO_DVBT2,tuner tune T2 then t
	Tuner_API_T->tuner_config.demo_type = PANASONIC_DEMODULATOR;
	Tuner_API_T->tuner_config.cTuner_Base_Addr = 0xC2;
}

void demod_d6158_OpenInstallFunc(TUNER_ScanTaskParam_T *Inst,
													TUNER_TunerType_T TunerType)
{
	if(YWTUNER_DELIVER_TER == Inst->Device)
	{
		Inst->DriverParam.Ter.DemodDriver.Demod_GetSignalInfo = demod_d6158_GetSignalInfo;
		Inst->DriverParam.Ter.DemodDriver.Demod_IsLocked = demod_d6158_IsLocked;
		Inst->DriverParam.Ter.DemodDriver.Demod_repeat = demod_d6158_Repeat;
		Inst->DriverParam.Ter.DemodDriver.Demod_reset = demod_d6158_Reset;
		if (TUNER_TUNER_MXL301 == TunerType)
		{
			Inst->DriverParam.Ter.DemodDriver.Demod_ScanFreq = demod_d6158_ScanFreq_301;
		}
		else if (TUNER_TUNER_MXL603 == TunerType)
		{
			Inst->DriverParam.Ter.DemodDriver.Demod_ScanFreq = demod_d6158_ScanFreq_603;
		}
		Inst->DriverParam.Ter.DemodDriver.Demod_GetTransponder = NULL;
		Inst->DriverParam.Ter.DemodDriver.Demod_GetTransponderNum = NULL;
	}
	else if(YWTUNER_DELIVER_CAB == Inst->Device)
	{
		Inst->DriverParam.Cab.DemodDriver.Demod_GetSignalInfo = demod_d6158_GetSignalInfo;
		Inst->DriverParam.Cab.DemodDriver.Demod_IsLocked = demod_d6158_IsLocked;
		Inst->DriverParam.Cab.DemodDriver.Demod_repeat = demod_d6158_Repeat;
		Inst->DriverParam.Cab.DemodDriver.Demod_reset = demod_d6158_Reset;
		if (TUNER_TUNER_MXL301 == TunerType)
		{
			Inst->DriverParam.Cab.DemodDriver.Demod_ScanFreq = demod_d6158_ScanFreq_301;
		}
		else if (TUNER_TUNER_MXL603 == TunerType)
		{
			Inst->DriverParam.Cab.DemodDriver.Demod_ScanFreq = demod_d6158_ScanFreq_603;
		}
	}
}

YW_ErrorType_T demod_d6158_Open(U8 Handle)
{
	TUNER_ScanTaskParam_T       *Inst = NULL;
    struct COFDM_TUNER_CONFIG_API	Tuner_API;
	YW_ErrorType_T YW_ErrorCode = 0;
	INT32 ret =0;

	printf("[%s]%d,IN ok \n",__FUNCTION__,__LINE__);

	Inst = TUNER_GetScanInfo(Handle);

	nim_config_d61558(&Tuner_API, 0, 0);
	ret = nim_panic6158_attach(Handle, &Tuner_API, Inst->pOpenParams);
	if (SUCCESS != ret)
	{
		printf("nim_panic6158_attach error\n");

		nim_config_EARDATEK11658(&Tuner_API, 0, 0);
		ret = nim_panic6158_attach_earda(Handle, &Tuner_API, Inst->pOpenParams);
		if (SUCCESS != ret)
		{
			printf("nim_panic6158_attach_earda error\n");
			return YWTUNER_ERROR_OPEN;
		}

		demod_d6158_OpenInstallFunc(Inst, TUNER_TUNER_MXL603);
	}
	else
	{
		demod_d6158_OpenInstallFunc(Inst, TUNER_TUNER_MXL301);
	}

    /*{
		struct nim_device           *dev;
		//struct NIM_Channel_Change param;

	    dev = (struct nim_device *)Inst->userdata;
		printf("[%s]%d,out ret=%d,dev =0x%x \n",__FUNCTION__,__LINE__,ret,dev );

		//YWLIB_Memset(&param,0,sizeof(NIM_Channel_Change));
		//nim_panic6158_channel_change(dev,&param);

	}*/

    return YW_ErrorCode;
}

void osal_task_sleep(U32 ms)
{
	YWOS_TaskSleep(ms);
}

OSAL_ID osal_mutex_create(void)
{
	return 1;
}

/* EOF------------------------------------------------------------------------*/

/* BOL-------------------------------------------------------------------*/
//$Log$
/* EOL-------------------------------------------------------------------*/

