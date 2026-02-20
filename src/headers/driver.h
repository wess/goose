#ifndef GOOSE_DRIVER_H
#define GOOSE_DRIVER_H

#include "config.h"

typedef struct {
    const char *name;
    const char *ext;
    const char *description;

    int (*build)(const Config *cfg, int release, void *ctx);

    int (*run)(const Config *cfg, int release, int argc, char **argv, void *ctx);
    int (*test)(const Config *cfg, int release, void *ctx);
    int (*install)(const Config *cfg, const char *prefix, void *ctx);
    int (*init)(const Config *cfg, const char *dir, void *ctx);
    void (*defaults)(Config *cfg, void *ctx);
    int (*transpile)(const Config *cfg, void *ctx);
    int (*convert)(int argc, char **argv, void *ctx);

    void *ctx;
} GooseDriver;

int goose_run(int argc, char **argv, const GooseDriver *driver);

#endif
