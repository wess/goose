#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "headers/main.h"
#include "headers/config.h"
#include "headers/cmd.h"
#include "headers/color.h"

typedef struct {
    const char *name;
    const char *desc;
    int (*fn)(int, char **);
} Command;

static Command commands[] = {
    {"new",     "Create a new project",                    cmd_new},
    {"init",    "Initialize project in current directory",  cmd_init},
    {"build",   "Build the project",                       cmd_build},
    {"run",     "Build and run the project",               cmd_run},
    {"test",    "Build and run tests",                     cmd_test},
    {"clean",   "Remove build artifacts",                  cmd_clean},
    {"add",     "Add a dependency",                        cmd_add},
    {"remove",  "Remove a dependency",                     cmd_remove},
    {"update",  "Update all dependencies",                 cmd_update},
    {"install", "Install binary to system",                cmd_install},
    {"convert", "Convert CMakeLists.txt to goose.yaml",   cmd_convert},
    {"task",    "Run a project task",                     cmd_task},
    {NULL, NULL, NULL}
};

static void usage(void) {
    cprintf(CLR_GREEN, "goose %s", GOOSE_VERSION);
    printf(" - A package manager for C\n\n");
    printf("Usage: goose <command> [options]\n\n");
    printf("Commands:\n");
    for (Command *c = commands; c->name; c++) {
        cprintf(CLR_CYAN, "  %-10s", c->name);
        printf(" %s\n", c->desc);
    }
    printf("\n  --version  Print version\n");
    printf("  --help     Print this help\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage();
        return 1;
    }

    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0) {
        printf("goose %s\n", GOOSE_VERSION);
        return 0;
    }

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        usage();
        return 0;
    }

    for (Command *c = commands; c->name; c++) {
        if (strcmp(argv[1], c->name) == 0)
            return c->fn(argc - 1, argv + 1);
    }

    /* fallback: check if it's a task name */
    Config cfg;
    if (config_load(GOOSE_CONFIG, &cfg) == 0) {
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
    fprintf(stderr, "Run 'goose --help' for usage.\n");
    return 1;
}
