#ifndef FS_H
#define FS_H

void fs_init();
int fs_read(const char* path, void* buffer);
int fs_write(const char* path, const void* buffer);

#endif
