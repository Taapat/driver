/*
 * @brief platform.c
 *
 * @author konfetti
 *
 * 	Copyright (C) 2011 duckbox
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/dvb/version.h>
#include <linux/dvb/frontend.h>
#include <linux/version.h>

#include "frontend_platform.h"

#include "avl2108_platform.h"
#include "avl2108_reg.h"
#include "avl2108.h"

#include "cx24116_platform.h"

#include "socket.h"
#include "tuner.h"
#include "lnb.h"

#if defined(ATEVIO7500)
struct avl_private_data_s avl_tuner_priv = {
    .ref_freq         = 1,
    .demod_freq       = 11200, /* fixme: the next three could be determined by the pll config!!! */
    .fec_freq         = 16800,
    .mpeg_freq        = 22400,
    .i2c_speed_khz    = TUNER_I2C_CLK,
    .agc_polarization = AGC_POL_INVERT,
    .mpeg_mode        = MPEG_FORMAT_TS_PAR,
    .mpeg_serial      = MPEG_MODE_PARALLEL,
    .mpeg_clk_mode    = MPEG_CLK_MODE_RISING,
    .max_lpf          = 0,
    .pll_config       = 5,
    .usedTuner        = cTUNER_INT_STV6306,
    .usedLNB          = cLNB_PIO,
    .lpf              = 193,
    .lock_mode        = LOCK_MODE_FIXED,
    .iq_swap          = CI_FLAG_IQ_NO_SWAPPED,
    .auto_iq_swap     = CI_FLAG_IQ_AUTO_BIT_AUTO,
    .agc_ref          = 0x30,
    .mpeg_data_clk    = -1,
};

struct platform_frontend_config_s avl2108_frontend = {
            .name               = "avl2108",

            .demod_i2c          = 0x0C,
            .tuner_i2c          = 0xC0,
            .private            = &avl_tuner_priv,
};

struct tunersocket_s atevio7500_socket = {
    .numSockets = 2,
    .socketList = (struct socket_s[]) {
        [0] = {
            .name               = "socket-1",

            .tuner_enable       = {3, 3, 1},
            .lnb                = {2, 6, 0, 2, 5, 1},
            .i2c_bus            = 0,
        },
        [1] = {
            .name               = "socket-2",

            .tuner_enable       = {3, 2, 1},
            .lnb                = {4, 4, 0, 4, 3, 1},
            .i2c_bus            = 1,
        },
    },
};

struct platform_device avl2108_frontend_device = {
    .name    = "avl2108",
    .id      = -1,
    .dev     = {
        .platform_data = &avl2108_frontend,
    },
    .num_resources        = 0,
    .resource             = NULL,
  };

struct platform_device atevio7500_socket_device = {
    .name    = "socket",
    .id      = -1,
    .dev     = {
        .platform_data = &atevio7500_socket,
    },
    .num_resources        = 0,
    .resource             = NULL,
  };

struct platform_device *platform[] __initdata = {
    &avl2108_frontend_device,
    &atevio7500_socket_device,
};

#elif defined(OCTAGON1008)
struct avl_private_data_s avl_tuner_priv = {
    .ref_freq         = 1,
    .demod_freq       = 11200, /* fixme: the next three could be determined by the pll config!!! */
    .fec_freq         = 16800,
    .mpeg_freq        = 22400,
    .i2c_speed_khz    = TUNER_I2C_CLK,
    .agc_polarization = AGC_POL_INVERT,
    .mpeg_mode        = MPEG_FORMAT_TS_PAR,
    .mpeg_serial      = MPEG_MODE_PARALLEL,
    .mpeg_clk_mode    = MPEG_CLK_MODE_RISING,
    .max_lpf          = 320,
    .pll_config       = 5,
    .usedTuner        = cTUNER_INT_STV6306,
    .usedLNB          = cLNB_PIO,
    .lpf              = 340,
    .lock_mode        = LOCK_MODE_ADAPTIVE,
    .iq_swap          = CI_FLAG_IQ_NO_SWAPPED,
    .auto_iq_swap     = CI_FLAG_IQ_AUTO_BIT_AUTO,
    .agc_ref          = 0x0,
    .mpeg_data_clk    = -1,
};

