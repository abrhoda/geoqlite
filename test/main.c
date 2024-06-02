#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "testing_utils.h"

int main(void) {
  printf("** STARTING TEST CASES **\n");

  int failed = 0;
  /*
  {
    char *input = NULL;
    failed += RESULT_TEST("Tokenize returns NULL for NULL input", input, NULL);
  }
  */
    
  
  // TODO add test for invalid column names like columnname#1 and columnname_
  // TODO null input
  // TODO check fields, rows, conditions, and aliases limit
  // TODO test wrong sql statements - Where with no operator, SELECT without
  // table etc.
  // TODO test `bool validate(Result *result)` function

  return 0;
}
