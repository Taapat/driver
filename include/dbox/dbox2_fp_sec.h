/*
 * $Id: dbox2_fp_sec.h,v 1.2 2003/02/09 19:53:34 obi Exp $
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
#ifndef __dbox2_fp_sec_h__
#define __dbox2_fp_sec_h__

#include <linux/types.h>

/* internal kernel api */
void dbox2_fp_sec_init (void);

/* public kernel api */
int dbox2_fp_sec_diseqc_cmd (u8 *cmd, u8 len);
int dbox2_fp_sec_get_status (void);
int dbox2_fp_sec_set_high_voltage (u8 high_voltage);
int dbox2_fp_sec_set_power (u8 power);
int dbox2_fp_sec_set_tone (u8 tone);
int dbox2_fp_sec_set_voltage (u8 voltage);

#endif /* __dbox2_fp_sec_h__ */
#endif /* __KERNEL__ */

