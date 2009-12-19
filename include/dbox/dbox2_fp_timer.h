/*
 * $Id: dbox2_fp_timer.h,v 1.3 2002/12/18 19:07:03 Zwen Exp $
 *
 * Copyright (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
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

#ifdef __KERNEL__
#ifndef __dbox2_fp_timer_h__
#define __dbox2_fp_timer_h__

#include <linux/types.h>

#define FP_WAKEUP_NOKIA		0x11
#define FP_WAKEUP_PHILIPS	0x01
#define FP_WAKEUP_SAGEM		0x01
#define FP_STATUS		0x20
#define FP_CLEAR_WAKEUP_NOKIA		0x2B
#define FP_CLEAR_WAKEUP_SAGEM		0x21

#define BOOT_TRIGGER_USER	0x00
#define BOOT_TRIGGER_TIMER	0x01

void dbox2_fp_timer_init (void);
int dbox2_fp_timer_set (u16 minutes);
int dbox2_fp_timer_get (void);
int dbox2_fp_timer_clear (void);
u8 dbox2_fp_timer_get_boot_trigger (void);

#endif /* __dbox2_fp_timer_h__ */
#endif /* __KERNEL__ */
