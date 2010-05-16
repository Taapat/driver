/* $Id: segment.c,v 1.30 2005/08/18 08:30:21 ppadala Exp $
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

/* currently, all functions assume that a read/write lock will be held
   by the caller, need to cleanup the interface soon */
#include <linux/pagemap.h>
#include <linux/buffer_head.h>
#include <linux/writeback.h>
#include <linux/swap.h>

#include "include/lfs_kernel.h"

sector_t segment_get_next_block(struct super_block *sb)
{
	struct segment *segp = LFS_SBI(sb)->s_curr;
	sector_t blk;

	blk = segp->start + segp->offset;
	//dprintk("Allocating block %Lu\n", blk);
	if(segp->start + segp->offset == segp->end - 1) {
		dprintk("segment_get_next_block:Allocating new segment");
		segment_allocate_new(sb, segp, blk + 1);
	}
	else
		++segp->offset;
	BUG_ON(segp->start + segp->offset >= segp->end); /* is this possible? */
		
	return blk;
}

/* Initializes the segment list and sets the current segment
   to the first in the list. 
   This does not allocate memory pages to the segments !  */
int init_segments(struct super_block *sb)
{
	struct segment *segp = NULL;
	struct list_head *prev = NULL;
	int i, err = 0;
	struct list_head *head ;

	head = NULL;
	for(i = 0;i < SEGS_IN_MEMORY; ++i) {
		segp = kcalloc(1, sizeof(*segp), GFP_KERNEL);
		if(!segp)
			goto fail_grab_memory;

		segp->size = LFS_SEGSIZE;
		segp->start = 0; 
		segp->end = 0;
		segp->offset = 0;
		segp->inode = sb->s_bdev->bd_inode;
		segp->status = SEG_CREATED;
		segp->sb = sb;
		segp->pages = NULL;
		if(i == 0) {
			INIT_LIST_HEAD(&segp->segs);
			head = &segp->segs;
		}
		else
			list_add(&segp->segs, prev);
		prev = &segp->segs;
	}
	LFS_SBI(sb)->s_curr = list_entry(prev, struct segment, segs);
	LFS_SBI(sb)->s_seglist = head;
	return err;

fail_grab_memory:
	destroy_segments(sb);
	err = 1;
	return err;
}

static inline void free_segpages(struct segment *segp)
{
	int i;
	if(segp->pages) {
		for(i = 0;i < LFS_SEGSIZE; ++i) if(segp->pages[i]) {
//dprintk("Unlocking page at index %lu\n", segment_page_index(segp, i));
			unlock_page(segp->pages[i]);
			page_cache_release(segp->pages[i]);
		}
		kfree(segp->pages);
	}
}

void destroy_segments(struct super_block *sb)
{
	struct segment *segp;
	struct list_head *p = LFS_SBI(sb)->s_seglist;

	do {
		segp = list_entry(p, struct segment, segs);
		//free_segsum(segp);
		free_segpages(segp);
		kfree(segp);
		p = p->next;
	} while(p != LFS_SBI(sb)->s_seglist);

	LFS_SBI(sb)->s_curr = NULL;
}

static void segment_allocate_pages(struct segment *segp)
{
	int i;
	pgoff_t index;
	sector_t block;

	block = segp->start;
	index = segment_page_index(segp);
	segp->pages = (struct page **)kcalloc(sizeof(struct page *), 	
				      LFS_SEGSIZE, GFP_KERNEL);

	for(i = 0; i < LFS_SEGSIZE; ++i, ++index) {
		struct page *page;
		struct buffer_head *bh, *head;

		/* The lock on the page will not be released
		   until it is written in write_segment !!! */
		//dprintk("Locking page at index %lu\n", index);
		page = segp->pages[i] = grab_cache_page(MAPPING(segp), index);
		BUG_ON(!page);
		if (!page_has_buffers(page))
			create_empty_buffers(page, LFS_BSIZE, 0);
		else
			dprintk("How did that happen??\n");
		
		head = page_buffers(page);
		bh = head;
		do {
			map_bh(bh, segp->sb, block++); 
			bh = bh->b_this_page;
		} while(bh != head);
	}
}

