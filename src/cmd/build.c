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

/* build a single member at member_dir, return 0 on success */
static int build_member(const char *member_dir, int release,
                        GooseFramework *fw) {
    char cwd[1024];
    if (!getcwd(cwd, sizeof(cwd))) {
        err("cannot read working directory");
        return 1;
    }

    if (chdir(member_dir) != 0) {
        err("cannot enter workspace member: %s", member_dir);
        return 1;
    }

    int rc = 1;
    Config cfg;
    if (config_load(fw->config_file, &cfg, fw) == 0) {
        info("Building", "%s v%s (%s)", cfg.name, cfg.version,
             release ? "release" : "debug");

        LockFile lf;
        lock_load(fw->lock_file, &lf);
        if (pkg_fetch_all(&cfg, &lf, fw) == 0) {
            lock_save(fw->lock_file, &lf);
            if (fw->on_transpile)
                fw->on_transpile(&cfg, fw->build_dir, fw->userdata);
            if (fw->on_build)
                rc = fw->on_build(&cfg, release, fw->build_dir, fw->pkg_dir,
                                  fw->config_file, fw->userdata) != 0 ? 1 : 0;
            else
                err("no build callback registered");
        }
    }

    if (chdir(cwd) != 0) {
        err("cannot restore working directory");
        return 1;
    }
    return rc;
}

/* does member `a` depend (via a path: dep) on member `b`? */
static int member_depends_on(const char *a_dir, const char *b_dir,
                             const char *config_file, GooseFramework *fw) {
    char cfg_path[1024];
    snprintf(cfg_path, sizeof(cfg_path), "%s/%s", a_dir, config_file);
    if (!fs_exists(cfg_path)) return 0;

    Config cfg;
    if (config_load(cfg_path, &cfg, fw) != 0) return 0;

    for (int i = 0; i < cfg.dep_count; i++) {
        if (!cfg.deps[i].path[0]) continue;
        char resolved[1024];
        snprintf(resolved, sizeof(resolved), "%s/%s", a_dir, cfg.deps[i].path);
        /* compare canonical paths */
        char rp_a[1024], rp_b[1024];
        const char *ca = realpath(resolved, rp_a) ? rp_a : resolved;
        const char *cb = realpath(b_dir, rp_b) ? rp_b : b_dir;
        if (strcmp(ca, cb) == 0) return 1;
    }
    return 0;
}

/* build all workspace members in dependency order (topological by inter-member
 * path deps). returns 0 if every member built. */
static int build_workspace(const Config *root, int release,
                           GooseFramework *fw) {
    int n = root->ws_member_count;
    info("Workspace", "%d member%s", n, n == 1 ? "" : "s");

    int built[MAX_WS_MEMBERS] = {0};
    int order[MAX_WS_MEMBERS];
    int ordered = 0;

    /* repeatedly pick members whose member-deps are all already built */
    for (int pass = 0; pass < n && ordered < n; pass++) {
        int progress = 0;
        for (int i = 0; i < n; i++) {
            if (built[i]) continue;
            int ready = 1;
            for (int j = 0; j < n; j++) {
                if (i == j || built[j]) continue;
                if (member_depends_on(root->ws_members[i], root->ws_members[j],
                                      fw->config_file, fw)) {
                    ready = 0;
                    break;
                }
            }
            if (ready) {
                order[ordered++] = i;
                built[i] = 1;
                progress = 1;
            }
        }
        if (!progress) break; /* cycle: fall back to declared order */
    }
    /* append any leftover (cycle) members in declared order */
    for (int i = 0; i < n; i++) {
        int seen = 0;
        for (int k = 0; k < ordered; k++)
            if (order[k] == i) { seen = 1; break; }
        if (!seen) order[ordered++] = i;
    }

    int failed = 0;
    for (int k = 0; k < ordered; k++) {
        const char *m = root->ws_members[order[k]];
        if (build_member(m, release, fw) == 0) {
            info("Member", "%s ok", m);
        } else {
            err("member failed: %s", m);
            failed++;
        }
    }

    info("Workspace", "%d ok, %d failed", ordered - failed, failed);
    return failed > 0 ? 1 : 0;
}

int cmd_build(int argc, char **argv, GooseFramework *fw) {
    int release = parse_release(argc, argv);

    Config cfg;
    if (config_load(fw->config_file, &cfg, fw) != 0)
        return 1;

    if (cfg.ws_member_count > 0)
        return build_workspace(&cfg, release, fw);

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
