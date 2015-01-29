
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "api.h"
#include "intern.h"

static jack_type_t get_type(jack_value_t* value) {
  return value ? value->type & JACK_TYPE_MASK : Nil;
}

static void free_value(jack_value_t* value);

static jack_value_t* ref_value(jack_value_t *value) {
  if (!value) return value;
  value->ref_count += JACK_REF_COUNT;
  return value;
}

// TODO: make static again.
jack_value_t* unref_value(jack_value_t *value) {
  if (!value) return NULL;
  value->ref_count -= JACK_REF_COUNT;
  if (value->ref_count >= JACK_REF_COUNT) return value;
  free_value(value);
  return NULL;
}

static jack_value_t* new_integer(intptr_t integer) {
  jack_value_t *value = malloc(sizeof(*value));
  value->type = Integer;
  value->integer = integer;
  return value;
}

static jack_value_t* new_boolean(bool boolean) {
  jack_value_t *value = malloc(sizeof(*value));
  value->type = Boolean;
  value->boolean = boolean;
  return value;
}

static jack_value_t* new_buffer(size_t size, const char* data) {
  jack_value_t *value = malloc(sizeof(*value));
  value->type = Buffer;
  value->buffer = malloc(sizeof(*value->buffer) + size);
  value->buffer->size = size;
  if (data) {
    memcpy(value->buffer->data, data, size);
  }
  else {
    memset(value->buffer->data, 0, size);
  }
  return value;
}

static jack_value_t* new_symbol(size_t size, const char* data) {
  jack_value_t *value = malloc(sizeof(*value));
  value->type = Symbol;
  value->buffer = jack_intern(size, data);
  return value;
}

static jack_value_t* new_list() {
  jack_value_t *value = malloc(sizeof(*value));
  value->type = List;
  value->list = malloc(sizeof(*value->list));
  memset(value->list, 0, sizeof(*value->list));
  return value;
}

static jack_value_t* new_map(int num_buckets) {
  jack_value_t *value = malloc(sizeof(*value));
  value->type = Map;
  size_t size = sizeof(jack_map_t) + sizeof(jack_pair_t*) * num_buckets;
  value->map = malloc(size);
  memset(value->map, 0, size);
  value->map->num_buckets = num_buckets;
  return value;
}

static jack_value_t* new_function(jack_call_t *call, int slots) {
  jack_value_t *value = malloc(sizeof(*value));
  value->type = Function;
  value->function = malloc(sizeof(*value->function));
  value->function->call = call;
  value->function->state = jack_new_state(slots);
  value->function->name = NULL;
  return value;
}

static void free_list(jack_list_t* list) {
  jack_node_t* node = list->head;
  while (node) {
    jack_node_t* next = node->next;
    unref_value(node->value);
    free(node);
    node = next;
  }
  free(list);
}

static void free_map(jack_map_t* map) {
  int i;
  for (i = 0; i < map->num_buckets; ++i) {
    jack_pair_t* pair = map->buckets[i];
    while (pair) {
      jack_pair_t* next = pair->next;
      unref_value(pair->key);
      unref_value(pair->value);
      free(pair);
      pair = next;
    }
  }
  free(map);
}

static void free_function(jack_function_t* function) {
  jack_free_state(function->state);
  free(function);
}

static void free_value(jack_value_t* value) {
  assert(value); // Don't pass in nil values
  assert(value->ref_count < JACK_REF_COUNT);
  printf(" FREE ");
  jack_dump_value(value);
  printf("\n");
  // Recursivly unref children.
  // Also free nested resources.
  switch (value->type) {
    case Integer: case Boolean: case Nil:
      break;
    case Buffer:
      free(value->buffer);
      break;
    case Symbol:
      jack_unintern(value->buffer);
      break;
    case List:
      free_list(value->list);
      break;
    case Map:
      free_map(value->map);
      break;
    case Function:
      free_function(value->function);
      break;
  }
  free(value);
}

// Constant is the nearest prime to 2^64 / phi
// This is a 64-bit modification of knuth's integer hash.
// The xor shift is to mix entropy to lower bits where it's lacking.
static uint64_t hash_integer(uint64_t integer) {
  return 11400714819674759057UL * (integer ^ integer >> 3);
}

