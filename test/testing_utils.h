#ifndef TESTING_UTILS_H
#define TESTING_UTILS_H

#include "parser.h"

int RESULT_EQ(Result *expected, Result *actual);
int RESULT_TEST(char *label, char *input, Result *expected);

#endif
