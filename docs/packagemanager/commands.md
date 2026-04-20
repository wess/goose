# Commands

Every `goose` subcommand, with options and behavior.

## Project setup

### `goose new <name>`

Create a new project directory.

```sh
goose new myapp
```

Creates `myapp/` with `goose.yaml`, `src/main.c`, and `.gitignore`.

### `goose init`

Initialize a goose project in the current directory.

```sh
goose init
```

Writes `goose.yaml` and `src/main.c` if they don't exist. The project name is taken from the directory name.

## Building

### `goose build`

Compile the project.

```sh
goose build           # debug
goose build --release # release (optimized)
goose build -r        # shorthand
```

| Mode | Flags | Output |
|------|-------|--------|
| Debug | `-g -DDEBUG` | `build/debug/<name>` |
| Release | `-O2 -DNDEBUG` | `build/release/<name>` |

Before compiling, goose fetches missing dependencies and syncs `goose.lock`. If plugins are configured, matching sources are transpiled to `build/gen/` first — see [Plugins](plugins.md).

### `goose run`

Build, then execute the binary.

```sh
goose run              # debug
goose run --release    # release
goose run -- arg1 arg2 # pass args to your binary
```

### `goose test`

Compile and run every `.c` file in `tests/` as a standalone binary. Tests have access to all project sources (except `main.c`) and all package sources. Exit code 0 = pass.

```sh
goose test             # debug
goose test --release   # release
```

Output:

```
     Testing myapp v0.1.0
      PASS math
      FAIL network (exit 1)

     Results 1 passed, 1 failed, 2 total
```

### `goose clean`

Delete `build/`.

```sh
goose clean
```

## Dependencies

### `goose add <git-url>`

Add a git dependency.

```sh
goose add <git-url>
goose add <git-url> --name <name>
goose add <git-url> --version <tag-or-branch>
```

Clones into `packages/<name>/`, writes the entry to `goose.yaml`, and pins the commit SHA in `goose.lock`. If `--name` is omitted, it's derived from the URL (e.g. `https://github.com/user/mylib.git` → `mylib`).

### `goose remove <name>`

Remove a dependency.

```sh
goose remove mylib
```

Deletes from `packages/` and removes the entry from `goose.yaml`.

### `goose update`

Pull the latest commits for every git dependency and refresh `goose.lock`. Path dependencies are skipped.

```sh
goose update
```

## Install

### `goose install`

Build release, then install the binary.

```sh
goose install                   # → /usr/local/bin
goose install --prefix ~/.local # custom prefix
```

Always builds in release mode.

## Interop

### `goose convert`

Convert a `CMakeLists.txt` to `goose.yaml`.

```sh
goose convert                          # ./CMakeLists.txt → ./goose.yaml
goose convert path/to/CMakeLists.txt   # explicit input
goose convert --input A --output B     # explicit both
```

Extracts project name/version, include paths, source lists, and linker flags. Handles: `project()`, `set()`, `list(APPEND)`, `option()`, `add_library()`, `add_executable()`, `include_directories()`, `target_include_directories()`, `target_link_libraries()`, `file(GLOB)`, `add_subdirectory()` (recursive), `CHECK_INCLUDE_FILE()`, `if()/elseif()/else()/endif()` with variable expansion and generator-expression stripping.

Best-effort. Complex CMake projects may need manual tuning afterwards.

## Tasks

### `goose task [name]`

List or run named tasks defined in `goose.yaml`.

```sh
goose task         # list all tasks
goose task demo    # run task 'demo'
goose demo         # shorthand: unknown commands fall back to task lookup
```

Tasks are plain key/value pairs:

```yaml
tasks:
  demo: "./build/debug/myapp examples/demo.txt"
  lint: "./build/debug/myapp --lint src/"
```

The command runs via `system()`; its exit code becomes goose's exit code. Up to 32 tasks per project (`MAX_TASKS`).

## Global

```sh
goose --version   # -V
goose --help      # -h
```
