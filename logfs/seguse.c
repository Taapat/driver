/* $Id: seguse.c,v 1.4 2005/08/18 08:31:13 ppadala Exp $
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

/* Routines for manipulating the LFS seguse_table */

#include <linux/buffer_head.h>
#include "include/lfs_kernel.h"

/* returns the iblock containing the segusage etnry for a given segment */
static inline sector_t get_iblock_nr(int segnum)
{	int offset; 
	sector_t iblock;

	offset = LFS_CI_SIZE + segnum * LFS_SEGUSE_SIZE;
	iblock = offset / LFS_BSIZE;

	return iblock;
}

/* returns the number of seguse entries in segusetbl */
static inline int nseguse(struct inode *inode)
{
	size_t size;

	size = inode->i_size;
	size -= LFS_CI_SIZE;
	return size / LFS_SEGUSE_SIZE; 
}

/* Reads the seguse entry for the corresponding segment and returns the 
 * buffer head containing it. Note that *pseguse points to the segusage within 
 * the returned buffer head data
 */
static struct buffer_head *segusetbl_read_entry(
	struct super_block *sb, int segnum, struct segusage **pseguse)
{
	int offset, iblock;
	struct buffer_head *bh;
	struct inode *inode = SEGUSETBL_INODE(sb);

	if(segnum > nseguse(inode) - 1) /* new entry */
		return NULL;

	/* read the block from the disk */
	iblock = get_iblock_nr(segnum);
	//dprintk("iblock for segusetable = %d\n", iblock);
	bh = lfs_read_block(inode, iblock);
	if(!bh)
		return NULL;

	offset = LFS_CI_SIZE + LFS_SEGUSE_SIZE * segnum;
	offset -= iblock * LFS_BSIZE;
	*pseguse = (struct segusage *)((char *)bh->b_data + offset);

	return bh; /* caller has to release the bh, after done with *pseguse */
}

static inline int is_new_segment(int segnum, struct inode *inode)
{
	int size = inode->i_size - LFS_CI_SIZE;
	int nseg = size / LFS_SEGUSE_SIZE - 1;

	//dprintk("segindex = %d, nseg = %d\n", segindex, nseg);
	if(segnum > nseg)
		return 1;
	return 0;
}


/* Primary function to update the seguse_table. Updates the segusage entry for
 * the partiulcar segment.
 */
void segusetbl_update_entry(struct super_block *sb, int segnum, 
			    struct segusage *seguse)
{
	int offset;
	sector_t newblock;
	struct buffer_head *bh;
	struct inode *inode = SEGUSETBL_INODE(sb);
	struct segusage *pseguse = NULL;

	if(is_new_segment(segnum, inode))
		inode->i_size += LFS_SEGUSE_SIZE;
	bh = segusetbl_read_entry(sb, segnum, &pseguse);
	/* update the seguse_table */
	if(bh) {
		/* updating the in-memory contents */
		//dprintk("Updating in-memory contents\n");
		offset = LFS_CI_SIZE + LFS_SEGUSE_SIZE * segnum;
		offset = offset % LFS_BSIZE;
		//dprintk("Updating at offset = %d\n", offset);
		newblock = segment_write_block_from_bh(
			 	sb, seguse, LFS_SEGUSE_SIZE, offset, bh);
		if(newblock != bh->b_blocknr)
			update_inode(inode, get_iblock_nr(segnum), newblock);
		brelse(bh);
	}
	else {	
		/* new seguse entry */
		offset = 0;
		newblock = segment_write_block(	sb, seguse, LFS_SEGUSE_SIZE, 
						offset, 0);
		update_inode(inode, get_iblock_nr(segnum), newblock);
		//dprintk("updating %Lu to %Lu block\n", get_iblock_nr(segp), newblock);
	}
	mark_inode_dirty(inode);
}

/* useful internal function */
static void read_or_init_seguse(struct super_block *sb, int segnum, 
				struct segusage *seguse)
{
	struct buffer_head *bh;
	struct segusage *pseguse = NULL;

	memset(seguse, 0, LFS_SEGUSE_SIZE);
	bh = segusetbl_read_entry(sb, segnum, &pseguse);
	if(bh) {
		seguse->su_nbytes = pseguse->su_nbytes;
		seguse->su_olastmod = pseguse->su_olastmod;
		seguse->su_nsums = pseguse->su_nsums;
		seguse->su_lastmod = pseguse->su_lastmod;
		seguse->su_flags = pseguse->su_flags;
		brelse(bh);
	}
	else { /* new entry for the seg */
		dprintk("Creating the first seguse entry for segment %d\n",
		segnum);
		seguse->su_nbytes = 0;
		seguse->su_olastmod = CURRENT_TIME.tv_sec;
		seguse->su_nsums = 1;
		seguse->su_lastmod = CURRENT_TIME.tv_sec;
		seguse->su_flags = SEGUSE_DIRTY;
	}
}

static void segusetbl_update_livebytes( struct super_block *sb, 
					int segnum, __u32 nbytes)
{
	struct segusage seguse;

	read_or_init_seguse(sb, segnum, &seguse);
	seguse.su_nbytes += nbytes;
	dprintk("%d\n", seguse.su_nbytes);
	segusetbl_update_entry(sb, segnum, &seguse);
}

void segusetbl_dec_livebytes(struct super_block *sb, int segnum, __u32 nbytes)
{
	dprintk("decrementing livebytes for segment %d by %d (total = ", segnum,
	nbytes);
	segusetbl_update_livebytes(sb, segnum, 0 - nbytes);
}

void segusetbl_add_livebytes(struct super_block *sb, int segnum, __u32 nbytes)
{
	dprintk("incrementing livebytes for segment %d by %d (total = ", segnum,
	nbytes);
	segusetbl_update_livebytes(sb, segnum, nbytes);
}

void segusetbl_set_segstate(struct super_block *sb, int segnum, int state)
{
	struct segusage seguse;

	read_or_init_seguse(sb, segnum, &seguse);
	seguse.su_flags = state;
	segusetbl_update_entry(sb, segnum, &seguse);
}
