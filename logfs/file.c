/* $Id: file.c,v 1.28 2005/08/18 08:29:04 ppadala Exp $
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
#include <linux/buffer_head.h>
#include <linux/pagemap.h>
#include <linux/backing-dev.h>
#include <linux/writeback.h>
#include <linux/fs.h>
#include <linux/swap.h>
#include <linux/mpage.h>

#include "include/lfs_kernel.h"

#if 0
static int lfs_copy_from_user( 	loff_t pos, int npages, int nbytes, 
				struct page **segpages, const char __user *buf)
{
	long page_fault=0;
	int i, offset; 

    	for ( i = 0, offset = (pos & (PAGE_CACHE_SIZE-1)); 
	      i < npages; i++,offset=0) {
		size_t count = min_t(size_t,PAGE_CACHE_SIZE-offset, nbytes);
		struct page *page = segpages[i];

		fault_in_pages_readable( buf, count);

		kmap(page);
		page_fault = __copy_from_user(page_address(page)+offset, 
					      buf, count);
		flush_dcache_page(page);
		kunmap(page);
		buf += count;
		nbytes -=count;

		if (page_fault)
	    		break;
    	}

	return page_fault?-EFAULT:0;
}
#endif

static inline void print_buffers(struct page *page, sector_t block)
{
	struct buffer_head *bh, *head;

	if(!page_has_buffers(page)) {
		printk("Warning: page doesn't have buffers, not sure how that happened, allocating buffer !!!\n");
		create_empty_buffers(page, LFS_BSIZE, 0);
		bh = head = page_buffers(page);
		do {
			map_bh(bh, page->mapping->host->i_sb, block++); 
			bh = bh->b_this_page;
		} while(bh != head);
	}

	bh = head = page_buffers(page);
	do {
		if(!buffer_mapped(bh))
			dprintk("The buffer seems to be not mapped ??");
		//dprintk("mapped to blocknr = %Lu\n", bh->b_blocknr);
		bh = bh->b_this_page;
	} while(bh != head);
}

struct page * get_seg_page(struct segment *segp)
{
	int index = segp->offset / BUF_IN_PAGE;
	struct page *page;

	//dprintk("get_seg_page:segnum=%d,segp->start=%Lu,segp->offset=%d,index=%d\n",
	//segp->segnum,segp->start, segp->offset, index);
	assert(index < LFS_SEGSIZE);
	page = segp->pages[index];
	print_buffers(page, segp->start + index * BUF_IN_PAGE);
//	dprintk("returning page with index %d that is mapped to %Lu\n", index, page_buffers(page)->b_blocknr);
	return page;
}

/* --- copied from mm/, the right place for the following functions --- */
/* 
 * An ugly hack to avoid kernel patch for now. All we need is an
 * EXPORT_SYMBOL(invalidate_complete_page)
 */
void __remove_from_page_cache(struct page *page)
{
	struct address_space *mapping = page->mapping;

	radix_tree_delete(&mapping->page_tree, page->index);
	page->mapping = NULL;
	mapping->nrpages--;
	pagecache_acct(-1);
}
int invalidate_complete_page(struct address_space *mapping, struct page *page)
{
	if (page->mapping != mapping)
		return 0;

	if (PagePrivate(page) && !try_to_release_page(page, 0))
		return 0;

	spin_lock_irq(&mapping->tree_lock);
	if (PageDirty(page)) {
		spin_unlock_irq(&mapping->tree_lock);
		return 0;
	}
	__remove_from_page_cache(page);
	spin_unlock_irq(&mapping->tree_lock);
	ClearPageUptodate(page);
	page_cache_release(page);	/* pagecache ref */
	return 1;
}
/* --- copy from mm/ ends --- */

void invalidate_old_page(struct inode *inode, loff_t pos)
{
	struct address_space *mapping = inode->i_mapping;
	int index = pos >> PAGE_CACHE_SHIFT;
	struct page *page;

	page = grab_cache_page(mapping, index);
	if(!page) {
		dprintk("page not in the cache\n");
		return;
	}
	invalidate_complete_page(mapping, page);
	unlock_page(page);
}

