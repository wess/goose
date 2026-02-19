# Configuration Reference

All configuration lives in `goose.yaml` at the project root.

## Full Example

```yaml
project:
  name: "myapp"
  version: "1.2.0"
  description: "A web server written in C"
  author: "Jane Doe"
  license: "MIT"

build:
  cc: "gcc"
  cflags: "-Wall -Wextra -std=c11 -pedantic"
  ldflags: "-lm -lpthread"
  src_dir: "src"
  includes:
    - "src"
    - "include"
    - "vendor/headers"
  sources:
    - "src/main.c"
    - "src/server.c"

dependencies:
  http:
    git: "https://github.com/user/http.git"
    version: "v2.1.0"
  json:
    git: "https://github.com/user/json.git"
  locallib:
    path: "libs/locallib"

plugins:
  lex:
    ext: ".l"
    command: "flex"
  yacc:
    ext: ".y"
    command: "bison -d"

tasks:
  demo: "./build/debug/myapp examples/demo.txt"
  lint: "./build/debug/myapp --lint src/"
```

## Sections

### project

| Field | Required | Default | Description |
|-------|----------|---------|-------------|
| `name` | yes | directory name | Project name, used as the binary name |
| `version` | yes | `0.1.0` | Semantic version string |
| `description` | no | `""` | Short project description |
| `author` | no | `""` | Author name or email |
| `license` | no | `MIT` | License identifier |

### build

| Field | Required | Default | Description |
|-------|----------|---------|-------------|
| `cc` | no | `cc` | C compiler command |
| `cflags` | no | `-Wall -Wextra -std=c11` | Compiler flags |
| `ldflags` | no | `""` | Linker flags (e.g., `-lm`) |
| `src_dir` | no | `src` | Directory containing source files |
| `includes` | no | `["src"]` | List of include directories (become `-I` flags) |
| `sources` | no | (auto-discover) | Explicit list of source files to compile |

The `includes` list specifies directories that are added as `-I` flags during compilation. This controls where the compiler searches for `#include` directives.

For packages, goose reads the package's own `goose.yaml` to determine its include paths. This means package authors control exactly which directories consumers can include from.

The `sources` list is optional. When omitted, goose recursively discovers all `.c` files in `src_dir`. When specified, only the listed files are compiled. This is primarily used by the CMake converter and by library packages that need precise control over which files are built.

### dependencies

Each dependency is a named entry with either `git` or `path`:

| Field | Required | Description |
|-------|----------|-------------|
| `git` | yes* | Git repository URL (HTTPS, SSH, or local path) |
| `version` | no | Git tag or branch to checkout (git deps only) |
| `path` | yes* | Local filesystem path to the dependency |

*One of `git` or `path` is required per dependency.

Git dependencies are cloned into the `packages/` directory. Path dependencies reference a local directory directly and skip all git operations (clone, pull, lock). All `.c` files found in the dependency's `src/` directory (or root, if no `src/` exists) are compiled and linked into your project.

### plugins

Each plugin is a named entry with:

| Field | Required | Description |
|-------|----------|-------------|
| `ext` | yes | File extension to match (e.g., `.l`, `.y`) |
| `command` | yes | Shell command to run on matching files |

Before compilation, goose scans `src_dir` for files matching each plugin's extension. Each file is processed with the command, and the generated `.c` output is placed in `build/gen/` for inclusion in the build.

```yaml
plugins:
  lex:
    ext: ".l"
    command: "flex"
  yacc:
    ext: ".y"
    command: "bison -d"
```

See [Plugins](plugins.md) for a full guide.

### tasks

Tasks are simple name-to-command mappings:

| Field | Type | Description |
|-------|------|-------------|
| (key) | string | Task name |
| (value) | string | Shell command to execute |

```yaml
tasks:
  demo: "./build/debug/myapp examples/demo.txt"
  lint: "./build/debug/myapp --lint src/"
  bench: "hyperfine './build/release/myapp input.txt'"
```

Run with `goose task <name>` or the shorthand `goose <name>`. Up to 32 tasks can be defined (`MAX_TASKS`).

## Build Modes

Goose supports two build modes:

| Mode | Flag | Compiler Flags | Output |
|------|------|---------------|--------|
| Debug (default) | none | `-g -DDEBUG` | `build/debug/<name>` |
| Release | `--release` or `-r` | `-O2 -DNDEBUG` | `build/release/<name>` |

These flags are appended after your `cflags`.

## Generated Files

| File | Purpose | Commit to git? |
|------|---------|----------------|
| `goose.yaml` | Project configuration | Yes |
| `goose.lock` | Pinned dependency commits (git deps only) | Yes (for apps), No (for libraries) |
| `build/` | Compiled binaries | No |
| `packages/` | Downloaded git dependencies | No |
