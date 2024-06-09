#ifndef PARSE_H
#define PARSE_H

#include "stringutils.h"

typedef enum {
  DELETE,
  GET,
  SET,
  DROP
} CommandType;

typedef struct {
  CommandType command_type;
  Span key;
  Span id;
} PreparedStatement;


typedef void (*error_callback)(int error_code, const char *error_message);
int make_prepared_statement(const char *cmd, PreparedStatement *prepared_statement, error_callback ec_func);

#endif
