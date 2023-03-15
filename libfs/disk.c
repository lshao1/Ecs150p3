#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"
#define BLOCK_SIZE 4096
#define NAME_SIZE 16
#define PADDING 10
#define FATSIZE 2
#define FAT_E0C 0xE00C
/* TODO: Phase 1 */
struct superblock {
	uint64_t Signature;
	uint16_t Block_Amounts;
	uint16_t Root_Dir;
	uint16_t Data_Start;
	uint16_t Data_Blocks_Amount;
	uint8_t Fat_Blocks;
}first_block;

struct root_nodes {
	char file_name[NAME_SIZE];
	uint32_t file_size;
	uint16_t index;
	char padding[10];
} root_dir[FS_FILE_MAX_COUNT];

uint16_t* fat_representation;
int fs_mount(const char *diskname)
{
	if(block_disk_open(diskname) == -1){
		// opening failed
		return -1;
	}
	// read into first block
	block_read(0,&first_block);
	// parse the signature
	char sig_parsed[8];
	for(int i = 0; i < 8; i++) {
		sig_parsed[i] = (first_block.Signature >> (i*8)) & 0xFF;
	}
	
	char check[] = "ECS150FS";
	for(int i = 0 ; i < 8 ; i++){
		if(sig_parsed[i] != check[i]){
			return -1;
		}
	}
	
	fat_representation = (uint16_t*) malloc(first_block.Data_Blocks_Amount * sizeof(uint16_t));
	// need to match the fats and put them into fat_representation
	int block_track = 1;
	int offset = 0;
	for(block_track; block_track < 1 + first_block.Fat_Blocks; block_track++){
		if(block_read(block_track,&fat_representation[offset]) == -1){
			return -1;
		}
		offset += first_block.Data_Blocks_Amount/first_block.Fat_Blocks;
	}
	block_read(block_track,root_dir);
	return 0;
	
}

int fs_umount(void)
{
	int offset = 0;
	/* TODO: Phase 1 */
	for(int i = 1 ; i < 1 + first_block.Fat_Blocks; i++){
		if(block_write(i,&fat_representation[offset]) == -1){
			return -1;
		}
		offset += first_block.Data_Blocks_Amount/first_block.Fat_Blocks;
	}
	free(fat_representation);
	// load in the root dir
	int root_location = 1 + first_block.Fat_Blocks;
	block_write(root_location,root_dir);
	block_disk_close();
}

int fs_info(void)
{
	printf("FS Info\n");
	// total blocks
	printf("total_blk_count = %d\n",first_block.Block_Amounts);
	// fat blocks
	printf("fat_blk_count = %d\n", first_block.Fat_Blocks);
	// which block is the rdir
	printf("rdir location = %d\n", first_block.Root_Dir);
	// where is data start
	printf("data_blk = %d\n",first_block.Data_Start);
	// how many data blocks there are
	printf("data_blk_count = %d\n",first_block.Block_Amounts - first_block.Fat_Blocks - 1 - 1);
	// how many are free(fat)
	int total_fat = BLOCK_SIZE/FATSIZE*first_block.Fat_Blocks;
	int free_fat = 0;
	for(int i = 0 ; i < total_fat;i++){
		if(*(fat_representation + i) == 0){
			free_fat += 1;
		}
	}
	printf("fat_free_ratio = %d", free_fat);
	printf("/%d\n",total_fat);
	// how many free rootdirs there are
	int root_dir_elements = FS_FILE_MAX_COUNT;
	int free_dir = 0;
	for(int i = 0; i < root_dir_elements;i++){
		if(root_dir[i].file_name[0] == '\0'){
			free_dir += 1;
		}
	}
	printf("rdir_free_ratio = %d",free_dir);
	printf("/%d\n",root_dir_elements);

}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */
	// first find an available root dir
	int root_dir_elements = FS_FILE_MAX_COUNT;
	for(int i = 0; i < root_dir_elements;i++){
		if(root_dir[i].file_name[0] == '\0'){
			struct root_nodes* this_root = &root_dir[i];
			strncpy(this_root->file_name,filename,sizeof(filename));
			this_root->file_size = 0;
			// need to find a fat block
			int total_fat = BLOCK_SIZE/FATSIZE*first_block.Fat_Blocks;
			for(int j = 0 ; j < total_fat;j++){
				if(*(fat_representation + j) == 0){
					// we found a free fat block that we can allocate
					this_root->index = j;
					// change the fat's value from 0 to FAT_E0C
					*(fat_representation + j) = FAT_E0C;
					return 0;
				}
			}	
		}
	}
	return -1;
}
/// @brief clear the directory, set the index to 0, set the name to empty, set size to 0
/// @param this_root 
void clear_directory (struct root_nodes* this_root){
	this_root -> index = 0;
	this_root -> file_size = 0;
	for(int i = 0 ; i < sizeof(this_root ->file_name); i++){
		(this_root -> file_name)[i] = '\000';
	}
	return;
}
// run through the fat and clear every item the fat is conencted to
void clear_fat(int head){
	int fat_location = head;
	while(1){
		uint16_t next_fat = *(fat_representation + fat_location);
		*(fat_representation + fat_location) = 0;
		if(next_fat == FAT_E0C){
			return;
		}
	}
}
int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
	// firsat find the file in the rootdir
	int root_dir_elements = FS_FILE_MAX_COUNT;
	for(int i = 0; i < root_dir_elements;i++){
		if(strncmp(root_dir[i].file_name,filename,sizeof(root_dir[i].file_name)) == 0){
			struct root_nodes* this_root = &root_dir[i];
			// first need to know fat index
			int fat_index = this_root -> index;
			// set the name to all \000
			clear_directory(this_root);
			// clear all of the linked listed fat and make them 0
			clear_fat(fat_index);
			return 0;
		}
	}
	return -1;
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

