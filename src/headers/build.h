#ifndef GOOSE_BUILD_H
#define GOOSE_BUILD_H

#include "config.h"

/* forward declaration */
typedef struct GooseFramework GooseFramework;

/* framework utilities for consumers */
int  build_transpile(const Config *cfg, const char *build_dir);
void build_include_flags(const Config *cfg, const char *pkg_dir,
                         const char *config_file, char *buf, int bufsz,
                         GooseFramework *fw);
int  build_collect_pkg_sources(const Config *cfg, const char *pkg_dir,
                               const char *config_file,
                               char files[][512], int max, int *count,
                               GooseFramework *fw);
void build_dep_base(const Dependency *dep, const char *pkg_dir,
                    char *buf, int bufsz);

#endif