// Equality is defined as the same type and same value.  Since  symbols are
// interned, this works for them too.
static bool value_is_equal(jack_value_t *one, jack_value_t *two) {
  return (get_type(one) == get_type(two)) &&
         one->buffer == two->buffer;
}

static jack_value_t* state_pop(jack_state_t* state) {
  jack_stack_t *stack = state->stack;
  assert(stack->top > 0);
  return stack->values[--stack->top];
}

static jack_value_t* state_push(jack_state_t* state, jack_value_t* value) {
  jack_stack_t *stack = state->stack;
  assert(stack->top < stack->length);
  // TODO: grow the slots if space runs out.
  return stack->values[stack->top++] = value;
}

static jack_value_t* state_get(jack_state_t* state, int index) {
  jack_stack_t *stack = state->stack;
  if (index < 0) index += stack->top;
  assert(index >= 0 && index < stack->length);
  jack_value_t *value = stack->values[index];
  return value;
}

static jack_value_t* state_get_as(jack_state_t* state, jack_type_t type, int index) {
  jack_dump_state(state);
  jack_value_t *value = state_get(state, index);
  jack_dump_state(state);
  assert(get_type(value) == type);
  return value;
}

static jack_value_t* new_value(jack_state_t *state, jack_value_t *value) {
  ref_value(state_push(state, value));
  return value;
}

// Append a value to the tail of a list.
static void list_push(jack_list_t* list, jack_value_t* value) {
  jack_node_t *node = malloc(sizeof(*node));
  node->value = value;
  node->next = NULL;
  node->prev = list->tail;
  if (list->tail) {
    list->tail->next = node;
    list->tail = node;
  }
  else {
    list->head = list->tail = node;
  }
  list->length++;
}

// Insert a value to the head of a list
static void list_insert(jack_list_t* list, jack_value_t* value) {
  jack_node_t *node = malloc(sizeof(*node));
  node->value = value;
  node->next = list->head;
  node->prev = NULL;
  if (list->head) {
    list->head->prev = node;
    list->head = node;
  }
  else {
    list->head = list->tail = node;
  }
  list->length++;
}

// Pop a value from the tail of a list.
static jack_value_t* list_pop(jack_list_t* list) {
  jack_node_t* tail = list->tail;
  if (!tail) return NULL;
  jack_node_t *prev = list->tail = tail->prev;
  jack_value_t* value = tail->value;
  free(tail);
  if (prev) prev->next = NULL;
  else list->head = NULL;
  list->length--;
  return value;
}

// Shift a value from the head of a list
static jack_value_t* list_shift(jack_list_t* list) {
  jack_node_t* head = list->head;
  if (!head) return NULL;
  jack_node_t* next = list->head = head->next;
  jack_value_t* value = head->value;
  free(head);
  if (next) next->prev = NULL;
  else list->tail = NULL;
  list->length--;
  return value;
}

static bool map_set(jack_map_t* map, jack_value_t* key, jack_value_t* value) {

  // Look for the key in the hash bucket
  jack_pair_t **parent = &(map->buckets[hash_integer(key->integer) % map->num_buckets]);
  jack_pair_t *pair = *parent;
  while (pair) {
    // If the key is already in the slot,
    if (value_is_equal(key, pair->key)) {
      // Replace the value with the new value
      unref_value(pair->value);
      pair->value = value;
      return false;
    }
    // Otherwise keep looking.
    parent = &pair->next;
    pair = *parent;
  }

  // If it wasn't there, append to the list.
  pair = malloc(sizeof(*pair));
  pair->key = key;
  pair->value = value;
  pair->next = NULL;
  *parent = pair;
  return true;
}

static bool map_set_symbol(jack_map_t* map, const char* symbol, jack_value_t* value) {
  jack_value_t* key = ref_value(new_symbol(strlen(symbol), symbol));
  return map_set(map, key, value);
}

static jack_value_t* map_get(jack_map_t* map, jack_value_t* key) {
  // Look for the key in the hash bucket
  jack_pair_t *pair = map->buckets[hash_integer(key->integer) % map->num_buckets];
  while (pair) {
    // When the key is found, return the corresponding value.
    if (value_is_equal(key, pair->key)) return pair->value;
    // Otherwise keep looking.
    pair = pair->next;
  }
  // If the key is not found, return NULL.
  return NULL;
}

