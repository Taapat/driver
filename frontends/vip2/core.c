#include "core.h"
#include "stv6110x.h"
#include "stv090x.h"
#include "ix2470.h"
#include "ix2470_cfg.h"
#include "zl10353.h"

#include <linux/version.h>
#include <linux/platform_device.h>
#include <asm/system.h>
#include <asm/io.h>
#include <linux/dvb/dmx.h>
#include <linux/proc_fs.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif

#include <pvr_config.h>

#define I2C_ADDR_STV090X	(0xd0 >> 1)
#define I2C_ADDR_STV6110X	(0xc0 >> 1)
#define I2C_ADDR_IX2470		(0xc0 >> 1)
#define I2C_ADDR_CE6353		(0x1e >> 1)

#define CLK_EXT_IX2470 		4000000
#define CLK_EXT_STV6110 	8000000

static char *demod1 = "stv090x";
static char *demod2 = "ce6353";
static char *tuner1 = "sharp7306";
static char *tuner2 = "sharp6465";

static int demodType1;
static int demodType2;
static int tunerType1;
static int tunerType2;

static struct core *core[MAX_DVB_ADAPTERS];

module_param(demod1,charp,0);
module_param(demod2,charp,0);
MODULE_PARM_DESC(demod1, "demodelator1 (default stv090x");// DVB-S2
MODULE_PARM_DESC(demod1, "demodelator2 (default ce6353"); // DVB-T

module_param(tuner1,charp,0);
module_param(tuner2,charp,0);
MODULE_PARM_DESC(tuner1, "tuner1 (default sharp7306"); // DVB-S2
MODULE_PARM_DESC(tuner2, "tuner2 (default sharp6465"); // DVB-T

enum {
	STV090X,
	CE6353,
};

enum {
	STV6110X,
	SHARP7306,
	SHARP6465,
};

enum fe_ctl {
	FE1_1318 	= 0,
	FE1_RST		= 1,
	FE1_LNB_EN	= 2,
	FE1_1419	= 3,
	FE0_RST		= 4,
	FE0_LNB_EN	= 5,
	FE0_1419	= 6,
	FE0_1318	= 7,
};

static unsigned char fctl = 0;

struct stpio_pin*	srclk; // shift clock
struct stpio_pin*	rclk;  // latch clock
struct stpio_pin*	sda;   // serial data

#define SRCLK_CLR() {stpio_set_pin(srclk, 0);}
#define SRCLK_SET() {stpio_set_pin(srclk, 1);}

#define RCLK_CLR() {stpio_set_pin(rclk, 0);}
#define RCLK_SET() {stpio_set_pin(rclk, 1);}

#define SDA_CLR() {stpio_set_pin(sda, 0);}
#define SDA_SET() {stpio_set_pin(sda, 1);}

void hc595_out(unsigned char ctls, int state)
{
	int i;

	if(state)
		fctl |= 1 << ctls;
	else
		fctl &= ~(1 << ctls);

	/*
	 * clear everything out just in case to
	 * prepare shift register for bit shifting
	 */

	SDA_CLR();
	SRCLK_CLR();
    udelay(10);

	printk("+++ fctl= ");

    for(i = 7; i >=0; i--)
	{
    	SRCLK_CLR();
		if(fctl & (1<<i))
		{
			SDA_SET();
			printk("1");
		}
		else
		{
			SDA_CLR();
			printk("0");
		}
		udelay(1);
		SRCLK_SET();
		udelay(1);
	}
	printk("\n");
    RCLK_CLR();
    udelay(1);
    RCLK_SET();
}

EXPORT_SYMBOL(hc595_out);

static struct stv090x_config stv090x_config = {
	.device			= STV0903,
	.demod_mode		= STV090x_DUAL/*STV090x_SINGLE*/,
	.clk_mode		= STV090x_CLK_EXT,

