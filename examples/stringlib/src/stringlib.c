#include <stringlib.h>
#include <ctype.h>
#include <string.h>

size_t str_len(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

char *str_copy(char *dst, const char *src) {
    char *d = dst;
    while ((*d++ = *src++));
    return dst;
}

int str_equal(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == *b;
}

int str_starts_with(const char *haystack, const char *prefix) {
    while (*prefix) {
        if (*haystack != *prefix) return 0;
        haystack++; prefix++;
    }
    return 1;
}

int str_ends_with(const char *haystack, const char *suffix) {
    size_t hlen = str_len(haystack);
    size_t slen = str_len(suffix);
    if (slen > hlen) return 0;
    return str_equal(haystack + hlen - slen, suffix);
}

char *str_upper(char *s) {
    for (char *p = s; *p; p++)
        *p = (char)toupper((unsigned char)*p);
    return s;
}

char *str_lower(char *s) {
    for (char *p = s; *p; p++)
        *p = (char)tolower((unsigned char)*p);
    return s;
}

char *str_trim(char *s) {
    char *start = s;
    while (isspace((unsigned char)*start)) start++;

    if (start != s)
        memmove(s, start, strlen(start) + 1);

    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end))
        *end-- = '\0';

    return s;
}
