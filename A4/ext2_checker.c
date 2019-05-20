#include "ext2_helper.h"
#include "ext2.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

unsigned char *disk;
struct ext2_super_block *sb;
struct ext2_group_desc *gd;
void *inode_table;
unsigned char *block_map;
unsigned char *inode_map;

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <image file name>\n", argv[0]);
		exit(EINVAL);
	}

	int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);

	gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE*2);

	inode_table = disk + EXT2_BLOCK_SIZE* gd->bg_inode_table;

	block_map = (unsigned char *)(disk + gd->bg_block_bitmap * EXT2_BLOCK_SIZE);

	inode_map = (unsigned char *)(disk + gd->bg_inode_bitmap * EXT2_BLOCK_SIZE);


	// Helpers
	struct ext2_inode *root = (struct ext2_inode *) (inode_table + (EXT2_ROOT_INO - 1) * sizeof(struct ext2_inode));
	int error = 0;
	error += check_inode_bitmap();
	error += check_block_bitmap();
	
	error += check_type(root);
	
	error += check_dtime();
	
	error += check_counter();

	if (error == 0)
		printf("No file system inconsistencies detected \n");
	else
		printf("%d file system inconsistencies detected\n", error);

	return 0;
}