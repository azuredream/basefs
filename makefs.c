#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BASEFS_MAGIC               0x62617365
#define BASEFS_DEFAULT_BLOCK_SIZE  4096

/*
 * This structure should match the on-disk BaseFS super_block layout.
 * We only need minimal fields for a demonstration.
 */
struct basefs_super_block {
	uint32_t magic;
	uint64_t blocks_count;
	uint64_t inodes_count;
};

/*
 * Usage:
 *   makefs <image-file> <number-of-blocks>
 *
 * Example:
 *   makefs basefs.img 1024
 * This creates a ~4 MB file (1024 * 4096) and writes a minimal superblock.
 */
int main(int argc, char *argv[])
{
	int fd;
	uint64_t blocks_count;
	uint64_t total_size;
	struct basefs_super_block sb;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <image-file> <number-of-blocks>\n", argv[0]);
		return 1;
	}

	blocks_count = strtoull(argv[2], NULL, 10);
	fd = open(argv[1], O_CREAT | O_RDWR, 0666);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	/*
	 * Expand the file to hold (blocks_count * block_size) bytes.
	 */
	total_size = blocks_count * (uint64_t)BASEFS_DEFAULT_BLOCK_SIZE;
	if (ftruncate(fd, total_size) < 0) {
		perror("ftruncate");
		close(fd);
		return 1;
	}

	/*
	 * Prepare and write the BaseFS superblock at the beginning (block #0).
	 */
	memset(&sb, 0, sizeof(sb));
	sb.magic        = BASEFS_MAGIC;
	sb.blocks_count = blocks_count;
	sb.inodes_count = 0; /* can be updated later */

	if (lseek(fd, 0, SEEK_SET) < 0) {
		perror("lseek");
		close(fd);
		return 1;
	}

	if (write(fd, &sb, sizeof(sb)) != sizeof(sb)) {
		perror("write superblock");
		close(fd);
		return 1;
	}

	close(fd);
	printf("Created BaseFS image '%s' with %llu blocks (%llu bytes total).\n",
	       argv[1],
	       (unsigned long long)blocks_count,
	       (unsigned long long)total_size);
	return 0;
}
