/* $Id: ifile.h,v 1.3 2005/08/23 20:30:45 ppadala Exp $
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

/* Exported interfaces for IFILE manipulation */

#ifndef _LFS_IFILE_H
#define _LFS_IFILE_H

#include "lfs.h"
#include "lfs_kernel.h"

#define IFE_SIZE (sizeof(struct ifile_entry))

ino_t ifile_get_next_free_inode(struct super_block *sb);

/* updates */
void ifile_update_ife(struct inode *inode, struct ifile_entry *ife);
void ifile_update_ife_block(struct inode *inode, sector_t block);
void ifile_delete_inode(struct inode *inode);

/* read */
sector_t ifile_get_daddr(struct super_block *sb, ino_t ino);

#endif
