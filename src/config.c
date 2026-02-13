#include <stdio.h>
#include <string.h>
#include <yaml.h>
#include "headers/config.h"
#include "headers/main.h"
#include "headers/color.h"

void config_default(Config *cfg, const char *name) {
    memset(cfg, 0, sizeof(Config));
    strncpy(cfg->name, name, MAX_NAME_LEN - 1);
    strncpy(cfg->version, "0.1.0", 63);
    strncpy(cfg->description, "", 255);
    strncpy(cfg->author, "", MAX_NAME_LEN - 1);
    strncpy(cfg->license, "MIT", 63);
    strncpy(cfg->src_dir, "src", MAX_PATH_LEN - 1);
    strncpy(cfg->cc, "cc", 63);
    strncpy(cfg->cflags, "-Wall -Wextra -std=c11", 255);
    strncpy(cfg->ldflags, "", 255);
    /* default include: src */
    strncpy(cfg->includes[0], "src", MAX_PATH_LEN - 1);
    cfg->include_count = 1;
    cfg->source_count = 0;
    cfg->dep_count = 0;
}

/* --- YAML Loading --- */

typedef enum {
    S_NONE, S_PROJECT, S_DEPS, S_DEP_ENTRY, S_BUILD
} Section;

int config_load(const char *path, Config *cfg) {
    FILE *f = fopen(path, "r");
    if (!f) {
        err("cannot open %s (are you in a goose project?)", path);
        return -1;
    }

    config_default(cfg, "unnamed");

    yaml_parser_t parser;
    yaml_event_t event;
    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, f);

    Section section = S_NONE;
    char key[256] = {0};
    int in_key = 0;
    int depth = 0;
    int in_includes = 0;
    int parsed_includes = 0;
    int in_sources = 0;
    Dependency *cur_dep = NULL;

    while (1) {
        if (!yaml_parser_parse(&parser, &event)) {
            err("YAML parse error in %s (line %lu)", path, parser.problem_mark.line + 1);
            yaml_parser_delete(&parser);
            fclose(f);
            return -1;
        }

        if (event.type == YAML_STREAM_END_EVENT) {
            yaml_event_delete(&event);
            break;
        }

        switch (event.type) {
        case YAML_SEQUENCE_START_EVENT:
            if (section == S_BUILD && strcmp(key, "includes") == 0) {
                in_includes = 1;
                if (!parsed_includes) {
                    cfg->include_count = 0;
                    parsed_includes = 1;
                }
            } else if (section == S_BUILD && strcmp(key, "sources") == 0) {
                in_sources = 1;
                cfg->source_count = 0;
            }
            break;

        case YAML_SEQUENCE_END_EVENT:
            in_includes = 0;
            in_sources = 0;
            in_key = 0;
            break;

        case YAML_MAPPING_START_EVENT:
            depth++;
            in_key = 0;
            if (depth == 2 && strcmp(key, "project") == 0)
                section = S_PROJECT;
            else if (depth == 2 && strcmp(key, "dependencies") == 0)
                section = S_DEPS;
            else if (depth == 2 && strcmp(key, "build") == 0)
                section = S_BUILD;
            else if (section == S_DEPS && depth == 3) {
                section = S_DEP_ENTRY;
                if (cfg->dep_count < MAX_DEPS) {
                    cur_dep = &cfg->deps[cfg->dep_count];
                    memset(cur_dep, 0, sizeof(Dependency));
                    strncpy(cur_dep->name, key, MAX_NAME_LEN - 1);
                }
            }
            break;

        case YAML_MAPPING_END_EVENT:
            depth--;
            in_key = 0;
            if (section == S_DEP_ENTRY && depth == 2) {
                if (cfg->dep_count < MAX_DEPS)
                    cfg->dep_count++;
                cur_dep = NULL;
                section = S_DEPS;
            }
            if (depth <= 1) section = S_NONE;
            break;

        case YAML_SCALAR_EVENT: {
            const char *val = (const char *)event.data.scalar.value;

            /* items inside includes sequence */
            if (in_includes) {
                if (cfg->include_count < MAX_INCLUDES) {
                    strncpy(cfg->includes[cfg->include_count], val, MAX_PATH_LEN - 1);
                    cfg->include_count++;
                }
                break;
            }

            /* items inside sources sequence */
            if (in_sources) {
                if (cfg->source_count < MAX_SRC_FILES) {
                    strncpy(cfg->sources[cfg->source_count], val, MAX_PATH_LEN - 1);
                    cfg->source_count++;
                }
                break;
            }

            if (!in_key) {
                strncpy(key, val, 255);
                key[255] = '\0';
                in_key = 1;
            } else {
                if (section == S_PROJECT) {
                    if (strcmp(key, "name") == 0)
                        strncpy(cfg->name, val, MAX_NAME_LEN - 1);
                    else if (strcmp(key, "version") == 0)
                        strncpy(cfg->version, val, 63);
                    else if (strcmp(key, "description") == 0)
                        strncpy(cfg->description, val, 255);
                    else if (strcmp(key, "author") == 0)
                        strncpy(cfg->author, val, MAX_NAME_LEN - 1);
                    else if (strcmp(key, "license") == 0)
                        strncpy(cfg->license, val, 63);
                } else if (section == S_DEP_ENTRY && cur_dep) {
                    if (strcmp(key, "git") == 0)
                        strncpy(cur_dep->git, val, MAX_PATH_LEN - 1);
                    else if (strcmp(key, "version") == 0)
                        strncpy(cur_dep->version, val, 63);
                } else if (section == S_BUILD) {
                    if (strcmp(key, "cc") == 0)
                        strncpy(cfg->cc, val, 63);
                    else if (strcmp(key, "cflags") == 0)
                        strncpy(cfg->cflags, val, 255);
                    else if (strcmp(key, "ldflags") == 0)
                        strncpy(cfg->ldflags, val, 255);
                    else if (strcmp(key, "src_dir") == 0)
                        strncpy(cfg->src_dir, val, MAX_PATH_LEN - 1);

                }
                in_key = 0;
            }
            break;
        }
        default:
            break;
        }

        yaml_event_delete(&event);
    }

    yaml_parser_delete(&parser);
    fclose(f);
    return 0;
}

