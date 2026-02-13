# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Goose — a Cargo-inspired package manager for C. Uses `goose.yaml` (YAML) for project configuration and git URLs for package sources.

## Project Structure

- `src/` — source files
- `src/headers/` — header files
- `Makefile` — builds the goose binary (requires libyaml)

### Source modules

| File | Purpose |
|------|---------|
| `main.c` | Entry point, CLI dispatch |
| `config.c` | YAML parsing/writing of `goose.yaml` |
| `build.c` | Compilation: collects sources, builds include flags, invokes cc |
| `pkg.c` | Package operations: git clone/pull/remove |
| `fs.c` | Filesystem helpers: mkdir, exists, rmrf, collect .c files |
| `cmd_init.c` | `goose init` and `goose new` commands |
| `cmd_build.c` | `goose build`, `goose run`, `goose clean` commands |
| `cmd_pkg.c` | `goose add` and `goose remove` commands |

## Build

```
make
```

Requires `libyaml` (`brew install libyaml` on macOS).

## Install

```
make install          # installs to /usr/local/bin
make install PREFIX=~/.local  # custom prefix
```

## Commands

```
goose new <name>       # create a new project
goose init             # initialize in current directory
goose build            # compile the project
goose run [args...]    # build and run
goose clean            # remove build artifacts
goose add <git-url>    # add a dependency
goose remove <name>    # remove a dependency
```
