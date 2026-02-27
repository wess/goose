#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "config.h"
#include "../headers/config.h"
#include "../headers/build.h"
#include "../headers/framework.h"
#include "../headers/fs.h"
#include "../headers/color.h"

/* collect ldflags from package config files */
static void collect_pkg_ldflags(const Config *cfg, const char *pkg_dir,
                                const char *config_file, char *buf, int bufsz,
                                GooseFramework *fw) {
    int off = (int)strlen(buf);
    for (int i = 0; i < cfg->dep_count; i++) {
        char base[512];
        build_dep_base(&cfg->deps[i], pkg_dir, base, sizeof(base));

        char pkg_cfg_path[512];
        snprintf(pkg_cfg_path, sizeof(pkg_cfg_path), "%s/%s", base, config_file);
        if (!fs_exists(pkg_cfg_path)) continue;

        Config pkg_cfg;
        if (config_load(pkg_cfg_path, &pkg_cfg, fw) != 0) continue;

        /* read ldflags from package custom_data */
        CConfig *pcc = (CConfig *)fw->custom_data;
        if (strlen(pcc->ldflags) == 0) continue;

        if (strstr(buf, pcc->ldflags) == NULL) {
            if (off > 0 && off < bufsz - 1) buf[off++] = ' ';
            off += snprintf(buf + off, bufsz - off, "%s", pcc->ldflags);
        }
    }
}

