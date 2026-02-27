#ifndef GOOSE_PKG_H
#define GOOSE_PKG_H

#include "config.h"
#include "lock.h"

/* forward declaration */
typedef struct GooseFramework GooseFramework;

int   pkg_fetch(const Dependency *dep, const char *pkg_dir, LockFile *lf,
                GooseFramework *fw);
int   pkg_remove(const char *name, const char *pkg_dir);
int   pkg_fetch_all(const Config *cfg, LockFile *lf, GooseFramework *fw);
int   pkg_update_all(const Config *cfg, LockFile *lf, GooseFramework *fw);
char *pkg_name_from_git(const char *git_url);
int   pkg_get_sha(const char *pkg_path, char *sha, int sha_size);

#endif
