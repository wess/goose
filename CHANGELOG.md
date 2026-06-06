# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Library mode (`project.type: "lib"`): `goose build` now compiles the project's
  sources to objects and archives them into `build/<mode>/lib<name>.a` (via
  `ar rcs`, with `ranlib` when available) instead of linking an executable.
- Binaries with a path dependency on a `type: "lib"` package now build that
  dependency as a static archive against its own include paths and link the
  resulting `.a`, rather than recompiling its sources inline.
- Workspace builds (`workspace.members`): `goose build` from a workspace root
  builds every member, ordered topologically by inter-member path dependencies,
  and reports a per-member result plus an overall `N ok, M failed` summary.
- Functional tests covering the new features: a `type: "lib"` project produces a
  `.a`, a binary consuming a path-dep lib builds and runs, and a workspace
  builds all of its members.

### Changed
- Functional test suite (`tests/run.sh`) exercising the CLI end to end: `new`,
  `build`, `run`, `clean`, `--release`, `convert`, path-dependency resolution,
  and the `test` runner against the bundled examples. Runs fully offline.
- `make test` target that builds the binary and runs the functional suite.
- `make fmt` and `make lint` targets backed by a `.clang-format` config.
- CI now builds on both Linux and macOS, runs `make test`, and builds and runs
  the `mathapp` example in addition to verifying the binary and library.
- The `mathapp` example now uses local path dependencies (`../mathlib`,
  `../stringlib`) so it builds, runs, and tests offline out of the box instead
  of pointing at non-existent git repositories.

## [0.1.1]

### Added
- Version is read from the `VERSION` file at build time via
  `-DGOOSE_VERSION_FROM_FILE`.

## [0.1.0]

### Added
- Initial release of goose, a Cargo-inspired build tool and package manager
  for C, built on a generic, language-agnostic framework (`libgoose.a`) with C
  wired up as the first consumer plugin.
- Commands: `new`, `init`, `build`, `run`, `test`, `clean`, `add`, `remove`,
  `update`, `install`, `convert`, and named `task` execution.
- `goose.yaml` project configuration and `goose.lock` reproducible dependency
  pinning.
- Git URL and local path dependencies with transitive resolution.
- CMakeLists.txt to `goose.yaml` converter.
- Vendored libyaml 0.2.5 (no system dependency required).

## Roadmap to 1.0

- Library mode (`type: "lib"`): produce a static archive instead of always
  linking an executable.
- Workspace builds (`workspace.members`): build all members in dependency order
  from the workspace root.
- A package registry and `publish` flow with semantic-version resolution for
  reproducible dependency management.

[Unreleased]: https://github.com/wess/goose/compare/v0.1.1...HEAD
[0.1.1]: https://github.com/wess/goose/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/wess/goose/releases/tag/v0.1.0
