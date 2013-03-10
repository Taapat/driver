#include <linux/module.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/slab.h>

#include <linux/dvb/version.h>

//#include "tuner_interface.h"

#include "nim_dev.h"
#include "mxl_common.h"
#include "d6158.h"

#include "dvb_frontend.h"

#include "stv0367ofdm_init.h"


struct dvb_d6158_fe_ofdm_state {
	struct nim_device 	   spark_nimdev;
	struct i2c_adapter			*i2c;
	struct dvb_frontend 		frontend;
	//IOARCH_Handle_t				IOHandle;
	TUNER_IOREG_DeviceMap_t		DeviceMap;
	struct dvb_frontend_parameters 	*p;
};


int d6158_read_snr(struct dvb_frontend* fe, u16* snr);
int d6158_read_ber(struct dvb_frontend* fe, UINT32 *ber);
int d6158_read_status(struct dvb_frontend *fe, enum fe_status *status);
int d6158_read_signal_strength(struct dvb_frontend* fe, u16 *strength);
int d6158_set_frontend(struct dvb_frontend* fe,struct dvb_frontend_parameters *p);
int d6158_read_ucblocks(struct dvb_frontend* fe,u32* ucblocks);
int dvb_d6158_get_property(struct dvb_frontend *fe, struct dtv_property* tvp);

INT32 tun_mxl301_init(UINT32 *tuner_idx, struct COFDM_TUNER_CONFIG_EXT * ptrTuner_Config);
INT32 tun_mxl301_status(UINT32 tuner_idx, UINT8 *lock);
INT32 tun_mxl301_control(UINT32 tuner_idx, UINT32 freq, UINT8 bandwidth,UINT8 AGC_Time_Const,UINT8 *data,UINT8 _i2c_cmd);
INT32 nim_panic6158_open(struct nim_device *dev);
YW_ErrorType_T tuner_mxl301_Identify(IOARCH_Handle_t   *i2c_adap);
YW_ErrorType_T  demod_d6158_Identify(struct nim_device *dev,U8 *pucActualID);



static struct dvb_frontend_ops dvb_d6158_fe_ofdm_ops = {

	.info = {
		.name			= "d6158 DVB-T2",
		.type			= FE_OFDM,
		.frequency_min		= 0,
		.frequency_max		= 863250000,
		.frequency_stepsize = 62500,
		.caps = FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4 |
				FE_CAN_FEC_4_5 | FE_CAN_FEC_5_6 | FE_CAN_FEC_6_7 |
				FE_CAN_FEC_7_8 | FE_CAN_FEC_8_9 | FE_CAN_FEC_AUTO |
				FE_CAN_QAM_16 | FE_CAN_QAM_64 | FE_CAN_QAM_AUTO |
				FE_CAN_TRANSMISSION_MODE_AUTO |
				FE_CAN_GUARD_INTERVAL_AUTO |
				FE_CAN_HIERARCHY_AUTO,
				},


	.init				= NULL, //d3501_init,
	.release			= NULL, //d3501_release,
	.sleep = NULL,//dvb_d0367_fe_ofdm_sleep,
	.set_frontend = d6158_set_frontend,
	.get_frontend = NULL,//dvb_d0367_fe_ofdm_get_frontend,

	.read_ber			= d6158_read_ber,
	.read_snr			= d6158_read_snr,
	.read_signal_strength	= d6158_read_signal_strength,
	.read_status		= d6158_read_status,

	.read_ucblocks = d6158_read_ucblocks,
	.i2c_gate_ctrl		=  NULL,//NULLdvb_d0367_fe_ofdm_i2c_gate_ctrl,
#if (DVB_API_VERSION < 5)
	.get_info			= NULL,
#else
		.get_property		= dvb_d6158_get_property,
#endif

};

struct dvb_frontend* dvb_d6158_fe_ofdm_attach(struct i2c_adapter* i2c)
{
	struct dvb_d6158_fe_ofdm_state* state = NULL;
	struct nim_panic6158_private *priv;
	int ret;
		UINT8 data;

	/* allocate memory for the internal state */
	state = kzalloc(sizeof(struct dvb_d6158_fe_ofdm_state), GFP_KERNEL);
	if (state == NULL) goto error;

	priv = (PNIM_PANIC6158_PRIVATE)YWOS_Malloc(sizeof(struct nim_panic6158_private));
	if(NULL == priv)
	{
		goto error;
	}

	/* create dvb_frontend */
	memcpy(&state->frontend.ops, &dvb_d6158_fe_ofdm_ops, sizeof(struct dvb_frontend_ops));
	state->frontend.demodulator_priv = state;
	state->i2c = i2c;

	state->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
	state->DeviceMap.Registers = STV0367ofdm_NBREGS;
	state->DeviceMap.Fields    = STV0367ofdm_NBFIELDS;
	state->DeviceMap.Mode      = IOREG_MODE_SUBADR_16;
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
	priv->system = DEMO_BANK_T2;
	priv->tuner_id = 2;

#if 1

	 struct COFDM_TUNER_CONFIG_API	Tuner_API;

	if(tuner_mxl301_Identify((IOARCH_Handle_t*)i2c)!= YW_NO_ERROR)
	{
		printk("tuner_mxl301_Identify error!\n");
	}
	YWOS_TaskSleep(50);

    YWLIB_Memset(&Tuner_API, 0, sizeof(struct COFDM_TUNER_CONFIG_API));

	Tuner_API.nim_Tuner_Init = tun_mxl301_init;
	Tuner_API.nim_Tuner_Status = tun_mxl301_status;
	Tuner_API.nim_Tuner_Control = tun_mxl301_control;
	Tuner_API.tune_t2_first = 0;//when demod_d6158_ScanFreq,param.priv_param >DEMO_DVBT2,tuner tune T2 then t
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
			priv->tc.nim_Tuner_Status  = Tuner_API.nim_Tuner_Status;
			YWLIB_Memcpy(&priv->tc.tuner_config,&Tuner_API.tuner_config, sizeof(struct COFDM_TUNER_CONFIG_EXT));
		}

	}

	ret = nim_panic6158_open(&state->spark_nimdev);
	printk("[%s]%d,open result=%d \n",__FUNCTION__,__LINE__,ret);

	/* Setup init work mode */

	#endif

	return &state->frontend;

error:
	kfree(state);
	return NULL;
}

//MODULE_DESCRIPTION("DVB d0367 Frontend");
//MODULE_AUTHOR("oSaiYa");
//MODULE_LICENSE("GPL");

EXPORT_SYMBOL(dvb_d6158_fe_ofdm_attach);


