VERSION = $(shell cat VERSION)

CC      = cc
CFLAGS  = -Wall -Wextra -std=c11 -Ilibs/libyaml/include -DGOOSE_VERSION_FROM_FILE=\"$(VERSION)\"
LDFLAGS =

YAML_SRC = $(wildcard libs/libyaml/src/*.c)
YAML_OBJ = $(YAML_SRC:libs/libyaml/src/%.c=obj/yaml_%.o)

SRC      = $(wildcard src/*.c)
CMD_SRC  = $(wildcard src/cmd/*.c)
OBJ      = $(SRC:src/%.c=obj/%.o) $(CMD_SRC:src/cmd/%.c=obj/cmd/%.o)
BIN      = goose

PREFIX  ?= /usr/local

.PHONY: all clean install uninstall

all: $(BIN)

$(BIN): $(OBJ) $(YAML_OBJ)
	$(CC) $(OBJ) $(YAML_OBJ) -o $@ $(LDFLAGS)

obj/%.o: src/%.c | obj
	$(CC) $(CFLAGS) -c $< -o $@

obj/cmd/%.o: src/cmd/%.c | obj
	$(CC) $(CFLAGS) -c $< -o $@

YAML_DEFS = -DYAML_VERSION_MAJOR=0 -DYAML_VERSION_MINOR=2 -DYAML_VERSION_PATCH=5 \
            -DYAML_VERSION_STRING=\"0.2.5\"

obj/yaml_%.o: libs/libyaml/src/%.c | obj
	$(CC) $(YAML_DEFS) -Ilibs/libyaml/include -Ilibs/libyaml/src -w -c $< -o $@

obj:
	mkdir -p obj obj/cmd

clean:
	rm -rf obj $(BIN)

install: $(BIN)
	install -d $(PREFIX)/bin
	install -m 755 $(BIN) $(PREFIX)/bin/$(BIN)

uninstall:
	rm -f $(PREFIX)/bin/$(BIN)