static jack_value_t* map_get_symbol(jack_map_t* map, const char* symbol) {
  jack_value_t* key = ref_value(new_symbol(strlen(symbol), symbol));
  jack_value_t* value = map_get(map, key);
  unref_value(key);
  return value;
}

static bool map_delete(jack_map_t* map, jack_value_t* key) {
  // Look for the key in the hash bucket
  jack_pair_t **parent = &(map->buckets[hash_integer(key->integer) % map->num_buckets]);
  jack_pair_t *pair = *parent;
  while (pair) {
    if (value_is_equal(key, pair->key)) {
      *parent = pair->next;
      unref_value(pair->value);
      return true;
    }
    // Otherwise keep looking.
    parent = &pair->next;
    pair = *parent;
  }
  return false;
}

static bool map_delete_symbol(jack_map_t* map, const char* symbol) {
  jack_value_t* key = ref_value(new_symbol(strlen(symbol), symbol));
  bool res = map_delete(map, key);
  unref_value(key);
  return res;
}

////////////////////////////////////////////////////////////////////////////////
//   PUBLIC API
////////////////////////////////////////////////////////////////////////////////

jack_state_t* jack_new_state(int slots) {
  jack_state_t *state = malloc(sizeof(*state));
  memset(state, 0, sizeof(*state));
  size_t size = sizeof(jack_stack_t) + sizeof(jack_value_t*) * slots;
  jack_stack_t *stack = state->stack = malloc(size);
  memset(stack, 0, size);
  stack->length = slots;
  stack->top = 0;
  return state;
}
void jack_xmove(jack_state_t *from, jack_state_t *to, int num) {
  jack_stack_t *a = from->stack;
  jack_stack_t *b = to->stack;
  // Make sure there is room to move the items.
  assert(a->top >= num && b->top + num <= b->length);
  a->top -= num;
  for (int i = 0; i < num; ++i) {
    b->values[b->top++] = a->values[a->top + i];
    a->values[a->top + i] = NULL;
  }
}

void jack_free_state(jack_state_t *state) {
  for (int i = 0; i < state->stack->top; ++i) {
    unref_value(state->stack->values[i]);
  }
  free(state->stack);
  jack_malloc_node_t *node = state->head;
  while (node) {
    jack_malloc_node_t *next = node->next;
    free(node);
    node = next;
  }
  free(state);
}


void* jack_malloc(jack_state_t *state, size_t size) {
  jack_malloc_node_t *node = malloc(sizeof(*node) + size);
  node->next = NULL;
  if (state->tail) {
    state->tail->next = node;
  }
  else {
    state->head = node;
  }
  state->tail = node;
  return &(node->data);
}

void jack_dump_value(jack_value_t *value) {
  jack_type_t type = get_type(value);
  switch (type) {
    case Nil:
      printf("(nil)");
    case Symbol:
      printf(":%.*s", value->buffer->size, value->buffer->data);
      break;
    case Integer:
      printf("%ld", value->integer);
      break;
    case Boolean:
      printf("%s", value->boolean ? "true" : "false");
      break;
    case Buffer:
      printf("Buffer[%d] %p", value->buffer->size, value->buffer->data);
      break;
    case List: {
      jack_node_t *node = value->list->head;
      printf("[");
      int count = 0;
      while (node) {
        if (count++) printf(", ");
        jack_dump_value(node->value);
        node = node->next;
      }
      printf("]");
      break;
    }
    case Map: {
      jack_map_t *map = value->map;
      printf("{");
      int i, count = 0;
      for (i = 0; i < map->num_buckets; ++i) {
        jack_pair_t *pair = map->buckets[i];
        while (pair) {
          if (count++) printf(", ");
          jack_dump_value(pair->key);
          printf(": ");
          jack_dump_value(pair->value);
          pair = pair->next;
        }
      }
      printf("}");
      break;
    }
    case Function:
      printf("<%s %p>", value->function->name ? value->function->name : "function", value->function);
      break;
  }

}

