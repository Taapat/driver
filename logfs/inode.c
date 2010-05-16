/* $Id: inode.c,v 1.35 2005/08/23 20:29:08 ppadala Exp $
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

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/pagemap.h>
#include <linux/backing-dev.h>
#include <linux/writeback.h>

#include "include/lfs_kernel.h"

static void fill_raw_inode(struct inode *inode, struct lfs_inode *raw_inode)
{	int i;

	raw_inode->i_ino = inode->i_ino;
	raw_inode->i_mode = inode->i_mode;
	raw_inode->i_uid = inode->i_uid;
	raw_inode->i_gid = inode->i_gid;
	raw_inode->i_size = inode->i_size;

	raw_inode->i_atime = inode->i_atime.tv_sec;
	raw_inode->i_ctime = inode->i_ctime.tv_sec;
	raw_inode->i_mtime = inode->i_mtime.tv_sec;

	raw_inode->i_links_count = inode->i_nlink;
	raw_inode->i_blocks = inode->i_blocks;
	for(i = 0;i < LFS_N_BLOCKS; ++i)
		raw_inode->i_block[i] = LFS_I(inode)->i_data[i];
	raw_inode->i_snapi = LFS_I(inode)->i_snapi;
}

struct lfs_inode *lfs_get_inode(struct super_block *sb, ino_t ino,
                                 struct buffer_head **p)
{
	struct buffer_head *bh;
	unsigned long block = 0;

	*p = NULL;

	dprintk("lfs_get_inode called for %lu\n", ino);
	if(ino > LFS_MAX_INODES)
		goto einval_err;
	if(ino == LFS_IFILE_INODE) {
		struct lfs_super_block *lfssb;
		struct lfs_sb_info *sbi;

		sbi = sb->s_fs_info;
		lfssb = sbi->s_lfssb;
		block = lfssb->s_ifile_iaddr[lfssb->s_snapi];
	}
	else {
		iread_lock(sb);
		block = ifile_get_daddr(sb, ino);
		iread_unlock(sb);
		if(block == 0)
			goto eio_err;
	}

	dprintk("Reading inode %lu from disk block %lu\n", ino, block);
	if (!(bh = sb_bread(sb, block)))
		goto eio_err;
	*p = bh;
	return (struct lfs_inode *) bh->b_data;

einval_err:
	lfs_error(sb, "lfs_get_inode", "bad inode number: %lu", 
		(unsigned long) ino);
	return ERR_PTR(-EINVAL);
eio_err:
	lfs_error(sb, "lfs_get_inode",
	"unable to read inode block - inode=%lu, block=%lu",
	(unsigned long) ino, block);
	return ERR_PTR(-EIO);
}

/*
 * Test whether an inode is a fast symlink.
 */
static inline int lfs_inode_is_fast_symlink(struct inode *inode)
{
	if(inode->i_size < sizeof (LFS_I(inode)->i_data))
		return 1;
	return 0;
}

