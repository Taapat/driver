#include "core.h"
#include "stb6100.h"
#include "stb6100_cfg.h"

#include "stv090x.h"
#include <linux/platform_device.h>
#include <asm/system.h>
#include <asm/io.h>
#include <linux/dvb/dmx.h>
#include <linux/proc_fs.h>

#include <pvr_config.h>

struct stpio_pin*	pin_rx_diseq;

short paramDebug = 1;
int bbgain = -1;



static struct core *core[MAX_DVB_ADAPTERS];

static struct stv090x_config tt1600_stv090x_config = {

	.device			= STV0900,
	.demod_mode		= STV090x_DUAL,

	.clk_mode		= STV090x_CLK_EXT,
	.xtal			= 27000000,

	.address		= 0x68,
	.ref_clk		= 16000000,


	.ts1_mode		= STV090x_TSMODE_SERIAL_CONTINUOUS,
	.ts2_mode		= STV090x_TSMODE_SERIAL_CONTINUOUS,

	.ts1_clk		= 0, /* diese regs werden in orig nicht gesetzt */
	.ts2_clk		= 0, /* diese regs werden in orig nicht gesetzt */


	.repeater_level		= STV090x_RPTLEVEL_64,



	.tuner_bbgain = 5,
	.adc1_range	= STV090x_ADC_2Vpp,
	.adc2_range	= STV090x_ADC_2Vpp,

	.diseqc_envelope_mode = false,

	.tuner_set_frequency	= stb6100_set_frequency, 
	.tuner_get_frequency	= stb6100_get_frequency, 
	.tuner_set_bandwidth	= stb6100_set_bandwidth, 
	.tuner_get_bandwidth	= stb6100_get_bandwidth, 
	
};

static struct stb6100_config stb6100_config = {
	.tuner_address		= 0x60, 
	.refclock		= 27000000,
};

static struct stb6100_config stb6100_config_1 = {
	.tuner_address  	= 0x63,
	.refclock		= 27000000,
};

static struct dvb_frontend * frontend_init(struct core_config *cfg, int i)
{
	
	struct dvb_frontend *frontend = NULL;

	printk (KERN_INFO "%s >\n", __FUNCTION__);


	if (i== 0)
		frontend = stv090x_attach(&tt1600_stv090x_config, cfg->i2c_adap, STV090x_DEMODULATOR_0, STV090x_TUNER1);
	else

		frontend = stv090x_attach(&tt1600_stv090x_config, cfg->i2c_adap, STV090x_DEMODULATOR_1, STV090x_TUNER2);
	
	if (frontend) {
		printk("%s: stv0900 attached\n", __FUNCTION__);
	if (i == 0){
				if (dvb_attach(stb6100_attach, frontend, &stb6100_config, cfg->i2c_adap,STB1) == 0) {
					printk (KERN_INFO "error attaching stb6100\n");
					goto error_out;
					}
				}else{
			
				if (dvb_attach(stb6100_attach,frontend, &stb6100_config_1, cfg->i2c_adap,STB2) == 0) {
					printk(KERN_INFO " error attaching stb6100\n");
					goto error_out;
				}
				 }
				
				
					printk("fe_core : stb6100 attached OK \n");
			} else {
				printk (KERN_INFO "%s: error attaching stv0900\n", __FUNCTION__);
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
  struct dvb_frontend *frontend;
  struct core_config *cfg;

  printk ("> (bus = %d) %s\n", tuner_cfg->i2c_bus,__FUNCTION__);

  cfg = kmalloc (sizeof (struct core_config), GFP_KERNEL);
  if (cfg == NULL)
  {
    printk ("stv090x: kmalloc failed\n");
    return NULL;
  }

  /* initialize the config data */
  cfg->tuner_enable_pin = NULL;
  cfg->i2c_adap = i2c_get_adapter (tuner_cfg->i2c_bus);

  printk("i2c adapter = 0x%0x\n", cfg->i2c_adap);

  cfg->i2c_addr = tuner_cfg->i2c_addr;

  printk("i2c addr = %02x\n", cfg->i2c_addr);



  if (cfg->i2c_adap == NULL)
  {
      printk ("[stv090x] failed to allocate resources (i2c adapter)\n");
      goto error;
  }

 


  frontend = frontend_init(cfg, i);

  if (frontend == NULL)
  {
      printk ("[stv090x] frontend init failed!\n");
      goto error;
  }

  printk (KERN_INFO "%s: Call dvb_register_frontend (adapter = 0x%x)\n",
           __FUNCTION__, (unsigned int) adapter);

  if (dvb_register_frontend (adapter, frontend))
  {
    printk ("[stv090x] frontend registration failed !\n");
    if (frontend->ops.release)
      frontend->ops.release (frontend);
    goto error;
  }

  return frontend;

error:
	if(cfg->tuner_enable_pin != NULL)
	{
		printk("[stv090x] freeing tuner enable pin\n");
		stpio_free_pin (cfg->tuner_enable_pin);
	}
	kfree(cfg);
  	return NULL;
}

struct plat_tuner_config tuner_resources[] = {

	[0] = {
		.adapter = 0,
		.i2c_bus = 0,
		.i2c_addr = 0x68,
	},
	[1] = {
		.adapter = 0,
		.i2c_bus = 0,
		.i2c_addr = 0x68,
	},

};
void fe_core_register_frontend(struct dvb_adapter *dvb_adap) 
{
	int i = 0;
	int vLoop = 0;

	printk (KERN_INFO "%s: stv090x DVB: 0.11 \n", __FUNCTION__);
		
		if(pin_rx_diseq==NULL) pin_rx_diseq = stpio_request_pin (5, 4, "pin_rx_diseq", STPIO_IN);
		if(pin_rx_diseq==NULL) {printk("Error DiseqC PIO\n");return;}
		printk("DiseqC OK\n");

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
	printk("stv090x loaded, tuner: stb6100\n");
    return 0;
}



static void __exit fe_core_exit(void)  
{  
   printk("stv090x unloaded\n");
}


module_init             (fe_core_init);
module_exit             (fe_core_exit); 


module_param(paramDebug, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(paramDebug, "Debug Output 0=disabled >0=enabled(debuglevel)");

module_param(bbgain, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(bbgain, "default=-1 (use default config = 10");



MODULE_DESCRIPTION      ("Tunerdriver");
MODULE_AUTHOR           ("Manu Abraham; adapted by TDT mod B4Team");
MODULE_LICENSE          ("GPL");
