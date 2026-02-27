#ifndef GOOSE_CC_CONFIG_H
#define GOOSE_CC_CONFIG_H

/* C consumer's language-specific config, stored in fw->custom_data */
typedef struct {
    char cc[64];
    char cflags[256];
    char ldflags[256];
} CConfig;

#endif
