# Plugins

Goose supports a plugin system for running custom transpilers or code generators before compilation. This lets you use tools like `flex`, `bison`, or any command that produces `.c` files from other file types.

## Configuration

Define plugins in the `plugins` section of `goose.yaml`. Each plugin has a name, a file extension to match, and a command to run:

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
| `ext` | yes | File extension to match (including the dot) |
| `command` | yes | Shell command to invoke on each matching file |

## How It Works

When you run `goose build` (or `goose run`, `goose test`), the build pipeline:

1. Scans `src_dir` recursively for files matching each plugin's extension
2. Runs the command on each file: `command 'input.l' > 'build/gen/input.c'`
3. Places generated `.c` files in `build/gen/`
4. Compiles the generated files alongside your regular source files
5. Adds `build/gen/` to the include path so generated headers are accessible

## Example: Flex and Bison

A project using flex for lexing and bison for parsing:

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

Running `goose build` will:

1. Find `src/lexer.l` and run `flex 'src/lexer.l' > 'build/gen/lexer.c'`
2. Find `src/parser.y` and run `bison -d 'src/parser.y' > 'build/gen/parser.c'`
3. Compile `src/main.c`, `build/gen/lexer.c`, and `build/gen/parser.c` together

## Writing a Custom Transpiler

Any command that reads a source file and writes C to stdout can be a plugin. For example, a script that generates C from a template format:

```yaml
plugins:
  tmpl:
    ext: ".tmpl"
    command: "./tools/tmplgen"
```

The command is invoked as:

```sh
./tools/tmplgen 'src/page.tmpl' > 'build/gen/page.c'
```

## Notes

- Plugin transpilation runs before the main compilation step
- Generated files live in `build/gen/` and are cleaned with `goose clean`
- The `build/gen/` directory is automatically added to the include path
- Plugins are processed in the order they appear in `goose.yaml`
- Each plugin can match multiple files -- all are transpiled
- Up to 16 plugins can be defined (`MAX_PLUGINS`)
