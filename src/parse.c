#include "geometry.h"
#include "parse.h"

#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

/* 
 * Implement look up tables to optimize the to upper, to lower, is alnum, is digit functions
 * #define to_upper(x) (x & ~(x & 0x20))
 */

/*
 * FIXME move to lookup table with switch/case instead of if statements and built in functions if optimizing to get the 'class' of the char (space = 7, illegal = 27, digit = 2, ... ) like in example from sqlite: https://github.com/sqlite/sqlite/blob/f722d91fa8e8e6e03308736696d67e77b71e4b39/ext/misc/normalize.c#L77 
 */

#define RESERVED_WORDS_COUNT 6

static const char SET_KEYWORD[] = "SET";
static const char GET_KEYWORD[] = "GET";
static const char DELETE_KEYWORD[] = "DEL";
static const char DROP_KEYWORD[] = "DROP";
static const char POINT_KEYWORD[] = "POINT";
static const char BOUND_KEYWORD[] = "BOUND";

static const char *const RESERVED_WORDS[RESERVED_WORDS_COUNT] = {
  SET_KEYWORD,
  GET_KEYWORD,
  DELETE_KEYWORD,
  DROP_KEYWORD,
  POINT_KEYWORD,
  BOUND_KEYWORD,
};

typedef enum {
  TOKEN_ERROR,
  TOKEN_EMPTY,
  TOKEN_SPACE,
  TOKEN_INTEGER,
  TOKEN_DOUBLE,
  TOKEN_STRING
} TokenType;

static size_t get_next_token_len(const char *pch, TokenType *token_type) {
  size_t len;
  if (isspace(pch[0])) {
    for(len=1; isspace(pch[len]); len++) {}
    *token_type = TOKEN_SPACE;
  } else if (isdigit((unsigned char) pch[0]) || ((pch[0] == '.' || pch[0] == '+' || pch[0] == '-') && isdigit(pch[1]))) {
    // FIXME this is messy and has redundant checks but without a fallthrough of a case statement, we need to recheck why the statement was entered with something like `pch[0] == '.'` or `has_dot` boolean. This can be cleaned up when the above mentioned lookup table and switch/case optimzation is being implemented but for now, this is good enough. This also doesn't allow the ',' instead of '.' for decimal point. This is common internationally.
    *token_type = TOKEN_INTEGER; 
    bool has_dot = false;
    if (pch[0] == '.') {
      has_dot = true;
      *token_type = TOKEN_DOUBLE;
    }
    for(len=1; isdigit((unsigned char) pch[len]); len++) {} 
    if (pch[len] == '.') {
      if (has_dot || !isdigit(pch[len+1])) {
        *token_type = TOKEN_ERROR;
        return len;
      }
      *token_type = TOKEN_DOUBLE;
      len++;
      for(; isdigit((unsigned char) pch[len]); len++) {}
    }

    // only allow whole number exponential notation
    if (pch[len] == 'e' || pch[len] == 'E') {
      if (isdigit((unsigned char) pch[len+1]) || ((pch[len+1] == '-' || pch[len+1] == '+') && isdigit((unsigned char) pch[len+2]))) {
        *token_type = TOKEN_DOUBLE;
        len+=2;
        for(; isdigit((unsigned char) pch[len]); len++) {}
      } else {
        *token_type = TOKEN_ERROR;
        return len;
      }
    }
  } else if (pch[0] == '\'' || pch[0] == '`' || pch[0] == '"') {
    *token_type = TOKEN_STRING;
    char delim = pch[0];
    
    for(len=1; pch[len]!= '\0'; len++) {
      if (pch[len] == delim) {
        len++;
        break; 
      }
    }
    if (pch[len] == '\0') {
      // exited for statement due to being at end of string without finding delim
      *token_type = TOKEN_ERROR;
    }
  } else {
    for(len=1; !isspace(pch[len]); len++) {}
    *token_type = TOKEN_STRING;
  }
  return len;
}

typedef enum {
  UNKNOWN_STEP,
  KEY,
  ID,
  BOUNDS_OR_POINT,
  BOUNDS,
  POINT,
  X_VALUE,
  Y_VALUE,
  Z_VALUE,
} Step;

/*
#define STEPS_ENUM_COUNT 9

static const char * const STEP_TO_STRING[STEPS_ENUM_COUNT] = {
  "UNKNOWN_STEP",
  "KEY",
  "ID",
  "BOUNDS_OR_POINT",
  "BOUNDS",
  "POINT",
  "X_VALUE",
  "Y_VALUE",
  "Z_VALUE",
};
*/

