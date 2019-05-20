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
#include "ext2.h"
#include "ext2_helper.h"

unsigned char *disk;
struct ext2_super_block *sb;

struct ext2_group_desc *gd;

void* inode_table;

void* bitmap_block;

unsigned int inode_bitmap_offset;


char *helper_path_handler(char* path) {
	/*
	Return the parsed file path.
	*/

	int i = 1;
	int j = 1;
	char *new_path = malloc(sizeof(char) * (strlen(path)+1));

	new_path[0] = path[0];
	while(i<strlen(path)) {
		if ((path[i] == path[i-1]) && (path[i] == '/')) {
			i++;
		} else {
			new_path[j] = path[i];
			j++;
			i++;
		}
	}
	if (new_path[j-1] == '/')
		new_path[j-1] = '\0';

	return new_path;
}

unsigned int find_free_inode(){
	/*
	Return an the first free inode.
	If there is no free inode left, return ENOSPC error.
	*/

	unsigned char* inode_bits = (unsigned char*)(disk + EXT2_BLOCK_SIZE*gd->bg_inode_bitmap);
	int byte;
	int bit;

	unsigned int index = 1;
	for (byte = 0; byte < 4; byte++) {
		for (bit = 0; bit < 8; bit++) {
        	int flag = (inode_bits[byte] & (1<<bit)) != 0;
            if (!flag)
				return index;
			index++;
        }
	}
	fprintf(stderr,"No more space. \n");
    exit(ENOSPC);
}

unsigned int find_free_block(){
	/*
	Return an the first free block.
	If there is no free block left, return ENOSPC error.
	*/

	int byte;
	int bit;
	unsigned int index = 1;
	unsigned char* block_bits = (unsigned char*)(disk + EXT2_BLOCK_SIZE*gd->bg_block_bitmap);
	for (byte = 0; byte < 16; byte++) {
        for (bit = 0; bit < 8; bit++) {
        	int flag = (block_bits[byte] & (1<<bit)) != 0;
            if (!flag){
				return index;
			}
			index++;
        }
    }
    fprintf(stderr,"No more space. \n");
    exit(ENOSPC);

}




int check_inode_clean(unsigned int inode_num){
	/*
	Check if the inode bitmap is 0 given an inode number.
	Return 1 if the given inode bitmap is 0.
	Return 0 if the given inode bitmap is 1.
	*/

	unsigned char* inode_bits = (unsigned char*)(disk + EXT2_BLOCK_SIZE*gd->bg_inode_bitmap);
	int byte;
	int bit;

	int index = 1;
	for (byte = 0; byte < 4; byte++) {
		for (bit = 0; bit < 8; bit++) {
        	if (inode_num == index){
        		int flag = (inode_bits[byte] & (1<<bit)) == 0;

        		return flag;
        	}
			index++;
        }
	}
	return 0;
}

int check_block_clean(unsigned int block_num){
	/*
	Check if the block bitmap is 0 given a block number.
	Return 1 if the given block bitmap is 0.
	Return 0 if the given block bitmap is 1.
	*/
	int byte;
	int bit;
	int index = 1;
	unsigned char* block_bits = (unsigned char*)(disk + EXT2_BLOCK_SIZE*gd->bg_block_bitmap);
	for (byte = 0; byte < 16; byte++) {
        for (bit = 0; bit < 8; bit++) {
        	if (block_num == index){
        		int flag = (block_bits[byte] & (1<<bit)) == 0;
        		return flag;

        	}
			index++;
        }
    }
    return 0;
}



int check_data_clean(unsigned int inode_num){
	/*
	Check if the data block has been reallocated given an inode number.
	Return 1 if no data block has been reallocated.
	Return 0 if any data block have been reallocated.
	*/

	//struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
	//void *inode_table = (void *)(disk + EXT2_BLOCK_SIZE * gd->bg_inode_bitmap);
	struct ext2_inode * target_inode = (struct ext2_inode *)(inode_table+ sb->s_inode_size*(inode_num-1));
	int i;
	for (i=0;i<13;i++){
		//indirect
		if (i == 12 && target_inode->i_block[i]){


			int j = 0;
			unsigned int *indirect = (unsigned int*)(disk + (target_inode->i_block[i])* EXT2_BLOCK_SIZE);

			while (indirect[j]){

				if (!check_block_clean(indirect[j]))
					return 0;
				j++;

			}
		}
		if (target_inode->i_block[i]){

			if (!check_block_clean(target_inode->i_block[i])){

				return 0;
			}

		}
	}
	return 1;
}




