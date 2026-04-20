# Configuration

All project settings live in `goose.yaml` at the project root.

## Full example

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

## `project`

| Field | Required | Default | Description |
|-------|----------|---------|-------------|
| `name` | yes | directory name | Project name — becomes the binary name |
| `version` | yes | `0.1.0` | Semantic version |
| `description` | no | `""` | Short description |
| `author` | no | `""` | Author name or email |
| `license` | no | `MIT` | License identifier |
| `type` | no | — | `"lib"` marks this as a library; omit for a binary |

## `build`

| Field | Required | Default | Description |
|-------|----------|---------|-------------|
| `cc` | no | `cc` | C compiler command |
| `cflags` | no | `-Wall -Wextra -std=c11` | Compiler flags |
| `ldflags` | no | `""` | Linker flags (e.g. `-lm`) |
| `src_dir` | no | `src` | Root directory scanned for `.c` files |
| `includes` | no | `["src"]` | Directories added as `-I` flags |
| `sources` | no | (auto-discover) | Explicit source list — overrides auto-discovery |

**Includes.** Package consumers automatically get each package's own `includes` as `-I` flags. Library authors: the entries here are your public API surface — only files under these directories are reachable from `#include <...>`.

**Sources.** When omitted, goose recursively discovers all `.c` files under `src_dir`. When present, only the listed files are compiled. Useful for packages that vendor third-party code they don't want swept in.

## `dependencies`

Each entry is `name: { git, version? }` or `name: { path }`:

| Field | Description |
|-------|-------------|
| `git` | Git repository URL (HTTPS, SSH, or local) |
| `version` | Optional git tag or branch — git deps only |
| `path` | Local filesystem path — skips all git operations |

Exactly one of `git` or `path` per dependency. See [Dependencies](dependencies.md).

## `plugins`

Each plugin is `name: { ext, command }`:

| Field | Description |
|-------|-------------|
| `ext` | File extension to match (include the dot: `.l`, `.y`) |
| `command` | Shell command invoked per matching file |

Before compilation, each matching file is run through the command; output `.c` lands in `build/gen/` and joins the main build. See [Plugins](plugins.md). Up to 16 plugins (`MAX_PLUGINS`).

## `tasks`

```yaml
tasks:
  demo: "./build/debug/myapp examples/demo.txt"
```

Run with `goose task demo` or the shorthand `goose demo`. Up to 32 tasks (`MAX_TASKS`).

## `workspace`

A workspace root declares member projects:

```yaml
workspace:
  members:
    - "mylib"
    - "myapp"
```

Currently: workspace metadata is parsed and round-trips through `config_save`, but the built-in `goose build` does not yet walk members. Use from the library API (`libgoose.a`) if you need workspace orchestration today.

## Build modes

| Mode | Flag | Flags appended | Output |
|------|------|----------------|--------|
| Debug (default) | — | `-g -DDEBUG` | `build/debug/<name>` |
| Release | `--release` / `-r` | `-O2 -DNDEBUG` | `build/release/<name>` |

Mode flags are appended **after** your `cflags`.

## Generated files

| File | Purpose | Commit? |
|------|---------|---------|
| `goose.yaml` | Project configuration | Yes |
| `goose.lock` | Pinned dep commits | Apps: yes. Libraries: no |
| `build/` | Compiled output | No |
| `packages/` | Cloned git deps | No |

## Limits

Fixed-size buffers. If you need to exceed them, bump the limit in `src/headers/config.h` and rebuild.

| Constant | Limit |
|----------|-------|
| `MAX_DEPS` | 64 |
| `MAX_SRC_FILES` | 256 |
| `MAX_INCLUDES` | 32 |
| `MAX_PLUGINS` | 16 |
| `MAX_TASKS` | 32 |
| `MAX_WS_MEMBERS` | 32 |
