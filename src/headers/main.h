#ifndef GOOSE_MAIN_H
#define GOOSE_MAIN_H

#ifdef GOOSE_VERSION_FROM_FILE
#define GOOSE_VERSION GOOSE_VERSION_FROM_FILE
#else
#define GOOSE_VERSION "0.0.0"
#endif
#define GOOSE_CONFIG  "goose.yaml"
#define GOOSE_LOCK    "goose.lock"
#define GOOSE_PKG_DIR "packages"
#define GOOSE_BUILD   "build"

#endif
