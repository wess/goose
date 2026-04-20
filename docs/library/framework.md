# Framework

`GooseFramework` is the struct every language plugin fills in. The framework calls back into you for anything language-specific; you call back into the framework for anything generic (config IO, dep resolution, filesystem).

```c
#include <goose/goose.h>
```

## The struct

From `headers/framework.h`:

```c
struct GooseFramework {
    /* identity + paths */
    char tool_name[64];
    char tool_version[64];
    char tool_description[256];
    char config_file[512];        // default: "goose.yaml"
    char lock_file[512];          // default: "goose.lock"
    char pkg_dir[512];            // default: "packages"
    char build_dir[512];          // default: "build"
    char src_dir[512];            // default: "src"
    char test_dir[512];           // default: "tests"
    char init_filename[512];      // default: "main.c"
    char gitignore_extra[1024];   // appended to generated .gitignore

    /* 11 lifecycle callbacks */
    goose_build_fn           on_build;
    goose_test_fn            on_test;
    goose_clean_fn           on_clean;
    goose_install_fn         on_install;
    goose_run_fn             on_run;
    goose_transpile_fn       on_transpile;
    goose_init_template_fn   on_init_template;
    goose_config_defaults_fn on_config_defaults;
    goose_config_parse_fn    on_config_parse;
    goose_config_write_fn    on_config_write;
    goose_pkg_convert_fn     on_pkg_convert;

    /* language-specific config scratch */
    char custom_data[8192];

    /* consumer-registered extra verbs */
    GooseExtraCmd extra_cmds[32];
    int extra_cmd_count;

    /* opaque pointer you own */
    void *userdata;
};
```

Stack-allocate it (`GooseFramework fw = {0}`) or heap-allocate via `goose_framework_new()`. Either works.

## Lifecycle

1. Zero the struct.
2. Call setters for identity and paths (`goose_framework_set_*`).
3. Register the callbacks you care about (`goose_framework_on_*`).
4. (Optional) Register extra verbs (`goose_framework_add_command`).
5. (Optional) Set `userdata` for anything you want to stash.
6. `goose_main(&fw, argc, argv)` — returns the process exit code.

## The 11 callbacks

Every callback receives `void *userdata` — whatever you stashed via `goose_framework_set_userdata`. The reference C plugin sets `userdata = &fw` itself so callbacks can reach `fw->custom_data` for language-specific config.

### `on_build`

```c
typedef int (*goose_build_fn)(const Config *cfg, int release,
                              const char *build_dir, const char *pkg_dir,
                              const char *config_file, void *userdata);
```

Called by `goose build`. Compile the project. `release` is 0 for debug, 1 for release. Return 0 on success.

### `on_run`

```c
typedef int (*goose_run_fn)(const Config *cfg, int release,
                            const char *build_dir, const char *pkg_dir,
                            const char *config_file,
                            int argc, char **argv, void *userdata);
```

Called by `goose run`. Typically: build, then exec the resulting binary with `argv` (`--release`/`-r` already consumed). Return the program's exit code.

### `on_test`

```c
typedef int (*goose_test_fn)(const Config *cfg, int release,
                             const char *build_dir, const char *pkg_dir,
                             const char *config_file, const char *test_dir,
                             void *userdata);
```

Called by `goose test`. Compile and run each test file in `test_dir`. Report pass/fail. Return non-zero if any test fails.

### `on_install`

```c
typedef int (*goose_install_fn)(const Config *cfg, const char *prefix,
                                const char *build_dir, const char *pkg_dir,
                                const char *config_file, void *userdata);
```

Called by `goose install [--prefix PATH]`. Default `prefix` is `/usr/local`. Typically: release build, then copy the binary to `{prefix}/bin/{cfg->name}`.

### `on_clean`

```c
typedef int (*goose_clean_fn)(const char *build_dir, void *userdata);
```

Called by `goose clean`. If you don't register it, the framework falls back to `fs_rmrf(build_dir)`.

### `on_transpile`

```c
typedef int (*goose_transpile_fn)(const Config *cfg, const char *build_dir,
                                  void *userdata);
```

Called at the start of `goose build` (before `on_build`). For C this dispatches the plugin system (flex/bison). If you don't use plugins, skip.

### `on_init_template`

```c
typedef int (*goose_init_template_fn)(const char *dir, const char *name,
                                      void *userdata);
```

Called by `goose new` and `goose init`. Write the starter source file into `dir` (a `src/` directory). The C plugin writes `main.c` with a `printf("Hello from <name>!")` template.

### `on_config_defaults`

```c
typedef void (*goose_config_defaults_fn)(Config *cfg, void *custom_data,
                                         void *userdata);
```

Called after `config_default` zeroes the struct. Fill in any language-specific defaults into `custom_data` (which points at `fw->custom_data`). C's implementation sets `cc=cc`, `cflags=-Wall -Wextra -std=c11`.

### `on_config_parse`

```c
typedef int (*goose_config_parse_fn)(const char *section, const char *key,
                                     const char *val, void *custom_data,
                                     void *userdata);
```

Called for every key in `build:` (and any other section you extend) that the core doesn't recognize. Store the value into your `custom_data`. Return 0.

### `on_config_write`

```c
typedef int (*goose_config_write_fn)(FILE *f, const void *custom_data,
                                     void *userdata);
```

