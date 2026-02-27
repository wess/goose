#ifndef GOOSE_FRAMEWORK_H
#define GOOSE_FRAMEWORK_H

#include "config.h"
#include "lock.h"
#include <stdio.h>

/* forward declaration */
typedef struct GooseFramework GooseFramework;

/* callback typedefs */
typedef int  (*goose_build_fn)(const Config *cfg, int release,
                               const char *build_dir, const char *pkg_dir,
                               const char *config_file, void *userdata);
typedef int  (*goose_test_fn)(const Config *cfg, int release,
                              const char *build_dir, const char *pkg_dir,
                              const char *config_file, const char *test_dir,
                              void *userdata);
typedef int  (*goose_clean_fn)(const char *build_dir, void *userdata);
typedef int  (*goose_install_fn)(const Config *cfg, const char *prefix,
                                 const char *build_dir, const char *pkg_dir,
                                 const char *config_file, void *userdata);
typedef int  (*goose_run_fn)(const Config *cfg, int release,
                             const char *build_dir, const char *pkg_dir,
                             const char *config_file,
                             int argc, char **argv, void *userdata);
typedef int  (*goose_transpile_fn)(const Config *cfg, const char *build_dir,
                                   void *userdata);
typedef int  (*goose_init_template_fn)(const char *dir, const char *name,
                                       void *userdata);
typedef void (*goose_config_defaults_fn)(Config *cfg, void *custom_data,
                                         void *userdata);
typedef int  (*goose_config_parse_fn)(const char *section, const char *key,
                                      const char *val, void *custom_data,
                                      void *userdata);
typedef int  (*goose_config_write_fn)(FILE *f, const void *custom_data,
                                      void *userdata);
typedef int  (*goose_pkg_convert_fn)(const char *pkg_path,
                                     const char *config_file,
                                     void *userdata);
typedef int  (*goose_cmd_fn)(int argc, char **argv, GooseFramework *fw);

/* extra consumer-defined command */
typedef struct {
    char name[64];
    char desc[256];
    goose_cmd_fn fn;
} GooseExtraCmd;

struct GooseFramework {
    /* naming/paths */
    char tool_name[64];
    char tool_version[64];
    char tool_description[256];
    char config_file[512];
    char lock_file[512];
    char pkg_dir[512];
    char build_dir[512];
    char src_dir[512];
    char test_dir[512];
    char init_filename[512];
    char gitignore_extra[1024];

    /* lifecycle callbacks */
    goose_build_fn          on_build;
    goose_test_fn           on_test;
    goose_clean_fn          on_clean;
    goose_install_fn        on_install;
    goose_run_fn            on_run;
    goose_transpile_fn      on_transpile;
    goose_init_template_fn  on_init_template;
    goose_config_defaults_fn on_config_defaults;

    /* config extension */
    goose_config_parse_fn   on_config_parse;
    goose_config_write_fn   on_config_write;

    /* package hooks */
    goose_pkg_convert_fn    on_pkg_convert;

    /* custom data buffer for language-specific config storage */
    char custom_data[8192];

    /* extra consumer-defined commands */
    GooseExtraCmd extra_cmds[32];
    int extra_cmd_count;

    /* opaque consumer pointer */
    void *userdata;
};

/* FFI API */
GooseFramework *goose_framework_new(void);
void goose_framework_free(GooseFramework *fw);
int  goose_main(GooseFramework *fw, int argc, char **argv);

/* field setters */
void goose_framework_set_tool_name(GooseFramework *fw, const char *name);
void goose_framework_set_tool_version(GooseFramework *fw, const char *version);
void goose_framework_set_tool_description(GooseFramework *fw, const char *desc);
void goose_framework_set_config_file(GooseFramework *fw, const char *filename);
void goose_framework_set_lock_file(GooseFramework *fw, const char *filename);
void goose_framework_set_pkg_dir(GooseFramework *fw, const char *dir);
void goose_framework_set_build_dir(GooseFramework *fw, const char *dir);
void goose_framework_set_src_dir(GooseFramework *fw, const char *dir);
void goose_framework_set_test_dir(GooseFramework *fw, const char *dir);
void goose_framework_set_init_filename(GooseFramework *fw, const char *filename);
void goose_framework_set_gitignore_extra(GooseFramework *fw, const char *extra);

/* callback registration */
void goose_framework_on_build(GooseFramework *fw, goose_build_fn fn);
void goose_framework_on_test(GooseFramework *fw, goose_test_fn fn);
void goose_framework_on_clean(GooseFramework *fw, goose_clean_fn fn);
void goose_framework_on_install(GooseFramework *fw, goose_install_fn fn);
void goose_framework_on_run(GooseFramework *fw, goose_run_fn fn);
void goose_framework_on_transpile(GooseFramework *fw, goose_transpile_fn fn);
void goose_framework_on_init_template(GooseFramework *fw, goose_init_template_fn fn);
void goose_framework_on_config_defaults(GooseFramework *fw, goose_config_defaults_fn fn);
void goose_framework_on_config_parse(GooseFramework *fw, goose_config_parse_fn fn);
void goose_framework_on_config_write(GooseFramework *fw, goose_config_write_fn fn);
void goose_framework_on_pkg_convert(GooseFramework *fw, goose_pkg_convert_fn fn);

/* extra commands */
int goose_framework_add_command(GooseFramework *fw, const char *name,
                                const char *desc, goose_cmd_fn fn);

/* userdata */
void  goose_framework_set_userdata(GooseFramework *fw, void *ptr);
void *goose_framework_get_userdata(const GooseFramework *fw);

#endif