void lfs_read_inode (struct inode *inode)
{
	struct buffer_head *bh;
	struct lfs_inode *raw_inode = lfs_get_inode(inode->i_sb,
				       inode->i_ino, &bh);
	int i;

	if (IS_ERR(raw_inode))
		goto bad_inode;

	inode->i_mode = raw_inode->i_mode;
	inode->i_uid = (uid_t)raw_inode->i_uid;
	inode->i_gid = (gid_t)raw_inode->i_gid;
	inode->i_nlink = raw_inode->i_links_count;

	if (inode->i_nlink == 0 && inode->i_mode == 0) {
		/* this inode is deleted */
		brelse (bh);
		goto bad_inode;
	}
	
	inode->i_size = raw_inode->i_size;
	inode->i_atime.tv_sec = raw_inode->i_atime;
	inode->i_ctime.tv_sec = raw_inode->i_ctime;
	inode->i_mtime.tv_sec = raw_inode->i_mtime;
	inode->i_atime.tv_nsec 
		= inode->i_mtime.tv_nsec 
		= inode->i_ctime.tv_nsec = 0;
	inode->i_blocks = raw_inode->i_blocks;
	for(i = 0;i < LFS_N_BLOCKS; ++i)
		LFS_I(inode)->i_data[i] = raw_inode->i_block[i];
	LFS_I(inode)->i_snapi = raw_inode->i_snapi;

	if(S_ISREG(inode->i_mode)) {
		inode->i_op = &lfs_file_inode_operations;
		inode->i_fop = &lfs_file_operations;
		inode->i_mapping->a_ops = &lfs_aops;
	}
	else if(S_ISDIR(inode->i_mode)) {
		inode->i_op = &lfs_dir_inode_operations;
		inode->i_fop = &lfs_dir_operations;
		inode->i_mapping->a_ops = &lfs_aops;
	}
	else if (S_ISLNK(inode->i_mode)) {
		if (lfs_inode_is_fast_symlink(inode))
			inode->i_op = &lfs_fast_symlink_inode_operations;
		else {
			dprintk("setting operations to slow symlink\n");
			inode->i_op = &lfs_symlink_inode_operations;
			inode->i_mapping->a_ops = &lfs_aops;
		}
	}

	brelse(bh);
	return;
bad_inode:
	make_bad_inode(inode);
	return;
}

/* FIXME: have to move these two to ifile.c */
static inline sector_t get_daddr(struct inode *inode)
{	ino_t ino = inode->i_ino;
	if(ino == LFS_IFILE_INODE) {
		int snapi = LFS_I(inode)->i_snapi;
		return LFS_SBI(inode->i_sb)->s_lfssb->s_ifile_iaddr[snapi];
	}
	else 
		return ifile_get_daddr(inode->i_sb, ino);
}

inline void set_daddr(struct inode *inode, sector_t block)
{
	ino_t ino = inode->i_ino;
	//dprintk("Updating inode block for %lu to %Lu\n", ino, block);

	if(ino == LFS_IFILE_INODE) {
		int snapi = LFS_I(inode)->i_snapi;
		LFS_SBI(inode->i_sb)->s_lfssb->s_ifile_iaddr[snapi] = block;
	}
	else
		ifile_update_ife_block(inode, block);
}

int lfs_write_inode(struct inode * inode, int do_sync)
{
	int err = 0;
	struct lfs_inode raw_inode;
	sector_t block, newblock;
	struct super_block *sb = inode->i_sb;
	struct segment *segp = LFS_SBI(sb)->s_curr;
	long unsigned ino = inode->i_ino;

dprintk("lfs_write_inode called for %lu with size %Lu\n", ino, inode->i_size);
	fill_raw_inode(inode, &raw_inode);

	lfs_lock(sb);

	block = get_daddr(inode);
	newblock = segment_write_block(sb,&raw_inode,LFS_INODE_SIZE,0,block);
	set_daddr(inode, newblock);

	/* FIXME: tricky recursion might happen or may be not ;) */
	if(ino != LFS_SEGUSE_TABLE_INODE && newblock != block) 
		segusetbl_add_livebytes(sb, segp->segnum, LFS_INODE_SIZE);

	/* Possible segment expansion before this ?? */
	segsum_add_inob(segp,newblock); 
	lfs_unlock(sb);

	return err;
}

inline unsigned long dir_blocks(struct inode *inode)
{
	if(inode->i_size % LFS_BSIZE == 0)
		return inode->i_size / LFS_BSIZE;
	else 
		return inode->i_size / LFS_BSIZE + 1;
}

static inline int lfs_match (int len, const char * const name, lfs_dirent * de)
{
	if (len != de->name_len)
		return 0;
	if (!de->inode)
		return 0;
	return !memcmp(name, de->name, len);
}

int lfs_is_empty_dir(struct inode *dir)
{	
	int empty = 1, i;
	unsigned long nblocks = dir_blocks(dir);
	struct buffer_head *bh;
	lfs_dirent *pde;
	int reclen = LFS_DIRENT_RECLEN;

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
			if(pde->inode) {
				empty = 0;
				goto out;
			}
			pde += reclen;
		}
		brelse(bh);
	}
