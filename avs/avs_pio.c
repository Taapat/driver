/*
 *   avs_pio.c -
 *
 *   spider-team 2011.
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
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
#include <linux/stpio.h>
#else
#include <linux/stm/pio.h>
#endif

#include "avs_core.h"
#include "avs_pio.h"

/* hold old values for mute/unmute */
static unsigned char t_mute;

/* hold old values for volume */
static unsigned char t_vol;

/* hold old values for standby */
static unsigned char ft_stnby=0;

static struct stpio_pin*	scart_cvbs_rgb;
static struct stpio_pin*	scart_169_43;
static struct stpio_pin*	scart_standby;
static struct stpio_pin*	scart_tv_sat;
static struct stpio_pin*	scart_mute;

int avs_pio_src_sel(int src)
{
	return 0;
}

inline int avs_pio_standby(int type)
{
	if ((type<0) || (type>1))
	{
		return -EINVAL;
	}

	if (type==1)
	{
		if (ft_stnby == 0)
		{
#if defined(HS7810A)
			stpio_set_pin(scart_169_43, 0);
			stpio_set_pin(scart_standby, 1);
			stpio_set_pin(scart_cvbs_rgb, 0);
#else
			stpio_set_pin(scart_standby, !ft_stnby);
#endif
			ft_stnby = 1;
		}
		else
			return -EINVAL;
	}
	else
	{
		if (ft_stnby == 1)
		{
#if defined(HS7810A)
			stpio_set_pin(scart_169_43, 0);
			stpio_set_pin(scart_standby, 0);
			stpio_set_pin(scart_cvbs_rgb, 0);
#else
			stpio_set_pin(scart_standby, !ft_stnby);
#endif
			ft_stnby = 0;
		}
		else
			return -EINVAL;
	}

	return 0;
}

int avs_pio_set_volume(int vol)
{
	int c=0;

	dprintk("[AVS]: %s (%d)\n", __func__, vol);
	c = vol;

	if (c > 63 || c < 0)
		return -EINVAL;

	c = 63 - c;

	c=c/2;

	t_vol = c;

	return 0;
}

inline int avs_pio_set_mute(int type)
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


#if defined(SPARK) || defined(SPARK7162)
	stpio_set_pin(scart_mute, t_mute);
#endif

	return 0;
}

int avs_pio_get_volume(void)
{
	int c;

	c = t_vol;

	return c;
}

inline int avs_pio_get_mute(void)
{
	return t_mute;
}

int avs_pio_set_mode(int val)
{
	switch(val)
	{
	case SAA_MODE_RGB:
		stpio_set_pin(scart_cvbs_rgb, 1);
		break;
	case SAA_MODE_FBAS:
		stpio_set_pin(scart_cvbs_rgb, 0);
		break;
	}

	return 0;
}

int avs_pio_set_encoder(int vol)
{
	return 0;
}

int avs_pio_set_wss(int val)
{
	dprintk("[AVS]: %s\n", __func__);

	if (val == SAA_WSS_43F)
	{
#if defined(HS7810A)
		stpio_set_pin(scart_standby, 0);
#endif
		stpio_set_pin(scart_169_43, 0);
	}
	else if (val == SAA_WSS_169F)
	{
#if defined(HS7810A)
		stpio_set_pin(scart_standby, 0);
#endif
		stpio_set_pin(scart_169_43, 1);
	}
	else if (val == SAA_WSS_OFF)
	{
#if !defined(HS7810A)
		stpio_set_pin(scart_169_43, 1);
#endif
	}
	else
	{
		return  -EINVAL;
	}

	return 0;
}

int avs_pio_command(unsigned int cmd, void *arg )
{
	int val=0;

	dprintk("[AVS]: %s(%d)\n", __func__, cmd);

	if (cmd & AVSIOSET)
	{
		if ( copy_from_user(&val,arg,sizeof(val)) )
		{
			return -EFAULT;
		}

		switch (cmd)
		{
			case AVSIOSVOL:
				return avs_pio_set_volume(val);
			case AVSIOSMUTE:
				return avs_pio_set_mute(val);
			case AVSIOSTANDBY:
				return avs_pio_standby(val);
			default:
				return -EINVAL;
		}
	}
	else if (cmd & AVSIOGET)
	{
		switch (cmd)
		{
			case AVSIOGVOL:
				val = avs_pio_get_volume();
				break;
			case AVSIOGMUTE:
				val = avs_pio_get_mute();
				break;
			default:
				return -EINVAL;
		}

		return put_user(val,(int*)arg);
	}
	else
	{
		dprintk("[AVS]: %s: SAA command\n", __func__);

		/* an SAA command */
		if ( copy_from_user(&val,arg,sizeof(val)) )
		{
			return -EFAULT;
		}

		switch(cmd)
		{
		case SAAIOSMODE:
           		 return avs_pio_set_mode(val);
 	        case SAAIOSENC:
        		 return avs_pio_set_encoder(val);
		case SAAIOSWSS:
			return avs_pio_set_wss(val);
		case SAAIOSSRCSEL:
        		return avs_pio_src_sel(val);
		default:
			dprintk("[AVS]: %s: SAA command not supported\n", __func__);
			return -EINVAL;
		}
	}

	return 0;
}