/* --- YAML Saving --- */

int config_save(const char *path, const Config *cfg) {
    FILE *f = fopen(path, "w");
    if (!f) {
        perror(path);
        return -1;
    }

    fprintf(f, "project:\n");
    fprintf(f, "  name: \"%s\"\n", cfg->name);
    fprintf(f, "  version: \"%s\"\n", cfg->version);
    fprintf(f, "  description: \"%s\"\n", cfg->description);
    fprintf(f, "  author: \"%s\"\n", cfg->author);
    fprintf(f, "  license: \"%s\"\n", cfg->license);
    fprintf(f, "\n");

    fprintf(f, "build:\n");
    fprintf(f, "  cc: \"%s\"\n", cfg->cc);
    fprintf(f, "  cflags: \"%s\"\n", cfg->cflags);
    if (strlen(cfg->ldflags) > 0)
        fprintf(f, "  ldflags: \"%s\"\n", cfg->ldflags);
    if (strcmp(cfg->src_dir, "src") != 0)
        fprintf(f, "  src_dir: \"%s\"\n", cfg->src_dir);
    fprintf(f, "  includes:\n");
    for (int i = 0; i < cfg->include_count; i++)
        fprintf(f, "    - \"%s\"\n", cfg->includes[i]);
    if (cfg->source_count > 0) {
        fprintf(f, "  sources:\n");
        for (int i = 0; i < cfg->source_count; i++)
            fprintf(f, "    - \"%s\"\n", cfg->sources[i]);
    }
    fprintf(f, "\n");

    fprintf(f, "dependencies:\n");
    for (int i = 0; i < cfg->dep_count; i++) {
        fprintf(f, "  %s:\n", cfg->deps[i].name);
        fprintf(f, "    git: \"%s\"\n", cfg->deps[i].git);
        if (strlen(cfg->deps[i].version) > 0)
            fprintf(f, "    version: \"%s\"\n", cfg->deps[i].version);
    }

    fclose(f);
    return 0;
}