struct platform_frontend_config_s avl2108_frontend = {
            .name             = "avl2108",

            .demod_i2c        = 0x0C,
            .tuner_i2c        = 0xC0,
            .private          = &avl_tuner_priv,
};

struct tunersocket_s octagon1008_socket = {
    .numSockets = 1,
    .socketList = (struct socket_s[]) {
        [0] = {
            .name             = "octagon-1",

            .tuner_enable       = {2, 2, 1},
            .lnb                = {1, 6, 0, 1, 4, 1},
            .i2c_bus            = 0,
        },
    },
};

struct platform_device octagon1008_socket_device = {
    .name    = "socket",
    .id      = -1,
    .dev     = {
        .platform_data = &octagon1008_socket,
    },
    .num_resources        = 0,
    .resource             = NULL,
  };

struct platform_device avl2108_frontend_device = {
    .name    = "avl2108",
    .id      = -1,
    .dev     = {
        .platform_data = &avl2108_frontend,
    },
    .num_resources        = 0,
    .resource             = NULL,
  };

struct platform_device *platform[] __initdata = {
    &avl2108_frontend_device,
    &octagon1008_socket_device,
};

#elif defined(UFS922)
struct avl_private_data_s avl_tuner_priv = {
    .ref_freq         = 1,
    .demod_freq       = 11200, /* fixme: the next three could be determined by the pll config!!! */
    .fec_freq         = 16800,
    .mpeg_freq        = 19200,
    .i2c_speed_khz    = TUNER_I2C_CLK,
    .agc_polarization = AGC_POL_NORMAL,
    .mpeg_mode        = MPEG_FORMAT_TS_PAR,
    .mpeg_serial      = MPEG_MODE_PARALLEL,
    .mpeg_clk_mode    = MPEG_CLK_MODE_RISING,
    .max_lpf          = 340,
    .pll_config       = 3,
    .usedTuner        = cTUNER_EXT_STV6110A,
    .usedLNB          = cLNB_LNBH221,
    .lpf              = 340,
    .lock_mode        = LOCK_MODE_FIXED,
    .iq_swap          = CI_FLAG_IQ_NO_SWAPPED,
    .auto_iq_swap     = CI_FLAG_IQ_AUTO_BIT_AUTO,
    .agc_ref          = 0x0,
    .mpeg_data_clk    = -1,
};

#warning fixxxxxxxxxmeeeeeeeeeeeeee: add cx24116 here

struct platform_frontend_config_s avl2108_frontend = {
            .name             = "avl2108",

            .demod_i2c        = 0x0C,
            .tuner_i2c        = 0xC0,
            .private          = &avl_tuner_priv,
};

struct tunersocket_s ufs922_socket = {
    .numSockets = 2,
    .socketList = (struct socket_s[]) {
        [0] = {
            .name             = "socket-1",

            .tuner_enable       = {2, 4, 1},
            .lnb                = {0, 0x0a, 0x08, 0xd0, 0xd4, 0xdc},
            .i2c_bus            = 0,
        },
        [1] = {
            .name               = "socket-2",

            .tuner_enable       = {2, 5, 1},
            .lnb                = {1, 0x0a, 0x08, 0xd0, 0xd4, 0xdc},
            .i2c_bus            = 1,
        },
    },
};

struct platform_device avl2108_frontend_device = {
    .name    = "avl2108",
    .id      = -1,
    .dev     = {
        .platform_data = &avl2108_frontend,
    },
    .num_resources        = 0,
    .resource             = NULL,
  };

struct platform_device ufs922_socket_device = {
    .name    = "socket",
    .id      = -1,
    .dev     = {
        .platform_data = &ufs922_socket,
    },
    .num_resources        = 0,
    .resource             = NULL,
  };