Called inside `config_save` after the standard `build:` header. Emit your language-specific keys (C emits `cc:`, `cflags:`, `ldflags:`). Just `fprintf(f, "  key: \"%s\"\n", ...)`.

### `on_pkg_convert`

```c
typedef int (*goose_pkg_convert_fn)(const char *pkg_path,
                                    const char *config_file, void *userdata);
```

Called after every git fetch and every time a lock-sync visits an existing package. A hook for detecting and converting foreign manifests. The C plugin detects `CMakeLists.txt` and runs `cmake_convert_file()` to generate a `goose.yaml`.

## The custom_data convention

Language plugins stash a language-specific config struct in `fw->custom_data` (8 KB). The reference C plugin uses:

```c
// src/cc/config.h
typedef struct {
    char cc[64];
    char cflags[256];
    char ldflags[256];
} CConfig;
```

In `on_config_parse`, write into `(CConfig *)custom_data`. In `on_config_write`, read from `(const CConfig *)custom_data`. Same buffer, opaque to the framework.

Two gotchas:

1. **The framework reuses the same `custom_data` buffer for every `config_load`.** If you load a package's `goose.yaml` from inside `on_build`, you'll overwrite your project's config. Save a copy or parse into a scratch buffer if you need both.
2. **`on_config_parse` currently only fires for the `build:` section.** Unknown keys elsewhere are ignored.

## Accessing the framework from callbacks

Callbacks receive `void *userdata` — not `GooseFramework *fw` — because some callbacks are public library APIs too (`goose_config_parse_fn` etc.) and should work without a framework. But the convention for plugins is:

```c
// in your setup:
fw->userdata = fw;                     // or: goose_framework_set_userdata(fw, fw);

// in any callback:
GooseFramework *fw = (GooseFramework *)userdata;
CConfig *cc = (CConfig *)fw->custom_data;
```

The C plugin does exactly this (see `src/cc/setup.c`).

## Extra verbs

Register your own top-level commands alongside the built-ins:

```c
static int cmd_deploy(int argc, char **argv, GooseFramework *fw) {
    (void)argc; (void)argv;
    info("Deploying", "via %s", fw->tool_name);
    return 0;
}

goose_framework_add_command(&fw, "deploy",
    "Ship the release build", cmd_deploy);
```

Now `mytool deploy` runs your callback. Extra commands dispatch **after** built-ins, so you can't override `build`/`run`/etc. They dispatch **before** task-name fallback — a task named `deploy` in the config becomes shadowed.

Limit: 32 extra commands per framework.

## Setters, registrars, and FFI

Complete list from `headers/framework.h`:

```c
/* lifecycle */
GooseFramework *goose_framework_new(void);
void goose_framework_free(GooseFramework *fw);
int  goose_main(GooseFramework *fw, int argc, char **argv);

/* identity */
void goose_framework_set_tool_name(GooseFramework *, const char *);
void goose_framework_set_tool_version(GooseFramework *, const char *);
void goose_framework_set_tool_description(GooseFramework *, const char *);

/* paths */
void goose_framework_set_config_file(GooseFramework *, const char *);
void goose_framework_set_lock_file(GooseFramework *, const char *);
void goose_framework_set_pkg_dir(GooseFramework *, const char *);
void goose_framework_set_build_dir(GooseFramework *, const char *);
void goose_framework_set_src_dir(GooseFramework *, const char *);
void goose_framework_set_test_dir(GooseFramework *, const char *);
void goose_framework_set_init_filename(GooseFramework *, const char *);
void goose_framework_set_gitignore_extra(GooseFramework *, const char *);

/* callbacks */
void goose_framework_on_build(GooseFramework *, goose_build_fn);
void goose_framework_on_test(GooseFramework *, goose_test_fn);
void goose_framework_on_clean(GooseFramework *, goose_clean_fn);
void goose_framework_on_install(GooseFramework *, goose_install_fn);
void goose_framework_on_run(GooseFramework *, goose_run_fn);
void goose_framework_on_transpile(GooseFramework *, goose_transpile_fn);
void goose_framework_on_init_template(GooseFramework *, goose_init_template_fn);
void goose_framework_on_config_defaults(GooseFramework *, goose_config_defaults_fn);
void goose_framework_on_config_parse(GooseFramework *, goose_config_parse_fn);
void goose_framework_on_config_write(GooseFramework *, goose_config_write_fn);
void goose_framework_on_pkg_convert(GooseFramework *, goose_pkg_convert_fn);

/* extras */
int  goose_framework_add_command(GooseFramework *, const char *name,
                                 const char *desc, goose_cmd_fn fn);

/* userdata */
void  goose_framework_set_userdata(GooseFramework *, void *);
void *goose_framework_get_userdata(const GooseFramework *);
```

## Reference implementation

The best example is `src/cc/`:

- [`setup.c`](../../src/cc/setup.c) — wires everything up in one function
- [`config.c`](../../src/cc/config.c) — parse/write/defaults for `CConfig`
- [`build.c`](../../src/cc/build.c) — the real compile/link/test driver
- [`init.c`](../../src/cc/init.c) — `main.c` template + CMake auto-convert

And the 10-line entry point in [`src/main.c`](../../src/main.c).

## Next

- [API Reference](api.md) — every module function you'll call from your callbacks
- [Examples](examples.md) — longer worked programs
