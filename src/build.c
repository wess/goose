#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "headers/build.h"
#include "headers/fs.h"
#include "headers/main.h"
#include "headers/color.h"

static int collect_pkg_sources(const Config *cfg, char files[][512], int max, int *count) {
    *count = 0;
    if (cfg->dep_count == 0) return 0;

    for (int i = 0; i < cfg->dep_count; i++) {
        char pkg_src[512];

        snprintf(pkg_src, sizeof(pkg_src), "%s/%s/src",
                 GOOSE_PKG_DIR, cfg->deps[i].name);

        if (!fs_exists(pkg_src))
            snprintf(pkg_src, sizeof(pkg_src), "%s/%s",
                     GOOSE_PKG_DIR, cfg->deps[i].name);

        int found = 0;
        fs_collect_sources(pkg_src, files + *count, max - *count, &found);
        *count += found;
    }
    return 0;
}

static void build_include_flags(const Config *cfg, char *buf, int bufsz) {
    buf[0] = '\0';
    int off = 0;

    /* project includes from goose.yaml */
    for (int i = 0; i < cfg->include_count; i++)
        off += snprintf(buf + off, bufsz - off, "-I%s ", cfg->includes[i]);

    /* package includes: read each package's goose.yaml for its includes,
       or fall back to common locations */
    for (int i = 0; i < cfg->dep_count; i++) {
        char pkg_cfg_path[512];
        snprintf(pkg_cfg_path, sizeof(pkg_cfg_path),
                 "%s/%s/%s", GOOSE_PKG_DIR, cfg->deps[i].name, GOOSE_CONFIG);

        if (fs_exists(pkg_cfg_path)) {
            Config pkg_cfg;
            if (config_load(pkg_cfg_path, &pkg_cfg) == 0) {
                for (int j = 0; j < pkg_cfg.include_count; j++) {
                    char inc[512];
                    snprintf(inc, sizeof(inc), "%s/%s/%s",
                             GOOSE_PKG_DIR, cfg->deps[i].name, pkg_cfg.includes[j]);
                    off += snprintf(buf + off, bufsz - off, "-I%s ", inc);
                }
                continue;
            }
        }

        /* fallback: try src/, root, include/ */
        char inc[512];
        snprintf(inc, sizeof(inc), "%s/%s/src", GOOSE_PKG_DIR, cfg->deps[i].name);
        if (fs_exists(inc))
            off += snprintf(buf + off, bufsz - off, "-I%s ", inc);

        snprintf(inc, sizeof(inc), "%s/%s", GOOSE_PKG_DIR, cfg->deps[i].name);
        off += snprintf(buf + off, bufsz - off, "-I%s ", inc);

        snprintf(inc, sizeof(inc), "%s/%s/include", GOOSE_PKG_DIR, cfg->deps[i].name);
        if (fs_exists(inc))
            off += snprintf(buf + off, bufsz - off, "-I%s ", inc);
    }
}

int build_project(const Config *cfg, int release) {
    char out_dir[512];
    snprintf(out_dir, sizeof(out_dir), "%s/%s",
             cfg->build_dir, release ? "release" : "debug");
    fs_mkdir(cfg->build_dir);
    fs_mkdir(out_dir);

    char src_files[MAX_SRC_FILES][512];
    int src_count = 0;
    fs_collect_sources(cfg->src_dir, src_files, MAX_SRC_FILES, &src_count);

    if (src_count == 0) {
        err("no source files found in %s/", cfg->src_dir);
        return -1;
    }

    char pkg_files[MAX_SRC_FILES][512];
    int pkg_count = 0;
    collect_pkg_sources(cfg, pkg_files, MAX_SRC_FILES, &pkg_count);

    char includes[4096];
    build_include_flags(cfg, includes, sizeof(includes));

    /* mode flags */
    const char *mode_flags = release ? "-O2 -DNDEBUG" : "-g -DDEBUG";

    char cmd[8192];
    int off = snprintf(cmd, sizeof(cmd), "%s %s %s %s ",
                       cfg->cc, cfg->cflags, mode_flags, includes);

    for (int i = 0; i < src_count; i++)
        off += snprintf(cmd + off, sizeof(cmd) - off, "'%s' ", src_files[i]);

    for (int i = 0; i < pkg_count; i++)
        off += snprintf(cmd + off, sizeof(cmd) - off, "'%s' ", pkg_files[i]);

    char output[512];
    snprintf(output, sizeof(output), "%s/%s", out_dir, cfg->name);
    off += snprintf(cmd + off, sizeof(cmd) - off, "-o '%s'", output);

    if (strlen(cfg->ldflags) > 0)
        off += snprintf(cmd + off, sizeof(cmd) - off, " %s", cfg->ldflags);

    info("Compiling", "%s (%s)", cfg->name, release ? "release" : "debug");
    fflush(stdout);
    int ret = system(cmd);
    if (ret != 0) {
        err("compilation failed");
        return -1;
    }

    info("Finished", "%s", output);
    return 0;
}

int build_clean(const Config *cfg) {
    if (fs_exists(cfg->build_dir)) {
        info("Cleaning", "%s/", cfg->build_dir);
        return fs_rmrf(cfg->build_dir);
    }
    info("Clean", "nothing to clean");
    return 0;
}
