#ifndef AUX_H
#define AUX_H

#include "fs.h"

int bitmap_alloc();
void bitmap_free(int block_index);

char* get_inode_block(Inode *inode, int index,bool clear_ref);
int alloc_inode_block(Inode *inode, int index);

Inode* search_free_inode();
DirEntry* find_entry(Inode *inode,const char* name);

void add_dir_entry (Inode *inode, const char* name, int id);
int remove_dir_entry(Inode *inode, int id);
int syncFS();

#endif 