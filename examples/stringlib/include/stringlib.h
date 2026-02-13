#ifndef STRINGLIB_H
#define STRINGLIB_H

#include <stddef.h>

/* Return length of string (without null terminator) */
size_t str_len(const char *s);

/* Copy src into dst, return dst. dst must have enough space. */
char *str_copy(char *dst, const char *src);

/* Return 1 if strings are equal, 0 otherwise */
int str_equal(const char *a, const char *b);

/* Return 1 if haystack starts with prefix */
int str_starts_with(const char *haystack, const char *prefix);

/* Return 1 if haystack ends with suffix */
int str_ends_with(const char *haystack, const char *suffix);

/* Convert string to uppercase in-place, return s */
char *str_upper(char *s);

/* Convert string to lowercase in-place, return s */
char *str_lower(char *s);

/* Trim leading and trailing whitespace in-place, return s */
char *str_trim(char *s);

#endif