void jack_dump_state(jack_state_t *state) {
  int i;
  printf("state: %p (%d/%d)", state, state->stack->top, state->stack->length);
  for (i = 0; i < state->stack->top; ++i) {
    jack_value_t* value = state->stack->values[i];
    if (value) {
      printf("\n%d: (%d) ", i, value->ref_count / JACK_REF_COUNT);
      jack_dump_value(value);
    }
    else {
      printf("\n%d: (nil) ", i);
    }
  }
  printf("\n");
}

void jack_new_nil(jack_state_t *state) {
  state_push(state, NULL);
}

void jack_new_integer(jack_state_t *state, intptr_t integer) {
  new_value(state, new_integer(integer));
};

void jack_new_boolean(jack_state_t *state, bool boolean) {
  new_value(state, new_boolean(boolean));
};

char* jack_new_buffer(jack_state_t *state, size_t length, const char* data) {
  return new_value(state, new_buffer(length, data))->buffer->data;
};

void jack_new_symbol(jack_state_t *state, const char* symbol) {
  new_value(state, new_symbol(strlen(symbol), symbol));
};


jack_function_t* jack_new_function(jack_state_t *state, jack_call_t *call, int argc) {
  jack_value_t* value = new_function(call, argc + 10);
  jack_xmove(state, value->function->state, argc);
  return new_value(state, value)->function;
}

static int do_call(jack_state_t* state, jack_function_t* function, int argc) {
  int old_top = function->state->stack->top;

  // Move the arguments to the function's state
  jack_xmove(state, function->state, argc);

  // Call the native function
  int retc = function->call(function->state);

  // Move the results back
  jack_xmove(function->state, state, retc);

  int new_top = function->state->stack->top;

  // Make sure the function didn't consume too many slots.
  assert(new_top >= old_top);
  if (new_top > old_top) {
    // Reset the function's state
    jack_popn(function->state, new_top - old_top);
  }

  return retc;
}

int jack_function_call(jack_state_t *state, int index, int argc) {
  return do_call(state, state_get_as(state, Function, index)->function, argc);
}

int jack_call(jack_state_t *state, jack_call_t *call, int argc) {
  jack_value_t* value = new_function(call, argc + 10);
  int retc = do_call(state, value->function, argc);
  free_value(value);
  return retc;
}

void jack_new_list(jack_state_t *state) {
  new_value(state, new_list());
}
int jack_list_length(jack_state_t *state, int index) {
  jack_list_t* list = state_get_as(state, List, index)->list;
  return list->length;
}
int jack_list_push(jack_state_t *state, int index) {
  jack_list_t* list = state_get_as(state, List, index)->list;
  list_push(list, state_pop(state));
  return list->length;
}
int jack_list_insert(jack_state_t *state, int index) {
  jack_list_t* list = state_get_as(state, List, index)->list;
  list_insert(list, state_pop(state));
  return list->length;
}
int jack_list_pop(jack_state_t *state, int index) {
  jack_list_t* list = state_get_as(state, List, index)->list;
  state_push(state, list_pop(list));
  return list->length;
}
int jack_list_shift(jack_state_t *state, int index) {
  jack_list_t* list = state_get_as(state, List, index)->list;
  state_push(state, list_shift(list));
  return list->length;
}
static int list_forward(jack_state_t *state) {
  jack_node_t* node = state->data;
  if (node) {
    new_value(state, node->value);
    state->data = node->next;
  }
  else {
    jack_new_nil(state);
  }
  return 1;
}
static int list_backward(jack_state_t *state) {
  jack_node_t* node = state->data;
  if (node) {
    new_value(state, node->value);
    state->data = node->prev;
  }
  else {
    jack_new_nil(state);
  }
  return 1;
}
void jack_list_forward(jack_state_t *state) {
  jack_list_t* list = state_get_as(state, List, -1)->list;
  jack_function_t* iter = jack_new_function(state, list_forward, 1);
  iter->name = "list-forward";
  iter->state->data = list->head;
}
void jack_list_backward(jack_state_t *state) {
  jack_list_t* list = state_get_as(state, List, -1)->list;
  jack_function_t* iter = jack_new_function(state, list_backward, 1);
  iter->name = "list-backward";
  iter->state->data = list->tail;
}