char *get_parent_path(char *path){
	/*
	Return the parent path.
	*/

  	int end=0;
  	char * result = malloc(sizeof(char) * (strlen(path) + 1) );
	if (result == NULL){
        exit(-1);
    }
  	if (strncmp(path,"/./",3) == 0){
      	strcpy(result,"/./");
      	return result;
  	}
  	strcpy(result,path);
  	for (end = strlen(result); end>=0; end--){
    	if (result[end]=='/') break;
	}

  	if (end == 0) return "/";
  	else {
    	result[end]='\0';
    	return result;
	}
}

char *get_file_name(char *path, char *parent_path){
	/*
	Return the target file name based on the path and parent path.
	*/

	char* result = malloc(sizeof(char)*strlen(path));

	int p_len = strlen(parent_path);
	if (path[p_len]!='/'){
		strncpy(result,path+p_len,strlen(path)-p_len);
	}
	else {

	    strncpy(result,path+p_len+1,strlen(path)-p_len);
	}
	return result;
}




int get_rec_len(int name_len){
	/*
	Return the actual rec_len based on the name_len.
	*/
	int rec_len = sizeof(struct ext2_dir_entry)+sizeof(char)*name_len; //padding
	if (rec_len % 4 == 0)
	  return rec_len;
	else
		return ((rec_len/4)+1)*4;
}



struct ext2_inode *find_inode_dir(char *path){
	/*
	* find and return the inode based on given path
	*/
	const char s[2] = "/";
	char *temp_path = malloc(sizeof(char)*strlen(path));
	temp_path = strncpy(temp_path,path,strlen(path));

	char *curr_dir= strtok(temp_path, s);

	//start from root inode

	unsigned int i;
	
	struct ext2_inode *dest_inode = (struct ext2_inode *)(inode_table + (EXT2_ROOT_INO-1)*sizeof(struct ext2_inode));


	int found = 1; 
	// loop over inode blocks to find dir_name directory entry
	while(curr_dir != NULL){
		found = 0;
		struct ext2_dir_entry *entry;
		unsigned int size = 0;
		for (i=0; i<12;i++){
			entry = (struct ext2_dir_entry *)(disk + dest_inode->i_block[i] * EXT2_BLOCK_SIZE);
			if (dest_inode->i_block[i] && (dest_inode->i_mode & EXT2_S_IFDIR)){
				while((size < EXT2_BLOCK_SIZE) && entry->inode){
					if((entry->file_type == EXT2_FT_DIR) && strncmp(entry->name, curr_dir, strlen(curr_dir))==0 && strlen(curr_dir) == entry->name_len){
						dest_inode = (struct ext2_inode *)(inode_table + (entry->inode-1)*sizeof(struct ext2_inode));
						curr_dir = strtok(NULL, s);
						found = 1;

						break;
					}
				//move to the next entry in the directory
				size += entry->rec_len;
				entry = (void *)entry + entry->rec_len;
				}
			}

			//stop when the i_block[i] is not found
			else
				break;

			//break the inner while loop
			if (found == 1) break;
		}
		//break the outer while loop
		if (found == 0) break;
	}

	if (found == 1){
		free(temp_path);
		return dest_inode;
	} else {
		free(temp_path);
		return NULL;
	}

}

