#include <stdio.h>

#include "api.h"

int main() {

  jack_state_t* state = jack_new_state(10);

  jack_new_cstring(state, "Hello World");
  jack_new_string(state, 5, "12345");
  jack_new_integer(state, 42);
  jack_new_symbol(state, "fun-stuff");
  jack_new_buffer(state, 12, "Hello World\n");

  jack_new_list(state);
  jack_new_cstring(state, "numbers!");
  jack_list_push(state, -2);
  int i;
  for (i = 0; i < 5; ++i) {
    jack_new_integer(state, i);
    jack_list_push(state, -2);
  }

  jack_new_set(state, 10);
  jack_new_cstring(state, "Apple");
  jack_set_add(state, -2);
  jack_new_symbol(state, "Orange");
  jack_set_add(state, -2);
  jack_new_cstring(state, "Grape");
  jack_set_add(state, -2);
  jack_new_cstring(state, "Cherry");
  jack_set_add(state, -2);
  jack_new_symbol(state, "Orange");
  jack_set_add(state, -2);

  jack_new_map(state, 10);

  // map.name = "Tim Caswell"
  jack_new_symbol(state, "name");
  jack_new_cstring(state, "Tim Caswell");
  jack_map_set(state, -3);

  // map.age = 32
  jack_new_symbol(state, "age");
  jack_new_integer(state, 32);
  jack_map_set(state, -3);

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
  for (i = 0; i < 0x1000; ++i) {
    jack_new_set(state, 10);
    // S
    jack_dup(state, -2);
    // Ss
    jack_set_add(state, -2);
    // S
    jack_new_integer(state, 42);
    // Si
    jack_set_add(state, -2);
    // S
    jack_new_list(state);
    // SL
    jack_new_cstring(state, "numbers!");
    // SLs
    jack_list_push(state, -2);
    // SL
    jack_set_add(state, -2);
    // S
    jack_pop(state);
  }

  return 0;
}
