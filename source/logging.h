#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <stdarg.h>
#include "config.h"

extern void error_log(const char *format, ...);

#endif
