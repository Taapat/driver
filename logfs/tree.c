/* $Id: tree.c,v 1.21 2005/08/17 05:40:43 ppadala Exp $
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

 /* Contains functions for manipulating the standard UNIX inode trees.
    parts of the code from ext2/ext3 code */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/pagemap.h>
#include <linux/backing-dev.h>
#include <linux/writeback.h>

#include "include/lfs_kernel.h"

static inline void add_chain(Indirect *p, struct buffer_head *bh, __u32 *v)
{
	p->key = *(p->p = v);
	p->bh = bh;
}

static inline int verify_chain(Indirect *from, Indirect *to)
{
	while (from <= to && from->key == *from->p)
		from++;
	return (from > to);
}

/**
 *	lfs_block_to_path - parse the block number into array of offsets
 *	@inode: inode in question (we are only interested in its superblock)
 *	@i_block: block number to be parsed
 *	@offsets: array to store the offsets in
 *      @boundary: set this non-zero if the referred-to block is likely to be
 *             followed (on disk) by an indirect block.
 *	To store the locations of file's data ext2 uses a data structure common
 *	for UNIX filesystems - tree of pointers anchored in the inode, with
 *	data blocks at leaves and indirect blocks in intermediate nodes.
 *	This function translates the block number into path in that tree -
 *	return value is the path length and @offsets[n] is the offset of
 *	pointer to (n+1)th node in the nth one. If @block is out of range
 *	(negative or too large) warning is printed and zero returned.
 *
 *	Note: function doesn't find node addresses, so no IO is needed. All
 *	we need to know is the capacity of indirect blocks (taken from the
 *	inode->i_sb).
 */

/*
 * Portability note: the last comparison (check that we fit into triple
 * indirect block) is spelled differently, because otherwise on an
 * architecture with 32-bit longs and 8Kb pages we might get into trouble
 * if our filesystem had 8Kb blocks. We might use long long, but that would
 * kill us on x86. Oh, well, at least the sign propagation does not matter -
 * i_block would have to be negative in the very beginning, so we would not
 * get there at all.
 */

static int lfs_block_to_path(struct inode *inode,
			long i_block, int offsets[4], int *boundary)
{
	int ptrs = LFS_ADDR_PER_BLOCK;
	int ptrs_bits = 8;
	const long direct_blocks = LFS_NDIR_BLOCKS,
		indirect_blocks = ptrs,
		double_blocks = (1 << (ptrs_bits * 2));
	int n = 0;
	int final = 0;

	if (i_block < 0) {
		lfs_warning (inode->i_sb, "lfs_block_to_path", "block < 0");
	} else if (i_block < direct_blocks) {
		offsets[n++] = i_block;
		final = direct_blocks;
	} else if ( (i_block -= direct_blocks) < indirect_blocks) {
		offsets[n++] = LFS_IND_BLOCK;
		offsets[n++] = i_block;
		final = ptrs;
	} else if ((i_block -= indirect_blocks) < double_blocks) {
		offsets[n++] = LFS_DIND_BLOCK;
		offsets[n++] = i_block >> ptrs_bits;
		offsets[n++] = i_block & (ptrs - 1);
		final = ptrs;
	} else if (((i_block -= double_blocks) >> (ptrs_bits * 2)) < ptrs) {
		offsets[n++] = LFS_TIND_BLOCK;
		offsets[n++] = i_block >> (ptrs_bits * 2);
		offsets[n++] = (i_block >> ptrs_bits) & (ptrs - 1);
		offsets[n++] = i_block & (ptrs - 1);
		final = ptrs;
	} else {
		lfs_warning (inode->i_sb, "lfs_block_to_path", "block > big");
	}
	if (boundary)
		*boundary = (i_block & (ptrs - 1)) == (final - 1);
	/*for(i = 0;i < n; ++i) {
		dprintk("offsets[%d] = %d\n", i, offsets[i]);
	}*/
	return n;
}

/**
 *	lfs_get_branch - read the chain of indirect blocks leading to data
 *	@inode: inode in question
 *	@depth: depth of the chain (1 - direct pointer, etc.)
 *	@offsets: offsets of pointers in inode/indirect blocks
 *	@chain: place to store the result
 *	@err: here we store the error value
 *
 *	Function fills the array of triples <key, p, bh> and returns %NULL
 *	if everything went OK or the pointer to the last filled triple
 *	(incomplete one) otherwise. Upon the return chain[i].key contains
 *	the number of (i+1)-th block in the chain (as it is stored in memory,
 *	i.e. little-endian 32-bit), chain[i].p contains the address of that
 *	number (it points into struct inode for i==0 and into the bh->b_data
 *	for i>0) and chain[i].bh points to the buffer_head of i-th indirect
 *	block for i>0 and NULL for i==0. In other words, it holds the block
 *	numbers of the chain, addresses they were taken from (and where we can
 *	verify that chain did not change) and buffer_heads hosting these
 *	numbers.
 *
 *	Function stops when it stumbles upon zero pointer (absent block)
 *		(pointer to last triple returned, *@err == 0)
 *	or when it gets an IO error reading an indirect block
 *		(ditto, *@err == -EIO)
 *	or when it notices that chain had been changed while it was reading
 *		(ditto, *@err == -EAGAIN)
 *	or when it reads all @depth-1 indirect blocks successfully and finds
 *	the whole chain, all way to the data (returns %NULL, *err == 0).
 */
