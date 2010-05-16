/* $Id: lfs.h,v 1.23 2005/08/23 20:31:04 ppadala Exp $
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

#ifndef _LFS_H
#define _LFS_H

#include <linux/types.h>
#include <linux/smp_lock.h>
#include <linux/ioctl.h>

#define LINUX_2_6_11

#define LFS_MAX_SNAPSHOTS 2

struct lfs_super_block {
	__u32 s_blocks_count; /* Total number of blocks available to the FS */
	__u32 s_free_blocks_count; /* Number of free blocks */
	__u32 s_minfreeseg;   /* Minimum free segment that should be available*/
	__u32 s_ckpt_address; /* Block address of the ckpoint region */
	__u32 s_snapi;	      /* Current snapshot index */
	__u32 s_ifile_iaddr[LFS_MAX_SNAPSHOTS];  /* IFILE inode block numbers */
	__u32 s_nino;	      /* Number of allocated inodes in the system */
	__u32 s_next_seg;     /* Block number of the next segment */
	__u32 s_nseg;         /* Number of segments in the FS */
	__u32 s_seg_offset;   /* Offset into the current segment */
	__u32 s_segsize;      /* Segment size in our file system  in blocks */
	__u32 s_magic;        /* LFS magic number */
	__u8  s_version;      /* SB version */
};

/* on-disk file information. One per file */
struct finfo {
	__u32 fi_nblocks;	/* number of blocks */
	__u32 fi_version;	/* version number */
	__u32 fi_ino;		/* inode number, identifies the file */
	__u32 fi_lastlength;    /* length of last block in array */
	daddr_t *fi_lbn;	/* array of logical block numbers */
	/* NetBSD doesn't have this, and uses some pointer magic in the
	 * cleaner to get the blocks, It's too error-prone and full of
	 * assumptions */
	daddr_t *fi_blocks;	/* array of the corresponding disk block 
				   numbers */
}__attribute__((__packed__));
/* note that FINFOSIZE doesn't include the fi_blocks pointer */
#define FINFOSIZE (4 * sizeof(__u32)) 

/* one can dynamically allocate finfos, inos, but the overhead of freeing
   and re-allocating everytime an finfo or ino is added is not worth the
   effort */
/* Is there a krealloc ? */
#define MAX_FIS (LFS_SEGSIZE / 2)
#define MAX_FI_BLOCKS (LFS_SEGSIZE * 4 - 1)
#define MAX_INO_BLOCKS (LFS_SEGSIZE * 4 - 1)

/* inode entries in the ifile */
struct ifile_entry {
	__u32 ife_version; 	/* inode version number */
	__u32 ife_daddr;	/* inode disk address	*/
	__u32 ife_atime;	/* last access time	*/
	__u32 pad;		/* padding, has to be aligned with block
				   boundary */
};
#define IFE_SIZE (sizeof(struct ifile_entry))

/* The in-memory ifile */
struct ifile {
	__u32 n_ife; 		/* number of ifile entries (number of inodes) */
	__u32 next_free; 	/* next available free inode */
	__u32 daddr; 		/* ifile data block */
	struct ifile_entry *ifes; /* array of ifes */
};

struct segsum {
	__u32 ss_sumsum;         /* check sum of summary block */
	__u32 ss_datasum;        /* check sum of data */
	__u32 ss_magic;          /* segment summary magic number */
#define SS_MAGIC        0x061561
	__u32 ss_next;           /* Next segment */
	__u32 ss_create;         /* Creation time stamp */
	__u32 ss_nfinfo;	 /* Number of finfos */
	__u32 ss_nino; 	 /* Number of inodes in the segment */
	/* inode block numbers and finfos */
};
#define SEGSUM_SIZE (sizeof(struct segsum))
#define DADDRT_SIZE (sizeof(daddr_t))

#define LFS_DIRENT_RECLEN 256
#define LFS_NAME_LEN (LFS_DIRENT_RECLEN - 5)

typedef struct {
	__u32  inode;                  /* Inode number */
	__u8    name_len;              /* Name length */
	char    name[LFS_NAME_LEN];    /* File name */
} lfs_dirent;

/*
 * Cleaner information structure.  We are storing this in segusage table
 * (NetBSD stored this in IFILE). The idea is to have all cleaning related
 * stuff in segusage table
 */
struct cleanerinfo{
	__u32 clean;                /* number of clean segments */
	__u32 dirty;                /* number of dirty segments */
	__u32 bfree;                /* disk blocks free */
	__u32 avail;                /* disk blocks available */
};
#define LFS_CI_SIZE (sizeof(struct cleanerinfo))

/* segusage table is an array segusage structures */
struct segusage {
	__u32 su_nbytes;            /* Number of live bytes */
	__u32 su_olastmod;          /* SEGUSE last modified timestamp */
	__u16 su_nsums;             /* Number of summaries in segment */
	__u64 su_lastmod;           /* Last modified timestamp */

#define SEGUSE_CLEAN            0x00    /*  segment just initd or cleaned */
#define SEGUSE_ACTIVE           0x01    /*  segment currently being written */
#define SEGUSE_DIRTY            0x02    /*  segment has data in it */
#define SEGUSE_ERROR            0x04    /*  cleaner: do not clean segment */
#define SEGUSE_EMPTY            0x08    /*  segment is empty */