int find_inode_num(char *path, char *file_name,int file_type){
	/*
	* find and return the inode based on given path
	*/

	
	const char s[2] = "/";
	char *temp_path = malloc(sizeof(char)*strlen(path));

	temp_path = strncpy(temp_path,path,strlen(path));
	char *curr_dir= strtok(temp_path, s);

	//start from root inode

	struct ext2_inode *dest_inode = (struct ext2_inode *)(inode_table + (EXT2_ROOT_INO-1)*sizeof(struct ext2_inode));
	unsigned int i;
	int result = EXT2_ROOT_INO-1;

	int found = 1;
	// loop over inode blocks to find dir_name directory entry

	while(curr_dir != NULL){

		found = 0;
		struct ext2_dir_entry *entry;

		unsigned int size = 0;

		for (i=0; i<12;i++){
			
			entry = (struct ext2_dir_entry *)(disk + dest_inode->i_block[i] * EXT2_BLOCK_SIZE);
			if (dest_inode->i_block[i] && (dest_inode->i_mode & EXT2_S_IFDIR)){
				
				while((size < EXT2_BLOCK_SIZE) && entry->inode){
					//find file type

					if((entry->file_type == file_type) &&strncmp(entry->name, file_name, strlen(file_name))==0 && strlen(file_name) == entry->name_len){
						curr_dir = strtok(NULL, s);
						//have to be the filename in the given directory
						if (curr_dir == NULL){
							dest_inode = (struct ext2_inode *)(inode_table + (entry->inode-1)*sizeof(struct ext2_inode));

							result = entry->inode;

							found = 1;
							free(temp_path);
							return result;
						}
					}
					else if((entry->file_type == EXT2_FT_DIR) &&strncmp(entry->name, curr_dir, strlen(curr_dir))==0 && strlen(curr_dir) == entry->name_len){

						dest_inode = (struct ext2_inode *)(inode_table + (entry->inode-1)*sizeof(struct ext2_inode));
						curr_dir = strtok(NULL, s);

						found = 1;
						break;
					}
				//move to the next entry in the directory
				size += entry->rec_len;
				entry = (void *)entry + entry->rec_len;
				}
			}
			//stop when the i_block[i] is not found
			else
				break;

			//break the inner while loop
			if (found == 1) break;
		}
		//break the outer while loop
		if (found == 0) break;
	}
	free(temp_path);
	return 0;
}







void add_bitmap(unsigned int index, int option){
	/*
	Add given inode/block number to corresponding bitmap.
	Option=1 indicates inode
	Option=2 indicates block
	*/
	
	unsigned char *bitmap;
	if (option == 1){
		bitmap = (unsigned char *)(disk + EXT2_BLOCK_SIZE * gd->bg_inode_bitmap);
		sb->s_free_inodes_count --;
		gd->bg_free_inodes_count --;
	}else{
		bitmap = (unsigned char *)(disk + EXT2_BLOCK_SIZE * gd->bg_block_bitmap);
		sb->s_free_blocks_count --;
		gd->bg_free_blocks_count --;
	}


	int byte_i = (index - 1) / 8;
	int bit_i = (index - 1) % 8;
	bitmap[byte_i] |= (1 << bit_i);

}





void remove_bitmap(unsigned int index,int option){
	/*

	Remove given in
		bitmap = ode/block number from the corresponding bitmap.
	Option=1 indicates inode
	Option=2 indicates block
	*/
	
	unsigned char *bitmap;
	if (option == 1){
		bitmap = (unsigned char *)(disk + EXT2_BLOCK_SIZE * gd->bg_inode_bitmap);
		sb->s_free_inodes_count ++;
		gd->bg_free_inodes_count ++;
	}else{
		bitmap = (unsigned char *)(disk + EXT2_BLOCK_SIZE * gd->bg_block_bitmap);
		sb->s_free_blocks_count ++;
		gd->bg_free_blocks_count ++;
	}

	int byte_i = (index - 1) / 8;
	int bit_i = (index - 1) % 8;
	bitmap[byte_i] &= ~(1 << bit_i);

}



void add_entry(struct ext2_inode *parent_inode,char *target_file_name, int free_inode,unsigned char file_type) {
	/*
	For the mkdir and cp.
	Add new entry into the corresponding directory block.
	If there is no enough space in the current directory block, allocate a new block.
	*/

	int found = 0;
	int file_name_len = get_rec_len(strlen(target_file_name));
	int parent_len;
	struct ext2_dir_entry *parent_entry;
	unsigned int i;

	for (i=0; i<12;i++){

		if (parent_inode->i_block[i]){

			unsigned int size = 0;
			parent_entry = (struct ext2_dir_entry *)(disk + parent_inode->i_block[i] * EXT2_BLOCK_SIZE);


			while((size < EXT2_BLOCK_SIZE) && parent_entry->inode){

				size += parent_entry->rec_len;
				parent_len = get_rec_len(parent_entry->name_len);
				if(size == EXT2_BLOCK_SIZE && (parent_entry->rec_len >= file_name_len + parent_len)){
					found = 1;
					break;
				}

				parent_entry = (void *)parent_entry + parent_entry->rec_len;
			}
		}
		else
			break;
		if (found == 1) break;
	}

	if (found){
		//there is enougth space
		int temp = parent_entry->rec_len;
		parent_entry->rec_len = parent_len;
		struct ext2_dir_entry *new_parent_entry = (void *)parent_entry + parent_len;
		new_parent_entry->file_type = file_type;
		new_parent_entry->inode =free_inode;
		new_parent_entry->name_len = strlen(target_file_name);
		strcpy(new_parent_entry->name,target_file_name);
		new_parent_entry->rec_len = temp - parent_len;

	}
	else{

		int new_free_block = find_free_block();
		add_bitmap(new_free_block,2);

		parent_inode->i_size += EXT2_BLOCK_SIZE;
		parent_inode->i_blocks += 2;
		parent_inode->i_block[i] =new_free_block;

		struct ext2_dir_entry *new_parent_entry = (struct ext2_dir_entry*)(disk + EXT2_BLOCK_SIZE*new_free_block);
		new_parent_entry->file_type = file_type;
		new_parent_entry->inode = free_inode;

		new_parent_entry->name_len = strlen(target_file_name);
		new_parent_entry->rec_len = EXT2_BLOCK_SIZE;
		strncpy(new_parent_entry->name,target_file_name,new_parent_entry->name_len);


	}


}



