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

void* inode_table;

void* bitmap_block;

unsigned int inode_bitmap_offset;

int main(int argc, char **argv) {
	char *src_path = malloc(256);
	char *dest_path = malloc(256);
	int symbolic = 0;

	if (argc != 4 && argc != 5) {
		fprintf(stderr, "Usage: %s <image file name> [-s] <source abosolute path> <dest abosolute path>\n", argv[0]);
		exit(EINVAL);
	}

	if (argc == 5) {
		if (strcmp(argv[2], "-s") != 0) {
			fprintf(stderr, "Usage: %s <image file name> [-s] <source abosolute path> <dest abosolute path>\n", argv[0]);
			exit(EINVAL);
		}
		symbolic = 1;
		strncpy(src_path, argv[3], strlen(argv[3]));
		strncpy(dest_path, argv[4], strlen(argv[4]));
	} else {
		src_path = argv[2];
		dest_path = argv[3];
	}

	if (strlen(src_path)>255 || strlen(dest_path)>255) {
		fprintf(stderr, "The abosolute path is too long\n");
		exit(EINVAL);
	}

	int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    char *dest_parent_path = get_parent_path(dest_path);
    char *src_parent_path = get_parent_path(src_path);
	char *dest_file_name = get_file_name(dest_path, dest_parent_path);
	char *source_file_name = get_file_name(src_path, src_parent_path);

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

    if (src_path[0] != '/' || strlen(src_path) < 2) {
		free(path);
    	fprintf(stderr, "link file has to be an absolute path\n");
		return -ENOENT;
    }

    if (dest_path[0] != '/' || strlen(dest_path) < 2) {
		free(path);
    	fprintf(stderr, "dest path has to be an absolute path\n");
		return -ENOENT;
    }


	unsigned int src_inode_num = find_inode_num(src_path, source_file_name,EXT2_FT_REG_FILE);

	// Get the inode of the source path
	struct ext2_inode *src = (struct ext2_inode *)(inode_table + (src_inode_num-1)*sizeof(struct ext2_inode));
	if (src->i_mode & EXT2_S_IFDIR) {
		free(path);
		fprintf(stderr, "the source file is a directory\n");
		exit(EISDIR);
	}

	//check if the source file exists
	
	if (src_inode_num == 0) {
		free(path);
		fprintf(stderr, "Source file does not exist\n");
		exit(ENOENT);
	}


	struct ext2_inode *parent_inode = find_inode_dir(dest_parent_path);
	if (parent_inode == NULL){
		free(path);
		fprintf(stderr, "The parent directory doesn't exist\n");
		return -EEXIST;
	}

	// Set up the parent directory entry
	add_entry_ln(parent_inode,src, dest_file_name,src_path,src_inode_num,symbolic);

	free(path);
	munmap(disk,128 * 1024);
	return 0;
}
