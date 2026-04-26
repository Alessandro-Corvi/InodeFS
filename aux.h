#ifndef AUX_H
#define AUX_H

#include "fs.h"

char* getBlockAt(int index);
int allocBlockAt(int index);

DirEntry* searchFreeDirEntry();
Inode* searchFreeInode();
int searchFreeBlock();

int findEntry(const char *dirname);
int findFile(const char* filename);

int addDirEntry (const char* name, int id);
int removeDirEntry(const char* name, int id);
int syncFS();

#endif 