int avs_pio_command_kernel(unsigned int cmd, void *arg)
{
    int val=0;

	if (cmd & AVSIOSET)
	{
		val = (int) arg;

      	dprintk("[AVS]: %s: AVSIOSET command\n", __func__);

		switch (cmd)
		{
			case AVSIOSVOL:
		            return avs_pio_set_volume(val);
		        case AVSIOSMUTE:
		            return avs_pio_set_mute(val);
		        case AVSIOSTANDBY:
		            return avs_pio_standby(val);
			default:
				return -EINVAL;
		}
	}
	else if (cmd & AVSIOGET)
	{
		dprintk("[AVS]: %s: AVSIOGET command\n", __func__);

		switch (cmd)
		{
			case AVSIOGVOL:
			    val = avs_pio_get_volume();
			    break;
			case AVSIOGMUTE:
			    val = avs_pio_get_mute();
			    break;
			default:
				return -EINVAL;
		}

		arg = (void *) val;
	        return 0;
	}
	else
	{
		dprintk("[AVS]: %s: SAA command (%d)\n", __func__, cmd);

		val = (int) arg;

		switch(cmd)
		{
		case SAAIOSMODE:
           		 return avs_pio_set_mode(val);
 	        case SAAIOSENC:
        		 return avs_pio_set_encoder(val);
		case SAAIOSWSS:
			return avs_pio_set_wss(val);
		case SAAIOSSRCSEL:
        		return avs_pio_src_sel(val);
		default:
			dprintk("[AVS]: %s: SAA command not supported\n", __func__);
			return -EINVAL;
		}
	}

	return 0;
}

int avs_pio_init(void)
{
#if defined(SPARK7162)
	scart_cvbs_rgb	= stpio_request_pin (11, 5, "scart_cvbs_rgb", STPIO_OUT);
	scart_169_43	= stpio_request_pin (11, 4, "scart_169_43", STPIO_OUT);
	scart_mute		= stpio_request_pin (11, 2, "scart_mute", STPIO_OUT);
	scart_standby	= stpio_request_pin (11, 3, "scart_standby", STPIO_OUT);
#elif defined(SPARK)
	scart_cvbs_rgb	= stpio_request_pin (6, 0, "scart_cvbs_rgb", STPIO_OUT);
	scart_169_43	= stpio_request_pin (6, 2, "scart_169_43", STPIO_OUT);
	scart_mute	    = stpio_request_pin (2, 4, "scart_mute", STPIO_OUT);
	scart_standby	= stpio_request_pin (6, 1, "scart_standby", STPIO_OUT);
#elif defined(HS7810A)
	scart_169_43	= stpio_request_pin (6, 5, "avs0", STPIO_OUT);
	scart_standby	= stpio_request_pin (6, 6, "avs1", STPIO_OUT);
	scart_cvbs_rgb	= stpio_request_pin (6, 4, "avs2", STPIO_OUT);
	scart_mute	    = NULL;
#else
	return 0;
#endif


	if ((scart_cvbs_rgb == NULL) || (scart_169_43 == NULL) || (scart_standby == NULL))
	{
		if(scart_cvbs_rgb != NULL)
			stpio_free_pin(scart_cvbs_rgb);
		else
			dprintk("[AVS]: scart_cvbs_rgb error\n");
		if(scart_169_43 != NULL)
			stpio_free_pin (scart_169_43);
		else
			dprintk("[AVS]: scart_169_43 error\n");

		if(scart_standby != NULL)
			stpio_free_pin(scart_standby);
		else
			dprintk("[AVS]: scart_standby error\n");

		return -1;
	}

#if defined(SPARK) || defined(SPARK7162)
	if (scart_mute == NULL)
	{
		dprintk("[AVS]: scart_mute error\n");
		return -1;
	}
#endif

	printk("[AVS]: init success\n");

  return 0;
}

int avs_pio_exit(void)
{
	if(scart_cvbs_rgb != NULL)
		stpio_free_pin(scart_cvbs_rgb);

	if(scart_169_43 != NULL)
		stpio_free_pin (scart_169_43);

	if(scart_standby != NULL)
		stpio_free_pin(scart_standby);

	if(scart_mute != NULL)
		stpio_free_pin(scart_mute);

  return 0;
}
