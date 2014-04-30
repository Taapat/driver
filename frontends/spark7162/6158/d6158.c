/*****************************************************************************
*    Copyright (C)2011 FULAN. All Rights Reserved.
*
*    File:    nim_panic6158.c
*
*    Description:    Source file in LLD.
*
*    History:
*
*           Date            Athor        Version          Reason
*	    ============	=============	=========	=================
*	1.2011/12/12     DMQ      Ver 0.1       Create file.
*
*****************************************************************************/
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
#include "Nim_tuner.h"
#include "nim_dev.h"
#include "d6158.h"

#include "dvb_frontend.h"
#include "stv0367ofdm_init.h"

#include "../../base/mxl301.h"


struct dvb_d6158_fe_ofdm_state {
	struct nim_device 	   spark_nimdev;
	struct i2c_adapter			*i2c;
	struct dvb_frontend 		frontend;
	//IOARCH_Handle_t				IOHandle;
	TUNER_IOREG_DeviceMap_t		DeviceMap;
	struct dvb_frontend_parameters 	*p;
};



UINT8 nim_panic6158_cnt = 0;
/*static const char *nim_panic6158_name[] =
{
	"NIM_PANIC6158_0",
	"NIM_PANIC6158_1",
};*/

//#define  TUNER_IOARCH_MAX_HANDLES   12

//IOARCH_HandleData_t IOARCH_Handle[TUNER_IOARCH_MAX_HANDLES];

INT32 tun_mxl301_set_addr(UINT32 tuner_idx, UINT8 addr, UINT32 i2c_mutex_id);
INT32 MxL_Check_RF_Input_Power(UINT32 tuner_idx, U32* RF_Input_Level);

#if 0
static INT32 nim_reg_write(struct nim_device *dev, UINT8 mode, UINT8 reg, UINT8 *data, UINT8 len)
{
	INT32 ret = SUCCESS;
	UINT16 i, cycle;
	UINT8 value[DEMO_I2C_MAX_LEN+1];
	struct nim_panic6158_private *priv = (struct nim_panic6158_private *)dev->priv;

	if (DEMO_BANK_C < mode)
		return !SUCCESS;

	cycle = len/DEMO_I2C_MAX_LEN;

	osal_mutex_lock(priv->i2c_mutex_id, OSAL_WAIT_FOREVER_TIME);
	for (i = 0; i < cycle; i++)
	{
		value[0] = reg+i*DEMO_I2C_MAX_LEN;
		MEMCPY(&value[1], &data[i*DEMO_I2C_MAX_LEN], DEMO_I2C_MAX_LEN);
		ret = i2c_write(dev->i2c_type_id, priv->i2c_addr[mode], value, DEMO_I2C_MAX_LEN+1);
		if (SUCCESS != ret)
		{
			osal_mutex_unlock(priv->i2c_mutex_id);
			NIM_PANIC6158_PRINTF("nim write i2c failed!\n");
			return ret;
		}
	}

	if (len > i*DEMO_I2C_MAX_LEN)
	{
		value[0] = reg+i*DEMO_I2C_MAX_LEN;
		MEMCPY(&value[1], &data[i*DEMO_I2C_MAX_LEN], len-i*DEMO_I2C_MAX_LEN);
		ret = i2c_write(dev->i2c_type_id, priv->i2c_addr[mode], value, len-i*DEMO_I2C_MAX_LEN+1);
		if (SUCCESS != ret)
		{
			osal_mutex_unlock(priv->i2c_mutex_id);
			NIM_PANIC6158_PRINTF("nim write i2c failed!\n");
			return ret;
		}
	}
	osal_mutex_unlock(priv->i2c_mutex_id);

	return ret;
}

static INT32 nim_reg_read(struct nim_device *dev, UINT8 mode, UINT8 reg, UINT8 *data, UINT8 len)
{
	INT32 ret = SUCCESS;
	struct nim_panic6158_private *priv = (struct nim_panic6158_private *)dev->priv;

	if (DEMO_BANK_C < mode || DEMO_I2C_MAX_LEN < len)
		return !SUCCESS;

	osal_mutex_lock(priv->i2c_mutex_id, OSAL_WAIT_FOREVER_TIME);
	ret = i2c_write(dev->i2c_type_id, priv->i2c_addr[mode], &reg, 1);
	if (SUCCESS != ret)
	{
		osal_mutex_unlock(priv->i2c_mutex_id);
		NIM_PANIC6158_PRINTF("nim write i2c failed!\n");
		return ret;
	}

	ret = i2c_read(dev->i2c_type_id, priv->i2c_addr[mode], data, len);
	if (SUCCESS != ret)
	{
		NIM_PANIC6158_PRINTF("nim read i2c failed!\n");
	}

	osal_mutex_unlock(priv->i2c_mutex_id);
	return ret;
}
#else
static INT32 nim_reg_read(struct nim_device *dev,UINT8 mode, UINT8 bMemAdr, UINT8 *pData, UINT8 bLen)
{

	//INT32 err;
	//err = TUNER_IOARCH_ReadWrite(dev->DemodIOHandle[mode], TUNER_IO_SA_READ, bMemAdr, pData, bLen, 50);

	//return err;
	int ret;
    struct nim_panic6158_private *priv_mem = (struct nim_panic6158_private *)dev->priv;
	u8 b0[] = { bMemAdr };

	struct i2c_msg msg[] = {
		{ .addr	= priv_mem->i2c_addr[mode] >>1 , .flags	= 0, 		.buf = b0,   .len = 1 },
		{ .addr	= priv_mem->i2c_addr[mode] >>1 , .flags	= I2C_M_RD,	.buf = pData, .len = bLen }
	};

	ret = i2c_transfer(priv_mem->i2c_adap, msg, 2);


#if 0
	int i;
	printk("[ DEMOD ]len:%d MemAddr:%x addr%x\n",bLen,bMemAdr,dev->base_addr);
	for(i=0;i<bLen;i++)
	{
		printk("[DATA]%x ",pData[i]);
	}
	printk("\n");
#endif
	if (ret != 2)
	{
		if (ret != -ERESTARTSYS)
			printk(	"READ ERROR, Reg=[0x%02x], Status=%d\n",bMemAdr, ret);

		return ret < 0 ? ret : -EREMOTEIO;
	}

	return SUCCESS;

}
static INT32 nim_reg_write(struct nim_device *dev, UINT8 mode,UINT8 bMemAdr, UINT8 *pData, UINT8 bLen)
{
    //INT32 err;
	//printf("mode = %d\n", mode);
	//printf("dev->DemodIOHandle[mode] = %d\n", dev->DemodIOHandle[mode]);
	//err = TUNER_IOARCH_ReadWrite(dev->DemodIOHandle[mode], TUNER_IO_SA_WRITE, bMemAdr, pData, bLen, 50) ;
   // return err;


	int ret;
	u8 buf[1 + bLen];

    struct nim_panic6158_private *priv_mem = (struct nim_panic6158_private *)dev->priv;

	struct i2c_msg i2c_msg = {.addr = priv_mem->i2c_addr[mode] >>1 , .flags = 0, .buf = buf, .len = 1 + bLen };


#if 0
	int i;
	for (i = 0; i < bLen; i++)
	{
		pData[i] = 0x92 + 3*i;
		printk("%02x ", pData[i]);
	}
	printk("  write addr %x\n",dev->base_addr);
#endif
	priv_mem = (struct nim_panic6158_private *)dev->priv;

	buf[0] = bMemAdr;
	memcpy(&buf[1], pData, bLen);

	ret = i2c_transfer(priv_mem->i2c_adap, &i2c_msg, 1);

	if (ret != 1) {
		//if (ret != -ERESTARTSYS)
			printk("WRITE ERR:Reg=[0x%04x], Data=[0x%02x ...], Count=%u, Status=%d\n",
				bMemAdr, pData[0], bLen, ret);
		return ret < 0 ? ret : -EREMOTEIO;
	}
    return SUCCESS;
}

#endif

static INT32 nim_reg_mask_write(struct nim_device *dev, UINT8 mode, UINT8 reg, UINT8 mask, UINT8 data)
{
	INT32 ret;
	UINT8 rd;
	//printf("[%s]%d\n",__FUNCTION__,__LINE__);

	ret = nim_reg_read(dev, mode, reg, &rd, 1);

	//printf("[%s]%d\n",__FUNCTION__,__LINE__);
	rd |= mask & data;
	rd &= (mask^0xff) | data;

	//printf("[%s]%d\n",__FUNCTION__,__LINE__);
	ret |= nim_reg_write(dev, mode, reg, &rd, 1);
	//printf("[%s]%d\n",__FUNCTION__,__LINE__);

	return ret;
}

static INT32 log10_easy(UINT32 cnr)
{
	INT32 ret;
	UINT32 c;

	c = 0;
	while( cnr > 100 )
	{
		cnr = cnr / 10;
		c++;
	}
	ret = logtbl[cnr] + c*1000 + 1000;

	return ret;
}

static INT32 nim_calculate_ber(struct nim_device *dev, UINT32 *err, UINT32 *len, UINT32 *sum)
{
	UINT8 data[3];
	struct nim_panic6158_private *priv;

	priv = (struct nim_panic6158_private *)dev->priv;

	if (DEMO_BANK_T == priv->system || DEMO_BANK_C == priv->system)
	{
		//SET BERSET1[5] = 0
		nim_reg_read(dev, DEMO_BANK_T, DMD_BERSET1_T, data, 1);
		data[0] &= 0xDF;	//1101_1111
		nim_reg_write(dev, DEMO_BANK_T, DMD_BERSET1_T, data, 1);
		//SET BERRDSET[3:0] = 0101
		nim_reg_read(dev, DEMO_BANK_T, DMD_BERRDSET_T, data, 1);
		data[0] = (data[0] & 0xF0 ) | 0x5;
		nim_reg_write(dev, DEMO_BANK_T, DMD_BERRDSET_T, data, 1);

		//Read ERROR
		nim_reg_read(dev, DEMO_BANK_T, DMD_BERRDU_T, data, 3);
		*err = data[0]*0x10000 + data[1]*0x100 + data[2];
		//Read BERLEN
		nim_reg_read(dev, DEMO_BANK_T, DMD_BERLENRDU_T, data, 2);
		*len = *sum = data[0]*0x100 + data[1];

		*sum = *sum * 203 * 8;
	}
	else if (DEMO_BANK_T2 == priv->system)
	{
		UINT8 common = 0;
		nim_reg_read(dev, DEMO_BANK_T2, DMD_BERSET , data, 1);

		if(common == 0 )
		{
			data[0] |= 0x20;//BERSET[5] = 1 (BER after LDPC)
			data[0] &= 0xef;//BERSET[4] = 0 (Data PLP)
		}
		else
		{
			data[0] |= 0x20;//BERSET[5] = 1 (BER after LDPC)
			data[0] |= 0x10;//BERSET[4] = 1 (Common PLP)
		}
		nim_reg_write(dev, DEMO_BANK_T2, DMD_BERSET, data, 1);

		//Read ERROR
		nim_reg_read(dev, DEMO_BANK_T2, DMD_BERRDU, data, 3);
		*err = data[0]*0x10000 + data[1]*0x100 + data[2];

		//Read BERLEN
		nim_reg_read(dev, DEMO_BANK_T2, DMD_BERLEN, data, 1);
		*len = *sum = (1 << (data[0] & 0xf));
		if(common == 0)
		{
			data[0] = 0x3;
			nim_reg_write(dev, DEMO_BANK_T2, DMD_TPDSET2, data, 1);
		}
		else
		{
			nim_reg_read(dev, DEMO_BANK_T2, DMD_TPD2, data, 1);
		}

		if((data[0] & 0x1) == 1)
		{
			//FEC TYPE = 1
			switch((data[0]>>2) & 0x7)
			{
				case 0:	*sum = (*sum) * 32400; break;
				case 1:	*sum = (*sum) * 38880; break;
				case 2:	*sum = (*sum) * 43200; break;
				case 3:	*sum = (*sum) * 48600; break;
				case 4:	*sum = (*sum) * 51840; break;
				case 5:	*sum = (*sum) * 54000; break;
			}
		}
		else
		{
			//FEC TYPE = 0
			switch((data[0]>>2) & 0x7)
			{
				case 0:	*sum = (*sum) * 7200 * 4; break;
				case 1:	*sum = (*sum) * 9720 * 4; break;
				case 2:	*sum = (*sum) * 10800* 4; break;
				case 3:	*sum = (*sum) * 11880* 4; break;
				case 4:	*sum = (*sum) * 12600* 4; break;
				case 5:	*sum = (*sum) * 13320* 4; break;
			}
		}
	}
	return SUCCESS;
}

static INT32 nim_calculate_cnr(struct nim_device *dev, UINT32 *cnr_i, UINT32 *cnr_d)
{
	UINT8 data[4];
	INT32 cnr;
	INT32 sig, noise;
	struct nim_panic6158_private *priv;

	priv = (struct nim_panic6158_private *)dev->priv;

	switch(priv->system)
	{
		case DEMO_BANK_T:
			nim_reg_read(dev, DEMO_BANK_T, DMD_CNRDU_T, &data[0], 2);
			cnr = data[0] * 0x100 + data[1];

			if( cnr != 0 )
			{
				cnr = 65536 / cnr;
				cnr = log10_easy( cnr ) + 200;
				if( cnr < 0 ) cnr = 0;
			}
			else
				cnr = 0;

			*cnr_i = (UINT32 ) cnr / 100;
			*cnr_d = (UINT32 ) cnr % 100;
			break;

		case DEMO_BANK_T2:
			nim_reg_read(dev, DEMO_BANK_T2, DMD_CNFLG, &data[0], 3);
			cnr = data[1]*0x100 + data[2];
			if(cnr != 0)
			{
				if(data[0] & 0x4)
				{
					//MISO
					cnr = 16384 / cnr;
					cnr = log10_easy( cnr ) - 600;
					if( cnr < 0 ) cnr = 0;
					*cnr_i = (UINT32 ) cnr / 100;
					*cnr_d = (UINT32 ) cnr % 100;
				}
				else
				{
					//SISO
					cnr = 65536 / cnr;
					cnr = log10_easy( cnr ) + 200;
					if( cnr < 0 ) cnr = 0;
					*cnr_i = (UINT32 ) cnr / 100;
					*cnr_d = (UINT32 ) cnr % 100;
				}
			}
			else
			{
				*cnr_i = 0;
				*cnr_d = 0;
			}
			break;

		case DEMO_BANK_C:
			nim_reg_read(dev, DEMO_BANK_C, DMD_CNMON1_C, data, 4);
			sig = data[0]*0x100 + data[1];
			noise = data[2]*0x100 + data[3];

			if( noise != 0 )
				cnr = log10_easy(sig * 8  / noise);
			else
				cnr = 0;

			if( cnr < 0 ) cnr = 0;
			*cnr_i = (UINT32 ) cnr / 100;
			*cnr_d = (UINT32 ) cnr % 100;
			break;

		default:
			break;
	}
	return 0;
}