	.xtal			= CLK_EXT_IX2470,
	.address		= I2C_ADDR_STV090X,

	.ts1_mode		= STV090x_TSMODE_DVBCI/*STV090x_TSMODE_SERIAL_CONTINUOUS*/,
	.ts2_mode		= STV090x_TSMODE_DVBCI/*STV090x_TSMODE_SERIAL_CONTINUOUS*/,
	.ts1_clk		= 0,
	.ts2_clk		= 0,

	.fe_rst			= 0,
	.fe_lnb_en		= 0,
	.fe_1419		= 0,
	.fe_1318		= 0,

	.repeater_level	= STV090x_RPTLEVEL_16,

	.tuner_init				= NULL,
	.tuner_set_mode			= NULL,
	.tuner_set_frequency	= NULL,
	.tuner_get_frequency	= NULL,
	.tuner_set_bandwidth	= NULL,
	.tuner_get_bandwidth	= NULL,
	.tuner_set_bbgain		= NULL,
	.tuner_get_bbgain		= NULL,
	.tuner_set_refclk		= NULL,
	.tuner_get_status		= NULL,
};

static struct stv6110x_config stv6110x_config = {
	.addr			= I2C_ADDR_STV6110X,
	.refclk			= CLK_EXT_STV6110,
};

static const struct ix2470_config bs2s7hz7306a_config = {
	.name		= "Sharp BS2S7HZ7306A",
	.addr		= I2C_ADDR_IX2470,
	.step_size 	= IX2470_STEP_1000,
	.bb_lpf		= IX2470_LPF_12,
	.bb_gain	= IX2470_GAIN_2dB,
};

static struct zl10353_config ce6353_config = {
	.demod_address = I2C_ADDR_CE6353,
	.parallel_ts = 1,
};

static struct dvb_frontend * frontend_init(struct core_config *cfg, int i)
{
	struct stv6110x_devctl *ctl;
	struct dvb_frontend *frontend = NULL;

	printk (KERN_INFO "%s >\n", __FUNCTION__);

