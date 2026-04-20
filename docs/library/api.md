# API Reference

Every public function in `libgoose.a`, grouped by module. All headers live under `<goose/headers/>` and are re-exported by `<goose/goose.h>`.

Convention: functions return `0` on success, `-1` on error. Errors print to `stderr` via the `err()` macro.

## `<goose/headers/config.h>` — project config

Loads and saves `goose.yaml`.

### Types

```c
typedef struct {
    char name[MAX_NAME_LEN];   // 128
    char git[MAX_PATH_LEN];    // 512
    char version[64];
    char path[MAX_PATH_LEN];   // mutually exclusive with git
} Dependency;

typedef struct {
    char name[MAX_NAME_LEN];
    char ext[MAX_EXT_LEN];     // 16
    char command[MAX_CMD_LEN]; // 256
} Plugin;

typedef struct {
    char name[MAX_NAME_LEN];
    char command[MAX_TASK_CMD];  // 512
} Task;

typedef struct {
    char name[MAX_NAME_LEN];
    char version[64];
    char description[256];
    char author[MAX_NAME_LEN];
    char license[64];
    char type[16];               // "lib" for libraries, empty for binaries

    char src_dir[MAX_PATH_LEN];
    char includes[MAX_INCLUDES][MAX_PATH_LEN];     // 32
    int  include_count;
    char sources[MAX_SRC_FILES][MAX_PATH_LEN];     // 256
    int  source_count;

    Dependency deps[MAX_DEPS];                     // 64
    int        dep_count;
    Plugin     plugins[MAX_PLUGINS];               // 16
    int        plugin_count;
    Task       tasks[MAX_TASKS];                   // 32
    int        task_count;

    char ws_members[MAX_WS_MEMBERS][MAX_PATH_LEN]; // 32
    int  ws_member_count;
} Config;
```

