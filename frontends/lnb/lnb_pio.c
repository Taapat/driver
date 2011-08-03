/*
 *   lnb_pio.c
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

#include <linux/i2c.h>

#include "lnb_core.h"

/* ---------------------------------------------------------------------- */
 
int lnb_pio_command(struct i2c_client *client, unsigned int cmd, void *arg )
{
	return 0;
}

/* ---------------------------------------------------------------------- */
 
int lnb_pio_command_kernel(struct i2c_client *client, unsigned int cmd, void *arg)
{
	return 0;
}

/* ---------------------------------------------------------------------- */

int lnb_pio_init(struct i2c_client *client)
{
	return 0;
}

/* ---------------------------------------------------------------------- */
