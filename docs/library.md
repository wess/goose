# Using libgoose.a

Goose ships as both a CLI tool and a static C library. The library exposes all of goose's internals -- config parsing, package management, build orchestration, lock files, and filesystem utilities -- so you can build your own package managers, build tools, or CI integrations in C.

## Installation

Build and install goose from source:

```sh
git clone https://github.com/wess/goose.git
cd goose
make
make install  # PREFIX=/usr/local by default
```

This installs:

| Path | Description |
|------|-------------|
| `PREFIX/bin/goose` | CLI binary |
| `PREFIX/lib/libgoose.a` | Static library |
| `PREFIX/include/goose/goose.h` | Main header |
| `PREFIX/include/goose/headers/*.h` | Internal headers |

To install to a custom location:

```sh
make install PREFIX=~/.local
```

## Getting Started

Include the single umbrella header and link against the static library:

```c
#include <goose/goose.h>
```

Compile with:

```sh
cc myprogram.c -I/usr/local/include -L/usr/local/lib -lgoose -o myprogram
```

If you installed to a custom prefix:

```sh
cc myprogram.c -I~/.local/include -L~/.local/lib -lgoose -o myprogram
```

## API Overview

The library is organized into modules, each with its own header. The umbrella `goose.h` includes all of them.

### Config (`config.h`)

Load, save, and create project configurations.

```c
Config cfg;

// Load from goose.yaml
config_load("goose.yaml", &cfg);

// Access fields
printf("Project: %s v%s\n", cfg.name, cfg.version);
printf("Compiler: %s\n", cfg.cc);
printf("Dependencies: %d\n", cfg.dep_count);

// Create a default config
config_default(&cfg, "myproject");

// Save to disk
config_save("goose.yaml", &cfg);
```

**Key types:**

```c
typedef struct {
    char name[128];
    char git[512];
    char version[64];
    char path[512];       // local path dep (mutually exclusive with git)
} Dependency;

typedef struct {
    char name[128];
    char ext[16];
    char command[256];
} Plugin;

typedef struct {
    char name[128];
    char command[512];
} Task;

typedef struct {
    char name[128];
    char version[64];
    char description[256];
    char author[128];
    char license[64];
    char src_dir[512];
    char cc[64];
    char cflags[256];
    char ldflags[256];
    char includes[32][512];
    int include_count;
    char sources[256][512];
    int source_count;
    Dependency deps[64];
    int dep_count;
    Plugin plugins[16];
    int plugin_count;
    Task tasks[32];
    int task_count;
} Config;
```

**Functions:**

| Function | Description |
|----------|-------------|
| `config_load(path, cfg)` | Parse a goose.yaml file into a Config struct |
| `config_save(path, cfg)` | Write a Config struct to a goose.yaml file |
| `config_default(cfg, name)` | Initialize a Config with sensible defaults |

### Lock File (`lock.h`)

Manage the `goose.lock` file for reproducible dependency pinning.

```c
LockFile lf;

// Load existing lock file
lock_load("goose.lock", &lf);

// Look up a specific package's SHA
const char *sha = lock_find_sha(&lf, "mathlib");
if (sha) {
    printf("mathlib is pinned to %s\n", sha);
}

// Update or add an entry
lock_update_entry(&lf, "mathlib",
    "https://github.com/user/mathlib.git",
    "abc123def456");

// Save
lock_save("goose.lock", &lf);
```

**Functions:**

| Function | Description |
|----------|-------------|
| `lock_load(path, lf)` | Parse a goose.lock file |
| `lock_save(path, lf)` | Write a lock file to disk |
| `lock_find_sha(lf, name)` | Look up a package's pinned commit SHA |
| `lock_update_entry(lf, name, git, sha)` | Add or update a lock entry |

### Package Management (`pkg.h`)

Fetch, remove, and update git-based dependencies.

