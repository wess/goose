#ifndef GOOSE_LOCK_H
#define GOOSE_LOCK_H

#include "config.h"

#define MAX_SHA_LEN 64

typedef struct {
    char name[MAX_NAME_LEN];
    char git[MAX_PATH_LEN];
    char sha[MAX_SHA_LEN];
} LockEntry;

typedef struct {
    LockEntry entries[MAX_DEPS];
    int count;
} LockFile;

int  lock_load(const char *path, LockFile *lf);
int  lock_save(const char *path, const LockFile *lf);
const char *lock_find_sha(const LockFile *lf, const char *name);
int  lock_update_entry(LockFile *lf, const char *name, const char *git, const char *sha);

#endif
