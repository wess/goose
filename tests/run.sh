#!/usr/bin/env sh
# Functional test suite for goose. Exercises the CLI end-to-end against
# the bundled examples and a scratch workspace. No network access required.
set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GOOSE="$ROOT/build/goose"
WORK="$(mktemp -d 2>/dev/null || mktemp -d -t goosetest)"

PASS=0
FAIL=0

cleanup() {
    rm -rf "$WORK"
    rm -rf "$ROOT/examples/mathapp/build" \
           "$ROOT/examples/mathapp/packages" \
           "$ROOT/examples/mathapp/goose.lock"
}
trap cleanup EXIT INT TERM

ok() {
    PASS=$((PASS + 1))
    printf '  ok   %s\n' "$1"
}

bad() {
    FAIL=$((FAIL + 1))
    printf '  FAIL %s\n' "$1"
}

check() {
    # check <description> <command...>
    desc="$1"
    shift
    if "$@" >/dev/null 2>&1; then
        ok "$desc"
    else
        bad "$desc"
    fi
}

if [ ! -x "$GOOSE" ]; then
    echo "error: $GOOSE not found; run 'make' first" >&2
    exit 1
fi

echo "goose functional tests"
echo "binary: $GOOSE"
echo

# --- version ---
"$GOOSE" --version | grep -q '^goose ' && ok "version reports goose" || bad "version reports goose"

# --- new / run / clean ---
( cd "$WORK" && "$GOOSE" new hello ) >/dev/null 2>&1
check "new creates goose.yaml" test -f "$WORK/hello/goose.yaml"
check "new creates src/main.c" test -f "$WORK/hello/src/main.c"
check "new creates .gitignore" test -f "$WORK/hello/.gitignore"

( cd "$WORK/hello" && "$GOOSE" build ) >/dev/null 2>&1
check "build produces debug binary" test -x "$WORK/hello/build/debug/hello"

RUNOUT="$( cd "$WORK/hello" && "$GOOSE" run 2>/dev/null )"
echo "$RUNOUT" | grep -q "Hello from hello" && ok "run executes the binary" || bad "run executes the binary"

( cd "$WORK/hello" && "$GOOSE" build --release ) >/dev/null 2>&1
check "release build produces release binary" test -x "$WORK/hello/build/release/hello"

( cd "$WORK/hello" && "$GOOSE" clean ) >/dev/null 2>&1
check "clean removes build dir" test ! -d "$WORK/hello/build"

# --- convert (CMake -> goose.yaml) ---
mkdir -p "$WORK/conv"
cat > "$WORK/conv/CMakeLists.txt" <<'CMAKE'
cmake_minimum_required(VERSION 3.10)
project(demo VERSION 1.2.3)
add_executable(demo src/main.c)
target_include_directories(demo PRIVATE include)
CMAKE
( cd "$WORK/conv" && "$GOOSE" convert CMakeLists.txt ) >/dev/null 2>&1
check "convert writes goose.yaml" test -f "$WORK/conv/goose.yaml"
grep -q 'name: "demo"' "$WORK/conv/goose.yaml" && ok "convert carries project name" || bad "convert carries project name"
grep -q 'version: "1.2.3"' "$WORK/conv/goose.yaml" && ok "convert carries version" || bad "convert carries version"

# --- path dependency resolution via the mathapp example ---
APP="$ROOT/examples/mathapp"
rm -rf "$APP/build" "$APP/packages" "$APP/goose.lock"
( cd "$APP" && "$GOOSE" build ) >/dev/null 2>&1
check "path-dep example builds offline" test -x "$APP/build/debug/mathapp"

APPRUN="$( cd "$APP" && "$GOOSE" run 2>/dev/null )"
echo "$APPRUN" | grep -q "2 + 3 = 5" && ok "path-dep example runs" || bad "path-dep example runs"

# --- test runner exercises tests/ against path deps ---
TESTOUT="$( cd "$APP" && "$GOOSE" test 2>&1 )"
echo "$TESTOUT" | grep -q "2 passed, 0 failed" && ok "example test runner reports 2 passed" || bad "example test runner reports 2 passed"

