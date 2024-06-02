#include "testing_utils.h"
#include <stdio.h>
#include <string.h>

int RESULT_EQ(Result *expected, Result *actual) {
  if (expected->parser_error != 0 && actual->parser_error != 0 && expected->parser_error == actual->parser_error) {
    return 0;
  }

  if (expected->parser_error != actual->parser_error) {
    printf("Parser errors do not match. Expected is %d but actual is %d\n",
           expected->parser_error, actual->parser_error);
    return 1;
  }

  Query *eq = expected->query;
  Query *aq = actual->query;

  // check that query types match
  if (eq->type != aq->type) {
    printf("Query types do not match. expected is %d but actual is %d.\n",
           eq->type, aq->type);
    return 1;
  }

  if (eq->subject != NULL && aq->subject == NULL) {
    printf("Subjects do not match. Expected is %s while actual was null.\n",
           eq->subject);
    return 1;
  }

  if (eq->subject == NULL && aq->subject != NULL) {
    printf("Subjects do not match. Expected is null while actual was %s.\n",
           aq->subject);
    return 1;
  }

  if (strcmp(eq->subject, aq->subject) != 0) {
    printf("query subject do not match. expected is %s but actual is %s.\n",
           eq->subject, aq->subject);
    return 1;
  }

  // check eq number of fields
  if (eq->fields->count != aq->fields->count) {
    printf(
        "Query fields count do not match. expected is %lu but actual is %lu.\n",
        eq->fields->count, aq->fields->count);
    return 1;
  }

  // check that the fields values match
  // FIXME start here
  size_t current_index = 0;
  Node *ah = aq->fields->head;
  Node *eh = eq->fields->head;
  while(ah != NULL) {
    char *ad = (char *)ah->data;
    char *ed = (char *)eh->data;
    if (strcmp(ed, ad) != 0) {
      printf(
          "Query fields at index %lu do not match. expected is '%s' but actual "
          "is '%s'.\n",
          current_index, ed, ad);
      return 1;
    }
    ah = ah->next;
    eh = eh->next;
    current_index++;
  }

  // check number of rows
  if (eq->rows_count != aq->rows_count) {
    printf(
        "Query rows count do not match. expected is %lu but actual is %lu.\n",
        eq->rows_count, aq->rows_count);
    return 1;
  }

  // check that the row values match
  for (size_t i = 0; i < eq->rows_count; i++) {
    Row *er = eq->rows[i];
    Row *ar = aq->rows[i];
    // check the rows values count matches
    if (er->values_count != ar->values_count) {
      printf(
          "query row value counts at index %lu do not match. expected is %lu "
          "but actual is %lu.\n",
          i, er->values_count, ar->values_count);
      return 1;
    }

    // check the values in rows[i] match
    for (size_t j = 0; j < er->values_count; j++) {
      char *ev = er->values[j];
      char *av = ar->values[j];
      if (strcmp(ev, av) != 0) {
        printf(
            "Query rows values field at row index %lu and values index %lu do "
            "not match. expected is '%s' but actual is '%s'.\n",
            i, j, er->values[j], ar->values[j]);
        return 1;
      }
    }
  }

  // check number of aliases match
  if (eq->aliases_count != aq->aliases_count) {
    printf(
        "Query aliases count do not match. expected is %lu but actual is "
        "%lu.\n",
        eq->aliases_count, aq->aliases_count);
    return 1;
  }

  for (size_t i = 0; i < eq->aliases_count; i++) {
    Alias *ea = eq->aliases[i];
    Alias *aa = aq->aliases[i];
    if (strcmp(ea->name, aa->name) != 0) {
      printf(
          "Query alias name field at index %lu do not match. expected is '%s' "
          "but actual is '%s'.\n",
          i, ea->name, aa->name);
      return 1;
    }

    if (strcmp(ea->as, aa->as) != 0) {
      printf(
          "Query alias as field at index %lu do not match. expected is '%s' "
          "but actual is '%s'.\n",
          i, ea->as, aa->as);
      return 1;
    }
  }

  // check that the number of conditions match
  if (eq->conditions_count != aq->conditions_count) {
    printf(
        "Query conditions count do not match. expected is %lu but actual is "
        "%lu.\n",
        eq->conditions_count, aq->conditions_count);
    return 1;
  }

  for (size_t i = 0; i < eq->conditions_count; i++) {
    Condition *ec = eq->conditions[i];
    Condition *ac = aq->conditions[i];

    if ((ec->lhs_is_field && !ac->lhs_is_field) ||
        (!ec->lhs_is_field && ac->lhs_is_field)) {
      printf(
          "Query conditions 'lhs_is_field' at index %lu do not match. expected "
          "is '%d' but actual is '%d'.\n",
          i, ec->lhs_is_field, ac->lhs_is_field);
      return 1;
    }

    if ((ec->rhs_is_field && !ac->rhs_is_field) ||
        (!ec->rhs_is_field && ac->rhs_is_field)) {
      printf(
          "Query conditions 'rhs_is_field' at index %lu do not match. expected "
          "is '%d' but actual is '%d'.\n",
          i, ec->rhs_is_field, ac->rhs_is_field);
      return 1;
    }

    if (strcmp(ec->lhs, ac->lhs) != 0) {
      printf(
          "Query conditions lhs field at index %lu do not match. expected is "
          "'%s' but actual is '%s'.\n",
          i, ec->lhs, ac->lhs);
      return 1;
    }

    if (strcmp(ec->rhs, ac->rhs) != 0) {
      printf(
          "Query conditions rhs field at index %lu do not match. expected is "
          "'%s' but actual is '%s'.\n",
          i, ec->rhs, ac->rhs);
      return 1;
    }
    if (ec->operator!= ac->operator) {
      printf(
          "Query conditions 'operator' at index %lu do not match. expected is "
          "'%d' but actual is '%d'.\n",
          i, ec->operator, ac->operator);
      return 1;
    }
  }

  return 0;
}

int RESULT_TEST(char *label, char *input, Result *expected) {
  printf("[TEST] - %s.\n", label);
  printf("[QUERY] - %s.\n", input);
  Result *actual = parse(input);

  int res = RESULT_EQ(expected, actual);
  free_result(actual);
  if (res != 0) {
    printf("[RESULT] - FAILED.\n\n");
  } else {
    printf("[RESULT] - PASSED.\n\n");
  }
  return res;
}
