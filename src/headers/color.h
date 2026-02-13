#ifndef GOOSE_COLOR_H
#define GOOSE_COLOR_H

#include <stdio.h>
#include <unistd.h>

#define CLR_RESET  "\033[0m"
#define CLR_BOLD   "\033[1m"
#define CLR_RED    "\033[31m"
#define CLR_GREEN  "\033[32m"
#define CLR_YELLOW "\033[33m"
#define CLR_CYAN   "\033[36m"

#define USE_COLOR (isatty(STDOUT_FILENO))

#define cprintf(color, fmt, ...) do { \
    if (USE_COLOR) \
        printf(CLR_BOLD color fmt CLR_RESET, ##__VA_ARGS__); \
    else \
        printf(fmt, ##__VA_ARGS__); \
} while(0)

#define ceprintf(color, fmt, ...) do { \
    if (isatty(STDERR_FILENO)) \
        fprintf(stderr, CLR_BOLD color fmt CLR_RESET, ##__VA_ARGS__); \
    else \
        fprintf(stderr, fmt, ##__VA_ARGS__); \
} while(0)

#define info(tag, fmt, ...)  do { cprintf(CLR_GREEN,  "%12s ", tag); printf(fmt "\n", ##__VA_ARGS__); } while(0)
#define warn(tag, fmt, ...)  do { cprintf(CLR_YELLOW, "%12s ", tag); printf(fmt "\n", ##__VA_ARGS__); } while(0)
#define err(fmt, ...)        do { ceprintf(CLR_RED, "%12s ", "error"); fprintf(stderr, fmt "\n", ##__VA_ARGS__); } while(0)

#endif
