/*$Source$*/
/*****************************文件头部注释*************************************/
//
//			Copyright (C), 2011-2016, AV Frontier Tech. Co., Ltd.
//
//
// 文 件 名：	$RCSfile$
//
// 创 建 者：	Administrator
//
// 创建时间：	2011.05.05
//
// 最后更新：	$Date$
//
//				$Author$
//
//				$Revision$
//
//				$State$
//
// 文件描述：	7162tuner驱动
//
/******************************************************************************/

/********************************  文件包含************************************/

#include <linux/bitops.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/dvb/version.h>

#include "dvbdev.h"
#include "dmxdev.h"
#include "dvb_frontend.h"


#include "spark7162.h"
#include "D3501_ext.h"
#include "D0367.h"

#include "../base/ix7306.h"
#include "../base/sharp6465.h"
#include "../base/sharp5469c.h"
#include "../base/sharp7803.h"

/********************************  常量定义************************************/

#define I2C_ADDR_IX7306		(0xc0 >> 1)
#define I2C_ADDR_SHARP7803  (0xc0 >> 1)
#define I2C_ADDR_SHARP6465	(0xc2 >> 1)
#define I2C_ADDR_SHARP5469C	(0xc2 >> 1)

enum
{
	UNION_TUNER_T,
	UNION_TUNER_C,
	UNION_TUNER_NUMS,
};

static int eUnionTunerType = UNION_TUNER_T;
static char *UnionTunerType = "t";

/*******************************  数据结构*********************************/

struct spark_tuner_config
{
	int adapter; 	/* DVB adapter number */
	int i2c_bus; 	/* i2c adapter number */
	int i2c_addr; 	/* i2c address */
};

struct spark_dvb_adapter_adddata {
	struct dvb_frontend *pD3501_frontend;
	struct dvb_frontend *pD3501_frontend_2;
	struct dvb_frontend *pD0367_frontend;
	struct dvb_frontend *pD0367_frontend_2;

	struct i2c_adapter	*qpsk_i2c_adap;
	struct i2c_adapter	*qpsk_i2c_adap_2;
	struct i2c_adapter	*ter_i2c_adap;
	struct i2c_adapter	*cab_i2c_adap;
};

/********************************  宏 定 义**********************************/

#define IS_UNION_TUNER_T(type) (UNION_TUNER_T == (type))
#define IS_UNION_TUNER_C(type) (UNION_TUNER_C == (type))

/********************************  变量定义********************************/

struct dvb_adapter 	spark_dvb_adapter;
struct device 		spark_device;

static const struct ix7306_config bs2s7hz7306a_config = {
	.name		= "Sharp BS2S7HZ7306A",
	.addr		= I2C_ADDR_IX7306,
	.step_size 	= IX7306_STEP_1000,
	.bb_lpf		= IX7306_LPF_12,
	.bb_gain	= IX7306_GAIN_2dB,
};

static const struct sharp7803_config bs2s7hz7803a_config = {
	.name		= "Sharp 7803",
	.addr		= I2C_ADDR_SHARP7803,
};

static const struct sharp6465_config s6465_config = {
	.name		= "Sharp 6465",
	.addr		= I2C_ADDR_SHARP6465,
	.bandwidth		= BANDWIDTH_8_MHZ,

	.Frequency	= 500000,
	.IF			= 36167,
	.TunerStep	= 16667,
};

static const struct sharp5469c_config s5469c_config = {
	.name		= "Sharp 5469c",
	.addr		= I2C_ADDR_SHARP5469C,
};

struct spark_tuner_config tuner_resources[] = {
        [0] = {
                .adapter = 0,
                .i2c_bus = 1,
        },
        [1] = {
                .adapter = 0,
                .i2c_bus = 3,
        },
        [2] = {
                .adapter = 0,
                .i2c_bus = 2,
        },
        [3] = {
                .adapter = 0,
                .i2c_bus = 2,
        },
};

