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
	int rflag = 0;
	char * path = malloc(sizeof(char)* (strlen(argv[2])+4) );
	if (argc != 3 && argc != 4) {
		fprintf(stderr, "Usage: %s <image file name> [-r] <abosolute path>\n", argv[0]);
		exit(EINVAL);
	}

	strncpy(path, argv[2],strlen(argv[2]));

	if (argc == 4) {
		if (strcmp(argv[2], "-r")!=0) {
			printf("wrong opt\n");
			fprintf(stderr, "Usage: %s <image file name> <abosolute path>\n", argv[0]);
			exit(EINVAL);
		} else {
			rflag = 1;
			strncpy(path, argv[3],strlen(argv[3]));
		}
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


	
    

	path = helper_path_handler(path);

	//validate the path
    // if (strcmp("/", path) == 0) {
    // 	fprintf(stderr, "The root directory alreay exists\n");
    // 	return -EEXIST;
    // }

    //check absolute path
	if (path[0] != '/') {
		fprintf(stderr, "Input has to be an absolute path\n");
		return -ENOENT;
	}

	char * parent_path = get_parent_path(path);

	char *file_name = get_file_name(path,parent_path);

	//check length of target file
	if (strlen(file_name) > EXT2_NAME_LEN) {
		fprintf(stderr, "The name of target is too long\n");
		return -EINVAL;
	}

	unsigned int target_inode_num_reg = find_inode_num(path,file_name,EXT2_FT_REG_FILE);
	unsigned int target_inode_num_slnk = find_inode_num(path,file_name,EXT2_FT_SYMLINK);
	unsigned int target_inode_num_dir = find_inode_num(path,file_name,EXT2_FT_DIR);

	// Check the type of the target
	// Proccessing the regular file
	if (target_inode_num_reg != 0){
		remove_file(target_inode_num_reg, parent_path, file_name);
	}
	// Proccessing the symbolic link file
	else if (target_inode_num_slnk != 0) {
		remove_file(target_inode_num_slnk, parent_path, file_name);
	}
	// Processing the directory
	else if (target_inode_num_dir != 0) {

		if (!rflag) {
			fprintf(stderr, "Use -r to remove directory\n");
			return -EINVAL;
		}
		//remove_dir(target_inode_num_dir, path);	
		char *parent_parent_path = get_parent_path(parent_path);
		char *parent_file_name = get_file_name(parent_path, parent_parent_path);
		int parent_inode_num = find_inode_num(parent_path, parent_file_name, EXT2_FT_DIR);
		char *target_file_name = get_file_name(path, parent_path);

		if (parent_inode_num == 0)
			parent_inode_num = 2;
		
		// printf("parent_inode_num: %d\n", parent_inode_num);
		// printf("parent_path: %s\n", parent_path);
		// printf("target_file_name: %s\n", target_file_name);
		remove_root_dir(parent_inode_num, parent_path, target_file_name);
		

		// printf("parent_path: %s\n", parent_path);
		// printf("path: %s\n", path);
		// printf("target_inode_num_dir: %d\n", target_inode_num_dir);
		
		// remove_file(parent_inode_num, path, "..");
		// remove_file(target_inode_num_dir, path, ".");
		// remove_file(target_inode_num_dir, parent_path, target_file_name);

		// remove_file(13, /abc/level1, "..");
		// remove_file(14, /abc/level1, ".");
		// remove_file(14, /abc, level1);
	} 
	// The target doesn't exist
	else {
		free(path);
		fprintf(stderr, "The target doesn't exist\n");
		return -EEXIST;
	}
	return 0;
}