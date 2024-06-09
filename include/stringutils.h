#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <stddef.h>

int strncmpci(char const *a, char const *b, size_t n);

typedef struct {
  const char *start;
  size_t length;
} Span;

#endif

