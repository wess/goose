#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../headers/cmd.h"
#include "../headers/config.h"
#include "../headers/build.h"
#include "../headers/pkg.h"
#include "../headers/lock.h"
#include "../headers/main.h"
#include "../headers/color.h"
#include "../headers/fs.h"

static int parse_release(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--release") == 0 || strcmp(argv[i], "-r") == 0)
            return 1;
    }
    return 0;
}

int cmd_build(int argc, char **argv) {
    int release = parse_release(argc, argv);

    Config cfg;
    if (config_load(GOOSE_CONFIG, &cfg) != 0)
        return 1;

    info("Building", "%s v%s (%s)", cfg.name, cfg.version,
         release ? "release" : "debug");

    LockFile lf;
    lock_load(GOOSE_LOCK, &lf);

    if (pkg_fetch_all(&cfg, &lf) != 0)
        return 1;

    lock_save(GOOSE_LOCK, &lf);

    return build_project(&cfg, release) != 0;
}

int cmd_run(int argc, char **argv) {
    int release = parse_release(argc, argv);

    Config cfg;
    if (config_load(GOOSE_CONFIG, &cfg) != 0)
        return 1;

    info("Building", "%s v%s (%s)", cfg.name, cfg.version,
         release ? "release" : "debug");

    LockFile lf;
    lock_load(GOOSE_LOCK, &lf);

    if (pkg_fetch_all(&cfg, &lf) != 0)
        return 1;

    lock_save(GOOSE_LOCK, &lf);

    if (build_project(&cfg, release) != 0)
        return 1;

    char bin[512];
    snprintf(bin, sizeof(bin), "./%s/%s/%s",
             cfg.build_dir, release ? "release" : "debug", cfg.name);

    info("Running", "%s", bin);
    printf("\n");
    fflush(stdout);

    /* pass remaining args (skip --release) */
    char cmd[4096];
    int off = snprintf(cmd, sizeof(cmd), "'%s'", bin);
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--release") == 0 || strcmp(argv[i], "-r") == 0)
            continue;
        off += snprintf(cmd + off, sizeof(cmd) - off, " '%s'", argv[i]);
    }

    return system(cmd);
}

int cmd_clean(int argc, char **argv) {
    (void)argc; (void)argv;

    Config cfg;
    if (config_load(GOOSE_CONFIG, &cfg) != 0)
        return 1;

    info("Cleaning", "%s", cfg.name);
    return build_clean(&cfg);
}

