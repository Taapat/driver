/*
 *   avs_core.c - audio/video switch core driver - Kathrein UFS-910
 *
 *   written by captaintrip - 19.Nov 2007
 *
 *   mainly based on avs_core.c from Gillem gillem@berlios.de / Tuxbox-Project
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>

#include <linux/sched.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <asm/string.h>

#include "avs_core.h"
#include "ak4705.h"
#include "stv6412.h"
#include "cxa2161.h"
#include "fake_avs.h"

enum
{
	AK4705,
	STV6412,
	CXA2161,
	FAKE_AVS
};

static int devType;
static char *type = "ak4705";

/*
 * Addresses to scan
 */
static unsigned short normal_i2c[2] = {0, I2C_CLIENT_END};
I2C_CLIENT_INSMOD;

static struct i2c_driver avs_i2c_driver;
static struct i2c_client client_template = {
	.name = "AVS",
	.flags = 0,
	.driver = &avs_i2c_driver
};

static int client_registered;

/*
 * i2c probe
 */

static int avs_attach(struct i2c_adapter *adap, int addr, int kind)
{
	struct i2c_client *client;

	dprintk("[AVS]: attach\n");

	if (client_registered > 0) {
		dprintk("[AVS]: attach failed\n");
		return -1;
	}

	client_registered++;

	dprintk("[AVS]: chip found @ 0x%x\n",addr);

	if (!(client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL))) {
		dprintk("[AVS]: attach nomem 1\n");
		return -ENOMEM;
	}

	/*
		NB: this code only works for a single client
	*/
	client_template.adapter = adap;
	client_template.addr = addr;
	memcpy(client, &client_template, sizeof(struct i2c_client));

	dprintk("[AVS]: attach final\n");
	i2c_attach_client(client);
	dprintk("[AVS]: attach final ok\n");

	return 0;
}

static int avs_probe(struct i2c_adapter *adap)
{
	int ret = 0;

	dprintk("[AVS]: probe\n");
	ret = i2c_probe(adap, &addr_data, avs_attach);
	dprintk("[AVS]: probe end %d\n",ret);

	return ret;
}

static int avs_detach(struct i2c_client *client)
{
	dprintk("[AVS]: detach\n");
	i2c_detach_client(client);
	if (client)
		kfree(client);

	return 0;
}

/*
 * devfs fops
 */

static int avs_command_ioctl(struct i2c_client *client, unsigned int cmd, void *arg)
{
	int err = 0;

	dprintk("[AVS]: command\n");
	switch(devType)
	{
	case AK4705:
		err = ak4705_command(client, cmd, arg);
		break;
	case STV6412:
		err = stv6412_command(client, cmd, arg);
		break;
	case CXA2161:
		err = cxa2161_command(client, cmd, arg);
		break;
	case FAKE_AVS:
		err = fake_avs_command(client, cmd, arg);
		break;
	}
	return err;
}

int avs_command_kernel(unsigned int cmd, void *arg)
{
	int err = 0;

	dprintk("[AVS]: command_kernel\n");
	switch(devType)
	{
	case AK4705:
		err = ak4705_command_kernel(&client_template, cmd, arg);
		break;
	case STV6412:
		err = stv6412_command_kernel(&client_template, cmd, arg);
		break;
	case CXA2161:
		err = cxa2161_command_kernel(&client_template, cmd, arg);
		break;
	case FAKE_AVS:
		err = fake_avs_command_kernel(&client_template, cmd, arg);
		break;
	}
	return err;
}

EXPORT_SYMBOL(avs_command_kernel);

static int avs_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	dprintk("[AVS]: IOCTL\n");
	return avs_command_ioctl(&client_template, cmd, (void *) arg);
}

static struct file_operations avs_fops = {
	owner:		THIS_MODULE,
	ioctl:		avs_ioctl
};

/*
 * i2c
 */

static struct i2c_driver avs_i2c_driver = {
	.driver = {
		.owner		= THIS_MODULE,
		.name		= "avs driver",
	},
	.id			= I2C_DRIVERID_AVS,
	.attach_adapter	= &avs_probe,
	.detach_client	= &avs_detach,
	.command		= &avs_command_ioctl
};

/*
 * module init/exit
 */

static struct miscdevice avs_dev = {
	.minor = AVS_MINOR,
	.name = "avs",
	.fops = &avs_fops
};

int __init avs_init(void)
{
	int res;

	if((type[0] == 0) || (strcmp("ak4705", type) == 0))
	{
		printk("A/V switch ak4705\n");
		devType = AK4705;
		normal_i2c[0] = 0x22 >> 1;
	}
	else if(strcmp("stv6412", type) == 0)
	{
		printk("A/V switch stv6412\n");
		devType = STV6412;
#if defined(TF7700)
		printk("AVS: TF7700 Handling\n");
		normal_i2c[0] = 0x96 >> 1;
#elif defined(HL101)
		printk("AVS: HL101 Handling\n");
		normal_i2c[0] = 0x96 >> 1;
#elif defined(UFS922)
		printk("AVS: UFS922 Handling\n");
		normal_i2c[0] = 0x4a;
#elif defined(CUBEREVO)
		printk("AVS: CUBEREVO Handling\n");
		normal_i2c[0] = 0x4a;
#elif defined(CUBEREVO_MINI)
		printk("AVS: CUBEREVO_MINI Handling\n");
		normal_i2c[0] = 0x4a;
#elif defined(CUBEREVO_MINI2)
		printk("AVS: CUBEREVO_MINI2 Handling\n");
		normal_i2c[0] = 0x4a;
#elif defined(FORTIS_HDBOX)
		printk("AVS:  FORTIS/HDBOX Handling\n");
		normal_i2c[0] = 0x4b;
#else
		printk("AVS: Unknown AVS Driver!!!!!!!!!!!!!!!!!111\n");
#endif
	}
	else if(strcmp("cxa2161", type) == 0)
	{
                printk("A/V switch cxa2161\n");
                devType = CXA2161;
                normal_i2c[0] = I2C_ADDRESS_CXA2161;
        }
	else if(strcmp("fake_avs", type) == 0)
	{
		printk("A/V switch fake avs\n");
		normal_i2c[0] = 0;
		devType = FAKE_AVS;
	}
	else
	{
		printk(KERN_ERR "Invalid parameter\n");
		return -EINVAL;
	}

	if ((res = i2c_add_driver(&avs_i2c_driver))) {
		dprintk("[AVS]: i2c add driver failed\n");
		return res;
	}

	if (!client_registered){
		printk(KERN_ERR "avs: no client found\n");
		i2c_del_driver(&avs_i2c_driver);
		return -EIO;
	}
	
	switch(devType)
	{
	case AK4705:
		ak4705_init(&client_template);
		break;
	case STV6412:
		stv6412_init(&client_template);
		break;
	case CXA2161:
		cxa2161_init(&client_template);
		break;
	case FAKE_AVS:
		fake_avs_init(&client_template);
		break;
	default:
		return -EINVAL;
	}

	if (misc_register(&avs_dev)<0){
		printk(KERN_ERR "avs: unable to register device\n");
		return -EIO;
	}

	return 0;
}

void __exit avs_exit(void)
{
	i2c_del_driver(&avs_i2c_driver);
	misc_deregister(&avs_dev);
}

module_init(avs_init);
module_exit(avs_exit);

MODULE_AUTHOR("Team Ducktales");
MODULE_DESCRIPTION("Multiplatform A/V scart switch driver");
MODULE_LICENSE("GPL");

module_param(type,charp,0);
MODULE_PARM_DESC(type, "device type (ak4705, stv6412, cxa2161)");

