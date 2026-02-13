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

typedef struct {
    char name[MAX_NAME_LEN];
    char version[64];
    char description[256];
    char author[MAX_NAME_LEN];
    char license[64];
    char src_dir[MAX_PATH_LEN];
    char build_dir[MAX_PATH_LEN];
    char cc[64];
    char cflags[256];
    char ldflags[256];
    char includes[MAX_INCLUDES][MAX_PATH_LEN];
    int include_count;
    Dependency deps[MAX_DEPS];
    int dep_count;
} Config;

int  config_load(const char *path, Config *cfg);
int  config_save(const char *path, const Config *cfg);
void config_default(Config *cfg, const char *name);

#endif
