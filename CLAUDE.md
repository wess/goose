# CLAUDE.md

## Project Overview

Goose — a Cargo-inspired package manager and build tool. The core is a **generic, language-agnostic framework** (`libgoose.a`) with C wired up as the first consumer plugin in `src/cc/`. Consumers can build a Cargo-like tool for any language by supplying a `GooseFramework` with the right callbacks. Uses `goose.yaml` for project configuration, `goose.lock` for reproducible dependency pinning, git URLs or local paths for package sources, and named tasks for shell commands. Version is read from the `VERSION` file at build time.

## Project Structure

```
src/
  main.c              — 10-line entry: zero a GooseFramework, goose_c_setup(&fw), goose_main(&fw, argc, argv)
  framework.c         — GooseFramework lifecycle, field setters, callback registration, command dispatch (goose_main), built-in verb table, task-name fallback
  config.c            — YAML parser/writer for goose.yaml. Known keys handled inline; unknown build:/project: keys fan out via fw->on_config_parse / on_config_write
  build.c             — shared build helpers (language-agnostic): build_dep_base, build_collect_pkg_sources, build_include_flags, build_transpile
  pkg.c               — package ops: git clone/pull/remove, transitive dep resolution, lock-SHA sync, on_pkg_convert hook for non-native packages
  fs.c                — filesystem helpers: mkdir, exists, rmrf, write_file, recursive .c/.ext collection
  lock.c              — goose.lock read/write/lookup (TOML-like [[package]] format)
  cmake.c             — CMakeLists.txt → goose.yaml converter (~820 LOC; variable table, if/else, add_subdirectory recursion)
  cmd/
    init.c            — goose init, goose new (calls fw->on_init_template for language-specific starter files)
    build.c           — goose build, goose run, goose clean, goose test, goose install (generic — loads config, resolves deps, then delegates to fw->on_build/on_run/on_test/on_install)
    pkg.c             — goose add, goose remove, goose update
    convert.c         — goose convert (CMake to goose.yaml)
    task.c            — goose task (list/run named tasks from goose.yaml)
  cc/                 — C language consumer plugin (wires framework to cc toolchain)
    setup.c           — goose_c_setup: registers all c_* callbacks, sets tool_name=goose, config_file=goose.yaml, pkg_dir=packages, etc.
    setup.h           — public entry for consumer setup
    config.c          — CConfig parse/write/defaults callbacks (cc, cflags, ldflags stored in fw->custom_data)
    config.h          — CConfig struct (language-specific build fields)
    build.c           — c_build, c_test, c_run, c_install, c_clean, c_transpile (the actual compile/link/test drivers)
    init.c            — c_init_template (default main.c), c_pkg_convert (CMakeLists.txt detection + convert)
  headers/
    main.h            — GOOSE_VERSION macro (from VERSION file via -DGOOSE_VERSION_FROM_FILE)
    framework.h       — GooseFramework struct, callback typedefs, FFI API (goose_framework_new/free, goose_main, setters, on_* registrars, add_command, userdata)
    config.h          — Config/Dependency/Task/Plugin structs, limits (MAX_DEPS=64, MAX_SRC_FILES=256, MAX_INCLUDES=32, MAX_PLUGINS=16, MAX_TASKS=32, MAX_WS_MEMBERS=32), workspace fields (type, ws_members, ws_member_count)
    build.h           — build_transpile(), build_include_flags(), build_collect_pkg_sources(), build_dep_base() — shared helpers consumed by language-specific build callbacks
    pkg.h             — pkg_fetch(), pkg_remove(), pkg_fetch_all(), pkg_update_all(), pkg_name_from_git(), pkg_get_sha()
    fs.h              — fs_mkdir(), fs_exists(), fs_rmrf(), fs_write_file(), fs_collect_sources(), fs_collect_ext()
    lock.h            — LockFile/LockEntry structs, lock_load/save/find_sha/update_entry
    cmake.h           — cmake_to_config(), cmake_convert_file()
    color.h           — ANSI color macros, info()/warn()/err() logging macros, TTY detection
    cmd.h             — all cmd_* function declarations (signature: int cmd_*(int argc, char **argv, GooseFramework *fw))
include/
  goose.h             — public library header, re-exports all internal headers for libgoose.a consumers
libs/libyaml/         — vendored libyaml 0.2.5 (compiled from source, no system dependency needed)
examples/             — example projects (mathapp, mathlib, stringlib, gdexample)
docs/                 — user-facing documentation (getting-started, commands, configuration, dependencies, creating-packages, plugins, library)
```

## Build

```
make                  # builds both build/libgoose.a and build/goose
make lib              # builds only build/libgoose.a (static library)
make cli              # builds only build/goose (CLI binary)
make clean            # removes build/
```

libyaml is vendored in `libs/libyaml/` and compiled directly — no system install required. The Makefile compiles `src/*.c`, `src/cmd/*.c`, and `src/cc/*.c` with `-std=c11 -Wall -Wextra`. Everything except `main.c` goes into `libgoose.a` (framework core + C consumer plugin + libyaml). The CLI binary is just `main.o` linked against the static lib. Version is injected via `-DGOOSE_VERSION_FROM_FILE` from the `VERSION` file. Object files go to `build/obj/`, `build/obj/cmd/`, `build/obj/cc/`.

