# Dependencies

## Adding Dependencies

Add a dependency from any git repository:

```sh
goose add https://github.com/user/mylib.git
```

Goose will:
1. Clone the repository into `packages/mylib/`
2. Add it to the `dependencies` section of `goose.yaml`
3. Record the exact commit SHA in `goose.lock`

### Options

```sh
# Specify a custom name
goose add https://github.com/user/repo.git --name mylib

# Pin to a specific tag or branch
goose add https://github.com/user/repo.git --version v1.0.0
```

### Supported URL Formats

```sh
# HTTPS
goose add https://github.com/user/lib.git

# SSH
goose add git@github.com:user/lib.git

# Local path (useful for development)
goose add /path/to/local/repo
```

## Removing Dependencies

```sh
goose remove mylib
```

This removes the package from `packages/`, removes the entry from `goose.yaml`.

## Updating Dependencies

```sh
goose update
```

This pulls the latest changes for all dependencies and updates `goose.lock` with new commit SHAs.

## The Lock File

`goose.lock` records the exact git commit SHA for each dependency:

```
# goose.lock - auto-generated, do not edit

[[package]]
name = "mylib"
git = "https://github.com/user/mylib.git"
sha = "a1b2c3d4e5f6..."
```

When `goose.lock` exists, `goose build` will ensure each package is checked out to the locked commit. This guarantees reproducible builds.

**For applications:** Commit `goose.lock` to version control so all developers build with identical dependencies.

**For libraries:** Generally don't commit `goose.lock`, so consumers use their own lock file.

## Transitive Dependencies

If a package has its own `goose.yaml` with dependencies, goose fetches those automatically. All transitive dependencies are placed in your project's `packages/` directory.

```
myapp/
  packages/
    libhttp/        <- direct dependency
    libjson/        <- transitive dependency (required by libhttp)
```

## How Include Paths Work

When you depend on a package, goose reads that package's `goose.yaml` to find its `includes` list. Each path is prefixed with `packages/<name>/` and added as a `-I` flag.

For example, if `mathlib` has:

```yaml
build:
  includes:
    - "include"
```

Then your project gets `-Ipackages/mathlib/include` during compilation, allowing:

```c
#include <mathlib.h>
```

If a package has no `goose.yaml`, goose falls back to checking `src/`, the package root, and `include/`.

## CMake Package Support

Many popular C libraries (cJSON, zlib, jansson, etc.) use CMake instead of goose. When you `goose add` a package that has a `CMakeLists.txt` but no `goose.yaml`, goose automatically generates a `goose.yaml` by parsing the CMake configuration.

```sh
# This just works — goose auto-converts CMakeLists.txt
goose add https://github.com/DaveGamble/cJSON.git
```

The converter extracts:
- Project name and version from `project()`
- Include directories from `include_directories()` and `target_include_directories()`
- Linker flags from `target_link_libraries()`
- Source directory layout from `add_library()` and `add_executable()` source lists

For manual conversion, use `goose convert`:

```sh
goose convert CMakeLists.txt
```

Note: The converter handles common CMake patterns but is not a full CMake interpreter. Complex projects with heavy use of generator expressions, custom functions, or conditional logic may need manual adjustment.

## How Source Files Work

Goose automatically discovers all `.c` files in a package's `src/` directory (recursively). If no `src/` directory exists, it scans the package root. These files are compiled and linked into your binary alongside your own source files.