void jack_new_map(jack_state_t *state, int num_buckets) {
  new_value(state, new_map(num_buckets));
}
int jack_map_length(jack_state_t *state, int index) {
  jack_map_t* map = state_get_as(state, Map, index)->map;
  return map->length;
}
bool jack_map_set(jack_state_t *state, int index) {
  jack_map_t* map = state_get_as(state, Map, index)->map;
  jack_value_t* value = state_pop(state);
  jack_value_t* key = state_pop(state);
  return map_set(map, key, value);
}
bool jack_map_set_symbol(jack_state_t *state, int index, const char* symbol) {
  jack_map_t* map = state_get_as(state, Map, index)->map;
  jack_value_t* value = state_pop(state);
  return map_set_symbol(map, symbol, value);
}
bool jack_map_get(jack_state_t *state, int index) {
  jack_map_t* map = state_get_as(state, Map, index)->map;
  jack_value_t* value = map_get(map, state_pop(state));
  new_value(state, value);
  return (bool)value;
}
bool jack_map_get_symbol(jack_state_t *state, int index, const char* symbol) {
  jack_map_t* map = state_get_as(state, Map, index)->map;
  jack_value_t* value = map_get_symbol(map, symbol);
  new_value(state, value);
  return (bool)value;
}
bool jack_map_has(jack_state_t *state, int index) {
  jack_map_t* map = state_get_as(state, Map, index)->map;
  return (bool)map_get(map, state_pop(state));
}
bool jack_map_has_symbol(jack_state_t *state, int index, const char* symbol) {
  jack_map_t* map = state_get_as(state, Map, index)->map;
  return (bool)map_get_symbol(map, symbol);
}
bool jack_map_delete(jack_state_t *state, int index) {
  jack_map_t* map = state_get_as(state, Map, index)->map;
  return map_delete(map, state_pop(state));
}
bool jack_map_delete_symbol(jack_state_t *state, int index, const char* symbol) {
  jack_map_t* map = state_get_as(state, Map, index)->map;
  return map_delete_symbol(map, symbol);
}

typedef struct {
  int bucket;
  jack_pair_t* pair;
} jack_map_iterator_t;

static int map_iterate(jack_state_t *state) {
  jack_map_iterator_t *iter = state->data;
  jack_map_t* map = state_get_as(state, Map, 0)->map;
  while (!iter->pair) {
    if (iter->bucket >= map->num_buckets) {
      jack_new_nil(state);
      jack_new_nil(state);
      return 2;
    }
    iter->pair = map->buckets[iter->bucket++];
  }
  new_value(state, iter->pair->key);
  new_value(state, iter->pair->value);
  iter->pair = iter->pair->next;
  return 2;
}
void jack_map_iterate(jack_state_t *state) {
  state_get_as(state, Map, -1);
  jack_function_t* iter = jack_new_function(state, map_iterate, 1);
  jack_map_iterator_t *iterator = jack_malloc(state, sizeof(*iterator));
  iterator->bucket = 0;
  iterator->pair = NULL;
  iter->name = "map-iterate";
  iter->state->data = iterator;
}

void jack_pop(jack_state_t *state) {
  unref_value(state_pop(state));
}

void jack_popn(jack_state_t *state, int count) {
  for (int i = 0; i < count; ++i) {
    jack_pop(state);
  }
}


void jack_dup(jack_state_t *state, int index) {
  ref_value(state_push(state, state_get(state, index)));
}

jack_type_t jack_get_type(jack_state_t *state, int index) {
  return get_type(state_get(state, index));
}

intptr_t jack_get_integer(jack_state_t *state, int index) {
  return state_get_as(state, Integer, index)->integer;
}
bool jack_get_boolean(jack_state_t *state, int index) {
  return state_get_as(state, Boolean, index)->boolean;
}
const char* jack_get_symbol(jack_state_t *state, int index, int *size) {
  jack_buffer_t* buffer = state_get_as(state, Symbol, index)->buffer;
  *size = buffer->size;
  return buffer->data;
}
char* jack_get_buffer(jack_state_t *state, int index, int* size) {
  jack_buffer_t* buffer = state_get_as(state, Buffer, index)->buffer;
  *size = buffer->size;
  return buffer->data;
}
