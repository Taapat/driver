#include "core.h"
#include "stv6110x.h"
#include "stv090x.h"
#include <linux/platform_device.h>
#include <asm/system.h>
#include <asm/io.h>
#include <linux/dvb/dmx.h>
#include <linux/proc_fs.h>
#if defined (CONFIG_KERNELVERSION) /* ST Linux 2.3 */
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif

#include <pvr_config.h>

short paramDebug = 0;

static struct core *core[MAX_DVB_ADAPTERS];

static struct stv090x_config tt1600_stv090x_config = {
	.device			= STV0903,
	.demod_mode		= STV090x_DUAL/*STV090x_SINGLE*/,
	.clk_mode		= STV090x_CLK_EXT,

	.xtal			= 16000000/*8000000*/,
	.address		= 0x68,

	.ts1_mode		= STV090x_TSMODE_DVBCI/*STV090x_TSMODE_SERIAL_CONTINUOUS*/,
	.ts2_mode		= STV090x_TSMODE_DVBCI/*STV090x_TSMODE_SERIAL_CONTINUOUS*/,
	.ts1_clk		= 0, /* diese regs werden in orig nicht gesetzt */
	.ts2_clk		= 0, /* diese regs werden in orig nicht gesetzt */

	.repeater_level	= STV090x_RPTLEVEL_16,

	.lnb_enable 		= NULL,
	.lnb_vsel	 		= NULL,

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
	.addr			= 0x60,
	.refclk			= 16000000,
};

static struct dvb_frontend * frontend_init(struct core_config *cfg, int i)
{
	struct stv6110x_devctl *ctl;
	struct dvb_frontend *frontend = NULL;

	printk (KERN_INFO "%s >\n", __FUNCTION__);

	if (i== 0)
		frontend = stv090x_attach(&tt1600_stv090x_config, cfg->i2c_adap, STV090x_DEMODULATOR_0);
	else
		return;
		//frontend = stv090x_attach(&tt1600_stv090x_config, cfg->i2c_adap, STV090x_DEMODULATOR_0, STV090x_TUNER2);

	if (frontend) {
		printk("%s: attached\n", __FUNCTION__);

		ctl = stv6110x_attach(frontend,
				      &stv6110x_config,
				      cfg->i2c_adap);

		tt1600_stv090x_config.tuner_init	  = ctl->tuner_init;
		tt1600_stv090x_config.tuner_set_mode	  = ctl->tuner_set_mode;
		tt1600_stv090x_config.tuner_set_frequency = ctl->tuner_set_frequency;
		tt1600_stv090x_config.tuner_get_frequency = ctl->tuner_get_frequency;
		tt1600_stv090x_config.tuner_set_bandwidth = ctl->tuner_set_bandwidth;
		tt1600_stv090x_config.tuner_get_bandwidth = ctl->tuner_get_bandwidth;
		tt1600_stv090x_config.tuner_set_bbgain	  = ctl->tuner_set_bbgain;
		tt1600_stv090x_config.tuner_get_bbgain	  = ctl->tuner_get_bbgain;
		tt1600_stv090x_config.tuner_set_refclk	  = ctl->tuner_set_refclk;
		tt1600_stv090x_config.tuner_get_status	  = ctl->tuner_get_status;


	} else {
		printk (KERN_INFO "%s: error attaching\n", __FUNCTION__);
		goto error_out;
	}

	return frontend;

error_out:
	printk("core: Frontend registration failed!\n");
	if (frontend)
		dvb_frontend_detach(frontend);
	return NULL;
}

static struct dvb_frontend *
init_fe_device (struct dvb_adapter *adapter,
                     struct plat_tuner_config *tuner_cfg, int i)
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
  cfg->i2c_adap = i2c_get_adapter (tuner_cfg->i2c_bus);

  printk("i2c adapter = 0x%0x\n", cfg->i2c_adap);

  cfg->i2c_addr = tuner_cfg->i2c_addr;

  printk("i2c addr = %02x\n", cfg->i2c_addr);

  printk("tuner reset = %d.%d\n", tuner_cfg->tuner_enable[0], tuner_cfg->tuner_enable[1]);

  cfg->tuner_reset_pin = stpio_request_pin (tuner_cfg->tuner_enable[0],
                                          tuner_cfg->tuner_enable[1],
                                          "TUNER RST", STPIO_OUT);

  cfg->lnb_enable = stpio_request_pin (tuner_cfg->lnb_enable[0],
                                          tuner_cfg->lnb_enable[1],
                                          "LNB_POWER", STPIO_OUT);

  cfg->lnb_vsel = stpio_request_pin (tuner_cfg->lnb_vsel[0],
                                          tuner_cfg->lnb_vsel[1],
                                          "LNB 13/18", STPIO_OUT);

  if ((cfg->i2c_adap == NULL) || (cfg->tuner_reset_pin == NULL) ||
		  (cfg->lnb_vsel==NULL) || (cfg->lnb_enable==NULL)) {

    printk ("stv090x: failed to allocate resources (%s)\n",
    		(cfg->i2c_adap == NULL)?"i2c":"STPIO error");

    if(cfg->tuner_reset_pin != NULL)
      stpio_free_pin (cfg->tuner_reset_pin);

    if(cfg->lnb_enable != NULL)
      stpio_free_pin (cfg->lnb_enable);

    if(cfg->lnb_vsel != NULL)
      stpio_free_pin (cfg->lnb_vsel);

    kfree (cfg);
    return NULL;
  }

  cfg->tuner_reset_act = tuner_cfg->tuner_enable[2];

  if (cfg->tuner_reset_pin != NULL)
  {
    /* set to low */
    stpio_set_pin (cfg->tuner_reset_pin, !cfg->tuner_reset_act);
    /* Wait for everything to die */
    msleep(50);
    /* Pull it up out of Reset state */
    stpio_set_pin (cfg->tuner_reset_pin, cfg->tuner_reset_act);
  }

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

struct plat_tuner_config tuner_resources[] = {

        [0] = {
                .adapter 	= 0,
                .i2c_bus 	= 0,
                .i2c_addr 	= 0x68,
                .tuner_enable 	= {2, 3, 1},
                .lnb_enable 	= {1, 6, 1},
                .lnb_vsel   	= {1, 2, 0},
        },
        /*[1] = {
                .adapter 	= 0,
                .i2c_bus 	= 1,
                .i2c_addr 	= 0x68,
                .tuner_enable 	= {2, 2, 1},
                .lnb_enable 	= {1, 6, 1},
                .lnb_vsel   	= {1, 2, 0},
        },*/
};

void fe_core_register_frontend(struct dvb_adapter *dvb_adap)
{
	int i = 0;
	int vLoop = 0;

	printk (KERN_INFO "%s: DVB: Spider-Team STV090X frontend core\n", __FUNCTION__);

	core[i] = (struct core*) kmalloc(sizeof(struct core),GFP_KERNEL);
	if (!core[i])
		return;

	memset(core[i], 0, sizeof(struct core));

	core[i]->dvb_adapter = dvb_adap;
	dvb_adap->priv = core[i];

	printk("tuner = %d\n", ARRAY_SIZE(tuner_resources));

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
