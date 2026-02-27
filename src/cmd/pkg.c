#include <stdio.h>
#include <string.h>
#include "../headers/cmd.h"
#include "../headers/config.h"
#include "../headers/framework.h"
#include "../headers/pkg.h"
#include "../headers/lock.h"
#include "../headers/color.h"

int cmd_add(int argc, char **argv, GooseFramework *fw) {
    if (argc < 2) {
        err("usage: %s add <git-url> [--name <name>] [--version <tag>]",
            fw->tool_name);
        return 1;
    }

    Config cfg;
    if (config_load(fw->config_file, &cfg, fw) != 0)
        return 1;

    const char *git_url = argv[1];
    const char *name = NULL;
    const char *version = "";

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--name") == 0 && i + 1 < argc)
            name = argv[++i];
        else if (strcmp(argv[i], "--version") == 0 && i + 1 < argc)
            version = argv[++i];
    }

    if (!name)
        name = pkg_name_from_git(git_url);

    for (int i = 0; i < cfg.dep_count; i++) {
        if (strcmp(cfg.deps[i].name, name) == 0) {
            err("dependency '%s' already exists", name);
            return 1;
        }
    }

    if (cfg.dep_count >= MAX_DEPS) {
        err("too many dependencies (max %d)", MAX_DEPS);
        return 1;
    }

    Dependency *dep = &cfg.deps[cfg.dep_count];
    memset(dep, 0, sizeof(Dependency));
    strncpy(dep->name, name, MAX_NAME_LEN - 1);
    strncpy(dep->git, git_url, MAX_PATH_LEN - 1);
    strncpy(dep->version, version, 63);
    cfg.dep_count++;

    info("Adding", "%s", name);

    LockFile lf;
    lock_load(fw->lock_file, &lf);

    if (pkg_fetch(dep, fw->pkg_dir, &lf, fw) != 0)
        return 1;

    lock_save(fw->lock_file, &lf);
    config_save(fw->config_file, &cfg, fw);
    info("Added", "%s to %s", name, fw->config_file);
    return 0;
}

int cmd_remove(int argc, char **argv, GooseFramework *fw) {
    if (argc < 2) {
        err("usage: %s remove <name>", fw->tool_name);
        return 1;
    }

    Config cfg;
    if (config_load(fw->config_file, &cfg, fw) != 0)
        return 1;

    const char *name = argv[1];

    int found = -1;
    for (int i = 0; i < cfg.dep_count; i++) {
        if (strcmp(cfg.deps[i].name, name) == 0) {
            found = i;
            break;
        }
    }

    if (found < 0) {
        err("dependency '%s' not found in %s", name, fw->config_file);
        return 1;
    }

    pkg_remove(name, fw->pkg_dir);

    for (int i = found; i < cfg.dep_count - 1; i++)
        cfg.deps[i] = cfg.deps[i + 1];
    cfg.dep_count--;

    config_save(fw->config_file, &cfg, fw);
    info("Removed", "%s from %s", name, fw->config_file);
    return 0;
}

int cmd_update(int argc, char **argv, GooseFramework *fw) {
    (void)argc; (void)argv;

    Config cfg;
    if (config_load(fw->config_file, &cfg, fw) != 0)
        return 1;

    info("Updating", "%s dependencies", cfg.name);

    LockFile lf;
    lock_load(fw->lock_file, &lf);

    if (pkg_update_all(&cfg, &lf, fw) != 0)
        return 1;

    lock_save(fw->lock_file, &lf);
    info("Updated", "lock file written to %s", fw->lock_file);
    return 0;
}
