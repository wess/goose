# Quickstart

The shortest path to something working. Two programs: one for each common shape of libgoose consumer.

## Shape A — a small tool

You want to read `goose.yaml` and do something with it. No framework, no callbacks. Just call the module functions directly.

```c
// audit.c — print every dependency and its locked commit
#include <goose/goose.h>

int main(void) {
    Config cfg;
    LockFile lf = {0};

    if (config_load("goose.yaml", &cfg, NULL) != 0)
        return 1;

    lock_load("goose.lock", &lf);

    printf("%s v%s  (%d deps)\n\n", cfg.name, cfg.version, cfg.dep_count);
    for (int i = 0; i < cfg.dep_count; i++) {
        const char *sha = lock_find_sha(&lf, cfg.deps[i].name);
        printf("  %-20s %s\n", cfg.deps[i].name, cfg.deps[i].git);
        printf("  %-20s %s\n\n", "", sha ? sha : "(unlocked)");
    }
    return 0;
}
```

Build:

```sh
cc audit.c -I/usr/local/include -L/usr/local/lib -lgoose -o audit
```

Run it from any goose project:

```sh
cd your-project
./audit
```

Note the `NULL` passed for the framework — `config_load` accepts `NULL` when you don't need the language-specific callback hooks (`on_config_parse`, `on_config_defaults`). Standard fields (`name`, `version`, `includes`, `sources`, `dependencies`, `plugins`, `tasks`) always parse; only the `build:` language-specific keys (C's `cc`/`cflags`/`ldflags`) need a framework.

## Shape B — a full CLI

You want every goose verb (`new`, `build`, `run`, `test`, `add`, `remove`, `update`, `install`, `clean`, `convert`, `task`) for your own language or toolchain. Register callbacks, call `goose_main`.

The minimum viable language plugin — it works, but every callback just prints what it would do:

```c
// mytool.c — a fake "build tool" that echoes everything
#include <goose/goose.h>

static int my_build(const Config *cfg, int release, const char *build_dir,
                    const char *pkg_dir, const char *config_file, void *ud) {
    (void)pkg_dir; (void)config_file; (void)ud;
    info("Building", "%s v%s (%s) → %s",
         cfg->name, cfg->version, release ? "release" : "debug", build_dir);
    return 0;
}

static int my_clean(const char *build_dir, void *ud) {
    (void)ud;
    info("Cleaning", "%s", build_dir);
    return fs_rmrf(build_dir);
}

static int my_init(const char *dir, const char *name, void *ud) {
    (void)ud;
    char path[512];
    snprintf(path, sizeof(path), "%s/main.mylang", dir);
    return fs_write_file(path, "// starter code\n");
}

int main(int argc, char **argv) {
    GooseFramework fw = {0};

    goose_framework_set_tool_name(&fw, "mytool");
    goose_framework_set_tool_version(&fw, "0.1.0");
    goose_framework_set_tool_description(&fw, "A fake language plugin");
    goose_framework_set_config_file(&fw, "mytool.yaml");
    goose_framework_set_lock_file(&fw, "mytool.lock");
    goose_framework_set_pkg_dir(&fw, "deps");
    goose_framework_set_build_dir(&fw, "out");
    goose_framework_set_src_dir(&fw, "src");
    goose_framework_set_test_dir(&fw, "tests");
    goose_framework_set_init_filename(&fw, "main.mylang");

    goose_framework_on_build(&fw, my_build);
    goose_framework_on_clean(&fw, my_clean);
    goose_framework_on_init_template(&fw, my_init);

    return goose_main(&fw, argc, argv);
}
```

Build and try it:

```sh
cc mytool.c -I/usr/local/include -L/usr/local/lib -lgoose -o mytool

./mytool --help
./mytool new demo
cd demo
../mytool build
```

You now have: `mytool new`, `mytool init`, `mytool build`, `mytool clean`, `mytool add <url>`, `mytool remove`, `mytool update`, `mytool task`, `mytool --help`, `mytool --version` — all working against a `mytool.yaml` config file. All without writing YAML parsing, git cloning, lock files, CLI dispatch, or argument parsing.

Verbs you *didn't* register (`run`, `test`, `install`, `convert`) will print `no <x> callback registered` when invoked. Register them as your tool grows.

## What you just saw

- **Shape A**: the module APIs work standalone. Good for one-off tools.
- **Shape B**: the framework handles the plumbing (config IO, deps, lock, dispatch); callbacks handle the language-specific parts (how to build, test, install).

The real `goose` binary is exactly shape B — see [`src/main.c`](../../src/main.c) (10 lines) and [`src/cc/setup.c`](../../src/cc/setup.c) (fills in all the C callbacks).

## Next

- [Framework](framework.md) — every callback explained
- [API Reference](api.md) — every module function
- [Examples](examples.md) — longer programs