static inline void segment_read_blocks(struct segment *segp)
{	sector_t block;

	if(segp->offset > 1) /* the segment has summary on the disk */
		read_segsum(segp);
	for(block = segp->start + 1;block < segp->start + segp->offset; ++block)
		sb_bread(segp->sb, block);
}

static int __segment_allocate_nostate(struct segment *segp, 
					sector_t block, int offset)
{
	int err = 0;

	if(segp->pages)
		dprintk("The pages not written yet ??\n");
	
	if(segp->start) {
		dprintk("segment starting from %Lu is re-assigned to %Lu\n", (long long unsigned int)segp->start, (long long unsigned int)block);
		memset(&segp->segsum, 0, SEGSUM_SIZE);
	}
	else
		alloc_segsum(segp);
	segp->start = block;
	segp->segnum = dtosn(segp->start);
	segp->end = segp->start + LFS_SEGSIZE * 4;
	segp->offset = offset;
	/*dprintk("seg start=%u, end=%u, offset=%u\n", 
			segp->start, segp->end, segp->offset);*/
	segment_allocate_pages(segp);
	segment_read_blocks(segp);
	segp->status = SEG_ALLOCATED;

	return err;
}

/* allocates memory for the segment and maps the blocks starting
   from @block. */
int segment_allocate(struct segment *segp, sector_t block, int offset)
{	int err = 0;

	dprintk("Allocating segment starting from block %Lu\n", (long long unsigned int)block);
	err = __segment_allocate_nostate(segp, block, offset);
	segusetbl_set_segstate(segp->sb, segp->segnum, SEGUSE_ACTIVE | SEGUSE_DIRTY);
	return err;
}

/* writes the segp and allocates a new segment starting from @block */
void segment_allocate_new(struct super_block *sb, struct segment *segp, 	
			  sector_t block)
{
	dprintk("Allocating new segment starting from block %Lu\n", (long long unsigned int)block);
	write_segment(segp);
	++LFS_SBI(sb)->s_lfssb->s_nseg;
	CURSEG(sb) = list_entry(segp->segs.next, struct segment, segs);
	segment_allocate(CURSEG(sb), block, 1);
	/* now that we have a new segment, let's write the old
	   segment's dirty flag. this is essential as segusetbl might
	   be expand */
	segusetbl_set_segstate(segp->sb, segp->segnum, SEGUSE_DIRTY);
}

int segment_allocate_first(struct super_block *sb)
{
	struct lfs_sb_info *sbi;
	struct lfs_super_block *lfssb;
	sector_t block;

	sbi = sb->s_fs_info;
	lfssb = sbi->s_lfssb;
	block = lfssb->s_next_seg;

	return __segment_allocate_nostate(LFS_SBI(sb)->s_curr, 
				lfssb->s_next_seg, lfssb->s_seg_offset);
}

static void segment_mark_pages_uptodate(struct segment *segp)
{	int i;

	if(!segp->pages)
		return;
	for(i = 0;i < LFS_SEGSIZE; ++i) {
		struct buffer_head *bh, *head;

		bh = head = page_buffers(segp->pages[i]);
		do {
			set_buffer_uptodate(bh);
			bh = bh->b_this_page;
		} while(bh != head);
		SetPageUptodate(segp->pages[i]);
	}
}

void write_segment(struct segment *segp)
{
extern int set_page_dirty(struct page *page);
	int i;
	pgoff_t index;

	//dprintk("In WRITE segment\n");
	write_segsum(segp);
	segment_mark_pages_uptodate(segp);
	index = segment_page_index(segp);
	for(i = 0;i < LFS_SEGSIZE; ++i) {
		//dprintk("Unlocking page at index %lu and setting it dirty\n", segment_page_index(segp,i));
		assert(segp->pages[i] != NULL);
		set_page_dirty(segp->pages[i]);
		unlock_page(segp->pages[i]);
		page_cache_release(segp->pages[i]);
	}
	segp->status = SEG_WRITTEN;
	//free_segsum(segp);
	kfree(segp->pages);
	segp->pages = NULL;
}

