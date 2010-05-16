/* $Id: ifile.c,v 1.7 2005/08/23 20:28:41 ppadala Exp $
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

/* Routines for manipulating the LFS IFILE */

#include <linux/buffer_head.h>

#include "include/lfs.h"
#include "include/lfs_kernel.h"

/* Returns the next available inode number in the IFILE */
ino_t ifile_get_next_free_inode(struct super_block *sb)
{
	ino_t ino = 0;
	struct ifile *ifile = IFILE(sb);

	ino = ifile->next_free;
	/* LFS can create as many inodes as we want, since the inode
	   map is a file. However, we are limited by the number of bits
	   available in ino_t */
	if(ino == LFS_MAX_INODES)
		return -1;
	++ifile->n_ife;
	++ifile->next_free;
	return ino;
}

/* returns the iblock containing the ife for the given inode */
static inline sector_t get_iblock_nr(ino_t ino)
{	int offset; 
	sector_t iblock;

	offset = IFE_SIZE * (ino - 1);
	iblock = offset / LFS_BSIZE;

	return iblock;
}

/* Reads the ife for the corresponding inode and returns the buffer head
 * containing it. Note that *pife points to the ifile_entry within the 
 * returned buffer head data
 */
static struct buffer_head *ifile_read_ife(struct super_block *sb, ino_t ino, 
			     		struct ifile_entry **pife)
{
	int offset, iblock;
	struct buffer_head *bh;
	struct inode *ifile_inode = IFILE_INODE(sb,CURR_SNAPI(sb));

	/* read the block from the disk */
	iblock = get_iblock_nr(ino);
	bh = lfs_read_block(ifile_inode, iblock);
	if(!bh)
		return NULL;

	offset = IFE_SIZE * (ino - 1);
	offset -= iblock * LFS_BSIZE;
	*pife = (struct ifile_entry *)((char *)bh->b_data + offset);

	return bh; /* caller has to release the bh, after done with *pife */
}

static inline int is_new_inode(ino_t ino, struct inode *ifile)
{
	int size = ifile->i_size, index;

	index = size/IFE_SIZE;
	//dprintk("index = %d, ino = %lu\n", index, ino);
	if(ino > index)
		return 1;
	return 0;
}

/* Primary function to update the ifile. Updates the ifile entry for
 * the partiulcar inode with @ife. Note that the ifile entry might already
 * exist and the caller may just be updating the entry
 */
void ifile_update_ife(struct inode *inode, struct ifile_entry *ife)
{
	int ino, offset;
	sector_t newblock;
	struct buffer_head *bh;
	struct super_block *sb = inode->i_sb;
	struct inode *ifile = IFILE_INODE(sb, CURR_SNAPI(sb));
	struct ifile_entry *pife = NULL;

	ino = inode->i_ino;

	if(is_new_inode(ino, ifile))
		ifile->i_size += IFE_SIZE;
	bh = ifile_read_ife(sb, ino, &pife);
	/* update the IFILE */
	if(bh) {
		/* updating the in-memory contents */
		offset = IFE_SIZE * (ino - 1);
		offset = offset % LFS_BSIZE;
		//dprintk("Updating ifile at offset = %d\n", offset);

		newblock = segment_write_block_from_bh(
			 	sb, ife, IFE_SIZE, offset, bh);
		if(newblock != bh->b_blocknr)
			/* FIXME: need to update segusetbl */
			update_inode(ifile, get_iblock_nr(ino), newblock);
		if(is_new_inode(ino, ifile))
			segusetbl_add_livebytes(sb, dtosn(newblock), IFE_SIZE);
		brelse(bh);
	}
	else {	
		/* new entry for ifile */
		offset = 0;
		newblock = segment_write_block(	sb, ife, IFE_SIZE, offset, 0);
		segusetbl_add_livebytes(sb, dtosn(newblock), IFE_SIZE);
		update_inode(ifile, get_iblock_nr(ino), newblock);
		//dprintk("updating %Lu to %Lu block\n", get_iblock_nr(ino), newblock);
	}
	mark_inode_dirty(ifile);
}

static void read_or_init_ife(struct super_block *sb, ino_t ino, 
			     struct ifile_entry *ife)
{
	struct buffer_head *bh;
	struct ifile_entry *pife = NULL;

	dprintk("read_or_init called for ino %lu\n", ino);
	memset(ife, 0, IFE_SIZE);
	bh = ifile_read_ife(sb, ino, &pife);
	if(bh) {
		if(pife->ife_daddr != LFS_INODE_FREE) { 
			dprintk("read_or_init: Returning an existing ife\n");
			ife->ife_version = pife->ife_version;
			ife->ife_daddr = pife->ife_daddr;
			ife->ife_atime = pife->ife_atime;
			brelse(bh);
			return;
		}
		brelse(bh);
	}

	dprintk("read_or_init: Creating new ife\n");
	ife->ife_version = 1;
	ife->ife_daddr = LFS_INODE_FREE;
	ife->ife_atime = CURRENT_TIME.tv_sec;
}

void ifile_update_ife_block(struct inode *inode, sector_t block)
{
	struct ifile_entry ife;

	read_or_init_ife(inode->i_sb, inode->i_ino, &ife);
	ife.ife_daddr = block;
	ifile_update_ife(inode, &ife);
}

/* 
 * Returns the disk block addr corresponding to a particular
 * inode
 */
sector_t ifile_get_daddr(struct super_block *sb, ino_t ino)
{
	sector_t block;
	struct buffer_head *bh;
	struct ifile_entry *pife = NULL;

	bh = ifile_read_ife(sb, ino, &pife);
	if(!pife)
		return 0;
	block = pife->ife_daddr;
	brelse(bh);

	return block;
}

void ifile_delete_inode(struct inode *inode)
{
	struct ifile_entry ife;
	struct super_block *sb = inode->i_sb;

	dprintk("ifile_delete_inode for ino %lu\n", inode->i_ino);
	segusetbl_dec_livebytes(sb, dtosn(ifile_get_daddr(sb, inode->i_ino)), 
				LFS_INODE_SIZE);
	ife.ife_daddr = LFS_INODE_FREE;
	++ife.ife_version;
	ifile_update_ife(inode, &ife);
	--IFILE(sb)->n_ife;
}
