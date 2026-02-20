#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <libgen.h>
#include "../headers/driver.h"
#include "../headers/config.h"
#include "../headers/pkg.h"
#include "../headers/lock.h"
#include "../headers/fs.h"
#include "../headers/main.h"
#include "../headers/color.h"

static int parse_release(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--release") == 0 || strcmp(argv[i], "-r") == 0)
            return 1;
    }
    return 0;
}

static int do_build(int argc, char **argv, const GooseDriver *drv) {
    int release = parse_release(argc, argv);

    Config cfg;
    if (config_load(GOOSE_CONFIG, &cfg) != 0)
        return 1;

    if (drv->defaults)
        drv->defaults(&cfg, drv->ctx);

    info("Building", "%s v%s (%s)", cfg.name, cfg.version,
         release ? "release" : "debug");

    LockFile lf;
    lock_load(GOOSE_LOCK, &lf);

    if (pkg_fetch_all(&cfg, &lf) != 0)
        return 1;

    lock_save(GOOSE_LOCK, &lf);

    if (drv->transpile && drv->transpile(&cfg, drv->ctx) != 0)
        return 1;

    return drv->build(&cfg, release, drv->ctx) != 0;
}

static int do_run(int argc, char **argv, const GooseDriver *drv) {
    int release = parse_release(argc, argv);

    Config cfg;
    if (config_load(GOOSE_CONFIG, &cfg) != 0)
        return 1;

    if (drv->defaults)
        drv->defaults(&cfg, drv->ctx);

    info("Building", "%s v%s (%s)", cfg.name, cfg.version,
         release ? "release" : "debug");

    LockFile lf;
    lock_load(GOOSE_LOCK, &lf);

    if (pkg_fetch_all(&cfg, &lf) != 0)
        return 1;

    lock_save(GOOSE_LOCK, &lf);

    if (drv->transpile && drv->transpile(&cfg, drv->ctx) != 0)
        return 1;

    if (drv->run)
        return drv->run(&cfg, release, argc, argv, drv->ctx);

    if (drv->build(&cfg, release, drv->ctx) != 0)
        return 1;

    char bin[512];
    snprintf(bin, sizeof(bin), "./%s/%s/%s",
             GOOSE_BUILD, release ? "release" : "debug", cfg.name);

    info("Running", "%s", bin);
    printf("\n");
    fflush(stdout);

    char cmd[4096];
    int off = snprintf(cmd, sizeof(cmd), "'%s'", bin);
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--release") == 0 || strcmp(argv[i], "-r") == 0)
            continue;
        off += snprintf(cmd + off, sizeof(cmd) - off, " '%s'", argv[i]);
    }

    return system(cmd);
}

static int do_test(int argc, char **argv, const GooseDriver *drv) {
    if (!drv->test) {
        err("test not supported for %s", drv->name);
        return 1;
    }

    int release = parse_release(argc, argv);

    Config cfg;
    if (config_load(GOOSE_CONFIG, &cfg) != 0)
        return 1;

    if (drv->defaults)
        drv->defaults(&cfg, drv->ctx);

    LockFile lf;
    lock_load(GOOSE_LOCK, &lf);
    if (pkg_fetch_all(&cfg, &lf) != 0)
        return 1;
    lock_save(GOOSE_LOCK, &lf);

    if (drv->transpile && drv->transpile(&cfg, drv->ctx) != 0)
        return 1;

    return drv->test(&cfg, release, drv->ctx);
}

static int do_clean(void) {
    if (fs_exists(GOOSE_BUILD)) {
        info("Cleaning", "%s/", GOOSE_BUILD);
        return fs_rmrf(GOOSE_BUILD);
    }
    info("Clean", "nothing to clean");
    return 0;
}

