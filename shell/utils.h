#ifndef UTILS_H
#define UTILS_H

#include "defs.h"

void lstrip(char **str);
void dispatch_args(char *args[], char *str);
void strcpy_until(char *dst, char *src, char end);

char *split_line(char *buf, char splitter);

int block_contains(char *buf, char c);

int printf_debug(char *format, ...);
int fprintf_debug(FILE *file, char *format, ...);

#endif  // UTILS_H
