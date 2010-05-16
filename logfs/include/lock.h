/* $Id: lock.h,v 1.3 2005/08/23 20:31:35 ppadala Exp $
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

#ifndef _LOCK_H
#define _LOCK_H

/* IFILE inode is protected by i_alloc_sem */
#define iread_lock(sb)	down_read(&CURR_IFILE_INODE(sb)->i_alloc_sem)
#define iread_unlock(sb) up_read(&CURR_IFILE_INODE(sb)->i_alloc_sem)
#define iwrite_lock(sb)	down_write(&CURR_IFILE_INODE(sb)->i_alloc_sem)
#define iwrite_unlock(sb) up_write(&CURR_IFILE_INODE(sb)->i_alloc_sem)

/* No more per segment locks, all segments are portected by a single lock */
#define seg_read_lock(sb)	down_read(&LFS_SBI(sb)->seg_sem)
#define seg_read_unlock(sb)	up_read(&LFS_SBI(sb)->seg_sem)
#define seg_write_lock(sb)	down_write(&LFS_SBI(sb)->seg_sem)
#define seg_write_unlock(sb)	up_write(&LFS_SBI(sb)->seg_sem)

static inline void lfs_lock(struct super_block *sb)
{
	seg_write_lock(sb);
	iwrite_lock(sb);
}

static inline void lfs_unlock(struct super_block *sb)
{
	iwrite_unlock(sb);
	seg_write_unlock(sb);
}

#endif