/* writes the partial segments that are not yet written to the disk */
void write_segments(struct super_block *sb)
{
	struct segment *segp;
	struct list_head *p = LFS_SBI(sb)->s_seglist;

	do {
		segp = list_entry(p, struct segment, segs);
		if(segp->status == SEG_DIRTY)
			write_segment(segp);
		p = p->next;
	} while(p != LFS_SBI(sb)->s_seglist);
}


struct buffer_head *segment_get_bh(struct segment *segp, sector_t blk) 
{
	int index;
	struct page *page;
	struct buffer_head *bh, *head;

	if(!segment_contains_block(segp, blk))
                return NULL;
	
	index = (blk - segp->start)/BUF_IN_PAGE;
	page = segp->pages[index];
	bh = head = page_buffers(page);
	do {
		if(!buffer_mapped(bh))
			dprintk("The buffer seems to be not mapped ??");
		//dprintk("mapped to blocknr = %Lu\n", bh->b_blocknr);
		bh = bh->b_this_page;
		if(bh->b_blocknr == blk)
			break;
	} while(bh != head);

	if(bh->b_blocknr == blk) {
                get_bh(bh);
                return bh;
        }
        return NULL;
}

struct buffer_head * lfs_sb_bread(struct super_block *sb, sector_t block)
{
        struct segment *segp = CURSEG(sb);
        struct buffer_head *bh;

        bh = segment_get_bh(segp, block);
        if(!bh) 
                bh = sb_bread(sb, block);
        return bh;
}

/* always writes a block of data at the end of the segment 
   and returns the block number it just wrote */
sector_t __segment_write_block(struct super_block *sb, void *block, __u32 size)
{
	struct segment *segp = LFS_SBI(sb)->s_curr;
	struct buffer_head *bh;
	sector_t blk;

	blk = segp->start + segp->offset;
	assert(blk < segp->end);
	//dprintk("__segment_write_block:Writing to block %Lu\n", blk);
	//bh = segment_get_bh(segp, blk);
	bh = segment_get_bh(segp, blk);
	if(!bh)
		return -1;
	memcpy(bh->b_data, block, size);
	brelse(bh);
	mark_segment_dirty(segp);
	if(blk == segp->end - 1) {/* current segment is completely filled */
		//dprintk("Allocating new segment\n");
		segment_allocate_new(sb, segp, blk + 1);
	}
	else
		++segp->offset;
	return blk;
}

/* My brain is not working, can't think of a better name */
sector_t segment_write_block_from_bh(struct super_block *sb, void *block, 
		     	    __u32 size, int offset, struct buffer_head *bh)
{
	sector_t newblock;
	sector_t blocknr = bh->b_blocknr;
	struct segment *segp = LFS_SBI(sb)->s_curr;

	assert(bh != NULL && offset >= 0 && offset + size <= LFS_BSIZE);
	dprintk("Writing to bh(blocknr=%Lu) at offset = %d\n", (long long unsigned int)bh->b_blocknr,
	offset);
	memcpy((char *)bh->b_data + offset, block, size);
	if(segment_contains_block(segp, blocknr)) {
		mark_segment_dirty(segp);
		newblock = blocknr;
	}
	else 
		newblock = __segment_write_block(sb, bh->b_data, LFS_BSIZE);
	return newblock;
}

sector_t segment_write_block(	struct super_block *sb, void *block, 
				__u32 size, int offset, sector_t blocknr)
{
	sector_t newblock;
	struct segment *segp = LFS_SBI(sb)->s_curr;

	assert(size <= LFS_BSIZE);

	dprintk("Writing to segment starting from %Lu on blocknr=%Lu\n", (long long unsigned int)segp->start, (long long unsigned int)blocknr);
	if(blocknr && segment_contains_block(segp, blocknr)) {
		char *buf = NULL;
		struct buffer_head *bh;

		assert(offset >= 0);
		//bh = sb_bread(sb, blocknr);
		bh = segment_get_bh(segp, blocknr);
		buf = bh->b_data + offset;
		memcpy(buf, block, size);
		mark_segment_dirty(segp);
		brelse(bh);
		newblock = blocknr;
	}
	else {
		/* blocknr might or might not be zero, it doesn't matter
		 * though */
		assert(offset == 0);
		newblock = __segment_write_block(sb, block, size);
	}
	return newblock;
}
