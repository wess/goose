# Getting Started

## Install

**Homebrew (macOS):**

```sh
brew tap wess/goose
brew install goose
```

**Shell script (macOS / Linux):**

```sh
curl -fsSL https://raw.githubusercontent.com/wess/goose/main/install.sh | sh
```

Install to a custom location:

```sh
GOOSE_INSTALL_DIR=~/.local curl -fsSL https://raw.githubusercontent.com/wess/goose/main/install.sh | sh
```

**asdf:**

```sh
asdf plugin add goose https://github.com/wess/goose.git
asdf install goose latest
asdf global goose latest
```

**From source:**

```sh
git clone https://github.com/wess/goose.git
cd goose
make
make install                  # → /usr/local
make install PREFIX=~/.local  # custom prefix
```

To remove: `make uninstall`.

## Your first project

```sh
goose new hello
cd hello
```

This creates:

```
hello/
  goose.yaml       ← project config
  src/
    main.c         ← "Hello from hello!" template
  .gitignore       ← ignores build/, packages/, goose.lock
```

## Build and run

```sh
goose build        # debug   → build/debug/hello
goose build -r     # release → build/release/hello
goose run          # build (debug), then execute
goose run -r       # build (release), then execute
goose run -- foo   # pass args to your binary
```

## The goose.yaml file

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

See [Configuration](configuration.md) for every field.

## Adding a dependency

```sh
goose add https://github.com/user/somelib.git
```

This clones the repo into `packages/somelib/`, writes it to `goose.yaml`, and pins the commit in `goose.lock`. Then in your code:

```c
#include <somelib.h>
```

Pin a specific tag or branch:

```sh
goose add https://github.com/user/somelib.git --version v1.0.0
```

More in [Dependencies](dependencies.md).

## Tests

Drop `.c` files into `tests/`. Each file is a standalone program; exit code 0 = pass:

```c
// tests/basic.c
#include <assert.h>

int main(void) {
    assert(1 + 1 == 2);
    return 0;
}
```

```sh
goose test
```

## Clean up

```sh
goose clean   # removes build/
```

## Next

- [Commands](commands.md) — full CLI reference
- [Configuration](configuration.md) — every `goose.yaml` field
- [Dependencies](dependencies.md) — deeper dep management
- [Packages](packages.md) — publish your own library
- [Plugins](plugins.md) — `flex`, `bison`, custom transpilers
