/* $Id: super.c,v 1.26 2005/08/23 20:29:48 ppadala Exp $
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>

#include <linux/fs.h>
#include <linux/list.h>
#include <linux/statfs.h>
#include <linux/buffer_head.h>
#include <linux/pagemap.h>

#include "include/lfs_kernel.h"

extern int lfs_write_inode(struct inode * inode, int do_sync);
extern void lfs_read_inode (struct inode *inode);

static struct file_system_type lfs_type;

static kmem_cache_t * lfs_inode_cachep;

static struct inode *lfs_alloc_inode(struct super_block *sb)
{
	struct lfs_inode_info *ei;
	ei = (struct lfs_inode_info *)kmem_cache_alloc(lfs_inode_cachep, SLAB_KERNEL);
	if (!ei)
		return NULL;
      	return &ei->vfs_inode;
}

static void lfs_destroy_inode(struct inode *inode)
{
	kmem_cache_free(lfs_inode_cachep, LFS_I(inode));
}

static void init_once(void * foo, kmem_cache_t * cachep, unsigned long flags)
{
	struct lfs_inode_info *ei = (struct lfs_inode_info *) foo;

	if ((flags & (SLAB_CTOR_VERIFY|SLAB_CTOR_CONSTRUCTOR)) ==
	    SLAB_CTOR_CONSTRUCTOR) {
		rwlock_init(&ei->i_meta_lock);
		inode_init_once(&ei->vfs_inode);
	}
}
 
static int init_inodecache(void)
{
	lfs_inode_cachep = kmem_cache_create("lfs_inode_cache",
					     sizeof(struct lfs_inode_info),
					     0, SLAB_HWCACHE_ALIGN|SLAB_RECLAIM_ACCOUNT,
					     init_once, NULL);
	if (lfs_inode_cachep == NULL)
		return -ENOMEM;
	return 0;
}

static void destroy_inodecache(void)
{
	if (kmem_cache_destroy(lfs_inode_cachep))
		printk(KERN_INFO "lfs_inode_cache: not all structures were freed\n");
}

/* called when inode->i_nlink = 0 */
/* quite easy for LFS. no bmap, just update ifile bit map */
void lfs_delete_inode (struct inode * inode)
{
	free_inode_blocks(inode);
	if(inode->i_ino != LFS_IFILE_INODE)
		ifile_delete_inode(inode);
	clear_inode(inode);     /* We must guarantee clearing of inode... */
}

/*
 * This will be called to get the filesystem information like size etc.
 */
int lfs_statfs( struct super_block *sb, struct kstatfs *buf ) 
{	struct lfs_sb_info *sbi = LFS_SBI(sb);
	
	buf->f_type = LFS_MAGIC;
	buf->f_bsize = sb->s_blocksize;
	buf->f_blocks = sbi->s_blocks_count;
	buf->f_bfree = sbi->s_free_blocks_count;
	buf->f_bavail = sbi->s_free_blocks_count;
	buf->f_files = LFS_MAX_INODES;
	buf->f_ffree = LFS_MAX_INODES - sbi->s_lfssb->s_nino;
	buf->f_namelen = LFS_NAME_LEN;
	dprintk( "lfs: super_operations.statfs called\n" );
	return 0;
}

#if 0
struct dentry *d_alloc_ifile(struct dentry *parent, struct inode *inode)
{	struct dentry *res = NULL;
	const struct qstr name = { .name = ".ifile", .len = 6 };

	res = d_alloc(parent, &name);
	d_add(res, inode);

	return res;
}
#endif