static INT32 nim_panic6158_set_reg(struct nim_device *dev, DMD_I2C_Register_t *reg)
{
	INT32 i;
	UINT8 mode;
	for (i = 0; i < DMD_REGISTER_MAX; i++)
	{
		if (DMD_E_END_OF_ARRAY == reg[i].flag)
			break;

		switch(reg[i].slvadr)
		{
			case 0x1c:
				mode = DEMO_BANK_T2;
				break;

			case 0x18:
				mode = DEMO_BANK_T;
				//continue;
				break;
			case 0x1a:
				mode = DEMO_BANK_C;
				//continue;
				break;

			default:
				return !SUCCESS;
				break;
		}
		if(SUCCESS != nim_reg_write(dev, mode, reg[i].adr, &reg[i].data, 1))
			return !SUCCESS;

	}
	return SUCCESS;
}

/* **************************************************** */
/*!	Set Register setting for each broadcast system & bandwidth */
/* **************************************************** */
static INT32 nim_panic6158_set_system(struct nim_device *dev, UINT8 system, UINT8 bw, DMD_IF_FREQ_t if_freq)
{
	INT32 ret = SUCCESS;
	UINT8 nosupport = 0;

	switch(system)
	{
		case DEMO_BANK_T2:
			switch(bw)
			{
				case DMD_E_BW_8MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_DVBT2_8MHZ);
					if(if_freq!=DMD_E_IF_5000KHZ)
						nosupport = 1;
					break;
				case DMD_E_BW_6MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_DVBT2_6MHZ);
					if(if_freq!=DMD_E_IF_5000KHZ)
						nosupport = 1;
					break;
				case DMD_E_BW_5MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_DVBT2_5MHZ);
					if(if_freq!=DMD_E_IF_5000KHZ)
						nosupport = 1;
					break;
				case DMD_E_BW_1_7MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_DVBT2_1_7MHZ);
					if(if_freq!=DMD_E_IF_5000KHZ)
						nosupport = 1;
					break;
				case DMD_E_BW_7MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_DVBT2_7MHZ);

					switch(if_freq)
					{
						case DMD_E_IF_5000KHZ:
							break;

						/* '11/08/12 : OKAMOTO Implement IF 4.5MHz for DVB-T/T2 7MHz. */
						case DMD_E_IF_4500KHZ:
							ret = nim_panic6158_set_reg(dev, MN88472_REG_DIFF_DVBT2_7MHZ_IF4500KHZ);
							break;

						default:
							nosupport = 1;
							break;
					}
					break;

				default:
					nosupport = 1;
					break;
			}
			break;

		case DEMO_BANK_T:
			switch(bw)
			{
				case DMD_E_BW_8MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_DVBT_8MHZ);
					if(if_freq!=DMD_E_IF_5000KHZ)
						nosupport = 1;
					break;
				case DMD_E_BW_7MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_DVBT_7MHZ);
					switch(if_freq)
					{
						case DMD_E_IF_5000KHZ:
							break;

						/* '11/08/12 : OKAMOTO Implement IF 4.5MHz for DVB-T/T2 7MHz. */
						case DMD_E_IF_4500KHZ:
							ret = nim_panic6158_set_reg(dev, MN88472_REG_DIFF_DVBT_7MHZ_IF4500KHZ);
							break;

						default:
							nosupport = 1;
							break;
					}
					break;

				case DMD_E_BW_6MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_DVBT_6MHZ);
					if(if_freq!=DMD_E_IF_5000KHZ)
						nosupport = 1;
					break;

				default:
					nosupport = 1;
					break;
			}
			break;

		case DEMO_BANK_C:
				ret = nim_panic6158_set_reg(dev, MN88472_REG_DVBC);
			break;

		default:
				nosupport = 1;
				break;
	}

	if(1 == nosupport)
		NIM_PANIC6158_PRINTF( "ERROR : Not Supported System");

	return ret;
}

static INT32 nim_panic6158_set_error_flag(struct nim_device *dev, UINT8 flag)
{
	UINT8 data;

	if (flag)
	{
		/* 1st,Adr:0x09(TSSET2) bit[2:0]=1 ("001") */
		if (SUCCESS !=  nim_reg_read(dev, DEMO_BANK_T2, DMD_TSSET2, &data, 1))
			return !SUCCESS;

		data &= 0xF8;
		data |= 0x1;
		if (SUCCESS != nim_reg_write(dev, DEMO_BANK_T2, DMD_TSSET2, &data, 1))
			return !SUCCESS;

		/* 1st,Adr:0xD9(FLGSET) bit[6:5]=0  ("00") */
		if (SUCCESS != nim_reg_read(dev, DEMO_BANK_T2, DMD_FLGSET, &data, 1))
			return !SUCCESS;

		data &= 0x9F;
		if (SUCCESS != nim_reg_write(dev, DEMO_BANK_T2, DMD_FLGSET, &data, 1))
			return !SUCCESS;
	}

	return SUCCESS;
}

/* '11/08/29 : OKAMOTO	Select TS output. */
static INT32 nim_panic6158_set_ts_output(struct nim_device *dev, DMD_TSOUT ts_out)
{
	UINT8 data;

	switch(ts_out)
	{
		case DMD_E_TSOUT_PARALLEL_FIXED_CLOCK:
			//TS parallel (Fixed clock mode) SSET1:0x00 FIFOSET:0xE1
			data = 0x0;
			if(SUCCESS != nim_reg_write(dev, DEMO_BANK_T2, DMD_TSSET1, &data, 1))
				return !SUCCESS;

			data = 0xE1;
			if(SUCCESS != nim_reg_write(dev, DEMO_BANK_T, DMD_FIFOSET, &data, 1))
				return !SUCCESS;
			break;

		case DMD_E_TSOUT_PARALLEL_VARIABLE_CLOCK:
			//TS parallel (Variable clock mode) TSSET1:0x00 FIFOSET:0xE3
			data = 0x0;
			if(SUCCESS != nim_reg_write(dev, DEMO_BANK_T2, DMD_TSSET1, &data, 1))
				return !SUCCESS;

			data = 0xE3;
			if(SUCCESS != nim_reg_write(dev, DEMO_BANK_T, DMD_FIFOSET, &data, 1))
				return !SUCCESS;
			break;

		case DMD_E_TSOUT_SERIAL_VARIABLE_CLOCK:
			//TS serial(Variable clock mode) TSSET1:0x1D	FIFOSET:0xE3
			data = 0x1D;
			if(SUCCESS != nim_reg_write(dev, DEMO_BANK_T2, DMD_TSSET1, &data, 1))
				return !SUCCESS;

			data = 0xE3;
			if(SUCCESS != nim_reg_write(dev, DEMO_BANK_T, DMD_FIFOSET, &data, 1))
				return !SUCCESS;
			break;

		default:
			break;
	}
	return SUCCESS;
}

INT32 nim_panic6158_set_info(struct nim_device *dev, UINT8 system, UINT32 id , UINT32 val)
{
	INT32 ret = !SUCCESS;
	UINT8 rd, data;

	switch(system)
	{
		case DEMO_BANK_T:
			switch( id )
			{
			case DMD_E_INFO_DVBT_HIERARCHY_SELECT:
				nim_reg_read(dev, system , DMD_RSDSET_T, &rd, 1);
				if( val == 1 )
					rd |= 0x10;
				else
					rd &= 0xef;
				nim_reg_write(dev, system, DMD_RSDSET_T , &rd, 1);
				//param->info[DMD_E_INFO_DVBT_HIERARCHY_SELECT]	= (rd >> 4) & 0x1;//dmq modify
				ret = SUCCESS;
				break;

			/* '11/11/14 : OKAMOTO	Update to "MN88472_Device_Driver_111028". */
			case DMD_E_INFO_DVBT_MODE:
				ret = SUCCESS;
				if( val == DMD_E_DVBT_MODE_NOT_DEFINED)
				{
					data = 0xba;
					nim_reg_write(dev, system, DMD_MDSET_T, &data, 1);
					data = 0x13;
					nim_reg_write(dev, system, DMD_MDASET_T, &data, 1);
				}
				else
				{
					nim_reg_mask_write(dev, system , DMD_MDSET_T, 0xf0 , 0xf0);
					data = 0;
					nim_reg_write(dev, system, DMD_MDASET_T, &data, 1);
					switch( val )
					{
					case DMD_E_DVBT_MODE_2K:
						nim_reg_mask_write(dev, system, DMD_MDSET_T, 0x0c, 0x00);
						break;
					case DMD_E_DVBT_MODE_8K:
						nim_reg_mask_write(dev, system, DMD_MDSET_T, 0x0c, 0x08);
						break;
					default:
						ret = !SUCCESS;
						break;
					}
				}
				break;

			/* '11/11/14 : OKAMOTO	Update to "MN88472_Device_Driver_111028". */
			case DMD_E_INFO_DVBT_GI:
				ret = SUCCESS;
				if( val == DMD_E_DVBT_GI_NOT_DEFINED)
				{
					data = 0xba;
					nim_reg_write(dev, system, DMD_MDSET_T, &data, 1);
					data = 0x13;
					nim_reg_write(dev, system, DMD_MDASET_T, &data, 1);
				}
				else
				{
					nim_reg_mask_write(dev, system, DMD_MDSET_T , 0xf0, 0xf0);
					data = 0;
					nim_reg_write(dev, system , DMD_MDASET_T, &data, 1);
					switch( val )
					{
					case DMD_E_DVBT_GI_1_32:
						nim_reg_mask_write(dev, system, DMD_MDSET_T, 0x03, 0x00);
						break;
					case DMD_E_DVBT_GI_1_16:
						nim_reg_mask_write(dev, system, DMD_MDSET_T, 0x03, 0x01);
						break;
					case DMD_E_DVBT_GI_1_8:
						nim_reg_mask_write(dev, system, DMD_MDSET_T, 0x03, 0x02);
						break;
					case DMD_E_DVBT_GI_1_4:
						nim_reg_mask_write(dev, system, DMD_MDSET_T, 0x03, 0x03);
						break;
					default:
						ret = !SUCCESS;
						break;
					}

				}
				break;
			}

		case DEMO_BANK_T2:
			switch(id)
			{
				case	DMD_E_INFO_DVBT2_SELECTED_PLP	:
					rd = (UINT8) val;
					nim_reg_write(dev, system, DMD_PLPID, &rd, 1);
					ret = SUCCESS;
					break;
			}

		/* '11/10/19 : OKAMOTO	Correct warning: enumeration value 乪DMD_E_ISDBT乫 not handled in switch */
		default:
			ret = !SUCCESS;
			break;
	}

	return ret;
}

static INT32 nim_panic6158_get_lock(struct nim_device *dev, UINT8 *lock)
{
	UINT8 data = 0;
	struct nim_panic6158_private *priv = (struct nim_panic6158_private *)dev->priv;

	*lock = 0;
	if (DEMO_BANK_T2 == priv->system)
	{
		nim_reg_read(dev, priv->system, DMD_SSEQFLG, &data, 1);
		//printk("T2[%s]%d,data =0x%x\n",__FUNCTION__,__LINE__,data);
		if (13 <= (0x0F & data))
			*lock = 1;
	}
	else if (DEMO_BANK_T == priv->system)
	{
		nim_reg_read(dev, priv->system, DMD_SSEQRD_T, &data, 1);
		//printf("[%s]%d,data =0x%x\n",__FUNCTION__,__LINE__,data);
		if (9 <= (0x0F & data))
			*lock = 1;
	}
	else if (DEMO_BANK_C == priv->system)
	{
		//printf("[%s]%d,DEMO_BANK_C\n",__FUNCTION__,__LINE__);
		nim_reg_read(dev, priv->system, DMD_SSEQMON2_C, &data, 1);
		//printf("[%s]%d,data =0x%x\n",__FUNCTION__,__LINE__,data);
		if (8 <= (0x0F & data))
			*lock = 1;
	}

	NIM_PANIC6158_PRINTF("data = %x\n", data&0xf);

	return SUCCESS;
}



