#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "headers/fs.h"

int fs_mkdir(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0)
        return 0;
    return mkdir(path, 0755);
}

int fs_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

int fs_rmrf(const char *path) {
    if (!fs_exists(path))
        return 0;

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", path);
    return system(cmd);
}

int fs_write_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (!f) {
        perror(path);
        return -1;
    }
    fputs(content, f);
    fclose(f);
    return 0;
}

static int collect_recursive_ext(const char *dir, const char *ext,
                                 char files[][512], int max, int *count) {
    DIR *d = opendir(dir);
    if (!d) return -1;

    size_t ext_len = strlen(ext);
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL && *count < max) {
        if (ent->d_name[0] == '.') continue;

        char full[512];
        snprintf(full, sizeof(full), "%s/%s", dir, ent->d_name);

        struct stat st;
        if (stat(full, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            collect_recursive_ext(full, ext, files, max, count);
        } else {
            size_t len = strlen(ent->d_name);
            if (len > ext_len &&
                strcmp(ent->d_name + len - ext_len, ext) == 0) {
                strncpy(files[*count], full, 511);
                files[*count][511] = '\0';
                (*count)++;
            }
        }
    }
    closedir(d);
    return 0;
}

int fs_collect_sources(const char *dir, char files[][512], int max, int *count) {
    *count = 0;
    return collect_recursive_ext(dir, ".c", files, max, count);
}

int fs_collect_ext(const char *dir, const char *ext,
                   char files[][512], int max, int *count) {
    *count = 0;
    return collect_recursive_ext(dir, ext, files, max, count);
}