void add_entry_ln(struct ext2_inode *parent_inode,struct ext2_inode *src, char *dest_file_name, char* src_path,unsigned int src_inode_num,int symbolic){
	/*
	For ext2_ln.
	Add new entry into the corresponding directory block.
	If there is no enough space in the current directory block, allocate a new block.
	*/
	int found = 0;
	int dir_name_len = get_rec_len(strlen(dest_file_name));
	int parent_len;
	struct ext2_dir_entry *parent_entry;
	unsigned int i;

	for (i=0; i<12;i++){

		if (parent_inode->i_block[i]){

			unsigned int size = 0;
			parent_entry = (struct ext2_dir_entry *)(disk + parent_inode->i_block[i] * EXT2_BLOCK_SIZE);

			while((size < EXT2_BLOCK_SIZE) && parent_entry->inode){

				size += parent_entry->rec_len;
				parent_len = get_rec_len(parent_entry->name_len);
				if(size == EXT2_BLOCK_SIZE && (parent_entry->rec_len >= dir_name_len + parent_len)){
					found = 1;
					break;
				}

				parent_entry = (void *)parent_entry + parent_entry->rec_len;

			}
		}
		else
			break;
		if (found == 1) break;
	}

	int free_inode;
	struct ext2_dir_entry *new_parent_entry;

	if (found) {
		//there is enougth space
		int temp = parent_entry->rec_len;
		parent_entry->rec_len = parent_len;
		new_parent_entry = (void *)parent_entry + parent_len;
		new_parent_entry->name_len = strlen(dest_file_name);
		strcpy(new_parent_entry->name, dest_file_name);
		new_parent_entry->rec_len = temp - parent_len;


	} else {
		// there is no enough space
		int new_free_block = find_free_block();
		add_bitmap(new_free_block,2);


		new_parent_entry = (struct ext2_dir_entry*)(disk + EXT2_BLOCK_SIZE*new_free_block);
		new_parent_entry->name_len = strlen(dest_file_name);
		parent_len = get_rec_len(new_parent_entry->name_len);
		new_parent_entry->rec_len = EXT2_BLOCK_SIZE;
		strncpy(new_parent_entry->name, dest_file_name,strlen(dest_file_name));

		parent_inode->i_size += EXT2_BLOCK_SIZE;
		parent_inode->i_blocks += 2;
		parent_inode->i_block[i] =new_free_block;

	}

	if (symbolic) {
		//if the target is a symbloic link
		free_inode = find_free_inode();
		add_bitmap(free_inode, 1);

		new_parent_entry->inode = free_inode;
		new_parent_entry->file_type = EXT2_FT_SYMLINK;
		struct ext2_inode *new_inode;
		new_inode = (struct ext2_inode *)(inode_table + (new_parent_entry->inode-1) * sb->s_inode_size);

		new_inode->i_links_count = 1;
		new_inode->i_mode = EXT2_S_IFLNK;
		new_inode->i_size = strlen(src_path);
		new_inode->i_blocks = src->i_blocks;
		new_inode->i_dtime = 0;

		int free_block;
		struct ext2_dir_entry *data;
		for(i=0; i<15; i++){
			new_inode->i_block[i] = 0;
		}

		i=0;
		unsigned int *indirect;
		for (i=0;i<(src->i_blocks/2);i++){
			if(i<12){
				free_block = find_free_block();
				new_inode->i_block[i] = free_block;
				add_bitmap(free_block,2);
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
			}
		}
		data= (struct ext2_dir_entry *)(disk + free_block* EXT2_BLOCK_SIZE);
		memcpy(data, src_path, strlen(src_path));

	} else {
		//if the target is hard link
		new_parent_entry->inode = src_inode_num;
		new_parent_entry->file_type = EXT2_FT_REG_FILE;
		src->i_links_count ++;
	}
}






