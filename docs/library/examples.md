# Examples

Longer working programs. Build each with:

```sh
cc example.c -I/usr/local/include -L/usr/local/lib -lgoose -o example
```

Run from any goose project.

## Dependency auditor

Print every dependency, the locked commit, and whether the checkout matches.

```c
// audit.c
#include <goose/goose.h>

int main(void) {
    Config cfg;
    if (config_load("goose.yaml", &cfg, NULL) != 0)
        return 1;

    LockFile lf = {0};
    lock_load("goose.lock", &lf);

    printf("%s v%s\n", cfg.name, cfg.version);
    printf("%d dependencies\n\n", cfg.dep_count);

    for (int i = 0; i < cfg.dep_count; i++) {
        const Dependency *d = &cfg.deps[i];

        if (d->path[0]) {
            info("PATH", "%-20s %s", d->name, d->path);
            continue;
        }

        const char *locked = lock_find_sha(&lf, d->name);

        char pkg_path[512];
        snprintf(pkg_path, sizeof(pkg_path), "packages/%s", d->name);

        if (!fs_exists(pkg_path)) {
            warn("MISSING", "%-20s %s", d->name, d->git);
            continue;
        }

        char current[MAX_SHA_LEN] = {0};
        pkg_get_sha(pkg_path, current, sizeof(current));

        if (!locked) {
            warn("UNLOCKED", "%-20s at %.12s", d->name, current);
        } else if (strcmp(current, locked) == 0) {
            info("OK", "%-20s %.12s", d->name, locked);
        } else {
            err("DRIFT %s: locked %.12s, on %.12s",
                d->name, locked, current);
        }
    }
    return 0;
}
```

Sample output:

```
myapp v0.1.0
3 dependencies

          OK libhttp              a1b2c3d4e5f6
          OK libjson              f6e5d4c3b2a1
        PATH locallib             ../locallib
```

## Project scaffolder

Create a project with a custom template, beyond what `goose new` provides.

```c
// scaffold.c
#include <goose/goose.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        err("usage: scaffold <name>");
        return 1;
    }

    const char *name = argv[1];
    char path[512];

    if (fs_exists(name)) {
        err("'%s' already exists", name);
        return 1;
    }

    fs_mkdir(name);
    snprintf(path, sizeof(path), "%s/src", name);
    fs_mkdir(path);
    snprintf(path, sizeof(path), "%s/tests", name);
    fs_mkdir(path);

    Config cfg;
    config_default(&cfg, name, NULL);
    strncpy(cfg.description, "Scaffolded project", sizeof(cfg.description) - 1);
    strncpy(cfg.author,      "You",                sizeof(cfg.author)      - 1);

    snprintf(path, sizeof(path), "%s/goose.yaml", name);
    config_save(path, &cfg, NULL);

    snprintf(path, sizeof(path), "%s/src/main.c", name);
    char src[1024];
    snprintf(src, sizeof(src),
        "#include <stdio.h>\n\n"
        "int main(void) {\n"
        "    printf(\"%s starting\\n\");\n"
        "    return 0;\n"
        "}\n", name);
    fs_write_file(path, src);

    snprintf(path, sizeof(path), "%s/tests/smoke.c", name);
    fs_write_file(path,
        "#include <assert.h>\n\n"
        "int main(void) { assert(1 + 1 == 2); return 0; }\n");

    snprintf(path, sizeof(path), "%s/.gitignore", name);
    fs_write_file(path, "build/\npackages/\ngoose.lock\n");

    info("Created", "%s", name);
    return 0;
}
```

## Companion tool: goose-deploy

A standalone tool that reads a project's `goose.yaml`, verifies a release build exists, and `scp`s it to a remote host. Runs alongside `goose` — no coupling to the C plugin.

```c
// goosedeploy.c
#include <goose/goose.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        err("usage: goosedeploy <user@host:path>");
        return 1;
    }
    const char *dest = argv[1];

    Config cfg;
    if (config_load("goose.yaml", &cfg, NULL) != 0)
        return 1;

    char src[512];
    snprintf(src, sizeof(src), "build/release/%s", cfg.name);

    if (!fs_exists(src)) {
        err("no release build at %s — run 'goose build --release' first", src);
        return 1;
    }

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "scp '%s' '%s'", src, dest);
    info("Deploying", "%s → %s", cfg.name, dest);
    return system(cmd);
}
```

