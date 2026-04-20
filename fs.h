#ifndef FS_H
#define FS_H

#define MAX_LINE    2048
#define MAX_TOKENS  256
#define MAX_PATH 128

extern char current_path[MAX_PATH];

void format_fs(const char *filename, int size);

#endif
