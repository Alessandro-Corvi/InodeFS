#ifndef AUX_H
#define AUX_H

#include "fs.h"

char* get_inode_block(Inode *inode,int index);
int alloc_inode_block(Inode *inode,int index);

DirEntry* search_free_entry();
Inode* search_free_inode();
int search_free_block();

DirEntry* findEntry(Inode *inode,const char *dirname);


void add_dir_entry (Inode *inode, const char* name, int id);
int remove_dir_entry(Inode *inode,int id);
int syncFS();

#endif 