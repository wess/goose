#include <stdio.h>
#include <string.h>
#include "config.h"
#include "../headers/config.h"

/* config defaults callback: set cc, cflags, ldflags */
void c_config_defaults(Config *cfg, void *custom_data, void *userdata) {
    (void)cfg;
    (void)userdata;
    CConfig *cc = (CConfig *)custom_data;
    strncpy(cc->cc, "cc", sizeof(cc->cc) - 1);
    strncpy(cc->cflags, "-Wall -Wextra -std=c11", sizeof(cc->cflags) - 1);
    cc->ldflags[0] = '\0';
}

/* config parse callback: handle cc, cflags, ldflags in build section */
int c_config_parse(const char *section, const char *key, const char *val,
                   void *custom_data, void *userdata) {
    (void)section;
    (void)userdata;
    CConfig *cc = (CConfig *)custom_data;

    if (strcmp(key, "cc") == 0)
        strncpy(cc->cc, val, sizeof(cc->cc) - 1);
    else if (strcmp(key, "cflags") == 0)
        strncpy(cc->cflags, val, sizeof(cc->cflags) - 1);
    else if (strcmp(key, "ldflags") == 0)
        strncpy(cc->ldflags, val, sizeof(cc->ldflags) - 1);

    return 0;
}

/* config write callback: emit cc, cflags, ldflags in build section */
int c_config_write(FILE *f, const void *custom_data, void *userdata) {
    (void)userdata;
    const CConfig *cc = (const CConfig *)custom_data;

    fprintf(f, "  cc: \"%s\"\n", cc->cc);
    fprintf(f, "  cflags: \"%s\"\n", cc->cflags);
    if (strlen(cc->ldflags) > 0)
        fprintf(f, "  ldflags: \"%s\"\n", cc->ldflags);

    return 0;
}
