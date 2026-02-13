#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "headers/cmake.h"
#include "headers/config.h"
#include "headers/color.h"

#define MAX_VARS      64
#define MAX_VAR_NAME  128
#define MAX_VAR_VAL   1024
#define MAX_LINE      2048
#define MAX_ARGS      64

typedef struct {
    char name[MAX_VAR_NAME];
    char value[MAX_VAR_VAL];
} CmakeVar;

typedef struct {
    CmakeVar vars[MAX_VARS];
    int count;
} VarTable;

/* ---- variable table ---- */

static void var_set(VarTable *vt, const char *name, const char *value) {
    for (int i = 0; i < vt->count; i++) {
        if (strcmp(vt->vars[i].name, name) == 0) {
            strncpy(vt->vars[i].value, value, MAX_VAR_VAL - 1);
            vt->vars[i].value[MAX_VAR_VAL - 1] = '\0';
            return;
        }
    }
    if (vt->count < MAX_VARS) {
        strncpy(vt->vars[vt->count].name, name, MAX_VAR_NAME - 1);
        vt->vars[vt->count].name[MAX_VAR_NAME - 1] = '\0';
        strncpy(vt->vars[vt->count].value, value, MAX_VAR_VAL - 1);
        vt->vars[vt->count].value[MAX_VAR_VAL - 1] = '\0';
        vt->count++;
    }
}

static const char *var_get(const VarTable *vt, const char *name) {
    for (int i = 0; i < vt->count; i++) {
        if (strcmp(vt->vars[i].name, name) == 0)
            return vt->vars[i].value;
    }
    return NULL;
}

/* expand ${VAR} references in src, write to dst */
static void var_expand(const VarTable *vt, const char *src, char *dst, int dst_size) {
    int di = 0;
    int len = (int)strlen(src);

    for (int i = 0; i < len && di < dst_size - 1; ) {
        if (src[i] == '$' && i + 1 < len && src[i + 1] == '{') {
            int start = i + 2;
            int end = start;
            while (end < len && src[end] != '}') end++;
            if (end < len) {
                char varname[MAX_VAR_NAME] = {0};
                int vlen = end - start;
                if (vlen >= MAX_VAR_NAME) vlen = MAX_VAR_NAME - 1;
                memcpy(varname, src + start, vlen);
                varname[vlen] = '\0';

                const char *val = var_get(vt, varname);
                if (val) {
                    int vallen = (int)strlen(val);
                    if (di + vallen < dst_size) {
                        memcpy(dst + di, val, vallen);
                        di += vallen;
                    }
                }
                i = end + 1;
            } else {
                dst[di++] = src[i++];
            }
        } else {
            dst[di++] = src[i++];
        }
    }
    dst[di] = '\0';
}

/* strip $<...> generator expressions */
static void strip_genexpr(char *s) {
    char buf[MAX_LINE];
    int bi = 0;
    int len = (int)strlen(s);
    int depth = 0;

    for (int i = 0; i < len && bi < MAX_LINE - 1; i++) {
        if (s[i] == '$' && i + 1 < len && s[i + 1] == '<') {
            depth++;
            i++; /* skip '<' */
        } else if (depth > 0 && s[i] == '>') {
            depth--;
        } else if (depth == 0) {
            buf[bi++] = s[i];
        }
    }
    buf[bi] = '\0';
    strcpy(s, buf);
}

/* ---- string helpers ---- */

static void trim(char *s) {
    char *start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != s)
        memmove(s, start, strlen(start) + 1);
    char *end = s + strlen(s) - 1;
    while (end >= s && isspace((unsigned char)*end))
        *end-- = '\0';
}

/* strip surrounding quotes from a string */
static void strip_quotes(char *s) {
    int len = (int)strlen(s);
    if (len >= 2 && ((s[0] == '"' && s[len - 1] == '"') ||
                     (s[0] == '\'' && s[len - 1] == '\''))) {
        memmove(s, s + 1, len - 2);
        s[len - 2] = '\0';
    }
}

/* case-insensitive prefix check */
static int istarts_with(const char *s, const char *prefix) {
    while (*prefix) {
        if (tolower((unsigned char)*s) != tolower((unsigned char)*prefix))
            return 0;
        s++;
        prefix++;
    }
    return 1;
}

