VERSION = $(shell cat VERSION)

CC      = cc
CFLAGS  = -Wall -Wextra -std=c11 -Ilibs/libyaml/include -DGOOSE_VERSION_FROM_FILE=\"$(VERSION)\"
LDFLAGS =

BUILD    = build
OBJDIR   = $(BUILD)/obj

YAML_SRC = $(wildcard libs/libyaml/src/*.c)
YAML_OBJ = $(YAML_SRC:libs/libyaml/src/%.c=$(OBJDIR)/yaml_%.o)

SRC      = $(wildcard src/*.c)
CMD_SRC  = $(wildcard src/cmd/*.c)
OBJ      = $(SRC:src/%.c=$(OBJDIR)/%.o) $(CMD_SRC:src/cmd/%.c=$(OBJDIR)/cmd/%.o)
BIN      = $(BUILD)/goose

PREFIX  ?= /usr/local

.PHONY: all clean install uninstall

all: $(BIN)

$(BIN): $(OBJ) $(YAML_OBJ)
	$(CC) $(OBJ) $(YAML_OBJ) -o $@ $(LDFLAGS)

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

install: $(BIN)
	install -d $(PREFIX)/bin
	install -m 755 $(BIN) $(PREFIX)/bin/$(BIN)

uninstall:
	rm -f $(PREFIX)/bin/$(BIN)