# --- library mode: type "lib" produces a static archive ---
LIB="$WORK/greet"
mkdir -p "$LIB/src" "$LIB/include"
cat > "$LIB/goose.yaml" <<'YAML'
project:
  name: "greet"
  version: "1.0.0"
  type: "lib"

build:
  cc: "cc"
  cflags: "-Wall -Wextra -std=c11"
  includes:
    - "include"

dependencies:
YAML
cat > "$LIB/include/greet.h" <<'H'
#ifndef GREET_H
#define GREET_H
int greet_value(void);
#endif
H
cat > "$LIB/src/greet.c" <<'C'
#include "greet.h"
int greet_value(void) { return 42; }
C
( cd "$LIB" && "$GOOSE" build ) >/dev/null 2>&1
check "lib build produces static archive" test -f "$LIB/build/debug/libgreet.a"

# --- binary consuming a path-dep lib builds and runs ---
CONS="$WORK/greetapp"
mkdir -p "$CONS/src"
cat > "$CONS/goose.yaml" <<'YAML'
project:
  name: "greetapp"
  version: "0.1.0"

build:
  cc: "cc"
  cflags: "-Wall -Wextra -std=c11"
  includes:
    - "src"

dependencies:
  greet:
    path: "../greet"
YAML
cat > "$CONS/src/main.c" <<'C'
#include <stdio.h>
#include <greet.h>
int main(void) {
    printf("greet=%d\n", greet_value());
    return 0;
}
C
( cd "$CONS" && "$GOOSE" build ) >/dev/null 2>&1
check "lib-consumer binary builds" test -x "$CONS/build/debug/greetapp"
CONSRUN="$( cd "$CONS" && "$GOOSE" run 2>/dev/null )"
echo "$CONSRUN" | grep -q "greet=42" && ok "lib-consumer binary runs" || bad "lib-consumer binary runs"

# --- workspace: builds all members in dependency order ---
WS="$WORK/ws"
mkdir -p "$WS/corelib/src" "$WS/corelib/include" "$WS/tool/src"
cat > "$WS/goose.yaml" <<'YAML'
workspace:
  members:
    - "tool"
    - "corelib"

project:
  name: "ws"
  version: "0.1.0"

build:
  cc: "cc"
  cflags: "-Wall -Wextra -std=c11"
  includes:
    - "src"

dependencies:
YAML
cat > "$WS/corelib/goose.yaml" <<'YAML'
project:
  name: "corelib"
  version: "1.0.0"
  type: "lib"

build:
  cc: "cc"
  cflags: "-Wall -Wextra -std=c11"
  includes:
    - "include"

dependencies:
YAML
cat > "$WS/corelib/include/corelib.h" <<'H'
#ifndef CORELIB_H
#define CORELIB_H
int core_answer(void);
#endif
H
cat > "$WS/corelib/src/corelib.c" <<'C'
#include "corelib.h"
int core_answer(void) { return 7; }
C
cat > "$WS/tool/goose.yaml" <<'YAML'
project:
  name: "tool"
  version: "0.1.0"

build:
  cc: "cc"
  cflags: "-Wall -Wextra -std=c11"
  includes:
    - "src"

dependencies:
  corelib:
    path: "../corelib"
YAML
cat > "$WS/tool/src/main.c" <<'C'
#include <stdio.h>
#include <corelib.h>
int main(void) {
    printf("answer=%d\n", core_answer());
    return 0;
}
C
WSOUT="$( cd "$WS" && "$GOOSE" build 2>&1 )"
echo "$WSOUT" | grep -q "2 ok, 0 failed" && ok "workspace reports all members built" || bad "workspace reports all members built"
check "workspace built lib member archive" test -f "$WS/corelib/build/debug/libcorelib.a"
check "workspace built binary member" test -x "$WS/tool/build/debug/tool"

echo
echo "results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
