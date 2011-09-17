/*
 *   a8293.c -
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
#include <linux/i2c.h>

#include "lnb_core.h"


unsigned char a8293_read(struct i2c_client *client)
{
	unsigned char byte;

	dprintk("[A8293]: %s >\n", __func__);

	byte = 0;

	if (1 != i2c_master_recv(client,&byte,1))
	{
		return -1;
	}

	dprintk("[A8293]: %s <\n", __func__);

	return byte;
}

int a8293_write(struct i2c_client *client, unsigned char reg)
{
	unsigned char byte;

	dprintk("[A8293]: %s >\n", __func__);

	byte = 0;

	if ( 1 != i2c_master_send(client, &reg, 1))
	{
		printk("[A8293]: %s: error sending data\n", __func__);
		return -EFAULT;
	}

	dprintk("[A8293]: %s ok <\n", __func__);

	return 0;
}

int a8293_command_kernel(struct i2c_client *client, unsigned int cmd, void *arg )
{
	unsigned char reg = 0x10;

	dprintk("[A8293]: %s (%x)\n", __func__, cmd);

	if(cmd != LNB_VOLTAGE_OFF)
		reg |= (1<<5);

	switch (cmd)
	{
		case LNB_VOLTAGE_OFF:
			dprintk("[A8293]: set voltage off\n");
			return a8293_write(client, reg);

		case LNB_VOLTAGE_VER:
			dprintk("[A8293]: set voltage vertical\n");
			reg |= 0x04;
			return a8293_write(client, reg);

		case LNB_VOLTAGE_HOR:
			dprintk("[A8293]: set voltage horizontal\n");
			reg |= 0x0B;
			return a8293_write(client, reg);
	}

	return 0;
}

 
int a8293_command(struct i2c_client *client, unsigned int cmd, void *arg)
{
	return a8293_command_kernel(client, cmd, NULL);
}


int a8293_init(struct i2c_client *client)
{
	return a8293_write(client, 0x82);
}

/* ---------------------------------------------------------------------- */