/********************************  变量引用********************************/

/********************************  函数声明********************************/

/********************************  函数定义********************************/

int spark_dvb_attach_s(struct dvb_adapter *dvb_adap,
								const struct d3501_config *config,
								struct dvb_frontend **ppFrontend)
{
	struct dvb_frontend	*pFrontend = NULL;

	pFrontend = dvb_d3501_fe_qpsk_attach(config, config->pI2c);
	if(!pFrontend)
	{
		printk (KERN_INFO "%s: error attaching d3501\n", __FUNCTION__);
		return -1;
	}
	printk("%s: d3501 attached\n", __FUNCTION__);

	if(!dvb_attach(ix7306_attach, pFrontend, &bs2s7hz7306a_config, config->pI2c))
	{
		printk (KERN_INFO "%s: error attaching IX7306\n", __FUNCTION__);
		dvb_frontend_detach(pFrontend);
		return -1;
	}
	printk("%s: IX7306 attached\n", __FUNCTION__);

	(*ppFrontend) = pFrontend;
	return 0;
}

int spark_dvb_register_s(struct dvb_adapter *dvb_adap,
								int	tuner_resource,
								struct dvb_frontend **ppFrontend,
								struct i2c_adapter **ppI2c)
{
	struct dvb_frontend					*pFrontend;
	struct i2c_adapter					*pI2c;

	struct d3501_config	d3501config;

  	pI2c = i2c_get_adapter(tuner_resources[tuner_resource].i2c_bus);
	printk("pI2c = 0x%0x\n", (int)pI2c);
	if (!pI2c)
	{
	    return -1;
	}

	d3501config.pI2c = pI2c;
	d3501config.i = tuner_resource;
	snprintf (d3501config.name, sizeof(d3501config.name), "d3501-%d", tuner_resource);

	if (spark_dvb_attach_s(dvb_adap, &d3501config, &pFrontend))
	{
		i2c_put_adapter(pI2c);
		return -1;
	}

	if (dvb_register_frontend(dvb_adap, pFrontend))
	{
		printk("dvb-d3501: Frontend registration failed!\n");
		dvb_frontend_detach(pFrontend);
		i2c_put_adapter(pI2c);
		return -1;
	}

	(*ppFrontend)	= pFrontend;
	(*ppI2c) 		= pI2c;

	return 0;
}

int spark_dvb_register_s0(struct dvb_adapter *dvb_adap,
								struct dvb_frontend **ppFrontend,
								struct i2c_adapter **ppI2c)
{
	return spark_dvb_register_s(dvb_adap, 0, ppFrontend, ppI2c);
}

int spark_dvb_register_s1(struct dvb_adapter *dvb_adap,
								struct dvb_frontend **ppFrontend,
								struct i2c_adapter **ppI2c)
{
	return spark_dvb_register_s(dvb_adap, 1, ppFrontend, ppI2c);
}

int spark_dvb_attach_t(struct dvb_adapter *dvb_adap,
							struct i2c_adapter 	*pI2c,
							struct dvb_frontend **ppFrontend)
{
	struct dvb_frontend	*pFrontend = NULL;

	pFrontend = dvb_d0367_fe_ofdm_attach(pI2c);
	if(!pFrontend)
	{
		printk (KERN_INFO "%s: error attaching d0367\n", __FUNCTION__);
		return -1;
	}
	printk("%s: d0367 fe ofdm attached\n", __FUNCTION__);

	if(!dvb_attach(sharp6465_attach, pFrontend, &s6465_config, pI2c))

	{
		printk (KERN_INFO "%s: error attaching SHARP6465\n", __FUNCTION__);
		dvb_frontend_detach(pFrontend);
		return -1;
	}

	printk("%s: SHARP6465 attached\n", __FUNCTION__);

	(*ppFrontend) = pFrontend;
	return 0;
}

