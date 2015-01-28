#include <stdio.h>

#include "api.h"

int main() {

  jack_state_t* state = jack_new_state(10);

  jack_new_symbol(state, "Hello World");
  jack_new_buffer(state, 5, "12345");
  jack_new_integer(state, 42);
  jack_new_symbol(state, "fun-stuff");
  jack_new_buffer(state, 12, "Hello World\n");

  jack_new_list(state);
  jack_new_symbol(state, "numbers!");
  jack_list_push(state, -2);
  int i;
  for (i = 0; i < 5; ++i) {
    jack_new_integer(state, i);
    jack_list_push(state, -2);
  }

  jack_new_map(state, 10);

  // map.name = "Tim Caswell"
  jack_new_symbol(state, "Tim Caswell");
  jack_map_set_symbol(state, -2, "name");

  // map.age = 32
  jack_new_integer(state, 32);
  jack_map_set_symbol(state, -2, "age");

  // print map.age
  jack_new_symbol(state, "age");
  jack_map_get(state, -2);
  printf("\nage = %ld\n", jack_get_integer(state, -1));
  jack_pop(state);

  jack_dump_state(state);
  jack_pop(state);
  jack_dump_state(state);
  jack_pop(state);
  jack_dump_state(state);
  jack_pop(state);
  jack_dump_state(state);

  jack_new_symbol(state, "eat some memory!");
  for (i = 0; i < 0x100000; ++i) {
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