/* FIXME: Ugliest function of all in LFS, need I say more? */
static ssize_t lfs_file_write( 	struct file *file, const char __user *buf, 	
				size_t count, loff_t *ppos)
{	
	loff_t pos;
	struct page *page;
	ssize_t res, written, bytes;
	struct inode *inode = file->f_dentry->d_inode;
	struct super_block *sb = inode->i_sb;
	struct segment *segp = LFS_SBI(sb)->s_curr;

	//dprintk("lfs_file_write called for %lu at pos %Lu\n", inode->i_ino, *ppos);
	if(file->f_flags & O_DIRECT) {
		dprintk("The file is requesting direct IO\n");
		return -EINVAL;
	}

	if (unlikely(count < 0 ))
		return -EINVAL;
	if (unlikely(!access_ok(VERIFY_READ, buf, count)))
		return -EFAULT;

	//down(&inode->i_sem);	/* lock the file */
	mutex_lock(&inode->i_mutex); //BrechREiZ: We need this for Kernel 2.6.17
	lfs_lock(sb);
		
	pos = *ppos;
	res = generic_write_checks(file, &pos, &count, 0);
	if (res)
		goto out;
	if(count == 0)
		goto out;
	
	res = remove_suid(file->f_dentry);
	if(res)
		goto out;
	//inode_update_time(inode, 1);	/* update mtime and ctime */
	file_update_time(inode); //BrechREiZ: We need this for Kernel 2.6.17

	written = 0;
	do {
		long offset;
		size_t copied;
		int i, siblock, eiblock, boffset;
		sector_t block;
				
		offset = (segp->offset % BUF_IN_PAGE) * LFS_BSIZE; 
		offset += pos & (LFS_BSIZE - 1); /* within block */
		bytes = PAGE_CACHE_SIZE - offset; /* number of bytes written
						     in this iteration */
		invalidate_old_page(inode, pos);

		if (bytes > count) 
			bytes = count;
		
		//dprintk("1:segp->start=%Lu,segp->offset=%d,segp->end=%Lu,offset=%lu,bytes=%d\n", segp->start, segp->offset, segp->end,offset,bytes);
		
		siblock = pos >> LFS_BSIZE_BITS;
		eiblock = (pos + bytes - 1) >> LFS_BSIZE_BITS;

		//dprintk("writing %d bytes at offset %ld (pos = %Lu)\n", bytes, offset, pos);
		//dprintk("siblock = %d, eiblock = %d\n", siblock, eiblock);
		

		/*
		 * Bring in the user page that we will copy from _first_.
		 * Otherwise there's a nasty deadlock on copying from the
		 * same page as we're writing to, without it being marked
		 * up-to-date.
		 */
		fault_in_pages_readable(buf, bytes);
		page = get_seg_page(segp);
		if (!page) {
			res = -ENOMEM;
			break;
		}

		/* fill the page with current inode blocks if any */
		boffset = offset / LFS_BSIZE;;
		for(i = siblock; i <= eiblock; ++i, ++boffset) {
			struct buffer_head *bh;
			//dprintk("Asking for block %d\n", i);
			bh = lfs_read_block(inode, i);
			if(!bh) /* new block */
				break;
			//dprintk("boffset = %d\n", boffset);
			memcpy(page_address(page) + LFS_BSIZE * boffset, bh->b_data, LFS_BSIZE);
			brelse(bh);
		}

		copied = __copy_from_user(page_address(page) + offset, buf, bytes);
		flush_dcache_page(page);

		block = segp->start + segp->offset;
		for(i = siblock;i <= eiblock; ++i, ++block)
			segsum_update_finfo(segp, inode->i_ino, i, block);

		block = segp->start + segp->offset;
		segp->offset += (bytes  - 1)/LFS_BSIZE + 1;
		//dprintk("2:segp->start=%Lu,segp->offset=%d,segp->end=%Lu,offset=%lu,bytes=%d\n",
		//segp->start, segp->offset, segp->end,offset,bytes);
		BUG_ON(segp->start + segp->offset > segp->end);
		if(segp->start + segp->offset == segp->end) {
			dprintk("allocating new segment\n");
			/* This also is going to write the previous segment */
			segment_allocate_new(inode->i_sb, segp, segp->start + segp->offset);
			segp = LFS_SBI(sb)->s_curr;
		}

		/* update the inode */
		for(i = siblock;i <= eiblock; ++i, ++block)
			update_inode(inode, i, block);
		//dprintk("start=%Lu,offset=%d,end=%Lu\n", segp->start, segp->offset, segp->end);
		segusetbl_add_livebytes(sb, segp->segnum, bytes);
		
		written += bytes;
		buf += bytes;
		pos += bytes;
		count -= bytes;
	} while(count);

	*ppos = pos;
	if(pos > inode->i_size)
		i_size_write(inode, pos);
	if(written)
		mark_inode_dirty(inode);
	
	lfs_unlock(sb);
	//up(&inode->i_sem);
	mutex_unlock(&inode->i_mutex); //BrechREiZ: and unlocking...
	return written ? written : res;
out:
	lfs_unlock(sb);
	//up(&inode->i_sem);
	mutex_unlock(&inode->i_mutex); //BrechREiZ: and unlocking...
	return res; 
}