void add_entry_restore(struct ext2_inode *parent_inode,char *file_name){
	/*
	For ext2_restore.
	Add new entry into the corresponding directory block.
	If there is no enough space in the current directory block, allocate a new block.
	*/
	int found = 0;
	int entry_rec_len = 0;
	struct ext2_dir_entry *parent_entry;
	struct ext2_dir_entry *target_entry;
	int i;
	struct ext2_inode * target_inode;
	int ptr_size = 0;


	for (i=0; i<12;i++){

		if (parent_inode->i_block[i]){

			unsigned int size = 0;
			target_entry = (struct ext2_dir_entry *)(disk + parent_inode->i_block[i] * EXT2_BLOCK_SIZE);

			while((size < EXT2_BLOCK_SIZE)){
				//may delete a few files located at the end of the block
				//record the rec_length of parent
				if (ptr_size != 0){
					ptr_size += entry_rec_len;
				}
				if (entry_rec_len != target_entry->rec_len){
					parent_entry = (void *)target_entry;
					ptr_size = get_rec_len(target_entry->name_len);
				}
				target_entry = (void *)target_entry + entry_rec_len;
				entry_rec_len = get_rec_len(target_entry->name_len);


				if ((!strncmp(file_name,target_entry->name,target_entry->name_len)) && (strlen(file_name) == target_entry->name_len)){
					found = 1;
					target_inode = (struct ext2_inode *)(inode_table+ sizeof(struct ext2_inode)*(target_entry->inode-1));
					break;
				}

				size += entry_rec_len;

			}
		}
		else
			break;
		if (found == 1) break;

	}

	if (found){

		if (target_entry->inode == 0){
			fprintf(stderr, "The file does not exist\n");
			exit(ENOENT);
		}

		//check if the inode has been reused
		if (!check_inode_clean(target_entry->inode)){
			fprintf(stderr, "The inode has already been reused\n");
			exit(ENOENT);
		}

		//check if the data blocks have been reallocated
		if (!check_data_clean(target_entry->inode)){
			fprintf(stderr, "The data blocks have already been reallocated\n");
			exit(ENOENT);
		}

		//restore data blocks
		for (i=0;i<13;i++){
			//indirect
			if (i == 12 && target_inode->i_block[i]){

				int j = 0;
				unsigned int *indirect = (unsigned int*)(disk + (target_inode->i_block[i])* EXT2_BLOCK_SIZE);

				while (indirect[j]){
					add_bitmap(indirect[j],2);
					j++;
				}
			}
			if (target_inode->i_block[i]){
				add_bitmap(target_inode->i_block[i],2);
			}
		}

		//reset parent_entry rec_len
		parent_entry->rec_len = ptr_size;
		//reset inode d_time
		target_inode->i_dtime = 0;
		target_inode->i_links_count = 1;

		//restore the inode
		add_bitmap(target_entry->inode,1);


	}
	//if no such file
	else{
		fprintf(stderr,"No such target file\n");
		exit(ENOENT);
	}

}



