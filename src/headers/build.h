#ifndef GOOSE_BUILD_H
#define GOOSE_BUILD_H

#include "config.h"

int build_project(const Config *cfg, int release);
int build_project_at(const Config *cfg, int release, const char *gen_dir);
int build_library(const Config *cfg, int release, const char *gen_dir);
int build_transpile(const Config *cfg);
int build_clean(void);

#endif
