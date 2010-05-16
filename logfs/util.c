/* $Id: util.c,v 1.4 2005/07/21 02:42:14 ppadala Exp $
 *
 * Copyright (c) 2005 Pradeep Padala. All rights reserved
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "include/lfs_kernel.h"

void lfs_error (struct super_block * sb, const char * function,
		 const char * fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	dprintk(KERN_CRIT "LFS error (device %s): %s: ",sb->s_id, function);
	printk(fmt, args);
	dprintk("\n");
	va_end(args);
}

void lfs_warning (struct super_block * sb, const char * function,
		   const char * fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	dprintk(KERN_WARNING "LFS warning (device %s): %s: ",
	       sb->s_id, function);
	printk(fmt, args);
	dprintk("\n");
	va_end(args);
}


