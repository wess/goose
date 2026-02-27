#include <string.h>
#include "setup.h"
#include "config.h"
#include "../headers/main.h"

/* external C callback declarations */
extern int  c_build(const Config *, int, const char *, const char *,
                    const char *, void *);
extern int  c_test(const Config *, int, const char *, const char *,
                   const char *, const char *, void *);
extern int  c_clean(const char *, void *);
extern int  c_install(const Config *, const char *, const char *,
                      const char *, const char *, void *);
extern int  c_run(const Config *, int, const char *, const char *,
                  const char *, int, char **, void *);
extern int  c_transpile(const Config *, const char *, void *);
extern int  c_init_template(const char *, const char *, void *);
extern int  c_pkg_convert(const char *, const char *, void *);
extern void c_config_defaults(Config *, void *, void *);
extern int  c_config_parse(const char *, const char *, const char *,
                           void *, void *);
extern int  c_config_write(FILE *, const void *, void *);

void goose_c_setup(GooseFramework *fw) {
    strncpy(fw->tool_name, "goose", sizeof(fw->tool_name) - 1);
    strncpy(fw->tool_version, GOOSE_VERSION, sizeof(fw->tool_version) - 1);
    strncpy(fw->tool_description, "A package manager for C",
            sizeof(fw->tool_description) - 1);
    strncpy(fw->config_file, "goose.yaml", sizeof(fw->config_file) - 1);
    strncpy(fw->lock_file, "goose.lock", sizeof(fw->lock_file) - 1);
    strncpy(fw->pkg_dir, "packages", sizeof(fw->pkg_dir) - 1);
    strncpy(fw->build_dir, "build", sizeof(fw->build_dir) - 1);
    strncpy(fw->src_dir, "src", sizeof(fw->src_dir) - 1);
    strncpy(fw->test_dir, "tests", sizeof(fw->test_dir) - 1);
    strncpy(fw->init_filename, "main.c", sizeof(fw->init_filename) - 1);
    fw->gitignore_extra[0] = '\0';

    fw->on_build           = c_build;
    fw->on_test            = c_test;
    fw->on_clean           = c_clean;
    fw->on_install         = c_install;
    fw->on_run             = c_run;
    fw->on_transpile       = c_transpile;
    fw->on_init_template   = c_init_template;
    fw->on_config_defaults = c_config_defaults;
    fw->on_config_parse    = c_config_parse;
    fw->on_config_write    = c_config_write;
    fw->on_pkg_convert     = c_pkg_convert;

    /* set fw as its own userdata so callbacks can access framework */
    fw->userdata = fw;
}