```c
Config cfg;
config_load("goose.yaml", &cfg);

LockFile lf;
lock_load("goose.lock", &lf);

// Fetch all dependencies (clones missing packages, checks locked SHAs)
pkg_fetch_all(&cfg, &lf);

// Save updated lock file
lock_save("goose.lock", &lf);

// Fetch a single dependency
Dependency dep = {0};
strncpy(dep.name, "mylib", sizeof(dep.name) - 1);
strncpy(dep.git, "https://github.com/user/mylib.git", sizeof(dep.git) - 1);
strncpy(dep.version, "v1.0", sizeof(dep.version) - 1);
pkg_fetch(&dep, "packages", &lf);

// Remove a package
pkg_remove("mylib", "packages");

// Update all packages to latest
pkg_update_all(&cfg, &lf);

// Extract package name from a git URL
char *name = pkg_name_from_git("https://github.com/user/mylib.git");
// name = "mylib"

// Get current commit SHA of a package
char sha[64];
pkg_get_sha("packages/mylib", sha, sizeof(sha));
```

**Functions:**

| Function | Description |
|----------|-------------|
| `pkg_fetch(dep, pkg_dir, lf)` | Clone or update a single dependency |
| `pkg_fetch_all(cfg, lf)` | Fetch all dependencies from config |
| `pkg_remove(name, pkg_dir)` | Delete a package from disk |
| `pkg_update_all(cfg, lf)` | Pull latest for all packages |
| `pkg_name_from_git(url)` | Extract package name from git URL |
| `pkg_get_sha(pkg_path, sha, size)` | Get HEAD commit SHA of a package |

### Build (`build.h`)

Compile projects using the goose build pipeline.

```c
Config cfg;
config_load("goose.yaml", &cfg);

// Run plugin transpilation (flex, bison, etc.)
build_transpile(&cfg);

// Build in debug mode
build_project(&cfg, 0);

// Build in release mode
build_project(&cfg, 1);

// Clean build artifacts
build_clean();
```

**Functions:**

| Function | Description |
|----------|-------------|
| `build_project(cfg, release)` | Full build: collect sources, resolve includes, compile |
| `build_transpile(cfg)` | Run plugin transpilers on matching source files |
| `build_clean()` | Remove the build directory |

### Filesystem (`fs.h`)

Portable filesystem utilities.

```c
// Create directories
fs_mkdir("build");
fs_mkdir("build/output");

// Check existence
if (fs_exists("goose.yaml")) {
    printf("Config found\n");
}

// Remove directory tree
fs_rmrf("build");

// Write a file
fs_write_file("src/main.c", "#include <stdio.h>\nint main(void) { return 0; }\n");

// Collect all .c files recursively
char files[256][512];
int count = 0;
fs_collect_sources("src", files, 256, &count);

for (int i = 0; i < count; i++)
    printf("  %s\n", files[i]);

// Collect files by extension
fs_collect_ext("src", ".h", files, 256, &count);
```

**Functions:**

| Function | Description |
|----------|-------------|
| `fs_mkdir(path)` | Create a directory (and parents) |
| `fs_exists(path)` | Check if a path exists |
| `fs_rmrf(path)` | Recursively delete a directory |
| `fs_write_file(path, content)` | Write string content to a file |
| `fs_collect_sources(dir, files, max, count)` | Recursively find all `.c` files |
| `fs_collect_ext(dir, ext, files, max, count)` | Recursively find files by extension |

### CMake Converter (`cmake.h`)

Convert CMakeLists.txt files to goose.yaml format.

```c
Config cfg;

// Parse CMakeLists.txt into a Config struct
cmake_to_config("CMakeLists.txt", &cfg);
printf("Converted project: %s\n", cfg.name);

// Or convert directly to a file
cmake_convert_file("CMakeLists.txt", "goose.yaml");
```

**Functions:**

| Function | Description |
|----------|-------------|
| `cmake_to_config(path, cfg)` | Parse CMakeLists.txt into a Config struct |
| `cmake_convert_file(cmake_path, yaml_path)` | Convert CMakeLists.txt to a goose.yaml file |

### Logging (`color.h`)

Goose's colored logging macros are available for consistent output:

```c
info("Fetching", "mathlib v1.0");   // green tag, right-aligned
warn("Skipping", "already exists"); // yellow tag
err("failed to open %s", path);     // red "error" tag, writes to stderr
```

Output format matches the goose CLI:

```
    Fetching mathlib v1.0
    Skipping already exists
       error failed to open goose.yaml
```

## Complete Example

