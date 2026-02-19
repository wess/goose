#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "headers/config.h"
#include "headers/main.h"
#include "headers/color.h"
#include "headers/cmd.h"

int cmd_task(int argc, char **argv) {
    Config cfg;
    if (config_load(GOOSE_CONFIG, &cfg) != 0)
        return 1;

    if (argc < 2) {
        if (cfg.task_count == 0) {
            info("Tasks", "none defined in %s", GOOSE_CONFIG);
            return 0;
        }
        printf("Available tasks:\n");
        for (int i = 0; i < cfg.task_count; i++) {
            cprintf(CLR_CYAN, "  %-16s", cfg.tasks[i].name);
            printf(" %s\n", cfg.tasks[i].command);
        }
        return 0;
    }

    const char *name = argv[1];
    for (int i = 0; i < cfg.task_count; i++) {
        if (strcmp(cfg.tasks[i].name, name) == 0) {
            info("Running", "%s", name);
            int ret = system(cfg.tasks[i].command);
            if (WIFEXITED(ret)) return WEXITSTATUS(ret);
            return 1;
        }
    }

    err("unknown task '%s'", name);
    return 1;
}
