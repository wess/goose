#ifndef GOOSE_BUILD_H
#define GOOSE_BUILD_H

#include "config.h"

int build_project(const Config *cfg, int release);
int build_clean(const Config *cfg);

#endif
