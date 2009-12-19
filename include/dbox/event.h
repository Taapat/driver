/*
 * $Id: event.h,v 1.7 2003/09/08 22:38:57 obi Exp $
 *
 * global event driver (dbox-II-project)
 *
 * Homepage: http://www.tuxbox.org
 *
 * Copyright (C) 2001 Gillem (gillem@berlios.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef EVENT_H
#define EVENT_H

/* global event defines
 */
#define EVENT_NOP		0
#define EVENT_VCR_CHANGED	1
#define EVENT_VHSIZE_CHANGE	4
#define EVENT_ARATIO_CHANGE	8
#define EVENT_SBTV_CHANGE	16	/* avs event pin 8 tv */
#define EVENT_SBVCR_CHANGE	32	/* avs event pin 8 vcr */

#define EVENT_SET_FILTER	1

/* other data
 */

struct event_t {
	unsigned int event;
};

#ifdef __KERNEL__
void event_write_message(struct event_t *buf, size_t count);
#endif

#endif
