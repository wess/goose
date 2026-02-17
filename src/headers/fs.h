#ifndef GOOSE_FS_H
#define GOOSE_FS_H

int  fs_mkdir(const char *path);
int  fs_exists(const char *path);
int  fs_rmrf(const char *path);
int  fs_write_file(const char *path, const char *content);
int  fs_collect_sources(const char *dir, char files[][512], int max, int *count);
int  fs_collect_ext(const char *dir, const char *ext, char files[][512], int max, int *count);

#endif
