# Getting Started

## Installation

Clone the repository and build from source:

```sh
git clone https://github.com/youruser/goose.git
cd goose
make
make install  # installs to /usr/local/bin
```

To install to a custom location:

```sh
make install PREFIX=~/.local
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
goose build        # debug build  -> builds/debug/hello
goose build -r     # release build -> builds/release/hello
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

Dependencies are git repositories:

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
goose clean  # removes builds/ directory
```

## Next Steps

- Read the [Configuration Reference](configuration.md) for all goose.yaml options
- Read [Dependencies](dependencies.md) to learn about package management
- Read [Creating Packages](creating-packages.md) to publish your own library
- See the [examples/](../examples/) directory for working examples