## Install

```
make install                  # installs binary, library, and headers to /usr/local
make install PREFIX=~/.local  # custom prefix
make uninstall                # removes installed files
```

Installs: `PREFIX/bin/goose`, `PREFIX/lib/libgoose.a`, `PREFIX/include/goose/goose.h`, `PREFIX/include/goose/headers/*.h`.

## Commands

```
goose new <name>                  # create new project directory with goose.yaml, src/main.c, .gitignore
goose init                        # initialize goose project in current directory
goose build [--release|-r]        # compile (fetches deps first, writes goose.lock)
goose run [--release|-r] [args]   # build then execute, passes extra args to binary
goose test [--release|-r]         # compile+run each .c in tests/ (excludes main.c from project sources)
goose clean                       # rm -rf build/
goose add <git-url> [--name N] [--version TAG]  # add dependency, fetch, update goose.yaml + goose.lock
goose remove <name>               # remove from goose.yaml + delete from packages/
goose update                      # git pull all deps, update goose.lock SHAs (skips path deps)
goose install [--prefix PATH]     # release build + install binary to prefix/bin/
goose convert [FILE] [--input F] [--output F]   # convert CMakeLists.txt to goose.yaml
goose task [name]                 # list tasks (no arg) or run named task from goose.yaml
goose <taskname>                  # shorthand: unknown commands fall back to task lookup
```

## goose.yaml Schema

```yaml
workspace:                 # optional, makes this a workspace root
  members:
    - "mylib"
    - "myapp"

project:
  name: "myapp"
  version: "0.1.0"
  description: ""
  author: ""
  license: "MIT"
  type: "lib"            # optional, "lib" for library (produces .a), omitted for binary

build:
  cc: "cc"
  cflags: "-Wall -Wextra -std=c11"
  ldflags: ""            # optional, omitted if empty
  src_dir: "src"         # optional, omitted if "src"
  includes:
    - "src"
  sources:               # optional, explicit source file list (overrides auto-discovery)
    - "src/foo.c"

dependencies:
  libname:
    git: "https://github.com/user/repo.git"
    version: "v1.0"     # optional git tag/branch
  locallib:
    path: "libs/locallib"  # local path dependency (skips git ops)

plugins:                 # optional, custom transpilers/code generators
  lex:
    ext: ".l"
    command: "flex"
  yacc:
    ext: ".y"
    command: "bison -d"

tasks:                   # optional, named shell commands
  demo: "./build/debug/myapp run examples/demo.txt"
  lint: "./build/debug/myapp --lint"
```

## goose.lock Format

TOML-like format with `[[package]]` sections, each containing `name`, `git`, `sha` fields. Auto-generated — records the exact commit SHA for each dependency.

## Key Architecture Details

**Framework/consumer split**: The framework core (`src/*.c`, `src/cmd/*.c`) knows nothing about C specifically. It exposes a `GooseFramework` struct holding tool identity (name, version, config_file, pkg_dir, build_dir, etc.), an 8 KB `custom_data[]` scratch buffer for language-specific config, and 11 callback slots: `on_build`, `on_test`, `on_run`, `on_install`, `on_clean`, `on_transpile`, `on_init_template`, `on_config_defaults`, `on_config_parse`, `on_config_write`, `on_pkg_convert`. Consumers also register extra CLI verbs via `goose_framework_add_command()` (up to 32). `src/cc/` is the reference C consumer; its `goose_c_setup()` binds `c_build`, `c_test`, etc. and stashes a `CConfig {cc, cflags, ldflags}` in `fw->custom_data`. `fw->userdata` is set to `fw` itself in `goose_c_setup` so callbacks can fetch the framework back from the opaque `void *userdata` parameter.

**Command dispatch** (`framework.c`): `goose_main()` matches `argv[1]` against a static `builtins[]` table (new, init, build, run, test, clean, add, remove, update, install, convert, task), then consumer-registered `extra_cmds[]`, then falls back to task-name lookup in `goose.yaml`. All built-in `cmd_*` functions load the config via `config_load(fw->config_file, &cfg, fw)`, call `pkg_fetch_all` with `fw->lock_file`, and delegate to the registered callback — no language-specific code lives in `cmd/`.

**Config parsing** (`config.c`): YAML parsed via libyaml. Known sections: `workspace`, `project`, `build`, `dependencies`, `plugins`, `tasks`. For the `build:` section, only `src_dir` is handled inline; every other key is delegated to `fw->on_config_parse("build", key, val, custom_data, userdata)`. `config_save` writes generic fields then calls `fw->on_config_write(f, custom_data, userdata)` to emit language-specific fields. `config_default` calls `fw->on_config_defaults` last so defaults are filled in after the zeroed state.