	if (i== 0)
	{
		switch (demodType1) {
		case STV090X:
		{
			frontend = dvb_attach(stv090x_attach, &stv090x_config,
					cfg->i2c_adap, STV090x_DEMODULATOR_0);
			if (frontend) {
				printk("%s: stv090x attached\n", __FUNCTION__);

				stv090x_config.fe_rst 	 = cfg->tuner->fe_rst;
				stv090x_config.fe_1318 	 = cfg->tuner->fe_1318;
				stv090x_config.fe_1419 	 = cfg->tuner->fe_1419;
				stv090x_config.fe_lnb_en = cfg->tuner->fe_lnb_en;

				switch (tunerType1) {
				case SHARP7306:
					if(dvb_attach(ix2470_attach, frontend, &bs2s7hz7306a_config, cfg->i2c_adap))
					{
						printk("%s: ix2470 attached\n", __FUNCTION__);
						stv090x_config.tuner_set_frequency 	= ix2470_set_frequency;
						stv090x_config.tuner_get_frequency 	= ix2470_get_frequency;
						stv090x_config.tuner_get_bandwidth 	= ix2470_get_bandwidth;
						stv090x_config.tuner_get_status	  	= frontend->ops.tuner_ops.get_status;
					}else{
						printk (KERN_INFO "%s: error attaching ix2470\n", __FUNCTION__);
						goto error_out;
					}
					break;
				case STV6110X:
				default:
					ctl = dvb_attach(stv6110x_attach, frontend, &stv6110x_config, cfg->i2c_adap);
					if(ctl)	{
						printk("%s: stv6110x attached\n", __FUNCTION__);
						stv090x_config.tuner_init	  	  	= ctl->tuner_init;
						stv090x_config.tuner_set_mode	  	= ctl->tuner_set_mode;
						stv090x_config.tuner_set_frequency 	= ctl->tuner_set_frequency;
						stv090x_config.tuner_get_frequency 	= ctl->tuner_get_frequency;
						stv090x_config.tuner_set_bandwidth 	= ctl->tuner_set_bandwidth;
						stv090x_config.tuner_get_bandwidth 	= ctl->tuner_get_bandwidth;
						stv090x_config.tuner_set_bbgain	  	= ctl->tuner_set_bbgain;
						stv090x_config.tuner_get_bbgain	  	= ctl->tuner_get_bbgain;
						stv090x_config.tuner_set_refclk	  	= ctl->tuner_set_refclk;
						stv090x_config.tuner_get_status	  	= ctl->tuner_get_status;
					} else {
						printk (KERN_INFO "%s: error attaching stv6110x\n", __FUNCTION__);
						goto error_out;
					}
				}
			} else {
				printk (KERN_INFO "%s: error attaching stv090x\n", __FUNCTION__);
				goto error_out;
			}
			break;
		}
		case CE6353:
		{
			frontend = dvb_attach(zl10353_attach, &ce6353_config, cfg->i2c_adap);
			if (frontend) {
				printk("%s: ce6353 attached\n", __FUNCTION__);
				switch (tunerType1) {
				case SHARP6465: // TODO
					break;
				}
			} else {
				printk (KERN_INFO "%s: error attaching ce6353\n", __FUNCTION__);
				goto error_out;
			}
			break;
		}
		}
	}
	else
	{
		switch (demodType2) {
		case STV090X:
		{
			frontend = dvb_attach(stv090x_attach, &stv090x_config,
					cfg->i2c_adap, STV090x_DEMODULATOR_1);
			if (frontend) {
				printk("%s: stv090x attached\n", __FUNCTION__);

				stv090x_config.fe_rst 	 = cfg->tuner->fe_rst;
				stv090x_config.fe_1318 	 = cfg->tuner->fe_1318;
				stv090x_config.fe_1419 	 = cfg->tuner->fe_1419;
				stv090x_config.fe_lnb_en = cfg->tuner->fe_lnb_en;

				switch (tunerType2) {
				case SHARP7306:
					if(dvb_attach(ix2470_attach, frontend, &bs2s7hz7306a_config, cfg->i2c_adap))
					{
						printk("%s: ix2470 attached\n", __FUNCTION__);
						stv090x_config.tuner_set_frequency 	= ix2470_set_frequency;
						stv090x_config.tuner_get_frequency 	= ix2470_get_frequency;
						stv090x_config.tuner_get_bandwidth 	= ix2470_get_bandwidth;
						stv090x_config.tuner_get_status	  	= frontend->ops.tuner_ops.get_status;
					}else{
						printk (KERN_INFO "%s: error attaching ix2470\n", __FUNCTION__);
						goto error_out;
					}
					break;
				case STV6110X:
				default:
					ctl = dvb_attach(stv6110x_attach, frontend, &stv6110x_config, cfg->i2c_adap);
					if(ctl)	{
						printk("%s: stv6110x attached\n", __FUNCTION__);
						stv090x_config.tuner_init	  	  	= ctl->tuner_init;
						stv090x_config.tuner_set_mode	  	= ctl->tuner_set_mode;
						stv090x_config.tuner_set_frequency 	= ctl->tuner_set_frequency;
						stv090x_config.tuner_get_frequency 	= ctl->tuner_get_frequency;
						stv090x_config.tuner_set_bandwidth 	= ctl->tuner_set_bandwidth;
						stv090x_config.tuner_get_bandwidth 	= ctl->tuner_get_bandwidth;
						stv090x_config.tuner_set_bbgain	  	= ctl->tuner_set_bbgain;
						stv090x_config.tuner_get_bbgain	  	= ctl->tuner_get_bbgain;
						stv090x_config.tuner_set_refclk	  	= ctl->tuner_set_refclk;
						stv090x_config.tuner_get_status	  	= ctl->tuner_get_status;
					} else {
						printk (KERN_INFO "%s: error attaching stv6110x\n", __FUNCTION__);
						goto error_out;
					}
				}
			} else {
				printk (KERN_INFO "%s: error attaching stv090x\n", __FUNCTION__);
				goto error_out;
			}
			break;
		}
		case CE6353:
		{
			frontend = dvb_attach(zl10353_attach, &ce6353_config, cfg->i2c_adap);
			if (frontend) {
				printk("%s: ce6353 attached\n", __FUNCTION__);
				switch (tunerType2) {
				case SHARP6465: // TODO
					break;
				}
			} else {
				printk (KERN_INFO "%s: error attaching ce6353\n", __FUNCTION__);
				goto error_out;
			}
			break;
		}
		}
	}

