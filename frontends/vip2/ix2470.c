/*
	IX2470 8PSK/QPSK tuner driver
	Copyright (C) Manu Abraham <abraham.manu@gmail.com>

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


#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "dvb_frontend.h"
#include "ix2470.h"

struct ix2470_state {
	struct dvb_frontend		*fe;
	struct i2c_adapter		*i2c;
	const struct ix2470_config	*config;

	/* state cache */
	u32 frequency;
	u32 bandwidth;
};

static int ix2470_write(struct ix2470_state *state, u8 *buf, u8 len)
{
	const struct ix2470_config *config = state->config;
	int err = 0;
	struct i2c_msg msg = { .addr = config->addr, .flags = 0, .buf = buf, .len = len };

	err = i2c_transfer(state->i2c, &msg, 1);
	if (err != 1)
		printk("%s: read error, err=%d\n", __func__, err);

	return err;
}

static int ix2470_read(struct ix2470_state *state, u8 *buf)
{
	const struct ix2470_config *config = state->config;
	int err = 0;
	u8 tmp;

	struct i2c_msg msg[] = {
		{ .addr = config->addr, .flags = 0,	   .buf = &tmp, .len = 1 },
		{ .addr = config->addr, .flags = I2C_M_RD, .buf = buf,  .len = 2 }
	};

	err = i2c_transfer(state->i2c, msg, 2);
	if (err != 2)
		printk("%s: read error, err=%d\n", __func__, err);

	return err;
}

static int ix2470_calc_vco(u32 freq, u8 *buf)
{
	int PSC;
	int DIV, BA210;

	if ((freq < 950000) || (freq > 2150000))
		goto exit;

	if (freq < 986000) {
		PSC = 1;
		DIV = 1;
		BA210 = 0x05;
	}

	if (986000 <= freq && freq < 1073000) {
		PSC = 1;
		DIV = 1;
		BA210 = 0x06;
	}

	if (1073000 <= freq && freq < 1154000) {
		PSC = 0;
		DIV = 1;
		BA210 = 0x07;
	}

	if (1154000 <= freq && freq < 1291000) {
		PSC = 0;
		DIV = 0;
		BA210 = 0x01;
	}

	if (1291000 <= freq && freq < 1447000) {
		PSC = 0;
		DIV = 0;
		BA210 = 0x02;
	}

	if (1447000 <= freq && freq < 1615000) {
		PSC = 0;
		DIV = 0;
		BA210 = 0x03;
	}

	if (1615000 <= freq && freq < 1791000) {
		PSC = 0;
		DIV = 0;
		BA210 = 0x04;
	}

	if (1791000 <= freq && freq < 1972000) {
		PSC = 0;
		DIV = 0;
		BA210 = 0x05;
	}

	if (1972000 <= freq) {
		PSC = 0;
		DIV = 0;
		BA210 = 0x06;
	}

	buf[4] &= 0xef;
	buf[4] |= PSC << 4;

	buf[4] &= 0x1d;
	buf[4] |= DIV << 1;
	buf[4] |= BA210 << 5;

	return 0;
exit:
	printk("%s: Frequency out of bounds, freq=%d\n", __func__, freq);
	return -EINVAL;
}

static u32 ix2470_calc_steps(u8 byte4)
{
	int REF;
	int R;

	REF = byte4 & 0x01;
	if (REF == 0)
		R = 4;
	else
		R = 8;

	return 4000 / R;
}

inline int ix2470_do_div(u64 n, u32 d)
{
	do_div(n, d);
	return n;
}

void ix2470_calc_plldiv(u32 freq, u8 *buf)
{
	int data;
	int P, N, A;
	int PSC;

	data = ix2470_do_div(freq, ix2470_calc_steps(buf[3]) + 1);

	PSC  = (buf[4] >> 4);
	PSC &= 0x01;
	if (PSC)
		P = 16; /* PSC=1 */
	else
		P = 32; /* PSC=0 */

	N = data / P;
	A = data - P * N;
	data = (N << 5) | A;

	buf[1] &= 0x60; /* BG 1/0 enabled */
	buf[1] |= (data >> 8) & 0x1f;
	buf[2]  =  data & 0xff;
}

