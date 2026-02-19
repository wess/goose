# Goose

A package manager for C, inspired by Rust's Cargo.

Goose handles project scaffolding, dependency management, building, testing, and installation -- all through a single command-line tool and a simple `goose.yaml` configuration file.

## Features

- **Project scaffolding** -- `goose new` creates a ready-to-build project
- **Dependency management** -- Add packages from any git repository or local path
- **Path dependencies** -- Use `path:` for local packages (like Cargo's `path = "..."`)
- **Automatic builds** -- Discovers `.c` files, resolves include paths, compiles and links
- **Debug and release modes** -- `-g` by default, `-O2` with `--release`
- **Lock file** -- `goose.lock` pins exact commits for reproducible builds
- **Transitive dependencies** -- Packages can depend on other packages
- **Task runner** -- Define and run named shell commands from `goose.yaml`
- **Test runner** -- Discovers and runs test files from `tests/`
- **Plugin system** -- Custom transpilers for `.l`, `.y`, or any file extension
- **CMake conversion** -- Auto-converts `CMakeLists.txt` packages to `goose.yaml`
- **Install command** -- Build and install binaries to your system
- **Library mode** -- Use goose as a static library (`libgoose.a`) to build your own tools

## Install

**Homebrew (macOS):**

```sh
brew tap wess/goose
brew install goose
```

**Shell script:**

```sh
curl -fsSL https://raw.githubusercontent.com/wess/goose/main/install.sh | sh
```

Custom install directory:

```sh
GOOSE_INSTALL_DIR=~/.local curl -fsSL https://raw.githubusercontent.com/wess/goose/main/install.sh | sh
```

**asdf:**

```sh
asdf plugin add goose https://github.com/wess/goose.git
asdf install goose latest
asdf global goose latest
```

**Package managers:**

Pre-built packages for each release are available on the [Releases](https://github.com/wess/goose/releases) page, including `.tar.gz`, `.deb`, `.rpm`, and Arch Linux `PKGBUILD`.

**Build from source:**

```sh
git clone https://github.com/wess/goose.git
cd goose
make
make install                  # installs to /usr/local
make install PREFIX=~/.local  # custom prefix
```

## Quick Start

```sh
# Create a new project
goose new myapp
cd myapp

# Build and run
goose run
```

Output:

```
     Created new goose project 'myapp'
    Building myapp v0.1.0 (debug)
   Compiling myapp (debug)
    Finished build/debug/myapp
     Running ./build/debug/myapp

Hello from myapp!
```

## Commands

| Command | Description |
|---------|-------------|
| `goose new <name>` | Create a new project |
| `goose init` | Initialize in current directory |
| `goose build [--release]` | Compile the project |
| `goose run [--release]` | Build and run |
| `goose test [--release]` | Build and run tests |
| `goose clean` | Remove build artifacts |
| `goose add <git-url>` | Add a dependency |
| `goose remove <name>` | Remove a dependency |
| `goose update` | Update all dependencies |
| `goose install [--prefix]` | Install binary to system |
| `goose convert [file]` | Convert CMakeLists.txt to goose.yaml |
| `goose task [name]` | List or run a project task |

## Configuration

Every project has a `goose.yaml`:

```yaml
project:
  name: "myapp"
  version: "0.1.0"
  description: "My application"
  author: "Your Name"
  license: "MIT"

build:
  cc: "cc"
  cflags: "-Wall -Wextra -std=c11"
  ldflags: "-lm"
  includes:
    - "src"
    - "include"
  sources:             # optional: explicit source list (overrides auto-discovery)
    - "src/main.c"
    - "src/util.c"

dependencies:
  mathlib:
    git: "https://github.com/user/mathlib.git"
    version: "v1.0.0"
  locallib:
    path: "libs/locallib"

plugins:
  lex:
    ext: ".l"
    command: "flex"

tasks:
  demo: "./build/debug/myapp examples/demo.txt"
  check: "./build/debug/myapp --lint"
```

### Build Section

| Field | Default | Description |
|-------|---------|-------------|
| `cc` | `cc` | C compiler |
| `cflags` | `-Wall -Wextra -std=c11` | Compiler flags |
| `ldflags` | (none) | Linker flags |
| `src_dir` | `src` | Source directory |
| `includes` | `["src"]` | Include directories (become `-I` flags) |
| `sources` | (auto) | Explicit source file list (optional, overrides auto-discovery) |

### Build Modes

```sh
goose build           # Debug:   -g -DDEBUG    -> build/debug/myapp
goose build --release # Release: -O2 -DNDEBUG  -> build/release/myapp
```

## Dependencies

Dependencies can be git repositories or local paths. Git dependencies are cloned into `packages/` while path dependencies reference a local directory directly.

```sh
# Add a git dependency
goose add https://github.com/user/mylib.git

# Add with a specific version tag
goose add https://github.com/user/mylib.git --version v1.0.0

# Add with a custom name
goose add https://github.com/user/repo.git --name mylib

# Remove a dependency
goose remove mylib

# Update all git dependencies to latest
goose update
```

### Path Dependencies

For local development or monorepo setups, use `path:` instead of `git:`:

```yaml
dependencies:
  mylib:
    path: "libs/mylib"
```

Path dependencies skip all git operations (clone, pull, lock). Goose reads the dependency's `goose.yaml` from the specified path and resolves its transitive dependencies. This is useful for:

- Working on a library and its consumer simultaneously
- Embedding a library as a git submodule
- Monorepo layouts with multiple packages

### Include Paths

Package authors specify their public include paths in their `goose.yaml`:

```yaml
# In the library's goose.yaml
build:
  includes:
    - "include"    # public headers live here
```

Consumers then include headers directly:

```c
#include <mathlib.h>
```

Goose reads each package's `goose.yaml` and adds the appropriate `-I` flags automatically.

### Lock File

`goose.lock` records exact git commit SHAs for reproducible builds:

```
[[package]]
name = "mathlib"
git = "https://github.com/user/mathlib.git"
sha = "a1b2c3d4e5f6..."
```

Commit this file for applications. When present, `goose build` ensures dependencies match the locked commits. Path dependencies are not tracked in the lock file.

### Transitive Dependencies

If a package has its own `goose.yaml` with dependencies, goose fetches them recursively. All git packages land in your project's `packages/` directory. Path dependencies resolve transitive deps from the local path.

## Tasks

Define named shell commands in `goose.yaml` and run them with `goose task`:

```yaml
tasks:
  demo: "./build/debug/myapp run examples/demo.txt"
  lint: "./build/debug/myapp --lint src/"
  bench: "hyperfine './build/release/myapp input.txt'"
```

```sh
# List all tasks
goose task

# Run a specific task
goose task demo

# Shorthand: unknown commands fall back to task lookup
goose demo
```

Tasks are simple key-value pairs where the key is the task name and the value is the shell command to execute.

## Testing

Create `.c` files in a `tests/` directory. Each file is compiled and run independently. A test passes if it exits with code 0.

```c
// tests/basic.c
#include <assert.h>
#include <stdio.h>
#include <mylib.h>

int main(void) {
    assert(my_add(2, 3) == 5);
    printf("passed\n");
    return 0;
}
```

```sh
$ goose test
     Testing myapp v0.1.0
      PASS basic
      PASS advanced

     Results 2 passed, 0 failed, 2 total
```

Test files have access to:
- All project source files (except `main.c`)
- All package source files and headers
- Your project's include paths

## Creating a Package

Any git repository can be a goose package. Add a `goose.yaml` to give consumers the best experience.

Recommended layout:

```
mylib/
  goose.yaml
  include/         # public headers
    mylib.h
  src/             # implementation
    mylib.c
  .gitignore
```

```yaml
project:
  name: "mylib"
  version: "1.0.0"
  description: "What it does"
  license: "MIT"

build:
  includes:
    - "include"

dependencies:
```

Push to GitHub and share:
> Coming soon

```sh
goose add https://github.com/you/mylib.git
```

See the [examples/](examples/) directory for complete working packages.

## Plugins

Goose supports custom transpilers that run before compilation. Define plugins in `goose.yaml` with a file extension and a command:

```yaml
plugins:
  lex:
    ext: ".l"
    command: "flex"
  yacc:
    ext: ".y"
    command: "bison -d"
```

During `goose build`, files matching each extension are found in `src/` and run through the command. Output `.c` files are placed in `build/gen/` and compiled into the final binary.

See [Plugins](docs/plugins.md) for details.

## Project Structure

```
myapp/
  goose.yaml           # project configuration
  goose.lock           # dependency lock file (auto-generated)
  src/                 # your source files
    main.c
  tests/               # test files (optional)
    test_math.c
  build/               # compiled output (auto-generated)
    debug/
      myapp
    release/
      myapp
  packages/            # downloaded git dependencies (auto-generated)
    mathlib/
    stringlib/
  .gitignore
```

## Examples

The `examples/` directory contains:

- **`mathlib/`** -- A math utility library demonstrating the `include/` + `src/` package convention
- **`stringlib/`** -- A string utility library with trim, upper/lower, starts/ends with
- **`mathapp/`** -- An application that depends on both libraries, with tests

## Using Goose as a Library

Goose can also be used as a C library. The build produces `libgoose.a` and installs headers to `include/goose/`.

```c
#include <goose/goose.h>

Config cfg;
config_load("goose.yaml", &cfg);
build_project(&cfg, 0);  // 0 = debug, 1 = release
```

To depend on goose as a library from another goose project:

```yaml
dependencies:
  goose:
    path: "libs/goose"
```

The `make install` target installs the binary, static library, and headers:

```sh
make install PREFIX=~/.local
# installs:
#   ~/.local/bin/goose
#   ~/.local/lib/libgoose.a
#   ~/.local/include/goose/goose.h
#   ~/.local/include/goose/headers/*.h
```

See [Using libgoose.a](docs/library.md) for a full tutorial.

## Documentation

Full documentation is in the [docs/](docs/) directory:

- [Getting Started](docs/getting-started.md)
- [Configuration Reference](docs/configuration.md)
- [Dependencies](docs/dependencies.md)
- [Creating Packages](docs/creating-packages.md)
- [Command Reference](docs/commands.md)
- [Plugins](docs/plugins.md)
- [Using libgoose.a](docs/library.md)

## License

MIT
