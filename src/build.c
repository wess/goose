#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "headers/build.h"
#include "headers/config.h"
#include "headers/framework.h"
#include "headers/fs.h"
#include "headers/color.h"

void build_dep_base(const Dependency *dep, const char *pkg_dir,
                    char *buf, int bufsz) {
    if (dep->path[0])
        snprintf(buf, bufsz, "%s", dep->path);
    else
        snprintf(buf, bufsz, "%s/%s", pkg_dir, dep->name);
}

int build_collect_pkg_sources(const Config *cfg, const char *pkg_dir,
                              const char *config_file,
                              char files[][512], int max, int *count,
                              GooseFramework *fw) {
    *count = 0;
    if (cfg->dep_count == 0) return 0;

    for (int i = 0; i < cfg->dep_count; i++) {
        char base[512];
        build_dep_base(&cfg->deps[i], pkg_dir, base, sizeof(base));

        char pkg_cfg_path[512];
        snprintf(pkg_cfg_path, sizeof(pkg_cfg_path), "%s/%s", base, config_file);

        /* if package has config with explicit sources, use those */
        if (fs_exists(pkg_cfg_path)) {
            Config pkg_cfg;
            if (config_load(pkg_cfg_path, &pkg_cfg, fw) == 0 && pkg_cfg.source_count > 0) {
                for (int j = 0; j < pkg_cfg.source_count && *count < max; j++) {
                    snprintf(files[*count], 512, "%s/%s", base, pkg_cfg.sources[j]);
                    (*count)++;
                }
                continue;
            }
        }

        /* fallback: collect all .c files */
        char pkg_src[512];
        snprintf(pkg_src, sizeof(pkg_src), "%s/src", base);

        if (!fs_exists(pkg_src))
            snprintf(pkg_src, sizeof(pkg_src), "%s", base);

        int found = 0;
        fs_collect_sources(pkg_src, files + *count, max - *count, &found);
        *count += found;
    }
    return 0;
}

void build_include_flags(const Config *cfg, const char *pkg_dir,
                         const char *config_file, char *buf, int bufsz,
                         GooseFramework *fw) {
    buf[0] = '\0';
    int off = 0;

    /* project includes from config */
    for (int i = 0; i < cfg->include_count; i++)
        off += snprintf(buf + off, bufsz - off, "-I%s ", cfg->includes[i]);

    /* package includes */
    for (int i = 0; i < cfg->dep_count; i++) {
        char base[512];
        build_dep_base(&cfg->deps[i], pkg_dir, base, sizeof(base));

        char pkg_cfg_path[512];
        snprintf(pkg_cfg_path, sizeof(pkg_cfg_path), "%s/%s", base, config_file);

        if (fs_exists(pkg_cfg_path)) {
            Config pkg_cfg;
            if (config_load(pkg_cfg_path, &pkg_cfg, fw) == 0) {
                for (int j = 0; j < pkg_cfg.include_count; j++) {
                    char inc[512];
                    snprintf(inc, sizeof(inc), "%s/%s", base, pkg_cfg.includes[j]);
                    off += snprintf(buf + off, bufsz - off, "-I%s ", inc);
                }
                continue;
            }
        }

        /* fallback: try src/, root, include/ */
        char inc[512];
        snprintf(inc, sizeof(inc), "%s/src", base);
        if (fs_exists(inc))
            off += snprintf(buf + off, bufsz - off, "-I%s ", inc);

        off += snprintf(buf + off, bufsz - off, "-I%s ", base);

        snprintf(inc, sizeof(inc), "%s/include", base);
        if (fs_exists(inc))
            off += snprintf(buf + off, bufsz - off, "-I%s ", inc);
    }
}

int build_transpile(const Config *cfg, const char *build_dir) {
    if (cfg->plugin_count == 0) return 0;

    fs_mkdir(build_dir);
    char gen_dir[512];
    snprintf(gen_dir, sizeof(gen_dir), "%s/gen", build_dir);
    fs_mkdir(gen_dir);

    for (int p = 0; p < cfg->plugin_count; p++) {
        char plugin_files[MAX_SRC_FILES][512];
        int file_count = 0;
        fs_collect_ext(cfg->src_dir, cfg->plugins[p].ext,
                       plugin_files, MAX_SRC_FILES, &file_count);

        for (int i = 0; i < file_count; i++) {
            const char *base = strrchr(plugin_files[i], '/');
            base = base ? base + 1 : plugin_files[i];

            char stem[256];
            strncpy(stem, base, sizeof(stem) - 1);
            stem[sizeof(stem) - 1] = '\0';
            char *dot = strrchr(stem, '.');
            if (dot) *dot = '\0';

            info("Transpiling", "%s (%s)", base, cfg->plugins[p].name);

            char cmd[2048];
            snprintf(cmd, sizeof(cmd), "'%s' '%s' > '%s/%s.c'",
                     cfg->plugins[p].command, plugin_files[i], gen_dir, stem);

            if (system(cmd) != 0) {
                err("transpile failed: %s", plugin_files[i]);
                return -1;
            }
        }
    }
    return 0;
}
