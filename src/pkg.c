#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "headers/pkg.h"
#include "headers/fs.h"
#include "headers/main.h"
#include "headers/color.h"
#include "headers/cmake.h"

char *pkg_name_from_git(const char *git_url) {
    static char name[128];
    const char *last_slash = strrchr(git_url, '/');
    const char *start = last_slash ? last_slash + 1 : git_url;

    strncpy(name, start, 127);
    name[127] = '\0';

    char *dot = strstr(name, ".git");
    if (dot && dot[4] == '\0')
        *dot = '\0';

    return name;
}

int pkg_get_sha(const char *pkg_path, char *sha, int sha_size) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "git -C '%s' rev-parse HEAD 2>/dev/null", pkg_path);

    FILE *p = popen(cmd, "r");
    if (!p) return -1;

    if (!fgets(sha, sha_size, p)) {
        pclose(p);
        return -1;
    }
    sha[strcspn(sha, "\n")] = '\0';
    pclose(p);
    return 0;
}

static int checkout_sha(const char *pkg_path, const char *sha) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "git -C '%s' fetch --quiet origin && git -C '%s' checkout --quiet '%s' 2>&1",
        pkg_path, pkg_path, sha);
    return system(cmd);
}

int pkg_fetch(const Dependency *dep, const char *pkg_dir, LockFile *lf) {
    /* path dependencies are local — skip git operations */
    if (dep->path[0]) {
        if (!fs_exists(dep->path)) {
            err("path dependency '%s' not found at %s", dep->name, dep->path);
            return -1;
        }
        /* resolve transitive deps from path dep's goose.yaml */
        char sub_cfg_path[512];
        snprintf(sub_cfg_path, sizeof(sub_cfg_path), "%s/%s", dep->path, GOOSE_CONFIG);
        if (fs_exists(sub_cfg_path)) {
            Config sub;
            if (config_load(sub_cfg_path, &sub) == 0 && sub.dep_count > 0) {
                info("Resolving", "transitive dependencies for %s", dep->name);
                for (int i = 0; i < sub.dep_count; i++)
                    pkg_fetch(&sub.deps[i], pkg_dir, lf);
            }
        }
        return 0;
    }

    char dest[512];
    snprintf(dest, sizeof(dest), "%s/%s", pkg_dir, dep->name);

    if (fs_exists(dest)) {
        /* if lock has a SHA, ensure we're on it */
        if (lf) {
            const char *locked = lock_find_sha(lf, dep->name);
            if (locked && strlen(locked) > 0) {
                char cur_sha[MAX_SHA_LEN] = {0};
                pkg_get_sha(dest, cur_sha, sizeof(cur_sha));
                if (strcmp(cur_sha, locked) != 0) {
                    info("Syncing", "%s to locked revision", dep->name);
                    fflush(stdout);
                    checkout_sha(dest, locked);
                }
            }
        }

        /* re-convert CMakeLists.txt if present (picks up converter improvements) */
        char cml[512];
        snprintf(cml, sizeof(cml), "%s/CMakeLists.txt", dest);
        if (fs_exists(cml)) {
            char ycfg[512];
            snprintf(ycfg, sizeof(ycfg), "%s/%s", dest, GOOSE_CONFIG);
            cmake_convert_file(cml, ycfg);
        }

        return 0;
    }

    info("Fetching", "%s from %s", dep->name, dep->git);
    fflush(stdout);
    fs_mkdir(pkg_dir);

    char cmd[1024];
    if (strlen(dep->version) > 0)
        snprintf(cmd, sizeof(cmd),
            "git clone --quiet --branch '%s' --depth 1 '%s' '%s' 2>&1",
            dep->version, dep->git, dest);
    else
        snprintf(cmd, sizeof(cmd),
            "git clone --quiet --depth 1 '%s' '%s' 2>&1",
            dep->git, dest);

    int ret = system(cmd);
    if (ret != 0) {
        err("failed to fetch '%s' from %s", dep->name, dep->git);
        return -1;
    }

    /* record SHA in lock */
    if (lf) {
        char sha[MAX_SHA_LEN] = {0};
        pkg_get_sha(dest, sha, sizeof(sha));
        lock_update_entry(lf, dep->name, dep->git, sha);
    }

    /* auto-convert CMakeLists.txt to goose.yaml */
    char sub_cfg_path[512];
    snprintf(sub_cfg_path, sizeof(sub_cfg_path), "%s/%s", dest, GOOSE_CONFIG);
    char cmake_path[512];
    snprintf(cmake_path, sizeof(cmake_path), "%s/CMakeLists.txt", dest);
    if (fs_exists(cmake_path)) {
        info("Converting", "CMakeLists.txt for %s", dep->name);
        cmake_convert_file(cmake_path, sub_cfg_path);
    }

    /* transitive deps: if package has goose.yaml, fetch its deps */
    if (fs_exists(sub_cfg_path)) {
        Config sub;
        if (config_load(sub_cfg_path, &sub) == 0 && sub.dep_count > 0) {
            info("Resolving", "transitive dependencies for %s", dep->name);
            for (int i = 0; i < sub.dep_count; i++)
                pkg_fetch(&sub.deps[i], pkg_dir, lf);
        }
    }

    return 0;
}

int pkg_remove(const char *name, const char *pkg_dir) {
    char dest[512];
    snprintf(dest, sizeof(dest), "%s/%s", pkg_dir, name);

    if (!fs_exists(dest)) {
        warn("Warn", "package '%s' not found in %s/", name, pkg_dir);
        return 0;
    }

    info("Removing", "%s", name);
    return fs_rmrf(dest);
}

int pkg_fetch_all(const Config *cfg, LockFile *lf) {
    if (cfg->dep_count == 0)
        return 0;

    info("Resolving", "dependencies (%d)", cfg->dep_count);
    for (int i = 0; i < cfg->dep_count; i++) {
        if (pkg_fetch(&cfg->deps[i], GOOSE_PKG_DIR, lf) != 0)
            return -1;
    }
    return 0;
}

int pkg_update_all(const Config *cfg, LockFile *lf) {
    if (cfg->dep_count == 0) {
        info("Update", "no dependencies to update");
        return 0;
    }

    for (int i = 0; i < cfg->dep_count; i++) {
        /* skip path dependencies — they're managed locally */
        if (cfg->deps[i].path[0]) continue;

        char dest[512];
        snprintf(dest, sizeof(dest), "%s/%s", GOOSE_PKG_DIR, cfg->deps[i].name);

        if (!fs_exists(dest)) {
            pkg_fetch(&cfg->deps[i], GOOSE_PKG_DIR, lf);
            continue;
        }

        info("Updating", "%s", cfg->deps[i].name);
        fflush(stdout);

        char cmd[1024];
        snprintf(cmd, sizeof(cmd),
            "git -C '%s' fetch --quiet origin && git -C '%s' pull --quiet 2>&1",
            dest, dest);
        system(cmd);

        /* update lock SHA */
        if (lf) {
            char sha[MAX_SHA_LEN] = {0};
            pkg_get_sha(dest, sha, sizeof(sha));
            lock_update_entry(lf, cfg->deps[i].name, cfg->deps[i].git, sha);
        }
    }
    return 0;
}
