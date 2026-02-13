#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "headers/cmake.h"
#include "headers/config.h"
#include "headers/color.h"
#include "headers/fs.h"

#define MAX_VARS      128
#define MAX_VAR_NAME  128
#define MAX_VAR_VAL   4096
#define MAX_LINE      2048
#define MAX_ARGS      128
#define MAX_IF_DEPTH  16

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

static void var_append(VarTable *vt, const char *name, const char *value) {
    const char *existing = NULL;
    for (int i = 0; i < vt->count; i++) {
        if (strcmp(vt->vars[i].name, name) == 0) {
            existing = vt->vars[i].value;
            int len = (int)strlen(vt->vars[i].value);
            if (len > 0 && len < MAX_VAR_VAL - 2) {
                vt->vars[i].value[len] = ' ';
                vt->vars[i].value[len + 1] = '\0';
            }
            strncat(vt->vars[i].value, value, MAX_VAR_VAL - strlen(vt->vars[i].value) - 1);
            return;
        }
    }
    (void)existing;
    var_set(vt, name, value);
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
    char buf[MAX_LINE * 4];
    int bi = 0;
    int len = (int)strlen(s);
    int depth = 0;

    for (int i = 0; i < len && bi < (int)sizeof(buf) - 1; i++) {
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

static void strip_quotes(char *s) {
    int len = (int)strlen(s);
    if (len >= 2 && ((s[0] == '"' && s[len - 1] == '"') ||
                     (s[0] == '\'' && s[len - 1] == '\''))) {
        memmove(s, s + 1, len - 2);
        s[len - 2] = '\0';
    }
}

static int istarts_with(const char *s, const char *prefix) {
    while (*prefix) {
        if (tolower((unsigned char)*s) != tolower((unsigned char)*prefix))
            return 0;
        s++;
        prefix++;
    }
    return 1;
}

/* case-insensitive compare */
static int iequals(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return 0;
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
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

    /* skip absolute paths and cmake variables */
    if (path[0] == '/' || strstr(path, "${") != NULL)
        return;

    /* normalize: strip leading ./ */
    const char *p = path;
    if (p[0] == '.' && p[1] == '/')
        p += 2;

    if (strlen(p) == 0 || strcmp(p, ".") == 0)
        p = ".";

    /* deduplicate after normalization */
    if (has_include(cfg, p)) return;

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

/* add a source file to cfg->sources[] */
static void add_source(Config *cfg, const char *prefix, const char *file) {
    if (cfg->source_count >= MAX_SRC_FILES) return;

    /* skip non-.c files */
    const char *dot = strrchr(file, '.');
    if (!dot || strcmp(dot, ".c") != 0) return;

    char path[MAX_PATH_LEN];
    if (prefix && strlen(prefix) > 0 && strcmp(prefix, ".") != 0)
        snprintf(path, sizeof(path), "%s/%s", prefix, file);
    else
        snprintf(path, sizeof(path), "%s", file);

    /* deduplicate */
    for (int i = 0; i < cfg->source_count; i++) {
        if (strcmp(cfg->sources[i], path) == 0)
            return;
    }

    strncpy(cfg->sources[cfg->source_count], path, MAX_PATH_LEN - 1);
    cfg->source_count++;
}

/* ---- conditional evaluation ---- */

static int eval_condition_value(const char *val) {
    if (!val || strlen(val) == 0) return 0;
    if (iequals(val, "ON") || iequals(val, "TRUE") ||
        iequals(val, "YES") || strcmp(val, "1") == 0)
        return 1;
    if (iequals(val, "OFF") || iequals(val, "FALSE") ||
        iequals(val, "NO") || strcmp(val, "0") == 0 ||
        iequals(val, "NOTFOUND") || strstr(val, "-NOTFOUND") != NULL)
        return 0;
    /* non-empty string is truthy */
    return 1;
}

/* evaluate a simple cmake if() condition */
static int eval_condition(const VarTable *vt, const char *body) {
    char args[MAX_ARGS][MAX_VAR_VAL];
    memset(args, 0, sizeof(args));
    int argc = parse_args(body, args, MAX_ARGS);
    if (argc == 0) return 0;

    /* NOT <cond> */
    if (argc >= 2 && strcmp(args[0], "NOT") == 0) {
        const char *val = var_get(vt, args[1]);
        return !eval_condition_value(val);
    }

    /* DEFINED <var> */
    if (argc >= 2 && strcmp(args[0], "DEFINED") == 0) {
        return var_get(vt, args[1]) != NULL;
    }

    /* <var> STREQUAL <val> */
    if (argc >= 3 && strcmp(args[1], "STREQUAL") == 0) {
        const char *lhs = var_get(vt, args[0]);
        if (!lhs) lhs = args[0];
        return strcmp(lhs, args[2]) == 0;
    }

    /* <var> MATCHES <pattern> - treat as false for simplicity */
    if (argc >= 3 && strcmp(args[1], "MATCHES") == 0)
        return 0;

    /* simple variable truthiness check */
    const char *val = var_get(vt, args[0]);
    return eval_condition_value(val);
}

/* ---- cmake file parser (recursive for subdirectories) ---- */

static int parse_cmake_file(const char *cmake_path, const char *base_dir,
                            Config *cfg, VarTable *vt) {
    FILE *f = fopen(cmake_path, "r");
    if (!f) return -1;

    /* save and set CMAKE_CURRENT_SOURCE_DIR */
    char saved_cur_dir[MAX_VAR_VAL] = {0};
    const char *prev = var_get(vt, "CMAKE_CURRENT_SOURCE_DIR");
    if (prev)
        strncpy(saved_cur_dir, prev, MAX_VAR_VAL - 1);
    var_set(vt, "CMAKE_CURRENT_SOURCE_DIR", base_dir);

    char line[MAX_LINE];
    char accumulated[MAX_LINE * 4] = {0};
    int paren_depth = 0;

    /* if/else/endif tracking */
    int if_depth = 0;
    int if_active[MAX_IF_DEPTH]; /* is current branch active? */
    int if_satisfied[MAX_IF_DEPTH]; /* has any branch been taken? */
    memset(if_active, 0, sizeof(if_active));
    memset(if_satisfied, 0, sizeof(if_satisfied));

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

        /* count parens */
        paren_depth = 0;
        for (int i = 0; accumulated[i]; i++) {
            if (accumulated[i] == '(') paren_depth++;
            else if (accumulated[i] == ')') paren_depth--;
        }
        if (paren_depth > 0) continue;

        /* complete command */
        char cmd[MAX_LINE * 4];
        strncpy(cmd, accumulated, sizeof(cmd) - 1);
        cmd[sizeof(cmd) - 1] = '\0';
        accumulated[0] = '\0';
        paren_depth = 0;

        /* expand variables */
        char expanded[MAX_LINE * 4];
        var_expand(vt, cmd, expanded, sizeof(expanded));
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

        /* ---- if/elseif/else/endif control flow ---- */
        if (iequals(cmd_name, "if")) {
            if (if_depth < MAX_IF_DEPTH) {
                int parent_active = (if_depth == 0) ? 1 : if_active[if_depth - 1];
                int result = parent_active ? eval_condition(vt, body) : 0;
                if_active[if_depth] = result;
                if_satisfied[if_depth] = result;
                if_depth++;
            }
            continue;
        }
        if (iequals(cmd_name, "elseif")) {
            if (if_depth > 0) {
                int idx = if_depth - 1;
                int parent_active = (idx == 0) ? 1 : if_active[idx - 1];
                if (parent_active && !if_satisfied[idx]) {
                    int result = eval_condition(vt, body);
                    if_active[idx] = result;
                    if (result) if_satisfied[idx] = 1;
                } else {
                    if_active[idx] = 0;
                }
            }
            continue;
        }
        if (iequals(cmd_name, "else")) {
            if (if_depth > 0) {
                int idx = if_depth - 1;
                int parent_active = (idx == 0) ? 1 : if_active[idx - 1];
                if_active[idx] = parent_active && !if_satisfied[idx];
                if (if_active[idx]) if_satisfied[idx] = 1;
            }
            continue;
        }
        if (iequals(cmd_name, "endif")) {
            if (if_depth > 0) if_depth--;
            continue;
        }

        /* skip commands in inactive if-branches */
        if (if_depth > 0 && !if_active[if_depth - 1])
            continue;

        char args[MAX_ARGS][MAX_VAR_VAL];
        memset(args, 0, sizeof(args));
        int argc = parse_args(body, args, MAX_ARGS);

        /* ---- project() ---- */
        if (iequals(cmd_name, "project") && argc > 0) {
            strncpy(cfg->name, args[0], MAX_NAME_LEN - 1);
            var_set(vt, "PROJECT_NAME", args[0]);

            /* set derived variables like GD uses: ${PROJECT_NAME} = "GD" */
            char upper[MAX_NAME_LEN] = {0};
            for (int i = 0; args[0][i] && i < MAX_NAME_LEN - 1; i++)
                upper[i] = toupper((unsigned char)args[0][i]);
            var_set(vt, upper, args[0]);

            /* set <PROJECT_NAME>_SOURCE_DIR = PROJECT_SOURCE_DIR */
            char src_dir_var[MAX_VAR_NAME];
            snprintf(src_dir_var, sizeof(src_dir_var), "%s_SOURCE_DIR", args[0]);
            var_set(vt, src_dir_var, ".");
            snprintf(src_dir_var, sizeof(src_dir_var), "%s_SOURCE_DIR", upper);
            var_set(vt, src_dir_var, ".");

            /* <PROJECT_NAME>_BINARY_DIR */
            char bin_dir_var[MAX_VAR_NAME];
            snprintf(bin_dir_var, sizeof(bin_dir_var), "%s_BINARY_DIR", args[0]);
            var_set(vt, bin_dir_var, ".");
            snprintf(bin_dir_var, sizeof(bin_dir_var), "%s_BINARY_DIR", upper);
            var_set(vt, bin_dir_var, ".");

            /* also common: ${PROJECT_NAME}_LIB */
            char lib_var[MAX_VAR_NAME];
            snprintf(lib_var, sizeof(lib_var), "%s_LIB", upper);
            /* lowercase name for lib */
            char lower[MAX_NAME_LEN] = {0};
            for (int i = 0; args[0][i] && i < MAX_NAME_LEN - 1; i++)
                lower[i] = tolower((unsigned char)args[0][i]);
            var_set(vt, lib_var, lower);

            for (int i = 1; i < argc - 1; i++) {
                if (strcmp(args[i], "VERSION") == 0) {
                    strncpy(cfg->version, args[i + 1], 63);
                    break;
                }
            }
        }

        /* ---- cmake_minimum_required() ---- */
        else if (iequals(cmd_name, "cmake_minimum_required")) {
            /* skip, no-op */
        }

        /* ---- option() ---- */
        else if (iequals(cmd_name, "option") && argc >= 2) {
            /* option(VAR "description" [default]) */
            const char *defval = "OFF";
            if (argc >= 3) {
                /* last arg is the default value (skip description string) */
                defval = args[argc - 1];
                /* check if it looks like a bool value */
                if (iequals(defval, "ON") || iequals(defval, "OFF") ||
                    iequals(defval, "TRUE") || iequals(defval, "FALSE") ||
                    strcmp(defval, "0") == 0 || strcmp(defval, "1") == 0) {
                    /* use it */
                } else {
                    defval = "OFF";
                }
            }
            /* only set if not already defined (allows parent scope to override) */
            if (!var_get(vt, args[0]))
                var_set(vt, args[0], defval);
        }

        /* ---- set() ---- */
        else if (iequals(cmd_name, "set") && argc >= 2) {
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
            var_set(vt, args[0], combined);
        }

        /* ---- list() ---- */
        else if (iequals(cmd_name, "list") && argc >= 3) {
            if (strcmp(args[0], "APPEND") == 0) {
                /* list(APPEND var item1 item2 ...) */
                for (int i = 2; i < argc; i++)
                    var_append(vt, args[1], args[i]);
            }
        }

        /* ---- add_library() ---- */
        else if (istarts_with(cmd_name, "add_library") && argc > 0) {
            int is_imported = 0;
            int is_alias = 0;
            int is_interface = 0;

            for (int i = 1; i < argc; i++) {
                if (strcmp(args[i], "IMPORTED") == 0) is_imported = 1;
                if (strcmp(args[i], "ALIAS") == 0) is_alias = 1;
                if (strcmp(args[i], "INTERFACE") == 0) is_interface = 1;
            }

            /* skip IMPORTED/ALIAS/INTERFACE libraries */
            if (is_imported || is_alias || is_interface)
                goto next_cmd;

            for (int i = 1; i < argc; i++) {
                if (strcmp(args[i], "STATIC") == 0 || strcmp(args[i], "SHARED") == 0 ||
                    strcmp(args[i], "OBJECT") == 0 || strcmp(args[i], "MODULE") == 0 ||
                    strcmp(args[i], "EXCLUDE_FROM_ALL") == 0)
                    continue;

                /* this is a source file — add to sources list */
                const char *src = args[i];

                /* infer src_dir from first source */
                if (cfg->source_count == 0) {
                    char dir[MAX_PATH_LEN];
                    infer_src_dir(src, dir, sizeof(dir));
                    if (strcmp(dir, ".") != 0 && strcmp(cfg->src_dir, "src") == 0)
                        strncpy(cfg->src_dir, dir, MAX_PATH_LEN - 1);
                }

                /* add source with base_dir prefix */
                add_source(cfg, base_dir, src);
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
            for (int i = 1; i < argc; i++)
                map_lib_to_ldflag(cfg, args[i]);
        }

        /* ---- file(GLOB ...) / file(GLOB_RECURSE ...) ---- */
        else if (istarts_with(cmd_name, "file") && argc >= 3) {
            if (strcmp(args[0], "GLOB") == 0 || strcmp(args[0], "GLOB_RECURSE") == 0) {
                char combined[MAX_VAR_VAL] = {0};
                for (int i = 2; i < argc; i++) {
                    if (strcmp(args[i], "RELATIVE") == 0 || strcmp(args[i], "CONFIGURE_DEPENDS") == 0) {
                        if (strcmp(args[i], "RELATIVE") == 0) i++;
                        continue;
                    }
                    strip_quotes(args[i]);

                    char dir[MAX_PATH_LEN];
                    infer_src_dir(args[i], dir, sizeof(dir));
                    if (strcmp(dir, ".") != 0)
                        add_include(cfg, dir);

                    if (strlen(combined) > 0)
                        strncat(combined, " ", MAX_VAR_VAL - strlen(combined) - 1);
                    strncat(combined, args[i], MAX_VAR_VAL - strlen(combined) - 1);
                }
                if (argc >= 2)
                    var_set(vt, args[1], combined);
            }
        }

        /* ---- CHECK_INCLUDE_FILE() / check_include_files() ---- */
        else if ((iequals(cmd_name, "CHECK_INCLUDE_FILE") ||
                  iequals(cmd_name, "check_include_file") ||
                  iequals(cmd_name, "check_include_files")) && argc >= 2) {
            /* CHECK_INCLUDE_FILE("stdint.h" HAVE_STDINT_H)
               on POSIX systems, standard C headers always exist */
            const char *header = args[0];
            const char *var = args[argc - 1];

            /* known-good standard headers on macOS/Linux */
            static const char *known_headers[] = {
                "stdint.h", "inttypes.h", "strings.h", "unistd.h",
                "dlfcn.h", "fcntl.h", "limits.h", "sys/types.h",
                "sys/stat.h", "dirent.h", "errno.h", "signal.h",
                "string.h", "stdlib.h", "stdio.h", "math.h",
                "pthread.h", "sys/time.h", "iconv.h",
                NULL
            };

            int known = 0;
            for (int k = 0; known_headers[k]; k++) {
                if (iequals(header, known_headers[k])) {
                    known = 1;
                    break;
                }
            }

            if (known) {
                var_set(vt, var, "1");
                /* add -DVAR to cflags */
                char def[256];
                snprintf(def, sizeof(def), "-D%s", var);
                int cflen = (int)strlen(cfg->cflags);
                if (cflen > 0 && cflen < 254) {
                    cfg->cflags[cflen] = ' ';
                    cfg->cflags[cflen + 1] = '\0';
                }
                strncat(cfg->cflags, def, 255 - strlen(cfg->cflags));
            } else {
                var_set(vt, var, "0");
            }
        }

        /* ---- add_subdirectory() ---- */
        else if (iequals(cmd_name, "add_subdirectory") && argc > 0) {
            /* recurse into subdirectory CMakeLists.txt */
            char sub_dir[MAX_PATH_LEN];
            if (strcmp(base_dir, ".") == 0)
                snprintf(sub_dir, sizeof(sub_dir), "%s", args[0]);
            else
                snprintf(sub_dir, sizeof(sub_dir), "%s/%s", base_dir, args[0]);

            /* derive file path relative to where cmake_path lives */
            char parent_dir[MAX_PATH_LEN] = ".";
            const char *last_slash = strrchr(cmake_path, '/');
            if (last_slash) {
                int plen = (int)(last_slash - cmake_path);
                if (plen >= (int)sizeof(parent_dir)) plen = (int)sizeof(parent_dir) - 1;
                memcpy(parent_dir, cmake_path, plen);
                parent_dir[plen] = '\0';
            }

            char sub_cmake[MAX_PATH_LEN];
            snprintf(sub_cmake, sizeof(sub_cmake), "%s/%s/CMakeLists.txt",
                     parent_dir, args[0]);

            if (fs_exists(sub_cmake))
                parse_cmake_file(sub_cmake, sub_dir, cfg, vt);
        }

        /* ---- find_package() ---- */
        else if (istarts_with(cmd_name, "find_package") && argc > 0) {
            warn("Note", "find_package(%s) detected — may need manual ldflags", args[0]);
        }

        next_cmd:;
    }

    fclose(f);

    /* restore CMAKE_CURRENT_SOURCE_DIR */
    if (strlen(saved_cur_dir) > 0)
        var_set(vt, "CMAKE_CURRENT_SOURCE_DIR", saved_cur_dir);

    return 0;
}

/* ---- public API ---- */

int cmake_to_config(const char *cmake_path, Config *cfg) {
    config_default(cfg, "unnamed");
    cfg->include_count = 0;
    cfg->source_count = 0;
    cfg->cflags[0] = '\0';
    cfg->ldflags[0] = '\0';

    VarTable vt;
    memset(&vt, 0, sizeof(vt));
    var_set(&vt, "CMAKE_CURRENT_SOURCE_DIR", ".");
    var_set(&vt, "CMAKE_SOURCE_DIR", ".");
    var_set(&vt, "PROJECT_SOURCE_DIR", ".");
    var_set(&vt, "CMAKE_BINARY_DIR", ".");
    var_set(&vt, "PROJECT_BINARY_DIR", ".");
    var_set(&vt, "CMAKE_CURRENT_BINARY_DIR", ".");
    var_set(&vt, "CMAKE_C_COMPILER", "cc");

    if (parse_cmake_file(cmake_path, ".", cfg, &vt) != 0)
        return -1;

    /* ensure at least "." include if none found */
    if (cfg->include_count == 0)
        add_include(cfg, ".");

    /* prepend default cflags to any accumulated defines */
    if (strlen(cfg->cflags) > 0) {
        char existing[256];
        strncpy(existing, cfg->cflags, 255);
        existing[255] = '\0';
        snprintf(cfg->cflags, 256, "-Wall -Wextra -std=c11 %s", existing);
    } else {
        strncpy(cfg->cflags, "-Wall -Wextra -std=c11", 255);
    }

    /* add -lm as default for C projects (math library) */
    add_ldflag(cfg, "-lm");

    return 0;
}

int cmake_convert_file(const char *cmake_path, const char *yaml_path) {
    Config cfg;
    if (cmake_to_config(cmake_path, &cfg) != 0)
        return -1;
    return config_save(yaml_path, &cfg);
}