static int do_install(int argc, char **argv, const GooseDriver *drv) {
    const char *prefix = "/usr/local";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--prefix") == 0 && i + 1 < argc)
            prefix = argv[++i];
    }

    Config cfg;
    if (config_load(GOOSE_CONFIG, &cfg) != 0)
        return 1;

    if (drv->defaults)
        drv->defaults(&cfg, drv->ctx);

    if (drv->install)
        return drv->install(&cfg, prefix, drv->ctx);

    info("Building", "%s v%s (release)", cfg.name, cfg.version);

    LockFile lf;
    lock_load(GOOSE_LOCK, &lf);
    if (pkg_fetch_all(&cfg, &lf) != 0)
        return 1;
    lock_save(GOOSE_LOCK, &lf);

    if (drv->transpile && drv->transpile(&cfg, drv->ctx) != 0)
        return 1;

    if (drv->build(&cfg, 1, drv->ctx) != 0)
        return 1;

    char src[512], dest[512];
    snprintf(src, sizeof(src), "%s/release/%s", GOOSE_BUILD, cfg.name);
    snprintf(dest, sizeof(dest), "%s/bin/%s", prefix, cfg.name);

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "install -d '%s/bin' && install -m 755 '%s' '%s'",
             prefix, src, dest);

    info("Installing", "%s -> %s", cfg.name, dest);
    fflush(stdout);
    if (system(cmd) != 0) {
        err("installation failed (try with sudo?)");
        return 1;
    }

    info("Installed", "%s", dest);
    return 0;
}

static int do_init(const GooseDriver *drv) {
    if (fs_exists(GOOSE_CONFIG)) {
        err("%s already exists in this directory", GOOSE_CONFIG);
        return 1;
    }

    char cwd[512];
    if (!getcwd(cwd, sizeof(cwd))) {
        err("cannot determine current directory");
        return 1;
    }

    char *name = basename(cwd);

    Config cfg;
    config_default(&cfg, name);
    if (drv->defaults)
        drv->defaults(&cfg, drv->ctx);

    fs_mkdir(cfg.src_dir);

    if (drv->init)
        drv->init(&cfg, ".", drv->ctx);

    config_save(GOOSE_CONFIG, &cfg);
    info("Created", "goose project '%s'", name);
    return 0;
}

static int do_new(int argc, char **argv, const GooseDriver *drv) {
    if (argc < 2) {
        err("usage: goose new <name>");
        return 1;
    }

    const char *name = argv[1];

    if (fs_exists(name)) {
        err("directory '%s' already exists", name);
        return 1;
    }

    fs_mkdir(name);

    char path[512];
    snprintf(path, sizeof(path), "%s/src", name);
    fs_mkdir(path);

    Config cfg;
    config_default(&cfg, name);
    if (drv->defaults)
        drv->defaults(&cfg, drv->ctx);

    char cfg_path[512];
    snprintf(cfg_path, sizeof(cfg_path), "%s/%s", name, GOOSE_CONFIG);
    config_save(cfg_path, &cfg);

    if (drv->init)
        drv->init(&cfg, name, drv->ctx);

    snprintf(path, sizeof(path), "%s/.gitignore", name);
    fs_write_file(path,
        "build/\n"
        "packages/\n"
        "goose.lock\n");

    info("Created", "new goose project '%s'", name);
    return 0;
}

static int do_add(int argc, char **argv) {
    if (argc < 2) {
        err("usage: goose add <git-url> [--name <name>] [--version <tag>]");
        return 1;
    }

    Config cfg;
    if (config_load(GOOSE_CONFIG, &cfg) != 0)
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
    lock_load(GOOSE_LOCK, &lf);

    if (pkg_fetch(dep, GOOSE_PKG_DIR, &lf) != 0)
        return 1;

    lock_save(GOOSE_LOCK, &lf);
    config_save(GOOSE_CONFIG, &cfg);
    info("Added", "%s to %s", name, GOOSE_CONFIG);
    return 0;
}

static int do_remove(int argc, char **argv) {
    if (argc < 2) {
        err("usage: goose remove <name>");
        return 1;
    }

    Config cfg;
    if (config_load(GOOSE_CONFIG, &cfg) != 0)
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
        err("dependency '%s' not found in %s", name, GOOSE_CONFIG);
        return 1;
    }

    pkg_remove(name, GOOSE_PKG_DIR);

    for (int i = found; i < cfg.dep_count - 1; i++)
        cfg.deps[i] = cfg.deps[i + 1];
    cfg.dep_count--;

    config_save(GOOSE_CONFIG, &cfg);
    info("Removed", "%s from %s", name, GOOSE_CONFIG);
    return 0;
}

