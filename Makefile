VERSION = $(shell cat VERSION)

CC      = cc
CFLAGS  = -Wall -Wextra -std=c11 -Isrc -Ilibs/libyaml/include -DGOOSE_VERSION_FROM_FILE=\"$(VERSION)\"
LDFLAGS =

BUILD    = build
OBJDIR   = $(BUILD)/obj

YAML_SRC = $(wildcard libs/libyaml/src/*.c)
YAML_OBJ = $(YAML_SRC:libs/libyaml/src/%.c=$(OBJDIR)/yaml_%.o)

SRC      = $(filter-out src/main.c, $(wildcard src/*.c))
CMD_SRC  = $(wildcard src/cmd/*.c)
LIB_OBJ  = $(SRC:src/%.c=$(OBJDIR)/%.o) $(CMD_SRC:src/cmd/%.c=$(OBJDIR)/cmd/%.o) $(YAML_OBJ)
CLI_OBJ  = $(OBJDIR)/main.o

LIB      = $(BUILD)/libgoose.a
BIN      = $(BUILD)/goose

PREFIX  ?= /usr/local

.PHONY: all lib cli clean install uninstall

all: lib cli

lib: $(LIB)

cli: $(BIN)

$(LIB): $(LIB_OBJ)
	ar rcs $@ $^

$(BIN): $(CLI_OBJ) $(LIB)
	$(CC) $(CLI_OBJ) $(LIB) -o $@ $(LDFLAGS)

$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/cmd/%.o: src/cmd/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

YAML_DEFS = -DYAML_VERSION_MAJOR=0 -DYAML_VERSION_MINOR=2 -DYAML_VERSION_PATCH=5 \
            -DYAML_VERSION_STRING=\"0.2.5\"

$(OBJDIR)/yaml_%.o: libs/libyaml/src/%.c | $(OBJDIR)
	$(CC) $(YAML_DEFS) -Ilibs/libyaml/include -Ilibs/libyaml/src -w -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR) $(OBJDIR)/cmd

clean:
	rm -rf $(BUILD)

install: all
	install -d $(PREFIX)/bin
	install -m 755 $(BIN) $(PREFIX)/bin/goose
	install -d $(PREFIX)/lib
	install -m 644 $(LIB) $(PREFIX)/lib/libgoose.a
	install -d $(PREFIX)/include/goose/headers
	install -m 644 include/goose.h $(PREFIX)/include/goose/goose.h
	for h in src/headers/*.h; do \
		install -m 644 "$$h" $(PREFIX)/include/goose/headers/; \
	done

uninstall:
	rm -f $(PREFIX)/bin/goose
	rm -f $(PREFIX)/lib/libgoose.a
	rm -rf $(PREFIX)/include/goose
