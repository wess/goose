# Dependencies

Goose resolves dependencies from git URLs or local paths. Everything lands in `packages/` (git) or stays in place (path). Commits are pinned in `goose.lock` for reproducibility.

## Adding

```sh
goose add https://github.com/user/mylib.git
```

This:

1. Clones with `--depth 1` into `packages/mylib/`
2. Adds the entry to `goose.yaml`
3. Records the commit SHA in `goose.lock`
4. Recursively fetches any transitive deps from the package's own `goose.yaml`

### Options

```sh
goose add <url> --name mylib           # override the auto-derived name
goose add <url> --version v1.0.0       # pin to a tag or branch
goose add <url> --name mylib --version v1.0.0
```

### URL formats

```sh
goose add https://github.com/user/lib.git     # HTTPS
goose add git@github.com:user/lib.git         # SSH
goose add /path/to/repo                       # local git repo
```

## Removing

```sh
goose remove mylib
```

Deletes `packages/mylib/` and removes the entry from `goose.yaml`.

## Updating

```sh
goose update
```

Runs `git fetch && git pull` for each git dep and rewrites `goose.lock` with new SHAs. Path dependencies are skipped.

## Path dependencies

Edit `goose.yaml` directly:

```yaml
dependencies:
  mylib:
    path: "libs/mylib"
```

Path deps:

- Skip all git operations (clone, pull, lock sync)
- Read the dep's `goose.yaml` from the path and resolve its transitive deps
- Are not recorded in `goose.lock`
- Are skipped by `goose update`

Use path deps for:

- Editing a library and its consumer simultaneously
- Monorepos with several packages
- Embedding a library as a git submodule

## The lock file

```
# goose.lock - auto-generated, do not edit

[[package]]
name = "mylib"
git = "https://github.com/user/mylib.git"
sha = "a1b2c3d4e5f6..."
```

When `goose.lock` exists, every `goose build` verifies each git package is checked out to the locked SHA. If the current HEAD differs, goose fetches origin and checks out the locked revision before building.

**Applications:** commit `goose.lock` so every developer gets bit-identical dependencies.

**Libraries:** generally don't commit — let consumers own their own lock files.

## Transitive dependencies

If a package has its own `goose.yaml` with deps, goose fetches them recursively. Everything lands flat in your project's `packages/`:

```
myapp/
  packages/
    libhttp/    ← direct
    libjson/    ← transitive (required by libhttp)
```

Path deps resolve transitives from the path's own `goose.yaml`.

## Include paths

When your project depends on a package, goose reads the package's `goose.yaml` and adds each entry in its `build.includes` list (prefixed with `packages/<name>/`) to your compile line.

Example — if `mathlib` has:

```yaml
build:
  includes:
    - "include"
```

Your build gets `-Ipackages/mathlib/include`, so you can write:

```c
#include <mathlib.h>
```

If a package has no `goose.yaml`, goose falls back to `src/`, the package root, and `include/` (whichever exist).

## CMake packages

Many C libraries ship `CMakeLists.txt` but no `goose.yaml`. Goose handles this automatically — on fetch, it runs its [CMake converter](commands.md#goose-convert) to produce a `goose.yaml`:

```sh
goose add https://github.com/DaveGamble/cJSON.git
# → auto-generates packages/cJSON/goose.yaml
```

The converter extracts: project name/version, include paths, source lists, linker flags. It's best-effort — complex CMake projects with heavy generator-expression logic or custom functions may need manual tuning afterwards.

For explicit conversion:

```sh
goose convert                       # ./CMakeLists.txt → ./goose.yaml
goose convert path/to/CMakeLists.txt
```

## Source discovery

Inside each package, goose prefers (in order):

1. The package's `goose.yaml` `build.sources:` list, if set.
2. Otherwise, every `.c` file found recursively under `src/`.
3. Otherwise, every `.c` file found recursively under the package root.

All of these are compiled and linked into your binary alongside your own sources.
