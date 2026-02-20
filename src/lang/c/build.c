#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "../../headers/config.h"
#include "../../headers/fs.h"
#include "../../headers/main.h"
#include "../../headers/color.h"

static void dep_base(const Dependency *dep, char *buf, int bufsz) {
    if (dep->path[0])
        snprintf(buf, bufsz, "%s", dep->path);
    else
        snprintf(buf, bufsz, "%s/%s", GOOSE_PKG_DIR, dep->name);
}

static int collect_pkg_sources(const Config *cfg, char files[][512], int max, int *count) {
    *count = 0;
    if (cfg->dep_count == 0) return 0;

    for (int i = 0; i < cfg->dep_count; i++) {
        char base[512];
        dep_base(&cfg->deps[i], base, sizeof(base));

        char pkg_cfg_path[512];
        snprintf(pkg_cfg_path, sizeof(pkg_cfg_path), "%s/%s", base, GOOSE_CONFIG);

        if (fs_exists(pkg_cfg_path)) {
            Config pkg_cfg;
            if (config_load(pkg_cfg_path, &pkg_cfg) == 0 && pkg_cfg.source_count > 0) {
                for (int j = 0; j < pkg_cfg.source_count && *count < max; j++) {
                    snprintf(files[*count], 512, "%s/%s", base, pkg_cfg.sources[j]);
                    (*count)++;
                }
                continue;
            }
        }

        char pkg_src[512];
        snprintf(pkg_src, sizeof(pkg_src), "%s/src", base);

        if (!fs_exists(pkg_src))
            snprintf(pkg_src, sizeof(pkg_src), "%s", base);

        int found = 0;
        fs_collect_sources(pkg_src, ".c", files + *count, max - *count, &found);
        *count += found;
    }
    return 0;
}

static void collect_pkg_ldflags(const Config *cfg, char *buf, int bufsz) {
    int off = (int)strlen(buf);
    for (int i = 0; i < cfg->dep_count; i++) {
        char base[512];
        dep_base(&cfg->deps[i], base, sizeof(base));

        char pkg_cfg_path[512];
        snprintf(pkg_cfg_path, sizeof(pkg_cfg_path), "%s/%s", base, GOOSE_CONFIG);
        if (!fs_exists(pkg_cfg_path)) continue;

        Config pkg_cfg;
        if (config_load(pkg_cfg_path, &pkg_cfg) != 0) continue;
        if (strlen(pkg_cfg.ldflags) == 0) continue;

        if (strstr(buf, pkg_cfg.ldflags) == NULL) {
            if (off > 0 && off < bufsz - 1) buf[off++] = ' ';
            off += snprintf(buf + off, bufsz - off, "%s", pkg_cfg.ldflags);
        }
    }
}

static void collect_pkg_defines(const Config *cfg, char *buf, int bufsz) {
    int off = (int)strlen(buf);
    for (int i = 0; i < cfg->dep_count; i++) {
        char base[512];
        dep_base(&cfg->deps[i], base, sizeof(base));

        char pkg_cfg_path[512];
        snprintf(pkg_cfg_path, sizeof(pkg_cfg_path), "%s/%s", base, GOOSE_CONFIG);
        if (!fs_exists(pkg_cfg_path)) continue;

        Config pkg_cfg;
        if (config_load(pkg_cfg_path, &pkg_cfg) != 0) continue;
        if (strlen(pkg_cfg.cflags) == 0) continue;

        const char *p = pkg_cfg.cflags;
        while (*p) {
            while (*p && isspace((unsigned char)*p)) p++;
            if (p[0] == '-' && p[1] == 'D') {
                const char *start = p;
                while (*p && !isspace((unsigned char)*p)) p++;
                int flen = (int)(p - start);
                char flag[256] = {0};
                if (flen < (int)sizeof(flag)) {
                    memcpy(flag, start, flen);
                    flag[flen] = '\0';
                    if (strstr(buf, flag) == NULL) {
                        if (off > 0 && off < bufsz - 1) buf[off++] = ' ';
                        off += snprintf(buf + off, bufsz - off, "%s", flag);
                    }
                }
            } else {
                while (*p && !isspace((unsigned char)*p)) p++;
            }
        }
    }
}