typedef enum {
  PARSE_OK,                                          // 0
  OUT_OF_MEMORY,                                     // 1
  INVALID_TOKEN,                                     // 2
  INVALID_COMMAND_TYPE,                              // 3
  INVALID_KEY_VALUE,                              // 3
  INVALID_ID_VALUE,                              // 3
  INVALID_STEP_ERROR,                                // 4
  INVALID_BOUNDS_OR_POINT,
  INVALID_X_VALUE,
  INVALID_Y_VALUE,
  INVALID_Z_VALUE,
  END_OF_TOKENS_REACHED,                             // 5
  EXPECTED_END_OF_TOKENS,                            // 6
} ParserResult;

#define PARSER_RESULTS_ENUM_COUNT // TODO

// TODO make this match ParseResult enum
static const char * const PARSER_RESULT_TO_STRING[PARSER_RESULTS_ENUM_COUNT] = {
  "PARSE_OK",
  "OUT_OF_MEMORY",
  "INVALID_TOKEN",
  "INVALID_COMMAND_TYPE",
  "INVALID_KEY_VALUE",
  "INVALID_ID_VALUE",
  "INVALID_STEP_ERROR",
  "INVALID_BOUNDS_OR_POINT",
  "INVALID_X_VALUE",
  "INVALID_Y_VALUE",
  "INVALID_Z_VALUE",
  "END_OF_TOKENS_REACHED",
  "EXPECTED_END_OF_TOKENS"
};

#define ERROR_MESSAGE_MAX_BUFFER_SIZE 512

/*
 * Function that is responsible for formatting the error message which will be sent to the user supplied error_callback function. NOTICE: buffer is a stack allocated char array (to avoid any alloc issues) so if the user wants to maintain the error message for longer, the error_callback function should make a copy of the error_message for the user to use.
 *
 * Params:
 *  - `error_call ec` user supplied error_callback function, possibly used to log to stderr.
 *  - `int error_code` the error_code associated with the reason for the error
 *  - `const char * cause` a short error cause message such as "failed to allocate memory for ___" or "Invalid token in sql string"
 *  - `size_t position` position in the sql string when the error started. If an error happened before the statement is being tokenized and parsed, this will be 0.
 *
 * Returns: void
 */
static void internal_error_callback_handler(error_callback ec, int error_code, const char * cause, int position) {
  // empty 512 byte buffer.
  char buffer[ERROR_MESSAGE_MAX_BUFFER_SIZE] = {'\0'};

  int bytes_written = snprintf(buffer, ERROR_MESSAGE_MAX_BUFFER_SIZE, "%s(%d) - cause: %s. position in statement %d.", PARSER_RESULT_TO_STRING[error_code], error_code, cause, position);
  if (bytes_written >= 0 || bytes_written < ERROR_MESSAGE_MAX_BUFFER_SIZE) {
    ec(error_code, buffer);
  } else {
    ec(error_code, "FAILED_TO_FORMAT_ERROR_MESSAGE\n");
  }
}

/*
 * returns 0 if `word` is in the reserved word list, else 1.
 * NOTE doesn't account for negative ptrdiff_t values
 */
static int is_reserved_word(const char *pch, size_t len) {
  for (int i = 0; i < RESERVED_WORDS_COUNT; i++) {
    // check case insensitive compare
    if (strncmpci(pch, RESERVED_WORDS[i], len) == 0) {
      return 0;
    }
  }
  return 1;
}

/*
 * creates a prepared statement from a given command string.
 *
 * parameters:
 *  - `cmd` is command to be transformed into a PreparedStatement
 *  - `prepared_statement` out param which contains the fully created PreparedStatement from the command string. If the function returns a non-zero value, this value is NULL.
 *  - `error_callback` function of type `void FUNC_NAME(int error_code, const char *error_message)` which is used to handle any logging that the user wants to do.
 *
 * returns a int (ParserResult enum value). Follows c common practice of:
 *  - 0 if successful
 *  - else, error where the return value matches the error_code (reason of failure).
 *
 */