static int lfs_readpage(struct file *file, struct page *page)
{	
	struct inode *inode = page->mapping->host;
	sector_t iblock, block;
	unsigned int blocksize;
	struct buffer_head *bh, *head;

	blocksize = 1 << inode->i_blkbits;
	if (!page_has_buffers(page))
		create_empty_buffers(page, blocksize, 0);
	head = page_buffers(page);
	bh = head;

	iblock = (sector_t)page->index << (PAGE_CACHE_SHIFT - inode->i_blkbits);
	do {
		struct buffer_head *bh_temp;
		block = lfs_disk_block(inode, iblock);

		dprintk("searching for block %Lu in segments: ", (long long unsigned int)block);
		bh_temp = lfs_read_block(inode, iblock);
		if(bh_temp) {
			dprintk("FOUND\n");

			memcpy(bh->b_data, bh_temp->b_data, LFS_BSIZE);
			set_buffer_uptodate(bh);
			brelse(bh_temp);
		}
		else
			dprintk("NOT FOUND\n");
	} while (iblock++, (bh = bh->b_this_page) != head);
	return block_read_full_page(page, lfs_map_block);
}

static int lfs_writepage(struct page *page, struct writeback_control *wbc)
{
	return block_write_full_page(page, lfs_map_block, wbc);
}

static int lfs_writepages(struct address_space *mapping, struct writeback_control *wbc)
{
	return mpage_writepages(mapping, wbc, lfs_map_block);
}

#if 0
ssize_t lfs_file_read(	struct file *filp, char __user *buf, 
			size_t count, loff_t *ppos)
{
	struct inode *inode = filep->f_mapping->host;
	sector_t iblock;

	iblock = *ppos / (LFS_BSIZE - 1);
	dprintk("read for %d iblock\n", iblock);
	block = lfs_disk_block(inode, iblock);
	if(segment_contains_block(LFS_SBI(inode->i_sb)->s_curr, block)) {
		bh = 
	}
}
#endif

int lfs_sync_file(struct file *file, struct dentry *dentry, int datasync)
{
	dprintk("Ha, joys of log structured file system\n");
	return 0;
}

struct address_space_operations lfs_aops = {
	.readpage = lfs_readpage,
	.writepage = lfs_writepage,
	.writepages = lfs_writepages,
	.sync_page = block_sync_page, /* probably this is not required */
};

struct file_operations lfs_file_operations = {
	.read  =  generic_file_read,
	.write =  lfs_file_write,
	.open  =  generic_file_open,
	.fsync = lfs_sync_file,
	.ioctl = lfs_ioctl,
};
