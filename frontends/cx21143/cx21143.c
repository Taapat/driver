/*
    Conexant cx21143 - DVBS/S2 Satellite demod/tuner driver

    based on cx24116 driver by
    Copyright (C) 2006 Steven Toth <stoth@hauppauge.com>
    Copyright (C) 2006 Georg Acher (acher (at) baycom (dot) de) for Reel Multimedia
                       Added Diseqc, auto pilot tuning and hack for old DVB-API
    Copyright (C) 2008 konfetti
    Copyright (C) 2009 Dagobert: adapted for ufs922: The name in the logfiles is
     cx21143 but I cant find such a driver on the conexant side.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#ifdef STLINUX20
#include <linux/platform.h>
#else
#include <linux/platform_device.h>
#endif
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include <linux/firmware.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif
#include <asm/system.h>
#include <asm/io.h>
#include <linux/dvb/dmx.h>
#include <linux/proc_fs.h>
#include "cx21143.h"

/* FIXME: to be replaced with the <linux/pvr_config.h> */
#include <pvr_config.h>

#define cx21143_DEFAULT_FIRMWARE "dvb-fe-cx21143.fw"
#define cx21143_SEARCH_RANGE_KHZ 5000
#define cMaxError 5

static short paramDebug = 0;
#define TAGDEBUG "[cx21143] "

#define dprintk(level, x...) do { \
if ((paramDebug) && (paramDebug > level)) printk(TAGDEBUG x); \
} while (0)

// Fast i2c delay (1ms ~ 1MHz) is only used to speed up the firmware
// download. All other read/write operations are executed with the default
// i2c bus speed (100kHz ~ 10ms).
#define I2C_FAST_DELAY 1

/* known registers */
#define cx21143_REG_QUALITY (0xd5)
#define cx21143_REG_FECSTATUS (0x9c)
#define cx21143_REG_STATUS  (0x9d)  /* signal high : status */
#define cx21143_REG_SIGNAL  (0x9e)  /* signal low           */
#define cx21143_REG_QSTATUS (0xbc)

#define cx21143_REG_BER0    (0xc6)
#define cx21143_REG_BER8    (0xc7)
#define cx21143_REG_BER16   (0xc8)
#define cx21143_REG_BER24   (0xc9)

#define cx21143_REG_UCB0    (0xcb)
#define cx21143_REG_UCB8    (0xca)

#define cx21143_FEC_FECMASK   (0x1f)

/* arg buffer size */
#define cx21143_ARGLEN (0x1e)

/* DiSEqC strategy (0=HACK, 1=CACHE) */
static int dsec = 0;

//0 = OFF
//1 = ON
//2 = AUTO
static int pilot = 0;

/* arg offset for DiSEqC */
#define cx21143_DISEQC_BURST  (1)
#define cx21143_DISEQC_ARG2_2 (2) /* unknown value=2 */
#define cx21143_DISEQC_ARG3_0 (3) /* unknown value=0 */
#define cx21143_DISEQC_ARG4_0 (4) /* unknown value=0 */
#define cx21143_DISEQC_MSGLEN (5)
#define cx21143_DISEQC_MSGOFS (6)

/* DiSEqC burst */
#define cx21143_DISEQC_MINI_A (0)
#define cx21143_DISEQC_MINI_B (1)

static int
cx21143_writereg_lnb_supply (struct cx21143_state *state, char data);

static int
cx21143_writeN (struct cx21143_state *state, char data);

static int
cx21143_readreg (struct cx21143_state *state, u8 reg);

struct firmware_cmd
{
  enum cmds id;                 /* Unique firmware command */
  int len;                      /* Commands args len + id byte */
  char *name;
}

cx21143_COMMANDS[] =
{
  { CMD_SET_VCO, 0x0A, "CMD_SET_VCO"},
  { CMD_TUNEREQUEST, 0x13, "CMD_TUNEREQUEST"},
  { CMD_MPEGCONFIG, 0x06, "CMD_MPEGCONFIG"},
  { CMD_TUNERINIT, 0x03, "CMD_TUNERINIT"},
  { CMD_BANDWIDTH, 0x02, "CMD_BANDWIDTH"},
  { CMD_LNBCONFIG, 0x08, "CMD_LNBCONFIG"},
  { CMD_LNBSEND, 0x0c, "CMD_LNBSEND"},
  { CMD_SET_TONEPRE, 0x02, "CMD_SET_TONEPRE"},
  { CMD_SET_TONE, 0x04, "CMD_SET_TONE"},
  { CMD_TUNERSLEEP, 0x02, "CMD_TUNERSLEEP"},
  { CMD_U1, 0x01, "CMD_U1"},
  { CMD_U2, 0x0A, "CMD_U2"},
  { CMD_GETAGC, 0x01, "CMD_GETAGC"},
  { CMD_MAX, 0x00, "CMD_MAX"}
};


/* A table of modulation, fec and configuration bytes for the demod.
 * Not all S2 mmodulation schemes are support and not all rates with
 * a scheme are support. Especially, no auto detect when in S2 mode.
 */
struct cx21143_modfec
{
  enum dvbfe_delsys delsys;
  enum dvbfe_modulation modulation;
  enum dvbfe_fec fec;
  u8 mask;                      /* In DVBS mode this is used to autodetect */
  u8 val;                       /* Passed to the firmware to indicate mode selection */
} cx21143_MODFEC_MODES[] =
{
  /* QPSK. For unknown rates we set to hardware to to detect 0xfe 0x30 */
  { DVBFE_DELSYS_DVBS, DVBFE_MOD_QPSK, DVBFE_FEC_NONE, 0xfe, 0x30},
  { DVBFE_DELSYS_DVBS, DVBFE_MOD_QPSK, DVBFE_FEC_1_2, 0x02, 0x2e},
  { DVBFE_DELSYS_DVBS, DVBFE_MOD_QPSK, DVBFE_FEC_2_3, 0x04, 0x2f},
  { DVBFE_DELSYS_DVBS, DVBFE_MOD_QPSK, DVBFE_FEC_3_4, 0x00, 0x30},
  { DVBFE_DELSYS_DVBS, DVBFE_MOD_QPSK, DVBFE_FEC_4_5, 0xfe, 0x30},
  { DVBFE_DELSYS_DVBS, DVBFE_MOD_QPSK, DVBFE_FEC_5_6, 0x00, 0x31},
  { DVBFE_DELSYS_DVBS, DVBFE_MOD_QPSK, DVBFE_FEC_6_7, 0xfe, 0x30},
  { DVBFE_DELSYS_DVBS, DVBFE_MOD_QPSK, DVBFE_FEC_7_8, 0x00, 0x32},
  { DVBFE_DELSYS_DVBS, DVBFE_MOD_QPSK, DVBFE_FEC_8_9, 0xfe, 0x30},
  { DVBFE_DELSYS_DVBS, DVBFE_MOD_QPSK, DVBFE_FEC_AUTO, 0xfe, /*0x2e*/ 0x30},
    /* NBC-QPSK */
  { DVBFE_DELSYS_DVBS2, DVBFE_MOD_QPSK, DVBFE_FEC_1_2, 0x00, 0x04},
  { DVBFE_DELSYS_DVBS2, DVBFE_MOD_QPSK, DVBFE_FEC_3_5, 0x00, 0x05},
  { DVBFE_DELSYS_DVBS2, DVBFE_MOD_QPSK, DVBFE_FEC_2_3, 0x00, 0x06},
  { DVBFE_DELSYS_DVBS2, DVBFE_MOD_QPSK, DVBFE_FEC_3_4, 0x00, 0x07},
  { DVBFE_DELSYS_DVBS2, DVBFE_MOD_QPSK, DVBFE_FEC_4_5, 0x00, 0x08},
  { DVBFE_DELSYS_DVBS2, DVBFE_MOD_QPSK, DVBFE_FEC_5_6, 0x00, 0x09},
  { DVBFE_DELSYS_DVBS2, DVBFE_MOD_QPSK, DVBFE_FEC_8_9, 0x00, 0x0a},
  { DVBFE_DELSYS_DVBS2, DVBFE_MOD_QPSK, DVBFE_FEC_9_10, 0x00, 0x0b},
    /* 8PSK */
  { DVBFE_DELSYS_DVBS2, DVBFE_MOD_8PSK, DVBFE_FEC_3_5, 0x00, 0x0c},
  { DVBFE_DELSYS_DVBS2, DVBFE_MOD_8PSK, DVBFE_FEC_2_3, 0x00, 0x0d},
  { DVBFE_DELSYS_DVBS2, DVBFE_MOD_8PSK, DVBFE_FEC_3_4, 0x00, 0x0e},
  { DVBFE_DELSYS_DVBS2, DVBFE_MOD_8PSK, DVBFE_FEC_5_6, 0x00, 0x0f},
  { DVBFE_DELSYS_DVBS2, DVBFE_MOD_8PSK, DVBFE_FEC_8_9, 0x00, 0x10},
  { DVBFE_DELSYS_DVBS2, DVBFE_MOD_8PSK, DVBFE_FEC_9_10, 0x00, /*0x11*/ 0x0b},
  { DVBFE_DELSYS_DVBS2, DVBFE_MOD_8PSK, DVBFE_FEC_AUTO, 0x00, 0x0b},};

