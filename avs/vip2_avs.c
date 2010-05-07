/*
 *   vip2_avs.c -
 *
 *   spider-team 2010.
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
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif

#include <linux/i2c.h>

#include "avs_core.h"
#include "vip2_avs.h"

/* hold old values for mute/unmute */
static unsigned char t_mute;

/* hold old values for volume */
static unsigned char t_vol;

/* hold old values for standby */
static unsigned char ft_stnby=0;
static unsigned char fctl=0;

enum scart_ctl {
	SCART_MUTE 		= 0,
	SCART_STANDBY	= 1,
	SCART_WSS		= 2,
	SCART_CVBS_RGB	= 3,
	SCART_TV_SAT	= 4,
	SCART_0_12V		= 5,
	FAN_VCCEN1		= 6,
	HDD_VCCEN2		= 7,
};

/* ---------------------------------------------------------------------- */
static struct stpio_pin*	srclk; // shift clock
static struct stpio_pin*	rclk;  // latch clock
static struct stpio_pin*	sda;   // serial data

#define SRCLK_CLR() {stpio_set_pin(srclk, 0);}
#define SRCLK_SET() {stpio_set_pin(srclk, 1);}

#define RCLK_CLR() {stpio_set_pin(rclk, 0);}
#define RCLK_SET() {stpio_set_pin(rclk, 1);}

#define SDA_CLR() {stpio_set_pin(sda, 0);}
#define SDA_SET() {stpio_set_pin(sda, 1);}

void vip2_avs_hc595_out(unsigned char ctls, int state)
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

    for(i = 7; i >=0; i--)
	{
    	SRCLK_CLR();
		if(fctl & (1<<i))
		{
			SDA_SET();
		}
		else
		{
			SDA_CLR();
		}
		udelay(1);
		SRCLK_SET();
		udelay(1);
	}

    RCLK_CLR();
    udelay(1);
    RCLK_SET();
}

int vip2_avs_src_sel( struct i2c_client *client, int src )
{
	if(src == SAA_SRC_ENC)
		vip2_avs_hc595_out(SCART_TV_SAT, 1);
	else if(src == SAA_SRC_SCART)
		vip2_avs_hc595_out(SCART_TV_SAT, 0);
	
	return 0;
}

/* ---------------------------------------------------------------------- */

inline int vip2_avs_standby( struct i2c_client *client, int type )
{

	if ((type<0) || (type>1))
	{
		return -EINVAL;
	}
 
	if (type==1) 
	{
		if (ft_stnby == 0)
		{
			vip2_avs_hc595_out(SCART_STANDBY, !ft_stnby);
			ft_stnby = 1;
		}
		else
			return -EINVAL;		
	}
	else
	{
		if (ft_stnby == 1)
		{
			vip2_avs_hc595_out(SCART_STANDBY, !ft_stnby);
			ft_stnby = 0;
		}
		else
			return -EINVAL;
	}

	return 0;
}
 
/* ---------------------------------------------------------------------- */
 
int vip2_avs_set_volume( struct i2c_client *client, int vol )
{
	int c=0;
 
	c = vol;
 
	if (c > 63 || c < 0)
		return -EINVAL;
 
	c = 63 - c;
 
	c=c/2;
 
	t_vol = c;
 
	return 0;
}
 
/* ---------------------------------------------------------------------- */
 
inline int vip2_avs_set_mute( struct i2c_client *client, int type )
{

	if ((type<0) || (type>1))
	{
		return -EINVAL;
	}
 
	if (type==AVS_MUTE) 
	{
		t_mute = 1;
	}
	else
	{
		t_mute = 0;
	}

	vip2_avs_hc595_out(SCART_MUTE, t_mute); 
	return 0;
}
 
/* ---------------------------------------------------------------------- */
 
int vip2_avs_get_volume(void)
{
	int c;
 
	c = t_vol;
 
	return c;
}
 
/* ---------------------------------------------------------------------- */
 
inline int vip2_avs_get_mute(void)
{
	return t_mute;
}
 
/* ---------------------------------------------------------------------- */
 
int vip2_avs_set_mode( struct i2c_client *client, int vol )
{
	dprintk("[AVS]: SAAIOSMODE command : %d\n", vol);
	switch(vol)
	{
	case	SAA_MODE_RGB:
		vip2_avs_hc595_out(SCART_CVBS_RGB, 1);
		break;
	case	SAA_MODE_FBAS:
		vip2_avs_hc595_out(SCART_CVBS_RGB, 0);
		break;
	default:
		break;

	}
	// fake avs do not need it
 
	return 0;
}
 
/* ---------------------------------------------------------------------- */
//NOT IMPLEMENTED
int vip2_avs_set_encoder( struct i2c_client *client, int vol )
{
	return 0;
}
 
/* ---------------------------------------------------------------------- */
 
