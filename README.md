# Goose

A package manager for C, inspired by Rust's Cargo.

Goose handles project scaffolding, dependency management, building, testing, and installation -- all through a single command-line tool and a simple `goose.yaml` configuration file.

## Features

- **Project scaffolding** -- `goose new` creates a ready-to-build project
- **Dependency management** -- Add packages from any git repository
- **Automatic builds** -- Discovers `.c` files, resolves include paths, compiles and links
- **Debug and release modes** -- `-g` by default, `-O2` with `--release`
- **Lock file** -- `goose.lock` pins exact commits for reproducible builds
- **Transitive dependencies** -- Packages can depend on other packages
- **Test runner** -- Discovers and runs test files from `tests/`
- **Install command** -- Build and install binaries to your system

## Install

**One-liner:**

```sh
curl -fsSL https://raw.githubusercontent.com/wess/goose/main/install.sh | sh
```

**Custom install directory:**

```sh
GOOSE_INSTALL_DIR=~/.local/bin curl -fsSL https://raw.githubusercontent.com/wess/goose/main/install.sh | sh
```

**Manual download:**

Pre-built binaries for each release are available on the [Releases](https://github.com/wess/goose/releases) page. Download the tarball for your platform, extract it, and place the `goose` binary on your `PATH`.

**Build from source:**

```sh
git clone https://github.com/wess/goose.git
cd goose
make
make install                  # /usr/local/bin/goose
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
    Finished builds/debug/myapp
     Running ./builds/debug/myapp

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

dependencies:
  mathlib:
    git: "https://github.com/user/mathlib.git"
    version: "v1.0.0"
```

### Build Section

| Field | Default | Description |
|-------|---------|-------------|
| `cc` | `cc` | C compiler |
| `cflags` | `-Wall -Wextra -std=c11` | Compiler flags |
| `ldflags` | (none) | Linker flags |
| `src_dir` | `src` | Source directory |
| `build_dir` | `builds` | Output directory |
| `includes` | `["src"]` | Include directories (become `-I` flags) |

### Build Modes

```sh
goose build           # Debug:   -g -DDEBUG    -> builds/debug/myapp
goose build --release # Release: -O2 -DNDEBUG  -> builds/release/myapp
```

## Dependencies

Dependencies are git repositories. Goose clones them into `packages/` and automatically compiles their `.c` files and sets up include paths.

```sh
# Add a dependency
goose add https://github.com/user/mylib.git

# Add with a specific version tag
goose add https://github.com/user/mylib.git --version v1.0.0

# Add with a custom name
goose add https://github.com/user/repo.git --name mylib

# Remove a dependency
goose remove mylib

# Update all to latest
goose update
```

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

Commit this file for applications. When present, `goose build` ensures dependencies match the locked commits.

### Transitive Dependencies

If a package has its own `goose.yaml` with dependencies, goose fetches them recursively. All packages land in your project's `packages/` directory.

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

```sh
goose add https://github.com/you/mylib.git
```

See the [examples/](examples/) directory for complete working packages.

## Project Structure

```
myapp/
  goose.yaml           # project configuration
  goose.lock           # dependency lock file (auto-generated)
  src/                 # your source files
    main.c
  tests/               # test files (optional)
    test_math.c
  builds/              # compiled binaries (auto-generated)
    debug/
      myapp
    release/
      myapp
  packages/            # downloaded dependencies (auto-generated)
    mathlib/
    stringlib/
  .gitignore
```

## Examples

The `examples/` directory contains:

- **`mathlib/`** -- A math utility library demonstrating the `include/` + `src/` package convention
- **`stringlib/`** -- A string utility library with trim, upper/lower, starts/ends with
- **`mathapp/`** -- An application that depends on both libraries, with tests

## Documentation

Full documentation is in the [docs/](docs/) directory:

- [Getting Started](docs/getting-started.md)
- [Configuration Reference](docs/configuration.md)
- [Dependencies](docs/dependencies.md)
- [Creating Packages](docs/creating-packages.md)
- [Command Reference](docs/commands.md)

## License

MIT