static int lfs_fill_super (struct super_block *sb, void *data, int silent)
{	struct inode *root, *ifile;
	struct buffer_head *bh;
	struct lfs_super_block *lfssb;
	struct lfs_sb_info *sbi;
	int blocksize;
	struct segment *segp;

	sbi = kmalloc(sizeof(*sbi), GFP_KERNEL);
	if (!sbi)
		return -ENOMEM;
	sb->s_fs_info = sbi;
	memset(sbi, 0, sizeof(*sbi));
	
	init_rwsem(&sbi->seg_sem);
	sbi->s_ifile = kmalloc(sizeof(struct ifile), GFP_KERNEL);
	if (!sbi->s_ifile) {
		kfree(sbi);
		return -ENOMEM;
	}

	sbi->s_seguse_table = kmalloc(sizeof(struct seguse_table), GFP_KERNEL);
	if (!sbi->s_seguse_table) {
		kfree(sbi->s_ifile);
		kfree(sbi);
		return -ENOMEM;
	}

	blocksize = sb_min_blocksize(sb, BLOCK_SIZE);
	if (!blocksize) {
		dprintk ("LFS: unable to set blocksize\n");
		goto failed_mount;
	}

	/* Read superblock */
	if (!(bh = sb_bread(sb, LFS_START))) {
		dprintk ("LFS: unable to read superblock\n");
		goto failed_mount;
	}
	
	sbi->s_sbh = bh;
	lfssb = (struct lfs_super_block *)(bh->b_data);
	sbi->s_lfssb = lfssb;

	if(lfssb->s_version != LFS_SB_VERSION) {
		printk(KERN_ERR "LFS: wrong version, bailing out ...\n");
		goto failed_mount;
	}

	if(lfssb->s_magic != LFS_MAGIC) {
		printk(KERN_ERR "LFS: not an LFS\n");
		goto failed_mount;
	}

	sbi->s_blocks_count = lfssb->s_blocks_count;
	sbi->s_free_blocks_count = lfssb->s_free_blocks_count;
	sbi->s_snapi = lfssb->s_snapi;
	/* before we start accessing sb, fill in various fields */
	sb->s_blocksize = blocksize;
    	sb->s_blocksize_bits = 10; /* change later */
    	sb->s_magic = lfssb->s_magic;
    	sb->s_op = &lfs_super_operations; // super block operations
    	sb->s_type = &lfs_type; // file_system_type
	
	init_segments(sb);
	segment_allocate_first(sb); /* create the first segment */

	dprintk(KERN_INFO "lfs: snapi: %d, IFILE inode is at %u\n",
		lfssb->s_snapi, lfssb->s_ifile_iaddr[lfssb->s_snapi]);

	/* Bootstrap, remember that root inode contains IFILE dentry, but
	   to read root, we need IFILE. */
	ifile = iget(sb, LFS_IFILE_INODE);
	if(!ifile)
		goto failed_mount;
	sbi->s_ifinodes[sbi->s_snapi] = ifile;
	IFILE(sb)->n_ife = lfssb->s_nino;
	IFILE(sb)->next_free = IFILE(sb)->n_ife + 1;

	sbi->s_seguse_table_inode = iget(sb, LFS_SEGUSE_TABLE_INODE);
	if(!sbi->s_seguse_table_inode)
		goto ifile_fail;

    	root = iget(sb, LFS_ROOT_INODE); // allocate an inode
    	root->i_op = &lfs_dir_inode_operations; // set the inode ops
    	root->i_mode = S_IFDIR|S_IRWXU;
    	root->i_fop = &lfs_dir_operations;

    	if(!(sb->s_root = d_alloc_root(root))) 
		goto root_fail;

	/* now, we can do some real business */
	segp = CURSEG(sb);
	segusetbl_set_segstate(sb, segp->segnum, SEGUSE_ACTIVE | SEGUSE_DIRTY);

	return 0;

root_fail:
	iput(root);
ifile_fail:
	iput(ifile);
failed_mount:
	sb->s_fs_info = NULL;
	return -EINVAL;
}


static void lfs_write_super (struct super_block * sb)
{
	struct lfs_sb_info *sbi = (struct lfs_sb_info *)sb->s_fs_info;

	sbi->s_lfssb->s_nino = IFILE(sb)->n_ife;
	sbi->s_lfssb->s_next_seg = sbi->s_curr->start;
	sbi->s_lfssb->s_seg_offset = sbi->s_curr->offset;
	sbi->s_lfssb->s_snapi = sbi->s_snapi;

	lock_kernel();
	mark_buffer_dirty(LFS_SBI(sb)->s_sbh);
	sync_dirty_buffer(sbi->s_sbh);
	unlock_kernel();
}

static void lfs_put_super (struct super_block * sb)
{	struct lfs_sb_info *sbi;
	int i;
	
	//dprintk("put_super called, iputting ifile\n");
	for(i = 0;i <= LFS_SBI(sb)->s_snapi; ++i)
		iput(LFS_SBI(sb)->s_ifinodes[i]);
	iput(LFS_SBI(sb)->s_seguse_table_inode);
	
	sbi = LFS_SBI(sb);
	lfs_write_super(sb);
	
	write_segments(sb);
	destroy_segments(sb);

	brelse(sbi->s_sbh);
	sb->s_fs_info = NULL;
	
	kfree(sbi->s_ifile);
	kfree(sbi->s_seguse_table);
	kfree(sbi);
}

static struct super_block *lfs_get_sb(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data)
{
	return get_sb_bdev(fs_type, flags, dev_name, data, lfs_fill_super);
}

struct super_operations lfs_super_operations = {
	.alloc_inode 	= lfs_alloc_inode,
	.destroy_inode  = lfs_destroy_inode,
	.read_inode	= lfs_read_inode,
	.write_inode	= lfs_write_inode,
	.delete_inode	= lfs_delete_inode,
	.statfs		= lfs_statfs,
	.put_super	= lfs_put_super,
	.write_super	= lfs_write_super,
};


static struct file_system_type lfs_type = {
	.owner		= THIS_MODULE,
	.name		= "lfs",
	.get_sb		= lfs_get_sb,
	.kill_sb	= kill_block_super,
	.fs_flags	= FS_REQUIRES_DEV,
};

static int __init init_lfs(void)
{
        int err = init_inodecache();
	if(err)
		goto out;
	
	err = register_filesystem(&lfs_type);
	if(err)
		goto free_icache;

	return 0;

free_icache:
	destroy_inodecache();
out:
	return err;
}

static void __exit exit_lfs(void)
{
	unregister_filesystem(&lfs_type);
	destroy_inodecache();
}

MODULE_AUTHOR("Pradeep Padala");
MODULE_DESCRIPTION("A Log Structured File System that Supports Snapshots");
MODULE_LICENSE("GPL");
MODULE_VERSION(LFS_VERSION);
module_init(init_lfs)
module_exit(exit_lfs)
