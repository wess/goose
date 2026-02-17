#ifndef GOOSE_CONFIG_H
#define GOOSE_CONFIG_H

#define MAX_DEPS      64
#define MAX_NAME_LEN  128
#define MAX_PATH_LEN  512
#define MAX_SRC_FILES 256
#define MAX_INCLUDES  32

typedef struct {
    char name[MAX_NAME_LEN];
    char git[MAX_PATH_LEN];
    char version[64];
} Dependency;

#define MAX_PLUGINS   16
#define MAX_EXT_LEN   16
#define MAX_CMD_LEN   256

typedef struct {
    char name[MAX_NAME_LEN];
    char ext[MAX_EXT_LEN];
    char command[MAX_CMD_LEN];
} Plugin;

typedef struct {
    char name[MAX_NAME_LEN];
    char version[64];
    char description[256];
    char author[MAX_NAME_LEN];
    char license[64];
    char src_dir[MAX_PATH_LEN];
    char cc[64];
    char cflags[256];
    char ldflags[256];
    char includes[MAX_INCLUDES][MAX_PATH_LEN];
    int include_count;
    char sources[MAX_SRC_FILES][MAX_PATH_LEN];
    int source_count;
    Dependency deps[MAX_DEPS];
    int dep_count;
    Plugin plugins[MAX_PLUGINS];
    int plugin_count;
} Config;

int  config_load(const char *path, Config *cfg);
int  config_save(const char *path, const Config *cfg);
void config_default(Config *cfg, const char *name);

#endif
