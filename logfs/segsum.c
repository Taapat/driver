/* $Id: segsum.c,v 1.3 2005/08/18 08:30:55 ppadala Exp $
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
#include <linux/buffer_head.h>

#ifndef LINUX_2_6_11
extern void *kcalloc(size_t n, size_t size, int flags);
#endif

/* Assumption: segsum fits in one block */
void write_segsum(struct segment *segp)
{
	int i;
	__u8 *data;
	struct buffer_head *bh;
	struct finfo *fip;

	bh = segment_get_bh(segp, segp->start);
	BUG_ON(!bh);
	data = bh->b_data;

	memcpy(data, &segp->segsum, SEGSUM_SIZE);
	data += SEGSUM_SIZE;
	memcpy(data, segp->inos, segp->segsum.ss_nino * DADDRT_SIZE);
	data += segp->segsum.ss_nino * DADDRT_SIZE;

	for(i = 0;i < segp->segsum.ss_nfinfo; ++i) {
		int j;
		fip = segp->fis + i;
		memcpy(data, fip, FINFOSIZE);
		data += FINFOSIZE;
		dprintk("Writing fi_blocks[ino=%d]: ", fip->fi_ino);
		for(j = 0;j < fip->fi_nblocks; ++j)
			dprintk("%d-%d ", fip->fi_lbn[j], fip->fi_blocks[j]);
		dprintk("\n");
		memcpy(data, fip->fi_lbn, fip->fi_nblocks * DADDRT_SIZE);
		data += fip->fi_nblocks * DADDRT_SIZE;
		memcpy(data, fip->fi_blocks, fip->fi_nblocks * DADDRT_SIZE);
		data += fip->fi_nblocks * DADDRT_SIZE;
	}
	brelse(bh);
}

static void read_finfos(struct finfo *fis, int nfi, __u8 *p)
{
	int i;
	
	for(i = 0;i < nfi; ++i) {
		int j;
		memcpy(&fis[i], p, FINFOSIZE);
		p += FINFOSIZE;
		fis[i].fi_lbn = (daddr_t *)kcalloc(MAX_FI_BLOCKS, 	
					   	DADDRT_SIZE,GFP_KERNEL);
		fis[i].fi_blocks = (daddr_t *)kcalloc(MAX_FI_BLOCKS, 
						DADDRT_SIZE, GFP_KERNEL);
		memcpy(fis[i].fi_lbn, p, fis[i].fi_nblocks * DADDRT_SIZE);
		p += DADDRT_SIZE * fis[i].fi_nblocks;
		memcpy(fis[i].fi_blocks, p, fis[i].fi_nblocks * DADDRT_SIZE);
		p += DADDRT_SIZE * fis[i].fi_nblocks;
		dprintk("fi_blocks(ino=%d): ", fis[i].fi_ino);
		for(j = 0;j < fis[i].fi_nblocks; ++j)
			dprintk("%d-%d ", fis[i].fi_lbn[j],fis[i].fi_blocks[j]);
		dprintk("\n");
	}
}

static inline void read_inos(daddr_t *inos, int nino, __u8 *p)
{	int i;
	memcpy(inos, p, DADDRT_SIZE * nino);
	dprintk("Inode blocks: ");
	for(i = 0;i < nino; ++i)
		dprintk("%d ", inos[i]);
	dprintk("\n");
}

void alloc_segsum(struct segment *segp)
{
	segp->segsum.ss_magic = SS_MAGIC;
	segp->segsum.ss_create = CURRENT_TIME.tv_sec;
	segp->segsum.ss_nfinfo = 0;
	segp->segsum.ss_nino = 0;
	segp->fis = kcalloc(MAX_FIS, sizeof(struct finfo), GFP_KERNEL);
	segp->inos = kcalloc(MAX_INO_BLOCKS, DADDRT_SIZE, GFP_KERNEL);
}

void read_segsum(struct segment *segp)
{
	struct buffer_head *bh;
	int nfi, nino;
	__u8 *p;

	bh = sb_bread(segp->sb, segp->start);
	p = bh->b_data;
	memcpy(&segp->segsum, p, sizeof(struct segsum));
	p += sizeof(struct segsum);

	nfi = segp->segsum.ss_nfinfo;
	nino = segp->segsum.ss_nino;
	dprintk("segnum = %d, nfi = %d, nino = %d\n", segp->segnum, nfi, nino);
	read_inos(segp->inos, nino, p);

	p += DADDRT_SIZE * nino;
	read_finfos(segp->fis, nfi, p);
	brelse(bh);
}

/* Adds a block to the finfo for a particular inode */
void segsum_update_finfo(struct segment *segp, ino_t ino, int lbn, sector_t block)
{
	int i;
	struct finfo *fip;
	/* small optimization to avoid search, may be we need a real hashmap */
	static int snum_store = -1;
	static struct finfo *store = NULL; 

	if(snum_store == segp->segnum && store && store->fi_ino == ino) {
		dprintk("The ino %lu found in store\n", ino);
		fip = store;
		goto found;
	}

	/* search */
	for(i = 0;i < segp->segsum.ss_nfinfo; ++i) {
		fip = segp->fis + i;
		if(fip->fi_ino == ino)
			goto found;
	}

	/* new entry */
	dprintk("Adding the block %Lu to ino %lu (new entry)\n", (long long unsigned int)block, ino);
	fip = segp->fis + i;
	fip->fi_nblocks = 1;
	fip->fi_version = 1;
	fip->fi_ino = ino;
	fip->fi_lbn = (daddr_t *)kcalloc(MAX_FI_BLOCKS, DADDRT_SIZE,GFP_KERNEL);
	BUG_ON(!fip->fi_lbn);
	fip->fi_blocks = (daddr_t *)kcalloc(MAX_FI_BLOCKS, DADDRT_SIZE,GFP_KERNEL);
	BUG_ON(!fip->fi_blocks);
	fip->fi_lbn[0] = lbn;
	fip->fi_blocks[0] = block;
	++segp->segsum.ss_nfinfo;
	goto update_store;

found:
	dprintk("Adding block %d-%Lu to ino %lu\n", lbn, (long long unsigned int)block, ino);
	fip->fi_lbn[fip->fi_nblocks] = lbn;
	fip->fi_blocks[fip->fi_nblocks] = block;
	++fip->fi_nblocks;

update_store:
	store = fip;
	snum_store = segp->segnum;
	return;
}

void segsum_add_inob(struct segment *segp, sector_t block)
{	int nino = segp->segsum.ss_nino;
	int i;

	for(i = 0;i < nino; ++i)
		if(segp->inos[i] == block) /* already recorded */
			return;

	segp->inos[nino] = block;
	++segp->segsum.ss_nino;
}

void free_segsum(struct segment *segp)
{	int i;

	if(segp->inos) kfree(segp->inos);
	if(segp->fis) {
		for(i = 0;i < segp->segsum.ss_nfinfo; ++i) {
			struct finfo *fip = segp->fis + i;
			if(fip->fi_lbn) kfree(fip->fi_lbn);
			if(fip->fi_blocks) kfree(fip->fi_blocks);
		}
		kfree(segp->fis);
	}
}