static int ix2470_pll_setdata(struct ix2470_state *state, u8 *buf)
{
	int err = 0;

	u8 data[5];
	u8 b2, b4, b5;

	b2 = buf[1];
	b4 = buf[3];
	b5 = buf[4];

	buf[3] &= 0xe3; /* TM=0, LPF=4MHz */
	buf[4] &= 0xf3; /* LPF=4MHz */

	buf[1] &= 0x9f;
	buf[1] |= 0x20;

	err = ix2470_write(state, buf, 5);
	if (err < 0)
		goto exit;

	buf[3] |= 0x04; /* TM=1 */

	data[0] = buf[0];
	data[1] = buf[3];
	err = ix2470_write(state, data, 2);
	if (err < 0)
		goto exit;

	msleep(50);

	buf[3]  = b4; /* restore original */
	buf[3] |= 0x04; /* TM=1 */

	buf[4] = b5; /* restore original */

	data[0] = buf[0];
	data[1] = buf[3];
	data[2] = buf[4];
	err = ix2470_write(state, data, 3);
	if (err < 0)
		goto exit;

	buf[1] = b2;
	err = ix2470_write(state, buf, 2); /* restore BG value */
	if (err < 0)
		goto exit;

	return 0;
exit:
	printk("%s: I/O error\n", __func__);
	return err;
}

static int ix2470_set_state(struct dvb_frontend *fe,
			    enum tuner_param param,
			    struct tuner_state *tstate)
{
	struct ix2470_state *state = fe->tuner_priv;

	int err = 0;
	u8 buf[5];

	if (param & DVBFE_TUNER_FREQUENCY) {
		err = ix2470_calc_vco(tstate->frequency, buf);
		if (err < 0) {
			printk("%s: VCO calculation failed\n", __func__);
			goto exit;
		}
		ix2470_calc_plldiv(tstate->frequency, buf);
		err = ix2470_pll_setdata(state, buf);
		if (err < 0) {
			printk("%s: PLL setup failed\n", __func__);
			goto exit;
		}
	} else {
		printk("%s: Unknown parameter (param=%d)\n", __func__, param);
		return -EINVAL;
	}

	return 0;
exit:
	printk("%s: Setup failed\n", __func__);
	return -1;
}

static int ix2470_get_state(struct dvb_frontend *fe,
			     enum tuner_param param,
			     struct tuner_state *tstate)
{
	struct ix2470_state *state = fe->tuner_priv;
	int err = 0;

	switch (param) {
	case DVBFE_TUNER_FREQUENCY:
		tstate->frequency = state->frequency;
		break;
	case DVBFE_TUNER_BANDWIDTH:
		tstate->bandwidth = 40000000; /* FIXME! need to calculate Bandwidth */
		break;
	default:
		printk("%s: Unknown parameter (param=%d)\n", __func__, param);
		err = -EINVAL;
		break;
	}

	return err;
}

static int ix2470_get_status(struct dvb_frontend *fe, u32 *status)
{
	struct ix2470_state *state = fe->tuner_priv;
	u8 result[2];
	int err = 0;

	*status = 0;

	err = ix2470_read(state, result);
	if (err < 0) {
		printk("%s: I/O Error\n", __func__);
		return err;
	}

	if ((result[1] >> 6) & 0x01) {
		printk("%s: Tuner Phase Locked\n", __func__);
		*status = 1;
	}

	return err;
}

static int ix2470_release(struct dvb_frontend *fe)
{
	struct ix2470_state *state = fe->tuner_priv;

	fe->tuner_priv = NULL;
	kfree(state);
	return 0;
}

static struct dvb_tuner_ops ix2470_ops = {

	.info = {
		.name		= "IX2470",
		.frequency_min	=  950000,
		.frequency_max	= 2150000,
		.frequency_step = 0
	},

	.set_state	= ix2470_set_state,
	.get_state	= ix2470_get_state,
	.get_status	= ix2470_get_status,
	.release	= ix2470_release
};

struct dvb_frontend *ix2470_attach(struct dvb_frontend *fe,
				   const struct ix2470_config *config,
				   struct i2c_adapter *i2c)
{
	struct ix2470_state *state = NULL;

	if ((state = kzalloc(sizeof (struct ix2470_state), GFP_KERNEL)) == NULL)
		goto exit;

	state->config		= config;
	state->i2c			= i2c;
	state->fe			= fe;
	fe->tuner_priv		= state;
	fe->ops.tuner_ops	= ix2470_ops;

	printk("%s: Attaching %s IX2470 8PSK/QPSK tuner\n",
		__func__, config->name);

	return fe;

exit:
	kfree(state);
	return NULL;
}

EXPORT_SYMBOL(ix2470_attach);
