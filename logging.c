#include <stdarg.h>
#include <stdio.h>

#include "logging.h"

static int _lvl = 0;

static const char *level_name[] = {
  "ERROR",
  "WARN",
  "INFO",
  "TRACE",
  "DEBUG"
};

void set_level(int lvl) {
  _lvl = lvl;
}

void _emit(int lvl, char *fmt, va_list args) {
  if (lvl > DEBUG) lvl = DEBUG;
  if (lvl < ERROR) lvl = ERROR;
  if (_lvl >= lvl) {
    printf("%s: ", level_name[lvl]); 
    vprintf(fmt, args);
    printf("\n");
  }
}

void debug(char *fmt, ...) {
  va_list arglist;
  va_start(arglist, *fmt);
  _emit(DEBUG, fmt, arglist);
}

void trace(char *fmt, ...) {
  va_list arglist;
  va_start(arglist, *fmt);
  _emit(TRACE, fmt, arglist);
}

void info(char *fmt, ...) {
  va_list arglist;
  va_start(arglist, *fmt);
  _emit(INFO, fmt, arglist);
}

void warn(char *fmt, ...) {
  va_list arglist;
  va_start(arglist, *fmt);
  _emit(WARN, fmt, arglist);
}

void error(char *fmt, ...) {
  va_list arglist;
  va_start(arglist, *fmt);
  _emit(ERROR, fmt, arglist);
}