int spark_dvb_register_t(struct dvb_adapter *dvb_adap,
								struct dvb_frontend **ppFrontend,
								struct i2c_adapter **ppI2c)
{
	struct dvb_frontend *pFrontend;
	struct i2c_adapter	*pI2c;

  	pI2c = i2c_get_adapter(2);
	printk("pI2c = 0x%0x\n", (int)pI2c);
	if (!pI2c)
	{
	    return -1;
	}

	if (spark_dvb_attach_t(dvb_adap, pI2c, &pFrontend))
	{
		i2c_put_adapter(pI2c);
		return -1;
	}

	if (dvb_register_frontend(dvb_adap, pFrontend))
	{
		printk("dvb-d0367 t: Frontend registration failed!\n");
		dvb_frontend_detach(pFrontend);
		i2c_put_adapter(pI2c);
		return -1;
	}

	(*ppFrontend) 	= pFrontend;
	(*ppI2c)  		= pI2c;

	return 0;
}

int spark_dvb_attach_c(struct dvb_adapter *dvb_adap,
							struct i2c_adapter 	*pI2c,
							struct dvb_frontend **ppFrontend)
{
	struct dvb_frontend	*pFrontend = NULL;

	pFrontend = dvb_d0367_fe_qam_attach(pI2c);
	if(!pFrontend)
	{
		printk (KERN_INFO "%s: error attaching d0367 qam\n", __FUNCTION__);
		return -1;
	}
	printk("%s: d0367 fe qam attached\n", __FUNCTION__);

	if(!dvb_attach(sharp5469c_attach, pFrontend, &s5469c_config, pI2c))
	{
		printk (KERN_INFO "%s: error attaching SHARP5469C\n", __FUNCTION__);
		dvb_frontend_detach(pFrontend);
		return -1;
	}

	printk("%s: SHARP5469C attached\n", __FUNCTION__);

	(*ppFrontend) = pFrontend;
	return 0;
}

int spark_dvb_register_c(struct dvb_adapter *dvb_adap,
								struct dvb_frontend **ppCabFrontend,
								struct i2c_adapter **ppCabI2c)
{
	struct dvb_frontend *pFrontend;
	struct i2c_adapter	*pI2c;

  	pI2c = i2c_get_adapter(2);
	printk("pI2c = 0x%0x\n", (int)pI2c);

	if (spark_dvb_attach_c(dvb_adap, pI2c, &pFrontend))
	{
		i2c_put_adapter(pI2c);
		return -1;
	}

	if (dvb_register_frontend(dvb_adap, pFrontend))
	{
		printk("dvb-d0367 c: Frontend registration failed!\n");
		dvb_frontend_detach(pFrontend);
		i2c_put_adapter(pI2c);
		return -1;
	}

	(*ppCabFrontend)	= pFrontend;
	(*ppCabI2c)			= pI2c;

	return 0;
}

int spark_dvb_register_tc_by_type(struct dvb_adapter *dvb_adap,
											int iTunerType)
{
	int iRet = -1;
	printk("spark7162 type == %d", iTunerType);
	struct spark_dvb_adapter_adddata *pDvbAddData;

	pDvbAddData = (struct spark_dvb_adapter_adddata *)dvb_adap->priv;
	if (IS_UNION_TUNER_T(iTunerType))
	{
		iRet = spark_dvb_register_t(dvb_adap, &pDvbAddData->pD0367_frontend,
								&pDvbAddData->ter_i2c_adap);
	}
	else if (IS_UNION_TUNER_C(iTunerType))
	{
		iRet = spark_dvb_register_c(dvb_adap, &pDvbAddData->pD0367_frontend_2,
								&pDvbAddData->cab_i2c_adap);
	}
	else
	{
		iRet = spark_dvb_register_t(dvb_adap, &pDvbAddData->pD0367_frontend,
								&pDvbAddData->ter_i2c_adap);
	}

	return iRet;
}