static INT32 nim_panic6158_get_SNR(struct nim_device *dev, UINT8 *snr)
{
	//INT32 result;
	UINT32 cnr;
	INT32 cnr_rel;
	UINT8 data[3];
	UINT32 beri;
	UINT32 mod, cr;
	UINT32 sqi, ber_sqi;
	UINT32 ber_err, ber_sum, ber_len;
	UINT32 cnr_i = 0, cnr_d = 0;
	struct nim_panic6158_private *priv;

	//UINT32 tick;
	//tick = osal_get_tick();

	priv = (struct nim_panic6158_private *)dev->priv;

	nim_calculate_ber(dev, &ber_err, &ber_len, &ber_sum);
	nim_calculate_cnr(dev, &cnr_i, &cnr_d);

	cnr = cnr_i * 100 + cnr_d;

	if(0 !=ber_err)
		beri = ber_sum / ber_err;
	else
		beri = 100000000;

	//printf("[%s]cnr=%d,beri=%d\n",__FUNCTION__,cnr,beri);

	if (DEMO_BANK_T == priv->system)
	{
		nim_reg_read(dev, DEMO_BANK_T, DMD_TMCCD2_T, data, 2);
		mod = (data[0] >> 3) & 0x3;
		cr = data[1] & 0x07;

		if(mod >= 3 || cr >= 5)
			return 0;
		cnr_rel = cnr - DMD_DVBT_CNR_P1[mod][cr];
	}
	else if (DEMO_BANK_T2 == priv->system)
	{
		data[0] = 0;
		data[1] = 0x3;
		nim_reg_write(dev, DEMO_BANK_T2, DMD_TPDSET1, data, 2);
		nim_reg_read(dev, DEMO_BANK_T2, DMD_TPD2, data, 1);
		mod = data[0]>>5;
		cr = ((data[0]>>2)&0x7);

		if( mod >= 4 || cr >= 6 )
			return 0;
		cnr_rel = cnr - DMD_DVBT2_CNR_P1[mod][cr];
	}
	else
	{
		//jhy add start
		if(beri)
			*snr = 100;
		//jhy add end
		return 0;
	}

	if( cnr_rel < -700 )
	{
		sqi = 0;
	}
	else
	{
		if( beri < 1000 )		//BER>10-3
		{
			ber_sqi = 0;
		}
		else if( beri < 10000000 )	//BER>10-7
		{
			ber_sqi = 20 * log10_easy(beri) - 40000;
			ber_sqi /= 1000;
		}
		else
		{
			ber_sqi = 100;
		}

		if( cnr_rel <= 300 )
			sqi = (((cnr_rel - 300) /10) + 100) * ber_sqi;
		else
			sqi = ber_sqi * 100;
	}

	*snr = (UINT8)(sqi / 100);

	NIM_PANIC6158_PRINTF("snr =%d\n", *snr);

	//soc_printf("%d\n", osal_get_tick() - tick);

	return SUCCESS;
}

static INT32 nim_panic6158_get_PER(struct nim_device *dev, UINT32 *per)
{
	UINT8 data[4];
	UINT32 err, sum;
	struct nim_panic6158_private *priv;

	priv = (struct nim_panic6158_private *)dev->priv;

	nim_reg_read(dev, DEMO_BANK_T, DMD_PERRDU, data, 2);
	err = data[0]*0x100 + data[1];
	nim_reg_read(dev, DEMO_BANK_T, DMD_PERLENRDU, data, 2);
	sum = data[0]*0x100 + data[1];

	if (0 != sum)
		*per = err*100 / sum;
	else
		*per = 0;

	NIM_PANIC6158_PRINTF("per = %d\n", *per);

	return SUCCESS;
}

static INT32 nim_panic6158_get_BER(struct nim_device *dev, UINT32 *ber)
{
	//INT32 result;
	UINT32 error, len, sum;

	nim_calculate_ber(dev, &error, &len, &sum);

	if (0 != sum)
	{
		*ber = (error*100)/len;
	}
	else
	{
		*ber = 0;
	}
	//NIM_PANIC6158_PRINTF("ber = %d\n", *ber);
	return SUCCESS;


}
static INT32 nim_panic6158_get_AGC(struct nim_device *dev, UINT8 *agc)
{
	//INT32 result;
	//UINT32 flgptn;
	U32  m_agc = 0;
	UINT32 ber, per;
	struct nim_panic6158_private *priv;

	priv = (struct nim_panic6158_private *)dev->priv;
    printk("[test]tuner id:%d   %d\n",priv->tuner_id,priv->system);
	MxL_Check_RF_Input_Power(priv->tuner_id, &m_agc);
	*agc = (UINT8)m_agc;
	NIM_PANIC6158_PRINTF("agc = %d\n", *agc);


	nim_panic6158_get_PER(dev, &per);
	nim_panic6158_get_BER(dev, &ber);
	return SUCCESS;
}

static INT32 nim_panic6158_get_freq(struct nim_device *dev, UINT32 *freq)
{
	struct nim_panic6158_private *priv;

	priv = (struct nim_panic6158_private *)dev->priv;
	*freq = priv->frq;
	NIM_PANIC6158_PRINTF("freq = %d KHz \n", *freq);

	return SUCCESS;
}

static INT32 nim_panic6158_get_system(struct nim_device *dev, UINT8 *system)
{
	struct nim_panic6158_private *priv;

	priv = (struct nim_panic6158_private *)dev->priv;

	if (DEMO_BANK_T == priv->system)
		*system = DEMO_DVBT;
	else if (DEMO_BANK_T2 == priv->system)
		*system = DEMO_DVBT2;
	else if (DEMO_BANK_C == priv->system)
		*system = DEMO_DVBC;
	else
		*system = 0;

	NIM_PANIC6158_PRINTF("Frontend type %d\n", *system);
	return SUCCESS;
}

static INT32 nim_panic6158_get_modulation(struct nim_device *dev, UINT8 *modulation)
{
	UINT8 data[2];
	UINT8 mode = 0;
	struct nim_panic6158_private *priv;

	priv = (struct nim_panic6158_private *)dev->priv;

	if (DEMO_BANK_T == priv->system)
	{
		nim_reg_read(dev, DEMO_BANK_T, DMD_TMCCD2_T, data, 1);
		mode = (data[0] >> 3) & 0x3;

		if(mode >= 3)
			return 0;
	}
	else if (DEMO_BANK_T2 == priv->system)
	{
		data[0] = 0;
		data[1] = 0x3;
		nim_reg_write(dev, DEMO_BANK_T2, DMD_TPDSET1, data, 2);
		nim_reg_read(dev, DEMO_BANK_T2, DMD_TPD2, data, 1);
		mode = data[0]>>5;
	}
	*modulation = mode;

	NIM_PANIC6158_PRINTF("modulation = %d KHz \n", *modulation);
	return SUCCESS;
}

static INT32 nim_panic6158_get_code_rate(struct nim_device *dev, UINT8 *fec)
{
	UINT8 data[2];
	UINT8 code_rate = 0;

	struct nim_panic6158_private *priv;

	priv = (struct nim_panic6158_private *)dev->priv;
	if (DEMO_BANK_T == priv->system)
	{
		nim_reg_read(dev, DEMO_BANK_T, DMD_TMCCD3_T, data, 1);
		code_rate = data[0] & 0x07;

		if(code_rate >= 5)
			return 0;
	}
	else if (DEMO_BANK_T2 == priv->system)
	{
		data[0] = 0;
		data[1] = 0x3;
		nim_reg_write(dev, DEMO_BANK_T2, DMD_TPDSET1, data, 2);
		nim_reg_read(dev, DEMO_BANK_T2, DMD_TPD2, data, 1);
		code_rate = ((data[0]>>2)&0x7);
	}

	*fec = code_rate;

	return SUCCESS;
}

static INT32 nim_panic6158_get_bandwidth(struct nim_device *dev, INT32 *bandwidth)
{
	struct nim_panic6158_private *priv;

	priv = (struct nim_panic6158_private *)dev->priv;
	*bandwidth = priv->bw;

	NIM_PANIC6158_PRINTF("bandwidth=%d KHz \n", *bandwidth);
	return SUCCESS;
}

static INT32 nim_panic6158_tune_action(struct nim_device *dev, UINT8 system, UINT32 frq, UINT8 bw, UINT8 qam, UINT8 plp_id, UINT32 wait_time)
{
	INT32 ret = !SUCCESS;
	INT32 retTuner = !SUCCESS;
	UINT16 i, j;
	UINT8 data, lock = 0;
	struct nim_panic6158_private *priv;
	UINT8 qam_array[] = {QAM64, QAM256, QAM128, QAM32, QAM16};

	priv = (struct nim_panic6158_private *) dev->priv;

	if (DEMO_BANK_T == system || DEMO_BANK_C == system)
	{
		NIM_PANIC6158_PRINTF("tune T\n");
        NIM_PANIC6158_PRINTF("[%s]%d,T frq =%d, bw= %d,system =%d\n",__FUNCTION__,__LINE__,frq, bw, system);
		if (DEMO_BANK_C == system)
		{
			for (i = 0; i < ARRAY_SIZE(qam_array); i++)
			{
				if (qam_array[i] == qam)
					break;
			}

			if (i >= ARRAY_SIZE(qam_array))
				i = 0;

			nim_reg_mask_write(dev, system, DMD_CHSRCHSET_C, 0x0F, 0x08 | i);

		}
		else if (DEMO_BANK_T == system)
		{
			nim_panic6158_set_info(dev, system, DMD_E_INFO_DVBT_MODE, DMD_E_DVBT_MODE_NOT_DEFINED);
			nim_panic6158_set_info(dev, system, DMD_E_INFO_DVBT_GI, DMD_E_DVBT_GI_NOT_DEFINED);
		}

		if (1 == priv->scan_stop_flag)
			return SUCCESS;

		//tune tuner
		tun_mxl301_set_addr(priv->tuner_id, priv->i2c_addr[DEMO_BANK_T2], priv->i2c_mutex_id);
		if (NULL != priv->tc.nim_Tuner_Control)
			retTuner = priv->tc.nim_Tuner_Control(priv->tuner_id, frq, bw, 0, NULL, 0);

		//osal_task_sleep(10);
		YWOS_TaskSleep(10);
		if (NULL != priv->tc.nim_Tuner_Status)
			priv->tc.nim_Tuner_Status(priv->tuner_id, &lock);

		if (1 == priv->scan_stop_flag)
			return SUCCESS;



		//get tuner lock state
		if (1 != lock)
		{
			NIM_PANIC6158_PRINTF("tuner lock failed \n");
			return !SUCCESS;
		}

		//osal_task_sleep(100);
		YWOS_TaskSleep(100);

		//reset demo
		data = 0x9F;
		nim_reg_write(dev, DEMO_BANK_T2, DMD_RSTSET1, &data, 1);
		NIM_PANIC6158_PRINTF("dev->get_lock = 0x%x\n", (int)dev->get_lock);
		for (i = 0; i < (wait_time / 50); i++)
		{
			if (NULL != dev->get_lock)
			{
				if (1 == priv->scan_stop_flag)
					break;

				//osal_task_sleep(50);
				YWOS_TaskSleep(50);

				dev->get_lock(dev, &lock);
				NIM_PANIC6158_PRINTF("[%s]%d,T/C demod lock =%d\n",__FUNCTION__,__LINE__,lock);
				if (1 == lock)
				{
					ret = SUCCESS;
					break;
				}
			}
		}
	}
	else if (DEMO_BANK_T2 == system)
	{
		data = 0x43;
		nim_reg_write(dev, DEMO_BANK_T2, DMD_SSEQSET, &data, 1);

		NIM_PANIC6158_PRINTF("tune T2\n");
        NIM_PANIC6158_PRINTF("[%s]%d,T2 frq =%d, bw= %d,system =%d,plp = %d\n",__FUNCTION__,__LINE__,frq, bw, system, plp_id);
		for (i = 0; i < PANIC6158_PLP_TUNE_NUM; i++)
		{
			data = 0x80;
			nim_reg_write(dev, DEMO_BANK_T2, DMD_FECSET1, &data, 1);
			data = plp_id;
			nim_reg_write(dev, DEMO_BANK_T2, DMD_PLPID, &data, 1);

			if (1 == priv->scan_stop_flag)
				break;

			//tune tuner
			tun_mxl301_set_addr(priv->tuner_id, priv->i2c_addr[DEMO_BANK_T2], priv->i2c_mutex_id);
			if (NULL != priv->tc.nim_Tuner_Control)
				priv->tc.nim_Tuner_Control(priv->tuner_id, frq, bw, 0, NULL, 0);

			//osal_task_sleep(10);
			YWOS_TaskSleep(10);
			if (NULL != priv->tc.nim_Tuner_Status)
				priv->tc.nim_Tuner_Status(priv->tuner_id, &lock);
				//printk("[%s]%d,T/C tuner lock =%d\n",__FUNCTION__,__LINE__,lock);

			//get tuner lock state
			if (1 != lock)
			{
				NIM_PANIC6158_PRINTF("tuner lock failed \n");
				return !SUCCESS;
			}

			if (1 == priv->scan_stop_flag)
				break;

			//osal_task_sleep(100);
			YWOS_TaskSleep(100);

			//reset demo
			data = 0x9F;
			nim_reg_write(dev, DEMO_BANK_T2, DMD_RSTSET1, &data, 1);

			//receive judgement
			for (j = 0; j < (PANIC6158_T2_TUNE_TIMEOUT / 50); j++)
			{
				if (1 == priv->scan_stop_flag)
					break;

				//osal_task_sleep(50);
				YWOS_TaskSleep(50);
				nim_reg_read(dev, priv->system, DMD_SSEQFLG, &data, 1);

				//if bit6 or bit5 is 1, then not need to wait
				if ((0x60 & data) || (1 == priv->scan_stop_flag))
				{
					break;
				}
				if (13 <= data)
				{
					data = 0x0;
					nim_reg_write(dev, DEMO_BANK_T2, DMD_TPDSET1, &data, 1);
					data = 0x7;
					nim_reg_write(dev, DEMO_BANK_T2, DMD_TPDSET2, &data, 1);

					nim_reg_read(dev, DEMO_BANK_T2, 0xCA, &data, 1);
					NIM_PANIC6158_PRINTF("%s T2 lock, PLP: %d! \n", __FUNCTION__, data);
					return SUCCESS;
				}
			}


			if (1 == priv->scan_stop_flag)
				break;
		}
	}

	NIM_PANIC6158_PRINTF("ret = %d\n", ret);
	return ret;
}

INT32 nim_panic6158_open(struct nim_device *dev);