**Dependency resolution** (`pkg.c`): `pkg_fetch()` clones git deps with `--depth 1`. Path deps (`dep->path[0]` set) skip all git operations — goose validates the path exists and recurses into the sub-package's `goose.yaml`. If a git package has a `goose.yaml`, transitive deps are fetched recursively. After fetch, `fw->on_pkg_convert(dest, config_file, userdata)` runs — the C consumer uses this to auto-convert `CMakeLists.txt` into `goose.yaml` via `cmake_convert_file()`. Lock-file SHAs are checked on every build: if current HEAD differs from locked SHA, `checkout_sha` fetches origin and checks out the locked revision. `pkg_update_all()` skips path deps.

**Path dependency resolution**: `build_dep_base()` in `build.c` returns `dep->path` for path deps or `{pkg_dir}/{dep->name}` for git deps. All source/include/ldflag/define collection in `src/build.c` and `src/cc/build.c` goes through it.

**C build pipeline** (`src/cc/build.c`): `c_build` reads `CConfig` from `fw->custom_data`, runs `build_transpile()` (plugins → `build/gen/`), collects project sources via `fs_collect_sources(cfg->src_dir)`, appends generated sources, appends package sources (prefers explicit `sources:` list in each package's `goose.yaml`, else recursive `.c` scan of `src/` or package root), builds include flags via `build_include_flags()`, extracts `-D` defines and ldflags from each package's own `CConfig`, then invokes `cc` as a single command. Debug: `-g -DDEBUG` → `build/debug/{name}`. Release: `-O2 -DNDEBUG` → `build/release/{name}`.

**Workspace support**: Config parses/saves `workspace.members` and `project.type`. The current C build callback does **not** walk workspace members or build `type: "lib"` as an archive — only the current project is built. Workspace dispatch is wired at the schema level but not yet implemented in `c_build`.

**Plugin/transpile system**: `build_transpile()` iterates `cfg->plugins[]`, collects files matching each plugin's `ext` from `cfg->src_dir`, and runs the plugin `command` on each (`'{command}' '{file}' > 'build/gen/{stem}.c'`). Generated `.c` files are picked up by the main compile.

**Task system**: Tasks are name→command mappings in `goose.yaml`. `cmd_task` lists (no arg) or runs a specific one. If `goose_main` finds no matching built-in or extra command, it loads the config and checks task names — so `goose demo` runs task `demo` directly.

**CMake converter** (`cmake.c`, ~820 LOC): Hand-rolled recursive-descent parser. Handles `project()`, `set()`, `list(APPEND)`, `option()`, `add_library()`, `add_executable()`, `include_directories()`, `target_include_directories()`, `target_link_libraries()`, `file(GLOB)`, `add_subdirectory()` (recursive), `CHECK_INCLUDE_FILE()`, and `if()/elseif()/else()/endif()` with variable expansion and `$<...>` generator-expression stripping. Called from `cmd_convert` (explicit) and `c_pkg_convert` (auto-invoked after fetching a package without `goose.yaml`).

**Test runner** (`c_test`): Compiles each `.c` in `fw->test_dir` (default `tests/`) as a standalone binary, linking against project sources (excluding `main.c`) + all package sources + both project and package include paths/ldflags. Reports PASS/FAIL per test.

**Library mode**: Everything except `src/main.c` is archived into `libgoose.a`. Consumers `#include <goose/goose.h>` and link against the static library to build their own Cargo-like tool. Typical consumer pattern: `goose_framework_new()` → set identity + callbacks → `goose_main(fw, argc, argv)`. See `src/main.c` for the minimal form and `src/cc/setup.c` for a full callback wiring.

## Code Conventions

- Pure C11, no C++ or external dependencies beyond libyaml (vendored)
- Fixed-size buffers throughout (mostly stack-allocated; `goose_framework_new` is the lone `calloc`)
- Shell commands via `system()` and `popen()` for git/rm; paths quoted with single quotes (paths containing `'` will break)
- All logging through `info()`/`warn()`/`err()` macros with 12-char right-aligned tags; colors suppressed when stdout/stderr isn't a TTY
- Built-in command functions: `int cmd_*(int argc, char **argv, GooseFramework *fw)` — return 0 on success
- Consumer callbacks receive an opaque `void *userdata`; the C consumer sets `fw->userdata = fw` so callbacks can reach `fw->custom_data`
- Config structs passed by pointer, never heap-allocated

## CI/CD

- `.github/workflows/ci.yml`: builds on push/PR to main, runs `make clean && make`, verifies binary and `libgoose.a`
- `.github/workflows/release.yml`: triggered by `VERSION` file changes on main. 5 build targets: linux-amd64, linux-arm64, linux-amd64-static (musl), darwin-amd64, darwin-arm64. Packages .deb (amd64+arm64), .rpm, Arch PKGBUILD. Each tarball includes binary + libgoose.a + headers. Generates install.sh and SHA256 checksums. Creates GitHub release with all assets.
- `install.sh`: curl-based installer that downloads latest release, installs binary + library + headers. Prefers static build on linux amd64. Supports `GOOSE_INSTALL_DIR` for custom prefix.
