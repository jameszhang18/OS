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
    path = helper_path_handler(path);

	//validate the path
    if (strcmp("/", path) == 0) {
		free(path);
    	fprintf(stderr, "The root directory alreay exists\n");
    	exit(EEXIST);
    }

	if (path[0] != '/') {
		free(path);
		fprintf(stderr, "Input has to be an absolute path\n");
		exit(ENOENT);
	}

	
	char * parent_path = get_parent_path(path);
	char *dir_name = get_file_name(path,parent_path);
	unsigned int target_inode_num_dir = find_inode_num(path,dir_name,EXT2_FT_DIR);
	unsigned int target_inode_num_reg = find_inode_num(path,dir_name,EXT2_FT_REG_FILE);
	unsigned int target_inode_num_slnk = find_inode_num(path,dir_name,EXT2_FT_SYMLINK);

	//check if it is a directory
	if (target_inode_num_dir != 0 || target_inode_num_slnk != 0 ||target_inode_num_reg!=0){
		free(path);
		fprintf(stderr, "Directory already exists\n");
		exit(EISDIR);
	}

	if (strlen(dir_name) > EXT2_NAME_LEN) {
		free(path);
		fprintf(stderr, "The name of directory is too long\n");
		exit(EINVAL);
	}

	struct ext2_inode *parent_inode = find_inode_dir(parent_path);

	if (parent_inode == NULL){
		free(path);
		fprintf(stderr, "The parent directory doesn't exist\n");
		exit(EEXIST);
	}


	inode_bitmap_offset = gd->bg_inode_bitmap;

	// Get an inode number for the directory
	unsigned int free_inode = find_free_inode();
	add_bitmap(free_inode,1);
	// Get the block for the directory
	unsigned int free_block = find_free_block();
	add_bitmap(free_block,2);

	// Set up dir data
	// .
	struct ext2_dir_entry *new_dir = (struct ext2_dir_entry*)(disk + EXT2_BLOCK_SIZE*free_block);
	new_dir->name_len = 1;
	new_dir->file_type = EXT2_FT_DIR;
	new_dir->inode = free_inode;
	new_dir->rec_len = get_rec_len(1);
	strcpy(new_dir->name,".");
	int temp_rec_len = new_dir->rec_len;


	// ..
	struct ext2_dir_entry *parent_dir = (struct ext2_dir_entry *)(disk + parent_inode->i_block[0] * EXT2_BLOCK_SIZE);
	new_dir = (void*)(new_dir) + new_dir->rec_len;
	new_dir->inode = parent_dir->inode; //inode of parent directory
	new_dir->name_len = 2;
	new_dir->file_type = EXT2_FT_DIR;
	new_dir->rec_len = EXT2_BLOCK_SIZE - temp_rec_len;
	strcpy(new_dir->name,"..");


	// Set up the inode data
	struct ext2_inode *new_inode = (struct ext2_inode *)(inode_table + (free_inode-1) * sb->s_inode_size);
	new_inode->i_mode = EXT2_S_IFDIR;
	new_inode->i_size = EXT2_BLOCK_SIZE;
	new_inode->i_links_count = 2;
	new_inode->i_blocks = 2;
	new_inode->i_dtime = 0;
	new_inode->i_block[0] = free_block;

	//update the parent inode
	parent_inode -> i_links_count ++ ;
	//update the group descriptor
	gd->bg_used_dirs_count ++;


	add_entry(parent_inode,dir_name,free_inode,EXT2_FT_DIR);
	free(dir_name);
	free(parent_path);
	free(path);
	munmap(disk,128 * 1024);
	return 0;
}