int spark_dvb_register_tc(struct dvb_adapter *dvb_adap,
								struct dvb_frontend **ppTerFrontend,
								struct i2c_adapter **ppTerI2c,
								struct dvb_frontend **ppCabFrontend,
								struct i2c_adapter **ppCabI2c)
{
	spark_dvb_register_t(dvb_adap, ppTerFrontend, ppTerI2c);
	spark_dvb_register_c(dvb_adap, ppCabFrontend, ppCabI2c);

	return 0;
}

int spark_dvb_unregister_f(struct dvb_frontend *pFrontend,
								struct i2c_adapter *pI2c)
{
	dvb_unregister_frontend(pFrontend);
	dvb_frontend_detach(pFrontend);
	i2c_put_adapter(pI2c);

	return 0;
}

int spark_dvb_unregister_s(struct dvb_frontend *pFrontend,
								struct i2c_adapter *pI2c)
{
	return spark_dvb_unregister_f(pFrontend, pI2c);
}

int spark_dvb_unregister_s0(struct dvb_adapter *dvb_adap)
{
	struct dvb_frontend *pFrontend;
	struct i2c_adapter	*pI2c;
	struct spark_dvb_adapter_adddata *pDvbAddData;

	pDvbAddData = (struct spark_dvb_adapter_adddata *)dvb_adap->priv;

	pFrontend 	= pDvbAddData->pD3501_frontend;
	pI2c 		= pDvbAddData->qpsk_i2c_adap;

	spark_dvb_unregister_s(pFrontend, pI2c);

	return 0;
}

int spark_dvb_unregister_s1(struct dvb_adapter *dvb_adap)
{
	struct dvb_frontend *pFrontend;
	struct i2c_adapter	*pI2c;
	struct spark_dvb_adapter_adddata *pDvbAddData;

	pDvbAddData = (struct spark_dvb_adapter_adddata *)dvb_adap->priv;

	pFrontend 	= pDvbAddData->pD3501_frontend_2;
	pI2c 		= pDvbAddData->qpsk_i2c_adap_2;

	spark_dvb_unregister_s(pFrontend, pI2c);

	return 0;
}

int spark_dvb_unregister_t(struct dvb_adapter *dvb_adap)
{
	struct dvb_frontend *pFrontend;
	struct i2c_adapter	*pI2c;
	struct spark_dvb_adapter_adddata *pDvbAddData;

	pDvbAddData = (struct spark_dvb_adapter_adddata *)dvb_adap->priv;

	pFrontend 	= pDvbAddData->pD0367_frontend;
	pI2c 		= pDvbAddData->ter_i2c_adap;

	spark_dvb_unregister_f(pFrontend, pI2c);

	return 0;
}

int spark_dvb_unregister_c(struct dvb_adapter *dvb_adap)
{
	struct dvb_frontend *pFrontend;
	struct i2c_adapter	*pI2c;
	struct spark_dvb_adapter_adddata *pDvbAddData;

	pDvbAddData = (struct spark_dvb_adapter_adddata *)dvb_adap->priv;

	pFrontend 	= pDvbAddData->pD0367_frontend_2;
	pI2c 		= pDvbAddData->cab_i2c_adap;

	spark_dvb_unregister_f(pFrontend, pI2c);

	return 0;
}

int spark_dvb_unregister_tc_by_type(struct dvb_adapter *dvb_adap,
											int iTunerType)
{
	int iRet = -1;

	if (IS_UNION_TUNER_T(iTunerType))
	{
		iRet = spark_dvb_unregister_t(dvb_adap);
	}
	else if (IS_UNION_TUNER_C(iTunerType))
	{
		iRet = spark_dvb_unregister_c(dvb_adap);
	}
	else
	{
		iRet = spark_dvb_unregister_t(dvb_adap);
	}

	return iRet;
}