Language-specific fields (`cc`, `cflags`, `ldflags` for C) do **not** live on `Config` — they live in `fw->custom_data`. See [Framework](framework.md#the-customdata-convention).

### Functions

```c
int  config_load(const char *path, Config *cfg, GooseFramework *fw);
int  config_save(const char *path, const Config *cfg, const GooseFramework *fw);
void config_default(Config *cfg, const char *name, GooseFramework *fw);
```

| | |
|---|---|
| `config_load(path, cfg, fw)` | Parse `path` into `cfg`. Calls `config_default` first so defaults survive partial YAML. Unknown `build:` keys fan out to `fw->on_config_parse`. Pass `fw = NULL` if you don't need language-specific parsing. |
| `config_save(path, cfg, fw)` | Write `cfg` to `path` as YAML. Calls `fw->on_config_write` to emit language-specific fields. `NULL` `fw` skips that fan-out. |
| `config_default(cfg, name, fw)` | Zero `cfg`, set `name`, version `0.1.0`, license `MIT`, `src_dir=src`, `includes=[src]`. Then calls `fw->on_config_defaults` for language extras. |

## `<goose/headers/lock.h>` — lock file

Reads and writes `goose.lock` (TOML-style `[[package]]` blocks).

### Types

```c
typedef struct {
    char name[MAX_NAME_LEN];
    char git[MAX_PATH_LEN];
    char sha[MAX_SHA_LEN];     // 64
} LockEntry;

typedef struct {
    LockEntry entries[MAX_DEPS];
    int count;
} LockFile;
```

### Functions

```c
int         lock_load(const char *path, LockFile *lf);
int         lock_save(const char *path, const LockFile *lf);
const char *lock_find_sha(const LockFile *lf, const char *name);
int         lock_update_entry(LockFile *lf, const char *name,
                              const char *git, const char *sha);
```

| | |
|---|---|
| `lock_load(path, lf)` | Parse the lock file. Zeroes `lf` first. Returns `-1` if the file doesn't exist. |
| `lock_save(path, lf)` | Write `lf` to disk. |
| `lock_find_sha(lf, name)` | Look up a package's pinned SHA. Returns `NULL` if not present. |
| `lock_update_entry(lf, name, git, sha)` | Insert or update an entry. |

## `<goose/headers/pkg.h>` — package management

Clones git dependencies, syncs lock SHAs, walks transitive deps.

```c
int   pkg_fetch(const Dependency *dep, const char *pkg_dir,
                LockFile *lf, GooseFramework *fw);
int   pkg_remove(const char *name, const char *pkg_dir);
int   pkg_fetch_all(const Config *cfg, LockFile *lf, GooseFramework *fw);
int   pkg_update_all(const Config *cfg, LockFile *lf, GooseFramework *fw);
char *pkg_name_from_git(const char *git_url);
int   pkg_get_sha(const char *pkg_path, char *sha, int sha_size);
```

| | |
|---|---|
| `pkg_fetch(dep, pkg_dir, lf, fw)` | Clone `dep` (if not already present) or sync to the locked SHA. Walks the package's own `goose.yaml` for transitives. Runs `fw->on_pkg_convert` after fetch. Path deps skip git ops entirely. |
| `pkg_remove(name, pkg_dir)` | `rm -rf {pkg_dir}/{name}`. |
| `pkg_fetch_all(cfg, lf, fw)` | Iterate `cfg->deps` and `pkg_fetch` each. Uses `fw->pkg_dir`. |
| `pkg_update_all(cfg, lf, fw)` | `git fetch && git pull` every git dep, rewrite lock SHAs. Path deps skipped. |
| `pkg_name_from_git(url)` | Extract `mylib` from `https://host/user/mylib.git`. Returns pointer into a static buffer — copy if you need to keep it. |
| `pkg_get_sha(pkg_path, buf, size)` | Run `git rev-parse HEAD` in `pkg_path`. Writes SHA into `buf`. |

## `<goose/headers/fs.h>` — filesystem helpers

Small portable wrappers over `stat`, `mkdir`, recursive traversal, and `rm -rf`.

```c
int fs_mkdir(const char *path);
int fs_exists(const char *path);
int fs_rmrf(const char *path);
int fs_write_file(const char *path, const char *content);
int fs_collect_sources(const char *dir, char files[][512], int max, int *count);
int fs_collect_ext(const char *dir, const char *ext,
                   char files[][512], int max, int *count);
```

| | |
|---|---|
| `fs_mkdir(path)` | Create a single directory (mode `0755`). No-op if it exists. Not recursive — call for each parent. |
| `fs_exists(path)` | Non-zero if `stat` succeeds. |
| `fs_rmrf(path)` | `rm -rf 'path'` via `system()`. |
| `fs_write_file(path, content)` | Overwrite `path` with `content`. |
| `fs_collect_sources(dir, files, max, count)` | Recursively find all `.c` under `dir`. Writes up to `max` paths into `files`. |
| `fs_collect_ext(dir, ext, files, max, count)` | Same, for any extension. Pass `ext` including the dot (`.h`, `.rs`). |

## `<goose/headers/build.h>` — shared build helpers

Language-agnostic building blocks used by `on_build` implementations. You'll usually call these from your own build callback.

```c
void build_dep_base(const Dependency *dep, const char *pkg_dir,
                    char *buf, int bufsz);
int  build_collect_pkg_sources(const Config *cfg, const char *pkg_dir,
                               const char *config_file,
                               char files[][512], int max, int *count,
                               GooseFramework *fw);
void build_include_flags(const Config *cfg, const char *pkg_dir,
                         const char *config_file, char *buf, int bufsz,
                         GooseFramework *fw);
int  build_transpile(const Config *cfg, const char *build_dir);
```

| | |
|---|---|
| `build_dep_base(dep, pkg_dir, buf, n)` | Write the base directory for a dependency into `buf`: `dep->path` for path deps, `{pkg_dir}/{dep->name}` for git deps. |
| `build_collect_pkg_sources(cfg, pkg_dir, cfg_file, files, max, count, fw)` | Walk every dep, collect source files. Prefers each package's explicit `sources:` list, else recursive `.c` scan. |
| `build_include_flags(cfg, pkg_dir, cfg_file, buf, n, fw)` | Build a string of `-I` flags covering project + every package's declared includes. |
| `build_transpile(cfg, build_dir)` | Run every plugin in `cfg->plugins` against matching files, write outputs to `{build_dir}/gen/`. |

## `<goose/headers/cmake.h>` — CMakeLists.txt converter

Converts CMake manifests to `goose.yaml`. Used by `goose convert` and called automatically for fetched packages that ship only `CMakeLists.txt`.

```c
typedef struct {
    char cflags[256];
    char ldflags[256];
} CmakeBuildInfo;

int cmake_to_config(const char *cmake_path, Config *cfg, CmakeBuildInfo *bi);
int cmake_convert_file(const char *cmake_path, const char *yaml_path);
```

| | |
|---|---|
| `cmake_to_config(cmake_path, cfg, bi)` | Parse a `CMakeLists.txt` into a `Config` plus a `CmakeBuildInfo` for C-specific fields (`cflags`, `ldflags`). |
| `cmake_convert_file(cmake_in, yaml_out)` | Parse and write `yaml_out` directly. Higher-level shortcut. |

Handles: `project()`, `set()`, `list(APPEND)`, `option()`, `add_library()`, `add_executable()`, `include_directories()`, `target_include_directories()`, `target_link_libraries()`, `file(GLOB)`, `add_subdirectory()` (recursive), `CHECK_INCLUDE_FILE()`, and `if()/elseif()/else()/endif()` with `${VAR}` expansion and `$<...>` generator-expression stripping.

## `<goose/headers/color.h>` — logging macros

Colored, right-aligned status lines matching the `goose` CLI. Colors are suppressed automatically when the target isn't a TTY.

```c
info("Fetching", "mathlib v1.0");      // green tag
warn("Skipping", "already exists");    // yellow tag
err("failed to open %s", path);        // red "error" tag, writes to stderr
```

Output:

```
    Fetching mathlib v1.0
    Skipping already exists
       error failed to open goose.yaml
```

Raw primitives also available:

```c
cprintf(CLR_GREEN, "fmt %s", arg);     // bold color to stdout
ceprintf(CLR_RED,  "fmt %s", arg);     // bold color to stderr
```

Color macros: `CLR_RESET`, `CLR_BOLD`, `CLR_RED`, `CLR_GREEN`, `CLR_YELLOW`, `CLR_CYAN`.

## `<goose/headers/cmd.h>` — built-in verbs

You almost never call these directly — they're what `goose_main` dispatches to — but you can invoke one from an extra verb if you want to wrap a built-in.

```c
int cmd_init   (int argc, char **argv, GooseFramework *fw);
int cmd_new    (int argc, char **argv, GooseFramework *fw);
int cmd_build  (int argc, char **argv, GooseFramework *fw);
int cmd_run    (int argc, char **argv, GooseFramework *fw);
int cmd_clean  (int argc, char **argv, GooseFramework *fw);
int cmd_add    (int argc, char **argv, GooseFramework *fw);
int cmd_remove (int argc, char **argv, GooseFramework *fw);
int cmd_update (int argc, char **argv, GooseFramework *fw);
int cmd_test   (int argc, char **argv, GooseFramework *fw);
int cmd_install(int argc, char **argv, GooseFramework *fw);
int cmd_convert(int argc, char **argv, GooseFramework *fw);
int cmd_task   (int argc, char **argv, GooseFramework *fw);
```

## `<goose/headers/main.h>` — version

```c
#define GOOSE_VERSION "0.1.1"   // resolved from the VERSION file at build time
```

## Compatibility notes

- All structs are stack-friendly; the only heap allocation is `goose_framework_new()`.
- Shell commands quote paths with single quotes. Paths containing `'` will break — don't pass them.
- `pkg_fetch` requires `git` on `$PATH`; `fs_rmrf` uses `rm`.
- Limits (`MAX_DEPS`, `MAX_SRC_FILES`, etc.) are fixed at compile time — bump them in `src/headers/config.h` and recompile if you need more.