static void build_include_flags(const Config *cfg, char *buf, int bufsz) {
    buf[0] = '\0';
    int off = 0;

    for (int i = 0; i < cfg->include_count; i++)
        off += snprintf(buf + off, bufsz - off, "-I%s ", cfg->includes[i]);

    for (int i = 0; i < cfg->dep_count; i++) {
        char base[512];
        dep_base(&cfg->deps[i], base, sizeof(base));

        char pkg_cfg_path[512];
        snprintf(pkg_cfg_path, sizeof(pkg_cfg_path), "%s/%s", base, GOOSE_CONFIG);

        if (fs_exists(pkg_cfg_path)) {
            Config pkg_cfg;
            if (config_load(pkg_cfg_path, &pkg_cfg) == 0) {
                for (int j = 0; j < pkg_cfg.include_count; j++) {
                    char inc[512];
                    snprintf(inc, sizeof(inc), "%s/%s", base, pkg_cfg.includes[j]);
                    off += snprintf(buf + off, bufsz - off, "-I%s ", inc);
                }
                continue;
            }
        }

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

int c_transpile(const Config *cfg, void *ctx) {
    (void)ctx;
    if (cfg->plugin_count == 0) return 0;

    fs_mkdir(GOOSE_BUILD);
    char gen_dir[512];
    snprintf(gen_dir, sizeof(gen_dir), "%s/gen", GOOSE_BUILD);
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

int c_build(const Config *cfg, int release, void *ctx) {
    (void)ctx;

    char out_dir[512];
    snprintf(out_dir, sizeof(out_dir), "%s/%s",
             GOOSE_BUILD, release ? "release" : "debug");
    fs_mkdir(GOOSE_BUILD);
    fs_mkdir(out_dir);

    char src_files[MAX_SRC_FILES][512];
    int src_count = 0;
    fs_collect_sources(cfg->src_dir, ".c", src_files, MAX_SRC_FILES, &src_count);

    char gen_dir[512];
    snprintf(gen_dir, sizeof(gen_dir), "%s/gen", GOOSE_BUILD);
    if (fs_exists(gen_dir)) {
        int gen_count = 0;
        fs_collect_sources(gen_dir, ".c", src_files + src_count,
                           MAX_SRC_FILES - src_count, &gen_count);
        src_count += gen_count;
    }

    if (src_count == 0) {
        err("no source files found in %s/", cfg->src_dir);
        return -1;
    }

    char pkg_files[MAX_SRC_FILES][512];
    int pkg_count = 0;
    collect_pkg_sources(cfg, pkg_files, MAX_SRC_FILES, &pkg_count);

    char includes[4096];
    build_include_flags(cfg, includes, sizeof(includes));

    if (fs_exists(gen_dir)) {
        int ioff = (int)strlen(includes);
        snprintf(includes + ioff, sizeof(includes) - ioff, "-I%s ", gen_dir);
    }

    char pkg_defines[2048] = {0};
    collect_pkg_defines(cfg, pkg_defines, sizeof(pkg_defines));

    const char *mode_flags = release ? "-O2 -DNDEBUG" : "-g -DDEBUG";

    char cmd[16384];
    int off = snprintf(cmd, sizeof(cmd), "%s %s %s %s %s ",
                       cfg->cc, cfg->cflags, pkg_defines, mode_flags, includes);

    for (int i = 0; i < src_count; i++)
        off += snprintf(cmd + off, sizeof(cmd) - off, "'%s' ", src_files[i]);

    for (int i = 0; i < pkg_count; i++)
        off += snprintf(cmd + off, sizeof(cmd) - off, "'%s' ", pkg_files[i]);

    char output[512];
    snprintf(output, sizeof(output), "%s/%s", out_dir, cfg->name);
    off += snprintf(cmd + off, sizeof(cmd) - off, "-o '%s'", output);

    char all_ldflags[1024] = {0};
    if (strlen(cfg->ldflags) > 0)
        strncpy(all_ldflags, cfg->ldflags, sizeof(all_ldflags) - 1);

    collect_pkg_ldflags(cfg, all_ldflags, sizeof(all_ldflags));

    if (strlen(all_ldflags) > 0)
        off += snprintf(cmd + off, sizeof(cmd) - off, " %s", all_ldflags);

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
