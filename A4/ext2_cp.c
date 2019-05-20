#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include "ext2.h"
#include "ext2_helper.h"

unsigned char *disk;

struct ext2_super_block *sb;

struct ext2_group_desc *gd;

void* inode_table;

void* bitmap_block;

unsigned int inode_bitmap_offset;





int main(int argc, char **argv) {

	if (argc != 4) {
		fprintf(stderr, "Usage: %s <image file name> <native path> <absolute path on image>\n", argv[0]);
		exit(1);
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


    FILE *fp;
    fp = fopen(argv[2], "r");
    //check if the source file does not exist
    if(fp == NULL){
        fprintf(stderr, "Source file does not exist\n");
        exit(ENOENT);
	}



	char * path = malloc(sizeof(char)* (strlen(argv[3])+4) );
	if (path == NULL){
            exit(-1);
    }
    strncpy(path, argv[3],strlen(argv[3]));


	if (path[0] != '/') {
		free(path);
		fprintf(stderr, "Input has to be an absolute path\n");
		return -ENOENT;
	}


	struct ext2_inode * possible_inode_num_dir = find_inode_dir(path);
	struct ext2_inode *parent_inode;
	char *target_file_name;
	char * parent_path;



	//!!!!!!!!!!!!!!!!!!!!!!!!can only handle filepath end up with a /
	//without a specific filename-> copy the same filename
	if (possible_inode_num_dir != NULL){
		path = helper_path_handler(path);
		parent_inode = possible_inode_num_dir;
		parent_path = get_parent_path(argv[2]);
		target_file_name = get_file_name(argv[2],parent_path);


		strcat(path,target_file_name);

		if (find_inode_num(path,target_file_name,EXT2_FT_REG_FILE) != 0){
			free(path);
			fprintf(stderr, "The file already exists\n");
			exit(EEXIST);
		}
	}

	//with specific filename
	else{

		parent_path = get_parent_path(path);
		target_file_name = get_file_name(path,parent_path);
		//check if the target is a file with the same name that already exists
    	if (find_inode_num(path,target_file_name,EXT2_FT_REG_FILE) != 0){
			free(path);
			fprintf(stderr, "The file already exists\n");
			exit(EEXIST);
		}


		//check if a file name is given
		if (target_file_name == NULL){
			free(path);
			fprintf(stderr, "Have to give a file name\n");
			exit(ENOENT);
		}



		parent_inode = find_inode_dir(parent_path);

		if (strlen(target_file_name) > EXT2_NAME_LEN) {
			free(path);
			fprintf(stderr, "The name of target file is too long\n");
			exit(EINVAL);
		}


		//check if the target is a valid path
		if (parent_inode == NULL){
			free(path);
			fprintf(stderr, "The target path is invalid\n");
			exit(ENOENT);
		}

	}





	fseek(fp, 0L, SEEK_END);
	int sz = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	int num_of_blocks = 0;

	num_of_blocks = sz / EXT2_BLOCK_SIZE;
	if (sz % EXT2_BLOCK_SIZE != 0)
		num_of_blocks++;



	//check if the file is too large
	if (num_of_blocks > 1012){
		free(path);
		fprintf(stderr, "The target file is too large\n");
		exit(ENOSPC);
	}

	unsigned int free_inode = find_free_inode();

	struct ext2_inode *new_inode = (struct ext2_inode *)(inode_table + (free_inode-1) * sb->s_inode_size);
	new_inode->i_mode = EXT2_S_IFREG;
	new_inode->i_size = sz;
	new_inode->i_links_count = 1;
	if (num_of_blocks > 12)
		new_inode->i_blocks = 2*(num_of_blocks+1);
	else
		new_inode->i_blocks = 2*num_of_blocks;
	new_inode->i_dtime = 0;

	unsigned int i;

	for(i=0; i<15; i++){
		new_inode->i_block[i] = 0;
	}



	unsigned int free_block;
  	unsigned int *indirect;
	for (i=0;i<num_of_blocks;i++){
		if(i<12){
			free_block = find_free_block();
			new_inode->i_block[i] = free_block;
			add_bitmap(free_block,2);
			fread((struct ext2_dir_entry *)(disk + free_block*EXT2_BLOCK_SIZE),EXT2_BLOCK_SIZE, 1, fp);

		}else{
			if(i == 12){
				free_block = find_free_block();
				add_bitmap(free_block,2);
				new_inode->i_block[i] = free_block;
				indirect = (unsigned int*)(disk + free_block* EXT2_BLOCK_SIZE);
			}


			free_block = find_free_block();

			add_bitmap(free_block,2);
			memcpy(indirect+(i-12), &free_block, sizeof(int));
			fread((struct ext2_dir_entry *)(disk + free_block* EXT2_BLOCK_SIZE),EXT2_BLOCK_SIZE, 1, fp);

		}
	}
	add_bitmap(free_inode,1);


	add_entry(parent_inode,target_file_name,free_inode,EXT2_FT_REG_FILE);

	free(path);
	munmap(disk,128 * 1024);
	close(fd);
	return 0;
}
