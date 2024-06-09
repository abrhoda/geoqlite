#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
//#include "testing_utils.h"

void stderr_logger(int ec, const char *msg) {

  fprintf(stderr, "Error_code %d: msg: %s\n", ec, msg);
}
int main(void) {
  printf("** STARTING TEST CASES **\n");

  int failed = 0;

  //char *s1 = "SET fleet 1 POINT +1 -2.0";
  //char *s2 = "SET fleet 2 POINT +1 -2.0 3e2";
  char *s3 = "SET fleet b1 BOUNDS +1 -2.0 3.34 -1e2";

  PreparedStatement ps;
  int rc;
  rc = make_prepared_statement(s3, &ps, stderr_logger);
  printf("%s\n", s3);
  printf("rc = %d\n", rc);

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