static INT32 nim_panic6158_channel_change(struct nim_device *dev, struct NIM_Channel_Change *param)
{
	UINT32 frq;
	INT32 i;//, j;
	INT32 tune_num = 1;
	UINT32 wait_time;
	UINT8 bw, mode[2], qam, plp_id;
	struct nim_panic6158_private *priv;
	printk("          nim_panic6158_channel_change\n");

	priv = (struct nim_panic6158_private *) dev->priv;

	frq = param->freq;//Hz
	bw = param->bandwidth;//MHz
	qam = param->modulation;//for DVBC
	plp_id = param->plp_id;//for DVB-T2

	//param->priv_param;//DEMO_UNKNOWN/DEMO_DVBC/DEMO_DVBT/DEMO_DVBT2
	if (DEMO_DVBC == param->priv_param)
	{
		mode[0] = DEMO_BANK_C;
		NIM_PANIC6158_PRINTF("frq:%dKHz, bw:%dMHz, system:%d, qam=%d\n", frq / 1000, bw, mode[0],qam);
	}
	else
	{
		if (DEMO_DVBT == param->priv_param)
		{
			mode[0] = DEMO_BANK_T;
			NIM_PANIC6158_PRINTF("frq:%dKHz, bw:%dMHz, system:%d\n", frq / 1000, bw, mode[0]);
		}
		else if (DEMO_DVBT2 == param->priv_param)
		{
			mode[0] = DEMO_BANK_T2;
			NIM_PANIC6158_PRINTF("frq:%dKHz, bw:%dMHz, system:%d, plp id:%d\n", frq / 1000, bw, mode[0], plp_id);
		}
		else
		{
			tune_num = PANIC6158_TUNE_MAX_NUM;
			mode[0] = (1 == priv->first_tune_t2) ? DEMO_BANK_T2 : DEMO_BANK_T;
			mode[1] = (DEMO_BANK_T2 == mode[0]) ? DEMO_BANK_T : DEMO_BANK_T2;
			NIM_PANIC6158_PRINTF("frq:%dKHz, bw:%dMHz, system:%d+%d, plp id:%d\n", frq / 1000, bw, mode[0], mode[1], plp_id);
		}
	}

	priv->scan_stop_flag = 0;

	for (i = 0; i < tune_num; i++)
	{

		priv->frq = frq;
		priv->bw = bw;
		priv->system = mode[i];

		//set IF freq
		if (7 == bw && (DEMO_BANK_T == mode[i] || DEMO_BANK_T2 == mode[i]))
			priv->if_freq = DMD_E_IF_4500KHZ;
		else
			priv->if_freq = DMD_E_IF_5000KHZ;
		printk("[%s]%d\n",__FUNCTION__,__LINE__);
		//nim_panic6158_open(dev);
		nim_panic6158_set_reg(dev, MN88472_REG_COMMON);
		//config demo
		nim_panic6158_set_system(dev, mode[i], bw, priv->if_freq);
		//nim_panic6158_set_error_flag(dev, 1);
		//nim_panic6158_set_ts_output(dev, DMD_E_TSOUT_SERIAL_VARIABLE_CLOCK);

		if (1 == priv->scan_stop_flag)
			break;

		//tune demo
		wait_time = (PANIC6158_TUNE_MAX_NUM == tune_num && 0 == i) ? 1000 : 300;//ms
		if (SUCCESS == nim_panic6158_tune_action(dev, mode[i], frq, bw, qam, plp_id, wait_time))
			break;
	}

	return SUCCESS;
}

static INT32 nim_panic6158_ioctl(struct nim_device *dev, INT32 cmd, UINT32 param)
{
	INT32 ret = SUCCESS;
	struct nim_panic6158_private *priv;

	priv = (struct nim_panic6158_private *)dev->priv;

	switch(cmd)
	{
		case NIM_DRIVER_STOP_CHANSCAN:
		case NIM_DRIVER_STOP_ATUOSCAN:
	            	priv->scan_stop_flag = 1;
	            	break;

		default:
			break;
	}
	return ret;
}

INT32 nim_panic6158_open(struct nim_device *dev)
{
	INT32 i;
	INT32 ret;
	UINT8 data;

	/* Demodulator LSI Initialize */
	ret = nim_panic6158_set_reg(dev, MN88472_REG_COMMON);
	if (SUCCESS != ret)
	{
			printk("nim_panic6158_open Error\n");
		NIM_PANIC6158_PRINTF( "ERROR: Demodulator LSI Initialize" );
		return ret;
	}


	/* Auto Control Sequence Transfer */
	data = 0x20;
	ret  = nim_reg_write(dev, DEMO_BANK_T, DMD_PSEQCTRL, &data, 1);
	data = 0x03;
#if 1
	ret |= nim_reg_write(dev, DEMO_BANK_T, DMD_PSEQSET, &data, 1);

	for (i = 0; i < MN88472_REG_AUTOCTRL_SIZE; i++)
		ret |= nim_reg_write(dev, DEMO_BANK_T, DMD_PSEQPRG, &MN88472_REG_AUTOCTRL[i], 1);

	data = 0x0;
	ret |= nim_reg_write(dev, DEMO_BANK_T, DMD_PSEQSET, &data, 1);

	/* Check Parity bit */
	ret |= nim_reg_read(dev, DEMO_BANK_T, DMD_PSEQFLG , &data, 1);
	if(data & 0x10)
	{
		NIM_PANIC6158_PRINTF( "ERROR: PSEQ Parity" );
		return !SUCCESS;
	}

	ret = nim_panic6158_set_error_flag(dev, 0);
	//printf("[%s]ret=0x%x................\n",__FUNCTION__,ret);
	nim_panic6158_set_ts_output(dev,DMD_E_TSOUT_PARALLEL_VARIABLE_CLOCK /**//*DMD_E_TSOUT_PARALLEL_FIXED_CLOCK*//*DMD_E_TSOUT_SERIAL_VARIABLE_CLOCK*/);
#endif
	return ret;
}

static INT32 nim_panic6158_close(struct nim_device *dev)
{
	//UINT8 i;
	INT32 ret = SUCCESS;

	/*if (OSAL_INVALID_ID != dev->mutex_id)
	{
		if (RET_SUCCESS != osal_mutex_delete(dev->mutex_id))
		{
			NIM_PANIC6158_PRINTF("nim_panic6158_close: Delete mutex failed!\n");
			return !SUCCESS;
		}
		dev->mutex_id = OSAL_INVALID_ID;
	}

	if (OSAL_INVALID_ID != dev->flags)
	{
		if (RET_SUCCESS != osal_flag_delete(dev->flags))
		{
			NIM_PANIC6158_PRINTF("nim_panic6158_close: Delete flag failed!\n");
			return !SUCCESS;
		}
		dev->flags = OSAL_INVALID_ID;
	}*/
	//FREE(dev->priv);
	//dev_free(dev);
	YWOS_Free(dev->priv);
	YWOS_Free(dev);
	dev = NULL;

	return ret;

}

INT32 nim_panic6158_attach(UINT8 Handle, PCOFDM_TUNER_CONFIG_API pConfig,TUNER_OpenParams_T *OpenParams)
{
	INT32 ret = 0;
	UINT8 i = 0,data= 0;
	struct nim_device *dev;
	struct nim_panic6158_private *priv;
	TUNER_ScanTaskParam_T       *Inst = NULL;
	TUNER_IOARCH_OpenParams_t  Tuner_IOParams;


	if(NULL == pConfig || 2 < nim_panic6158_cnt)
		return ERR_NO_DEV;

	Inst = TUNER_GetScanInfo(Handle);

	//dev = (struct nim_device *)dev_alloc(nim_panic6158_name[nim_panic6158_cnt], HLD_DEV_TYPE_NIM, sizeof(struct nim_device));
	dev =(struct nim_device *)YWOS_Malloc(sizeof(struct nim_device));
	if(NULL == dev)
		return ERR_NO_MEM;

	priv = (PNIM_PANIC6158_PRIVATE )YWOS_Malloc(sizeof(struct nim_panic6158_private));
	if(NULL == priv)
	{
		//dev_free(dev);
		YWOS_Free(dev);
		//NIM_PANIC6158_PRINTF("Alloc nim device prive memory error!/n");
		return ERR_NO_MEM;
	}

	YWLIB_Memset(dev, 0, sizeof(struct nim_device));
	YWLIB_Memset(priv, 0, sizeof(struct nim_panic6158_private));
	YWLIB_Memcpy((void*)&(priv->tc), (void*)pConfig, sizeof(struct COFDM_TUNER_CONFIG_API));

	//priv->tuner_if_freq = pConfig->tuner_config.wTuner_IF_Freq;
	dev->priv = (void*)priv;

	//jhy add start
	if(YWTUNER_DELIVER_TER == Inst->Device)
    {

	    dev->DemodIOHandle[0] = Inst->DriverParam.Ter.DemodIOHandle;
	    dev->DemodIOHandle[1] = Inst->DriverParam.Ter.TunerIOHandle;
	}
	else if(YWTUNER_DELIVER_CAB == Inst->Device)
	{
	    dev->DemodIOHandle[0] = Inst->DriverParam.Cab.DemodIOHandle;
	    dev->DemodIOHandle[1] = Inst->DriverParam.Cab.TunerIOHandle;
	}
	//printf("dev->DemodIOHandle[0] = %d\n", dev->DemodIOHandle[0]);
	//printf("dev->DemodIOHandle[1] = %d\n", dev->DemodIOHandle[1]);
	Tuner_IOParams.IORoute = TUNER_IO_DIRECT;
	Tuner_IOParams.IODriver = OpenParams->TunerIO.Driver;
	YWLIB_Strcpy((S8 *)Tuner_IOParams.DriverName, (S8 *)OpenParams->TunerIO.DriverName);
	Tuner_IOParams.Address = PANIC6158_C_ADDR;//OpenParams->TunerIO.Address;
	Tuner_IOParams.hInstance = NULL;//&Inst->DriverParam.Ter.DemodIOHandle;
	Tuner_IOParams.TargetFunction = NULL;//Inst->DriverParam.Ter.DemodDriver.Demod_repeat;
	Tuner_IOParams.I2CIndex = OpenParams->I2CBusNo;
	TUNER_IOARCH_Open(&dev->DemodIOHandle[2], &Tuner_IOParams);
	//printf("dev->DemodIOHandle[2] = %d\n", dev->DemodIOHandle[2]);
	//printf("[%s]%d,DemodIOHandle[1]=0x%x,DemodIOHandle[2]=0x%x\n",__FUNCTION__,__LINE__,dev->DemodIOHandle[1],dev->DemodIOHandle[2]);
	//jhy add end

	dev->i2c_type_id = pConfig->ext_dm_config.i2c_type_id;
	dev->nim_idx = pConfig->ext_dm_config.nim_idx;//distinguish left or right
	dev->init = NULL;
	dev->open = nim_panic6158_open;
	dev->stop = nim_panic6158_close;
	dev->disable = NULL;    //nim_m3101_disable;
	dev->do_ioctl = nim_panic6158_ioctl;//nim_m3101_ioctl;
	dev->channel_change = nim_panic6158_channel_change;
	dev->channel_search = NULL;//nim_m3101_channel_search;
	dev->get_lock = nim_panic6158_get_lock;
	dev->get_freq = nim_panic6158_get_freq;//nim_m3101_get_freq;
	dev->get_FEC =nim_panic6158_get_code_rate;////nim_m3101_get_code_rate;
	dev->get_AGC = nim_panic6158_get_AGC;
	dev->get_SNR = nim_panic6158_get_SNR;
	dev->get_BER = nim_panic6158_get_BER;
	dev->get_PER = nim_panic6158_get_PER;
	dev->get_bandwidth = nim_panic6158_get_bandwidth;//nim_m3101_get_bandwith;
	dev->get_frontend_type = nim_panic6158_get_system;;

	//added for DVB-T additional elements
	dev->get_guard_interval = NULL;//nim_m3101_get_GI;
	dev->get_fftmode = NULL;//nim_m3101_get_fftmode;
	dev->get_modulation = nim_panic6158_get_modulation;//nim_m3101_get_modulation;
	dev->get_spectrum_inv = NULL;//nim_m3101_get_specinv;

	dev->get_freq_offset = NULL;//nim_m3101_get_freq_offset;

	priv->i2c_addr[0] = PANIC6158_T_ADDR;
	priv->i2c_addr[1] = PANIC6158_T2_ADDR;
	priv->i2c_addr[2] = PANIC6158_C_ADDR;
	priv->if_freq = DMD_E_IF_5000KHZ;
	priv->flag_id = OSAL_INVALID_ID;
	priv->i2c_mutex_id = OSAL_INVALID_ID;
	priv->first_tune_t2 = pConfig->tune_t2_first;
	priv->tuner_id = Handle;  //jhy added
	//priv->system = DEMO_BANK_T2;//jhy added ,DEMO_BANK_T or DEMO_BANK_T2

	ret = SUCCESS;
	for (i = 0; i < 3; i++)
	{
		ret = nim_reg_read(dev, DEMO_BANK_T, DMD_CHIPRD, &data, 1);
		if (2 == data)
			break;
	}
	//printf("[%s]%d,i=%d,ret=%d \n",__FUNCTION__,__LINE__,i,ret);

	if (3 <= i || 0 < ret)
	{
		YWOS_Free(priv);
		YWOS_Free(dev);
		return !SUCCESS;
	}

	if (NULL != pConfig->nim_Tuner_Init)
	{
		if (SUCCESS == pConfig->nim_Tuner_Init(&priv->tuner_id, &pConfig->tuner_config))
		{
			//priv_mem->tuner_type = tuner_type;
			priv->tc.nim_Tuner_Init = pConfig->nim_Tuner_Init;
			priv->tc.nim_Tuner_Control = pConfig->nim_Tuner_Control;
			priv->tc.nim_Tuner_Status  = pConfig->nim_Tuner_Status;
			YWLIB_Memcpy(&priv->tc.tuner_config,&pConfig->tuner_config, sizeof(struct COFDM_TUNER_CONFIG_EXT));
		}
		else
		{
			NIM_PANIC6158_PRINTF("Error: Init Tuner Failure!\n");
			//FREE(priv);
			//dev_free(dev);
			YWOS_Free(priv);
			YWOS_Free(dev);
			return ERR_NO_DEV;
		}
	}
    Inst->userdata = (void *)dev;

	nim_panic6158_cnt++;

	/* Open this device */
	ret = nim_panic6158_open(dev);
	//printf("[%s]%d,open result=%d \n",__FUNCTION__,__LINE__,ret);

	/* Setup init work mode */
	if (ret != SUCCESS)
	{
		YWOS_Free(priv);
		YWOS_Free(dev);
		return ERR_NO_DEV;
	}

	return SUCCESS;
}


int d6158_read_snr(struct dvb_frontend* fe, u16* snr)
{
	int 	iRet;
	struct dvb_d6158_fe_ofdm_state *state = fe->demodulator_priv;
    iRet = nim_panic6158_get_SNR(&(state->spark_nimdev),(UINT8*)snr);
	*snr = *snr * 255 * 255 /100;
	return iRet;
}

