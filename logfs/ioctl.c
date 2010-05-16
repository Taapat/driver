/* $Id: ioctl.c,v 1.2 2005/08/23 20:28:55 ppadala Exp $
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
#include "include/lfs_kernel.h"

extern void lfs_read_inode (struct inode *inode);
extern void set_daddr(struct inode *inode, sector_t block);
extern int lfs_write_inode(struct inode * inode, int do_sync);

static void map_blocks(struct super_block *sb, BLOCK_INFO *bip, int nblocks)
{	int i;
	/* Locking not necessary, lfs_get_inode gets the ifile lock */
	/* FIXME: Lock inode->i_sem ?? */
	for(i = 0;i < nblocks; ++i) {
		if(bip[i].bi_lbn == LFS_UNUSED_LBN) /* inode block */
			bip[i].bi_daddr = ifile_get_daddr(sb, bip[i].bi_inode);
		else { /* file block */
			struct inode *inode;

			inode = new_inode(sb);
			inode->i_ino = bip[i].bi_inode;
			lfs_read_inode(inode);
			iput(inode);
			bip[i].bi_daddr = lfs_disk_block(inode, bip[i].bi_lbn);
		}
		/* cleaner expects disk addresses, not block numbers */
		bip[i].bi_daddr = bip[i].bi_daddr * LFS_BSIZE; 
		/*dprintk("ino %d lbn %d daddr = %lu\n", bip[i].bi_inode, 
			 bip[i].bi_lbn, bip[i].bi_daddr);*/
	}
}

static void mark_blocks(struct super_block *sb, BLOCK_INFO *bip, int nblocks)
{
	int i;
	/* FIXME: Locks ?? */
	for(i = 0;i < nblocks; ++i) {
		struct inode *inode;

		inode = new_inode(sb);
		inode->i_ino = bip[i].bi_inode;
		lfs_read_inode(inode);

		if(bip[i].bi_lbn == LFS_UNUSED_LBN) /* inode block */
			set_daddr(inode, bip[i].bi_daddr / LFS_BSIZE);
		else
			update_inode(inode, bip[i].bi_lbn, 
				     bip[i].bi_daddr / LFS_BSIZE);
		mark_inode_dirty(inode);
		iput(inode);
	}
}
int lfs_cleaner_ioctl (	struct inode * inode, struct file * filp, 
			unsigned int cmd, unsigned long arg)
{
	struct lfs_ioc lioc;
	BLOCK_INFO *bip;
	int nblocks;

	if(inode->i_ino != LFS_IFILE_INODE)
		return -ENOTTY;

	if(copy_from_user(&lioc, (struct lfs_ioc __user *)arg, LFS_IOC_SIZE))
		return -EFAULT;
	nblocks = lioc.nblocks;
	if(!(nblocks > 0 && nblocks <= MAX_FI_BLOCKS))
		return -ENOTTY;
	bip = kcalloc(nblocks, sizeof(BLOCK_INFO), GFP_KERNEL);
	BUG_ON(!bip);
	if(copy_from_user(bip, lioc.bip, nblocks * sizeof(BLOCK_INFO)))
		goto free_bip;

	/*for(i = 0;i < nblocks; ++i) {
		dprintk("inode: %d lbn: %d daddr: 0x%lx\n", bip[i].bi_inode, 
			bip[i].bi_lbn, (unsigned long)(bip[i].bi_daddr));
	}*/

	switch(cmd) {
		case LFS_BMAP:
			map_blocks(inode->i_sb, bip, nblocks);
			break;
		case LFS_MARK:
			mark_blocks(inode->i_sb, bip, nblocks);
			break;
		default:
			goto free_bip;
	}

	if(copy_to_user(lioc.bip, bip, nblocks * sizeof(BLOCK_INFO)))
		goto free_bip;

	kfree(bip);
	return 0;

free_bip:
	kfree(bip);
	return -ENOTTY;

}

void copy_ifile_inode(struct super_block *sb, int to, int from)
{
	int i;
	struct inode *toi;
	struct inode *fromi = IFILE_INODE(sb, from);

	toi = new_inode(sb);
	toi->i_ino = fromi->i_ino;
	toi->i_mode = fromi->i_mode;
	toi->i_uid = fromi->i_uid;
	toi->i_gid = fromi->i_gid;
	toi->i_size = fromi->i_size;

	toi->i_atime = toi->i_ctime = toi->i_mtime = CURRENT_TIME;

	toi->i_nlink = fromi->i_nlink;
	toi->i_blocks = fromi->i_blocks;
	for(i = 0;i < LFS_N_BLOCKS; ++i)
		LFS_I(toi)->i_data[i] = LFS_I(fromi)->i_data[i];
	LFS_I(toi)->i_snapi = to;

	IFILE_INODE(sb, to) = toi;
}

int lfs_snap_ioctl (struct inode * inode, struct file * filp, unsigned int cmd,
		unsigned long arg)
{
	struct super_block *sb = inode->i_sb;
	int snapi;

	/* locks ?? */
	snapi = CURR_SNAPI(sb);
	CURR_SNAPI(sb) = snapi + 1;
	copy_ifile_inode(sb, snapi+1, snapi);

	lfs_write_inode(IFILE_INODE(sb, CURR_SNAPI(sb)), 1);

	return 0;
}

int lfs_ioctl (struct inode * inode, struct file * filp, unsigned int cmd,
		unsigned long arg)
{	

	dprintk("lfs_ioctl called\n");
	switch(cmd) {
		case LFS_BMAP:
		case LFS_MARK:
			return lfs_cleaner_ioctl(inode, filp, cmd, arg);
		case LFS_SNAP_CREATE:
			return lfs_snap_ioctl(inode, filp, cmd, arg);
		default:
			return -ENOTTY;

	}
}
