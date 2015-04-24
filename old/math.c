// Original Jack program
/*

vars cache, fib
-- Create an empty list to cache values we're seen already
cache = {}
-- Define a function to calculate fib that uses the closure cache
fib = {i|
-- Special cases, these always return 1
if i <= 2 { return 1 }
-- Then look in the cache and return if it's already there
if i in cache { return cache[i] }
-- If not, calculate the new value, store in the cache and return
cache[i] = fib(i - 1) + fib(i - 2)
}
-- Call the function and return the fib of 10
print(fib(42))

*/
#include <assert.h>
#include <stdio.h>

#include "../api.h"

static int add(jack_state_t *state) {
  jack_new_integer(state,
    jack_get_integer(state, 0) + jack_get_integer(state, 1)
  );
  return 1;
}

// Manually converted to C API
static int fib(jack_state_t *state) {

  // 0 - cache map
  // 1 - index
  intptr_t i = jack_get_integer(state, 1);
  if (i < 2) {
    jack_new_integer(state, 1);
    return 1;
  }
  // push cache[index]
  jack_dup(state, 1);
  jack_map_get(state, 0);
  if (jack_get_type(state, -1) != Nil) {
    return 1;
  }

  jack_dup(state, 0);
  jack_new_integer(state, i - 1);
  jack_call(state, fib, 2);

  jack_dup(state, 0);
  jack_new_integer(state, i - 2);
  jack_call(state, fib, 2);

  jack_call(state, add, 2);

  // Store value in the cache
  jack_dup(state, 1);
  jack_dup(state, -2);
  jack_map_set(state, 0);
  return 1;
}

int jack_math(jack_state_t *state) {
  jack_new_map(state, 10); // math module

  // Create fib function with cache map embedded
  jack_new_map(state, 10);
  jack_new_function(state, fib, 1);
  jack_map_set_symbol(state, -2, "fib");
  return 1;
}