	__u32 su_flags;             /* Segment flags */

};
#define LFS_SEGUSE_SIZE (sizeof(struct segusage))

/* The in-memory array of seguse_table file */
struct seguse_table {
	struct cleanerinfo ci;
	int nseguse; /* Number of segusage structures in this array */
};

typedef struct block_info {
	__u32   bi_inode;               /* inode # */
	__u32 	bi_lbn;                 /* logical block w/in file */
	unsigned long bi_daddr;              /* disk address of block */
	__u64   bi_segcreate;       	/* origin segment create time */
	__u32   bi_version;             /* file version number */
	void    *bi_bp;                 /* data buffer */
	size_t  bi_size;                /* size of the block (if fragment) */
} BLOCK_INFO;

/*
 * Constants relative to the data blocks
 */
#define	LFS_NDIR_BLOCKS		12
#define	LFS_IND_BLOCK		LFS_NDIR_BLOCKS
#define	LFS_DIND_BLOCK		(LFS_IND_BLOCK + 1)
#define	LFS_TIND_BLOCK		(LFS_DIND_BLOCK + 1)
#define	LFS_N_BLOCKS		(LFS_TIND_BLOCK + 1)

#define LFS_UNUSED_DADDR        0 
/*
 * Structure of an inode on the disk, copied from ext2 and modified
 */
struct lfs_inode {
	__u32   i_ino;		/* Inode number, needed by cleaner */
	__u16	i_mode;		/* File mode */
	__u16	i_uid;		/* Low 16 bits of Owner Uid */
	__u16	i_gid;		/* Low 16 bits of Group Id */
	__u32	i_size;		/* Size in bytes */
	__u32	i_atime;	/* Access time */
	__u32	i_ctime;	/* Creation time */
	__u32	i_mtime;	/* Modification time */
	__u32	i_dtime;	/* Deletion Time */
	__u16	i_links_count;	/* Links count */
	__u32	i_blocks;	/* Blocks count */
	__u32	i_flags;	/* File flags */
	__u32	i_block[LFS_N_BLOCKS];/* Pointers to blocks */
	__u32	i_generation;	/* File version (for NFS) */
	__u32	i_file_acl;	/* File ACL */
	__u32	i_dir_acl;	/* Directory ACL */
	__u32	i_snapi;	/* Snapshot index */
};
#define LFS_INODE_SIZE (sizeof(struct lfs_inode))

#define LFS_MAGIC 0x010203ff

#define LFS_START 0 
	/* might have to change if we have to incorporate boot block */
#define LFS_BSIZE 1024 /* 1 disk block */
#define LFS_BSIZE_BITS 10 /*  should be derived from above number */
#define	LFS_ADDR_PER_BLOCK (LFS_BSIZE / sizeof (__u32))

#define LFS_IFILE_INODE 1
#define LFS_ROOT_INODE 2
#define LFS_SEGUSE_TABLE_INODE 3
#define LFS_SNAPSHOT_INODE 4
#define LFS_RESERVED_INODES 4
#define LFS_IFILE_NAME ".ifile"
#define LFS_SEGUSE_TABLE_NAME ".seguse_table"
#define LFS_SNAPSHOT_NAME ".snapshot"

#define LFS_SEGSIZE 128 /* 1/2 MB  */
//#define LFS_SEGSIZE 8 /* temporary for testing the cleaner */
#define LFS_MAX_INODES (1 << 31)
#define LFS_INODE_FREE 0

#define BUF_IN_PAGE (PAGE_SIZE / LFS_BSIZE)
#define LFS_VERSION "0.0.2"
#define LFS_SB_VERSION 1

/* where first segment starts, this has to be aligned with a page */
#define LFS_SEGSTART 4	
#define LFS_MAX_SEG_BLOCKS (LFS_SEGSIZE * 4096 - 1)	

#ifndef __KERNEL__
#define PAGE_SIZE 4096
#endif

#define lblkno(fs, loc) (loc >> LFS_BSIZE_BITS)
static inline int dtosn(unsigned long daddr)
{	
	return (daddr - LFS_SEGSTART) / (LFS_SEGSIZE * 4);
}

static inline daddr_t sntod(struct lfs_super_block *lfsp, int segnum)
{	daddr_t addr;

	addr = LFS_SEGSTART * LFS_BSIZE ;
	addr += segnum * LFS_SEGSIZE * PAGE_SIZE;
	return  addr;
}

/* LFS ioctl numbers */
struct lfs_ioc {
	BLOCK_INFO *bip;
	int nblocks;
};
#define LFS_IOC_SIZE (sizeof(struct lfs_ioc))

#define LFS_IOC_IDENT 0xEE
#define LFS_BMAP _IOWR(LFS_IOC_IDENT, 1, struct lfs_ioc *)
#define LFS_MARK _IOW(LFS_IOC_IDENT, 2, struct lfs_ioc *)
#define LFS_SNAP_CREATE _IOW(LFS_IOC_IDENT, 3, void *)

#define LFS_UNUSED_LBN  -1
#endif
