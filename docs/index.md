# Goose Documentation

Goose has two parts, documented separately.

## [Package Manager](packagemanager/index.md)

The `goose` CLI — a Cargo-inspired build tool and package manager for C. Start here if you want to use goose to build C projects.

- [Getting Started](packagemanager/start.md) — your first project in two minutes
- [Commands](packagemanager/commands.md) — every subcommand with options
- [Configuration](packagemanager/configuration.md) — the `goose.yaml` schema
- [Dependencies](packagemanager/dependencies.md) — git deps, path deps, lock files
- [Packages](packagemanager/packages.md) — authoring and publishing libraries
- [Plugins](packagemanager/plugins.md) — flex, bison, and custom transpilers

## [Library](library/index.md)

`libgoose.a` — the framework core, language-agnostic. Start here if you want to embed goose in your own tool or build a Cargo-like manager for a different language.

- [Overview](library/index.md) — what ships, when to use it
- [Getting Started](library/start.md) — a minimal working consumer
- [Framework](library/framework.md) — `GooseFramework`, callbacks, lifecycle
- [API Reference](library/api.md) — config, pkg, lock, fs, build, cmake modules
- [Examples](library/examples.md) — dependency auditor, project scaffolder, custom verb

## Examples

Working code lives in [`examples/`](../examples/):

- `examples/mathlib/` — library package with `include/` + `src/` split
- `examples/stringlib/` — another library package
- `examples/mathapp/` — app consuming both libraries, with tests
- `examples/gdexample/` — app depending on a CMake-based library (libgd)
