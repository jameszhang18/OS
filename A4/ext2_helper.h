#ifndef HELPER_LK
#define HELPER_LK


extern unsigned char *disk;

char *helper_path_handler(char* path);
unsigned int find_free_inode();
unsigned int find_free_block();
int check_inode_clean(unsigned int inode_num);
int check_block_clean(unsigned int block_num);
int check_data_clean(unsigned int inode_num);
char *get_parent_path(char *path);
char *get_file_name(char *path, char *parent_path);
int get_rec_len(int name_len);
struct ext2_inode *find_inode_dir(char *path);
int find_inode_num(char *path, char *file_name,int file_type);
void add_bitmap(unsigned int index, int option);
// void add_bitmap_inode(unsigned int index);
// void add_bitmap_block(unsigned int index);
void remove_bitmap(unsigned int index, int option);
// void remove_bitmap_inode(unsigned int index);
// void remove_bitmap_block(unsigned int index);
void add_entry(struct ext2_inode *parent_inode,char *target_file_name,int free_inode,unsigned char file_type);
void add_entry_ln(struct ext2_inode *parent_inode,struct ext2_inode *src,char *dest_file_name, char * src_path,unsigned int src_inode_num,int symbolic);
void add_entry_restore(struct ext2_inode *parent_inode,char *file_name);
void add_entry_rm(struct ext2_inode *parent_inode,struct ext2_inode *target_inode,char * file_name);
int check_counter();
int check_type(struct ext2_inode *root);
int check_inode_bitmap();
int check_dtime();
int check_block_bitmap();
void remove_file(int inode_num, char * parent_path, char *file_name);
void remove_dir(int root_inode_num, char *path);
void remove_root_dir(int root_inode_num, char *path, char *target_file_name);
#endif