/* parse arguments from inside parentheses, respecting quotes */
static int parse_args(const char *body, char args[][MAX_VAR_VAL], int max_args) {
    int count = 0;
    const char *p = body;

    while (*p && count < max_args) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;

        char arg[MAX_VAR_VAL] = {0};
        int ai = 0;

        if (*p == '"') {
            p++; /* skip opening quote */
            while (*p && *p != '"' && ai < MAX_VAR_VAL - 1)
                arg[ai++] = *p++;
            if (*p == '"') p++;
        } else {
            while (*p && !isspace((unsigned char)*p) && ai < MAX_VAR_VAL - 1)
                arg[ai++] = *p++;
        }
        arg[ai] = '\0';
        if (ai > 0) {
            strncpy(args[count], arg, MAX_VAR_VAL - 1);
            count++;
        }
    }
    return count;
}

/* ---- include path helpers ---- */

static int has_include(const Config *cfg, const char *path) {
    for (int i = 0; i < cfg->include_count; i++) {
        if (strcmp(cfg->includes[i], path) == 0)
            return 1;
    }
    return 0;
}

static void add_include(Config *cfg, const char *path) {
    if (cfg->include_count >= MAX_INCLUDES) return;
    if (has_include(cfg, path)) return;

    /* skip absolute paths and cmake variables */
    if (path[0] == '/' || strstr(path, "${") != NULL)
        return;

    /* normalize: strip leading ./ */
    const char *p = path;
    if (p[0] == '.' && p[1] == '/')
        p += 2;

    if (strlen(p) == 0 || strcmp(p, ".") == 0) {
        if (!has_include(cfg, "."))
            strncpy(cfg->includes[cfg->include_count++], ".", MAX_PATH_LEN - 1);
        return;
    }

    strncpy(cfg->includes[cfg->include_count], p, MAX_PATH_LEN - 1);
    cfg->include_count++;
}

/* append to ldflags, space-separated */
static void add_ldflag(Config *cfg, const char *flag) {
    if (strstr(cfg->ldflags, flag) != NULL)
        return;
    int len = (int)strlen(cfg->ldflags);
    if (len > 0 && len < 254) {
        cfg->ldflags[len] = ' ';
        cfg->ldflags[len + 1] = '\0';
        len++;
    }
    strncat(cfg->ldflags, flag, 255 - len);
}

/* map library name to linker flag */
static void map_lib_to_ldflag(Config *cfg, const char *lib) {
    /* skip cmake keywords */
    if (strcmp(lib, "PUBLIC") == 0 || strcmp(lib, "PRIVATE") == 0 ||
        strcmp(lib, "INTERFACE") == 0 || strcmp(lib, "IMPORTED") == 0 ||
        strcmp(lib, "STATIC") == 0 || strcmp(lib, "SHARED") == 0)
        return;

    /* skip the target's own name and cmake variables */
    if (strstr(lib, "${") != NULL || strstr(lib, "::") != NULL)
        return;

    char flag[128];
    if (lib[0] == '-') {
        /* already a flag like -lpthread */
        snprintf(flag, sizeof(flag), "%s", lib);
    } else {
        snprintf(flag, sizeof(flag), "-l%s", lib);
    }
    add_ldflag(cfg, flag);
}

/* infer directory from a source file path */
static void infer_src_dir(const char *src_file, char *dir, int dir_size) {
    const char *slash = strrchr(src_file, '/');
    if (slash) {
        int len = (int)(slash - src_file);
        if (len >= dir_size) len = dir_size - 1;
        memcpy(dir, src_file, len);
        dir[len] = '\0';
    } else {
        strncpy(dir, ".", dir_size - 1);
        dir[dir_size - 1] = '\0';
    }
}

/* ---- main parser ---- */

