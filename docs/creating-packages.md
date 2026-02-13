# Creating Packages

Any git repository with C source files can be a goose package. Adding a `goose.yaml` gives consumers the best experience.

## Package Structure

Recommended layout:

```
mylib/
  goose.yaml
  include/           <- public headers (consumers include from here)
    mylib.h
  src/               <- implementation files
    mylib.c
    internal.h       <- private headers
  tests/             <- optional tests
    test_mylib.c
  .gitignore
```

## goose.yaml for Libraries

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

The `includes` list is critical for libraries. It tells consumers which directories contain your public headers. Only paths listed here become `-I` flags in the consumer's build.

### Separating Public and Private Headers

A common pattern:

- `include/` - Public API headers that consumers use
- `src/` - Implementation files and private headers

In your `goose.yaml`, only list `include/` in `includes`. Your own `.c` files can use relative includes for private headers:

```c
// src/mylib.c
#include <mylib.h>       // public header (via -Iinclude)
#include "internal.h"    // private header (relative to src/)
```

If your `.c` files need to include from `include/` too, add both:

```yaml
build:
  includes:
    - "include"
    - "src"
```

## Publishing

Currently, goose uses git URLs directly. To make your package available:

1. Push your repository to GitHub (or any git host)
2. Tag releases: `git tag v1.0.0 && git push --tags`
3. Share the URL: `goose add https://github.com/you/mylib.git`

Users can pin to a tag:

```sh
goose add https://github.com/you/mylib.git --version v1.0.0
```

## Dependencies in Libraries

Libraries can have their own dependencies in `goose.yaml`. When a consumer adds your library, goose fetches transitive dependencies automatically.

## Testing Your Package

Add tests in a `tests/` directory:

```c
// tests/test_mylib.c
#include <assert.h>
#include <mylib.h>

int main(void) {
    // test your library
    assert(my_function() == expected);
    return 0;  // 0 = pass
}
```

Run with `goose test`.

## Example

See the `examples/` directory in the goose repository:

- `examples/mathlib/` - A math utility library with `include/` and `src/` separation
- `examples/stringlib/` - A string utility library
- `examples/mathapp/` - An application consuming both libraries