int spark7162_register_frontend(struct dvb_adapter *dvb_adap)
{
	struct spark_dvb_adapter_adddata *pDvbAddData;

	pDvbAddData = kmalloc(sizeof(struct spark_dvb_adapter_adddata), GFP_KERNEL);

	memset(pDvbAddData, 0, sizeof(struct spark_dvb_adapter_adddata));

	dvb_adap->priv = (void *)pDvbAddData;

	spark_dvb_register_tc_by_type(dvb_adap, eUnionTunerType);

	#if 1
	spark_dvb_register_s1(dvb_adap,
							&pDvbAddData->pD3501_frontend_2,
							&pDvbAddData->qpsk_i2c_adap_2);
	spark_dvb_register_s0(dvb_adap,
							&pDvbAddData->pD3501_frontend,
							&pDvbAddData->qpsk_i2c_adap);
	#endif  /* 0 */
	return 0;
}

void spark7162_unregister_frontend(struct dvb_adapter *dvb_adap)
{
	struct spark_dvb_adapter_adddata *pDvbAddData;

	pDvbAddData = (struct spark_dvb_adapter_adddata *)dvb_adap->priv;

#if 1
	spark_dvb_unregister_s0(dvb_adap);

	printk("%s:%d!\n", __FUNCTION__, __LINE__);

	spark_dvb_unregister_s1(dvb_adap);

	printk("%s:%d!\n", __FUNCTION__, __LINE__);
#endif	/* 0 */

	spark_dvb_unregister_tc_by_type(dvb_adap, eUnionTunerType);

	printk("%s:%d!\n", __FUNCTION__, __LINE__);


	kfree(pDvbAddData);
}

EXPORT_SYMBOL(spark7162_register_frontend);
EXPORT_SYMBOL(spark7162_unregister_frontend);

int UnionTunerConfig(char *pcTunerType)
{
	int eTunerType = UNION_TUNER_T;
	if (!pcTunerType)
	{
	    return UNION_TUNER_T;
	}
	if (!strcmp("t", pcTunerType))
	{
		eTunerType = UNION_TUNER_T;
	}
	else if (!strcmp("c", pcTunerType))
	{
		eTunerType = UNION_TUNER_C;
	}
	else
	{
		eTunerType = UNION_TUNER_T;
	}
	return eTunerType;
}


int __init spark_init(void)
{
    int ret;
#if (DVB_API_VERSION > 3)

    short int AdapterNumbers[] = { -1 };

#endif

    printk("%s >\n", __func__);

	eUnionTunerType = UnionTunerConfig(UnionTunerType);

#if (DVB_API_VERSION > 3)
	ret = dvb_register_adapter(&spark_dvb_adapter, "spark",
				  				THIS_MODULE, &spark_device, AdapterNumbers);
#else
	ret = dvb_register_adapter(&spark_dvb_adapter, "spark",
				  				THIS_MODULE, &spark_device);
#endif
	printk("ret = %d\n", ret);

	ret = spark7162_register_frontend(&spark_dvb_adapter);
	if (ret < 0)
	{
		dvb_unregister_adapter(&spark_dvb_adapter);
		return -1;
	}
    printk("%s < %d\n", __func__, ret);

    return ret;
}

static void spark_cleanup(void)
{
    printk("%s >\n", __func__);
	spark7162_unregister_frontend(&spark_dvb_adapter);
	dvb_unregister_adapter(&spark_dvb_adapter);
    printk("%s <\n", __func__);
}

int __init spark_get_tuner_type(void)
{
	eUnionTunerType = UnionTunerConfig(UnionTunerType);
}

//#define D3501_SINGLE
#ifdef D3501_SINGLE
module_init(spark_init);
module_exit(spark_cleanup);
#else
module_init(spark_get_tuner_type);
#endif

module_param(UnionTunerType, charp, 0);
MODULE_PARM_DESC(UnionTunerType, "Union Tuner Type (t, c)");

/* EOF------------------------------------------------------------------------*/

/* BOL-------------------------------------------------------------------*/
//$Log$
/* EOL-------------------------------------------------------------------*/

