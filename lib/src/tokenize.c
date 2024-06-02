//#include "tokenize.h"
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>


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
static const char BOUNDS_KEYWORD[] = "BOUNDS";

static const char *const RESERVED_WORDS[RESERVED_WORDS_COUNT] = {
  SET_KEYWORD,
  GET_KEYWORD,
  DELETE_KEYWORD,
  DROP_KEYWORD,
  POINT_KEYWORD,
  BOUNDS_KEYWORD,
};

typedef enum {
  TOKEN_ERROR,
  TOKEN_EMPTY,
  TOKEN_SPACE,
  TOKEN_INTEGER,
  TOKEN_FLOAT,
  TOKEN_STRING
} TokenType;

static size_t get_next_token_len(const char *pch, TokenType *token_type) {
  size_t len;
  if (isspace(pch[0])) {
    for(len=1; isspace(pch[len]); len++) {}
    *token_type = TOKEN_SPACE;
  } else if (isdigit((unsigned char) pch[0]) || ((pch[0] == '.' || pch[0] == '-') && isdigit(pch[1]))) {
    // FIXME this is messy and has redundant checks but without a fallthrough of a case statement, we need to recheck why the statement was entered with something like `pch[0] == '.'` or `has_dot` boolean. This can be cleaned up when the above mentioned lookup table and switch/case optimzation is being implemented but for now, this is good enough. This also doesn't allow the ',' instead of '.' for floating point. This is common internationally.
    *token_type = TOKEN_INTEGER; 
    bool has_dot = false;
    if (pch[0] == '.') {
      has_dot = true;
      *token_type = TOKEN_FLOAT;
    }
    for(len=1; isdigit((unsigned char) pch[len]); len++) {} 
    if (pch[len] == '.') {
      if (has_dot || !isdigit(pch[len+1])) {
        *token_type = TOKEN_ERROR;
        return len;
      }
      *token_type = TOKEN_FLOAT;
      len++;
      for(; isdigit((unsigned char) pch[len]); len++) {}
    }

    // only allow whole number exponential notation
    if (pch[len] == 'e' || pch[len] == 'E') {
      if (isdigit((unsigned char) pch[len+1]) || ((pch[len+1] == '-' || pch[len+1] == '+') && isdigit((unsigned char) pch[len+2]))) {
        *token_type = TOKEN_FLOAT;
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
  DROP_KEY, //
  ID,
  POINT,
  X_VALUE,
  Y_VALUE,
  Z_VALUE,
  BOUNDS,
} Step;

#define STEPS_ENUM_COUNT 12

static const char * const STEP_TO_STRING[STEPS_ENUM_COUNT] = {
  "UNKNOWN_STEP",
  "KEY",
  "ID",
  "POINT",
  "X_VALUE,
  "Y_VALUE",
  "Z_VALUE",
  "BOUNDS",
};



typedef enum {
  PARSE_OK,                                          // 0
  OUT_OF_MEMORY,                                     // 1
  INVALID_TOKEN,                                     // 2
  INVALID_COMMAND_TYPE,                              // 3
  INVALID_KEY_VALUE,                              // 3
  INVALID_ID_VALUE,                              // 3
  INVALID_STEP_ERROR,                                // 4
  END_OF_TOKENS_REACHED,                             // 5
  EXPECTED_END_OF_TOKENS,                            // 6
} ParserResult;

#define PARSER_RESULTS_ENUM_COUNT // TODO

static const char * const PARSER_RESULT_TO_STRING[PARSER_RESULTS_ENUM_COUNT] = {
  "PARSE_OK",
  "OUT_OF_MEMORY",
  "INVALID_TOKEN",
  "INVALID_COMMAND_TYPE",
  "INVALID_KEY_VALUE",
  "INVALID_ID_VALUE",
  "INVALID_STEP_ERROR",
  "END_OF_TOKENS_REACHED",
  "EXPECTED_END_OF_TOKENS"
};

#define ERROR_MESSAGE_MAX_BUFFER_SIZE 512
typedef void (*error_callback)(int error_code, const char *error_message);

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

  int bytes_written = snprint(buffer, ERROR_MESSAGE_MAX_BUFFER_SIZE, "%s(%d) - cause: %s. position in statement %d.", PARSER_RESULT_TO_STRING[error_code], error_code, cause, position);
  if (bytes_written >= 0 || bytes_written < ERROR_MESSAGE_MAX_BUFFER_SIZE) {
    ec(error_code, buffer);
  } else {
    ec(error_code, "FAILED_TO_FORMAT_ERROR_MESSAGE\n");
  }
}

typedef enum {
  DELETE,
  GET,
  SET,
  DROP
} CommandType;

typedef struct {
  float lat;
  float lon;
  float z;
} Point;

typedef struct {
  CommandType command_type;
  char *key;
  char *id;
  char **values;
  size_t values_count;
} PreparedStatement;


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

/*
 * returns 0 if `word` is in the reserved word list, else 1.
 * NOTE doesn't account for negative ptrdiff_t values
 */
bool is_reserved_word(const char *pch, size_t len) {
  for (int i = 0; i < RESERVED_WORDS_COUNT; i++) {
    // check lengths match
    if (len != strlen(RESERVED_WORDS[i])) {
      continue;
    }

    // check case insensitive compare
    if (strncmpci(pch, RESERVED_WORDS[i], len) == 0) {
      return true;
    }
  }
  return false;
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
static int make_prepared_statement(const char *cmd, *PreparedStatement prepared_statement, *error_callback ec_func) {
  
  TokenType tt;
  size_t len;
  char *cursor = cmd;
  Step step = UNKNOWN_STEP;
  while (*cursor != '\0') {
    len = get_next_token_len(cursor, &tt);
    printf("[DEBUG] Got token of type %d and value '%.*s'\n", tt, (int) len, cursor);
    if (tt = TOKEN_ERROR) {
      if (ec_func != NULL) {
        internal_error_callback_handler(ec_func, INVALID_TOKEN, "Invalid token in sql statement", (cursor-cmd));
      }
      return INVALID_TOKEN;
    }

    if (tt == TOKEN_SPACE) {
      printf("[DEBUG] Token was a space. Continuing to next token.\n");
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
          step = DROP_KEY;
        }
        // didn't set next step
        if (step != KEY || step != DROP_KEY) {
          if (ec_func != NULL) {
            internal_error_callback_handler(ec_func, INVALID_COMMAND_TYPE, "Invalid command type keyword", (cursor-cmd));
          }
          return INVALID_COMMAND_TYPE;
        }
        // advance cursor by len
        cursor += len;
        break;
      }
      case DROP_KEY:
      case KEY: {
        if (tt == TOKEN_INTEGER || tt == TOKEN_FLOAT || (tt == TOKEN_STRING && (is_reserved_word(cursor, len) != 0))) {
          char *key = (char *)malloc(sizeof(char) * len + 1);
          if (key == NULL) {
            if (ec_func != NULL) {
              internal_error_callback_handler(ec_func, OUT_OF_MEMORY, "Malloc returned NULL for key.", (cursor-cmd));
            }
            return OUT_OF_MEMORY;
          }
          strncpy(key, cursor, len);
          key[len] = '\0';
          prepared_statement->key = key;

        } else {
          if (ec_func != NULL) {
            internal_error_callback_handler(ec_func, INVALID_KEY_VALUE, "Invalid key value", (cursor-cmd));
          }
          return INVALID_KEY_VALUE;
        }
        cursor += len;
        // TODO get rid of DROP_KEY and just checke the command type here
        if (step == DROP_KEY) {
          // check that this is the end of the command string. TODO include TOKEN_SPACE to allow for spaces at the end of the command string. 
          if ((cursor != '\0') || (get_next_token_len(cursor, &tt) != 0)) {
            if (ec_func != NULL) {
              internal_error_callback_handler(ec_func, INVALID_KEY_VALUE, "Expected end of tokens in drop statement.", (cursor-cmd));
            }
            return INVALID_KEY_VALUE;
          }
          return PARSE_OK;
        } else {
          step = ID;
        }
        break;
      }
      case ID: {
        if (tt == TOKEN_STRING || tt == TOKEN_INTEGER || tt == TOKEN_FLOAT) {
          char *id = (char *)malloc(sizeof(char) * len + 1);
          if (id == NULL) {
            if (ec_func != NULL) {
              internal_error_callback_handler(ec_func, OUT_OF_MEMORY, "Malloc returned NULL for id.", (cursor-cmd));
            }
            return OUT_OF_MEMORY;
          }
          strncpy(id, cursor, len);
          id[len] = '\0';
          prepared_statement->id = id;

        } else {
          if (ec_func != NULL) {
            internal_error_callback_handler(ec_func, INVALID_KEY_VALUE, "Invalid id value", (cursor-cmd));
          }
          return INVALID_KEY_VALUE;
        }
        step = ID;
        cursor += len;
        break;
      }
      default: {
        // TODO
        return INVALID_STEP_ERROR;
      }
    }
  }
}

void stderr_logger(int ec, const char *msg) {

  fprintf(stderr, "Error_code %d: msg: %s\n", ec,msg);
}

void main(void) {

  char *sql = "CREATE    \n    TABLE fleet (ID, name, longitude, latitude, z-index) with values 1, \"TEST\", -1, .1e+12, 1.0";

  while (*pch != '\0') {
    len = get_next_token_len(pch, &tt);
    printf("Got token of type %d and value '%.*s'\n", tt, (int) len, pch);
    pch += len;
  }
}
