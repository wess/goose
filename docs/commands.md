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
- Debug: `builds/debug/<name>` (compiled with `-g -DDEBUG`)
- Release: `builds/release/<name>` (compiled with `-O2 -DNDEBUG`)

Before compiling, goose fetches any missing dependencies and updates `goose.lock`.

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

Deletes the `builds/` directory.

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

Supported CMake commands: `project()`, `set()`, `add_library()`, `add_executable()`, `include_directories()`, `target_include_directories()`, `target_link_libraries()`, `file(GLOB ...)`, `add_subdirectory()`, and `find_package()`.

Note: This is a best-effort conversion. Complex CMake projects may require manual adjustment of the generated `goose.yaml`.

## Global Options

```sh
goose --version    # print version
goose -V           # print version (shorthand)
goose --help       # print help
goose -h           # print help (shorthand)
```