	return frontend;

error_out:
	printk("core: Frontend registration failed!\n");
	if (frontend)
		dvb_frontend_detach(frontend);
	return NULL;
}

static struct dvb_frontend *init_fe_device (struct dvb_adapter *adapter,
                     struct tuner_config *tuner_cfg, int i)
{
  struct core_state *state;
  struct dvb_frontend *frontend;
  struct core_config *cfg;

  printk ("> (bus = %d) %s\n", tuner_cfg->i2c_bus,__FUNCTION__);

  cfg = kmalloc (sizeof (struct core_config), GFP_KERNEL);
  if (cfg == NULL)
  {
    printk ("fe-core: kmalloc failed\n");
    return NULL;
  }

  /* initialize the config data */
  cfg->tuner = tuner_cfg;
  cfg->i2c_adap = i2c_get_adapter (tuner_cfg->i2c_bus);
  printk("i2c adapter = 0x%0x\n", cfg->i2c_adap);

  if (cfg->i2c_adap == NULL)
  {
    printk ("stv090x: failed to allocate i2c resources\n");

    kfree (cfg);
    return NULL;
  }

  /* set to low */
  hc595_out (tuner_cfg->fe_rst, 0);
  /* Wait for everything to die */
  msleep(50);
  /* Pull it up out of Reset state */
  hc595_out (tuner_cfg->fe_rst, 1);
  /* Wait for PLL to stabilize */
  msleep(250);
  /*
   * PLL state should be stable now. Ideally, we should check
   * for PLL LOCK status. But well, never mind!
   */
  frontend = frontend_init(cfg, i);

  if (frontend == NULL)
  {
	printk("No frontend found !\n");
    return NULL;
  }

  printk (KERN_INFO "%s: Call dvb_register_frontend (adapter = 0x%x)\n",
           __FUNCTION__, (unsigned int) adapter);

  frontend->id = i;
  if (dvb_register_frontend (adapter, frontend))
  {
    printk ("%s: Frontend registration failed !\n", __FUNCTION__);
    if (frontend->ops.release)
      frontend->ops.release (frontend);
    return NULL;
  }

  state = frontend->demodulator_priv;

  return frontend;
}

struct tuner_config tuner_resources[] = {

        [0] = {
                .adapter 	= 0,
                .i2c_bus 	= 0,
          	  	.fe_rst 	= FE0_RST,
          	  	.fe_1318 	= FE0_1318,
          	  	.fe_1419 	= FE0_1419,
          	  	.fe_lnb_en  = FE0_LNB_EN,
        },
        [1] = {
                .adapter 	= 0,
                .i2c_bus 	= 1,
          	  	.fe_rst 	= FE1_RST,
          	  	.fe_1318 	= FE1_1318,
          	  	.fe_1419 	= FE1_1419,
          	  	.fe_lnb_en  = FE1_LNB_EN,
        },
};