int d6158_read_ber(struct dvb_frontend* fe, UINT32 *ber)
{
	struct dvb_d6158_fe_ofdm_state *state = fe->demodulator_priv;

	return nim_panic6158_get_BER(&state->spark_nimdev, ber);

}

int d6158_read_signal_strength(struct dvb_frontend* fe, u16 *strength)
{
	int 	iRet;
	u32 	Strength;
	u32 	*Intensity = &Strength;
	struct dvb_d6158_fe_ofdm_state *state = fe->demodulator_priv;
    iRet = nim_panic6158_get_AGC(&state->spark_nimdev, (UINT8*)Intensity);
	if(*Intensity>90)
        *Intensity = 90;
    if (*Intensity < 10)
        *Intensity = *Intensity * 11/2;
    else
        *Intensity = *Intensity / 2 + 50;
    if(*Intensity>90)
        *Intensity = 90;

	*Intensity = *Intensity * 255 * 255 /100;
	printk("*Intensity = %d\n", *Intensity);
	*strength = (*Intensity);
	YWOS_TaskSleep(100);

	return iRet;
}

int d6158_read_status(struct dvb_frontend *fe, enum fe_status *status)
{
	int 	j,iRet;
	struct dvb_d6158_fe_ofdm_state* state = fe->demodulator_priv;
	UINT8 bIsLocked;
	//printk("%s>>\n", __FUNCTION__);

	for (j = 0; j < (PANIC6158_T2_TUNE_TIMEOUT / 50); j++)
	{
		YWOS_TaskSleep(50);
		iRet= nim_panic6158_get_lock(&state->spark_nimdev,&bIsLocked);

		if (bIsLocked)
		{
			break;
		}

	}
	printk("bIsLocked = %d\n", bIsLocked);

	if (bIsLocked)
	{
		*status = FE_HAS_SIGNAL
				| FE_HAS_CARRIER
				| FE_HAS_VITERBI
				| FE_HAS_SYNC
				| FE_HAS_LOCK;
	}
	else
	{
		*status = 0;
	}

	return 0;

}



int d6158_read_ucblocks(struct dvb_frontend* fe,
														u32* ucblocks)
{
	*ucblocks = 0;
	return 0;
}

int dvb_d6158_get_property(struct dvb_frontend *fe, struct dtv_property* tvp)
{
	//struct dvb_d0367_fe_ofdm_state* state = fe->demodulator_priv;

	/* get delivery system info */
	if(tvp->cmd==DTV_DELIVERY_SYSTEM){
		switch (tvp->u.data) {
		case SYS_DVBT2:
		case SYS_DVBT:
		case SYS_DVBC_ANNEX_AC:
			break;
		default:
			return -EINVAL;
		}
	}
	return 0;
}


