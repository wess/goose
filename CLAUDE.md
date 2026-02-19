# CLAUDE.md

## Project Overview

Goose — a Cargo-inspired package manager and build tool for C. Uses `goose.yaml` for project configuration, `goose.lock` for reproducible dependency pinning, git URLs or local paths for package sources, and named tasks for shell commands. Also builds as a static library (`libgoose.a`) for embedding into other tools. Version is read from the `VERSION` file at build time.

## Project Structure

```
src/
  main.c              — entry point, CLI command dispatch table
  config.c            — YAML parsing/writing of goose.yaml (uses libyaml)
  build.c             — compilation: source collection, plugin transpilation, include/define/ldflag aggregation, cc invocation
  pkg.c               — package ops: git clone/pull/remove, transitive dep resolution, lock sync
  fs.c                — filesystem helpers: mkdir, exists, rmrf, write_file, recursive .c/.ext collection
  lock.c              — goose.lock read/write/lookup (TOML-like [[package]] format)
  cmake.c             — CMakeLists.txt to goose.yaml converter (variable table, if/else, subdirectories)
  cmd/
    init.c            — goose init, goose new
    build.c           — goose build, goose run, goose clean, goose test, goose install
    pkg.c             — goose add, goose remove, goose update
    convert.c         — goose convert (CMake to goose.yaml)
    task.c            — goose task (list/run named tasks from goose.yaml)
  headers/
    main.h            — version macro, path constants (GOOSE_CONFIG, GOOSE_LOCK, GOOSE_PKG_DIR, GOOSE_BUILD)
    config.h          — Config/Dependency/Task/Plugin structs, limits (MAX_DEPS=64, MAX_SRC_FILES=256, MAX_INCLUDES=32, MAX_PLUGINS=16, MAX_TASKS=32)
    build.h           — build_project(), build_transpile(), build_clean()
    pkg.h             — pkg_fetch(), pkg_remove(), pkg_fetch_all(), pkg_update_all(), pkg_name_from_git(), pkg_get_sha()
    fs.h              — fs_mkdir(), fs_exists(), fs_rmrf(), fs_write_file(), fs_collect_sources(), fs_collect_ext()
    lock.h            — LockFile/LockEntry structs, lock_load/save/find_sha/update_entry
    cmake.h           — cmake_to_config(), cmake_convert_file()
    color.h           — ANSI color macros, info()/warn()/err() logging macros, TTY detection
    cmd.h             — all cmd_* function declarations
include/
  goose.h             — public library header, re-exports all internal headers for libgoose.a consumers
libs/libyaml/         — vendored libyaml 0.2.5 (compiled from source, no system dependency needed)
examples/             — example projects (mathapp, mathlib, stringlib, gdexample)
docs/                 — user-facing documentation (getting-started, commands, configuration, dependencies, creating-packages)
```

## Build

```
make                  # builds both build/libgoose.a and build/goose
make lib              # builds only build/libgoose.a (static library)
make cli              # builds only build/goose (CLI binary)
make clean            # removes build/
```

libyaml is vendored in `libs/libyaml/` and compiled directly — no system install required. The Makefile compiles all `src/*.c` and `src/cmd/*.c` with `-std=c11 -Wall -Wextra`. Version is injected via `-DGOOSE_VERSION_FROM_FILE` from the `VERSION` file.

The CLI binary links against `libgoose.a`. Object files go to `build/obj/` and `build/obj/cmd/`.

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
project:
  name: "myapp"
  version: "0.1.0"
  description: ""
  author: ""
  license: "MIT"

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

**Dependency resolution**: `pkg_fetch()` clones git deps with `--depth 1`. Path deps (`dep->path[0]` set) skip all git operations — goose validates the path exists and resolves transitive deps from the local `goose.yaml`. If a git package has a `goose.yaml` with deps, it recursively fetches transitive dependencies. If a package only has `CMakeLists.txt`, it auto-converts to `goose.yaml` via `cmake_convert_file()`. Lock file SHAs are checked on every build — if current HEAD differs from locked SHA, it checks out the locked revision. `pkg_update_all()` skips path deps.

**Path dependency resolution**: `dep_base()` helper in `build.c` returns `dep->path` for path deps or `GOOSE_PKG_DIR/dep->name` for git deps. All source/include/ldflag/define collection functions use `dep_base()` to resolve the correct base directory.

**Build pipeline**: `build_project()` runs `build_transpile()` first (processes plugins), then collects project sources from `src_dir`, generated sources from `build/gen/`, and package sources (prefers explicit `sources` list from package `goose.yaml`, falls back to recursive `.c` collection). Assembles include flags from both project and package configs, collects `-D` defines and ldflags from package cflags, then invokes `cc` as a single compilation command. Debug builds use `-g -DDEBUG`, release uses `-O2 -DNDEBUG`. Output goes to `build/debug/` or `build/release/`.

**Plugin/transpile system**: `build_transpile()` iterates plugins defined in `goose.yaml`, collects files matching each plugin's `ext`, and runs the plugin `command` on each file, writing output to `build/gen/`. Generated `.c` files are included in the main compilation.

**Task system**: Tasks are simple name-to-command mappings in `goose.yaml`. `cmd_task()` lists or runs them. Unknown commands in `main()` fall back to task name lookup before erroring.

**CMake converter** (`cmake.c`): A substantial (~820 line) recursive-descent parser that handles `project()`, `set()`, `list(APPEND)`, `option()`, `add_library()`, `add_executable()`, `include_directories()`, `target_include_directories()`, `target_link_libraries()`, `file(GLOB)`, `add_subdirectory()` (recursive), `CHECK_INCLUDE_FILE()`, and `if()/elseif()/else()/endif()` with variable expansion and generator expression stripping.

**Test runner**: Compiles each `.c` file in `tests/` as a standalone binary, linking against project sources (minus `main.c`) and package sources. Reports PASS/FAIL per test file.

**Library mode**: All source files (except `main.c`) are compiled into `libgoose.a`. Users can `#include <goose/goose.h>` and link against the static library to call `config_load()`, `pkg_fetch()`, `build_project()`, etc. directly.

## Code Conventions

- Pure C11, no C++ or external dependencies beyond libyaml (vendored)
- Fixed-size buffers throughout (no malloc), stack-allocated structs
- Shell commands via `system()` and `popen()` for git operations and rm
- All logging through `info()`/`warn()`/`err()` macros with 12-char right-aligned tags
- Command functions follow `int cmd_*(int argc, char **argv)` signature, return 0 on success
- Config structs passed by pointer, never heap-allocated

## CI/CD

- `.github/workflows/ci.yml`: builds on push/PR to main, runs `make clean && make`, verifies binary and `libgoose.a`
- `.github/workflows/release.yml`: triggered by `VERSION` file changes on main. 5 build targets: linux-amd64, linux-arm64, linux-amd64-static (musl), darwin-amd64, darwin-arm64. Packages .deb (amd64+arm64), .rpm, Arch PKGBUILD. Each tarball includes binary + libgoose.a + headers. Generates install.sh and SHA256 checksums. Creates GitHub release with all assets.
- `install.sh`: curl-based installer that downloads latest release, installs binary + library + headers. Prefers static build on linux amd64. Supports `GOOSE_INSTALL_DIR` for custom prefix.