static Indirect *lfs_get_branch(struct inode *inode,
				 int depth,
				 int *offsets,
				 Indirect chain[4],
				 int *err)
{
	struct super_block *sb = inode->i_sb;
	Indirect *p = chain;
	struct buffer_head *bh;

	*err = 0;
	/* i_data is not going away, no lock needed */
	add_chain (chain, NULL, LFS_I(inode)->i_data + *offsets);
	if (!p->key)
		goto no_block;

	while (--depth) {
		bh = lfs_sb_bread(sb, p->key);
		if (!bh)
			goto failure;
		read_lock(&LFS_I(inode)->i_meta_lock);
		if (!verify_chain(chain, p))
			goto changed;
		add_chain(++p, bh, (__u32*)bh->b_data + *++offsets);
		read_unlock(&LFS_I(inode)->i_meta_lock);
		if (!p->key)
			goto no_block;
	}
	return NULL;

changed:
	read_unlock(&LFS_I(inode)->i_meta_lock);
	brelse(bh);
	*err = -EAGAIN;
	goto no_block;
failure:
	*err = -EIO;
no_block:
	return p;
}

/**
 *	lfs_alloc_branch - allocate and set up a chain of blocks.
 *	@inode: owner
 *	@num: depth of the chain (number of blocks to allocate)
 *	@offsets: offsets (in the blocks) to store the pointers to next.
 *	@branch: place to store the chain in.
 *
 *	This function allocates @num blocks, zeroes out all but the last one,
 *	links them into chain and (if we are synchronous) writes them to disk.
 *	In other words, it prepares a branch that can be spliced onto the
 *	inode. It stores the information about that chain in the branch[], in
 *	the same format as ext2_get_branch() would do. We are calling it after
 *	we had read the existing part of chain and partial points to the last
 *	triple of that (one with zero ->key). Upon the exit we have the same
 *	picture as after the successful ext2_get_block(), excpet that in one
 *	place chain is disconnected - *branch->p is still zero (we did not
 *	set the last link), but branch->key contains the number that should
 *	be placed into *branch->p to fill that gap.
 *
 *	If allocation fails we free all blocks we've allocated (and forget
 *	their buffer_heads) and return the error value the from failed
 *	ext2_alloc_block() (normally -ENOSPC). Otherwise we set the chain
 *	as described above and return 0.
 */

static int lfs_alloc_branch(struct inode *inode,
			     int num,
			     unsigned long goal,
			     int *offsets,
			     Indirect *branch)
{
	struct super_block *sb = inode->i_sb;
	int blocksize = sb->s_blocksize;
	int n;

	assert(num >= 1 && num <= 4);
	for(n = num - 1; n > 0; --n) {
		int nr;
		struct buffer_head *bh;
		struct segment *segp = LFS_SBI(sb)->s_curr;

		branch[n].key = goal;
		nr = segment_get_next_block(sb);
		segusetbl_add_livebytes(sb, segp->segnum, LFS_BSIZE);
	
		bh = lfs_sb_bread(sb, nr);
		BUG_ON(!bh);
		lock_buffer(bh);
		memset(bh->b_data, 0, blocksize);
		branch[n].bh = bh;
		branch[n].p = (__u32 *) bh->b_data + offsets[n];
		*branch[n].p = branch[n].key;
		//set_buffer_uptodate(bh);
		unlock_buffer(bh);

		mark_segment_dirty(segp);
		brelse(bh);
		goal = nr;
	}

	branch[0].key = goal;

	if (n == 0)
		return 0;
	BUG();	/* should never reach here */
	return 0;
}

int lfs_map_block(struct inode * inode, sector_t block, 
			 struct buffer_head * bh, int create)
{
	int offsets[4];
	Indirect chain[4];
	Indirect *partial;
	int boundary = 0;
	int depth = lfs_block_to_path(inode, block, offsets, &boundary);
	int err = -EIO;
	
	partial = lfs_get_branch(inode, depth, offsets, chain, &err);

