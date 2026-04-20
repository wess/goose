# Package Manager

`goose` is a Cargo-inspired CLI for C projects. It handles scaffolding, dependency management, compilation, testing, and installation through a single binary and a `goose.yaml` file.

## Contents

- [Getting Started](start.md) — install goose, create your first project, build and run
- [Commands](commands.md) — reference for every subcommand
- [Configuration](configuration.md) — `goose.yaml` schema and defaults
- [Dependencies](dependencies.md) — adding git/path deps, the lock file, CMake packages
- [Packages](packages.md) — publishing your own library for others to consume
- [Plugins](plugins.md) — custom transpilers (`flex`, `bison`, anything)

## What goose gives you

| | |
|---|---|
| **Scaffolding** | `goose new myapp` produces a ready-to-build project |
| **Deps** | `goose add <git-url>` clones, resolves transitives, pins to `goose.lock` |
| **Builds** | auto-discovered sources, `-I` flags resolved from each package's own config |
| **Modes** | debug (`-g -DDEBUG`) or release (`-O2 -DNDEBUG`) |
| **Tests** | every `.c` in `tests/` compiled and run as an independent binary |
| **Tasks** | named shell commands in `goose.yaml`, invoked as `goose <name>` |
| **CMake** | automatic `CMakeLists.txt` → `goose.yaml` conversion on fetch |

## Example

```sh
goose new hello
cd hello
goose add https://github.com/user/mylib.git --version v1.0.0
goose run
```

---

Next: [Getting Started](start.md)