A minimal tool that fetches dependencies and builds a project:

```c
// fetchbuild.c
#include <goose/goose.h>

int main(int argc, char **argv) {
    int release = 0;
    if (argc > 1 && strcmp(argv[1], "--release") == 0)
        release = 1;

    Config cfg;
    if (config_load(GOOSE_CONFIG, &cfg) != 0)
        return 1;

    info("Loading", "%s v%s", cfg.name, cfg.version);

    LockFile lf = {0};
    lock_load(GOOSE_LOCK, &lf);

    if (pkg_fetch_all(&cfg, &lf) != 0) {
        err("dependency fetch failed");
        return 1;
    }

    lock_save(GOOSE_LOCK, &lf);

    if (build_project(&cfg, release) != 0)
        return 1;

    info("Done", "build complete");
    return 0;
}
```

Compile:

```sh
cc fetchbuild.c -I/usr/local/include -L/usr/local/lib -lgoose -o fetchbuild
```

Run in any goose project:

```sh
./fetchbuild
./fetchbuild --release
```

## Example: Dependency Auditor

A tool that lists all dependencies and their pinned commits:

```c
// audit.c
#include <goose/goose.h>

int main(void) {
    Config cfg;
    if (config_load(GOOSE_CONFIG, &cfg) != 0)
        return 1;

    LockFile lf = {0};
    lock_load(GOOSE_LOCK, &lf);

    printf("Project: %s v%s\n", cfg.name, cfg.version);
    printf("Dependencies: %d\n\n", cfg.dep_count);

    for (int i = 0; i < cfg.dep_count; i++) {
        const char *sha = lock_find_sha(&lf, cfg.deps[i].name);
        printf("  %-20s %s\n", cfg.deps[i].name, cfg.deps[i].git);

        if (cfg.deps[i].version[0])
            printf("  %-20s version: %s\n", "", cfg.deps[i].version);

        if (sha)
            printf("  %-20s pinned:  %.12s\n", "", sha);
        else
            printf("  %-20s pinned:  (not locked)\n", "");

        printf("\n");
    }

    return 0;
}
```

## Example: Project Scaffolder

A tool that creates new projects with custom templates:

```c
// scaffold.c
#include <goose/goose.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        err("usage: scaffold <name>");
        return 1;
    }

    const char *name = argv[1];

    // Create project directory
    fs_mkdir(name);

    char path[512];
    snprintf(path, sizeof(path), "%s/src", name);
    fs_mkdir(path);

    // Write config
    Config cfg;
    config_default(&cfg, name);
    strncpy(cfg.description, "Generated project", sizeof(cfg.description) - 1);

    snprintf(path, sizeof(path), "%s/goose.yaml", name);
    config_save(path, &cfg);

    // Write main.c with custom template
    snprintf(path, sizeof(path), "%s/src/main.c", name);
    char src[1024];
    snprintf(src, sizeof(src),
        "#include <stdio.h>\n\n"
        "int main(void) {\n"
        "    printf(\"Hello from %s!\\n\");\n"
        "    return 0;\n"
        "}\n", name);
    fs_write_file(path, src);

    // Write .gitignore
    snprintf(path, sizeof(path), "%s/.gitignore", name);
    fs_write_file(path, "build/\npackages/\n");

    info("Created", "%s", name);
    return 0;
}
```

## Constants

The library provides path constants matching the goose CLI conventions:

| Constant | Value | Description |
|----------|-------|-------------|
| `GOOSE_VERSION` | from `VERSION` file | Current goose version string |
| `GOOSE_CONFIG` | `"goose.yaml"` | Default config filename |
| `GOOSE_LOCK` | `"goose.lock"` | Default lock filename |
| `GOOSE_PKG_DIR` | `"packages"` | Default package directory |
| `GOOSE_BUILD` | `"build"` | Default build directory |

## Notes

- All structs use fixed-size stack buffers. There is no heap allocation and nothing to free.
- Functions return `0` on success and `-1` on error.
- Error messages are printed to stderr via the `err()` macro.
- The library links against vendored libyaml, so no external dependencies are needed at runtime.
- Git operations use `system()` and `popen()` internally, so `git` must be available on the PATH.