void fe_core_register_frontend(struct dvb_adapter *dvb_adap)
{
	int i = 0;
	int vLoop = 0;

	printk (KERN_INFO "%s: Spider-Team plug and play frontend core\n", __FUNCTION__);

	core[i] = (struct core*) kmalloc(sizeof(struct core),GFP_KERNEL);
	if (!core[i])
		return;

	memset(core[i], 0, sizeof(struct core));

	core[i]->dvb_adapter = dvb_adap;
	dvb_adap->priv = core[i];

	printk("tuner = %d\n", ARRAY_SIZE(tuner_resources));

	srclk= stpio_request_pin (2, 2, "HC595_SRCLK", STPIO_OUT);
	rclk = stpio_request_pin (2, 3, "HC595_RCLK", STPIO_OUT);
	sda  = stpio_request_pin (2, 4, "HC595_SDA", STPIO_OUT);

	if ((srclk == NULL) ||  (rclk==NULL) || (sda==NULL))
	{
	    printk ("stv090x: failed to allocate IO resources\n");

	    if(srclk != NULL)
	      stpio_free_pin (srclk);

	    if(rclk != NULL)
	      stpio_free_pin (rclk);

	    if(sda != NULL)
	      stpio_free_pin (sda);

	    return;
	}

	for (vLoop = 0; vLoop < ARRAY_SIZE(tuner_resources); vLoop++)
	{
	  if (core[i]->frontend[vLoop] == NULL)
	  {
      	     printk("%s: init tuner %d\n", __FUNCTION__, vLoop);
	     core[i]->frontend[vLoop] =
				   init_fe_device (core[i]->dvb_adapter, &tuner_resources[vLoop], vLoop);
	  }
	}

	printk (KERN_INFO "%s: <\n", __FUNCTION__);

	return;
}

EXPORT_SYMBOL(fe_core_register_frontend);

int __init fe_core_init(void)
{
	/****** FRONT 0 ********/
	if((demod1[0] == 0) || (strcmp("stv090x", demod1) == 0))
	{
		printk("demodelator1: stv090x dvb-s2    ");
		demodType1 = STV090X;
	}
	else if(strcmp("ce6353", demod1) == 0)
	{
		printk("demodelator1: ce6353 dvb-t    ");
		demodType1 = CE6353;
	}

	if((tuner1[0] == 0) || (strcmp("sharp7306", tuner1) == 0))
	{
		printk("tuner1: sharp7306\n");
		tunerType1 = SHARP7306;
	}
	else if(strcmp("stv6110x", tuner1) == 0)
	{
		printk("tuner1: stv6110x\n");
		tunerType1 = STV6110X;
	}
	else if(strcmp("sharp6465", tuner1) == 0)
	{
		printk("tuner1: sharp6465\n");
		tunerType1 = SHARP6465;
	}

	/****** FRONT 1 ********/
	if((demod2[0] == 0) || (strcmp("stv090x", demod2) == 0))
	{
		printk("demodelator2: stv090x dvb-s2    ");
		demodType2 = STV090X;
	}
	else if(strcmp("ce6353", demod2) == 0)
	{
		printk("demodelator2: ce6353 dvb-t    ");
		demodType2 = CE6353;
	}

	if((tuner2[0] == 0) || (strcmp("sharp7306", tuner2) == 0))
	{
		printk("tuner2: sharp7306\n");
		tunerType2 = SHARP7306;
	}
	else if(strcmp("stv6110x", tuner2) == 0)
	{
		printk("tuner2: stv6110x\n");
		tunerType2 = STV6110X;
	}
	else if(strcmp("sharp6465", tuner2) == 0)
	{
		printk("tuner2: sharp6465\n");
		tunerType2 = SHARP6465;
	}

	printk("frontend core loaded\n");
    return 0;
}

static void __exit fe_core_exit(void)
{
   printk("frontend core unloaded\n");
}

module_init             (fe_core_init);
module_exit             (fe_core_exit);

MODULE_DESCRIPTION      ("Tunerdriver");
MODULE_AUTHOR           ("Spider-Team");
MODULE_LICENSE          ("GPL");