int dvb_d6158_fe_qam_get_property(struct dvb_frontend *fe, struct dtv_property* tvp)
{
	//struct dvb_d0367_fe_ofdm_state* state = fe->demodulator_priv;

	/* get delivery system info */
	if(tvp->cmd==DTV_DELIVERY_SYSTEM){
		switch (tvp->u.data) {
		case SYS_DVBC_ANNEX_AC:
			break;
		default:
			return -EINVAL;
		}
	}
	return 0;
}




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
	ret = i2c_transfer(i2c_adap, msg, 2);
	if (ret == 2)
	{
    	if (pucActualID != 0x2)
        {
           // YWOSTRACE(( YWOS_TRACE_INFO, "[%s] YWHAL_ERROR_UNKNOWN_DEVICE!\n",__FUNCTION__) );
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
YW_ErrorType_T  demod_d6158_Repeat(IOARCH_Handle_t				DemodIOHandle, /*demod io ??±ú*/
									IOARCH_Handle_t				TunerIOHandle, /*?°?? io ??±ú*/
									TUNER_IOARCH_Operation_t Operation,
									unsigned short SubAddr,
									unsigned char *Data,
									unsigned int TransferSize,
									unsigned int Timeout)
{
    return 0;
}




static YW_ErrorType_T demod_d6158_ScanFreq(struct dvb_frontend_parameters *p,
	                                            struct nim_device *dev,UINT8 System, UINT8 plp_id)
{
   // struct nim_device           *dev;
    INT32 ret = 0;
    struct NIM_Channel_Change param;

	memset(&param, 0, sizeof(struct NIM_Channel_Change));

	if(dev == NULL)
	{

		return YWHAL_ERROR_BAD_PARAMETER;
	}

	if(SYS_DVBC_ANNEX_AC == System)
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
	else if(SYS_DVBC_ANNEX_B == System)
	{
		param.priv_param = DEMO_DVBC;
		param.bandwidth = MxL_BW_6MHz;
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
	else if(SYS_DVBT2 == System)
	{
		//printf("TuneMode=%d\n", Inst->DriverParam.Ter.Param.TuneMode);
		//param.priv_param = DEMO_UNKNOWN;//T2  T
		param.priv_param = DEMO_DVBT2;//T2

		//printk("p->frequency:%dKHz, bw:%dMHz\n",
		//		p->frequency, p->u.ofdm.bandwidth);
		param.freq = p->frequency;
		param.plp_id = plp_id;
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
	else if(SYS_DVBT == System)
	{
		//printf("TuneMode=%d\n", Inst->DriverParam.Ter.Param.TuneMode);
		param.priv_param = DEMO_UNKNOWN;//T2  T

		//printk("p->frequency:%dKHz, bw:%dMHz\n",
		//		p->frequency, p->u.ofdm.bandwidth);
		param.freq = p->frequency;
		param.plp_id = plp_id;
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

	//printf("[%s]%d,dev=0x%x\n",__FUNCTION__,__LINE__,dev);

    ret = nim_panic6158_channel_change(dev,&param);


    return ret;

}
YW_ErrorType_T demod_d6158_Reset(U8 Index)
{
	YW_ErrorType_T Error = YW_NO_ERROR;
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

	/* Read noise estimations for C/N and BER */
	nim_panic6158_get_BER(dev, Ber);

    nim_panic6158_get_AGC(dev, (UINT8*)Intensity);  //level
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


YW_ErrorType_T demod_d6158_Open(U8 Handle,TUNER_OpenParams_T * OpenParams)
{
	TUNER_ScanTaskParam_T       *Inst = NULL;
    struct COFDM_TUNER_CONFIG_API	Tuner_API;
	YW_ErrorType_T YW_ErrorCode = 0;
	INT32 ret =0;

	//printf("[%s]%d,IN ok \n",__FUNCTION__,__LINE__);

	Inst = TUNER_GetScanInfo(Handle);

    YWLIB_Memset(&Tuner_API, 0, sizeof(struct COFDM_TUNER_CONFIG_API));

	Tuner_API.nim_Tuner_Init = tun_mxl301_init;
	Tuner_API.nim_Tuner_Status = tun_mxl301_status;
	Tuner_API.nim_Tuner_Control = tun_mxl301_control;
	Tuner_API.tune_t2_first = 0;//when demod_d6158_ScanFreq,param.priv_param >DEMO_DVBT2,tuner tune T2 then t
	Tuner_API.tuner_config.demo_type = PANASONIC_DEMODULATOR;
	Tuner_API.tuner_config.cTuner_Base_Addr = 0xC2;


	ret = nim_panic6158_attach(Handle, &Tuner_API,OpenParams);


	/*------------------驱动函数指针----------------*/
	if(YWTUNER_DELIVER_TER == Inst->Device)
	{
		Inst->DriverParam.Ter.DemodDriver.Demod_GetSignalInfo = demod_d6158_GetSignalInfo;
		Inst->DriverParam.Ter.DemodDriver.Demod_IsLocked = demod_d6158_IsLocked;
		Inst->DriverParam.Ter.DemodDriver.Demod_repeat = demod_d6158_Repeat;
		Inst->DriverParam.Ter.DemodDriver.Demod_reset = demod_d6158_Reset;
		//Inst->DriverParam.Ter.DemodDriver.Demod_ScanFreq = demod_d6158_ScanFreq;
	}
	else if(YWTUNER_DELIVER_CAB == Inst->Device)
	{
		Inst->DriverParam.Cab.DemodDriver.Demod_GetSignalInfo = demod_d6158_GetSignalInfo;
		Inst->DriverParam.Cab.DemodDriver.Demod_IsLocked = demod_d6158_IsLocked;
		Inst->DriverParam.Cab.DemodDriver.Demod_repeat = demod_d6158_Repeat;
		Inst->DriverParam.Cab.DemodDriver.Demod_reset = demod_d6158_Reset;
		//Inst->DriverParam.Cab.DemodDriver.Demod_ScanFreq = demod_d6158_ScanFreq;
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

#define NO_STREAM_ID_FILTER	(~0U)

int d6158_set_frontend(struct dvb_frontend* fe,
										struct dvb_frontend_parameters *p)
{
	struct dtv_frontend_properties *props = &fe->dtv_property_cache;
	struct dvb_d6158_fe_ofdm_state* state = fe->demodulator_priv;
	struct nim_device *dev = &state->spark_nimdev;
	struct nim_panic6158_private *priv = dev->priv;
	UINT8 plp_id;

	plp_id = props->stream_id != NO_STREAM_ID_FILTER ? props->stream_id : 0;
	state->p = p;
	printk("-----------------------d6158_set_frontend\n");
	demod_d6158_ScanFreq(p,&state->spark_nimdev,props->delivery_system,plp_id);
	state->p = NULL;

	return 0;
}


#define BURST_SZ 6
static UINT32 mxl301_tuner_cnt = 0;
static struct COFDM_TUNER_CONFIG_EXT *  mxl301_cfg[YWTUNERi_MAX_TUNER_NUM] = {NULL};



#if 0
INT32 tun_mxl301_mask_write(UINT32 tuner_id, UINT8 addr, UINT8 reg, UINT8 mask , UINT8 data)
{
	INT32 ret;
	UINT8 value[2];
	TUNER_ScanTaskParam_T 		*Inst = NULL;
	IOARCH_Handle_t				IOHandle;


    Inst = TUNER_GetScanInfo(tuner_id);
	IOHandle = Inst->DriverParam.Ter.TunerIOHandle;

	value[0] = reg;
	//ret = i2c_write(i2c_id, addr, value, 1);
	//ret = i2c_read(i2c_id, addr, value, 1);
	//ret = I2C_ReadWrite(&IOARCH_Handle[IOHandle].ExtDeviceHandle, TUNER_IO_WRITE,addr, value, 1,  100);
	ret = I2C_ReadWrite(&IOARCH_Handle[IOHandle].ExtDeviceHandle, TUNER_IO_READ,addr, value, 1,  100);

	value[0]|= mask & data;
	value[0] &= (mask^0xff) | data;
	value[1] = value[0];
	value[0] = reg;
	//ret |= i2c_write(i2c_id, addr, value, 2);
	ret |= I2C_ReadWrite(&IOARCH_Handle[IOHandle].ExtDeviceHandle, TUNER_IO_WRITE,addr, value, 2,  100);

	return ret;
}
#else
INT32 tun_mxl301_mask_write(UINT32 tuner_id, UINT8 reg, UINT8 mask , UINT8 data)
{
	INT32 ret;
	UINT8 value[2];
	TUNER_ScanTaskParam_T 		*Inst = NULL;
	IOARCH_Handle_t				IOHandle = 0;

    Inst = TUNER_GetScanInfo(tuner_id);

	if(Inst->Device == YWTUNER_DELIVER_TER)
		IOHandle = Inst->DriverParam.Ter.TunerIOHandle;
	else if(Inst->Device == YWTUNER_DELIVER_CAB)
		IOHandle = Inst->DriverParam.Cab.TunerIOHandle;

	//value[0] = reg;
	//ret = i2c_write(i2c_id, addr, value, 1);
	//ret = i2c_read(i2c_id, addr, value, 1);

	ret = I2C_ReadWrite(mxl301_cfg[tuner_id]->i2c_adap, TUNER_IO_SA_READ,reg, value, 1,  100);

	value[0]|= mask & data;
	value[0] &= (mask^0xff) | data;
	//value[1] = value[0];
	//value[0] = reg;
	//ret |= i2c_write(i2c_id, addr, value, 2);
	ret |= I2C_ReadWrite(mxl301_cfg[tuner_id]->i2c_adap, TUNER_IO_SA_WRITE,reg, value, 1,  100);

	return ret;
}

#endif
#if 0
INT32 tun_mxl301_i2c_write(UINT32 tuner_id, UINT8* pArray, UINT32 count)
{
	INT32 result = SUCCESS;
	INT32 i/*, j*/, cycle;
	UINT8 tmp[BURST_SZ+2];
	struct COFDM_TUNER_CONFIG_EXT * mxl301_ptr = NULL;
	TUNER_ScanTaskParam_T 		*Inst = NULL;
	IOARCH_Handle_t				IOHandle;

	//if(tuner_id >= mxl301_tuner_cnt)
	//	return ERR_FAILUE;

    Inst = TUNER_GetScanInfo(tuner_id);
	IOHandle = Inst->DriverParam.Ter.TunerIOHandle;

	cycle = count / BURST_SZ;
	mxl301_ptr = mxl301_cfg[tuner_id];

	//osal_mutex_lock(mxl301_ptr->i2c_mutex_id, OSAL_WAIT_FOREVER_TIME);
	if (PANASONIC_DEMODULATOR == mxl301_ptr->demo_type)
	{
		//tun_mxl301_mask_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, DMD_TCBSET, 0x7f, 0x53);
		tun_mxl301_mask_write(tuner_id, mxl301_ptr->demo_addr, DMD_TCBSET, 0x7f, 0x53);

		tmp[0] = DMD_TCBADR;
		tmp[1] = 0;
		//result |= i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, tmp, 2);
		result |= I2C_ReadWrite(&IOARCH_Handle[IOHandle].ExtDeviceHandle, TUNER_IO_WRITE, mxl301_ptr->demo_addr, tmp, 2,  100);

		for (i = 0; i < cycle; i++)
		{
			YWLIB_Memcpy(&tmp[2], &pArray[BURST_SZ*i], BURST_SZ);
			tmp[0] = DMD_TCBCOM;
			tmp[1] = mxl301_ptr->cTuner_Base_Addr;
			//result += i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, tmp, BURST_SZ+2);
			result = I2C_ReadWrite(&IOARCH_Handle[IOHandle].ExtDeviceHandle, TUNER_IO_WRITE, mxl301_ptr->demo_addr, tmp,BURST_SZ+2,  100);

			//for (j = 0; j < (BURST_SZ/2); j++)
			///	NIM_PRINTF("reg[0x%2x], value[0x%02x]\n", tmp[2+2*j], tmp[2+2*j+1]);
		}

		if (BURST_SZ*i < count)
		{
			YWLIB_Memcpy(&tmp[2], &pArray[BURST_SZ*i], count - BURST_SZ*i);
			tmp[0] = DMD_TCBCOM;
			tmp[1] = mxl301_ptr->cTuner_Base_Addr;
			//result += i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, tmp, count-BURST_SZ*i+2);
			result += I2C_ReadWrite(&IOARCH_Handle[IOHandle].ExtDeviceHandle, TUNER_IO_WRITE, mxl301_ptr->demo_addr, tmp,count-BURST_SZ*i+2,100);

			//for (j = 0; j < ((count-BURST_SZ*i)/2); j++)
			//	NIM_PRINTF("reg[0x%2x], value[0x%02x]\n", tmp[2+2*j], tmp[2+2*j+1]);
		}
	}
	else
	{
		for (i = 0; i < cycle; i++)
		{
			YWLIB_Memcpy(&tmp[2], &pArray[BURST_SZ*i], BURST_SZ);
			tmp[0] = 0x09;
			tmp[1] = mxl301_ptr->cTuner_Base_Addr;
			//result += i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, tmp, BURST_SZ+2);
			result += I2C_ReadWrite(&IOARCH_Handle[IOHandle].ExtDeviceHandle, TUNER_IO_WRITE, mxl301_ptr->demo_addr, tmp,BURST_SZ+2,100);

			//for (j = 0; j < (BURST_SZ/2); j++)
			//	NIM_PRINTF("reg[0x%2x], value[0x%02x]\n", tmp[2+2*j], tmp[2+2*j+1]);
		}

		if (BURST_SZ*i < count)
		{
			YWLIB_Memcpy(&tmp[2], &pArray[BURST_SZ*i], count - BURST_SZ*i);
			tmp[0] = 0x09;
			tmp[1] = mxl301_ptr->cTuner_Base_Addr;
			//result += i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, tmp, count-BURST_SZ*i+2);
			result += I2C_ReadWrite(&IOARCH_Handle[IOHandle].ExtDeviceHandle, TUNER_IO_WRITE, mxl301_ptr->demo_addr, tmp,count-BURST_SZ*i+2,100);
			//for (j = 0; j < ((count-BURST_SZ*i)/2); j++)
			//	NIM_PRINTF("reg[0x%2x], value[0x%02x]\n", tmp[2+2*j], tmp[2+2*j+1]);
		}
	}
	//osal_mutex_unlock(mxl301_ptr->i2c_mutex_id);

	//if (SUCCESS != result)
	//	NIM_PRINTF("%s write i2c failed\n");

	return result;
}
#else
INT32 tun_mxl301_i2c_write(UINT32 tuner_id, UINT8* pArray, UINT32 count)
{
	INT32 result = SUCCESS;
	INT32 i/*, j*/, cycle;
	UINT8 tmp[BURST_SZ+2];
	struct COFDM_TUNER_CONFIG_EXT * mxl301_ptr = NULL;

	//if(tuner_id >= mxl301_tuner_cnt)
	//	return ERR_FAILUE;


	cycle = count / BURST_SZ;
	mxl301_ptr = mxl301_cfg[tuner_id];
	//osal_mutex_lock(mxl301_ptr->i2c_mutex_id, OSAL_WAIT_FOREVER_TIME);
	if (PANASONIC_DEMODULATOR == mxl301_ptr->demo_type)
	{

		//tun_mxl301_mask_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, DMD_TCBSET, 0x7f, 0x53);
		tun_mxl301_mask_write(tuner_id, DMD_TCBSET, 0x7f, 0x53);
		//tmp[0] = DMD_TCBADR;
		//tmp[1] = 0;
		//result |= i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, tmp, 2);
		tmp[0] = 0;
		result |= I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_SA_WRITE,DMD_TCBADR, tmp, 1,  100);
		for (i = 0; i < cycle; i++)
		{
			//YWLIB_Memcpy(&tmp[2], &pArray[BURST_SZ*i], BURST_SZ);
			//tmp[0] = DMD_TCBCOM;
			//tmp[1] = mxl301_ptr->cTuner_Base_Addr;
			//result += i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, tmp, BURST_SZ+2);
			YWLIB_Memcpy(&tmp[1], &pArray[BURST_SZ*i], BURST_SZ);
			tmp[0] = mxl301_ptr->cTuner_Base_Addr;
			result = I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_SA_WRITE,DMD_TCBCOM , tmp,BURST_SZ+1,  100);

			//for (j = 0; j < (BURST_SZ/2); j++)
			///	NIM_PRINTF("reg[0x%2x], value[0x%02x]\n", tmp[2+2*j], tmp[2+2*j+1]);
		}
		if (BURST_SZ*i < count)
		{
			//YWLIB_Memcpy(&tmp[2], &pArray[BURST_SZ*i], count - BURST_SZ*i);
			//tmp[0] = DMD_TCBCOM;
			//tmp[1] = mxl301_ptr->cTuner_Base_Addr;
			//result += i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, tmp, count-BURST_SZ*i+2);
			YWLIB_Memcpy(&tmp[1], &pArray[BURST_SZ*i], count - BURST_SZ*i);
			tmp[0] = mxl301_ptr->cTuner_Base_Addr;
			result += I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_SA_WRITE,DMD_TCBCOM, tmp,count-BURST_SZ*i+1,100);

			//for (j = 0; j < ((count-BURST_SZ*i)/2); j++)
			//	NIM_PRINTF("reg[0x%2x], value[0x%02x]\n", tmp[2+2*j], tmp[2+2*j+1]);
		}
	}
	else
	{
		for (i = 0; i < cycle; i++)
		{
			//YWLIB_Memcpy(&tmp[2], &pArray[BURST_SZ*i], BURST_SZ);
			//tmp[0] = 0x09;
			//tmp[1] = mxl301_ptr->cTuner_Base_Addr;
			//result += i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, tmp, BURST_SZ+2);
			YWLIB_Memcpy(&tmp[1], &pArray[BURST_SZ*i], BURST_SZ+1);
			tmp[0] = mxl301_ptr->cTuner_Base_Addr;
			printk("loop %p\n",mxl301_ptr->i2c_adap);
			result += I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_SA_WRITE,0x09,tmp,BURST_SZ,100);

			//for (j = 0; j < (BURST_SZ/2); j++)
			//	NIM_PRINTF("reg[0x%2x], value[0x%02x]\n", tmp[2+2*j], tmp[2+2*j+1]);
		}

		if (BURST_SZ*i < count)
		{
			//YWLIB_Memcpy(&tmp[2], &pArray[BURST_SZ*i], count - BURST_SZ*i);
			//tmp[0] = 0x09;
			//tmp[1] = mxl301_ptr->cTuner_Base_Addr;
			//result += i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, tmp, count-BURST_SZ*i+2);
			YWLIB_Memcpy(&tmp[1], &pArray[BURST_SZ*i], count - BURST_SZ*i);
			tmp[0] = mxl301_ptr->cTuner_Base_Addr;
			result += I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_SA_WRITE,0x09,tmp,count-BURST_SZ*i+1,100);


			//for (j = 0; j < ((count-BURST_SZ*i)/2); j++)
			//	NIM_PRINTF("reg[0x%2x], value[0x%02x]\n", tmp[2+2*j], tmp[2+2*j+1]);
		}
	}
	//osal_mutex_unlock(mxl301_ptr->i2c_mutex_id);

	//if (SUCCESS != result)
	//	NIM_PRINTF("%s write i2c failed\n");

	return result;
}

#endif
#if 0
INT32 tun_mxl301_i2c_read(UINT32 tuner_id, UINT8 Addr, UINT8* mData)
{
	INT32 ret = 0;
	UINT8 cmd[4];
	struct COFDM_TUNER_CONFIG_EXT * mxl301_ptr = NULL;
	TUNER_ScanTaskParam_T 		*Inst = NULL;
	IOARCH_Handle_t				IOHandle;

	//if(tuner_id >= mxl301_tuner_cnt)
	//	return ERR_FAILUE;

    Inst = TUNER_GetScanInfo(tuner_id);
	IOHandle = Inst->DriverParam.Ter.TunerIOHandle;

	mxl301_ptr = mxl301_cfg[tuner_id];

	//osal_mutex_lock(mxl301_ptr->i2c_mutex_id, OSAL_WAIT_FOREVER_TIME);
	if (PANASONIC_DEMODULATOR == mxl301_ptr->demo_type)
	{
		//tun_mxl301_mask_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, DMD_TCBSET, 0x7f, 0x53);
		tun_mxl301_mask_write(tuner_id, mxl301_ptr->demo_addr, DMD_TCBSET, 0x7f, 0x53);
		cmd[0] = DMD_TCBADR;
		cmd[1] = 0;
		//ret |= i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, cmd, 2);

		cmd[0] = DMD_TCBCOM;
		cmd[1] = mxl301_ptr->cTuner_Base_Addr;
		cmd[2] = 0xFB;
		cmd[3] = Addr;
		//ret |= i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, cmd, 4);

		//tun_mxl301_mask_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, DMD_TCBSET, 0x7f, 0x53);
		//cmd[0] = DMD_TCBADR;
		//cmd[1] = 0;
		//ret |= i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, cmd, 2);

		cmd[0] = DMD_TCBCOM;
		cmd[1] = mxl301_ptr->cTuner_Base_Addr | 0x1;
		//ret |= i2c_write_read(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, cmd, 2, 1);

		*mData = cmd[0];
	}
	else
	{
		cmd[0] = 0x09;
		cmd[1] = mxl301_ptr->cTuner_Base_Addr;
		cmd[2] = 0xFB;
		cmd[3] = Addr;
		//ret = i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, cmd, 4);

		cmd[1] |= 0x1;
		//ret = i2c_write_read(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, cmd, 2, 1);

		*mData = cmd[0];
	}
	//osal_mutex_unlock(mxl301_ptr->i2c_mutex_id);

	//if (SUCCESS != ret)
	//	NIM_PRINTF("%s read i2c failed\n");

	return ret;
}
#else
INT32 tun_mxl301_i2c_read(UINT32 tuner_id, UINT8 Addr, UINT8* mData)
{
	INT32 ret = 0;
	UINT8 cmd[4];
	struct COFDM_TUNER_CONFIG_EXT * mxl301_ptr = NULL;

	//if(tuner_id >= mxl301_tuner_cnt)
	//	return ERR_FAILUE

	mxl301_ptr = mxl301_cfg[tuner_id];

	//osal_mutex_lock(mxl301_ptr->i2c_mutex_id, OSAL_WAIT_FOREVER_TIME);
	if (PANASONIC_DEMODULATOR == mxl301_ptr->demo_type)
	{
		tun_mxl301_mask_write(tuner_id, DMD_TCBSET, 0x7f, 0x53);

		cmd[0] = 0;
		ret |= I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_SA_WRITE,DMD_TCBADR, cmd,1,100);

		cmd[0] = mxl301_ptr->cTuner_Base_Addr;
		cmd[1] = 0xFB;
		cmd[2] = Addr;
		ret |= I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_SA_WRITE,DMD_TCBCOM, cmd,3,100);

		cmd[0] = mxl301_ptr->cTuner_Base_Addr | 0x1;
		ret |= I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_SA_WRITE, DMD_TCBCOM, cmd,1,100);
		ret |= I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_READ,DMD_TCBCOM, cmd,1,100);
		//printk("[%s]%d,cTuner_Base_Addr=0x%x,Addr=0x%x,ret=%d,cmd[0]=0x%x \n",__FUNCTION__,__LINE__,mxl301_ptr->cTuner_Base_Addr,Addr,ret,cmd[0]);

		*mData = cmd[0];
	}
	else
	{
		//cmd[0] = 0x09;
		//cmd[1] = mxl301_ptr->cTuner_Base_Addr;
		//cmd[2] = 0xFB;
		//cmd[3] = Addr;
		//ret = i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, cmd, 4);
		cmd[0] = mxl301_ptr->cTuner_Base_Addr;
		cmd[1] = 0xFB;
		cmd[2] = Addr;
		ret = I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_SA_WRITE,0x09, cmd,3,100);

		cmd[0] |= 0x1;
		//ret = i2c_write_read(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, cmd, 2, 1);
		ret= I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_SA_WRITE, 0x09, cmd,1,100);
		ret= I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_READ, 0x09, cmd,1,100);

		*mData = cmd[0];
	}
	//osal_mutex_unlock(mxl301_ptr->i2c_mutex_id);

	//if (SUCCESS != ret)
	//	NIM_PRINTF("%s read i2c failed\n");

	return ret;
}

#endif

INT32 MxL_Set_Register(UINT32 tuner_idx, UINT8 RegAddr, UINT8 RegData)
{
	INT32 ret = 0;
	UINT8 pArray[2];
	pArray[0] = RegAddr;
	pArray[1] = RegData;

	ret = tun_mxl301_i2c_write(tuner_idx, pArray, 2);

	return ret;
}


INT32 MxL_Get_Register(UINT32 tuner_idx, UINT8 RegAddr, UINT8 *RegData)
{
	INT32 ret = 0;

	ret = tun_mxl301_i2c_read(tuner_idx, RegAddr, RegData);

	return ret;
}

INT32 MxL_Soft_Reset(UINT32 tuner_idx)
{
	UINT8 reg_reset = 0xFF;
	INT32 ret = 0;

	ret = tun_mxl301_i2c_write(tuner_idx, &reg_reset, 1);

	return ret;
}

