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
  build_dir: "builds"
  includes:
    - "src"
    - "include"
    - "vendor/headers"

dependencies:
  http:
    git: "https://github.com/user/http.git"
    version: "v2.1.0"
  json:
    git: "https://github.com/user/json.git"
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
| `build_dir` | no | `builds` | Output directory for binaries |
| `includes` | no | `["src"]` | List of include directories (become `-I` flags) |

The `includes` list specifies directories that are added as `-I` flags during compilation. This controls where the compiler searches for `#include` directives.

For packages, goose reads the package's own `goose.yaml` to determine its include paths. This means package authors control exactly which directories consumers can include from.

### dependencies

Each dependency is a named entry with:

| Field | Required | Description |
|-------|----------|-------------|
| `git` | yes | Git repository URL (HTTPS, SSH, or local path) |
| `version` | no | Git tag or branch to checkout |

Dependencies are cloned into the `packages/` directory. All `.c` files found in the dependency's `src/` directory (or root, if no `src/` exists) are compiled and linked into your project.

## Build Modes

Goose supports two build modes:

| Mode | Flag | Compiler Flags | Output |
|------|------|---------------|--------|
| Debug (default) | none | `-g -DDEBUG` | `builds/debug/<name>` |
| Release | `--release` or `-r` | `-O2 -DNDEBUG` | `builds/release/<name>` |

These flags are appended after your `cflags`.

## Generated Files

| File | Purpose | Commit to git? |
|------|---------|----------------|
| `goose.yaml` | Project configuration | Yes |
| `goose.lock` | Pinned dependency commits | Yes (for apps), No (for libraries) |
| `builds/` | Compiled binaries | No |
| `packages/` | Downloaded dependencies | No |
