#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../headers/cmd.h"
#include "../headers/config.h"
#include "../headers/build.h"
#include "../headers/framework.h"
#include "../headers/pkg.h"
#include "../headers/lock.h"
#include "../headers/color.h"
#include "../headers/fs.h"

static int parse_release(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--release") == 0 || strcmp(argv[i], "-r") == 0)
            return 1;
    }
    return 0;
}

int cmd_build(int argc, char **argv, GooseFramework *fw) {
    int release = parse_release(argc, argv);

    Config cfg;
    if (config_load(fw->config_file, &cfg, fw) != 0)
        return 1;

    info("Building", "%s v%s (%s)", cfg.name, cfg.version,
         release ? "release" : "debug");

    LockFile lf;
    lock_load(fw->lock_file, &lf);

    if (pkg_fetch_all(&cfg, &lf, fw) != 0)
        return 1;

    lock_save(fw->lock_file, &lf);

    if (fw->on_transpile)
        fw->on_transpile(&cfg, fw->build_dir, fw->userdata);

    if (!fw->on_build) {
        err("no build callback registered");
        return 1;
    }

    return fw->on_build(&cfg, release, fw->build_dir, fw->pkg_dir,
                        fw->config_file, fw->userdata) != 0;
}

int cmd_run(int argc, char **argv, GooseFramework *fw) {
    int release = parse_release(argc, argv);

    Config cfg;
    if (config_load(fw->config_file, &cfg, fw) != 0)
        return 1;

    info("Building", "%s v%s (%s)", cfg.name, cfg.version,
         release ? "release" : "debug");

    LockFile lf;
    lock_load(fw->lock_file, &lf);

    if (pkg_fetch_all(&cfg, &lf, fw) != 0)
        return 1;

    lock_save(fw->lock_file, &lf);

    if (!fw->on_run) {
        err("no run callback registered");
        return 1;
    }

    return fw->on_run(&cfg, release, fw->build_dir, fw->pkg_dir,
                      fw->config_file, argc, argv, fw->userdata);
}

int cmd_clean(int argc, char **argv, GooseFramework *fw) {
    (void)argc; (void)argv;

    if (fw->on_clean)
        return fw->on_clean(fw->build_dir, fw->userdata);

    info("Cleaning", "%s/", fw->build_dir);
    if (fs_exists(fw->build_dir))
        return fs_rmrf(fw->build_dir);
    info("Clean", "nothing to clean");
    return 0;
}

int cmd_test(int argc, char **argv, GooseFramework *fw) {
    int release = parse_release(argc, argv);

    Config cfg;
    if (config_load(fw->config_file, &cfg, fw) != 0)
        return 1;

    LockFile lf;
    lock_load(fw->lock_file, &lf);
    if (pkg_fetch_all(&cfg, &lf, fw) != 0)
        return 1;
    lock_save(fw->lock_file, &lf);

    if (!fw->on_test) {
        err("no test callback registered");
        return 1;
    }

    return fw->on_test(&cfg, release, fw->build_dir, fw->pkg_dir,
                       fw->config_file, fw->test_dir, fw->userdata);
}

int cmd_install(int argc, char **argv, GooseFramework *fw) {
    const char *prefix = "/usr/local";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--prefix") == 0 && i + 1 < argc)
            prefix = argv[++i];
    }

    Config cfg;
    if (config_load(fw->config_file, &cfg, fw) != 0)
        return 1;

    info("Building", "%s v%s (release)", cfg.name, cfg.version);

    LockFile lf;
    lock_load(fw->lock_file, &lf);
    if (pkg_fetch_all(&cfg, &lf, fw) != 0)
        return 1;
    lock_save(fw->lock_file, &lf);

    if (!fw->on_install) {
        err("no install callback registered");
        return 1;
    }

    return fw->on_install(&cfg, prefix, fw->build_dir, fw->pkg_dir,
                          fw->config_file, fw->userdata);
}
