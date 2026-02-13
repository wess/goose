#ifndef GOOSE_CMAKE_H
#define GOOSE_CMAKE_H

#include "config.h"

int cmake_to_config(const char *cmake_path, Config *cfg);
int cmake_convert_file(const char *cmake_path, const char *yaml_path);

#endif
