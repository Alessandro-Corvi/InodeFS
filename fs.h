#ifndef FS_H
#define FS_H

#define MAX_LINE    2048
#define MAX_TOKENS  256
#define MAX_PATH 128

#define NULL_PTR -1

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

extern char current_path[MAX_PATH];


typedef struct {
    int fs_size;
    int block_size;
    int num_inodes;
    int num_blocks;
    int free_inodes;
    int free_blocks;
}__attribute__((packed))Superblock;

typedef struct{
    char name[256];
    int id;
}__attribute__((packed)) DirEntry;


typedef struct{
    int size;
    int parent_id;
    int is_dir;
    int used; //1 se è occupato 0 se libero
    int direct[12];
    uint32_t indirect;
    uint32_t double_indirect;
}__attribute__((packed)) Inode;


typedef struct{
    Superblock *sb;
    char *data_bitmap;
    Inode *inode_table;
    char * data;
    Inode *current_dir;
}FileSystem;

extern FileSystem *fs;



int format_fs(const char *filename, int size);

#endif
