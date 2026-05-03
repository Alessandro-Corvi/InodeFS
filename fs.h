#ifndef FS_H
#define FS_H


#include <fcntl.h>      // open
#include <unistd.h>     // ftruncate, close
#include <sys/mman.h>   // mmap, munmap
#include <sys/stat.h>   // mode constants (opzionale ma consigliata)
#include <stdio.h>    // printf
#include <string.h>   // memset
#include <stdlib.h>   // malloc
#include <unistd.h>   // ftruncate
#include <math.h>


#define NULL_PTR -1
#define NUM_PTRS 140
#define NUM_DIRECT 12
#define NUM_BLOCK_PTRS 128

#define ENTRIES_PER_BLOCK 16
#define MAX_ENTRIES ((NUM_DIRECT*ENTRIES_PER_BLOCK) + (NUM_BLOCK_PTRS*ENTRIES_PER_BLOCK))
#define MAX_SIZE (NUM_DIRECT*fs->sb->block_size) + (NUM_PTRS * fs->sb->block_size)


typedef struct{
    int fs_size;
    int block_size;
    int num_blocks;
    int num_inodes;
    int free_blocks;
    int free_inodes;
}__attribute__((packed))Superblock;


typedef struct{
    char name[28];
    int id;
}__attribute__((packed)) DirEntry;

typedef struct{
    int id;
    int size;
    int is_dir;
    int used;
    int num_entries;
    int direct[12];
    int indirect;
}__attribute__((packed))Inode;

typedef struct{
    Superblock* sb;
    char *data_bitmap;
    Inode *inodes;
    char *data;
    Inode *current_dir;
    char current_path[256];
}Filesystem;


extern Filesystem *fs;


int format_fs(const char* filename, int fs_size);
int load_fs(const char* filename);

int create_dir(const char* dirname);
void change_dir(const char* dirname);
void print_dir();
int remove_dir(const char* dirname);

int create_file(const char* filename);
int write_file(const char* filename, const char* data);
int read_file(const char* filename);

#endif 