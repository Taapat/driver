/* $Id: symlink.c,v 1.4 2005/08/18 08:31:41 ppadala Exp $
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

/* copied from ext2 code */
#include "include/lfs_kernel.h"

static int
lfs_readlink(struct dentry *dentry, char __user *buffer, int buflen)
{
	struct lfs_inode_info *ei = LFS_I(dentry->d_inode);
	return vfs_readlink(dentry, buffer, buflen, (char *)ei->i_data);
}

static int lfs_follow_link(struct dentry *dentry, struct nameidata *nd)
{
	struct lfs_inode_info *ei = LFS_I(dentry->d_inode);
	return vfs_follow_link(nd, (char *)ei->i_data);
}

struct inode_operations lfs_symlink_inode_operations = {
#ifdef LINUX_2_6_11
	.readlink	= generic_readlink,
	.follow_link	= page_follow_link_light,
#else
	.readlink	= page_readlink,
	.follow_link	= page_follow_link,
#endif
};
 
struct inode_operations lfs_fast_symlink_inode_operations = {
	.readlink	= lfs_readlink,
	.follow_link	= lfs_follow_link,
};
