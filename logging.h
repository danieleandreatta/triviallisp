#ifndef __LOGGING_H__
#define __LOGGING_H__

#define ERROR 0
#define WARN 1
#define INFO 2
#define TRACE 3
#define DEBUG 4

void set_level(int lvl);
void debug(char *fmt, ...);
void trace(char *fmt, ...);
void info(char *fmt, ...);
void warn(char *fmt, ...);
void error(char *fmt, ...);

#endif
