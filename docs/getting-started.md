# Getting Started

## Installation

**One-liner:**

```sh
curl -fsSL https://raw.githubusercontent.com/wess/goose/main/install.sh | sh
```

**Build from source:**

```sh
git clone https://github.com/wess/goose.git
cd goose
make
make install  # installs binary, library, and headers to /usr/local
```

To install to a custom location:

```sh
make install PREFIX=~/.local
```

To uninstall:

```sh
make uninstall
```

## Creating Your First Project

```sh
goose new hello
cd hello
```

This creates the following structure:

```
hello/
  goose.yaml
  src/
    main.c
  .gitignore
```

## Building and Running

```sh
goose build        # debug build  -> build/debug/hello
goose build -r     # release build -> build/release/hello
goose run          # build + run (debug)
goose run -r       # build + run (release)
```

## The goose.yaml File

Every goose project has a `goose.yaml` at its root:

```yaml
project:
  name: "hello"
  version: "0.1.0"
  description: "My first goose project"
  author: "Your Name"
  license: "MIT"

build:
  cc: "cc"
  cflags: "-Wall -Wextra -std=c11"
  includes:
    - "src"

dependencies:
```

## Adding a Dependency

Dependencies are git repositories or local paths:

```sh
goose add https://github.com/user/somelib.git
```

This clones the repo into `packages/somelib/`, adds it to `goose.yaml`, and records the exact commit in `goose.lock`.

You can specify a name and version tag:

```sh
goose add https://github.com/user/somelib.git --name mylib --version v1.0.0
```

Then include its headers in your code:

```c
#include <somelib.h>
```

## Running Tests

Create a `tests/` directory with `.c` files. Each test file must have its own `main()`:

```c
// tests/basic.c
#include <assert.h>
#include <stdio.h>

int main(void) {
    assert(1 + 1 == 2);
    printf("passed\n");
    return 0;  // 0 = pass, non-zero = fail
}
```

Run all tests:

```sh
goose test
```

## Cleaning Up

```sh
goose clean  # removes build/ directory
```

## Next Steps

- Read the [Configuration Reference](configuration.md) for all goose.yaml options
- Read [Dependencies](dependencies.md) to learn about package management
- Read [Creating Packages](creating-packages.md) to publish your own library
- Read [Plugins](plugins.md) to use custom transpilers (flex, bison, etc.)
- Read [Using libgoose.a](library.md) to embed goose in your own tools
- See the [examples/](../examples/) directory for working examples
