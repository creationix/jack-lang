#include <stdio.h>
#include <assert.h>

#include "api.h"

static int integer_add(jack_state_t *state, void* data, int argc) {
  printf("\nInside add call\n");
  jack_dump_state(state);
  int sum = 0;
  for (int i = 0; i < argc; ++i) {
    jack_value_t* value = state->stack[i];
    assert((value->type & JACK_TYPE_MASK) == Integer);
    sum += value->integer;
  }
  jack_new_integer(state, sum);
  return 1;
}

struct list_iter {
  jack_value_t* value;
  jack_node_t* node;
};

static int list_iterate(jack_state_t *state, void* data, int argc) {
  assert(argc == 0);
  printf("\nInside list_iterate call\n");
  jack_dump_state(state);
  struct list_iter *iter = data;
  if (iter->node) {
    jack_new_value(state, iter->node->value);
    iter->node = iter->node->next;
  }
  else {
    if (iter->value) {
      unref_value(iter->value);
      iter->value = NULL;
    }
    jack_new_nil(state);
  }
  return 1;
}

static int make_list_iterator(jack_state_t *state, void* data, int argc) {
  assert(argc == 1);
  printf("\nInside list_iterator call\n");
  jack_dump_state(state);
  jack_value_t* value = state->stack[0];
  assert((value->type & JACK_TYPE_MASK) == List);
  struct list_iter *iter = jack_new_function(state, list_iterate, sizeof(struct list_iter));
  iter->value = value;
  iter->value->ref_count += JACK_REF_COUNT;
  iter->node = value->list->head;
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
  assert(jack_function_call(state, 3) == 1);
  printf("\nAfter add function call\n");
  jack_dump_state(state);
  jack_pop(state);

  jack_new_function(state, make_list_iterator, 0);
  jack_new_list(state);
  jack_new_symbol(state, "numbers!");
  jack_list_push(state, -2);
  int i;
  for (i = 0; i < 5; ++i) {
    jack_new_integer(state, i);
    jack_list_push(state, -2);
  }

  printf("\nBefore iter function call\n");
  jack_dump_state(state);
  assert(jack_function_call(state, 1) == 1);
  printf("\nAfter iter function call\n");
  jack_dump_state(state);

  while (true) {
    jack_dup(state, -1);
    printf("\nBefore iteration call\n");
    jack_dump_state(state);
    jack_function_call(state, 0);
    printf("\nAfter iteration call\n");
    jack_dump_state(state);
    if (!state->stack[state->filled - 1]) {
      jack_pop(state);
      break;
    }
    jack_pop(state);
  }
  jack_pop(state);
  return 0;

  jack_new_symbol(state, "Hello World");
  jack_new_buffer(state, 5, "12345");
  jack_new_integer(state, 42);
  jack_new_integer(state, -42);


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
