/* $Id: lfs_kernel.h,v 1.21 2005/08/23 20:31:13 ppadala Exp $
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

#ifndef _LFS_KERNEL_H
#define _LFS_KERNEL_H

#include <linux/fs.h>

#include "lfs.h"

/* ------------------------------DEBUG-STUFF------------------------------- */
#define DEBUG

#ifdef DEBUG
#define dprintk(x...)   printk(x)
#else
#define dprintk(x...)
#endif

#ifdef CONFIG_KERNEL_ASSERTS
#define assert(p) KERNEL_ASSERT(#p, p)
#else
#define assert(p) do {  \
	if (!(p)) {     \
		printk(KERN_CRIT "BUG at %s:%d assert(%s)\n",   \
			__FILE__, __LINE__, #p);                 \
			BUG();  \
		}               \
} while (0)
#endif
/* ------------------------------DEBUG-STUFF-END--------------------------- */


/* The in memory lfs sb info */
struct lfs_sb_info {
	int s_snapi; /* current ifile inode index */
	struct inode *s_ifinodes[LFS_MAX_SNAPSHOTS];
	struct inode *s_seguse_table_inode;
	struct ifile  *s_ifile;
	struct seguse_table *s_seguse_table;
	struct segment *s_curr; /* current segment that's being written to */
	struct lfs_super_block *s_lfssb;
	struct buffer_head *s_sbh; /* Pointer to sb buffer head */
	struct list_head *s_seglist; /* List of segments */
	/* Global segment lock, per segment locks seem to be tough, as writes
	   past segment boundary are creating problems (check older CVS
	   verions for per segment locks) */
	struct rw_semaphore seg_sem; 

	/* copies of stuff from super block */
	__u32 s_blocks_count;
	__u32 s_free_blocks_count;
};

#define LFS_SBI(sb) ((struct lfs_sb_info *)(sb->s_fs_info))
#define CURR_SNAPI(sb) (LFS_SBI(sb)->s_snapi)
#define IFILE(sb) (LFS_SBI(sb)->s_ifile)
#define SEGUSETBL(sb) (LFS_SBI(sb)->s_seguse_table)
#define CURR_IFILE_INODE(sb) (IFILE_INODE(sb, LFS_SBI(sb)->s_snapi))
#define IFILE_INODE(sb,si) (LFS_SBI(sb)->s_ifinodes[si])
#define SEGUSETBL_INODE(sb) (LFS_SBI(sb)->s_seguse_table_inode)

/*
 * in-memory inode data
 */
struct lfs_inode_info {
	__u32	i_data[15];
	__u32	i_flags;
	__u16	i_state;
	__u32	i_file_acl;
	__u32	i_dir_acl;
	__u32	i_dtime;
	__u32   i_snapi;	/* snapshot index */

#ifdef CONFIG_LFS_POSIX_ACL
	struct posix_acl	*i_acl;
	struct posix_acl	*i_default_acl;
#endif
	rwlock_t i_meta_lock;
	struct inode	vfs_inode;
};

static inline struct lfs_inode_info *LFS_I(struct inode *inode)
{
	return container_of(inode, struct lfs_inode_info, vfs_inode);
}

extern struct super_operations lfs_super_operations;

extern struct file_operations lfs_dir_operations;
extern struct file_operations lfs_file_operations;

extern struct inode_operations lfs_dir_inode_operations;
extern struct inode_operations lfs_file_inode_operations;
extern struct inode_operations lfs_symlink_inode_operations;
extern struct inode_operations lfs_fast_symlink_inode_operations;

extern struct address_space_operations lfs_aops;

/* utility functions */
extern void lfs_error (struct super_block * sb, const char * function,
		 const char * fmt, ...); 
extern void lfs_warning (struct super_block * sb, const char * function,
		   const char * fmt, ...);

/* inode.c */
struct buffer_head * lfs_find_entry(struct inode * dir, 
	const char * name, int namelen, lfs_dirent ** res_dir, 
	sector_t *iblock);

/* ioctl.c */
int lfs_ioctl (struct inode * inode, struct file * filp, unsigned int cmd,
		unsigned long arg);

#include "ifile.h"
#include "tree.h"
#include "segment.h"
#include "lock.h"

#endif
