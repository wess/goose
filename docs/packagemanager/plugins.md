# Plugins

Goose plugins run a custom command on matching source files before compilation. Use them for transpilers (`flex`, `bison`) or any code generator that emits C.

## Configuration

```yaml
plugins:
  lex:
    ext: ".l"
    command: "flex"
  yacc:
    ext: ".y"
    command: "bison -d"
```

| Field | Required | Description |
|-------|----------|-------------|
| `ext` | yes | File extension to match (include the leading dot) |
| `command` | yes | Shell command invoked on each matching file |

## Pipeline

On every `goose build` / `run` / `test`, before compiling the main project goose:

1. Scans `src_dir` recursively for each plugin's `ext`.
2. For every match, runs: `command 'input.{ext}' > 'build/gen/{stem}.c'`.
3. Picks up all generated `.c` files and compiles them with the rest.
4. Adds `build/gen/` to the include path so generated headers resolve.

## Example: flex + bison

```
myparser/
  goose.yaml
  src/
    main.c
    lexer.l
    parser.y
```

```yaml
project:
  name: "myparser"
  version: "0.1.0"

build:
  cflags: "-Wall -std=c11"
  includes:
    - "src"

plugins:
  lex:
    ext: ".l"
    command: "flex"
  yacc:
    ext: ".y"
    command: "bison -d"

dependencies:
```

`goose build` runs:

```sh
flex    'src/lexer.l'  > 'build/gen/lexer.c'
bison -d 'src/parser.y' > 'build/gen/parser.c'
cc ... src/main.c build/gen/lexer.c build/gen/parser.c -o build/debug/myparser
```

## Custom transpilers

Any command that reads a file and writes C to stdout works. A template-based generator:

```yaml
plugins:
  tmpl:
    ext: ".tmpl"
    command: "./tools/tmplgen"
```

Runs: `./tools/tmplgen 'src/page.tmpl' > 'build/gen/page.c'`.

## Notes

- Plugin transpilation happens before the main compile.
- Generated `.c` files live in `build/gen/` and are removed by `goose clean`.
- `build/gen/` is automatically on the include path.
- Plugins are processed in the order they appear in `goose.yaml`.
- Each plugin can match any number of files.
- Up to 16 plugins per project (`MAX_PLUGINS`).