int vip2_avs_set_wss( struct i2c_client *client, int vol )
{
	if (vol == SAA_WSS_43F)
	{
		vip2_avs_hc595_out(SCART_WSS, 0);
	}
	else if (vol == SAA_WSS_169F)
	{
		vip2_avs_hc595_out(SCART_WSS, 1);
	}
	else if (vol == SAA_WSS_OFF)
	{
		vip2_avs_hc595_out(SCART_WSS, 1);
	}
	else
	{
		return  -EINVAL;
	}
 
	return 0;
}

/* ---------------------------------------------------------------------- */

int vip2_avs_command(struct i2c_client *client, unsigned int cmd, void *arg )
{
	int val=0;

	printk("[AVS]: command\n");
	
	if (cmd & AVSIOSET)
	{
		if ( copy_from_user(&val,arg,sizeof(val)) )
		{
			return -EFAULT;
		}

		switch (cmd)
		{
			case AVSIOSVOL:
				return vip2_avs_set_volume(client,val);
			case AVSIOSMUTE:
				return vip2_avs_set_mute(client,val);
			case AVSIOSTANDBY:
				return vip2_avs_standby(client,val);
			default:
				return -EINVAL;
		}
	}
	else if (cmd & AVSIOGET)
	{
		switch (cmd)
		{
			case AVSIOGVOL:
				val = vip2_avs_get_volume();
				break;
			case AVSIOGMUTE:
				val = vip2_avs_get_mute();
				break;
			default:
				return -EINVAL;
		}

		return put_user(val,(int*)arg);
	}
	else
	{
		printk("[AVS]: SAA command\n");

		/* an SAA command */
		if ( copy_from_user(&val,arg,sizeof(val)) )
		{
			return -EFAULT;
		}

		switch(cmd)
		{
		case SAAIOSMODE:
           		 return vip2_avs_set_mode(client,val);
 	        case SAAIOSENC:
        		 return vip2_avs_set_encoder(client,val);
		case SAAIOSWSS:
			return vip2_avs_set_wss(client,val);
		case SAAIOSSRCSEL:
        		return vip2_avs_src_sel(client,val);
		default:
			dprintk("[AVS]: SAA command not supported\n");
			return -EINVAL;
		}
	}

	return 0;
}

/* ---------------------------------------------------------------------- */
 
int vip2_avs_command_kernel(struct i2c_client *client, unsigned int cmd, void *arg)
{
   int val=0;

	dprintk("[AVS]: command_kernel(%u)\n", cmd);
	
	if (cmd & AVSIOSET)
	{
		val = (int) arg;

      		dprintk("[AVS]: AVSIOSET command\n");

		switch (cmd)
		{
			case AVSIOSVOL:
		            return vip2_avs_set_volume(client,val);
		        case AVSIOSMUTE:
		            return vip2_avs_set_mute(client,val);
		        case AVSIOSTANDBY:
		            return vip2_avs_standby(client,val);
			default:
				return -EINVAL;
		}
	} else if (cmd & AVSIOGET)
	{
		dprintk("[AVS]: AVSIOGET command\n");

		switch (cmd)
		{
			case AVSIOGVOL:
			    val = vip2_avs_get_volume();
			    break;
			case AVSIOGMUTE:
			    val = vip2_avs_get_mute();
			    break;
			default:
				return -EINVAL;
		}

		arg = (void *) val;
	        return 0;
	}
	else
	{
		printk("[AVS]: SAA command (%d)\n", (int)arg);

		val = (int) arg;

		switch(cmd)
		{
		case SAAIOSMODE:
           		 return vip2_avs_set_mode(client,val);
 	        case SAAIOSENC:
        		 return vip2_avs_set_encoder(client,val);
		case SAAIOSWSS:
			return vip2_avs_set_wss(client,val);
		case SAAIOSSRCSEL:
        		return vip2_avs_src_sel(client,val);
		default:
			dprintk("[AVS]: SAA command not supported\n");
			return -EINVAL;
		}
	}

	return 0;
}

/* ---------------------------------------------------------------------- */

int vip2_avs_init(struct i2c_client *client)
{
  srclk= stpio_request_pin (2, 5, "AVS_HC595_SRCLK", STPIO_OUT);
  rclk = stpio_request_pin (2, 6, "AVS_HC595_RCLK", STPIO_OUT);
  sda  = stpio_request_pin (2, 7, "AVS_HC595_SDA", STPIO_OUT);

  if ((srclk == NULL) || (rclk == NULL) || sda == NULL)
  {
	if(srclk != NULL)
		stpio_free_pin (srclk);
   	if(rclk != NULL)
		stpio_free_pin (rclk);
	if(sda != NULL)
		stpio_free_pin(sda);
	return -1;
  }

//	vip2_avs_hc595_out (SCART_WSS, 1);
//	vip2_avs_hc595_out (SCART_STANDBY, 1);
	vip2_avs_hc595_out(SCART_TV_SAT, 1); // to enc

  return 0;
}

/* ---------------------------------------------------------------------- */