void add_entry_rm(struct ext2_inode *parent_inode,struct ext2_inode *target_inode,char *file_name){
	/*
	For ext2_rm.
	Find the target file and remove
	*/
	int found = 0;

	struct ext2_dir_entry *parent_entry;
	struct ext2_dir_entry *current_entry;
	int i;


	for (i=0; i<12;i++){

		if (parent_inode->i_block[i]){

			unsigned int size = 0;
			parent_entry = (struct ext2_dir_entry *)(disk + parent_inode->i_block[i] * EXT2_BLOCK_SIZE);
			if ((!strncmp(file_name,parent_entry->name, parent_entry->name_len)) && (strlen(file_name) == parent_entry->name_len)){
				remove_bitmap(parent_entry->inode, 1);
				parent_entry->inode = 0;

				found = 0;
				break;
			}


			current_entry = (void *)parent_entry + parent_entry->rec_len;

			while((size < EXT2_BLOCK_SIZE) && parent_entry->inode && current_entry->inode){

				size += parent_entry->rec_len;
				//should check name instead
				if ((!strncmp(file_name,current_entry->name,current_entry->name_len)) && (strlen(file_name) == current_entry->name_len)){
					found = 1;
					parent_entry->rec_len += current_entry->rec_len;
					break;
				}

				parent_entry = (void *)parent_entry + parent_entry->rec_len;
				current_entry = (void *)parent_entry + parent_entry->rec_len;

			}
		}

		if (found == 1) break;

	}


	if(found == 1){

		target_inode->i_links_count --;
		time_t seconds;
		seconds = time(NULL);

		target_inode->i_dtime = seconds;
		if (target_inode->i_links_count == 0){
			remove_bitmap(current_entry->inode,1);

			for (i=0;i<13;i++){
				//indirect
				if (i == 12 && target_inode->i_block[i]){

					int j = 0;
					unsigned int *indirect = (unsigned int*)(disk + (target_inode->i_block[i])* EXT2_BLOCK_SIZE);

					while (indirect[j]){
						remove_bitmap(indirect[j],2);

						j++;
					}
				}
				if (target_inode->i_block[i]){
					remove_bitmap(target_inode->i_block[i],2);

				}
			}
		}

		else{
			//do nothing
		}

	}

}



//helpers for ext2_checker
int check_counter() {
	struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
	struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE*2);
	unsigned char *block_map = (unsigned char *)(disk + gd->bg_block_bitmap * EXT2_BLOCK_SIZE);
	unsigned char *inode_map = (unsigned char *)(disk + gd->bg_inode_bitmap * EXT2_BLOCK_SIZE);

	int error = 0;
	int byte;
	int bit;
	int block_free = 0;
	int inode_free = 0;

	for (byte = 0; byte < (sb->s_blocks_count / 8); byte++) {
		for (bit = 0; bit < 8; bit++) {
			if ((block_map[byte] & (1<<bit)) == 0)
				block_free++;
		}
	}

	if (block_free != sb->s_free_blocks_count) {
		unsigned int diff = abs(sb->s_free_blocks_count - block_free);
		sb->s_free_blocks_count = block_free;
		error += diff;
		printf("Fixed: superblock's free blocks counter was off by %d compared to the bitmap\n", diff);

	}

	if (block_free != gd->bg_free_blocks_count) {
		unsigned int diff = abs(gd->bg_free_blocks_count - block_free);
		gd->bg_free_blocks_count = block_free;
		error += diff;
		printf("Fixed: block group's free blocks counter was off by %d compared to the bitmap\n", diff);

	}

	for (byte = 0; byte < (sb->s_inodes_count / 8); byte++) {
		for (bit = 0; bit < 8; bit++) {
			if ((inode_map[byte] & (1<<bit)) == 0)
				inode_free++;
		}
	}

	if (inode_free != sb->s_free_inodes_count) {
		unsigned int diff = abs(sb->s_free_inodes_count - inode_free);
		sb->s_free_inodes_count = inode_free;
		error += diff;
		printf("Fixed: superblock's free inodes counter was off by %d compared to the bitmap\n", diff);

	}

	if (inode_free != gd->bg_free_inodes_count) {
		unsigned int diff = abs(gd->bg_free_inodes_count - inode_free);
		gd->bg_free_inodes_count = inode_free;
		error += diff;
		printf("Fixed: block group's free inodes counter was off by %d compared to the bitmap\n", diff);

	}

	return error;
}


