#ifndef COMPAT_H
#define COMPAT_H
#ifdef _WIN32
int asprintf(char **strp, const char *fmt, ...);
int vasprintf(char **strp, const char *fmt, va_list ap);
char *strcasestr(const char *haystack, const char *needle);
int nanosleep(const struct timespec *req, struct timespec *rem);
#endif
#endif
