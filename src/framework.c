#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "headers/framework.h"
#include "headers/cmd.h"
#include "headers/color.h"

/* --- FFI allocation --- */

GooseFramework *goose_framework_new(void) {
    GooseFramework *fw = calloc(1, sizeof(GooseFramework));
    return fw;
}

void goose_framework_free(GooseFramework *fw) {
    free(fw);
}

/* --- field setters --- */

void goose_framework_set_tool_name(GooseFramework *fw, const char *name) {
    strncpy(fw->tool_name, name, sizeof(fw->tool_name) - 1);
}

void goose_framework_set_tool_version(GooseFramework *fw, const char *version) {
    strncpy(fw->tool_version, version, sizeof(fw->tool_version) - 1);
}

void goose_framework_set_tool_description(GooseFramework *fw, const char *desc) {
    strncpy(fw->tool_description, desc, sizeof(fw->tool_description) - 1);
}

void goose_framework_set_config_file(GooseFramework *fw, const char *filename) {
    strncpy(fw->config_file, filename, sizeof(fw->config_file) - 1);
}

void goose_framework_set_lock_file(GooseFramework *fw, const char *filename) {
    strncpy(fw->lock_file, filename, sizeof(fw->lock_file) - 1);
}

void goose_framework_set_pkg_dir(GooseFramework *fw, const char *dir) {
    strncpy(fw->pkg_dir, dir, sizeof(fw->pkg_dir) - 1);
}

void goose_framework_set_build_dir(GooseFramework *fw, const char *dir) {
    strncpy(fw->build_dir, dir, sizeof(fw->build_dir) - 1);
}

void goose_framework_set_src_dir(GooseFramework *fw, const char *dir) {
    strncpy(fw->src_dir, dir, sizeof(fw->src_dir) - 1);
}

void goose_framework_set_test_dir(GooseFramework *fw, const char *dir) {
    strncpy(fw->test_dir, dir, sizeof(fw->test_dir) - 1);
}

void goose_framework_set_init_filename(GooseFramework *fw, const char *filename) {
    strncpy(fw->init_filename, filename, sizeof(fw->init_filename) - 1);
}

void goose_framework_set_gitignore_extra(GooseFramework *fw, const char *extra) {
    strncpy(fw->gitignore_extra, extra, sizeof(fw->gitignore_extra) - 1);
}

/* --- callback registration --- */

void goose_framework_on_build(GooseFramework *fw, goose_build_fn fn) {
    fw->on_build = fn;
}

void goose_framework_on_test(GooseFramework *fw, goose_test_fn fn) {
    fw->on_test = fn;
}

void goose_framework_on_clean(GooseFramework *fw, goose_clean_fn fn) {
    fw->on_clean = fn;
}

void goose_framework_on_install(GooseFramework *fw, goose_install_fn fn) {
    fw->on_install = fn;
}

void goose_framework_on_run(GooseFramework *fw, goose_run_fn fn) {
    fw->on_run = fn;
}

void goose_framework_on_transpile(GooseFramework *fw, goose_transpile_fn fn) {
    fw->on_transpile = fn;
}

void goose_framework_on_init_template(GooseFramework *fw, goose_init_template_fn fn) {
    fw->on_init_template = fn;
}

void goose_framework_on_config_defaults(GooseFramework *fw, goose_config_defaults_fn fn) {
    fw->on_config_defaults = fn;
}

void goose_framework_on_config_parse(GooseFramework *fw, goose_config_parse_fn fn) {
    fw->on_config_parse = fn;
}

void goose_framework_on_config_write(GooseFramework *fw, goose_config_write_fn fn) {
    fw->on_config_write = fn;
}

void goose_framework_on_pkg_convert(GooseFramework *fw, goose_pkg_convert_fn fn) {
    fw->on_pkg_convert = fn;
}

/* --- extra commands --- */

