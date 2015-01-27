#include "api.h"

int main() {

  jack_state_t* state = jack_new_state(10);

  jack_push_cstring(state, "Hello World");
  jack_push_string(state, 5, "12345");
  jack_push_integer(state, 42);
  jack_push_symbol(state, "fun-stuff");
  jack_push_buffer(state, 12, "Hello World\n");

  jack_push_list(state);
  jack_push_cstring(state, "numbers!");
  jack_list_push(state, -2);
  int i;
  for (i = 0; i < 5; ++i) {
    jack_push_integer(state, i);
    jack_list_push(state, -2);
  }

  jack_push_set(state, 10);
  jack_push_cstring(state, "Apple");
  jack_set_add(state, -2);
  jack_push_symbol(state, "Orange");
  jack_set_add(state, -2);
  jack_push_cstring(state, "Grape");
  jack_set_add(state, -2);
  jack_push_cstring(state, "Cherry");
  jack_set_add(state, -2);
  jack_push_symbol(state, "Orange");
  jack_set_add(state, -2);

  jack_push_map(state, 10);

  jack_push_symbol(state, "name");
  jack_push_cstring(state, "Tim Caswell");
  jack_map_set(state, -3);

  jack_push_symbol(state, "age");
  jack_push_integer(state, 32);
  jack_map_set(state, -3);


  jack_dump_state(state);
}
