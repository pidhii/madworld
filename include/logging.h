#ifndef LOGGING_H
#define LOGGING_H

#include <stdarg.h>
#include <stdio.h>

static void __attribute((format(printf, 1, 2)))
error(const char *fmt, ...)
{
  va_list args;
  fputs("\e[38;5;1;1mERROR\e[0m ", stdout);
  va_start(args, fmt);
  vfprintf(stdout, fmt, args);
  va_end(args);
  fputc('\n', stdout);
}

static void __attribute((format(printf, 1, 2)))
warning(const char *fmt, ...)
{
  va_list args;
  fputs("\e[38;5;3;1mWARNING\e[0m ", stdout);
  va_start(args, fmt);
  vfprintf(stdout, fmt, args);
  va_end(args);
  fputc('\n', stdout);
}

static void __attribute((format(printf, 1, 2)))
info(const char *fmt, ...)
{
  va_list args;
  fputs("\e[1mINFO\e[0m ", stdout);
  va_start(args, fmt);
  vfprintf(stdout, fmt, args);
  va_end(args);
  fputc('\n', stdout);
}


#endif