```sh
cc goosedeploy.c -I/usr/local/include -L/usr/local/lib -lgoose -o goosedeploy

goose build -r
./goosedeploy user@server:/opt/bin/
```

Want `deploy` to be a real `goose` subcommand rather than a companion binary? Fork the source tree and `goose_framework_add_command(&fw, "deploy", ...)` inside `src/main.c`, or build your own framework consumer from scratch (see the Rust example below).

## A minimal language plugin — Rust

A toy Rust frontend. Doesn't actually invoke `rustc` — replace `my_build` with real calls.

```c
// goosers.c
#include <goose/goose.h>
#include <string.h>

typedef struct {
    char edition[16];   // "2021", "2024", etc.
    char rustflags[256];
} RustConfig;

static void rs_defaults(Config *cfg, void *custom, void *ud) {
    (void)cfg; (void)ud;
    RustConfig *rc = custom;
    strncpy(rc->edition, "2024", sizeof(rc->edition) - 1);
    rc->rustflags[0] = '\0';
}

static int rs_parse(const char *section, const char *key, const char *val,
                    void *custom, void *ud) {
    (void)section; (void)ud;
    RustConfig *rc = custom;
    if (strcmp(key, "edition") == 0)   strncpy(rc->edition,   val, 15);
    else if (strcmp(key, "rustflags") == 0) strncpy(rc->rustflags, val, 255);
    return 0;
}

static int rs_write(FILE *f, const void *custom, void *ud) {
    (void)ud;
    const RustConfig *rc = custom;
    fprintf(f, "  edition: \"%s\"\n", rc->edition);
    if (rc->rustflags[0])
        fprintf(f, "  rustflags: \"%s\"\n", rc->rustflags);
    return 0;
}

static int rs_build(const Config *cfg, int release, const char *build_dir,
                    const char *pkg_dir, const char *config_file, void *ud) {
    (void)pkg_dir; (void)config_file;
    GooseFramework *fw = ud;
    RustConfig *rc = (RustConfig *)fw->custom_data;

    info("Building", "%s v%s (%s, edition %s)",
         cfg->name, cfg->version, release ? "release" : "debug", rc->edition);

    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
        "rustc --edition %s %s %s src/main.rs -o %s/%s/%s",
        rc->edition, release ? "-O" : "",
        rc->rustflags, build_dir, release ? "release" : "debug", cfg->name);

    fs_mkdir(build_dir);
    char out[512];
    snprintf(out, sizeof(out), "%s/%s", build_dir, release ? "release" : "debug");
    fs_mkdir(out);

    return system(cmd);
}

static int rs_init(const char *dir, const char *name, void *ud) {
    (void)ud;
    char path[512];
    snprintf(path, sizeof(path), "%s/main.rs", dir);
    char src[256];
    snprintf(src, sizeof(src),
        "fn main() {\n    println!(\"Hello from %s!\");\n}\n", name);
    return fs_write_file(path, src);
}

int main(int argc, char **argv) {
    GooseFramework fw = {0};
    goose_framework_set_tool_name(&fw,        "goosers");
    goose_framework_set_tool_version(&fw,     "0.1.0");
    goose_framework_set_tool_description(&fw, "Goose for Rust");
    goose_framework_set_config_file(&fw,      "goosers.yaml");
    goose_framework_set_lock_file(&fw,        "goosers.lock");
    goose_framework_set_pkg_dir(&fw,          "crates");
    goose_framework_set_build_dir(&fw,        "target");
    goose_framework_set_src_dir(&fw,          "src");
    goose_framework_set_init_filename(&fw,    "main.rs");

    goose_framework_on_config_defaults(&fw, rs_defaults);
    goose_framework_on_config_parse(&fw,    rs_parse);
    goose_framework_on_config_write(&fw,    rs_write);
    goose_framework_on_build(&fw,           rs_build);
    goose_framework_on_init_template(&fw,   rs_init);

    goose_framework_set_userdata(&fw, &fw);

    return goose_main(&fw, argc, argv);
}
```

```sh
cc goosers.c -I/usr/local/include -L/usr/local/lib -lgoose -o goosers

./goosers new hello
cd hello
../goosers build
```

Produces a `goosers.yaml` with `edition: "2024"`, scaffolds `src/main.rs`, and drives `rustc`. Dependency management, lock file, `task`, `add`/`remove`, `--help` — all inherited.

## Next

- [Framework](framework.md) — callback reference
- [API Reference](api.md) — every function