struct platform_device *platform[] __initdata = {
    &avl2108_frontend_device,
    &ufs922_socket_device,
};
#elif defined (CUBEREVO_MINI2) || defined (CUBEREVO_MINI) || defined (CUBEREVO_250HD) || defined (CUBEREVO_9500HD) || defined (CUBEREVO_2000HD) || defined (CUBEREVO_MINI_FTA)
struct cx24116_private_data_s cx24116_tuner_priv = {
    .useUnknown       = 1,
    .usedLNB          = cLNB_PIO,
    .fw_name          = "dvb-fe-cx24116.fw",
    .fastDelay        = 1,
};

struct platform_frontend_config_s cx24116_frontend = {
            .name               = "cx24116",
            .demod_i2c          = 0x05,
            .tuner_i2c          = 0xff,
            .private            = &cx24116_tuner_priv,
};

struct tunersocket_s cuberevo_socket = {
    .numSockets = 1,
    .socketList = (struct socket_s[]) {
        [0] = {
            .name               = "socket-1",
            .tuner_enable       = {-1, -1, -1},
            .lnb                = {2, 6, 1, 2, 5, 0},
            .i2c_bus            = 0,
        },
    },
};

struct platform_device cx24116_frontend_device = {
    .name    = "cx24116",
    .id      = -1,
    .dev     = {
        .platform_data = &cx24116_frontend,
    },
    .num_resources        = 0,
    .resource             = NULL,
  };

struct platform_device cuberevo_socket_device = {
    .name    = "socket",
    .id      = -1,
    .dev     = {
        .platform_data = &cuberevo_socket,
    },
    .num_resources        = 0,
    .resource             = NULL,
  };

struct platform_device *platform[] __initdata = {
    &cx24116_frontend_device,
//TODO add other fronend's here
    &cuberevo_socket_device,
};
#elif defined (CUBEREVO) || defined (CUBEREVO_9500HD)	
struct cx24116_private_data_s cx24116_tuner_priv = {
    .useUnknown       = 1,
    .usedLNB          = cLNB_PIO,
    .fw_name          = "dvb-fe-cx24116.fw",
    .fastDelay        = 1,
};

struct platform_frontend_config_s cx24116_frontend = {
            .name               = "cx24116",
            .demod_i2c          = 0x05,
            .tuner_i2c          = 0xff,
            .private            = &cx24116_tuner_priv,
};

struct tunersocket_s cuberevo_socket = {
    .numSockets = 2,
    .socketList = (struct socket_s[]) {
        [0] = {
            .name               = "socket-1",
            .tuner_enable       = {-1, -1, -1},
            .lnb                = {2, 6, 1, 2, 5, 0},
            .i2c_bus            = 0,
        },
        [1] = {
            .name               = "socket-2",
            .tuner_enable       = {-1, -1, -1},
            .lnb                = {2, 4, 1, 2, 3, 0},
            .i2c_bus            = 0,
        },
    },
};

struct platform_device cx24116_frontend_device = {
    .name    = "cx24116",
    .id      = -1,
    .dev     = {
        .platform_data = &cx24116_frontend,
    },
    .num_resources        = 0,
    .resource             = NULL,
  };

struct platform_device cuberevo_socket_device = {
    .name    = "socket",
    .id      = -1,
    .dev     = {
        .platform_data = &cuberevo_socket,
    },
    .num_resources        = 0,
    .resource             = NULL,
  };

struct platform_device *platform[] __initdata = {
    &cx24116_frontend_device,
//TODO add other fronend's here
    &cuberevo_socket_device,
};
#else
#warning unsupported arch
#endif

int __init frontend_platform_init(void)
{
    int ret;

    ret = platform_add_devices(platform, sizeof(platform)
                               / sizeof(struct platform_device*));
    if (ret != 0)
    {
        printk("failed to register platform device\n");
    }

    return ret;
}


MODULE_DESCRIPTION("Frontend platform module");

MODULE_AUTHOR("konfetti");
MODULE_LICENSE("GPL");

module_init(frontend_platform_init);
