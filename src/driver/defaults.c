#include <string.h>
#include "../headers/config.h"

void config_default(Config *cfg, const char *name) {
    memset(cfg, 0, sizeof(Config));
    strncpy(cfg->name, name, MAX_NAME_LEN - 1);
    strncpy(cfg->version, "0.1.0", 63);
    strncpy(cfg->description, "", 255);
    strncpy(cfg->author, "", MAX_NAME_LEN - 1);
    strncpy(cfg->license, "MIT", 63);
    strncpy(cfg->src_dir, "src", MAX_PATH_LEN - 1);
    strncpy(cfg->cc, "", 63);
    strncpy(cfg->cflags, "", 255);
    strncpy(cfg->ldflags, "", 255);
    strncpy(cfg->includes[0], "src", MAX_PATH_LEN - 1);
    cfg->include_count = 1;
}
