// compat.c
#ifdef _WIN32
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "compat.h"

int vasprintf(char **strp, const char *fmt, va_list ap) {
    va_list ap_copy;
    va_copy(ap_copy, ap);
    int len = _vscprintf(fmt, ap_copy);
    va_end(ap_copy);
    if (len < 0) return -1;

    char *buf = (char *)malloc(len + 1);
    if (!buf) return -1;

    vsprintf_s(buf, len + 1, fmt, ap);
    *strp = buf;
    return len;
}

int asprintf(char **strp, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int len = vasprintf(strp, fmt, ap);
    va_end(ap);
    return len;
}

char *strcasestr(const char *haystack, const char *needle) {
    if (!haystack || !needle) return NULL;
    size_t needle_len = strlen(needle);
    for (; *haystack; haystack++) {
        if (_strnicmp(haystack, needle, needle_len) == 0)
            return (char *)haystack;
    }
    return NULL;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    (void)rem;
    Sleep((DWORD)(req->tv_sec * 1000 + req->tv_nsec / 1000000));
    return 0;
}
#endif
