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
#include <time.h>


unsigned char *disk;

struct ext2_super_block *sb;

struct ext2_group_desc *gd;

void* inode_table;

void* bitmap_block;

unsigned int inode_bitmap_offset;

int main(int argc, char **argv) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <image file name> <abosolute path>\n", argv[0]);
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

	bitmap_block = disk + EXT2_BLOCK_SIZE * gd->bg_block_bitmap;

	inode_bitmap_offset = gd->bg_inode_bitmap;


	char * path = malloc(sizeof(char)* (strlen(argv[2])+4) );
	if (path == NULL){
        exit(-1);
    }
    strncpy(path, argv[2],strlen(argv[2]));

    //check absolute path
	if (path[0] != '/') {
		free(path);
		fprintf(stderr, "Input has to be an absolute path\n");
		exit(ENOENT);
	}


	char * parent_path = get_parent_path(argv[2]);

	char *file_name = get_file_name(argv[2],parent_path);

	//check length of target file
	if (strlen(file_name) > EXT2_NAME_LEN) {
		free(path);
		fprintf(stderr, "The name of target is too long\n");
		exit(EINVAL);
	}

	unsigned int target_inode_num_reg = find_inode_num(path,file_name,EXT2_FT_REG_FILE);
	unsigned int target_inode_num_slnk = find_inode_num(path,file_name,EXT2_FT_SYMLINK);
	unsigned int target_inode_num_dir = find_inode_num(path,file_name,EXT2_FT_DIR);

	//check if it is a directory
	if (target_inode_num_dir != 0){
		free(path);
		fprintf(stderr, "The target must be a file or link\n");
		exit(EISDIR);
	}

	//check if the target exists
	if (target_inode_num_reg == 0 && target_inode_num_slnk == 0){
		free(path);
		fprintf(stderr, "The target doesn't exist\n");
		exit(EEXIST);
	}


	struct ext2_inode *target_inode;
	if (target_inode_num_reg != 0){
		target_inode = (struct ext2_inode *)(inode_table + (target_inode_num_reg-1)*sizeof(struct ext2_inode));
	}
	if (target_inode_num_slnk != 0){
		target_inode = (struct ext2_inode *)(inode_table + (target_inode_num_slnk-1)*sizeof(struct ext2_inode));
	}




	struct ext2_inode *parent_inode = find_inode_dir(parent_path);

	add_entry_rm(parent_inode,target_inode,file_name);

	munmap(disk,128 * 1024);
	free(path);
	return 0;

}
