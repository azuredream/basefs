#ifndef _BASEFS_H
#define _BASEFS_H

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/buffer_head.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/uaccess.h>

/*
 * A unique "magic number" for BaseFS.
 * You can choose any 32-bit value that doesn't collide with known filesystems.
 */
#define BASEFS_MAGIC 0x62617365  /* 'b','a','s','e' in hex */

/*
 * Theoretical maximum file size: 1 PB = 2^50 bytes.
 */
#define BASEFS_MAX_FILESIZE (1ULL << 50)

/*
 * Default block size: 128 KB.
 * You may change this as needed.
 */
#define BASEFS_DEFAULT_BLOCK_SIZE 1024 * 128
/*
 * On-disk superblock structure.
 * For simplicity, we store minimal metadata here.
 * Real filesystems often include versioning, block bitmaps, etc.
 */
struct basefs_super_block {
	__le32 magic;
	__le64 blocks_count;
	__le64 inodes_count;
	/*
	 * Add more fields if needed:
	 *   - version
	 *   - block bitmap location
	 *   - inode bitmap location
	 *   - etc.
	 */
};

/*
 * In-memory superblock info.
 * Holds a pointer to the on-disk copy and possibly other runtime data.
 */
struct basefs_sb_info {
	struct basefs_super_block *raw_sb;
	unsigned long block_size;
	/*
	 * Additional runtime data (e.g., bitmaps, indexes) could go here.
	 */
};

/*
 * Inode private data for BaseFS.
 * We embed an actual struct inode and can store extra info if needed.
 */
struct basefs_inode_info {
	/*
	 * Add any custom data for BaseFS inodes:
	 *   - block offsets
	 *   - times
	 *   - etc.
	 */
	struct inode vfs_inode;  /* Must be last for container_of() usage */
};

/* Forward declarations for objects defined in other .c files. */
extern struct file_system_type basefs_fs_type;
extern const struct super_operations  basefs_super_ops;
extern const struct inode_operations  basefs_inode_ops;
extern const struct file_operations   basefs_file_ops;
extern const struct address_space_operations basefs_aops;

/* Function prototypes */
int basefs_fill_super(struct super_block *sb, void *data, int silent);
int basefs_save_sb(struct super_block *sb);  /* Optional for superblock writes */

#endif /* _BASEFS_H */