int goose_framework_add_command(GooseFramework *fw, const char *name,
                                const char *desc, goose_cmd_fn fn) {
    if (fw->extra_cmd_count >= 32) return -1;
    GooseExtraCmd *cmd = &fw->extra_cmds[fw->extra_cmd_count];
    strncpy(cmd->name, name, sizeof(cmd->name) - 1);
    strncpy(cmd->desc, desc, sizeof(cmd->desc) - 1);
    cmd->fn = fn;
    fw->extra_cmd_count++;
    return 0;
}

/* --- userdata --- */

void goose_framework_set_userdata(GooseFramework *fw, void *ptr) {
    fw->userdata = ptr;
}

void *goose_framework_get_userdata(const GooseFramework *fw) {
    return fw->userdata;
}

/* --- built-in command dispatch table --- */

typedef struct {
    const char *name;
    const char *desc;
    int (*fn)(int, char **, GooseFramework *);
} BuiltinCmd;

static BuiltinCmd builtins[] = {
    {"new",     "Create a new project",                   cmd_new},
    {"init",    "Initialize project in current directory", cmd_init},
    {"build",   "Build the project",                      cmd_build},
    {"run",     "Build and run the project",              cmd_run},
    {"test",    "Build and run tests",                    cmd_test},
    {"clean",   "Remove build artifacts",                 cmd_clean},
    {"add",     "Add a dependency",                       cmd_add},
    {"remove",  "Remove a dependency",                    cmd_remove},
    {"update",  "Update all dependencies",                cmd_update},
    {"install", "Install binary to system",               cmd_install},
    {"convert", "Convert CMakeLists.txt to config",       cmd_convert},
    {"task",    "Run a project task",                     cmd_task},
    {NULL, NULL, NULL}
};

static void fw_usage(const GooseFramework *fw) {
    cprintf(CLR_GREEN, "%s %s", fw->tool_name, fw->tool_version);
    printf(" - %s\n\n", fw->tool_description);
    printf("Usage: %s <command> [options]\n\n", fw->tool_name);
    printf("Commands:\n");
    for (BuiltinCmd *c = builtins; c->name; c++) {
        cprintf(CLR_CYAN, "  %-10s", c->name);
        printf(" %s\n", c->desc);
    }
    for (int i = 0; i < fw->extra_cmd_count; i++) {
        cprintf(CLR_CYAN, "  %-10s", fw->extra_cmds[i].name);
        printf(" %s\n", fw->extra_cmds[i].desc);
    }
    printf("\n  --version  Print version\n");
    printf("  --help     Print this help\n");
}

int goose_main(GooseFramework *fw, int argc, char **argv) {
    if (argc < 2) {
        fw_usage(fw);
        return 1;
    }

    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0) {
        printf("%s %s\n", fw->tool_name, fw->tool_version);
        return 0;
    }

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        fw_usage(fw);
        return 0;
    }

    /* built-in commands */
    for (BuiltinCmd *c = builtins; c->name; c++) {
        if (strcmp(argv[1], c->name) == 0)
            return c->fn(argc - 1, argv + 1, fw);
    }

    /* extra consumer commands */
    for (int i = 0; i < fw->extra_cmd_count; i++) {
        if (strcmp(argv[1], fw->extra_cmds[i].name) == 0)
            return fw->extra_cmds[i].fn(argc - 1, argv + 1, fw);
    }

    /* fallback: check if it's a task name */
    Config cfg;
    if (config_load(fw->config_file, &cfg, fw) == 0) {
        for (int i = 0; i < cfg.task_count; i++) {
            if (strcmp(argv[1], cfg.tasks[i].name) == 0) {
                info("Running", "%s", cfg.tasks[i].name);
                int ret = system(cfg.tasks[i].command);
                if (WIFEXITED(ret)) return WEXITSTATUS(ret);
                return 1;
            }
        }
    }

    err("unknown command '%s'", argv[1]);
    fprintf(stderr, "Run '%s --help' for usage.\n", fw->tool_name);
    return 1;
}