INT32 MxL_Check_RF_Input_Power(UINT32 tuner_idx, U32* RF_Input_Level)
{
	UINT8 RFin1, RFin2, RFOff1, RFOff2;
	//float RFin, RFoff;
	//float cal_factor;
	U32  RFin, RFoff;
	U32 cal_factor;
	MxLxxxRF_TunerConfigS *myTuner;

	//if(tuner_idx >= mxl301_tuner_cnt)
	//	return ERR_FAILUE;

	myTuner = mxl301_cfg[tuner_idx]->priv;

	printk("[test]tuner id:%d\n",tuner_idx);


	if (MxL_Set_Register(tuner_idx, 0x14, 0x01))
		return MxL_ERR_SET_REG;

	MxL_Delay(1);
	if(MxL_Get_Register(tuner_idx, 0x18, &RFin1))  /* LSBs */
		return MxL_ERR_SET_REG;
	if(MxL_Get_Register(tuner_idx, 0x19, &RFin2))  /* MSBs */
		return MxL_ERR_SET_REG;

	if(MxL_Get_Register(tuner_idx, 0xD6, &RFOff1))  /* LSBs */
		return MxL_ERR_SET_REG;
	if(MxL_Get_Register(tuner_idx, 0xD7, &RFOff2))  /* MSBs */
		return MxL_ERR_SET_REG;

	//RFin = (float)(((RFin2 & 0x07) << 5) + ((RFin1 & 0xF8) >> 3) + ((RFin1 & 0x07) * 0.125));
	//RFoff = (float)(((RFOff2 & 0x0F) << 2) + ((RFOff1 & 0xC0) >> 6) + (((RFOff1 & 0x38)>>3) * 0.125));
	RFin = (((RFin2 & 0x07) << 5) + ((RFin1 & 0xF8) >> 3) + ((RFin1 & 0x07)<<3));
	RFoff = (((RFOff2 & 0x0F) << 2) + ((RFOff1 & 0xC0) >> 6) + (((RFOff1 & 0x38)>>3)<<3));
	if(myTuner->Mode == MxL_MODE_DVBT)
		cal_factor = 113;
	else if(myTuner->Mode == MxL_MODE_ATSC)
		cal_factor = 109;
	else if(myTuner->Mode == MxL_MODE_CAB_STD)
		cal_factor = 110;
	else
		cal_factor = 107;

	*RF_Input_Level = (RFin - RFoff - cal_factor);

	return MxL_OK;
}


/*****************************************************************************
* INT32 tun_mxl301_init(struct COFDM_TUNER_CONFIG_EXT * ptrTuner_Config)
*
* Tuner mxl301 Initialization
*
* Arguments:
*  Parameter1: struct COFDM_TUNER_CONFIG_EXT * ptrTuner_Config		: pointer for Tuner configuration structure
*
* Return Value: INT32			: Result
*****************************************************************************/
INT32 tun_mxl301_init(UINT32 *tuner_idx, struct COFDM_TUNER_CONFIG_EXT * ptrTuner_Config)
{
	INT32 result;
	UINT8 data = 1;
	UINT8 pArray[MAX_ARRAY_SIZE];	/* a array pointer that store the addr and data pairs for I2C write */
	UINT32 Array_Size = 0;	/* a integer pointer that store the number of element in above array */
	struct COFDM_TUNER_CONFIG_EXT *mxl301_ptr = NULL;
	MxLxxxRF_TunerConfigS *priv_ptr;


	/* check Tuner Configuration structure is available or not */
	if ((ptrTuner_Config == NULL) || mxl301_tuner_cnt >= YWTUNERi_MAX_TUNER_NUM)
		return ERR_FAILUE;

	mxl301_ptr = (struct COFDM_TUNER_CONFIG_EXT *)YWOS_Malloc(sizeof(struct COFDM_TUNER_CONFIG_EXT));
	if(NULL == mxl301_ptr)
		return ERR_FAILUE;

	YWLIB_Memcpy(mxl301_ptr, ptrTuner_Config, sizeof(struct COFDM_TUNER_CONFIG_EXT));

	mxl301_ptr->priv = (MxLxxxRF_TunerConfigS*)YWOS_Malloc(sizeof(MxLxxxRF_TunerConfigS));
	if (NULL == mxl301_ptr->priv)
	{
		YWOS_Free(mxl301_ptr);
		return ERR_FAILUE;
	}

	//mxl301_cfg[mxl301_tuner_cnt] = mxl301_ptr;
	//*tuner_idx = mxl301_tuner_cnt;
	mxl301_cfg[*tuner_idx] = mxl301_ptr; //jhy modified
	mxl301_tuner_cnt++;

	YWLIB_Memset(mxl301_ptr->priv, 0, sizeof(MxLxxxRF_TunerConfigS));
	priv_ptr = (MxLxxxRF_TunerConfigS *)mxl301_ptr->priv;

	priv_ptr->I2C_Addr = MxL_I2C_ADDR_97;
	priv_ptr->TunerID = MxL_TunerID_MxL301RF;
	priv_ptr->Mode = MxL_MODE_DVBT;
	priv_ptr->Xtal_Freq = MxL_XTAL_24_MHZ;
	priv_ptr->IF_Freq = MxL_IF_5_MHZ;
	priv_ptr->IF_Spectrum = MxL_NORMAL_IF;
	priv_ptr->ClkOut_Amp = MxL_CLKOUT_AMP_0;
	priv_ptr->Xtal_Cap = MxL_XTAL_CAP_8_PF;
	priv_ptr->AGC = MxL_AGC_SEL1;
	priv_ptr->IF_Path = MxL_IF_PATH1;
	priv_ptr->idac_setting = MxL_IDAC_SETTING_OFF;

	/* '11/10/06 : OKAMOTO	Control AGC set point. */
	priv_ptr->AGC_set_point = 0x93;

	/* '11/10/06 : OKAMOTO	Select AGC external or internal. */
	priv_ptr->bInternalAgcEnable = TRUE;

	/*Soft reset tuner */
	MxL_Soft_Reset(*tuner_idx);
	//printf("[%s]%d \n",__FUNCTION__,__LINE__);

	result = tun_mxl301_i2c_read(*tuner_idx, 0x17, &data);
	//printk("[%s]%d,result=%d,data=0x%x \n",__FUNCTION__,__LINE__,result,data);
	//NIM_PRINTF("result = %x, 0x%x %s!\n", result, data, __FUNCTION__);
	if (SUCCESS != result || 0x09 != (UINT8)(data&0x0F))
	{
		//YWOS_Free(mxl301_ptr->priv);
		//YWOS_Free(mxl301_ptr);
		return !SUCCESS;
	}

	/*perform initialization calculation */
	MxL301RF_Init(pArray, &Array_Size, (UINT8)priv_ptr->Mode, (UINT32)priv_ptr->Xtal_Freq,
			(UINT32)priv_ptr->IF_Freq, (UINT8)priv_ptr->IF_Spectrum, (UINT8)priv_ptr->ClkOut_Setting, (UINT8)priv_ptr->ClkOut_Amp,
			(UINT8)priv_ptr->Xtal_Cap, (UINT8)priv_ptr->AGC, (UINT8)priv_ptr->IF_Path,priv_ptr->bInternalAgcEnable);

	/* perform I2C write here */
	if(SUCCESS != tun_mxl301_i2c_write(*tuner_idx, pArray, Array_Size))
	{
		//libc_printf("MxL301RF_Init failed\n");
		//printf("[%s]%d,MxL301RF_Init failed! \n",__FUNCTION__,__LINE__);
		//YWOS_Free(mxl301_ptr->priv);
		//YWOS_Free(mxl301_ptr);
		return !SUCCESS;
	}

	//osal_task_sleep(1);	/* 1ms delay*/
	YWOS_TaskSleep(1);
	return SUCCESS;
}

INT32 tun_mxl301_set_addr(UINT32 tuner_idx, UINT8 addr, UINT32 i2c_mutex_id)
{
	struct COFDM_TUNER_CONFIG_EXT * mxl301_ptr = NULL;

	//if(tuner_idx >= mxl301_tuner_cnt)
	//	return ERR_FAILUE;

	mxl301_ptr = mxl301_cfg[tuner_idx];
	mxl301_ptr->demo_addr = addr;
	mxl301_ptr->i2c_mutex_id = i2c_mutex_id;

	return SUCCESS;
}

/*****************************************************************************
* INT32 tun_mxl301_status(UINT8 *lock)
*
* Tuner read operation
*
* Arguments:
*  Parameter1: UINT8 *lock		: Phase lock status
*
* Return Value: INT32			: Result
*****************************************************************************/

INT32 tun_mxl301_status(UINT32 tuner_idx, UINT8 *lock)
{
	INT32 result;
	UINT8 data;

	//if(tuner_idx >= mxl301_tuner_cnt)
	//	return ERR_FAILUE;

	result = MxL_Get_Register(tuner_idx, 0x16, &data);

	*lock = (0x03 == (data & 0x03));

	return result;
}

/*****************************************************************************
* INT32 nim_mxl301_control(UINT32 freq, UINT8 bandwidth,UINT8 AGC_Time_Const,UINT8 *data,UINT8 _i2c_cmd)
*
* Tuner write operation
*
* Arguments:
*  Parameter1: UINT32 freq		: Synthesiser programmable divider
*  Parameter2: UINT8 bandwidth		: channel bandwidth
*  Parameter3: UINT8 AGC_Time_Const	: AGC time constant
*  Parameter4: UINT8 *data		:
*
* Return Value: INT32			: Result
*****************************************************************************/
INT32 tun_mxl301_control(UINT32 tuner_idx, UINT32 freq, UINT8 bandwidth,UINT8 AGC_Time_Const,UINT8 *data,UINT8 _i2c_cmd)
{
	INT32 ret;
	INT16 Data;
	UINT8 i, Data1, Data2;
	MxLxxxRF_IF_Freq if_freq;
	MxLxxxRF_TunerConfigS *myTuner;
	struct COFDM_TUNER_CONFIG_EXT * mxl301_ptr = NULL;
	UINT8 pArray[MAX_ARRAY_SIZE];	/* a array pointer that store the addr and data pairs for I2C write */
	UINT32 Array_Size = 0;				/* a integer pointer that store the number of element in above array */

	//printf("[%s]%d,tuner_idx=%d\n",__FUNCTION__,__LINE__,tuner_idx);

	if(/*tuner_idx >= mxl301_tuner_cnt ||*/ tuner_idx>=YWTUNERi_MAX_TUNER_NUM)
		return ERR_FAILUE;

	mxl301_ptr = mxl301_cfg[tuner_idx];
	myTuner = mxl301_ptr->priv;

	//  do tuner init first.
	//printf("[%s]%d,frq =%d, bw= %d\n",__FUNCTION__,__LINE__,freq, bandwidth);

	if( bandwidth == MxL_BW_6MHz )
	{
		if_freq = MxL_IF_4_MHZ;
	}
	else if( bandwidth == MxL_BW_7MHz )
	{
		if_freq = MxL_IF_4_5_MHZ;
	}
	else// if( bandwidth == MxL_BW_8MHz )
	{
		if_freq = MxL_IF_5_MHZ;
	}

	if (if_freq != myTuner->IF_Freq)
	{
		/*Soft reset tuner */
		MxL_Soft_Reset(tuner_idx);

		/*perform initialization calculation */
		MxL301RF_Init(pArray, &Array_Size, (UINT8)myTuner->Mode, (UINT32)myTuner->Xtal_Freq,
				(UINT32)myTuner->IF_Freq, (UINT8)myTuner->IF_Spectrum, (UINT8)myTuner->ClkOut_Setting, (UINT8)myTuner->ClkOut_Amp,
				(UINT8)myTuner->Xtal_Cap, (UINT8)myTuner->AGC, (UINT8)myTuner->IF_Path,myTuner->bInternalAgcEnable);

		/* perform I2C write here */
		if(SUCCESS != tun_mxl301_i2c_write(tuner_idx, pArray, Array_Size))
		{
			//NIM_PRINTF("MxL301RF_Init failed\n");
		}

		//osal_task_sleep(10);	/* 1ms delay*/
		YWOS_TaskSleep(10);
		myTuner->IF_Freq = if_freq;
	}

	//  then do tuner tune
	myTuner->RF_Freq_Hz = freq;  //modify by yanbinL
	myTuner->BW_MHz = bandwidth;

	/* perform Channel Change calculation */

	ret = MxL301RF_RFTune(pArray, &Array_Size, myTuner->RF_Freq_Hz, myTuner->BW_MHz, myTuner->Mode);
	if (SUCCESS != ret)
		return MxL_ERR_RFTUNE;

	/* perform I2C write here */
	if (SUCCESS != tun_mxl301_i2c_write(tuner_idx, pArray, Array_Size))
		return !SUCCESS;

	MxL_Delay(1); /* Added V9.2.1.0 */

	/* Register read-back based setting for Analog M/N split mode only */
	if (myTuner->TunerID == MxL_TunerID_MxL302RF && myTuner->Mode == MxL_MODE_ANA_MN && myTuner->IF_Split == MxL_IF_SPLIT_ENABLE)
	{
		MxL_Get_Register(tuner_idx, 0xE3, &Data1);
		MxL_Get_Register(tuner_idx, 0xE4, &Data2);
		Data = ((Data2&0x03)<<8) + Data1;
		if(Data >= 512)
			Data = Data - 1024;

		if(Data < 20)
		{
			MxL_Set_Register(tuner_idx, 0x85, 0x43);
			MxL_Set_Register(tuner_idx, 0x86, 0x08);
		}
		else if (Data >= 20)
		{
			MxL_Set_Register(tuner_idx, 0x85, 0x9E);
			MxL_Set_Register(tuner_idx, 0x86, 0x0F);
		}

		for(i = 0; i<Array_Size; i+=2)
		{
			if(pArray[i] == 0x11)
				Data1 = pArray[i+1];
			if(pArray[i] == 0x12)
				Data2 = pArray[i+1];
		}
		MxL_Set_Register(tuner_idx, 0x11, Data1);
		MxL_Set_Register(tuner_idx, 0x12, Data2);
	}

	if (myTuner->TunerID == MxL_TunerID_MxL302RF)
		MxL_Set_Register(tuner_idx, 0x13, 0x01);

	if (myTuner->TunerID == MxL_TunerID_MxL302RF && myTuner->Mode >= MxL_MODE_ANA_MN && myTuner->IF_Split == MxL_IF_SPLIT_ENABLE)
	{
		if(MxL_Set_Register(tuner_idx, 0x00, 0x01))
			return MxL_ERR_RFTUNE;
	}
	MxL_Delay(30);

	if((myTuner->Mode == MxL_MODE_DVBT) || (myTuner->Mode >= MxL_MODE_ANA_MN))
	{
		if(MxL_Set_Register(tuner_idx, 0x1A, 0x0D))
			return MxL_ERR_SET_REG;
	}
	if (myTuner->TunerID == MxL_TunerID_MxL302RF && myTuner->Mode >= MxL_MODE_ANA_MN && myTuner->IF_Split == MxL_IF_SPLIT_ENABLE)
	{
		if(MxL_Set_Register(tuner_idx, 0x00, 0x00))
			return MxL_ERR_RFTUNE;
	}

	/* '11/03/16 : OKAMOTO	Select IDAC setting in "MxL_Tuner_RFTune". */
	switch(myTuner->idac_setting)
	{
		case MxL_IDAC_SETTING_AUTO:
		{
			UINT8 Array[] =
			{
				0x0D, 0x00,
				0x0C, 0x67,
				0x6F, 0x89,
				0x70, 0x0C,
				0x6F, 0x8A,
				0x70, 0x0E,
				0x6F, 0x8B,
				0x70, 0x10,
			};

			if(myTuner->idac_hysterisis<0 || myTuner->idac_hysterisis>=MxL_IDAC_HYSTERISIS_MAX)
			{
				ret =  MxL_ERR_OTHERS;
				break;
			}
			else
			{
				UINT8 ui8_idac_hysterisis;
				ui8_idac_hysterisis = (UINT8)myTuner->idac_hysterisis;
				Array[15] = Array[15]+ui8_idac_hysterisis;
			}
			ret = tun_mxl301_i2c_write(tuner_idx, Array, sizeof(Array));
		}
			break;

		case MxL_IDAC_SETTING_MANUAL:
			if(myTuner->dig_idac_code>=63 || myTuner->dig_idac_code<0)
			{
				ret =  MxL_ERR_OTHERS;
				break;
			}
			else
			{
				UINT8 Array[] = {0x0D, 0x0};
				Array[1] = 0xc0 + myTuner->dig_idac_code;	//DIG_ENIDAC_BYP(0x0D[7])=1, DIG_ENIDAC(0x0D[6])=1
				ret = tun_mxl301_i2c_write(tuner_idx, Array, sizeof(Array));
			}
			break;

		case MxL_IDAC_SETTING_OFF:
			ret = MXL301_register_write_bit_name(tuner_idx, MxL_BIT_NAME_DIG_ENIDAC, 0);//0x0D[6]	0
			break;

		default:
			ret = MxL_ERR_OTHERS;
			break;
	}

	return ret;
}