int cmd_test(int argc, char **argv) {
    int release = parse_release(argc, argv);

    Config cfg;
    if (config_load(GOOSE_CONFIG, &cfg) != 0)
        return 1;

    if (!fs_exists("tests")) {
        err("no tests/ directory found");
        return 1;
    }

    /* collect test files */
    char test_files[MAX_SRC_FILES][512];
    int test_count = 0;
    fs_collect_sources("tests", test_files, MAX_SRC_FILES, &test_count);

    if (test_count == 0) {
        warn("Test", "no test files found in tests/");
        return 0;
    }

    LockFile lf;
    lock_load(GOOSE_LOCK, &lf);
    if (pkg_fetch_all(&cfg, &lf) != 0)
        return 1;
    lock_save(GOOSE_LOCK, &lf);

    /* collect project sources (excluding main.c) */
    char src_files[MAX_SRC_FILES][512];
    int src_count = 0;
    fs_collect_sources(cfg.src_dir, src_files, MAX_SRC_FILES, &src_count);

    /* collect package sources */
    char pkg_files[MAX_SRC_FILES][512];
    int pkg_count = 0;
    for (int i = 0; i < cfg.dep_count; i++) {
        char pkg_src[512];
        snprintf(pkg_src, sizeof(pkg_src), "%s/%s/src",
                 GOOSE_PKG_DIR, cfg.deps[i].name);
        if (!fs_exists(pkg_src))
            snprintf(pkg_src, sizeof(pkg_src), "%s/%s",
                     GOOSE_PKG_DIR, cfg.deps[i].name);
        int found = 0;
        fs_collect_sources(pkg_src, pkg_files + pkg_count,
                           MAX_SRC_FILES - pkg_count, &found);
        pkg_count += found;
    }

    info("Testing", "%s v%s", cfg.name, cfg.version);

    int passed = 0, failed = 0;

    for (int t = 0; t < test_count; t++) {
        /* derive test name from file */
        const char *fname = strrchr(test_files[t], '/');
        fname = fname ? fname + 1 : test_files[t];
        char tname[128];
        strncpy(tname, fname, 127);
        char *dot = strrchr(tname, '.');
        if (dot) *dot = '\0';

        char out_dir[512];
        snprintf(out_dir, sizeof(out_dir), "%s/test", cfg.build_dir);
        fs_mkdir(cfg.build_dir);
        fs_mkdir(out_dir);

        char bin[512];
        snprintf(bin, sizeof(bin), "%s/%s", out_dir, tname);

        /* compile: test file + project sources (skip main.c) + pkg sources */
        char cmd[8192];
        const char *mode_flags = release ? "-O2 -DNDEBUG" : "-g -DDEBUG";
        int off = snprintf(cmd, sizeof(cmd), "%s %s %s ", cfg.cc, cfg.cflags, mode_flags);

        /* project includes */
        for (int i = 0; i < cfg.include_count; i++)
            off += snprintf(cmd + off, sizeof(cmd) - off, "-I%s ", cfg.includes[i]);

        /* package include paths */
        for (int i = 0; i < cfg.dep_count; i++) {
            char pkg_cfg_path[512];
            snprintf(pkg_cfg_path, sizeof(pkg_cfg_path),
                     "%s/%s/%s", GOOSE_PKG_DIR, cfg.deps[i].name, GOOSE_CONFIG);
            if (fs_exists(pkg_cfg_path)) {
                Config pkg_cfg;
                if (config_load(pkg_cfg_path, &pkg_cfg) == 0) {
                    for (int j = 0; j < pkg_cfg.include_count; j++) {
                        char inc[512];
                        snprintf(inc, sizeof(inc), "%s/%s/%s",
                                 GOOSE_PKG_DIR, cfg.deps[i].name, pkg_cfg.includes[j]);
                        off += snprintf(cmd + off, sizeof(cmd) - off, "-I%s ", inc);
                    }
                    continue;
                }
            }
            /* fallback */
            char inc[512];
            snprintf(inc, sizeof(inc), "%s/%s/src", GOOSE_PKG_DIR, cfg.deps[i].name);
            if (fs_exists(inc))
                off += snprintf(cmd + off, sizeof(cmd) - off, "-I%s ", inc);
            snprintf(inc, sizeof(inc), "%s/%s", GOOSE_PKG_DIR, cfg.deps[i].name);
            off += snprintf(cmd + off, sizeof(cmd) - off, "-I%s ", inc);
        }

        /* test file itself */
        off += snprintf(cmd + off, sizeof(cmd) - off, "'%s' ", test_files[t]);

        /* project .c files except main.c */
        for (int i = 0; i < src_count; i++) {
            const char *base = strrchr(src_files[i], '/');
            base = base ? base + 1 : src_files[i];
            if (strcmp(base, "main.c") == 0) continue;
            off += snprintf(cmd + off, sizeof(cmd) - off, "'%s' ", src_files[i]);
        }

        /* package .c files */
        for (int i = 0; i < pkg_count; i++)
            off += snprintf(cmd + off, sizeof(cmd) - off, "'%s' ", pkg_files[i]);

        off += snprintf(cmd + off, sizeof(cmd) - off, "-o '%s'", bin);
        if (strlen(cfg.ldflags) > 0)
            off += snprintf(cmd + off, sizeof(cmd) - off, " %s", cfg.ldflags);

        /* compile */
        fflush(stdout);
        if (system(cmd) != 0) {
            cprintf(CLR_RED, "      FAIL ");
            printf("%s (compile error)\n", tname);
            failed++;
            continue;
        }

        /* run */
        char runcmd[1024];
        snprintf(runcmd, sizeof(runcmd), "'%s' 2>&1", bin);
        int ret = system(runcmd);
        if (ret == 0) {
            cprintf(CLR_GREEN, "      PASS ");
            printf("%s\n", tname);
            passed++;
        } else {
            cprintf(CLR_RED, "      FAIL ");
            printf("%s (exit %d)\n", tname, ret);
            failed++;
        }
    }

    printf("\n");
    info("Results", "%d passed, %d failed, %d total",
         passed, failed, passed + failed);

    return failed > 0 ? 1 : 0;
}

int cmd_install(int argc, char **argv) {
    const char *prefix = "/usr/local";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--prefix") == 0 && i + 1 < argc)
            prefix = argv[++i];
    }

    Config cfg;
    if (config_load(GOOSE_CONFIG, &cfg) != 0)
        return 1;

    info("Building", "%s v%s (release)", cfg.name, cfg.version);

    LockFile lf;
    lock_load(GOOSE_LOCK, &lf);
    if (pkg_fetch_all(&cfg, &lf) != 0)
        return 1;
    lock_save(GOOSE_LOCK, &lf);

    if (build_project(&cfg, 1) != 0)
        return 1;

    char src[512], dest[512];
    snprintf(src, sizeof(src), "%s/release/%s", cfg.build_dir, cfg.name);
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
