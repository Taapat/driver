/* $Id: segment.h,v 1.14 2005/08/18 08:32:46 ppadala Exp $
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

#ifndef __SEGMENT_H
#define __SEGMENT_H

#include "lfs_kernel.h"
#include <linux/types.h>
#include <linux/pagemap.h>

enum status_t {
	SEG_CREATED,
	SEG_ALLOCATED,
	SEG_DIRTY,
	SEG_WRITTEN,
};

struct segment {
	int segnum;
	sector_t start, end;
	size_t offset, size;
	enum status_t status;
	struct inode *inode;
	struct list_head segs;
	struct super_block *sb;
	struct page **pages;

	/* seg summary */
	struct segsum segsum;
	struct finfo *fis;
	daddr_t *inos;
};
#define MAPPING(segp) (segp->inode->i_mapping)
#define CURSEG(sb) (LFS_SBI(sb)->s_curr)
#define SEGS_IN_MEMORY 16

int init_segments(struct super_block *sb);
void destroy_segments(struct super_block *sb);
void write_segments(struct super_block *sb);

void write_segment(struct segment *segp);

int segment_allocate_first(struct super_block *sb);
int segment_allocate(struct segment *segp, sector_t block, int offset);
void segment_allocate_new(struct super_block *sb, struct segment *segp, 	
			  sector_t block);
sector_t segment_write_block(	struct super_block *sb, void *block, 
				__u32 size, int offset, sector_t blocknr);
sector_t __segment_write_block(struct super_block *sb, void *block, __u32 size);
sector_t segment_write_block_from_bh(struct super_block *sb, void *block, 
		     	    __u32 size, int offset, struct buffer_head *bh);

sector_t segment_get_next_block(struct super_block *sb);
struct buffer_head * lfs_sb_bread(struct super_block *sb, sector_t block);
struct buffer_head *segment_get_bh(struct segment *segp, sector_t blk);

static inline pgoff_t segment_page_index(struct segment *segp)
{
	return (segp->start >> (PAGE_CACHE_SHIFT - segp->inode->i_blkbits));
}

static inline int segment_contains_block(struct segment *segp, sector_t block)
{
	return (block >= segp->start && block < segp->end) ? 1 : 0;
}

static inline void mark_segment_dirty(struct segment *segp)
{
	segp->status = SEG_DIRTY;
}

/* seguse.c */
void segusetbl_add_livebytes(struct super_block *sb, int segnum, __u32 nbytes);
void segusetbl_dec_livebytes(struct super_block *sb, int segnum, __u32 nbytes);
void segusetbl_set_segstate(struct super_block *sb, int segnum, int state);

/* segsum.c */
void alloc_segsum(struct segment *segp);
void read_segsum(struct segment *segp);
void write_segsum(struct segment *segp);
void free_segsum(struct segment *segp);
void segsum_update_finfo(struct segment *segp, ino_t ino, int lbn, sector_t block);
void segsum_add_inob(struct segment *segp, sector_t block);

#endif