out:
	return empty;
}


int lfs_add_entry(struct inode * dir, const char * name, 
		  int namelen, int ino)
{	lfs_dirent de;
	lfs_dirent *pde;
	int err = 0, i;
	int offset = 0;
	sector_t block;
	struct segment *segp;
	struct buffer_head *bh = NULL;
	struct super_block *sb = dir->i_sb;
	unsigned reclen = LFS_DIRENT_RECLEN;
	unsigned long nblocks = dir_blocks(dir);
	

	//dprintk("lfs: lfs_add_entry called for %lu dir to add %s file with len = %d\n", dir->i_ino, name, namelen);
	for(i = 0;i < nblocks; ++i) {
		int dirend;

		bh = lfs_read_block(dir, i);
		if (!bh) {
			err = -EIO;
			goto out;
		}

		pde = (lfs_dirent *) bh->b_data;
		if(i != nblocks - 1)
			dirend = LFS_BSIZE;
		else
			dirend = dir->i_size % LFS_BSIZE;
		for(offset = 0;offset < dirend; offset += reclen) {
			if(lfs_match(namelen, name, pde))
				goto found;	
			pde += reclen;
		}
		brelse(bh);
	}
	
	de.inode = ino;
	de.name_len = namelen;
	strncpy(de.name, name, namelen);

	dir->i_mtime = CURRENT_TIME;
	dir->i_size += reclen;

	/* store the current segment, as segment_write might change it later */
	segp = LFS_SBI(sb)->s_curr;

	if(offset > 0 && offset < LFS_BSIZE) {
		/* the block exists and there's room for more de */
		bh = lfs_read_block(dir, nblocks - 1);
		block = segment_write_block_from_bh(sb,&de,reclen,offset,bh); 
		if(block != bh->b_blocknr)
			update_inode(dir, nblocks - 1, block);
		brelse(bh);
	}
	else {
		++dir->i_blocks;
		block = segment_write_block(sb, &de, reclen, 0, 0); 
		update_inode(dir, nblocks, block);
	}
	segusetbl_add_livebytes(sb, segp->segnum, reclen);
	mark_inode_dirty(dir);
	goto out;

found:
	brelse(bh);
	err = -EEXIST;
out:
	return err;
}

static inline unsigned lfs_last_byte(struct inode *inode, int i)
{
	unsigned last_byte = inode->i_size;

	last_byte -= i * LFS_BSIZE;
	if(last_byte > LFS_BSIZE)
		last_byte = LFS_BSIZE;
	return last_byte;
}

struct buffer_head * lfs_find_entry(struct inode * dir, 
	const char * name, int namelen, lfs_dirent ** res_dir, sector_t *iblock)
{
	unsigned reclen = LFS_DIRENT_RECLEN;
	unsigned long nblocks = dir_blocks(dir);
	struct buffer_head * bh = NULL;
	lfs_dirent * pde;
	int i;

	
	if (namelen > LFS_NAME_LEN)
		return NULL;

	if(nblocks == 0)
		goto not_found;
	
	*res_dir = NULL;

	for(i = 0;i < nblocks; ++i) {
		int dirend, offset;

		bh = lfs_read_block(dir, i);
		if (!bh) {
			dprintk("bh not found ??\n");
			goto not_found;
		}

		pde = (lfs_dirent *) bh->b_data;

		dirend = lfs_last_byte(dir, i);
	
		for(offset = 0;offset < dirend;offset += reclen) {
			pde = (lfs_dirent *)(bh->b_data + offset);
			if(lfs_match(namelen, name, pde))
				goto found;	
		}
		brelse(bh);
	}

not_found:
	return NULL;
found:
	*res_dir = pde;
	if(iblock)
		*iblock = i;
	return bh;
}

struct dentry *lfs_lookup (struct inode * dir, struct dentry *dentry,
			   struct nameidata *nd)
{

        struct inode * inode = NULL;
	struct buffer_head *bh;
	lfs_dirent *de;
        