/* collect -D defines from package cflags */
static void collect_pkg_defines(const Config *cfg, const char *pkg_dir,
                                const char *config_file, char *buf, int bufsz,
                                GooseFramework *fw) {
    int off = (int)strlen(buf);
    for (int i = 0; i < cfg->dep_count; i++) {
        char base[512];
        build_dep_base(&cfg->deps[i], pkg_dir, base, sizeof(base));

        char pkg_cfg_path[512];
        snprintf(pkg_cfg_path, sizeof(pkg_cfg_path), "%s/%s", base, config_file);
        if (!fs_exists(pkg_cfg_path)) continue;

        Config pkg_cfg;
        if (config_load(pkg_cfg_path, &pkg_cfg, fw) != 0) continue;

        CConfig *pcc = (CConfig *)fw->custom_data;
        if (strlen(pcc->cflags) == 0) continue;

        /* extract -D flags from package cflags */
        const char *p = pcc->cflags;
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

int c_build(const Config *cfg, int release, const char *build_dir,
            const char *pkg_dir, const char *config_file, void *userdata) {
    (void)userdata;

    /* get framework from userdata to access custom_data */
    GooseFramework *fw = (GooseFramework *)userdata;
    CConfig *cc = (CConfig *)fw->custom_data;

    char out_dir[512];
    snprintf(out_dir, sizeof(out_dir), "%s/%s",
             build_dir, release ? "release" : "debug");
    fs_mkdir(build_dir);
    fs_mkdir(out_dir);

    char src_files[MAX_SRC_FILES][512];
    int src_count = 0;
    fs_collect_sources(cfg->src_dir, src_files, MAX_SRC_FILES, &src_count);

    /* collect generated sources */
    char gen_dir[512];
    snprintf(gen_dir, sizeof(gen_dir), "%s/gen", build_dir);
    if (fs_exists(gen_dir)) {
        int gen_count = 0;
        fs_collect_sources(gen_dir, src_files + src_count,
                           MAX_SRC_FILES - src_count, &gen_count);
        src_count += gen_count;
    }

    if (src_count == 0) {
        err("no source files found in %s/", cfg->src_dir);
        return -1;
    }

    char pkg_files[MAX_SRC_FILES][512];
    int pkg_count = 0;
    build_collect_pkg_sources(cfg, pkg_dir, config_file, pkg_files,
                              MAX_SRC_FILES, &pkg_count, fw);

    char includes[4096];
    build_include_flags(cfg, pkg_dir, config_file, includes,
                        sizeof(includes), fw);

    /* add generated source include path */
    if (fs_exists(gen_dir)) {
        int ioff = (int)strlen(includes);
        snprintf(includes + ioff, sizeof(includes) - ioff, "-I%s ", gen_dir);
    }

    /* collect package -D defines */
    char pkg_defines[2048] = {0};
    collect_pkg_defines(cfg, pkg_dir, config_file, pkg_defines,
                        sizeof(pkg_defines), fw);

    /* mode flags */
    const char *mode_flags = release ? "-O2 -DNDEBUG" : "-g -DDEBUG";

    char cmd[16384];
    int off = snprintf(cmd, sizeof(cmd), "%s %s %s %s %s ",
                       cc->cc, cc->cflags, pkg_defines, mode_flags, includes);

    for (int i = 0; i < src_count; i++)
        off += snprintf(cmd + off, sizeof(cmd) - off, "'%s' ", src_files[i]);

    for (int i = 0; i < pkg_count; i++)
        off += snprintf(cmd + off, sizeof(cmd) - off, "'%s' ", pkg_files[i]);

    char output[512];
    snprintf(output, sizeof(output), "%s/%s", out_dir, cfg->name);
    off += snprintf(cmd + off, sizeof(cmd) - off, "-o '%s'", output);

    /* project ldflags */
    char all_ldflags[1024] = {0};
    if (strlen(cc->ldflags) > 0)
        strncpy(all_ldflags, cc->ldflags, sizeof(all_ldflags) - 1);

    /* package ldflags */
    collect_pkg_ldflags(cfg, pkg_dir, config_file, all_ldflags,
                        sizeof(all_ldflags), fw);

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

int c_test(const Config *cfg, int release, const char *build_dir,
           const char *pkg_dir, const char *config_file, const char *test_dir,
           void *userdata) {
    GooseFramework *fw = (GooseFramework *)userdata;
    CConfig *cc = (CConfig *)fw->custom_data;

    if (!fs_exists(test_dir)) {
        err("no %s/ directory found", test_dir);
        return 1;
    }

    /* collect test files */
    char test_files[MAX_SRC_FILES][512];
    int test_count = 0;
    fs_collect_sources(test_dir, test_files, MAX_SRC_FILES, &test_count);

    if (test_count == 0) {
        warn("Test", "no test files found in %s/", test_dir);
        return 0;
    }

    if (build_transpile(cfg, build_dir) != 0)
        return 1;

    /* collect project sources (excluding main.c) */
    char src_files[MAX_SRC_FILES][512];
    int src_count = 0;
    fs_collect_sources(cfg->src_dir, src_files, MAX_SRC_FILES, &src_count);

    /* collect generated sources */
    char gen_dir[512];
    snprintf(gen_dir, sizeof(gen_dir), "%s/gen", build_dir);
    int has_gen = fs_exists(gen_dir);
    if (has_gen) {
        int gen_count = 0;
        fs_collect_sources(gen_dir, src_files + src_count,
                           MAX_SRC_FILES - src_count, &gen_count);
        src_count += gen_count;
    }

    /* collect package sources */
    char pkg_files[MAX_SRC_FILES][512];
    int pkg_count = 0;
    build_collect_pkg_sources(cfg, pkg_dir, config_file, pkg_files,
                              MAX_SRC_FILES, &pkg_count, fw);

    info("Testing", "%s v%s", cfg->name, cfg->version);

    int passed = 0, failed = 0;

    for (int t = 0; t < test_count; t++) {
        /* derive test name from file */
        const char *fname = strrchr(test_files[t], '/');
        fname = fname ? fname + 1 : test_files[t];
        char tname[128];
        strncpy(tname, fname, 127);
        char *dot = strrchr(tname, '.');
        if (dot) *dot = '\0';

        char tout_dir[512];
        snprintf(tout_dir, sizeof(tout_dir), "%s/test", build_dir);
        fs_mkdir(build_dir);
        fs_mkdir(tout_dir);

        char bin[512];
        snprintf(bin, sizeof(bin), "%s/%s", tout_dir, tname);

        /* compile: test file + project sources (skip main.c) + pkg sources */
        char cmd[8192];
        const char *mode_flags = release ? "-O2 -DNDEBUG" : "-g -DDEBUG";
        int off = snprintf(cmd, sizeof(cmd), "%s %s %s ",
                           cc->cc, cc->cflags, mode_flags);

        /* project includes */
        for (int i = 0; i < cfg->include_count; i++)
            off += snprintf(cmd + off, sizeof(cmd) - off, "-I%s ",
                            cfg->includes[i]);

        /* package include paths */
        for (int i = 0; i < cfg->dep_count; i++) {
            char base[512];
            build_dep_base(&cfg->deps[i], pkg_dir, base, sizeof(base));
            char pcfg[512];
            snprintf(pcfg, sizeof(pcfg), "%s/%s", base, config_file);
            if (fs_exists(pcfg)) {
                Config pkg_cfg;
                if (config_load(pcfg, &pkg_cfg, fw) == 0) {
                    for (int j = 0; j < pkg_cfg.include_count; j++) {
                        char inc[512];
                        snprintf(inc, sizeof(inc), "%s/%s",
                                 base, pkg_cfg.includes[j]);
                        off += snprintf(cmd + off, sizeof(cmd) - off,
                                        "-I%s ", inc);
                    }
                    continue;
                }
            }
            /* fallback */
            char inc[512];
            snprintf(inc, sizeof(inc), "%s/src", base);
            if (fs_exists(inc))
                off += snprintf(cmd + off, sizeof(cmd) - off, "-I%s ", inc);
            off += snprintf(cmd + off, sizeof(cmd) - off, "-I%s ", base);
        }

        /* generated source include path */
        if (has_gen)
            off += snprintf(cmd + off, sizeof(cmd) - off, "-I%s ", gen_dir);

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

        /* ldflags */
        if (strlen(cc->ldflags) > 0)
            off += snprintf(cmd + off, sizeof(cmd) - off, " %s", cc->ldflags);
        for (int pi = 0; pi < cfg->dep_count; pi++) {
            char base[512];
            build_dep_base(&cfg->deps[pi], pkg_dir, base, sizeof(base));
            char plp[512];
            snprintf(plp, sizeof(plp), "%s/%s", base, config_file);
            if (!fs_exists(plp)) continue;
            Config pc;
            if (config_load(plp, &pc, fw) == 0) {
                CConfig *pcc = (CConfig *)fw->custom_data;
                if (strlen(pcc->ldflags) > 0)
                    off += snprintf(cmd + off, sizeof(cmd) - off,
                                    " %s", pcc->ldflags);
            }
        }

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

int c_run(const Config *cfg, int release, const char *build_dir,
          const char *pkg_dir, const char *config_file,
          int argc, char **argv, void *userdata) {
    GooseFramework *fw = (GooseFramework *)userdata;

    if (c_build(cfg, release, build_dir, pkg_dir, config_file, userdata) != 0)
        return 1;

    char bin[512];
    snprintf(bin, sizeof(bin), "./%s/%s/%s",
             build_dir, release ? "release" : "debug", cfg->name);

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

    (void)fw;
    return system(cmd);
}

int c_install(const Config *cfg, const char *prefix, const char *build_dir,
              const char *pkg_dir, const char *config_file, void *userdata) {
    if (c_build(cfg, 1, build_dir, pkg_dir, config_file, userdata) != 0)
        return 1;

    char src[512], dest[512];
    snprintf(src, sizeof(src), "%s/release/%s", build_dir, cfg->name);
    snprintf(dest, sizeof(dest), "%s/bin/%s", prefix, cfg->name);

    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "install -d '%s/bin' && install -m 755 '%s' '%s'",
             prefix, src, dest);

    info("Installing", "%s -> %s", cfg->name, dest);
    fflush(stdout);
    if (system(cmd) != 0) {
        err("installation failed (try with sudo?)");
        return 1;
    }

    info("Installed", "%s", dest);
    return 0;
}

int c_clean(const char *build_dir, void *userdata) {
    (void)userdata;
    if (fs_exists(build_dir)) {
        info("Cleaning", "%s/", build_dir);
        return fs_rmrf(build_dir);
    }
    info("Clean", "nothing to clean");
    return 0;
}

int c_transpile(const Config *cfg, const char *build_dir, void *userdata) {
    (void)userdata;
    return build_transpile(cfg, build_dir);
}
