#include <string.h>
#include "driver.h"
#include "../../headers/config.h"

int c_build(const Config *cfg, int release, void *ctx);
int c_test(const Config *cfg, int release, void *ctx);
int c_init(const Config *cfg, const char *dir, void *ctx);
int c_transpile(const Config *cfg, void *ctx);
int c_convert(int argc, char **argv, void *ctx);

static void c_defaults(Config *cfg, void *ctx) {
    (void)ctx;
    strncpy(cfg->cc, "cc", 63);
    strncpy(cfg->cflags, "-Wall -Wextra -std=c11", 255);
}

GooseDriver goose_c_driver(void) {
    GooseDriver drv;
    memset(&drv, 0, sizeof(drv));
    drv.name        = "c";
    drv.ext         = ".c";
    drv.description = "A package manager for C";
    drv.build       = c_build;
    drv.test        = c_test;
    drv.init        = c_init;
    drv.transpile   = c_transpile;
    drv.convert     = c_convert;
    drv.defaults    = c_defaults;
    return drv;
}