int cmake_to_config(const char *cmake_path, Config *cfg) {
    FILE *f = fopen(cmake_path, "r");
    if (!f) {
        err("cannot open %s", cmake_path);
        return -1;
    }

    config_default(cfg, "unnamed");
    cfg->include_count = 0; /* clear default includes, we'll populate from CMake */
    cfg->cflags[0] = '\0';
    cfg->ldflags[0] = '\0';

    VarTable vt;
    memset(&vt, 0, sizeof(vt));
    /* set common cmake vars */
    var_set(&vt, "CMAKE_CURRENT_SOURCE_DIR", ".");
    var_set(&vt, "PROJECT_SOURCE_DIR", ".");

    char line[MAX_LINE];
    char accumulated[MAX_LINE * 4] = {0};
    int paren_depth = 0;

    while (fgets(line, sizeof(line), f)) {
        /* strip comment (outside of quotes) */
        int in_quotes = 0;
        for (int i = 0; line[i]; i++) {
            if (line[i] == '"') in_quotes = !in_quotes;
            if (!in_quotes && line[i] == '#') {
                line[i] = '\0';
                break;
            }
        }
        trim(line);
        if (strlen(line) == 0) continue;

        /* handle multi-line commands */
        if (paren_depth > 0) {
            if (strlen(accumulated) + strlen(line) + 2 < sizeof(accumulated)) {
                strcat(accumulated, " ");
                strcat(accumulated, line);
            }
        } else {
            strncpy(accumulated, line, sizeof(accumulated) - 1);
            accumulated[sizeof(accumulated) - 1] = '\0';
        }

        /* count parens in full accumulated string */
        paren_depth = 0;
        for (int i = 0; accumulated[i]; i++) {
            if (accumulated[i] == '(') paren_depth++;
            else if (accumulated[i] == ')') paren_depth--;
        }

        if (paren_depth > 0) continue;

        /* we have a complete command — process it */
        char cmd[MAX_LINE * 4];
        strncpy(cmd, accumulated, sizeof(cmd) - 1);
        cmd[sizeof(cmd) - 1] = '\0';
        accumulated[0] = '\0';
        paren_depth = 0;

        /* expand variables */
        char expanded[MAX_LINE * 4];
        var_expand(&vt, cmd, expanded, sizeof(expanded));
        strip_genexpr(expanded);

        /* find command name and body */
        char *paren = strchr(expanded, '(');
        if (!paren) continue;

        *paren = '\0';
        char *cmd_name = expanded;
        trim(cmd_name);

        char *body = paren + 1;
        char *close = strrchr(body, ')');
        if (close) *close = '\0';
        trim(body);

        char args[MAX_ARGS][MAX_VAR_VAL];
        memset(args, 0, sizeof(args));
        int argc = parse_args(body, args, MAX_ARGS);

        /* ---- project() ---- */
        if (istarts_with(cmd_name, "project") && argc > 0) {
            strncpy(cfg->name, args[0], MAX_NAME_LEN - 1);
            var_set(&vt, "PROJECT_NAME", args[0]);
            for (int i = 1; i < argc - 1; i++) {
                if (strcmp(args[i], "VERSION") == 0) {
                    strncpy(cfg->version, args[i + 1], 63);
                    break;
                }
            }
        }

        /* ---- set() ---- */
        else if (istarts_with(cmd_name, "set") && argc >= 2) {
            /* concatenate all values with spaces */
            char combined[MAX_VAR_VAL] = {0};
            for (int i = 1; i < argc; i++) {
                if (strcmp(args[i], "CACHE") == 0 || strcmp(args[i], "PARENT_SCOPE") == 0 ||
                    strcmp(args[i], "FORCE") == 0 || strcmp(args[i], "STRING") == 0 ||
                    strcmp(args[i], "BOOL") == 0 || strcmp(args[i], "PATH") == 0 ||
                    strcmp(args[i], "FILEPATH") == 0 || strcmp(args[i], "INTERNAL") == 0)
                    break;
                if (i > 1 && strlen(combined) > 0)
                    strncat(combined, " ", MAX_VAR_VAL - strlen(combined) - 1);
                strncat(combined, args[i], MAX_VAR_VAL - strlen(combined) - 1);
            }
            var_set(&vt, args[0], combined);
        }

        /* ---- add_library() ---- */
        else if (istarts_with(cmd_name, "add_library") && argc > 0) {
            /* first source file tells us src_dir */
            for (int i = 1; i < argc; i++) {
                if (strcmp(args[i], "STATIC") == 0 || strcmp(args[i], "SHARED") == 0 ||
                    strcmp(args[i], "OBJECT") == 0 || strcmp(args[i], "MODULE") == 0 ||
                    strcmp(args[i], "INTERFACE") == 0 || strcmp(args[i], "IMPORTED") == 0 ||
                    strcmp(args[i], "ALIAS") == 0 || strcmp(args[i], "EXCLUDE_FROM_ALL") == 0)
                    continue;

                /* looks like a source file */
                char dir[MAX_PATH_LEN];
                infer_src_dir(args[i], dir, sizeof(dir));
                if (strcmp(dir, ".") != 0 && strcmp(cfg->src_dir, "src") == 0)
                    strncpy(cfg->src_dir, dir, MAX_PATH_LEN - 1);
                break;
            }
        }

        /* ---- add_executable() ---- */
        else if (istarts_with(cmd_name, "add_executable") && argc > 0) {
            if (strcmp(cfg->name, "unnamed") == 0)
                strncpy(cfg->name, args[0], MAX_NAME_LEN - 1);

            for (int i = 1; i < argc; i++) {
                if (strcmp(args[i], "WIN32") == 0 || strcmp(args[i], "MACOSX_BUNDLE") == 0 ||
                    strcmp(args[i], "EXCLUDE_FROM_ALL") == 0)
                    continue;

                char dir[MAX_PATH_LEN];
                infer_src_dir(args[i], dir, sizeof(dir));
                if (strcmp(dir, ".") != 0 && strcmp(cfg->src_dir, "src") == 0)
                    strncpy(cfg->src_dir, dir, MAX_PATH_LEN - 1);
                break;
            }
        }

        /* ---- include_directories() ---- */
        else if (istarts_with(cmd_name, "include_directories")) {
            for (int i = 0; i < argc; i++) {
                if (strcmp(args[i], "BEFORE") == 0 || strcmp(args[i], "AFTER") == 0 ||
                    strcmp(args[i], "SYSTEM") == 0)
                    continue;
                strip_quotes(args[i]);
                add_include(cfg, args[i]);
            }
        }

        /* ---- target_include_directories() ---- */
        else if (istarts_with(cmd_name, "target_include_directories") && argc > 1) {
            for (int i = 1; i < argc; i++) {
                if (strcmp(args[i], "PUBLIC") == 0 || strcmp(args[i], "PRIVATE") == 0 ||
                    strcmp(args[i], "INTERFACE") == 0 || strcmp(args[i], "SYSTEM") == 0 ||
                    strcmp(args[i], "BEFORE") == 0 || strcmp(args[i], "AFTER") == 0)
                    continue;
                strip_quotes(args[i]);
                add_include(cfg, args[i]);
            }
        }

        /* ---- target_link_libraries() ---- */
        else if (istarts_with(cmd_name, "target_link_libraries") && argc > 1) {
            for (int i = 1; i < argc; i++) {
                map_lib_to_ldflag(cfg, args[i]);
            }
        }

        /* ---- file(GLOB ...) / file(GLOB_RECURSE ...) ---- */
        else if (istarts_with(cmd_name, "file") && argc >= 3) {
            if (strcmp(args[0], "GLOB") == 0 || strcmp(args[0], "GLOB_RECURSE") == 0) {
                /* args[1] is variable name, args[2..] are patterns */
                /* extract directory from patterns for include hints */
                char combined[MAX_VAR_VAL] = {0};
                for (int i = 2; i < argc; i++) {
                    if (strcmp(args[i], "RELATIVE") == 0 || strcmp(args[i], "CONFIGURE_DEPENDS") == 0) {
                        if (strcmp(args[i], "RELATIVE") == 0) i++; /* skip the path argument */
                        continue;
                    }
                    strip_quotes(args[i]);

                    /* infer directory from glob pattern */
                    char dir[MAX_PATH_LEN];
                    infer_src_dir(args[i], dir, sizeof(dir));
                    if (strcmp(dir, ".") != 0)
                        add_include(cfg, dir);

                    if (strlen(combined) > 0)
                        strncat(combined, " ", MAX_VAR_VAL - strlen(combined) - 1);
                    strncat(combined, args[i], MAX_VAR_VAL - strlen(combined) - 1);
                }
                /* store so ${VAR} references work later */
                if (argc >= 2)
                    var_set(&vt, args[1], combined);
            }
        }

        /* ---- add_subdirectory() ---- */
        else if (istarts_with(cmd_name, "add_subdirectory") && argc > 0) {
            add_include(cfg, args[0]);
        }

        /* ---- find_package() ---- */
        else if (istarts_with(cmd_name, "find_package") && argc > 0) {
            /* can't resolve, note it */
            warn("Note", "find_package(%s) detected — may need manual ldflags", args[0]);
        }
    }

    fclose(f);

    /* ensure at least "." include if none found */
    if (cfg->include_count == 0)
        add_include(cfg, ".");

    /* set reasonable default cflags */
    strncpy(cfg->cflags, "-Wall -Wextra -std=c11", 255);

    return 0;
}

int cmake_convert_file(const char *cmake_path, const char *yaml_path) {
    Config cfg;
    if (cmake_to_config(cmake_path, &cfg) != 0)
        return -1;
    return config_save(yaml_path, &cfg);
}