int check_type(struct ext2_inode *root) {
	unsigned int curr_size = 0;
	int error = 0;
	struct ext2_dir_entry *entry;
	struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE*2);
	void *inode_table = disk + EXT2_BLOCK_SIZE* gd->bg_inode_table;

	int i;
	for (i=0; i<12 && root->i_block[i]!=0; i++) {
		while(curr_size<EXT2_BLOCK_SIZE) {
			entry = (struct ext2_dir_entry *)(disk + (root->i_block[i])*EXT2_BLOCK_SIZE + curr_size);
			struct ext2_inode *inode = (struct ext2_inode *)(inode_table + (entry->inode-1)*sizeof(struct ext2_inode));
			if (inode->i_mode & EXT2_S_IFDIR) {
				if (entry->file_type != EXT2_FT_DIR) {
					entry->file_type = EXT2_FT_DIR;
					error++;
					printf("Fixed: Entry type vs inode mismatch: inode [%d]\n", entry->inode);
				}	


				// If the directory is not '.' or '..', need to do the recursion to check subdirectories
				if (strncmp(entry->name, ".", 1) != 0 && strncmp(entry->name, "..", 2) != 0)
					error += check_type(inode);
			}
			else if (inode->i_mode & EXT2_S_IFREG) {
				if (entry->file_type != EXT2_FT_REG_FILE) {
					entry->file_type = EXT2_FT_REG_FILE;
					error++;
					printf("Fixed: Entry type vs inode mismatch: inode [%d]\n", entry->inode);
				}	

			}

			else if (inode->i_mode & EXT2_S_IFLNK) {

				if (entry->file_type != EXT2_FT_SYMLINK) {
					entry->file_type = EXT2_FT_SYMLINK;
					error++;
					printf("Fixed: Entry type vs inode mismatch: inode [%d]\n", entry->inode);

				}
			}
			else
				fprintf(stderr, "Error: file type\n");
			curr_size += entry->rec_len;
		}
	}
	return error;
}

int check_inode_bitmap() {
	int byte;
	int bit;
	int index = 1;
	int error = 0;
	struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE*2);
    unsigned char* inode_map = (unsigned char*)(disk + EXT2_BLOCK_SIZE*gd->bg_inode_bitmap);

    for (byte = 0; byte < (sb->s_inodes_count/8); byte++) {
    	for (bit=0; bit<8; bit++) {
    		struct ext2_inode *inode = (struct ext2_inode *)(disk + gd->bg_inode_table*EXT2_BLOCK_SIZE + (index-1)*sb->s_inode_size);
    		if ((inode->i_mode & EXT2_S_IFDIR || inode->i_mode & EXT2_S_IFREG || inode->i_mode & EXT2_S_IFLNK) && (inode->i_size != 0)) {
        		//unsigned char *inode_byte = (unsigned char *)(disk + gd->bg_inode_bitmap * EXT2_BLOCK_SIZE + byte/8);
        		if ((inode_map[byte] & (1<<bit)) == 0) {
        			add_bitmap(index, 1);
        			printf("Fixed: inode [%d] not marked as in-use\n", index);

        			error ++;
        		}
        	}
        	index++;
    	}

    }

    return error;

}

int check_dtime() {
	int byte;
	int bit;
	int index = 1;
	int error = 0;
	struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE*2);

    for (byte = 0; byte < (sb->s_inodes_count/8); byte++) {
    	for (bit=0; bit<8; bit++) {
    		struct ext2_inode *inode = (struct ext2_inode *)(disk + gd->bg_inode_table*EXT2_BLOCK_SIZE + (index-1)*sb->s_inode_size);
    		if ((inode->i_mode & EXT2_S_IFDIR || inode->i_mode & EXT2_S_IFREG || inode->i_mode & EXT2_S_IFLNK) && (inode->i_size != 0)) {
        		if (inode->i_dtime != 0) {
        			inode->i_dtime = 0;
        			printf("Fixed: valid inode marked for deletion: [%d]\n", index);

        			error ++;
        		}
        	}
        	index++;
    	}
    }
    return error;
}

int fix_block_clean(unsigned int block_num){
	// int free_block = -1;
	int byte;
	int bit;
	int index = 1;
	unsigned char* block_bits = (unsigned char*)(disk + EXT2_BLOCK_SIZE*gd->bg_block_bitmap);
	for (byte = 0; byte < 16; byte++) {
        for (bit = 0; bit < 8; bit++) {
        	if (block_num == index){
        		if ((block_bits[byte] & (1<<bit)) == 0) {
        			add_bitmap(index, 2);

        			return 1;
        		}
        	}
			index++;
        }
    }
    return 0;
}

int check_block_bitmap() {
	int byte;
	int i;
	int error = 0;
	struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE*2);

	for (byte=0; byte<sb->s_inodes_count; byte++) {
		struct ext2_inode *inode = (struct ext2_inode *) (disk +gd->bg_inode_table*EXT2_BLOCK_SIZE + byte*sb->s_inode_size);
		int temp = 0;
		// direct
		for (i=0; i<12 && inode->i_block[i]!=0; i++) {
			temp += fix_block_clean(inode->i_block[i]);
		}
		if (temp) {
			printf("Fixed: %d in-use data blocks not marked in data bitmap for inode: [%d]\n", temp, byte+1);

		}
		error += temp;
	}
    return error;
}