int make_prepared_statement(const char *cmd, PreparedStatement *prepared_statement, error_callback ec_func) {
  
  TokenType tt = 0;
  size_t len;
  const char *cursor = cmd;
  Point *cur_point;
  Step step = UNKNOWN_STEP;
  
  while (*cursor != '\0') {
    len = get_next_token_len(cursor, &tt);
    if (tt == TOKEN_ERROR) {
      if (ec_func != NULL) {
        internal_error_callback_handler(ec_func, INVALID_TOKEN, "Invalid token in statement", (cursor-cmd));
      }
      return INVALID_TOKEN;
    }

    if (tt == TOKEN_SPACE) {
      cursor += len;
      continue;
    }

    switch(step) {
      case UNKNOWN_STEP: {
        if (strncmpci(cursor, GET_KEYWORD, len) == 0) {
          prepared_statement->command_type = GET;
          step = KEY;
        } else if (strncmpci(cursor, SET_KEYWORD, len) == 0) {
          prepared_statement->command_type = SET;
          step = KEY;
        } else if (strncmpci(cursor, DELETE_KEYWORD, len) == 0) {
          prepared_statement->command_type = DELETE;
          step = KEY;
        } else if (strncmpci(cursor, DROP_KEYWORD, len) == 0) {
          prepared_statement->command_type = DROP;
          step = KEY;
        }
        // didn't set next step
        if (step != KEY) {
          if (ec_func != NULL) {
            internal_error_callback_handler(ec_func, INVALID_COMMAND_TYPE, "Invalid command type keyword", (cursor-cmd));
          }
          return INVALID_COMMAND_TYPE;
        }
        // advance cursor by len
        cursor += len;
        break;
      }
      case KEY: {
        if (tt == TOKEN_INTEGER || tt == TOKEN_DOUBLE || ((tt == TOKEN_STRING) && (is_reserved_word(cursor, len) != 0))) {
          /* Changing to Span
          char *key = malloc(sizeof(char) * len + 1);
          if (key == NULL) {
            if (ec_func != NULL) {
              internal_error_callback_handler(ec_func, OUT_OF_MEMORY, "Malloc returned NULL for key.", (cursor-cmd));
            }
            return OUT_OF_MEMORY;
          }
          
          strncpy(key, cursor, len);
          key[len] = '\0';
          prepared_statement->key = key;
          */
          prepared_statement->key = (Span){ .start = cursor, .length = len };
        } else {
          if (ec_func != NULL) {
            internal_error_callback_handler(ec_func, INVALID_KEY_VALUE, "Invalid key value", (cursor-cmd));
          }
          return INVALID_KEY_VALUE;
        }
        // increment cursor
        cursor += len;
        step = ID;
        break;
      }
      case ID: {
        // drop commands shouldn't have an ID
        if (prepared_statement->command_type == DROP) {
          if (ec_func != NULL) {
            internal_error_callback_handler(ec_func, EXPECTED_END_OF_TOKENS, "Expected end of tokens in drop statement.", (cursor-cmd));
          }
          return EXPECTED_END_OF_TOKENS;
        }

        if (tt == TOKEN_INTEGER || tt == TOKEN_DOUBLE || (tt == TOKEN_STRING && (is_reserved_word(cursor, len) != 0))) {
          /* Changing to Span
          char *id = malloc(sizeof(char) * len + 1);
          if (id == NULL) {
            if (ec_func != NULL) {
              internal_error_callback_handler(ec_func, OUT_OF_MEMORY, "Malloc returned NULL for id.", (cursor-cmd));
            }
            return OUT_OF_MEMORY;
          }
          strncpy(id, cursor, len);
          id[len] = '\0';
          prepared_statement->id = id;
          */

          prepared_statement->key = (Span){ .start = cursor, .length = len };
        } else {
          if (ec_func != NULL) {
            internal_error_callback_handler(ec_func, INVALID_ID_VALUE, "Invalid id value", (cursor-cmd));
          }
          return INVALID_ID_VALUE;
        }


        cursor += len;
        step = BOUNDS_OR_POINT;
        break;
      }
      case BOUNDS_OR_POINT: {
        if (prepared_statement->command_type == GET || prepared_statement->command_type == DELETE) {
          if (ec_func != NULL) {
            internal_error_callback_handler(ec_func, EXPECTED_END_OF_TOKENS, "Expected end of tokens in GET/DELETE statement.", (cursor-cmd));
          }
          return EXPECTED_END_OF_TOKENS;
        } else {
          

          if ((strncmpci(cursor, BOUND_KEYWORD, len) != 0) && (strncmpci(cursor, POINT_KEYWORD, len) != 0)) {
            if (ec_func != NULL) {
              internal_error_callback_handler(ec_func, INVALID_BOUNDS_OR_POINT, "Expected BOUND or POINT keyword", (cursor-cmd));
            }
            return INVALID_BOUNDS_OR_POINT;
          }
        }

        // FIXME uses heap :(
        cur_point = malloc(sizeof(Point));
        if (cur_point == NULL) {
          if (ec_func != NULL) {
            internal_error_callback_handler(ec_func, OUT_OF_MEMORY, "Malloc returned NULL for point.", (cursor-cmd));
          }
          return OUT_OF_MEMORY;
        }

        step = X_VALUE;
        cursor += len;
        break;
      }
      case X_VALUE: {
        if (tt != TOKEN_DOUBLE && tt != TOKEN_INTEGER) {
          if (ec_func != NULL) {
            internal_error_callback_handler(ec_func, INVALID_X_VALUE, "Expected integer or double x value", (cursor-cmd));
          }
          return INVALID_X_VALUE;
        }
        char *tail;
        errno = 0;
        double val = strtod(cursor, &tail);        

        // check if errno got set (overflow happened).
        if (errno != 0) {
          if (ec_func != NULL) {
            // FIXME strerror -> strerror_s?
            internal_error_callback_handler(ec_func, INVALID_X_VALUE, strerror(errno), (cursor-cmd));
          }
          return INVALID_X_VALUE;
        }

        //check we get the whole expected number parsed.
        if (*tail != *(cursor + len)) {
          if (ec_func != NULL) {
            internal_error_callback_handler(ec_func, INVALID_X_VALUE, "Failed to parse entire x value.", (cursor-cmd));
          }
          return INVALID_X_VALUE;
        }

        cur_point->x = val;
        cursor += len;
        step = Y_VALUE;
        break;
      }
      case Y_VALUE: {
        if (tt != TOKEN_DOUBLE && tt != TOKEN_INTEGER) {
          if (ec_func != NULL) {
            internal_error_callback_handler(ec_func, INVALID_Y_VALUE, "Expected integer or double y value", (cursor-cmd));
          }
          return INVALID_Y_VALUE;
        }
        char *tail;
        errno = 0;
        double val = strtod(cursor, &tail);        

        // check if errno got set (overflow happened).
        if (errno != 0) {
          if (ec_func != NULL) {
            // FIXME strerror -> strerror_s?
            internal_error_callback_handler(ec_func, INVALID_Y_VALUE, strerror(errno), (cursor-cmd));
          }
          return INVALID_Y_VALUE;
        }

        //check we get the whole expected number parsed.
        if (*tail != *(cursor + len)) {
          if (ec_func != NULL) {
            internal_error_callback_handler(ec_func, INVALID_Y_VALUE, "Failed to parse entire y value.", (cursor-cmd));
          }
          return INVALID_Y_VALUE;
        }

        cur_point->y = val;
        cursor += len;
        step = Z_VALUE;
        break;
      }
      case Z_VALUE: {
        cur_point->has_z = true;
        if (tt != TOKEN_DOUBLE && tt != TOKEN_INTEGER) {
          if (ec_func != NULL) {
            internal_error_callback_handler(ec_func, INVALID_Z_VALUE, "Expected integer or double z value", (cursor-cmd));
          }
          return INVALID_Z_VALUE;
        }
        char *tail;
        errno = 0;
        double val = strtod(cursor, &tail);        

        // check if errno got set (overflow happened).
        if (errno != 0) {
          if (ec_func != NULL) {
            // FIXME strerror -> strerror_s?
            internal_error_callback_handler(ec_func, INVALID_Z_VALUE, strerror(errno), (cursor-cmd));
          }
          return INVALID_Z_VALUE;
        }

        //check we get the whole expected number parsed.
        if (*tail != *(cursor + len)) {
          if (ec_func != NULL) {
            internal_error_callback_handler(ec_func, INVALID_Z_VALUE, "Failed to parse entire z value.", (cursor-cmd));
          }
          return INVALID_Z_VALUE;
        }

        cur_point->y = val;
        cursor += len;
        step = Z_VALUE;
        break;
      }

      default: {
        if (ec_func != NULL) {
          internal_error_callback_handler(ec_func, INVALID_STEP_ERROR, "Invalid step value", (cursor-cmd));
        }
        return INVALID_STEP_ERROR;
      }
    }
  }
  return PARSE_OK;
}
