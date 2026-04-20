# Authoring Packages

Any git repository with C source files can be a goose package. Adding a `goose.yaml` gives consumers the best experience.

## Layout

```
mylib/
  goose.yaml
  include/          ← public headers (consumers include from here)
    mylib.h
  src/              ← implementation
    mylib.c
    internal.h      ← private header
  tests/            ← optional
    test_mylib.c
  .gitignore
```

## goose.yaml for a library

```yaml
project:
  name: "mylib"
  version: "1.0.0"
  description: "What this library does"
  author: "Your Name"
  license: "MIT"

build:
  cc: "cc"
  cflags: "-Wall -Wextra -std=c11"
  includes:
    - "include"

dependencies:
```

The `includes` list defines your public API surface. Only directories listed here become `-I` flags in the consumer's build, so anything not under `include/` is invisible to them.

## Splitting public and private headers

Common pattern:

- `include/` — public headers
- `src/` — implementation files and private headers

Your public `goose.yaml` lists only `include/`. Your own `.c` files use relative includes for private headers:

```c
// src/mylib.c
#include <mylib.h>       // public  → via -Iinclude
#include "internal.h"    // private → relative to src/
```

If your own implementation needs to include from `include/` (normal), add both directories:

```yaml
build:
  includes:
    - "include"
    - "src"
```

## Transitive dependencies

Libraries can declare their own dependencies:

```yaml
dependencies:
  util:
    git: "https://github.com/user/util.git"
    version: "v1.0.0"
```

When a consumer adds your library, goose fetches these transitives into the consumer's `packages/` directory and links them in.

## Publishing

Goose uses git URLs directly — no registry.

1. Push the repository to GitHub (or any git host).
2. Tag releases: `git tag v1.0.0 && git push --tags`.
3. Share: `goose add https://github.com/you/mylib.git`.

Consumers pin to a tag:

```sh
goose add https://github.com/you/mylib.git --version v1.0.0
```

## Testing your package

Add tests in a `tests/` directory. Each file is a standalone program:

```c
// tests/test_mylib.c
#include <assert.h>
#include <mylib.h>

int main(void) {
    assert(my_function() == expected);
    return 0;    // 0 = pass
}
```

Run with `goose test`.

## Locking in a library repo

Generally, **don't** commit `goose.lock` in a library. Let consumers control their own pinning. If you want reproducible CI for your library's own tests, add `goose.lock` to `.gitignore` and let each CI run regenerate it.

## Don't commit

- `build/` — build output
- `packages/` — cloned deps
- (optional) `goose.lock` — for libraries

A default `goose new` creates a `.gitignore` covering all three.

## Examples

Complete working packages live in [`examples/`](../../examples/):

- `examples/mathlib/` — `include/` + `src/` split
- `examples/stringlib/` — trim, upper/lower, starts/ends with
- `examples/mathapp/` — app consuming both, with tests
