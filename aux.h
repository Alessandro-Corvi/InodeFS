#ifndef AUX_H
#define AUX_H

#include "fs.h"

char* getBlockAt(int index);
int allocBlockAt(int index);

DirEntry* searchFreeDirEntry();
Inode* searchFreeInode();
int searchFreeBlock();

DirEntry* findEntry(const char *dirname);


void addDirEntry (const char* name, int id);
int removeDirEntry(int id);
int syncFS();

#endif 