#ifndef LOGGING_H
#define LOGGING_H

#include <stdarg.h>
#include <stdio.h>

static void __attribute((format(printf, 1, 2)))
_error(const char *fmt, ...)
{
  va_list args;
  fputs("\e[38;5;1;1mERROR\e[0m ", stdout);
  va_start(args, fmt);
  vfprintf(stdout, fmt, args);
  va_end(args);
  fputc('\n', stdout);
}

static void __attribute((format(printf, 1, 2)))
_warning(const char *fmt, ...)
{
  va_list args;
  fputs("\e[38;5;3;1mWARNING\e[0m ", stdout);
  va_start(args, fmt);
  vfprintf(stdout, fmt, args);
  va_end(args);
  fputc('\n', stdout);
}

static void __attribute((format(printf, 1, 2)))
_info(const char *fmt, ...)
{
  va_list args;
  fputs("\e[1mINFO\e[0m ", stdout);
  va_start(args, fmt);
  vfprintf(stdout, fmt, args);
  va_end(args);
  fputc('\n', stdout);
}

#define error(fmt, ...) _error("%s:%d " fmt, __FILE_NAME__, __LINE__, ##__VA_ARGS__)
#define warning(fmt, ...) _warning("%s:%d " fmt, __FILE_NAME__, __LINE__, ##__VA_ARGS__)
#define info(fmt, ...) _info("%s:%d " fmt, __FILE_NAME__, __LINE__, ##__VA_ARGS__)

#endif
