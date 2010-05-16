/* $Id: dir.c,v 1.23 2005/08/18 08:28:33 ppadala Exp $
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

#include <linux/fs.h>
#include <linux/buffer_head.h>

#include "include/lfs_kernel.h"

extern int lfs_add_entry(struct inode * dir, const char * name, 
			 int namelen, int ino);
extern int lfs_create (struct inode * dir, struct dentry * dentry, 
                       int mode, struct nameidata *nd);
extern struct dentry *lfs_lookup (struct inode * dir, struct dentry *dentry,
			   	  struct nameidata *nd);
extern int lfs_symlink (struct inode * dir, struct dentry * dentry,
			 const char * symname);
extern struct inode *lfs_new_inode(struct inode *dir, int mode);
extern unsigned long dir_blocks(struct inode *inode);

static int lfs_readdir (struct file * f, void * dirent, filldir_t filldir)
{
	lfs_dirent *de;
	int block;
	int reclen = LFS_DIRENT_RECLEN;
	unsigned int offset;
	struct buffer_head * bh;
	struct inode * dir = f->f_dentry->d_inode;

	while (f->f_pos < dir->i_size) {
		offset = f->f_pos & (LFS_BSIZE-1);
		block = f->f_pos >> LFS_BSIZE_BITS;
		bh = lfs_read_block(dir, block);
		if (!bh)
			goto err_eio;
		do {
			//dprintk("block = %d, offset = %d\n", block, offset);
			de = (lfs_dirent *)(bh->b_data + offset);
			if (de->inode) {
				//dprintk("Filling entries for %d %s %d\n", de->inode, de->name, de->name_len);
				if (filldir(dirent, de->name, de->name_len, f->f_pos, de->inode, DT_UNKNOWN) < 0) {
					brelse(bh);
					goto done;
				}
			}
			offset += reclen;
			f->f_pos += reclen;
		} while (offset < LFS_BSIZE && f->f_pos < dir->i_size);
		brelse(bh);
	}

done:
	return 0;
err_eio:
	return -EIO;
}

static int lfs_link (	struct dentry * old, struct inode * dir,
			struct dentry * new)
{
	struct inode * inode = old->d_inode;
	int err;
	
	lfs_lock(inode->i_sb);

	dprintk("adding link %s\n", new->d_name.name);
	err = lfs_add_entry(	dir, new->d_name.name, 
				new->d_name.len, inode->i_ino);
	if (err)
		return err;
	inode->i_nlink++;
	inode->i_ctime = CURRENT_TIME;
	mark_inode_dirty(inode);
	atomic_inc(&inode->i_count);

	lfs_unlock(inode->i_sb);

	d_instantiate(new, inode);
	return 0;
}

static int lfs_unlink(struct inode * dir, struct dentry * dentry)
{
	struct inode * inode = dentry->d_inode;
	struct buffer_head * bh;
	lfs_dirent * de;
	sector_t block;
	int err = -ENOENT; 
	sector_t iblock;

	dprintk("unlinking %s\n", dentry->d_name.name);

	bh = lfs_find_entry(dir, dentry->d_name.name, dentry->d_name.len, &de, &iblock);
	if (!bh || de->inode != inode->i_ino) 
		goto out_brelse;

	lfs_lock(dir->i_sb);
	if (!inode->i_nlink) {
		dprintk("unlinking non-existent file %s:%lu (nlink=%d)\n", 
			 inode->i_sb->s_id, inode->i_ino, inode->i_nlink);
		inode->i_nlink = 1;
	}
	de->inode = 0;

	if(!segment_contains_block(LFS_SBI(dir->i_sb)->s_curr, bh->b_blocknr)) {
		dprintk("Writing to end of segment\n");
		block = __segment_write_block(dir->i_sb, bh->b_data, LFS_BSIZE);
		update_inode(dir, iblock, block);
	}
	else {
		dprintk("Just marking the segment dirty\n");
		mark_segment_dirty(LFS_SBI(dir->i_sb)->s_curr);
	}

	dir->i_ctime = dir->i_mtime = CURRENT_TIME;
	mark_inode_dirty(dir);
	inode->i_nlink--;
	inode->i_ctime = dir->i_ctime;
	mark_inode_dirty(inode);

	lfs_unlock(dir->i_sb);
	err = 0;

out_brelse:
	brelse(bh);
	return err;
}

static inline void lfs_inc_count(struct inode *inode)
{
	inode->i_nlink++;
}

static inline void lfs_dec_count(struct inode *inode)
{
	inode->i_nlink--;
}

static int lfs_mkdir(struct inode * dir, struct dentry * dentry, int mode)
{
	struct inode * inode;
	int err = -EMLINK;

	lfs_inc_count(dir);

	inode = lfs_new_inode (dir, S_IFDIR | mode);
	err = PTR_ERR(inode);
	if (IS_ERR(inode))
		goto out_dir;

	inode->i_op = &lfs_dir_inode_operations;
	inode->i_fop = &lfs_dir_operations;
	inode->i_mapping->a_ops = &lfs_aops;

	lfs_inc_count(inode);

	/* TODO: add .. entry, see ext2_make_empty */
	
	lfs_lock(dir->i_sb);
	err = lfs_add_entry(dir, dentry->d_name.name, dentry->d_name.len,
				inode->i_ino);
	lfs_unlock(dir->i_sb);
	if (err)
		goto out_fail;

	insert_inode_hash(inode);
	mark_inode_dirty(inode);
	mark_inode_dirty(dir);
	
	d_instantiate(dentry, inode);