static int do_update(void) {
    Config cfg;
    if (config_load(GOOSE_CONFIG, &cfg) != 0)
        return 1;

    info("Updating", "%s dependencies", cfg.name);

    LockFile lf;
    lock_load(GOOSE_LOCK, &lf);

    if (pkg_update_all(&cfg, &lf) != 0)
        return 1;

    lock_save(GOOSE_LOCK, &lf);
    info("Updated", "lock file written to %s", GOOSE_LOCK);
    return 0;
}

static int do_task(int argc, char **argv) {
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

typedef struct {
    const char *name;
    const char *desc;
} CmdEntry;

static void usage(const GooseDriver *drv) {
    cprintf(CLR_GREEN, "goose %s", GOOSE_VERSION);
    printf(" - %s\n\n", drv->description);
    printf("Usage: goose <command> [options]\n\n");
    printf("Commands:\n");

    CmdEntry cmds[] = {
        {"new",     "Create a new project"},
        {"init",    "Initialize project in current directory"},
        {"build",   "Build the project"},
        {"run",     "Build and run the project"},
        {"test",    "Build and run tests"},
        {"clean",   "Remove build artifacts"},
        {"add",     "Add a dependency"},
        {"remove",  "Remove a dependency"},
        {"update",  "Update all dependencies"},
        {"install", "Install binary to system"},
        {"task",    "Run a project task"},
        {NULL, NULL}
    };

    if (drv->convert) {
        /* insert convert before task */
    }

    for (CmdEntry *c = cmds; c->name; c++) {
        cprintf(CLR_CYAN, "  %-10s", c->name);
        printf(" %s\n", c->desc);
    }

    if (drv->convert) {
        cprintf(CLR_CYAN, "  %-10s", "convert");
        printf(" Convert CMakeLists.txt to goose.yaml\n");
    }

    printf("\n  --version  Print version\n");
    printf("  --help     Print this help\n");
}

int goose_run(int argc, char **argv, const GooseDriver *driver) {
    if (argc < 2) {
        usage(driver);
        return 1;
    }

    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0) {
        printf("goose %s\n", GOOSE_VERSION);
        return 0;
    }

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        usage(driver);
        return 0;
    }

    const char *cmd = argv[1];
    int sub_argc = argc - 1;
    char **sub_argv = argv + 1;

    if (strcmp(cmd, "new") == 0)     return do_new(sub_argc, sub_argv, driver);
    if (strcmp(cmd, "init") == 0)    return do_init(driver);
    if (strcmp(cmd, "build") == 0)   return do_build(sub_argc, sub_argv, driver);
    if (strcmp(cmd, "run") == 0)     return do_run(sub_argc, sub_argv, driver);
    if (strcmp(cmd, "test") == 0)    return do_test(sub_argc, sub_argv, driver);
    if (strcmp(cmd, "clean") == 0)   return do_clean();
    if (strcmp(cmd, "add") == 0)     return do_add(sub_argc, sub_argv);
    if (strcmp(cmd, "remove") == 0)  return do_remove(sub_argc, sub_argv);
    if (strcmp(cmd, "update") == 0)  return do_update();
    if (strcmp(cmd, "install") == 0) return do_install(sub_argc, sub_argv, driver);
    if (strcmp(cmd, "task") == 0)    return do_task(sub_argc, sub_argv);

    if (strcmp(cmd, "convert") == 0 && driver->convert)
        return driver->convert(sub_argc, sub_argv, driver->ctx);

    /* fallback: check if it's a task name */
    Config cfg;
    if (config_load(GOOSE_CONFIG, &cfg) == 0) {
        for (int i = 0; i < cfg.task_count; i++) {
            if (strcmp(cmd, cfg.tasks[i].name) == 0) {
                info("Running", "%s", cfg.tasks[i].name);
                int ret = system(cfg.tasks[i].command);
                if (WIFEXITED(ret)) return WEXITSTATUS(ret);
                return 1;
            }
        }
    }

    err("unknown command '%s'", cmd);
    fprintf(stderr, "Run 'goose --help' for usage.\n");
    return 1;
}
