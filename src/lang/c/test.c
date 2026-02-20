#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../../headers/config.h"
#include "../../headers/fs.h"
#include "../../headers/main.h"
#include "../../headers/color.h"

int c_test(const Config *cfg, int release, void *ctx) {
    (void)ctx;

    if (!fs_exists("tests")) {
        err("no tests/ directory found");
        return 1;
    }

    char test_files[MAX_SRC_FILES][512];
    int test_count = 0;
    fs_collect_sources("tests", ".c", test_files, MAX_SRC_FILES, &test_count);

    if (test_count == 0) {
        warn("Test", "no test files found in tests/");
        return 0;
    }

    char src_files[MAX_SRC_FILES][512];
    int src_count = 0;
    fs_collect_sources(cfg->src_dir, ".c", src_files, MAX_SRC_FILES, &src_count);

    char gen_dir[512];
    snprintf(gen_dir, sizeof(gen_dir), "%s/gen", GOOSE_BUILD);
    int has_gen = fs_exists(gen_dir);
    if (has_gen) {
        int gen_count = 0;
        fs_collect_sources(gen_dir, ".c", src_files + src_count,
                           MAX_SRC_FILES - src_count, &gen_count);
        src_count += gen_count;
    }

    /* collect package sources */
    char pkg_files[MAX_SRC_FILES][512];
    int pkg_count = 0;
    for (int i = 0; i < cfg->dep_count; i++) {
        char base[512];
        if (cfg->deps[i].path[0])
            snprintf(base, sizeof(base), "%s", cfg->deps[i].path);
        else
            snprintf(base, sizeof(base), "%s/%s", GOOSE_PKG_DIR, cfg->deps[i].name);

        char pkg_cfg_path[512];
        snprintf(pkg_cfg_path, sizeof(pkg_cfg_path), "%s/%s", base, GOOSE_CONFIG);
        if (fs_exists(pkg_cfg_path)) {
            Config pkg_cfg;
            if (config_load(pkg_cfg_path, &pkg_cfg) == 0 && pkg_cfg.source_count > 0) {
                for (int j = 0; j < pkg_cfg.source_count && pkg_count < MAX_SRC_FILES; j++) {
                    snprintf(pkg_files[pkg_count], 512, "%s/%s", base, pkg_cfg.sources[j]);
                    pkg_count++;
                }
                continue;
            }
        }
        char pkg_src[512];
        snprintf(pkg_src, sizeof(pkg_src), "%s/src", base);
        if (!fs_exists(pkg_src))
            snprintf(pkg_src, sizeof(pkg_src), "%s", base);
        int found = 0;
        fs_collect_sources(pkg_src, ".c", pkg_files + pkg_count,
                           MAX_SRC_FILES - pkg_count, &found);
        pkg_count += found;
    }

    info("Testing", "%s v%s", cfg->name, cfg->version);

    int passed = 0, failed = 0;

    for (int t = 0; t < test_count; t++) {
        const char *fname = strrchr(test_files[t], '/');
        fname = fname ? fname + 1 : test_files[t];
        char tname[128];
        strncpy(tname, fname, 127);
        tname[127] = '\0';
        char *dot = strrchr(tname, '.');
        if (dot) *dot = '\0';

        char out_dir[512];
        snprintf(out_dir, sizeof(out_dir), "%s/test", GOOSE_BUILD);
        fs_mkdir(GOOSE_BUILD);
        fs_mkdir(out_dir);

        char bin[512];
        snprintf(bin, sizeof(bin), "%s/%s", out_dir, tname);

        char cmd[8192];
        const char *mode_flags = release ? "-O2 -DNDEBUG" : "-g -DDEBUG";
        int off = snprintf(cmd, sizeof(cmd), "%s %s %s ", cfg->cc, cfg->cflags, mode_flags);

        for (int i = 0; i < cfg->include_count; i++)
            off += snprintf(cmd + off, sizeof(cmd) - off, "-I%s ", cfg->includes[i]);

        for (int i = 0; i < cfg->dep_count; i++) {
            char base[512];
            if (cfg->deps[i].path[0])
                snprintf(base, sizeof(base), "%s", cfg->deps[i].path);
            else
                snprintf(base, sizeof(base), "%s/%s", GOOSE_PKG_DIR, cfg->deps[i].name);

            char pkg_cfg_path[512];
            snprintf(pkg_cfg_path, sizeof(pkg_cfg_path), "%s/%s", base, GOOSE_CONFIG);
            if (fs_exists(pkg_cfg_path)) {
                Config pkg_cfg;
                if (config_load(pkg_cfg_path, &pkg_cfg) == 0) {
                    for (int j = 0; j < pkg_cfg.include_count; j++) {
                        char inc[512];
                        snprintf(inc, sizeof(inc), "%s/%s", base, pkg_cfg.includes[j]);
                        off += snprintf(cmd + off, sizeof(cmd) - off, "-I%s ", inc);
                    }
                    continue;
                }
            }
            char inc[512];
            snprintf(inc, sizeof(inc), "%s/src", base);
            if (fs_exists(inc))
                off += snprintf(cmd + off, sizeof(cmd) - off, "-I%s ", inc);
            off += snprintf(cmd + off, sizeof(cmd) - off, "-I%s ", base);
        }

        if (has_gen)
            off += snprintf(cmd + off, sizeof(cmd) - off, "-I%s ", gen_dir);

        off += snprintf(cmd + off, sizeof(cmd) - off, "'%s' ", test_files[t]);

        for (int i = 0; i < src_count; i++) {
            const char *base = strrchr(src_files[i], '/');
            base = base ? base + 1 : src_files[i];
            if (strcmp(base, "main.c") == 0) continue;
            off += snprintf(cmd + off, sizeof(cmd) - off, "'%s' ", src_files[i]);
        }

        for (int i = 0; i < pkg_count; i++)
            off += snprintf(cmd + off, sizeof(cmd) - off, "'%s' ", pkg_files[i]);

        off += snprintf(cmd + off, sizeof(cmd) - off, "-o '%s'", bin);

        if (strlen(cfg->ldflags) > 0)
            off += snprintf(cmd + off, sizeof(cmd) - off, " %s", cfg->ldflags);
        for (int pi = 0; pi < cfg->dep_count; pi++) {
            char plp[512];
            if (cfg->deps[pi].path[0])
                snprintf(plp, sizeof(plp), "%s/%s", cfg->deps[pi].path, GOOSE_CONFIG);
            else
                snprintf(plp, sizeof(plp), "%s/%s/%s",
                         GOOSE_PKG_DIR, cfg->deps[pi].name, GOOSE_CONFIG);
            if (!fs_exists(plp)) continue;
            Config pc;
            if (config_load(plp, &pc) == 0 && strlen(pc.ldflags) > 0)
                off += snprintf(cmd + off, sizeof(cmd) - off, " %s", pc.ldflags);
        }

        fflush(stdout);
        if (system(cmd) != 0) {
            cprintf(CLR_RED, "      FAIL ");
            printf("%s (compile error)\n", tname);
            failed++;
            continue;
        }

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
