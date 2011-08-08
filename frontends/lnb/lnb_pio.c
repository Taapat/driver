/*
 *   lnb_pio.c
 *	 multi platform lnb power supply controller
 *
 *   spider-team 2011.
 *
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/types.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
#include <linux/stpio.h>
#else
#include <linux/stm/pio.h>
#endif

#include "lnb_core.h"

static struct stpio_pin	*lnb_power;
static struct stpio_pin	*lnb_13_18;
static struct stpio_pin	*lnb_14_19;

int lnb_pio_command_kernel(unsigned int cmd, void *arg)
{
	dprintk("[LNB-PIO]: %s (%x)\n", __func__, cmd);

	if((cmd != LNB_VOLTAGE_OFF) && lnb_power)
		stpio_set_pin(lnb_power, 1);

	switch (cmd)
	{
		case LNB_VOLTAGE_OFF:
			dprintk("[LNB-PIO]: set voltage off\n");
			if(lnb_power)
				stpio_set_pin(lnb_power, 0);
			break;

		case LNB_VOLTAGE_VER:
			dprintk("[LNB-PIO]: set voltage vertical\n");
			if(lnb_13_18)
				stpio_set_pin(lnb_13_18, 1);
			break;

		case LNB_VOLTAGE_HOR:
			dprintk("[LNB-PIO]: set voltage horizontal\n");
			if(lnb_13_18)
				stpio_set_pin(lnb_13_18, 0);
			break;
	}

	return 0;
}

int lnb_pio_command(unsigned int cmd, void *arg )
{
	return lnb_pio_command_kernel(cmd, NULL);
}

int lnb_pio_init(void)
{
	lnb_power = NULL;
	lnb_13_18 = NULL;
	lnb_14_19 = NULL;

#if defined(SPARK)
	lnb_power	= stpio_request_pin (6, 5, "lnb_power", 	STPIO_OUT);
	lnb_13_18	= stpio_request_pin (6, 6, "lnb_13/18", 	STPIO_OUT);
	lnb_14_19	= stpio_request_pin (5, 5, "lnb_14/19", 	STPIO_OUT);

	if ((lnb_power == NULL) || (lnb_13_18 == NULL) || (lnb_14_19 == NULL))
	{
		if(lnb_power != NULL)
			stpio_free_pin(lnb_power);
		else
			dprintk("[LNB] error requesting lnb power pin\n");

		if(lnb_13_18 != NULL)
			stpio_free_pin (lnb_13_18);
		else
			dprintk("[LNB] error requesting lnb 13/18 pin\n");

		if(lnb_14_19 != NULL)
			stpio_free_pin(lnb_14_19);
		else
			dprintk("[LNB] error requesting lnb 14/19 pin\n");

		return -EIO;
	}
#endif

	stpio_set_pin(lnb_power, 0); // set power off

	dprintk("[LNB-PIO] init success...\n");
	return 0;
}

int lnb_pio_exit(void)
{
	if(lnb_power != NULL)
		stpio_free_pin(lnb_power);

	if(lnb_13_18 != NULL)
		stpio_free_pin(lnb_13_18);

	if(lnb_14_19 != NULL)
		stpio_free_pin(lnb_14_19);

	return 0;
}
