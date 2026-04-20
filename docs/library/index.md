# libgoose.a

Goose ships as a CLI **and** a static library. The library is the framework itself — every `cmd_*`, the YAML parser, the git fetcher, the lock-file logic, the CMake converter, the filesystem helpers. The `goose` CLI is just a small `main()` that wires C to the framework.

That means you can use `libgoose.a` for two different things.

## Who this is for

### Tool builders

You want to write a small tool that reads `goose.yaml`, audits dependencies, scaffolds projects, or wraps goose with some project-specific logic. You'll use the module APIs directly — `config_load`, `pkg_fetch_all`, `lock_find_sha`, `fs_collect_sources`, etc.

→ Start with [Quickstart](start.md) and [API Reference](api.md).

### Language plugin authors

You want to build a Cargo-like tool for Rust, Python, Zig, whatever. You'll fill in a `GooseFramework` with callbacks for your language's toolchain and inherit every verb (`new`, `build`, `run`, `test`, `add`, `install`, `clean`, `task`, `convert`) for free.

→ Start with [Framework](framework.md) and read `src/cc/` as the reference implementation.

## Install

Build from source — `make install` drops the binary, static lib, and headers:

```sh
git clone https://github.com/wess/goose.git
cd goose
make
make install                  # /usr/local
make install PREFIX=~/.local  # custom prefix
```

Layout:

| Path | |
|------|---|
| `PREFIX/bin/goose` | CLI binary |
| `PREFIX/lib/libgoose.a` | Static library |
| `PREFIX/include/goose/goose.h` | Umbrella header |
| `PREFIX/include/goose/headers/*.h` | Module headers |

Link with:

```sh
cc myprogram.c -I/usr/local/include -L/usr/local/lib -lgoose -o myprogram
```

`libgoose.a` includes vendored libyaml, so no runtime dependency.

## Architecture at a glance

```
                                  ┌──────────────────────────┐
                                  │  your tool / main()      │
                                  └────────────┬─────────────┘
                                               │
                       ┌───────────────────────▼───────────────────────┐
                       │              libgoose.a                        │
                       │                                                │
                       │  framework.c   ← GooseFramework, goose_main    │
                       │  cmd/*.c       ← generic verbs (build, run…)   │
                       │                                                │
                       │  config.c      ← YAML parse/write              │
                       │  pkg.c  lock.c ← deps + lock file              │
                       │  build.c       ← shared build helpers          │
                       │  fs.c          ← filesystem utilities          │
                       │  cmake.c       ← CMakeLists.txt converter      │
                       │                                                │
                       │  libyaml       ← vendored                      │
                       └────────────────────────────────────────────────┘
                                               ▲
                                               │ callbacks
                                  ┌────────────┴─────────────┐
                                  │  language plugin         │
                                  │  (e.g. src/cc/)          │
                                  └──────────────────────────┘
```

The framework handles everything language-agnostic. Language plugins plug in via callbacks stored on a `GooseFramework`. The standard `goose` binary is just the framework + the C language plugin.

## Contents

- [Quickstart](start.md) — the minimal consumer
- [Framework](framework.md) — `GooseFramework`, the 11 callbacks, lifecycle
- [API Reference](api.md) — `config`, `pkg`, `lock`, `fs`, `build`, `cmake`, `color`
- [Examples](examples.md) — dependency auditor, project scaffolder, custom verb
