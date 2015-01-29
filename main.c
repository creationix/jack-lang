#include <stdio.h>

#include "api.h"

static int sample_function(jack_state_t *state, void* data, int argc) {
  printf("This is a native function!!!\n");
  printf("It was called in state %p with %d arg(s)\n", state, argc);
  jack_new_boolean(state, true);
  jack_new_symbol(state, "My Result");
  return 2;
}

int main() {

  jack_state_t* state = jack_new_state(15);

  jack_new_symbol(state, "Hello World");
  jack_new_buffer(state, 5, "12345");
  jack_new_integer(state, 42);
  jack_new_integer(state, -42);

  jack_new_list(state);
  jack_new_symbol(state, "numbers!");
  jack_list_push(state, -2);
  int i;
  for (i = 0; i < 5; ++i) {
    jack_new_integer(state, i);
    jack_list_push(state, -2);
  }

  jack_new_function(state, sample_function, 0);
  jack_new_boolean(state, false);

  jack_dump_state(state);
  jack_function_call(state, 1);
  jack_dump_state(state);

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

  jack_new_boolean(state, true);
  jack_new_boolean(state, false);

  jack_dump_state(state);
  while (state->filled > 0) {
    jack_pop(state);
    jack_dump_state(state);
  }

  jack_new_symbol(state, "eat some memory!");
  for (i = 0; i < 0x1000; ++i) {
    jack_new_list(state);
    jack_dup(state, -2);
    jack_list_insert(state, -2);
    jack_new_integer(state, 42);
    jack_list_push(state, -2);
    jack_new_list(state);
    jack_new_symbol(state, "numbers!");
    jack_list_push(state, -2);
    jack_list_insert(state, -2);
    jack_pop(state);
  }

  return 0;
}
