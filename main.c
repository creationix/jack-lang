#include <stdio.h>
#include <assert.h>

#include "api.h"

static int integer_add(jack_state_t *state) {
  printf("\nInside add call\n");
  jack_dump_state(state);
  int sum = 0;
  for (int i = 0; i < state->stack->top; ++i) {
    sum += jack_get_integer(state, i);
  }
  jack_new_integer(state, sum);
  return 1;
}

int main() {

  jack_state_t* state = jack_new_state(15);

  jack_new_function(state, integer_add, 0);
  jack_new_integer(state, 1);
  jack_new_integer(state, 2);
  jack_new_integer(state, 3);

  printf("\nBefore add function call\n");
  jack_dump_state(state);
  int retc = jack_function_call(state, -4, 3);
  printf("\nAfter add function call\n");
  jack_dump_state(state);
  jack_popn(state, retc + 1);

  jack_new_list(state);
  jack_new_symbol(state, "numbers!");
  jack_list_push(state, -2);
  int i;
  for (i = 0; i < 5; ++i) {
    jack_new_integer(state, i);
    jack_list_push(state, -2);
  }

  printf("\nBefore forward iter\n");
  jack_dump_state(state);
  jack_dup(state, -1);
  jack_list_forward(state);
  printf("\nAfter forward iter\n");
  jack_dump_state(state);

  while (true) {
    printf("\nBefore iteration\n");
    jack_dump_state(state);
    int retn = jack_function_call(state, -1, 0);
    printf("\nAfter iteration\n");
    jack_dump_state(state);
    if (jack_get_type(state, -1) == Nil) {
      jack_popn(state, retn + 1);
      break;
    }
    jack_popn(state, retn);
  }

  printf("\nBefore iter function call\n");
  jack_dump_state(state);
  jack_dup(state, -1);
  jack_list_backward(state);
  printf("\nAfter iter function call\n");
  jack_dump_state(state);

  while (true) {
    int retn = jack_function_call(state, -1, 0);
    printf("\nAfter iteration\n");
    jack_dump_state(state);
    if (jack_get_type(state, -1) == Nil) {
      jack_popn(state, retn + 2);
      break;
    }
    jack_popn(state, retn);
  }

  jack_new_map(state, 10);
  // map.name = "Tim Caswell"
  jack_new_symbol(state, "Tim Caswell");
  jack_map_set_symbol(state, -2, "name");
  // map.age = 32
  jack_new_integer(state, 32);
  jack_map_set_symbol(state, -2, "age");

  // print map.age
  jack_map_get_symbol(state, -1, "age");
  printf("\nage = %ld\n", jack_get_integer(state, -1));
  jack_pop(state);

  printf("\nBefore map iter\n");
  jack_dump_state(state);
  jack_map_iterate(state);
  printf("\nAfter map iter\n");
  jack_dump_state(state);

  while (true) {
    int retn = jack_function_call(state, -1, 0);
    printf("\nAfter iteration\n");
    jack_dump_state(state);
    if (jack_get_type(state, -1) == Nil) {
      jack_popn(state, retn + 1);
      break;
    }
    jack_popn(state, retn);
  }


  jack_new_boolean(state, true);
  jack_new_boolean(state, false);

  jack_dump_state(state);
  jack_popn(state, state->stack->top);
  jack_dump_state(state);

  jack_new_symbol(state, "eat some memory!");
  jack_new_symbol(state, "numbers!");
  for (i = 0; i < 0x10000; ++i) {
    jack_new_list(state);
    jack_dup(state, -3);
    jack_list_insert(state, -2);
    jack_new_integer(state, 42);
    jack_list_push(state, -2);
    jack_new_list(state);
    jack_dup(state, -3);
    jack_list_push(state, -2);
    jack_list_insert(state, -2);
    jack_pop(state);
  }

  jack_free_state(state);

  return 0;
}