	dprintk("lfs: lfs_lookup for %s\n", dentry->d_name.name);
	bh = lfs_find_entry(dir, dentry->d_name.name, dentry->d_name.len, &de, NULL);
	if(bh) {
		unsigned long ino = le32_to_cpu(de->inode);
		brelse(bh);
		inode = iget(dir->i_sb, ino);
		if (!inode) {
			return ERR_PTR(-EACCES);
		}
	}
        d_add(dentry, inode);
        return NULL;
}

struct inode *lfs_new_inode(struct inode *dir, int mode)
{	
	struct super_block *sb = dir->i_sb;
	struct inode *inode;
	struct lfs_inode_info *ei;
	ino_t ino = 0;

	inode = new_inode(sb);
	if(!inode)
		return ERR_PTR(-ENOMEM);
	ei = LFS_I(inode);
	ino = ifile_get_next_free_inode(sb);
	
	inode->i_uid = current->fsuid;
	inode->i_gid = (dir->i_mode & S_ISGID) ? dir->i_gid : current->fsgid;
	inode->i_mode = mode;
	inode->i_ino = ino;
	inode->i_blksize = LFS_BSIZE;
	inode->i_blocks = 0;
	inode->i_size = 0;
	inode->i_nlink = 0;
	inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;
	memset(ei->i_data, 0, sizeof(ei->i_data)); /* zero the data blocks */

	return inode;
}

static int lfs_write_symlink(struct inode *inode, const char *symname, unsigned len)
{
	int block;

	block = __segment_write_block(inode->i_sb, (void *)symname, len);
	return update_inode(inode, 0, block);
}

int lfs_symlink (struct inode * dir, struct dentry * dentry,
			 const char * symname)
{
	struct super_block * sb = dir->i_sb;
	struct inode * inode;
	int err = -ENAMETOOLONG;
	unsigned l = strlen(symname)+1;

	if (l > sb->s_blocksize)
		goto out;

	inode = lfs_new_inode (dir, S_IFLNK | S_IRWXUGO);
	err = PTR_ERR(inode);
	if (IS_ERR(inode))
		goto out;

	if (l > sizeof (LFS_I(inode)->i_data)) {
		/* slow symlink */
		inode->i_op = &lfs_symlink_inode_operations;
		inode->i_mapping->a_ops = &lfs_aops;
		err = lfs_write_symlink(inode, symname, l);
		if (err)
			goto out_fail;
	} else {
		/* fast symlink */
		inode->i_op = &lfs_fast_symlink_inode_operations;
		memcpy((char*)(LFS_I(inode)->i_data),symname,l);
		inode->i_size = l-1;
	}
	insert_inode_hash(inode);
	mark_inode_dirty(inode);
	
	err = lfs_add_entry(dir, dentry->d_name.name, 	
			    dentry->d_name.len, inode->i_ino);
	if(err)
		goto out_fail;
	d_instantiate(dentry, inode);
out:
	return err;

out_fail:
	mark_inode_dirty(inode);
	iput (inode);
	goto out;
}

int lfs_create (struct inode * dir, struct dentry * dentry, 
                       int mode, struct nameidata *nd)
{
	struct inode * inode = lfs_new_inode (dir, mode);
	int err = PTR_ERR(inode);

	dprintk("lfs_create called for %s\n", dentry->d_name.name);
	if (!IS_ERR(inode)) {
		inode->i_op = &lfs_file_inode_operations;
		inode->i_fop = &lfs_file_operations;
		inode->i_mapping->a_ops = &lfs_aops;
		inode->i_nlink++;
		insert_inode_hash(inode);
		mark_inode_dirty(inode);  
		lfs_lock(inode->i_sb);
		err = lfs_add_entry(dir, dentry->d_name.name, 
				    dentry->d_name.len, inode->i_ino);
		lfs_unlock(inode->i_sb);
		if(err)
			goto out_fail;
		d_instantiate(dentry, inode);
	}
	return 0;
out_fail:
	--inode->i_nlink;
	mark_inode_dirty(inode);
	iput(inode);
	return err;
}

struct inode_operations lfs_file_inode_operations;
