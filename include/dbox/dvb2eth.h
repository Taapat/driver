/*
 * AViA GTX/eNX demux and ethernet "busmaster" driver definitions
 *
 * Homepage: http://www.tuxbox.org
 *
 * Copyright (C) 2004 Wolfram Joost <dbox2@frokaschwei.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*
 * IOCTL-Parm DB2ETH_SET_DEST - sets destination of network packets
 */

struct dvb2eth_dest {
	unsigned long	dest_ip;
	unsigned short	dest_port;
	unsigned short	src_port;
	unsigned char	dest_mac[6];
	unsigned char	ttl;
};

/*
 * Callback-function. transmits the data
 */

#ifdef __KERNEL__

typedef int (*dvb2eth_callback) (unsigned, unsigned ,unsigned, unsigned);

extern dvb2eth_callback avia_gt_napi_dvr_send;

#endif

/*
 * IOCTL-definitions
 */

#define DVB2ETH_IOCTL_BASE	'o'
#define DVB2ETH_STOP		_IO(DVB2ETH_IOCTL_BASE,242)
#define DVB2ETH_START		_IO(DVB2ETH_IOCTL_BASE,243)
#define DVB2ETH_SET_DEST	_IOW(DVB2ETH_IOCTL_BASE,244,sizeof(struct dvb2eth_dest))
