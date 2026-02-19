# Command Reference

## goose new

Create a new project in a new directory.

```sh
goose new <name>
```

Creates `<name>/` with `goose.yaml`, `src/main.c`, and `.gitignore`.

## goose init

Initialize a goose project in the current directory.

```sh
goose init
```

Creates `goose.yaml` and `src/main.c` if they don't exist. The project name is taken from the current directory name.

## goose build

Compile the project.

```sh
goose build              # debug build
goose build --release    # release build (optimized)
goose build -r           # shorthand for --release
```

Output:
- Debug: `build/debug/<name>` (compiled with `-g -DDEBUG`)
- Release: `build/release/<name>` (compiled with `-O2 -DNDEBUG`)

Before compiling, goose fetches any missing dependencies and updates `goose.lock`. If plugins are configured, matching source files are transpiled before compilation (see [Plugins](plugins.md)).

## goose run

Build and immediately execute the project binary.

```sh
goose run                # debug
goose run --release      # release
goose run -- arg1 arg2   # pass arguments to the binary
```

## goose test

Discover and run tests from the `tests/` directory.

```sh
goose test               # debug
goose test --release     # release
```

Each `.c` file in `tests/` is compiled as a separate test binary. Test files get access to all project source files (except `main.c`) and all package sources. A test passes if it exits with code 0.

Output example:

```
     Testing myapp v0.1.0
      PASS math
      FAIL network (exit 1)

     Results 1 passed, 1 failed, 2 total
```

## goose clean

Remove all build artifacts.

```sh
goose clean
```

Deletes the `build/` directory.

## goose add

Add a git dependency to the project.

```sh
goose add <git-url>
goose add <git-url> --name <name>
goose add <git-url> --version <tag>
goose add <git-url> --name <name> --version <tag>
```

The package is cloned into `packages/<name>/`, added to `goose.yaml`, and its commit SHA is recorded in `goose.lock`.

If `--name` is not provided, the name is derived from the git URL (e.g., `https://github.com/user/mylib.git` becomes `mylib`).

## goose remove

Remove a dependency.

```sh
goose remove <name>
```

Deletes the package from `packages/` and removes it from `goose.yaml`.

## goose update

Update all dependencies to their latest versions.

```sh
goose update
```

Pulls the latest changes for each dependency and refreshes `goose.lock`.

## goose install

Build a release binary and install it to the system.

```sh
goose install                       # installs to /usr/local/bin
goose install --prefix ~/.local     # custom prefix
```

This always builds in release mode.

## goose convert

Convert a `CMakeLists.txt` file to `goose.yaml`.

```sh
goose convert                          # convert ./CMakeLists.txt to goose.yaml
goose convert path/to/CMakeLists.txt   # specify input file
goose convert --output pkg.yaml        # specify output file
```

This is useful for importing existing CMake-based C libraries into the goose ecosystem. The converter extracts project name, version, include directories, source directories, and linker flags from common CMake commands.

Supported CMake commands: `project()`, `set()`, `list(APPEND)`, `option()`, `add_library()`, `add_executable()`, `include_directories()`, `target_include_directories()`, `target_link_libraries()`, `file(GLOB)`, `add_subdirectory()`, `CHECK_INCLUDE_FILE()`, and `if()/elseif()/else()/endif()` with variable expansion.

Note: This is a best-effort conversion. Complex CMake projects may require manual adjustment of the generated `goose.yaml`.

## goose task

List or run named tasks defined in `goose.yaml`.

```sh
goose task               # list all available tasks
goose task <name>        # run a named task
goose <name>             # shorthand: unknown commands fall back to task lookup
```

Tasks are simple key-value pairs in `goose.yaml`:

```yaml
tasks:
  demo: "./build/debug/myapp examples/demo.txt"
  lint: "./build/debug/myapp --lint src/"
```

`goose task` with no arguments prints all available tasks and their commands. With a name, it runs the command via `system()` and returns its exit code.

Unknown commands (not a built-in like `build`, `run`, etc.) automatically fall back to task lookup, so `goose demo` is equivalent to `goose task demo`.

## Global Options

```sh
goose --version    # print version
goose -V           # print version (shorthand)
goose --help       # print help
goose -h           # print help (shorthand)
```