struct MXL301_state {
	struct dvb_frontend		*fe;
	struct i2c_adapter		*i2c;
	const struct MXL301_config	*config;

	u32 frequency;
	u32 bandwidth;
};


static struct dvb_tuner_ops mxl301_ops =
{
	.set_params	= NULL,
	.release = NULL,
};


struct dvb_frontend *mxl301_attach(struct dvb_frontend *fe,
				    const struct MXL301_config *config,
				    struct i2c_adapter *i2c)
{
	struct MXL301_state *state = NULL;
	struct dvb_tuner_info *info;

	state = kzalloc(sizeof(struct MXL301_state), GFP_KERNEL);
	if (state == NULL)
		goto exit;

	state->config		= config;
	state->i2c		= i2c;
	state->fe		= fe;
	fe->tuner_priv		= state;
	fe->ops.tuner_ops	=  mxl301_ops;
	info			 = &fe->ops.tuner_ops.info;

	memcpy(info->name, config->name, sizeof(config->name));

	printk("%s: Attaching sharp6465 (%s) tuner\n", __func__, info->name);

	return fe;

exit:
	kfree(state);
	return NULL;
}


//add by yabin


 /***********************************************************************
	 函数名称:	 tuner_mxl301_Identify

	 函数说明:	 mxl301 的校验函数

 ************************************************************************/
 YW_ErrorType_T tuner_mxl301_Identify(IOARCH_Handle_t	*i2c_adap)
 {
	 YW_ErrorType_T ret = YW_NO_ERROR;
	 unsigned char	ucData = 0;
	 U8 value[2]={0};
	 U8 reg=0xEC;
	 U8 mask=0x7F;
	 U8 data=0x53;
	 U8 cmd[3];
	 U8 cTuner_Base_Addr = 0xC2;

	 //tuner soft  reset
	 cmd[0] = cTuner_Base_Addr;
	 cmd[1] = 0xFF;
	 ret |= I2C_ReadWrite(i2c_adap, TUNER_IO_SA_WRITE,0XF7, cmd,2,100);
	 YWOS_TaskSleep(20);
	 //reset end

	 ret |= I2C_ReadWrite(i2c_adap, TUNER_IO_SA_READ,reg, value, 1,  100);

	 value[0]|= mask & data;
	 value[0] &= (mask^0xff) | data;
	 ret |= I2C_ReadWrite(i2c_adap, TUNER_IO_SA_WRITE,reg, value, 1,  100);

	 cmd[0] = 0;
	 ret |= I2C_ReadWrite(i2c_adap, TUNER_IO_SA_WRITE,0XEE, cmd,1,100);

	 cmd[0] = cTuner_Base_Addr;
	 cmd[1] = 0xFB;
	 cmd[2] = 0x17;
	 ret |= I2C_ReadWrite(i2c_adap, TUNER_IO_SA_WRITE,0XF7, cmd,3,100);



	 cmd[0] = cTuner_Base_Addr| 0x1;
	 ret |= I2C_ReadWrite(i2c_adap, TUNER_IO_SA_WRITE, 0XF7, cmd,1,100);
	 ret |= I2C_ReadWrite(i2c_adap, TUNER_IO_READ,0XF7, cmd,1,100);
	 ucData = cmd[0];
	 if (YW_NO_ERROR != ret || 0x09 != (U8)(ucData&0x0F))
	 {
		 return YWHAL_ERROR_UNKNOWN_DEVICE;
	 }

	 return ret;
 }


 static struct dvb_frontend_ops dvb_d6158_fe_ofdm_ops = {

	 .info = {
		 .name			 = "Tuner3-T/T2/C",
		 .type			 = FE_OFDM,
		 .frequency_min 	 = 47000000,
		 .frequency_max 	 = 863250000,
		 .frequency_stepsize = 62500,
		 .caps = FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4 |
				 FE_CAN_FEC_4_5 | FE_CAN_FEC_5_6 | FE_CAN_FEC_6_7 |
				 FE_CAN_FEC_7_8 | FE_CAN_FEC_8_9 | FE_CAN_FEC_AUTO |
				 FE_CAN_QAM_16 | FE_CAN_QAM_64 | FE_CAN_QAM_AUTO |
				 FE_CAN_TRANSMISSION_MODE_AUTO |
				 FE_CAN_GUARD_INTERVAL_AUTO |
				 FE_CAN_HIERARCHY_AUTO |
				 FE_CAN_2G_MODULATION |
				 FE_CAN_MULTISTREAM
				 },


	 .init				 = NULL,
	 .release			 = NULL,
	 .sleep = NULL,
	 .set_frontend = d6158_set_frontend,
	 .get_frontend = NULL,

	 .read_ber			 = d6158_read_ber,
	 .read_snr			 = d6158_read_snr,
	 .read_signal_strength	 = d6158_read_signal_strength,
	 .read_status		 = d6158_read_status,

	 .read_ucblocks = d6158_read_ucblocks,
	 .i2c_gate_ctrl 	 =	NULL,
#if (DVB_API_VERSION < 5)
	 .get_info			 = NULL,
#else
	.get_property		 = dvb_d6158_get_property,
#endif

 };


static struct dvb_frontend_ops dvb_d6158_fe_qam_ops = {

	.info = {
		.name			= "Tuner3-T/T2/C",
		.type			= FE_QAM,
		.frequency_stepsize	= 62500,
		.frequency_min		= 51000000,
		.frequency_max		= 858000000,
		.symbol_rate_min	= (57840000/2)/64,     /* SACLK/64 == (XIN/2)/64 */
		.symbol_rate_max	= (57840000/2)/4,      /* SACLK/4 */
		.caps = FE_CAN_QAM_16 | FE_CAN_QAM_32 | FE_CAN_QAM_64 |
			FE_CAN_QAM_128 | FE_CAN_QAM_256 |
			FE_CAN_FEC_AUTO | FE_CAN_INVERSION_AUTO
	},

	.init				 = NULL,
	.release			 = NULL,
	.sleep = NULL,
	.set_frontend = d6158_set_frontend,
	.get_frontend = NULL,

	.read_ber			 = d6158_read_ber,
	.read_snr			 = d6158_read_snr,
	.read_signal_strength	 = d6158_read_signal_strength,
	.read_status		 = d6158_read_status,

	.read_ucblocks = d6158_read_ucblocks,
	.i2c_gate_ctrl 	 =	NULL,
#if (DVB_API_VERSION < 5)
	.get_info			 = NULL,
#else
	.get_property		 = dvb_d6158_fe_qam_get_property,
#endif

};


struct dvb_frontend* dvb_d6158_attach(struct i2c_adapter* i2c,UINT8 system)
 {
	 struct dvb_d6158_fe_ofdm_state* state = NULL;
	 struct nim_panic6158_private *priv;
	 int ret;
	 struct COFDM_TUNER_CONFIG_API  Tuner_API;

	 /* allocate memory for the internal state */
	 state = kzalloc(sizeof(struct dvb_d6158_fe_ofdm_state), GFP_KERNEL);
	 if (state == NULL) goto error;

	 priv = (PNIM_PANIC6158_PRIVATE)YWOS_Malloc(sizeof(struct nim_panic6158_private));
	 if(NULL == priv)
	 {
		 goto error;
	 }

	 /* create dvb_frontend */
	 if(system == DEMO_BANK_T2)   //dvb-t
	 {
		printk("DEMO_BANK_T2\n");
		memcpy(&state->frontend.ops, &dvb_d6158_fe_ofdm_ops, sizeof(struct dvb_frontend_ops));
	 }

	 else if (system == DEMO_BANK_C) //dvb-c
	 {
		printk("DEMO_BANK_C\n");
		memcpy(&state->frontend.ops, &dvb_d6158_fe_qam_ops, sizeof(struct dvb_frontend_ops));
	 }
	 state->frontend.demodulator_priv = state;
	 state->i2c = i2c;

	 state->DeviceMap.Timeout	= IOREG_DEFAULT_TIMEOUT;
	 state->DeviceMap.Registers = STV0367ofdm_NBREGS;
	 state->DeviceMap.Fields	= STV0367ofdm_NBFIELDS;
	 state->DeviceMap.Mode		= IOREG_MODE_SUBADR_16;
	 state->DeviceMap.RegExtClk = 27000000; //Demod External Crystal_HZ
	 state->DeviceMap.RegMap = (TUNER_IOREG_Register_t *)
					 kzalloc(state->DeviceMap.Registers * sizeof(TUNER_IOREG_Register_t),
							 GFP_KERNEL);
	 state->DeviceMap.priv = (void *)state;

	 state->spark_nimdev.priv = priv;
	 state->spark_nimdev.base_addr =  PANIC6158_T2_ADDR;

	 //state->spark_nimdev.i2c_type_id= pConfig->ext_dm_config.i2c_type_id;
	 //state->spark_nimdev.nim_idx = pConfig->ext_dm_config.nim_idx;//distinguish left or right

	 priv->i2c_adap = i2c;
	 priv->i2c_addr[0] = PANIC6158_T_ADDR;
	 priv->i2c_addr[1] = PANIC6158_T2_ADDR;
	 priv->i2c_addr[2] = PANIC6158_C_ADDR;
	 priv->if_freq = DMD_E_IF_5000KHZ;
	 priv->flag_id = OSAL_INVALID_ID;
	 priv->i2c_mutex_id = OSAL_INVALID_ID;
	 priv->system = system;   //T2 C
	 priv->tuner_id = 2;


	 if(tuner_mxl301_Identify((IOARCH_Handle_t*)i2c)!= YW_NO_ERROR)
	 {
		 printk("tuner_mxl301_Identify error!\n");
	 }
	 YWOS_TaskSleep(50);

	 YWLIB_Memset(&Tuner_API, 0, sizeof(struct COFDM_TUNER_CONFIG_API));

	 Tuner_API.nim_Tuner_Init = tun_mxl301_init;
	 Tuner_API.nim_Tuner_Status = tun_mxl301_status;
	 Tuner_API.nim_Tuner_Control = tun_mxl301_control;
	 Tuner_API.tune_t2_first = 1;//when demod_d6158_ScanFreq,param.priv_param >DEMO_DVBT2,tuner tune T2 then t
	 Tuner_API.tuner_config.demo_type = PANASONIC_DEMODULATOR;
	 Tuner_API.tuner_config.cTuner_Base_Addr = 0xC2;
	 Tuner_API.tuner_config.i2c_adap = (IOARCH_Handle_t*)i2c;



	 //printf("[%s]%d,i=%d,ret=%d \n",__FUNCTION__,__LINE__,i,ret);

	 if (NULL != Tuner_API.nim_Tuner_Init)
	 {
		 if (SUCCESS == Tuner_API.nim_Tuner_Init(&priv->tuner_id,&Tuner_API.tuner_config))
		 {
			 priv->tc.nim_Tuner_Init = Tuner_API.nim_Tuner_Init;
			 priv->tc.nim_Tuner_Control = Tuner_API.nim_Tuner_Control;
			 priv->tc.nim_Tuner_Status	= Tuner_API.nim_Tuner_Status;
			 YWLIB_Memcpy(&priv->tc.tuner_config,&Tuner_API.tuner_config, sizeof(struct COFDM_TUNER_CONFIG_EXT));
		 }

	 }

	state->spark_nimdev.get_lock = nim_panic6158_get_lock;

	 ret = nim_panic6158_open(&state->spark_nimdev);
	 printk("[%s]%d,open result=%d \n",__FUNCTION__,__LINE__,ret);

	 /* Setup init work mode */


	 return &state->frontend;

 error:
	 kfree(state);
	 return NULL;
 }


