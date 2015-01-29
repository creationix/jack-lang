#include <stdio.h>
#include <assert.h>

#include "api.h"
#include "lib/math.c"
#include "lib/test.c"

int main() {
  jack_state_t* state = jack_new_state(15);
  jack_call(state, jack_math, 0);
  jack_map_get_symbol(state, -1, "fib");
  jack_new_integer(state, 4);
  jack_dump_state(state);
  jack_function_call(state, -2, 1);
  jack_dump_state(state);

  jack_call(state, jack_test, 0);

  jack_free_state(state);
  return 0;
}
