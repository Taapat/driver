/* $Id: tree.h,v 1.6 2005/08/14 08:28:49 ppadala Exp $
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

#ifndef _TREE_H
#define _TREE_H
/* structures for tree manipulation */
typedef struct {
	__u32  *p;
	__u32  key;
	struct buffer_head *bh;
} Indirect;


int lfs_map_block(struct inode * inode, sector_t block, 
			 struct buffer_head * bh, int create);
int update_inode(struct inode *inode, sector_t iblock, sector_t block);
sector_t lfs_disk_block(struct inode *inode, sector_t iblock);
struct buffer_head *lfs_read_block(struct inode *inode, sector_t iblock);
void free_inode_blocks(struct inode *inode);

#endif
