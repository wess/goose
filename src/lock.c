#include <stdio.h>
#include <string.h>
#include "headers/lock.h"
#include "headers/main.h"

int lock_load(const char *path, LockFile *lf) {
    memset(lf, 0, sizeof(LockFile));

    FILE *f = fopen(path, "r");
    if (!f)
        return -1;

    char line[1024];
    LockEntry *cur = NULL;

    while (fgets(line, sizeof(line), f)) {
        /* strip newline */
        line[strcspn(line, "\n")] = '\0';

        if (strncmp(line, "[[package]]", 11) == 0) {
            if (lf->count >= MAX_DEPS) break;
            cur = &lf->entries[lf->count++];
            memset(cur, 0, sizeof(LockEntry));
            continue;
        }

        if (!cur) continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;

        /* trim key */
        *eq = '\0';
        char *key = line;
        while (*key == ' ') key++;
        char *kend = eq - 1;
        while (kend > key && *kend == ' ') *kend-- = '\0';

        /* trim value (strip quotes) */
        char *val = eq + 1;
        while (*val == ' ' || *val == '"') val++;
        char *vend = val + strlen(val) - 1;
        while (vend > val && (*vend == '"' || *vend == ' ')) *vend-- = '\0';

        if (strcmp(key, "name") == 0)
            strncpy(cur->name, val, MAX_NAME_LEN - 1);
        else if (strcmp(key, "git") == 0)
            strncpy(cur->git, val, MAX_PATH_LEN - 1);
        else if (strcmp(key, "sha") == 0)
            strncpy(cur->sha, val, MAX_SHA_LEN - 1);
    }

    fclose(f);
    return 0;
}

int lock_save(const char *path, const LockFile *lf) {
    FILE *f = fopen(path, "w");
    if (!f) return -1;

    fprintf(f, "# goose.lock - auto-generated, do not edit\n\n");
    for (int i = 0; i < lf->count; i++) {
        fprintf(f, "[[package]]\n");
        fprintf(f, "name = \"%s\"\n", lf->entries[i].name);
        fprintf(f, "git = \"%s\"\n", lf->entries[i].git);
        fprintf(f, "sha = \"%s\"\n\n", lf->entries[i].sha);
    }

    fclose(f);
    return 0;
}

const char *lock_find_sha(const LockFile *lf, const char *name) {
    for (int i = 0; i < lf->count; i++) {
        if (strcmp(lf->entries[i].name, name) == 0)
            return lf->entries[i].sha;
    }
    return NULL;
}

int lock_update_entry(LockFile *lf, const char *name, const char *git, const char *sha) {
    for (int i = 0; i < lf->count; i++) {
        if (strcmp(lf->entries[i].name, name) == 0) {
            strncpy(lf->entries[i].sha, sha, MAX_SHA_LEN - 1);
            strncpy(lf->entries[i].git, git, MAX_PATH_LEN - 1);
            return 0;
        }
    }
    if (lf->count >= MAX_DEPS) return -1;

    LockEntry *e = &lf->entries[lf->count++];
    memset(e, 0, sizeof(LockEntry));
    strncpy(e->name, name, MAX_NAME_LEN - 1);
    strncpy(e->git, git, MAX_PATH_LEN - 1);
    strncpy(e->sha, sha, MAX_SHA_LEN - 1);
    return 0;
}