struct cx21143_U2
{
  u8 U2_1;
  u8 U2_2;
  u8 U2_3;
  u8 U2_4;
  u8 U2_5;
  u8 U2_6;
  u8 U2_7;
  u8 U2_8;
  u8 U2_9;
} cx21143_U2_TABLE[] =
{
  /* fec none */ { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
  /* fec none */ { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
  /* 1/2      */ { 0x10, 0x01, 0x2a, 0x08, 0x01, 0x2a, 0x08, 0x01, 0x00}, //ok
  /* 1/2      */ { 0x10, 0x01, 0x2a, 0x08, 0x01, 0x2a, 0x08, 0x01, 0x00}, //ok
  /* 2/3      */ { 0x10, 0x02, 0x54, 0x10, 0x01, 0xbf, 0x0c, 0x01, 0x55}, //ok
  /* 2/3      */ { 0x10, 0x02, 0x54, 0x10, 0x01, 0xbf, 0x0c, 0x01, 0x55}, //ok
  /* 3/4      */ { 0x10, 0x03, 0x7e, 0x18, 0x02, 0x54, 0x10, 0x01, 0x80}, //ok
  /* 3/4      */ { 0x10, 0x03, 0x7e, 0x18, 0x02, 0x54, 0x10, 0x01, 0x80}, //ok
  /* 4/5      */ { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //unknown
  /* 4/5      */ { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //unknown
  /* 5/6      */ { 0x10, 0x05, 0xd2, 0x28, 0x03, 0x7e, 0x18, 0x01, 0xaa}, //ok
  /* 5/6      */ { 0x10, 0x05, 0xd2, 0x28, 0x03, 0x7e, 0x18, 0x01, 0xaa}, //ok
  /* 6/7      */ { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //unknown
  /* 6/7      */ { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //unknown
  /* 7/8      */ { 0x10, 0x08, 0x26, 0x38, 0x04, 0xa8, 0x20, 0x01, 0xc0}, //ok
  /* 7/8      */ { 0x10, 0x08, 0x26, 0x38, 0x04, 0xa8, 0x20, 0x01, 0xc0}, //ok
  /* 8/9      */ { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //unknown
  /* 8/9      */ { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //unknown
  /* fec auto */ { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //ok
  /* fec auto */ { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //ok
    /* NBC-QPSK */
  /* 1/2      */ { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //unknown
  /* 1/2      */ { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //unknown
  /* 3/5      */ { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //unknown
  /* 3/5      */ { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //unknown
  /* 2/3      */ { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //unknown
  /* 2/3      */ { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //unknown
  /* 3/4      */ { 0x10, 0x06, 0x44, 0xa4, 0x03, 0xe2, 0x29, 0x01, 0x9d}, //ok
  /* 3/4      */ { 0x10, 0x06, 0x44, 0xa4, 0x03, 0xe2, 0x29, 0x01, 0x9d}, //ok
  /* 4/5      */ { 0x10, 0x06, 0xb0, 0x38, 0x03, 0xfa, 0x65, 0x01, 0xae}, //ok
  /* 4/5      */ { 0x10, 0x06, 0xb0, 0x38, 0x03, 0xfa, 0x65, 0x01, 0xae}, //ok
  /* 5/6      */ { 0x10, 0x00, 0x77, 0x00, 0x00, 0x42, 0x47, 0x01, 0xcb}, //ok->waiting on feedback
  /* 5/6      */ { 0x10, 0x00, 0x77, 0x00, 0x00, 0x42, 0x47, 0x01, 0xcb}, //ok->waiting on feedback
  /* 8/9      */ { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //unknown
  /* 8/9      */ { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //unknown
  /* 9/10     */ { 0x10, 0x07, 0x89, 0x80, 0x03, 0xe2, 0x29, 0x01, 0xf0}, //ok
  /* 9/10     */ { 0x10, 0x07, 0x89, 0x80, 0x03, 0xe2, 0x29, 0x01, 0xf0}, //ok
    /* 8PSK */
  /* 3/5      */ { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //unknown
  /* 3/5      */ { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //unknown
  /* 2/3      */ { 0x10, 0x00, 0x5f, 0x18, 0x00, 0x2c, 0x3f, 0x02, 0x26}, //0x00 0x05 0x98 0x00 0x02 0xd3 pilot off
  /* 2/3      */ { 0x10, 0x01, 0xdb, 0x78, 0x00, 0xe2, 0x5f, 0x02, 0x19}, //0x00 0x1b 0xf8 0x00 0x0e 0x73 pilot on
  /* 3/4      */ { 0x10, 0x06, 0x44, 0xa4, 0x02, 0x97, 0xb1, 0x02, 0x6a}, //ok
  /* 3/4      */ { 0x10, 0x06, 0x44, 0xa4, 0x02, 0x97, 0xb1, 0x02, 0x6a}, //ok
  /* 5/6      */ { 0x10, 0x00, 0x77, 0x00, 0x00, 0x2c, 0x3f, 0x02, 0xb0}, //0x00 0x07 0x00 0x00 0x02 0xd3 pilot off
  /* 5/6      */ { 0x10, 0x02, 0x53, 0x00, 0x00, 0xe2, 0x5f, 0x02, 0xa0}, //ok
  /* 8/9      */ { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //unknown
  /* 8/9      */ { 0x10, 0x07, 0x71, 0x98, 0x02, 0xa7, 0x1d, 0x02, 0xce}, //0x00 0x70 0x18 0x00 0x2b 0x59 pilot on
  /* 9/10     */ { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //unknown
  /* 9/10     */ { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //unknown
  };



static struct dvbfe_info dvbs_info = {
  .name = "Conexant cx21143 DVB-S",
  .delivery = DVBFE_DELSYS_DVBS,
  .delsys = {
             .dvbs.modulation = DVBFE_MOD_QPSK,
             .dvbs.fec = DVBFE_FEC_1_2 | DVBFE_FEC_2_3 |
             DVBFE_FEC_3_4 | DVBFE_FEC_4_5 |
             DVBFE_FEC_5_6 | DVBFE_FEC_6_7 | DVBFE_FEC_7_8 | DVBFE_FEC_AUTO},

  .frequency_min = 950000,
  .frequency_max = 2150000,
  .frequency_step = 1011,
  .frequency_tolerance = 5000,
  .symbol_rate_min = 1000000,
  .symbol_rate_max = 45000000
};

static const struct dvbfe_info dvbs2_info = {
  .name = "Conexant cx21143 DVB-S2",
  .delivery = DVBFE_DELSYS_DVBS2,
  .delsys = {
             .dvbs2.modulation = DVBFE_MOD_QPSK | DVBFE_MOD_8PSK,

             /* TODO: Review these */
             .dvbs2.fec = DVBFE_FEC_1_4 | DVBFE_FEC_1_3 |
             DVBFE_FEC_2_5 | DVBFE_FEC_1_2 |
             DVBFE_FEC_3_5 | DVBFE_FEC_2_3 |
             DVBFE_FEC_3_4 | DVBFE_FEC_4_5 |
             DVBFE_FEC_5_6 | DVBFE_FEC_8_9 | DVBFE_FEC_9_10,

             },

  .frequency_min = 950000,
  .frequency_max = 2150000,
  .frequency_step = 0,
  .symbol_rate_min = 1000000,
  .symbol_rate_max = 45000000,
  .symbol_rate_tolerance = 0
};

struct config_entry
{
  int count;
  struct plat_tuner_config tuner_cfg[MAX_TUNERS_PER_ADAPTER];
};

static struct config_entry config_data[MAX_DVB_ADAPTERS];

static struct cx21143_core *core[MAX_DVB_ADAPTERS];

static int
cx21143_lookup_fecmod (struct cx21143_state *state,
                      enum dvbfe_delsys d, enum dvbfe_modulation m,
                      enum dvbfe_fec f)
{
  unsigned int i;
  int ret = -EOPNOTSUPP;

  dprintk (10, "delsys %d, modulation %d, fec = %d\n", d, m, f);

  for (i = 0;
       i < sizeof (cx21143_MODFEC_MODES) / sizeof (struct cx21143_modfec); i++)
  {
    if ((d == cx21143_MODFEC_MODES[i].delsys) &&
        (m == cx21143_MODFEC_MODES[i].modulation) &&
        (f == cx21143_MODFEC_MODES[i].fec))
    {
      ret = i;
      break;
    }
  }

  dprintk (10, "%s: ret = %d\n", __FUNCTION__, ret);
  return ret;

}

/* ***************************
 * Request PIO Pins and reset 
 */
static int
cx21143_reset (struct cx21143_state *state)
{
  const struct cx21143_config *cfg = state->config;

  dprintk (10, "%s: > \n", __FUNCTION__);

  if (cfg->tuner_enable_pin != NULL)
  {
    stpio_set_pin (cfg->tuner_enable_pin, !cfg->tuner_enable_act);
    stpio_set_pin (cfg->tuner_enable_pin, cfg->tuner_enable_act);

    /* first reset then read is much better ;) */
    cx21143_writeN (state, 0x00);
    dprintk(1,"reading 0x%x\n", cx21143_readreg (state, 0x00));
  }

  /* set by default to horizontal */
  cx21143_writereg_lnb_supply(state, 0xdc);

  dprintk (10, "%s: < \n", __FUNCTION__);

  return 0;
}

/* ********************************
 *
 */
static int
cx21143_writereg (struct cx21143_state *state, int reg, int data)
{
  u8 buf[] = { reg, data };
  struct i2c_msg msg = {.addr = state->config->i2c_addr,
    .flags = 0,.buf = buf,.len = 2
  };
  int err = 0;

  dprintk (100, "cx21143: %s:  write reg 0x%02x, value 0x%02x\n", __FUNCTION__, reg,
           data);

  if ((err = i2c_transfer (state->config->i2c_adap, &msg, 1)) != 1)
  {
    printk ("%s: writereg error(err == %i, reg == 0x%02x,"
            " data == 0x%02x)\n", __FUNCTION__, err, reg, data);
    err = -EREMOTEIO;
  }

  return err;
}

/* 
 * LNBH221 write function:
 * i2c bus 0 and 1, addr 0x08 
 * seen values: 0xd4 (vertical) 0xdc (horizontal), 0xd0 shutdown
 * 0xd8 ??? fixme ->was genau heisst das, nochmal lesen besonders
 * mit den "is controlled by dsqin pin" ...
 *
 * From Documentation (can be downloaded on st.com):

PCL TTX TEN LLC VSEL EN OTF OLF Function
             0    0   1  X    X V OUT=13.25V, VUP=15.25V
             0    1   1  X    X VOUT=18V, VUP=20V
             1    0   1  X    X VOUT=14.25V, VUP=16.25V
             1    1   1  X    X VOUT=19.5V, VUP=21.5V
          0           1  X    X 22KHz tone is controlled by DSQIN pin
          1           1  X    X 22KHz tone is ON, DSQIN pin disabled
       0              1  X    X VORX output is ON, output voltage controlled by VSEL and LLC
       1              1  X    X VOTX output is ON, 22KHz controlled by DSQIN or TEN,
                                output voltage level controlled by VSEL and LLC
  0                   1  X    X Pulsed (dynamic) current limiting is selected
  1                   1  X    X Static current limiting is selected
  X    X   X X    X   0  X    X Power blocks disabled
 
 * 
 */
static int
cx21143_writereg_lnb_supply (struct cx21143_state *state, char data)
{
  int ret = -EREMOTEIO;
  struct i2c_msg msg;
  u8 buf;

  buf = data;

  msg.addr = state->config->i2c_addr_lnb_supply;
  msg.flags = 0;
  msg.buf = &buf;
  msg.len = 1;

  dprintk (100, "cx21143: %s:  write 0x%02x to 0x08\n", __FUNCTION__, data);

  if ((ret = i2c_transfer (state->config->i2c_adap, &msg, 1)) != 1)
  {
    //wohl nicht LNBH23, mal mit LNBH221 versuchen
    msg.addr = 0x08;
    msg.flags = 0;
    msg.buf = &buf;
    msg.len = 1;
    if ((ret = i2c_transfer (state->config->i2c_adap, &msg, 1)) != 1)
    {
      printk ("%s: writereg error(err == %i)\n",
              __FUNCTION__, ret);
      ret = -EREMOTEIO;
    }
  }

  return ret;
}

static int
cx21143_writeN (struct cx21143_state *state, char data)
{
  int ret = -EREMOTEIO;
  struct i2c_msg msg;
  u8 buf;

  buf = data;

  msg.addr = state->config->i2c_addr;
  msg.flags = 0;
  msg.buf = &buf;
  msg.len = 1;

  dprintk (100, "cx21143: %s:  write 0x%02x to 0x08\n", __FUNCTION__, data);

  if ((ret = i2c_transfer (state->config->i2c_adap, &msg, 1)) != 1)
  {
    printk ("%s: writereg error(err == %i)\n",
            __FUNCTION__, ret);
    ret = -EREMOTEIO;
  }

  return ret;
}

/* *******************************************
 *  Bulk byte writes to a single I2C address, for 32k firmware load
 */

static int
cx21143_writeregN (struct cx21143_state *state, int reg, u8 * data, u16 len)
{
  int ret = -EREMOTEIO;
  struct i2c_msg msg;
  u8 *buf;
#ifndef CONFIG_I2C_STM
  struct i2c_algo_bit_data *algo_data = state->config->i2c_adap->algo_data;
  int udelay = algo_data->udelay;
#endif

  buf = kmalloc (len + 1, GFP_KERNEL);

  if (buf == NULL)
  {
    printk ("Unable to kmalloc\n");
    ret = -ENOMEM;
    goto error;
  }

  *(buf) = reg;
  memcpy (buf + 1, data, len);

  msg.addr = state->config->i2c_addr;
  msg.flags = 0;
  msg.buf = buf;
  msg.len = len + 1;

  dprintk (100, "cx21143: %s:  write regN 0x%02x, len = %d\n", __FUNCTION__, reg,
           len);

#ifndef CONFIG_I2C_STM
  /* temporarily increase the bus speed */
  algo_data->udelay = I2C_FAST_DELAY;
#else

#warning " !!!!!!!!!!!!! add fast i2c support (400mhz) for ssc_i2c"

#endif

  if ((ret = i2c_transfer (state->config->i2c_adap, &msg, 1)) != 1)
  {
    printk ("%s: writereg error(err == %i, reg == 0x%02x\n",
            __FUNCTION__, ret, reg);
    ret = -EREMOTEIO;
  }

#ifndef CONFIG_I2C_STM
  /* restore the bus speed */
  algo_data->udelay = udelay;
#else
#warning " !!!!!!!!!!!!! add fast i2c support (400mhz) for ssc_i2c"

#endif

error:
  kfree (buf);

  return ret;
}

/* ******************************************
 *
 */

static int
cx21143_readreg (struct cx21143_state *state, u8 reg)
{
  int ret;
  u8 b0[] = { reg };
  u8 b1[] = { 0 };

  struct i2c_msg msg[] = {
    {.addr = state->config->i2c_addr,.flags = 0,.buf = b0,.len = 1},
    {.addr = state->config->i2c_addr,.flags = I2C_M_RD,.buf = b1,.len = 1}
  };

  dprintk (100, "%s: i2c_addr=0x%x \n", __FUNCTION__, state->config->i2c_addr);
  dprintk (100, "%s: flag = %d \n", __FUNCTION__, I2C_M_RD);

  ret = i2c_transfer (state->config->i2c_adap, msg, 2);

  if (ret != 2)
  {
    printk ("%s: reg=0x%x (error=%d)\n", __FUNCTION__, reg, ret);
    return ret;
  }

  dprintk (100, "cx21143: read reg 0x%02x, value 0x%02x\n", reg, b1[0]);
  return b1[0];
}

/* **********************************************
 *
 */
static int
cx21143_read_status (struct dvb_frontend *fe, fe_status_t * status)
{
  struct cx21143_state *state = fe->demodulator_priv;

  int lock = cx21143_readreg (state, cx21143_REG_STATUS);

  dprintk (150, "%s: status = 0x%02x\n", __FUNCTION__, lock);

  *status = 0;

  if (lock & 0x01)
    *status |= FE_HAS_SIGNAL;
  if (lock & 0x02)
    *status |= FE_HAS_CARRIER;
  if (lock & 0x04)
    *status |= FE_HAS_VITERBI;
  if (lock & 0x08)
    *status |= FE_HAS_SYNC | FE_HAS_LOCK;

  return 0;
}

/* **********************************************
 *
 */
#define FE_IS_TUNED (FE_HAS_SIGNAL + FE_HAS_LOCK + FE_HAS_CARRIER + FE_HAS_VITERBI)
static int
cx21143_is_tuned (struct dvb_frontend *fe)
{
  fe_status_t tunerstat;

  cx21143_read_status (fe, &tunerstat);

  return ((tunerstat & FE_IS_TUNED) == FE_IS_TUNED);
}

/* **********************************************
 *
 */
static int
cx21143_get_tune_settings (struct dvb_frontend *fe,
                          struct dvb_frontend_tune_settings *fetunesettings)
{
  struct cx21143_state *state = fe->demodulator_priv;

  dprintk (10, "%s: > \n", __FUNCTION__);

/* FIXME: Hab das jetzt mal eingebaut, da bei SET_FRONTEND DVB-S2 nicht gehandelt wird.
 * Im Prinzip setze ich hier die Werte aus SET_FRONTEND (siehe dvb-core) mal von min_delay
 * abgesehen.
 * DIES MUSS MAL BEOBACHTETE WERDEN
 */
  fetunesettings->step_size = state->dcur.symbol_rate / 16000;
  fetunesettings->max_drift = state->dcur.symbol_rate / 2000;
  fetunesettings->min_delay_ms = 500; // For pilot auto tune

  dprintk (10, "%s: < \n", __FUNCTION__);

  return 0;
}

/* **********************************************
 *
 */
static int
cx21143_read_ber (struct dvb_frontend *fe, u32 * ber)
{
  struct cx21143_state *state = fe->demodulator_priv;

  dprintk (20, "%s: > \n", __FUNCTION__);

  *ber =
    (cx21143_readreg (state, cx21143_REG_BER24) << 24) |
    (cx21143_readreg (state, cx21143_REG_BER16) << 16) |
    (cx21143_readreg (state, cx21143_REG_BER8) << 8) | cx21143_readreg (state,
                                                                      cx21143_REG_BER0);

  dprintk (20,  "%s: < \n", __FUNCTION__);

  return 0;
}

/* **********************************************
 *
 */
static int
cx21143_read_snr (struct dvb_frontend *fe, u16 * snr)
{
  struct cx21143_state *state = fe->demodulator_priv;

  u8 snr_reading;
  static const u32 snr_tab[] = {  /* 10 x Table (rounded up) */
    0x00000, 0x0199A, 0x03333, 0x04ccD, 0x06667, 0x08000, 0x0999A, 0x0b333,
    0x0cccD, 0x0e667,
    0x10000, 0x1199A, 0x13333, 0x14ccD, 0x16667, 0x18000
  };

  dprintk (20, "%s()\n", __FUNCTION__);

  snr_reading = cx21143_readreg (state, cx21143_REG_QUALITY);

  if (snr_reading >= 0xa0 /* 100% */ )
    *snr = 0xffff;
  else
    *snr = snr_tab[(snr_reading & 0xf0) >> 4] +
      (snr_tab[(snr_reading & 0x0f)] >> 4);

  dprintk (20, "%s: SNR (raw / cooked) = (0x%02x / 0x%04x)\n", __FUNCTION__,
           snr_reading, *snr);

  return 0;
}

/* **********************************************
 *
 */
static int
cx21143_read_ucblocks (struct dvb_frontend *fe, u32 * ucblocks)
{
  struct cx21143_state *state = fe->demodulator_priv;

  dprintk (20, "%s: > \n", __FUNCTION__);

  *ucblocks =
    (cx21143_readreg (state, cx21143_REG_UCB8) << 8) | cx21143_readreg (state,
                                                                      cx21143_REG_UCB0);

  dprintk (20, "%s: %d < \n", __FUNCTION__,*ucblocks);

  return 0;
}

/* **********************************************
 *
 */

#define FEC_S2_QPSK_1_2 (fe_code_rate_t)(FEC_AUTO+1)
#define FEC_S2_QPSK_2_3 (fe_code_rate_t)(FEC_S2_QPSK_1_2+1)
#define FEC_S2_QPSK_3_4 (fe_code_rate_t)(FEC_S2_QPSK_2_3+1)
#define FEC_S2_QPSK_5_6 (fe_code_rate_t)(FEC_S2_QPSK_3_4+1)
#define FEC_S2_QPSK_7_8 (fe_code_rate_t)(FEC_S2_QPSK_5_6+1)
#define FEC_S2_QPSK_8_9 (fe_code_rate_t)(FEC_S2_QPSK_7_8+1)
#define FEC_S2_QPSK_3_5 (fe_code_rate_t)(FEC_S2_QPSK_8_9+1)
#define FEC_S2_QPSK_4_5 (fe_code_rate_t)(FEC_S2_QPSK_3_5+1)
#define FEC_S2_QPSK_9_10 (fe_code_rate_t)(FEC_S2_QPSK_4_5+1)
#define FEC_S2_8PSK_1_2 (fe_code_rate_t)(FEC_S2_QPSK_9_10+1)
#define FEC_S2_8PSK_2_3 (fe_code_rate_t)(FEC_S2_8PSK_1_2+1)
#define FEC_S2_8PSK_3_4 (fe_code_rate_t)(FEC_S2_8PSK_2_3+1)
#define FEC_S2_8PSK_5_6 (fe_code_rate_t)(FEC_S2_8PSK_3_4+1)
#define FEC_S2_8PSK_7_8 (fe_code_rate_t)(FEC_S2_8PSK_5_6+1)
#define FEC_S2_8PSK_8_9 (fe_code_rate_t)(FEC_S2_8PSK_7_8+1)
#define FEC_S2_8PSK_3_5 (fe_code_rate_t)(FEC_S2_8PSK_8_9+1)
#define FEC_S2_8PSK_4_5 (fe_code_rate_t)(FEC_S2_8PSK_3_5+1)
#define FEC_S2_8PSK_9_10 (fe_code_rate_t)(FEC_S2_8PSK_4_5+1)

/* Why isn't this a generic frontend core function? */
static enum dvbfe_fec
cx21143_convert_oldfec_to_new (enum fe_code_rate c)
{
  enum dvbfe_fec fec = DVBFE_FEC_NONE;

  switch (c)
  {
  case FEC_NONE:
    fec = DVBFE_FEC_NONE;
    break;
  case FEC_1_2:
    fec = DVBFE_FEC_1_2;
    break;
  case FEC_2_3:
    fec = DVBFE_FEC_2_3;
    break;
  case FEC_3_4:
    fec = DVBFE_FEC_3_4;
    break;
  case FEC_4_5:
    fec = DVBFE_FEC_4_5;
    break;
  case FEC_5_6:
    fec = DVBFE_FEC_5_6;
    break;
  case FEC_6_7:
    fec = DVBFE_FEC_6_7;
    break;
  case FEC_7_8:
    fec = DVBFE_FEC_7_8;
    break;
  case FEC_8_9:
    fec = DVBFE_FEC_8_9;
    break;
  case FEC_AUTO:
    fec = DVBFE_FEC_AUTO;
    break;

//enigma2
  case FEC_S2_QPSK_1_2:
    fec = DVBFE_FEC_1_2;
    break;
  case FEC_S2_QPSK_2_3:
    fec = DVBFE_FEC_2_3;
    break;
  case FEC_S2_QPSK_3_4:
    fec = DVBFE_FEC_3_4;
    break;
  case FEC_S2_QPSK_5_6:
    fec = DVBFE_FEC_5_6;
    break;
  case FEC_S2_QPSK_7_8:
    fec = DVBFE_FEC_7_8;
    break;
  case FEC_S2_QPSK_8_9:
    fec = DVBFE_FEC_8_9;
    break;
  case FEC_S2_QPSK_3_5:
    fec = DVBFE_FEC_3_5;
    break;
  case FEC_S2_QPSK_4_5:
    fec = DVBFE_FEC_4_5;
    break;
  case FEC_S2_QPSK_9_10:
    fec = DVBFE_FEC_9_10;
    break;
  case FEC_S2_8PSK_1_2:
    fec = DVBFE_FEC_1_2;
    break;
  case FEC_S2_8PSK_2_3:
    fec = DVBFE_FEC_2_3;
    break;
  case FEC_S2_8PSK_3_4:
    fec = DVBFE_FEC_3_4;
    break;
  case FEC_S2_8PSK_5_6:
    fec = DVBFE_FEC_5_6;
    break;
  case FEC_S2_8PSK_7_8:
    fec = DVBFE_FEC_7_8;
    break;
  case FEC_S2_8PSK_8_9:
    fec = DVBFE_FEC_8_9;
    break;
  case FEC_S2_8PSK_3_5:
    fec = DVBFE_FEC_3_5;
    break;
  case FEC_S2_8PSK_4_5:
    fec = DVBFE_FEC_4_5;
    break;
  case FEC_S2_8PSK_9_10:
    fec = DVBFE_FEC_9_10;
    break;
  default:
    fec = FEC_NONE;
    break;
  }

  return fec;
}


/* **********************************************
 *
 */
/* Why isn't this a generic frontend core function? */
static enum dvbfe_fec
cx21143_convert_newfec_to_old (enum dvbfe_fec c, enum dvbfe_delsys d,
                              enum dvbfe_modulation m)
{
  fe_code_rate_t fec;

  dprintk (20, "delsys %d, fec %d, mod %d\n", d, c, m);

  if (d == DVBFE_DELSYS_DVBS)
  {
    switch (c)
    {
    case DVBFE_FEC_NONE:
      fec = FEC_NONE;
      break;
    case DVBFE_FEC_1_2:
      fec = FEC_1_2;
      break;
    case DVBFE_FEC_2_3:
      fec = FEC_2_3;
      break;
    case DVBFE_FEC_3_4:
      fec = FEC_3_4;
      break;
    case DVBFE_FEC_4_5:
      fec = FEC_4_5;
      break;
    case DVBFE_FEC_5_6:
      fec = FEC_5_6;
      break;
    case DVBFE_FEC_6_7:
      fec = FEC_6_7;
      break;
    case DVBFE_FEC_7_8:
      fec = FEC_7_8;
      break;
    case DVBFE_FEC_8_9:
      fec = FEC_8_9;
      break;
    default:
      fec = FEC_NONE;
      break;
    }
  }
  else
  {
    if (m == DVBFE_MOD_QPSK)
    {
      switch (c)
      {
      case DVBFE_FEC_1_2:
        fec = FEC_S2_QPSK_1_2;
        break;
      case DVBFE_FEC_2_3:
        fec = FEC_S2_QPSK_2_3;
        break;
      case DVBFE_FEC_3_4:
        fec = FEC_S2_QPSK_3_4;
        break;
      case DVBFE_FEC_5_6:
        fec = FEC_S2_QPSK_5_6;
        break;
      case DVBFE_FEC_7_8:
        fec = FEC_S2_QPSK_7_8;
        break;
      case DVBFE_FEC_8_9:
        fec = FEC_S2_QPSK_8_9;
        break;
      case DVBFE_FEC_3_5:
        fec = FEC_S2_QPSK_3_5;
        break;
      case DVBFE_FEC_4_5:
        fec = FEC_S2_QPSK_4_5;
        break;
      case DVBFE_FEC_9_10:
        fec = FEC_S2_QPSK_9_10;
        break;
      default:
        fec = FEC_NONE;
        break;
      }
    }
    else
    {
      switch (c)
      {
      case DVBFE_FEC_1_2:
        fec = FEC_S2_8PSK_1_2;
        break;
      case DVBFE_FEC_2_3:
        fec = FEC_S2_8PSK_2_3;
        break;
      case DVBFE_FEC_3_4:
        fec = FEC_S2_8PSK_3_4;
        break;
      case DVBFE_FEC_5_6:
        fec = FEC_S2_8PSK_5_6;
        break;
      case DVBFE_FEC_7_8:
        fec = FEC_S2_8PSK_7_8;
        break;
      case DVBFE_FEC_8_9:
        fec = FEC_S2_8PSK_8_9;
        break;
      case DVBFE_FEC_3_5:
        fec = FEC_S2_8PSK_3_5;
        break;
      case DVBFE_FEC_4_5:
        fec = FEC_S2_8PSK_4_5;
        break;
      case DVBFE_FEC_9_10:
        fec = FEC_S2_8PSK_9_10;
        break;
      default:
        fec = FEC_NONE;
        break;
      }
    }
  }

  dprintk (20, "%s: fec = %d\n", __FUNCTION__, fec);
  return fec;
}

//Hack
static enum dvbfe_modulation
cx21143_get_modulation_from_fec (enum fe_code_rate c)
{
  switch (c)
  {
  case FEC_NONE:
  case FEC_1_2:
  case FEC_2_3:
  case FEC_3_4:
  case FEC_4_5:
  case FEC_5_6:
  case FEC_6_7:
  case FEC_7_8:
  case FEC_8_9:
  case FEC_AUTO:
    return DVBFE_MOD_QPSK;
    break;
  case FEC_S2_QPSK_1_2:
  case FEC_S2_QPSK_2_3:
  case FEC_S2_QPSK_3_4:
  case FEC_S2_QPSK_5_6:
  case FEC_S2_QPSK_7_8:
  case FEC_S2_QPSK_8_9:
  case FEC_S2_QPSK_3_5:
  case FEC_S2_QPSK_4_5:
  case FEC_S2_QPSK_9_10:
    return DVBFE_MOD_QPSK;
    break;
  case FEC_S2_8PSK_1_2:
  case FEC_S2_8PSK_2_3:
  case FEC_S2_8PSK_3_4:
  case FEC_S2_8PSK_5_6:
  case FEC_S2_8PSK_7_8:
  case FEC_S2_8PSK_8_9:
  case FEC_S2_8PSK_3_5:
  case FEC_S2_8PSK_4_5:
  case FEC_S2_8PSK_9_10:
    return DVBFE_MOD_8PSK;
    break;
  default:
    return DVBFE_MOD_QPSK;
    break;
  }
}


/* Why isn't this a generic frontend core function? */
static int
cx21143_create_new_qpsk_feparams (struct dvb_frontend *fe,
                                 struct dvb_frontend_parameters *pFrom,
                                 struct dvbfe_params *pTo)
{
  int ret = 0;
  int rolloff;
  enum dvbfe_rolloff Rollofftab[4] =
  { DVBFE_ROLLOFF_35, DVBFE_ROLLOFF_25, DVBFE_ROLLOFF_20,
    DVBFE_ROLLOFF_UNKNOWN
  };

  dprintk (20, "%s\n", __FUNCTION__);

  memset (pTo, 0, sizeof (struct dvbfe_params));

  dprintk (10, "cx21143: (vor Umwandlung) FREQ %i SR %i FEC %x \n",
           pFrom->frequency, pFrom->u.qpsk.symbol_rate,
           pFrom->u.qpsk.fec_inner);

  pTo->frequency = pFrom->frequency;
  // HACK from E2: Bits 2..3 are rolloff in DVB-S2; inversion can have normally values 0,1,2
  pTo->inversion = pFrom->inversion & 3;
  // modulation in bits 23-16s
  if (pFrom->u.qpsk.fec_inner > FEC_AUTO)
  {
    switch (pFrom->inversion & 0xc)
    {
    default:                   // unknown rolloff
    case 0:                    // 0.35
      rolloff = 0;
      break;
    case 4:                    // 0.25
      rolloff = 1;
      break;
    case 8:                    // 0.20
      rolloff = 2;
      break;
    }

    if (pFrom->u.qpsk.fec_inner > FEC_S2_QPSK_9_10)
    {
      switch (pFrom->inversion & 0x30)
      {
      case 0:                  // pilot off
        pilot = 0;
        break;
      case 0x10:               // pilot on
        pilot = 1;
        break;
      case 0x20:               // pilot auto
        pilot = 2;
        break;
      }
    }
    pTo->delivery = DVBFE_DELSYS_DVBS2;
    pTo->delsys.dvbs.rolloff = Rollofftab[rolloff & 3];
  }
  else
  {
    pTo->delivery = DVBFE_DELSYS_DVBS;
//FIXME: ist das richtig?  
    pTo->delsys.dvbs.rolloff = DVBFE_ROLLOFF_35;

  }

  pTo->delsys.dvbs.modulation =
    cx21143_get_modulation_from_fec (pFrom->u.qpsk.fec_inner);

  pTo->delsys.dvbs.symbol_rate = pFrom->u.qpsk.symbol_rate;
  pTo->delsys.dvbs.fec =
    cx21143_convert_oldfec_to_new (pFrom->u.qpsk.fec_inner & 255);

  dprintk (1, "cx21143: FREQ %i SR %i FEC %x FECN %08x MOD %i DS %i \n",
          pTo->frequency, pTo->delsys.dvbs.symbol_rate,
          pFrom->u.qpsk.fec_inner,
          pTo->delsys.dvbs.fec, pTo->delsys.dvbs.modulation, pTo->delivery);

  /* I guess we could do more validation on the old fe params and return error */
  return ret;
}

/* Why isn't this a generic frontend core function? */
static int
cx21143_create_old_qpsk_feparams (struct dvb_frontend *fe,
                                 struct dvbfe_params *pFrom,
                                 struct dvb_frontend_parameters *pTo)
{
  int ret = 0;

  dprintk (10, "%s\n", __FUNCTION__);

  memset (pTo, 0, sizeof (struct dvb_frontend_parameters));

  pTo->frequency = pFrom->frequency;
  pTo->inversion = pFrom->inversion;

  switch (pFrom->delivery)
  {
  case DVBFE_DELSYS_DVBS:
    pTo->u.qpsk.fec_inner =
      cx21143_convert_newfec_to_old (pFrom->delsys.dvbs.fec,
                                    DVBFE_DELSYS_DVBS,
                                    pFrom->delsys.dvbs.modulation);
    pTo->u.qpsk.symbol_rate = pFrom->delsys.dvbs.symbol_rate;
    break;
  case DVBFE_DELSYS_DVBS2:
    pTo->u.qpsk.fec_inner =
      cx21143_convert_newfec_to_old (pFrom->delsys.dvbs2.fec,
                                    DVBFE_DELSYS_DVBS2,
                                    pFrom->delsys.dvbs2.modulation);
    pTo->u.qpsk.symbol_rate = pFrom->delsys.dvbs2.symbol_rate;
    break;
  default:
    ret = -1;
  }

  /* I guess we could do more validation on the old fe params and return error */
  return ret;
}

/* Wait for LNB */
static int
cx21143_wait_for_lnb (struct dvb_frontend *fe)
{

#ifdef no_maruapp
  dprintk (10, "%s() qstatus = 0x%02x\n", __FUNCTION__,
           cx21143_readreg (state, cx21143_REG_QSTATUS));

  /* Wait for up to 500 ms */
  for (i = 0; i < 50; i++)
  {
    if (cx21143_readreg (state, cx21143_REG_QSTATUS) & 0x20)
      return 0;
    msleep (10);
  }

  dprintk (10, "%s(): LNB not ready\n", __FUNCTION__);
#else
  return 0;
#endif
  return -ETIMEDOUT;            /* -EBUSY ? */
}

static int cx21143_load_firmware (struct dvb_frontend *fe,
                                 const struct firmware *fw);

static int
cx21143_firmware_ondemand (struct dvb_frontend *fe)
{
  struct cx21143_state *state = fe->demodulator_priv;
  int ret = 0;
  int syschipid, gotreset;
  const struct firmware *fw;

  dprintk (10, "%s >()\n", __FUNCTION__);

  /* lock semaphore to ensure data consistency */
  if(down_trylock(&state->fw_load_sem))
  {
    return 0;
  }

  gotreset = cx21143_readreg (state, 0x20);  // implicit watchdog
  syschipid = cx21143_readreg (state, 0x94);

  if (gotreset || syschipid != 5 || state->not_responding >= cMaxError)
  {
    dprintk (1, "%s: Start Firmware upload ... \n", __FUNCTION__);

    ret =
      request_firmware (&fw, cx21143_DEFAULT_FIRMWARE,
                        &state->config->i2c_adap->dev);
    dprintk (1, "%s: Waiting for firmware (%s) upload(2)...\n", __FUNCTION__,
            cx21143_DEFAULT_FIRMWARE);
    if (ret)
    {
      printk ("%s: No firmware uploaded (%d - timeout or file not found?)\n",
              __FUNCTION__, ret);
      up(&state->fw_load_sem);
      return ret;
    }

    ret = cx21143_load_firmware (fe, fw);

    if (ret)
      printk ("%s: Writing firmware to device failed\n", __FUNCTION__);

    release_firmware (fw);

    dprintk (1, "%s: Firmware upload %s\n", __FUNCTION__,
            ret == 0 ? "complete" : "failed");
  }
  else
  {
    dprintk (20, "%s: Firmware upload not needed\n", __FUNCTION__);
  }

  up(&state->fw_load_sem);

  dprintk (20, "%s <()\n", __FUNCTION__);

  return ret;
}

/* Take a basic firmware command structure, format it and forward it for processing */
static int
cx21143_cmd_execute (struct dvb_frontend *fe, struct cx21143_cmd *cmd)
{
  struct cx21143_state *state = fe->demodulator_priv;
  unsigned int i;
  int ret;

  dprintk (50, "%s:\n", __FUNCTION__);

  /* Load the firmware if required */
  if ((ret = cx21143_firmware_ondemand (fe)) != 0)
  {
    printk ("%s(): Unable initialise the firmware\n", __FUNCTION__);
    return ret;
  }

  for (i = 0; i < (sizeof (cx21143_COMMANDS) / sizeof (struct firmware_cmd));
       i++)
  {
    if (cx21143_COMMANDS[i].id == CMD_MAX)
    {
      printk("Unknown CMD %02x\n",cmd->id);
      return -EINVAL;
    }

    if (cx21143_COMMANDS[i].id == cmd->id)
    {
      dprintk(50, "CMD %02x len = %d\n", cx21143_COMMANDS[i].id, cx21143_COMMANDS[i].len); 
      cmd->len = cx21143_COMMANDS[i].len;
      break;
    }
  }

  cmd->args[0x00] = cmd->id;

  /* Write the command */
  for (i = 0; i < cmd->len ; i++)
  {
    if (i < cmd->len)
    {
      dprintk (50, "%s: 0x%02x == 0x%02x\n", __FUNCTION__, i, cmd->args[i]);
      cx21143_writereg (state, i, cmd->args[i]);
    }
    else
      cx21143_writereg (state, i, 0);
  }


  /* Start execution and wait for cmd to terminate */
  cx21143_writereg (state, 0x1f, 0x01);
  msleep (2); /*give tuner some easy time*/
  while (cx21143_readreg (state, 0x1f))
  {
    msleep (10);
    if (i++ > 64 /*128 */ )
    {
      /* Avoid looping forever if the firmware does no respond */
      printk ("%s() Firmware not responding\n", __FUNCTION__);
      state->not_responding++;

      return -EREMOTEIO;
    }
  }

  return 0;
}

/* XXX Actual scale is non-linear */
static int
cx21143_read_signal_strength (struct dvb_frontend *fe, u16 * signal_strength)
{
  struct cx21143_state *state = fe->demodulator_priv;
  u16 sig_reading;

  dprintk (20, "%s()\n", __FUNCTION__);

  sig_reading =
//    ((cx21143_readreg (state, cx21143_REG_STATUS) & 0xc0) >>1) |
//    (cx21143_readreg (state, cx21143_REG_SIGNAL) << 7);
    ((cx21143_readreg (state, cx21143_REG_STATUS) & 0xc0) <<8) |
    (cx21143_readreg (state, cx21143_REG_SIGNAL) << 6);

  *signal_strength = 0 - sig_reading;

  dprintk (20, "%s: Signal strength (raw / cooked) = (0x%04x / 0x%04x)\n",
           __FUNCTION__, sig_reading, *signal_strength);

  return 0;
}


static int
cx21143_load_firmware (struct dvb_frontend *fe, const struct firmware *fw)
{
  struct cx21143_state *state = fe->demodulator_priv;
  struct cx21143_cmd cmd;
  int ret;

  dprintk (10, "%s\n", __FUNCTION__);
  dprintk (10, "Firmware is %zu bytes (%02x %02x .. %02x %02x)\n", fw->size,
           fw->data[0], fw->data[1], fw->data[fw->size - 2],
           fw->data[fw->size - 1]);

  /* Toggle 88x SRST pin to reset demod */
  cx21143_reset (state);

  // PLL
  cx21143_writereg (state, 0xF1, 0x08);
  cx21143_writereg (state, 0xF2, 0x12);

  /* Begin the firmware load process */
  /* Prepare the demod, load the firmware, cleanup after load */

  cx21143_writereg (state, 0xF3, 0x46);
  cx21143_writereg (state, 0xF9, 0x00);

  cx21143_writereg (state, 0xF0, 0x03);
  cx21143_writereg (state, 0xF4, 0x81);
  cx21143_writereg (state, 0xF5, 0x00);
  cx21143_writereg (state, 0xF6, 0x00);

  /* write the entire firmware as one transaction */
  cx21143_writeregN (state, 0xF7, fw->data, fw->size);

  cx21143_writereg (state, 0xF4, 0x10);
  cx21143_writereg (state, 0xF0, 0x00);
  cx21143_writereg (state, 0xF8, 0x06);

  /* Firmware CMD 10: Main init */
  cmd.id = CMD_SET_VCO;
  cmd.args[0x01] = 0x05;
  cmd.args[0x02] = 0x8d;
  cmd.args[0x03] = 0xdc;
  cmd.args[0x04] = 0xb8;
  cmd.args[0x05] = 0x5e;
  cmd.args[0x06] = 0x04;
  cmd.args[0x07] = 0x9d;
  cmd.args[0x08] = 0xfc;
  cmd.args[0x09] = 0x06;
  ret = cx21143_cmd_execute (fe, &cmd);
  if (ret != 0)
    return ret;

  cx21143_writereg (state, 0x9d, 0x00);

  /* Firmware CMD 14: Tuner Init */
  cmd.id = CMD_TUNERINIT;
  cmd.args[0x01] = 0x00;
  cmd.args[0x02] = 0x00;
  ret = cx21143_cmd_execute (fe, &cmd);
  if (ret != 0)
    return ret;

  cx21143_writereg (state, 0xe5, 0x00);


  /* Firmware CMD 13: MPEG/TS output config */
  cmd.id = CMD_MPEGCONFIG;
  cmd.args[0x01] = 0x01;
  cmd.args[0x02] = 0x70;
  cmd.args[0x03] = 0x00;
  cmd.args[0x04] = 0x01;
  cmd.args[0x05] = 0x00;

  ret = cx21143_cmd_execute (fe, &cmd);
  if (ret != 0)
    return ret;

  cx21143_writereg (state, 0xe0, 0x08);

  // Firmware CMD 20: LNB/Diseqc Config
  cmd.id = CMD_LNBCONFIG;
  cmd.args[1] = 0x00;
  cmd.args[2] = 0x03;
  cmd.args[3] = 0x00;
  cmd.args[4] = 0x8e;
  cmd.args[5] = 0xff;
  cmd.args[6] = 0x02;
  cmd.args[7] = 0x01;

  ret = cx21143_cmd_execute (fe, &cmd);
  if (ret != 0)
    return ret;

  return 0;
}


static int
cx21143_set_tone (struct dvb_frontend *fe, fe_sec_tone_mode_t tone)
{
  struct cx21143_cmd cmd;
  int ret;

  dprintk (10, "%s(%d)\n", __FUNCTION__, tone);
  if ((tone != SEC_TONE_ON) && (tone != SEC_TONE_OFF))
  {
    printk ("%s: Invalid, tone=%d\n", __FUNCTION__, tone);
    return -EINVAL;
  }

  /* Wait for LNB ready */
  ret = cx21143_wait_for_lnb (fe);
  if (ret != 0)
    return ret;

  msleep (100);

  /* This is always done before the tone is set */
  cmd.id = CMD_SET_TONEPRE;
  cmd.args[0x01] = 0x00;
  ret = cx21143_cmd_execute (fe, &cmd);
  if (ret != 0)
    return ret;

  /* Now we set the tone */
  cmd.id = CMD_SET_TONE;
  cmd.args[0x01] = 0x00;
  cmd.args[0x02] = 0x00;
  switch (tone)
  {
  case SEC_TONE_ON:
    dprintk (10, "%s: setting tone on\n", __FUNCTION__);
    cmd.args[0x03] = 0x01;
    break;
  case SEC_TONE_OFF:
    dprintk (10, "%s: setting tone off\n", __FUNCTION__);
    cmd.args[0x03] = 0x00;
    break;
  }
  ret = cx21143_cmd_execute (fe, &cmd);

  msleep (100);

  return ret;


}

/* Initialise DiSEqC */
static int
cx21143_diseqc_init (struct dvb_frontend *fe)
{
  struct cx21143_state *state = fe->demodulator_priv;

  /* Prepare a DiSEqC command */
  state->dsec_cmd.id = CMD_LNBSEND;

  /* DiSEqC burst */
  state->dsec_cmd.args[cx21143_DISEQC_BURST] = cx21143_DISEQC_MINI_B;

  /* Unknown */
  state->dsec_cmd.args[cx21143_DISEQC_ARG2_2] = 0x02;
  state->dsec_cmd.args[cx21143_DISEQC_ARG3_0] = 0x00;
  state->dsec_cmd.args[cx21143_DISEQC_ARG4_0] = 0x00; /* Continuation flag? */

  /* DiSEqC message length */
  state->dsec_cmd.args[cx21143_DISEQC_MSGLEN] = 0x00;

  /* Command length */
  state->dsec_cmd.len = cx21143_DISEQC_MSGOFS;

  return 0;
}

/* Send with derived burst (hack) / previous burst OR cache DiSEqC message */
static int
cx21143_send_diseqc_msg (struct dvb_frontend *fe,
                        struct dvb_diseqc_master_cmd *d)
{
  struct cx21143_state *state = fe->demodulator_priv;
  int i, ret;

  dprintk(20, "%s >\n", __func__);

  /* Dump DiSEqC message */
  if (paramDebug)
  {
    dprintk (50, "cx21143: %s(", __FUNCTION__);
    for (i = 0; i < d->msg_len;)
    {
      dprintk (50, "0x%02x", d->msg[i]);
      if (++i < d->msg_len)
        dprintk (50, ", ");
    }
    dprintk (50, ") dsec=%s\n", (dsec) ? "CACHE" : "HACK");
  }

  /* Validate length */
  if (d->msg_len > (cx21143_ARGLEN - cx21143_DISEQC_MSGOFS))
    return -EINVAL;

  /* DiSEqC message */
  for (i = 0; i < d->msg_len; i++)
    state->dsec_cmd.args[cx21143_DISEQC_MSGOFS + i] = d->msg[i];

  /* DiSEqC message length */
  state->dsec_cmd.args[cx21143_DISEQC_MSGLEN] = d->msg_len;

  /* Command length */
  state->dsec_cmd.len =
    cx21143_DISEQC_MSGOFS + state->dsec_cmd.args[cx21143_DISEQC_MSGLEN];

  if (dsec)
  {
    /* Return with command/message cached (diseqc_send_burst MUST follow) (dsec=CACHE) */
    return 0;
  }

  /* Hack: Derive burst from command else use previous burst */
  if (d->msg_len >= 4 && d->msg[2] == 0x38)
    state->dsec_cmd.args[cx21143_DISEQC_BURST] = (d->msg[3] >> 2) & 1;

  if (paramDebug)
    dprintk (50, "%s burst=%d\n", __FUNCTION__,
             state->dsec_cmd.args[cx21143_DISEQC_BURST]);

  /* Wait for LNB ready */
  ret = cx21143_wait_for_lnb (fe);
  if (ret != 0)
    return ret;

  /* Command */
  return cx21143_cmd_execute (fe, &state->dsec_cmd);
}

/* Send DiSEqC burst */
static int
cx21143_diseqc_send_burst (struct dvb_frontend *fe, fe_sec_mini_cmd_t burst)
{
  struct cx21143_state *state = fe->demodulator_priv;
  int ret;

  dprintk (20, "%s(%d) dsec=%s\n", __FUNCTION__, (int) burst,
           (dsec) ? "CACHE" : "HACK");

  /* DiSEqC burst */
  if (burst == SEC_MINI_A)
    state->dsec_cmd.args[cx21143_DISEQC_BURST] = cx21143_DISEQC_MINI_A;
  else if (burst == SEC_MINI_B)
    state->dsec_cmd.args[cx21143_DISEQC_BURST] = cx21143_DISEQC_MINI_B;
  else
    return -EINVAL;

  if (!dsec)
  {
    /* Return when using derived burst strategy (dsec=HACK) */
    return 0;
  }

  /* Wait for LNB ready */
  ret = cx21143_wait_for_lnb (fe);
  if (ret != 0)
    return ret;

  /* Command */
  return cx21143_cmd_execute (fe, &state->dsec_cmd);
}


static int
cx21143_sleep (struct dvb_frontend *fe)
{
  struct cx21143_state *state = fe->demodulator_priv;
  struct cx21143_cmd cmd;

  dprintk (10, "%s\n", __FUNCTION__);

  cmd.id = CMD_TUNERSLEEP;
  cmd.args[1] = 1;
  cx21143_cmd_execute (fe, &cmd);

//FIXME: Stimmt das?
  // Shutdown clocks
  cx21143_writereg (state, 0xea, 0xff);
  cx21143_writereg (state, 0xe1, 1);
  cx21143_writereg (state, 0xe0, 1);
  return 0;
}

static void
cx21143_release (struct dvb_frontend *fe)
{
  struct cx21143_state *state = fe->demodulator_priv;
  dprintk (10, "%s\n", __FUNCTION__);
  if (state->config != NULL)
  {
    if(state->config->tuner_enable_pin != NULL)
      stpio_free_pin (state->config->tuner_enable_pin);
  }
  kfree (state->config);
  kfree (state);
}

static int
cx21143_set_fec (struct cx21143_state *state, struct dvbfe_params *p)
{
  int ret = -1;

  dprintk (10, "%s()\n", __FUNCTION__);

  switch (p->delivery)
  {
  case DVBFE_DELSYS_DVBS:
    ret = cx21143_lookup_fecmod (state,
                                p->delivery, p->delsys.dvbs.modulation,
                                p->delsys.dvbs.fec);
    break;
  case DVBFE_DELSYS_DVBS2:
    ret = cx21143_lookup_fecmod (state,
                                p->delivery, p->delsys.dvbs2.modulation,
                                p->delsys.dvbs2.fec);
    break;
  default:
    printk ("%s(return enotsupp)\n", __FUNCTION__);
    ret = -EOPNOTSUPP;
  }
  if (ret >= 0)
  {
    state->dnxt.fec_numb = ret;
    state->dnxt.fec_val = cx21143_MODFEC_MODES[ret].val;
    state->dnxt.fec_mask = cx21143_MODFEC_MODES[ret].mask;
    dprintk (1, "%s() numb/fec/mask = 0x%02x/0x%02x/0x%02x\n", __FUNCTION__,
             ret,state->dnxt.fec_val, state->dnxt.fec_mask);

    ret = 0;
  }
  dprintk (10, "%s(ret =%d)\n", __FUNCTION__, ret);
  return ret;
}


static int
cx21143_set_symbolrate (struct cx21143_state *state, struct dvbfe_params *p)
{
  int ret = 0;

  dprintk (10, "%s()\n", __FUNCTION__);

  switch (p->delivery)
  {
  case DVBFE_DELSYS_DVBS:
    state->dnxt.symbol_rate = p->delsys.dvbs.symbol_rate;
    break;
  case DVBFE_DELSYS_DVBS2:
    state->dnxt.symbol_rate = p->delsys.dvbs2.symbol_rate;
    break;
  default:
    printk ("%s(return enotsupp)\n", __FUNCTION__);
    ret = -EOPNOTSUPP;
  }

  dprintk (1, "%s() symbol_rate = %d\n", __FUNCTION__, state->dnxt.symbol_rate);

  /*  check if symbol rate is within limits */
  if ((state->dnxt.symbol_rate > state->frontend.ops.info.symbol_rate_max) ||
      (state->dnxt.symbol_rate < state->frontend.ops.info.symbol_rate_min))
    ret = -EOPNOTSUPP;

  return ret;
}

/* GA Hack: Inversion is nwo implicitely auto. The set_inversion-call is used via the 
   zig zag scan to find the correct the pilot on/off-modes for S2 tuning.  
   The demod can't detect it on its own :-(
*/

static int
cx21143_set_inversion (struct cx21143_state *state,
                      fe_spectral_inversion_t inversion)
{
  dprintk (1, "%s(%d)\n", __FUNCTION__, inversion);

//FIXME: Bei maruapp hab ich bisher nur inversion 0x0c beobachtet

  switch (inversion)
  {
  case INVERSION_OFF:
    state->dnxt.inversion_val = 0x00;
    break;
  case INVERSION_ON:
    state->dnxt.inversion_val = 0x04;
    break;
  case INVERSION_AUTO:
    state->dnxt.inversion_val = 0x0C;
    break;
  default:
    printk ("%s: ret einval\n", __FUNCTION__);
    return -EINVAL;
  }

  state->dnxt.inversion = inversion;

  return 0;
}

static int
cx21143_get_inversion (struct cx21143_state *state,
                      fe_spectral_inversion_t * inversion)
{
  dprintk (10, "%s()\n", __FUNCTION__);
  *inversion = state->dcur.inversion;
  return 0;
}

static int
cx21143_get_params (struct dvb_frontend *fe, struct dvbfe_params *p)
{
  struct cx21143_state *state = fe->demodulator_priv;
  int ret = 0;
  s16 frequency_offset;
  s16 sr_offset;

  dprintk (10, "%s()\n", __FUNCTION__);

  frequency_offset =
    (cx21143_readreg (state, 0x9f) << 8) + cx21143_readreg (state, 0xa0);
  sr_offset = 0; //(cx21143_readreg(state, 0xa1)<<8)+cx21143_readreg(state, 0xa2);
  dprintk (10, "fr_offs=%d sr_offs=%d\n",frequency_offset,sr_offset);
  p->frequency = state->dcur.frequency + frequency_offset;  // unit seems to be 1kHz

  if (cx21143_get_inversion (state, &p->inversion) != 0)
  {
    printk ("%s: Failed to get inversion status\n", __FUNCTION__);
    return -EREMOTEIO;
  }

  p->delivery = state->dcur.delivery;

  switch (p->delivery)
  {
  case DVBFE_DELSYS_DVBS2:
    p->delsys.dvbs2.fec = state->dcur.fec;
    p->delsys.dvbs2.symbol_rate = state->dcur.symbol_rate + sr_offset * 1000;
    break;
  case DVBFE_DELSYS_DVBS:
    p->delsys.dvbs.fec = state->dcur.fec;
    p->delsys.dvbs.symbol_rate = state->dcur.symbol_rate + sr_offset * 1000;
    break;
  default:
    ret = -ENOTSUPP;
  }

  return ret;
}

static int
cx21143_set_params (struct dvb_frontend *fe, struct dvbfe_params *p)
{
  struct cx21143_state *state = fe->demodulator_priv;
  struct cx21143_cmd cmd;
  int ret, i;
  u8 status, retune = 1;

  dprintk (10, "%s() >\n", __FUNCTION__);


//FIXME: Das mit den cx21143_tunesettings dnxt und dcur ist doch auch
//totaler quatsch. warum merke ich mir nicht dvbfe_params???

  state->dnxt.delivery = p->delivery;
  state->dnxt.frequency = p->frequency;

  if (p->delivery == DVBFE_DELSYS_DVBS2)
  {
    state->dnxt.rolloff = p->delsys.dvbs2.rolloff;
    state->dnxt.fec = p->delsys.dvbs2.fec;
    state->dnxt.modulation = p->delsys.dvbs2.modulation;
  }
  else
  {
    state->dnxt.rolloff = p->delsys.dvbs.rolloff;
    state->dnxt.fec = p->delsys.dvbs.fec;
    state->dnxt.modulation = p->delsys.dvbs.modulation;
  }
  if ((ret = cx21143_set_inversion (state, p->inversion)) != 0)
    return ret;

  if ((ret = cx21143_set_fec (state, p)) != 0)
    return ret;

  if ((ret = cx21143_set_symbolrate (state, p)) != 0)
    return ret;

  /* discard the 'current' tuning parameters and prepare to tune */
  memcpy (&state->dcur, &state->dnxt, sizeof (state->dcur));

  dprintk
    (1, "cx21143: FREQ %i SR %i FECN %d DS %i INV = %d ROLL = %d INV_VAL = %d VAL/MASK %d/%d\n",
     state->dcur.frequency, state->dcur.symbol_rate, state->dcur.fec,
     state->dcur.delivery, state->dcur.inversion, state->dcur.rolloff,
     state->dcur.inversion_val, state->dcur.fec_val, state->dcur.fec_mask);

  /* Prepare a tune request */
  cmd.id = CMD_TUNEREQUEST;

  /* Frequency */
  cmd.args[0x01] = (state->dcur.frequency & 0xff0000) >> 16;
  cmd.args[0x02] = (state->dcur.frequency & 0x00ff00) >> 8;
  cmd.args[0x03] = (state->dcur.frequency & 0x0000ff);

  /* Symbol Rate */
  cmd.args[0x04] = ((state->dcur.symbol_rate / 1000) & 0xff00) >> 8;
  cmd.args[0x05] = ((state->dcur.symbol_rate / 1000) & 0x00ff);

  /* Automatic Inversion */
  cmd.args[0x06] = state->dcur.inversion_val;

  /* Modulation / FEC */
  cmd.args[0x07] = state->dcur.fec_val;
  if (pilot > 0)
      cmd.args[0x07] |= 0x40;
  cmd.args[0x08] = cx21143_SEARCH_RANGE_KHZ >> 8;
  cmd.args[0x09] = cx21143_SEARCH_RANGE_KHZ & 0xff;
  cmd.args[0x0a] = 0x00;
  cmd.args[0x0b] = 0x00;

  if (state->dcur.rolloff == DVBFE_ROLLOFF_25)
    cmd.args[0x0c] = 0x01;
  else if (state->dcur.rolloff == DVBFE_ROLLOFF_20)
    cmd.args[0x0c] = 0x00;
  else
    cmd.args[0x0c] = 0x02;      // 0.35

  cmd.args[0x0d] = state->dcur.fec_mask;
  cmd.args[0x0e] = 0x06;
  cmd.args[0x0f] = 0x00;
  cmd.args[0x10] = 0x00;
  cmd.args[0x11] = 0xec;
  cmd.args[0x12] = 0xfa;

  /* Set/Reset unknown */
  cx21143_writereg (state, 0xF3, 0x46); /* clock devidor */
  cx21143_writereg (state, 0xF9, 0x00); /* mode dvb-s 1/2 */

  if ((state->dcur.delivery == DVBFE_DELSYS_DVBS2) || (state->dcur.fec != DVBFE_FEC_AUTO)) //fec_auto
    retune = 2;

  do
  {
    /* Reset status register */
    status = cx21143_readreg (state, cx21143_REG_STATUS) & 0xc0;
    cx21143_writereg (state, cx21143_REG_STATUS, status);

    /* Tune */
    ret = cx21143_cmd_execute (fe, &cmd);
    if (ret != 0)
      return ret;
    msleep (10);
    /* Wait for up to 500 ms */
    for (i = 0; i < 50; i++)
    {
      if (cx21143_is_tuned (fe))
        goto tuned;
      msleep (10);
    }

    if (state->dcur.delivery == DVBFE_DELSYS_DVBS2)
    {
      /* Toggle pilot bit */
      cmd.args[0x07] ^= 0x40;
    }
    else
    {
      /* dvbs try fec_auto on second try */
      cmd.args[0x07] =0x2e;
      cmd.args[0x0d] =0xfe;
      state->dcur.fec = DVBFE_FEC_AUTO;
      state->dcur.fec_numb = 9;
    }
  }
  while (--retune);
  return ret;
tuned:                         /* Set/Reset B/W */
  cmd.id = CMD_GETAGC; /* wird wohl nicht getagc sein oder? denn er liest nicht */
  ret = cx21143_cmd_execute (fe, &cmd);
  
  cx21143_writereg (state, cx21143_REG_STATUS, 0x0f);

  cmd.id = CMD_BANDWIDTH;
  cmd.args[0x01] = 0x00;
  cmd.len = 0x02;
  ret = cx21143_cmd_execute (fe, &cmd);
  if (ret != 0)
  {
    printk("error CMD_BANDWIDTH\n");
    return ret;
  }
  status = cx21143_readreg (state, cx21143_REG_FECSTATUS);
  dprintk(1, "tuned fec=%02x new fec=%02x\n",state->dcur.fec_numb,status);
  if ((state->dcur.fec_numb != (status & cx21143_FEC_FECMASK)) && (state->dcur.delivery == DVBFE_DELSYS_DVBS))
  {
    state->dcur.fec_numb = status & cx21143_FEC_FECMASK;
    state->dcur.fec=cx21143_MODFEC_MODES[state->dcur.fec_numb].fec;
  }
  cmd.id = CMD_U1;
  ret = cx21143_cmd_execute (fe, &cmd);
  dprintk(1, "U1 data %02x ",cmd.args[0x07]);
  for (i = 0; i < 6; i++)
  {
      state->dcur.U1[i]=cx21143_readreg(state,i+1);
      dprintk(1, " %02x",state->dcur.U1[i]);
  }
  dprintk(1, "\n");
  
  ret = state->dcur.fec_numb * 2;
  
  if ((cmd.args[0x07] & 0x40) == 0x40)
  {
      ret++;
      pilot = 1;
  }
  else
      pilot = 0;
      
  cmd.id = CMD_U2;
  cmd.args[1] = cx21143_U2_TABLE[ret].U2_1;
  cmd.args[2] = cx21143_U2_TABLE[ret].U2_2;
  cmd.args[3] = cx21143_U2_TABLE[ret].U2_3;
  cmd.args[4] = cx21143_U2_TABLE[ret].U2_4;
  cmd.args[5] = cx21143_U2_TABLE[ret].U2_5;
  cmd.args[6] = cx21143_U2_TABLE[ret].U2_6;
  cmd.args[7] = cx21143_U2_TABLE[ret].U2_7;
  cmd.args[8] = cx21143_U2_TABLE[ret].U2_8;
  cmd.args[9] = cx21143_U2_TABLE[ret].U2_9;

  ret = cx21143_cmd_execute (fe, &cmd);

  dprintk (10, "%s < %d\n", __FUNCTION__, ret);

  return ret;

}


static int
cx21143_get_frontend (struct dvb_frontend *fe,
                     struct dvb_frontend_parameters *p)
{
  struct dvbfe_params feparams;
  int ret;

  dprintk (10, "%s: > \n", __FUNCTION__);

  ret = cx21143_get_params (fe, &feparams);
  if (ret != 0)
    return ret;

  return cx21143_create_old_qpsk_feparams (fe, &feparams, p);
}

static int
cx21143_set_frontend (struct dvb_frontend *fe,
                     struct dvb_frontend_parameters *p)
{
  struct dvbfe_params newfe;
  int ret = 0;

  dprintk (10, "%s: > \n", __FUNCTION__);

  ret = cx21143_create_new_qpsk_feparams (fe, p, &newfe);

  if (ret != 0)
    return ret;

  return cx21143_set_params (fe, &newfe);
}

static int
cx21143_fe_init (struct dvb_frontend *fe)
{
  struct cx21143_state *state = fe->demodulator_priv;
  struct cx21143_cmd cmd;

  dprintk(10, "%s > \n", __func__);

//FIXME: Macht maruapp nicht, stimmt das?

  // Powerup
  cx21143_writereg (state, 0xe0, 0);
  cx21143_writereg (state, 0xe1, 0);
  cx21143_writereg (state, 0xea, 0);

  cmd.id = CMD_TUNERSLEEP;
  cmd.args[1] = 0;
  cx21143_cmd_execute (fe, &cmd);

  dprintk(10,"%s < \n", __func__);

  return cx21143_diseqc_init (fe);
}

int
cx21143_set_voltage (struct dvb_frontend *fe, fe_sec_voltage_t voltage)
{
  struct cx21143_state *state = fe->demodulator_priv;
  int ret = 0;

  dprintk (10, "%s(%p, %d)\n", __FUNCTION__, fe, voltage);

  switch (voltage)
  {
  case SEC_VOLTAGE_OFF:
    cx21143_writereg_lnb_supply(state, 0xd0);
    break;
  case SEC_VOLTAGE_13: //vertical
    cx21143_writereg_lnb_supply(state, 0xd4);
    break;
  case SEC_VOLTAGE_18: //horizontal
    cx21143_writereg_lnb_supply(state, 0xdc);
    break;
  default:
    return -EINVAL;
  }

  return ret;
}


static int
cx21143_get_info (struct dvb_frontend *fe, struct dvbfe_info *fe_info)
{
  dprintk (10, "%s\n", __FUNCTION__);

  switch (fe_info->delivery)
  {
  case DVBFE_DELSYS_DVBS:
    dprintk (10,"%s(DVBS)\n", __FUNCTION__);
    memcpy (fe_info, &dvbs_info, sizeof (dvbs_info));
    break;
  case DVBFE_DELSYS_DVBS2:
    dprintk (10, "%s(DVBS2)\n", __FUNCTION__);
    memcpy (fe_info, &dvbs2_info, sizeof (dvbs2_info));
    break;
  default:
    printk ("%s() invalid arg\n", __FUNCTION__);
    return -EINVAL;
  }

  return 0;
}

/* TODO: The hardware does DSS too, how does the kernel demux handle this? */
static int
cx21143_get_delsys (struct dvb_frontend *fe, enum dvbfe_delsys *fe_delsys)
{
  dprintk (10, "%s()\n", __FUNCTION__);
  *fe_delsys = DVBFE_DELSYS_DVBS | DVBFE_DELSYS_DVBS2;

  return 0;
}

/* TODO: is this necessary? */
static enum dvbfe_algo
cx21143_get_algo (struct dvb_frontend *fe)
{
  dprintk (10, "%s()\n", __FUNCTION__);
  return DVBFE_ALGO_SW;
}

static struct dvb_frontend_ops dvb_cx21143_fe_qpsk_ops;

struct dvb_frontend *
cx21143_fe_qpsk_attach (const struct cx21143_config *config)
{
  struct cx21143_state *state = NULL;
  int ret;

  dprintk (10, "%s: >\n", __FUNCTION__);

  /* allocate memory for the internal state */
  state = kmalloc (sizeof (struct cx21143_state), GFP_KERNEL);
  if (state == NULL)
  {
    return NULL;
  }

  /* setup the state */
  memcpy (&state->ops, &dvb_cx21143_fe_qpsk_ops,
          sizeof (struct dvb_frontend_ops));

  state->config = config;

  cx21143_reset (state);

  /* check if the demod is present */
  ret = (cx21143_readreg (state, 0xFF) << 8) | cx21143_readreg (state, 0xFE);
  if (ret != 0x0501)
  {
    printk ("ret = %02x\n", ret);
    printk ("Invalid probe, probably not a cx21143 device\n");
    kfree (state);
    return NULL;
  }

  state->not_responding = 0;

  /* create dvb_frontend */
  state->frontend.ops = state->ops;
  state->frontend.demodulator_priv = state;

#if defined(TUNER_PROCFS)
  state->value[0] = 0x01;
  state->value[1] = 0x75;
  state->value[2] = 0x00;
  state->value[3] = 0x03;
  state->value[4] = 0x00;
#endif

  return &state->frontend;
}

#if defined(TUNER_PROCFS)
static int
tuner_write_proc (struct file *file, const char __user * buf,
                  unsigned long count, void *data)
{
  char *page;
  ssize_t ret = -ENOMEM;
  struct cx21143_cmd cmd;
  struct dvb_frontend *fe = data;
  struct cx21143_state *state = fe->demodulator_priv;
  char s1[10], s2[10], s3[10], s4[10], s5[10];
  int result;

  page = (char *) __get_free_page (GFP_KERNEL);
  if (page)
  {
    ret = -EFAULT;
    if (copy_from_user (page, buf, count))
      goto out;

    result = sscanf (page, "%3s %3s %3s %3s %3s", s1, s2, s3, s4, s5);

    if (result != 5)
    {
      printk ("result = %d\n", result);
      return -EINVAL;
    }
    sscanf (s1, "%hhd", &state->value[0]);
    sscanf (s2, "%hhd", &state->value[1]);
    sscanf (s3, "%hhd", &state->value[2]);
    sscanf (s4, "%hhd", &state->value[3]);
    sscanf (s5, "%hhd", &state->value[4]);

    printk ("0x%x 0x%x 0x%x 0x%x 0x%x\n", state->value[0],
            state->value[1], state->value[2],
            state->value[3], state->value[4]);

    /* Firmware CMD 13: MPEG/TS output config */
    cmd.id = CMD_MPEGCONFIG;
    cmd.args[0x01] = state->value[0];
    cmd.args[0x02] = state->value[1];
    cmd.args[0x03] = state->value[2];
    cmd.args[0x04] = state->value[3];
    cmd.args[0x05] = state->value[4];

    ret = cx21143_cmd_execute (fe, &cmd);
    if (ret != 0)
    {
      ret = -EFAULT;
      goto out;
    }
    ret = count;

  }
out:
  free_page ((unsigned long) page);
  return ret;
}

static int
tuner_read_proc (char *page, char **start, off_t off, int count,
                 int *eof, void *data)
{
  int len;
  struct dvb_frontend *fe = data;
  struct cx21143_state *state = fe->demodulator_priv;

  dprintk (10, "%s %d\n", __FUNCTION__, count);

  len = sprintf (page, "%d %d %d %d %d\n", state->value[0],
                 state->value[1], state->value[2],
                 state->value[3], state->value[4]);
  *eof = 0;

  *start = page;
  return len;
}
#endif

static struct dvb_frontend *
init_cx21143_device (struct dvb_adapter *adapter,
                     struct plat_tuner_config *tuner_cfg)
{
  struct cx21143_state *state;
  struct dvb_frontend *frontend;
  struct cx21143_config *cfg;

  dprintk (1, "> (bus = %d) %s\n", tuner_cfg->i2c_bus,__FUNCTION__);

  cfg = kmalloc (sizeof (struct cx21143_config), GFP_KERNEL);
  if (cfg == NULL)
  {
    printk ("cx21143: kmalloc failed\n");
    return NULL;
  }

  /* initialize the config data */
  cfg->i2c_adap = i2c_get_adapter (tuner_cfg->i2c_bus);

#ifdef CONFIG_I2C_STM

#warning "fixme fixme fixme static funktion um speed zu setzen"

// FIXME: iic_stm_control ist leider static und nur per ioctl
// zu erreichen. muss ich mal aendern
			    
#endif

  cfg->i2c_addr = tuner_cfg->i2c_addr;

//hacky
  cfg->i2c_addr_lnb_supply = tuner_cfg->lnb_vsel[0];
  cfg->vertical = tuner_cfg->lnb_vsel[1];
  cfg->horizontal = tuner_cfg->lnb_vsel[2];

  dprintk(1, "tuner enable = %d.%d\n", tuner_cfg->tuner_enable[0], tuner_cfg->tuner_enable[1]);

  cfg->tuner_enable_pin = stpio_request_pin (tuner_cfg->tuner_enable[0],
                                          tuner_cfg->tuner_enable[1],
                                          "tuner enabl", STPIO_OUT);

  if ((cfg->i2c_adap == NULL) || (cfg->tuner_enable_pin == NULL))
  {

    printk ("cx21143: failed to allocate resources\n");
    if(cfg->tuner_enable_pin != NULL)
      stpio_free_pin (cfg->tuner_enable_pin);

    kfree (cfg);
    return NULL;
  }

  cfg->tuner_enable_act = tuner_cfg->tuner_enable[2];

  frontend = cx21143_fe_qpsk_attach (cfg);

  if (frontend == NULL)
  {
    return NULL;
  }

  dprintk (1, "%s: Call dvb_register_frontend (adapter = 0x%x)\n",
           __FUNCTION__, (unsigned int) adapter);

  if (dvb_register_frontend (adapter, frontend))
  {
    printk ("%s: Frontend registration failed !\n", __FUNCTION__);
    if (frontend->ops.release)
      frontend->ops.release (frontend);
    return NULL;
  }

  state = frontend->demodulator_priv;

  /* The semaphore should be initialized with 0 when using the
     nowait firmware request.
     When using the on-demand loading or the loader thread the
     semaphore should be set to 1. */
  sema_init(&state->fw_load_sem, 1);

#if defined(TUNER_PROCFS)
  /* FIXME: how many procfs entries are necessary? */
  if ((state->proc_tuner = create_proc_entry ("tuner", 0, proc_root_driver)))
  {
    state->proc_tuner->data = frontend;
    state->proc_tuner->read_proc = tuner_read_proc;
    state->proc_tuner->write_proc = tuner_write_proc;
  }
  else
    printk ("%s: failed to create procfs entry\n", __FUNCTION__);
#endif

  return frontend;
}

static void
update_cx21143_devices (void)
{
  int i, j;

  dprintk (1, "%s()\n", __FUNCTION__);

  /* loop over the configuration entries and try to
     register new devices */
  for (i = 0; i < MAX_DVB_ADAPTERS; i++)
  {
    if (core[i] == NULL)
    {
      break;
    }

    for (j = 0; j < config_data[i].count; j++)
    {
      if (core[i]->frontend[j] == NULL)
      {
        /* found new entry for this adapter */
        core[i]->frontend[j] = init_cx21143_device (core[i]->dvb_adap,
                                                    &config_data[i].
                                                    tuner_cfg[j]);
      }
    }
  }
}

/*
* This function only retrieves the configuration data.
*/
static int
cx21143_probe (struct device *dev)
{
  struct platform_device *pdev = to_platform_device (dev);
  struct plat_tuner_data *plat_data = pdev->dev.platform_data;
  int i;

  dprintk (1, "> %s\n", __FUNCTION__);

  if (plat_data == NULL)
  {
    printk ("cx21143_probe: no platform device data found\n");
    return -1;
  }

  /* loop over the list of provided devices and add the new ones
     to the configuration array */
  for (i = 0; i < plat_data->num_entries; i++)
  {
    if ((plat_data->tuner_cfg[i].adapter >= 0) &&
        (plat_data->tuner_cfg[i].adapter < MAX_DVB_ADAPTERS))
    {
      /* check whether the configuration already exists */
      int index = plat_data->tuner_cfg[i].adapter;
      int j;

      for (j = 0; j < config_data[index].count; j++)
      {
        if ((plat_data->tuner_cfg[i].i2c_bus ==
             config_data[index].tuner_cfg[j].i2c_bus) &&
            (plat_data->tuner_cfg[i].i2c_addr ==
             config_data[index].tuner_cfg[j].i2c_addr))
        {
          /* same bus, same address - discard */
          break;
        }
      }

      if ((j == config_data[index].count)
          && (j < (MAX_TUNERS_PER_ADAPTER - 1)))
      {
        /* a new config entry */
        dprintk (1, "cx21143: new device config i2c(%d, %d)\n",
                plat_data->tuner_cfg[i].i2c_bus,
                plat_data->tuner_cfg[i].i2c_addr);
        config_data[index].tuner_cfg[j] = plat_data->tuner_cfg[i];
        config_data[index].count++;
      }
    }
  }

  update_cx21143_devices ();

  dprintk (1, "%s >\n", __FUNCTION__);

  return 0;
}

static int
cx21143_remove (struct device *dev)
{
  /* TODO: add code to free resources */

  dprintk (1, "%s: not implemented yet\n", __FUNCTION__);
  return 0;
}

static struct device_driver cx21143_driver = {
  .name = "cx21143",
  .bus = &platform_bus_type,
  .owner = THIS_MODULE,
  .probe = cx21143_probe,
  .remove = cx21143_remove,
};

/* FIXME: move the tuner configuration data either to the
   board/stb71xx/setup.c or to a module handling configuration */
struct plat_tuner_config tuner_resources[] = {
	/* ufs922 tuner resources */
        [0] = {
                .adapter = 0,
                .i2c_bus = 0,
                .i2c_addr = 0x0a >> 1,
                .tuner_enable = {2, 4, 1},
                .lnb_enable = {-1, -1, -1},
//hacky
//              .lnb_vsel = {0x08, 0xd4, 0xdc},
//GOst4711 setze LNBH23 als Standart
                .lnb_vsel = {0x0a, 0xd4, 0xdc},
        },
        [1] = {
                .adapter = 0,
                .i2c_bus = 1,
                .i2c_addr = 0x0a >> 1,
                .tuner_enable = {2, 5, 1},
                .lnb_enable = {-1, -1, -1},
//hacky
//              .lnb_vsel = {0x08, 0xd4, 0xdc},
//GOst4711 setze LNBH23 als Standart
                .lnb_vsel = {0x0a, 0xd4, 0xdc},
        },
};

struct plat_tuner_data tuner_data = {
        .num_entries = ARRAY_SIZE(tuner_resources),
        .tuner_cfg = tuner_resources
};

static struct platform_device tuner_device = {
        .name           = "cx21143",
        .dev = {
                .platform_data = &tuner_data
        }
};

static struct platform_device *pvr_devices[] =
{
	&tuner_device,
};

void cx21143_register_frontend(struct dvb_adapter *dvb_adap)
{
  int i = 0;
  dprintk (1, "%s: cx21143 DVB: 0.50 \n", __FUNCTION__);

  /* TODO: add an array of cores to support multiple adapters */
  if (core[i])
  {
    printk ("cx21143 core already registered\n");
    return;
  }

  core[i] =
    (struct cx21143_core *) kmalloc (sizeof (struct cx21143_core),
                                     GFP_KERNEL);
  if (!core[i])
  {
    printk ("%s: Out of mem\n", __FUNCTION__);
    return;
  }
  memset (core[0], 0, sizeof (*core[i]));

  core[i]->dvb_adap = dvb_adap;

  dvb_adap->priv = core[i];

  if (driver_register (&cx21143_driver) < 0)
  {
    printk ("cx21143: error registering device driver\n");
  }
  else
  {
    /* check whether there is anything to update */
    update_cx21143_devices ();
  }

  /* FIXME: remove the resource registration once the
     configuration is provided externally */
  dprintk(1, "adding tuner configuration data\n");
  platform_add_devices(pvr_devices,  ARRAY_SIZE(pvr_devices));

  dprintk (1, "%s: <\n", __FUNCTION__);

  return;
}

EXPORT_SYMBOL(cx21143_register_frontend);

static struct dvb_frontend_ops dvb_cx21143_fe_qpsk_ops = {
  .info = {
           .name = "Conexant cx21143 DVB-S2",
           .type = FE_QPSK,
           .frequency_min = 950000,
           .frequency_max = 2150000,
           .frequency_stepsize = 1011,  /* kHz for QPSK frontends */
           .frequency_tolerance = 5000,
           .symbol_rate_min = 1000000,
           .symbol_rate_max = 45000000,

           .caps =              /* FE_CAN_INVERSION_AUTO | */// (ab)use inversion for pilot tones
           FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4 |
           FE_CAN_FEC_4_5 | FE_CAN_FEC_5_6 | FE_CAN_FEC_6_7 |
           FE_CAN_FEC_7_8 | FE_CAN_FEC_AUTO | FE_CAN_QPSK | FE_CAN_RECOVER},

  .release = cx21143_release,

  .init = cx21143_fe_init,
  .sleep = cx21143_sleep,

  .set_frontend = cx21143_set_frontend,
  .get_frontend = cx21143_get_frontend,

  .read_status = cx21143_read_status,
  .read_ber = cx21143_read_ber,
  .read_signal_strength = cx21143_read_signal_strength,
  .read_snr = cx21143_read_snr,
  .read_ucblocks = cx21143_read_ucblocks,

  .set_voltage = cx21143_set_voltage,
  .set_tone = cx21143_set_tone,

  .diseqc_send_master_cmd = cx21143_send_diseqc_msg,
  .diseqc_send_burst = cx21143_diseqc_send_burst,

  .get_tune_settings = cx21143_get_tune_settings,

  .get_info = cx21143_get_info,
  .get_delsys = cx21143_get_delsys,
  .get_frontend_algo = cx21143_get_algo,

  .set_params = cx21143_set_params,
  .get_params = cx21143_get_params,
};

int __init cx21143_init(void)
{
    dprintk(1,"cx21143 loaded\n");
    return 0;
}

static void __exit cx21143_exit(void)
{   
   dprintk(1,"cx21143 unloaded\n");
}

module_param(paramDebug, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(paramDebug, "Debug Output 0=disabled >0=enabled(debuglevel)");

module_init             (cx21143_init);
module_exit             (cx21143_exit);

MODULE_DESCRIPTION      ("Tunerdriver");
MODULE_AUTHOR           ("Dagobert");
MODULE_LICENSE          ("GPL");