	if(!create) {
		map_bh(bh, inode->i_sb, chain[depth - 1].key);
		return 0;
	}
	return err;
}

/* returns the corresponding disk block for a particular iblock */
sector_t lfs_disk_block(struct inode *inode, sector_t iblock)
{
	int err = -EIO;
	int offsets[4];
	Indirect chain[4];
	Indirect *partial;
	int boundary = 0;
	int depth = lfs_block_to_path(inode, iblock, offsets, &boundary);
	
	partial = lfs_get_branch(inode, depth, offsets, chain, &err);
	if(!partial) {
		//dprintk("The block %u already exists\n",chain[depth-1].key); 
		partial = chain + depth - 1;
		while(partial > chain) {
			brelse(partial->bh);
			partial--;
		}
		return chain[depth - 1].key;
	}
	return 0;
}

struct buffer_head *lfs_read_block(struct inode *inode, sector_t iblock) 
{	
	sector_t block;
	struct buffer_head *bh = NULL;

	block = lfs_disk_block(inode, iblock);
	if(block > 0) {
		bh = lfs_sb_bread(inode->i_sb, block);
		return bh;
	}
	return bh;
}

/* update the disk block for iblock, expand inode as needed */
int update_inode(struct inode *inode, sector_t iblock, sector_t block)
{
	int err = -EIO;
	int offsets[4];
	Indirect chain[4];
	Indirect *partial;
	int left;
	int boundary = 0;
	int depth = lfs_block_to_path(inode, iblock, offsets, &boundary);

	//dprintk("Inode(%lu): updating %Lu block to %Lu disk block\n", inode->i_ino, iblock, block); 
	if(depth == 0)
		goto out;

	partial = lfs_get_branch(inode, depth, offsets, chain, &err);

	if(offsets[0] < LFS_NDIR_BLOCKS) {
		LFS_I(inode)->i_data[offsets[0]] = block;
		err = 0;
	} else {	
		int i;
		struct super_block *sb = inode->i_sb;

		if(!partial) /* the block exists */
			partial = chain + depth - 1;
		left = (chain + depth) - partial;
		err = lfs_alloc_branch(inode, left, block,
					     offsets+(partial-chain), partial);
		/*for(i = 0;i < depth; ++i)
			dprintk("chain[%d].key=%d\n", i, chain[i].key);*/
		for(i = partial - chain - 1; i >= 0; --i) {
			struct buffer_head *bh;
			__u32 *buf;
			int in_segment;

			/* 
			 * TODO: Convert to a single segment_write_block 
			 * tricky, careful !
			 */
			in_segment = segment_contains_block(LFS_SBI(sb)->s_curr, chain[i].key);

			bh = lfs_sb_bread(sb, chain[i].key);
			buf = (__u32 *)bh->b_data;
			*(buf + offsets[i + 1]) = chain[i + 1].key;
			if(!in_segment) {
				struct segment *segp = LFS_SBI(sb)->s_curr;
				chain[i].key = __segment_write_block(sb, buf, LFS_BSIZE);
				segusetbl_add_livebytes(sb, segp->segnum, LFS_BSIZE);
			}
		}
		LFS_I(inode)->i_data[offsets[0]] = chain[0].key;
	}	
out:
	return err;
}

/* we do not have a bitmap, just have to update segusetbl entries */
void free_inode_blocks(struct inode *inode)
{
	int err = -EIO;
	long iblock;
	int offsets[4];
	Indirect chain[4];
	Indirect *partial;
	int depth, i;
	//int left, boundary = 0, depth, i;
	__le32 *data = LFS_I(inode)->i_data; 
	struct super_block *sb = inode->i_sb;

	/* last block */
	iblock = (inode->i_size + LFS_BSIZE - 1) >> LFS_BSIZE_BITS;
	depth = lfs_block_to_path(inode, iblock, offsets, NULL);

	/* nothing to free */
	if(depth == 0)
		return;

	partial = lfs_get_branch(inode, depth, offsets, chain, &err);
	if(offsets[0] < LFS_NDIR_BLOCKS) {
		size_t size = inode->i_size;
		for(i = 0;i < offsets[0] - 1; ++i) {
			int segnum = dtosn(data[i]);

			//dprintk("i_data[%d]=%d\n",i, data[i]);
			segusetbl_dec_livebytes(sb, segnum, LFS_BSIZE);
			size -= LFS_BSIZE;
		}
		//dprintk("i_data[%d]=%d\n",i, data[i]);
		segusetbl_dec_livebytes(sb, dtosn(data[i]), size);
	}

	/*for(i = 0;i < depth; ++i)
		dprintk("chain[%d].key=%d\n", i, chain[i].key);*/
}