void remove_file(int inode_num, char * parent_path, char *file_name) {
	struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE*2);
	void* inode_table = disk + EXT2_BLOCK_SIZE* gd->bg_inode_table;

	struct ext2_inode *target_inode;

	target_inode = (struct ext2_inode *)(inode_table + (inode_num-1)*sizeof(struct ext2_inode));

	struct ext2_inode *parent_inode = find_inode_dir(parent_path);

	add_entry_rm(parent_inode,target_inode,file_name);
}

void remove_root_dir(int root_inode_num, char *path, char *target_file_name) {
	struct ext2_inode *root = (struct ext2_inode *)(inode_table + (root_inode_num-1)*sizeof(struct ext2_inode));
	unsigned int curr_size = 0;
	struct ext2_dir_entry *entry;
	struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE*2);


	int dot_inode = 0;
	int double_dot_inode = 0;

	int i;
	//printf("root inode num: [%d]\n", root_inode_num);
	for (i=0; i<12 && root->i_block[i]!=0; i++) {
		while(curr_size<EXT2_BLOCK_SIZE) {
			entry = (struct ext2_dir_entry *)(disk + (root->i_block[i])*EXT2_BLOCK_SIZE + curr_size);
			if (strncmp(entry->name, ".", 1) == 0)
				dot_inode = entry->inode;
			else if (strncmp(entry->name, "..", 2) == 0)
				double_dot_inode = entry->inode;

			if (strcmp(entry->name,target_file_name) == 0) {
				char *new_path = malloc(256);
				strcpy(new_path, path);
				strcat(new_path, "/");
				strcat(new_path, entry->name);
				remove_dir(entry->inode, new_path);
				remove_file(double_dot_inode, new_path, "..");
				root->i_links_count++;
				gd->bg_used_dirs_count --;
				remove_file(dot_inode, new_path, ".");
				remove_file(dot_inode, path, entry->name);
				root->i_dtime = 0;
			}
			
			curr_size += entry->rec_len;
		}
	}
}

void remove_dir(int root_inode_num, char * path) {

	char * parent_path = get_parent_path(path);
	char *file_name = get_file_name(path,parent_path);
	struct ext2_inode *root = (struct ext2_inode *)(inode_table + (root_inode_num-1)*sizeof(struct ext2_inode));
	unsigned int curr_size = 0;
	struct ext2_dir_entry *entry;
	struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE*2);
	void *inode_table = disk + EXT2_BLOCK_SIZE* gd->bg_inode_table;

	int dot_inode = 0;
	int double_dot_inode = 0;

	int i;
	for (i=0; i<12 && root->i_block[i]!=0; i++) {
		while(curr_size<EXT2_BLOCK_SIZE) {
			entry = (struct ext2_dir_entry *)(disk + (root->i_block[i])*EXT2_BLOCK_SIZE + curr_size);
			struct ext2_inode *inode = (struct ext2_inode *)(inode_table + (entry->inode-1)*sizeof(struct ext2_inode));
			char *curr_path = malloc(256);
			strcpy(curr_path, path);

			if (inode->i_mode & EXT2_S_IFREG)
				remove_file(entry->inode, curr_path, entry->name);

			else if (inode->i_mode & EXT2_S_IFLNK)
				remove_file(entry->inode, parent_path, file_name);

			else if (inode->i_mode & EXT2_S_IFDIR) {
				if (strncmp(entry->name, ".", 1) == 0)
					dot_inode = entry->inode;
				else if (strncmp(entry->name, "..", 2) == 0)
					double_dot_inode = entry->inode;
				// If the directory is not '.' or '..', need to do the recursion to check subdirectories
				if (strncmp(entry->name, ".", 1) != 0 && strncmp(entry->name, "..", 2) != 0) {
					char *new_path = malloc(256);
					strcpy(new_path, path);
					strcat(new_path, "/");
					strcat(new_path, entry->name);
					remove_dir(entry->inode, new_path);
					inode->i_links_count++;
					gd->bg_used_dirs_count --;
					// remove itself
					remove_file(double_dot_inode, new_path, "..");
					remove_file(dot_inode, new_path, ".");
					remove_file(dot_inode, path, entry->name);
					inode->i_dtime = 0;
				}
			}

			curr_size += entry->rec_len;
		}
	}
	remove_bitmap(root_inode_num,2);

}

