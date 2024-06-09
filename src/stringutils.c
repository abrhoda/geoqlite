#include "stringutils.h"
#include <ctype.h>

/*
 * case insensitive string comparison of strings a and b.
 *
 * returns 0 if case insensitive compare is equal. any other values is not
 * equal.
 */
int strncmpci(char const *a, char const *b, size_t n) {
  while (n && *a &&
         (tolower(*(unsigned char *)a) == tolower(*(unsigned char *)b))) {
    a++;
    b++;
    n--;
  }
  if (n == 0) {
    return 0;
  } else {
    return (tolower(*(unsigned char *)a) - tolower(*(unsigned char *)b));
  }
}
