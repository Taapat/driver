/*
 * $Id: dbox2_fp_irkbd.h,v 1.1 2002/10/21 11:38:59 obi Exp $
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
#ifndef __dbox2_fp_irkbd__
#define __dbox2_fp_irkbd__

#include <linux/config.h>
#include <linux/init.h>

#ifdef CONFIG_INPUT_MODULE
#include <linux/input.h>
#endif /* CONFIG_INPUT_MODULE */

#define IRKBD_MOUSER	0x02
#define IRKBD_MOUSEL	0x04
#define IRKBD_FN	0x80

#ifdef CONFIG_INPUT_MODULE
#define DIR_RIGHT_UP	0x01
#define DIR_UP_RIGHT	0x02
#define DIR_UP_LEFT	0x04
#define DIR_LEFT_UP	0x08
#define DIR_LEFT_DOWN	0x10
#define DIR_DOWN_LEFT	0x20
#define DIR_DOWN_RIGHT	0x40
#define DIR_RIGHT_DOWN	0x80
#endif /* CONFIG_INPUT_MODULE */

/* linux kernel api */
int dbox2_fp_irkbd_getkeycode (unsigned int scancode);
int dbox2_fp_irkbd_setkeycode (unsigned int scancode, unsigned int keycode);
int dbox2_fp_irkbd_translate (unsigned char scancode, unsigned char *keycode, char raw_mode);
char dbox2_fp_irkbd_unexpected_up (unsigned char keycode);
void dbox2_fp_irkbd_leds (unsigned char leds);
void __init dbox2_fp_irkbd_init_hw (void);

#ifdef CONFIG_INPUT_MODULE
int dbox2_fp_irkbd_event (struct input_dev * dev, unsigned int type, unsigned int code, int value);
#endif /* CONFIG_INPUT_MODULE */

/* internal kernel api */
void dbox2_fp_irkbd_init (void);
void dbox2_fp_irkbd_exit (void);
void dbox2_fp_handle_ir_keyboard (struct fp_data * dev);
void dbox2_fp_handle_ir_mouse (struct fp_data * dev);

#endif /* __dbox2_fp_irkbd__ */
#endif /* __KERNEL__ */