out:
	return err;

out_fail:
	lfs_dec_count(inode);
	lfs_dec_count(inode);
	iput(inode);
out_dir:
	lfs_dec_count(dir);
	goto out;
}

static int lfs_empty_dir(struct inode *dir)
{	
	int empty = 1, i;
	lfs_dirent *pde;
	struct buffer_head *bh;
	int reclen = LFS_DIRENT_RECLEN;
	unsigned long nblocks = dir_blocks(dir);

	for(i = 0;i < nblocks; ++i) {
		int dirend, offset;

		bh = lfs_read_block(dir, i);
		if (!bh)
			break;

		pde = (lfs_dirent *) bh->b_data;
		if(i != nblocks - 1)
			dirend = LFS_BSIZE;
		else
			dirend = dir->i_size % LFS_BSIZE;
		for(offset = 0;offset < dirend; offset += reclen) {
			if(pde->inode)
				empty = 0;
			pde += reclen;
		}
		brelse(bh);
	}
	return empty;
}

static int lfs_rmdir (struct inode * dir, struct dentry *dentry)
{
	struct inode * inode = dentry->d_inode;
	int err = -ENOTEMPTY;

	if (lfs_empty_dir(inode)) {
		err = lfs_unlink(dir, dentry);
		if (!err) {
			inode->i_size = 0;
			lfs_dec_count(inode);
			lfs_dec_count(dir);
		}
	}
	return err;
}

static int lfs_mknod (struct inode * dir, struct dentry *dentry, int mode, dev_t rdev)
{
	struct inode * inode;
	int err;

	if (!new_valid_dev(rdev))
		return -EINVAL;

	inode = lfs_new_inode (dir, mode);

	err = PTR_ERR(inode);
	if (!IS_ERR(inode)) {
		init_special_inode(inode, inode->i_mode, rdev);
		inode->i_nlink++;
		insert_inode_hash(inode);
		mark_inode_dirty(inode);  
		lfs_lock(dir->i_sb);
		err = lfs_add_entry(dir, dentry->d_name.name, 
			            dentry->d_name.len, inode->i_ino);
			
		lfs_unlock(dir->i_sb);
		if(err)
			goto out_fail;
		d_instantiate(dentry, inode);

	}
	return err;
out_fail:
	--inode->i_nlink;
	mark_inode_dirty(inode);
	iput(inode);
	return err;
}


struct file_operations lfs_dir_operations = {
	.llseek	 = generic_file_llseek,
	.read    = generic_read_dir,
        .readdir = lfs_readdir,
	.fsync   = file_fsync,
};

struct inode_operations lfs_dir_inode_operations = {
	.create	= lfs_create,
	.lookup	= lfs_lookup,
	.link	= lfs_link,
	.unlink = lfs_unlink,
	.symlink = lfs_symlink,
	.mkdir  = lfs_mkdir,
	.rmdir  = lfs_rmdir,
	.mknod	= lfs_mknod,
};
