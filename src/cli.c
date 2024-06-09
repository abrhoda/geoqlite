#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// library headers
#include "parse.h"

typedef struct {
  char *buffer;
  size_t buffer_length;
  ssize_t input_length;
} InputBuffer;

InputBuffer *new_input_buffer() {
  InputBuffer *input_buffer = (InputBuffer *)malloc(sizeof(InputBuffer));
  input_buffer->buffer = NULL;
  input_buffer->buffer_length = 0;
  input_buffer->input_length = 0;

  return input_buffer;
}

void close_input_buffer(InputBuffer *input_buffer) {
  free(input_buffer->buffer);
  free(input_buffer);
}

void read_input(InputBuffer *input_buffer) {
  ssize_t bytes_read =
      getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);

  if (bytes_read <= 0) {
    printf("Error reading input\n");
    exit(EXIT_FAILURE);
  }

  input_buffer->input_length = bytes_read - 1;
  input_buffer->buffer[bytes_read - 1] = 0;
}

void print_prompt() { printf("db > "); }

void stderr_logger(int ec, const char * emsg) {
  fprintf(stderr, "Error code: %d. Message: %s\n", ec, emsg);
}

int main() {

  InputBuffer *input_buffer = new_input_buffer();

  PreparedStatement prepared_statement;

  while(1) {
    print_prompt();
    read_input(input_buffer);
    
    printf("read %d bytes\n",(int) input_buffer->buffer_length);
    printf("command read in: %s\n", input_buffer->buffer);
    
    if (strcmp(input_buffer->buffer, ".exit") == 0) {
      break;
    }

    int rc = make_prepared_statement(input_buffer->buffer, &prepared_statement, stderr_logger);

    printf("Return code from `make_prepared_statment` was %d\n", rc);
  }

  close_input_buffer(input_buffer);
  exit(EXIT_SUCCESS);

}
