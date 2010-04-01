/*
	IX2470 8PSK/QPSK tuner driver
	Copyright (C) Manu Abraham <abraham.manu@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __IX2470_H
#define __IX2470_H

enum ix2470_step {
	IX2470_STEP_1000 = 0,	/* 1000 kHz */
	IX2470_STEP_500		/*  500 kHz */
};

enum ix2470_bblpf {
	IX2470_LPF_10 = 3,	/*   10 MHz */
	IX2470_LPF_12,		/*   12 MHz */
	IX2470_LPF_14,		/*   14 MHz */
	IX2470_LPF_16,		/*   16 MHz */
	IX2470_LPF_18,		/*   18 MHz */
	IX2470_LPF_20,		/*   20 MHz */
	IX2470_LPF_22,		/*   22 MHz */
	IX2470_LPF_24,		/*   24 MHz */
	IX2470_LPF_26,		/*   26 MHz */
	IX2470_LPF_28,		/*   28 MHz */
	IX2470_LPF_30,		/*   30 MHz */
	IX2470_LPF_32,		/*   32 MHz */
	IX2470_LPF_34,		/*   34 MHz */
};

enum ix2470_bbgain {
	IX2470_GAIN_0dB = 1,	/*  0dB Att */
	IX2470_GAIN_2dB,	/* -2dB Att */
	IX2470_GAIN_4dB		/* -4dB Att */
};

struct ix2470_config {
	u8 			name[32];
	u8 			addr;
	enum ix2470_step 	step_size;
	enum ix2470_bblpf	bb_lpf;
	enum ix2470_bbgain	bb_gain;
};

extern struct dvb_frontend *ix2470_attach(struct dvb_frontend *fe,
					  const struct ix2470_config *config,
					  struct i2c_adapter *i2c);

#endif /* __IX2470_H */
