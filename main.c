#include <stdio.h>
#include <assert.h>

#include "api.h"
#include "lib/math.c"
#include "lib/test.c"

static jack_state_t* state;

static intptr_t jack_fib(intptr_t n) {
  jack_new_integer(state, n);
  jack_function_call(state, 1, 1);
  return 1;
}

int main() {
  for (int j = 0; j < 0x1000; ++j) {

    state = jack_new_state(15);
    jack_call(state, jack_math, 0);
    jack_map_get_symbol(state, -1, "fib");
    jack_new_list(state);
    // 1 - fib
    // 2 - list

    for (intptr_t i = 0; i <= 91; ++i) {
      printf("fib(%ld) = %ld\n", i, jack_fib(i));
      jack_list_push(state, 2);
    }
    jack_dump_state(state);

    jack_free_state(state);
  }
  return 0;
}
