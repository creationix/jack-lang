
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "api.h"
#include "intern.h"

static void free_value(jack_value_t* value);

static jack_value_t* ref_value(jack_value_t *value) {
  value->ref_count += JACK_REF_COUNT;
  return value;
}

static jack_value_t* unref_value(jack_value_t *value) {
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

static jack_value_t* new_buffer(size_t length, const char* data) {
  jack_value_t *value = malloc(sizeof(*value));
  value->type = Buffer;
  value->buffer = malloc(sizeof(*value->buffer) + length);
  value->buffer->length = length;
  if (data) {
    memcpy(value->buffer->data, data, length);
  }
  else {
    memset(value->buffer->data, 0, length);
  }
  return value;
}

static jack_value_t* new_symbol(size_t length, const char* data) {
  jack_value_t *value = malloc(sizeof(*value));
  value->type = Symbol;
  value->buffer = jack_intern(length, data);
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

static void free_list(jack_list_t* list) {
  jack_list_node_t* node = list->head;
  while (node) {
    jack_list_node_t* next = node->next;
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
  // TODO: Implement
  free(function);
}

static void free_value(jack_value_t* value) {
  assert(value->ref_count < JACK_REF_COUNT);
  // Recursivly unref children.
  // Also free nested resources.
  switch (value->type) {
    case Integer: break;
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
static unsigned long long hash_value(jack_value_t *value) {
  unsigned long long integer = value->integer ^ (value->type & JACK_TYPE_MASK);
  return 11400714819674759057UL * (integer ^ integer >> 3);
}

// Equality is defined as the same type and same value.  Since  symbols are
// interned, this works for them too.
static bool value_is_equal(jack_value_t *one, jack_value_t *two) {
  return (one->type & JACK_TYPE_MASK) == (two->type & JACK_TYPE_MASK) &&
         one->buffer == two->buffer;
}

static jack_value_t* state_pop(jack_state_t* state) {
  assert(state->filled > 0);
  return state->stack[--state->filled];
}

static jack_value_t* state_push(jack_state_t* state, jack_value_t* value) {
  assert(state->filled < state->slots);
  // TODO: grow the slots if space runs out.
  return state->stack[state->filled++] = value;
}

static jack_value_t* state_get(jack_state_t* state, int index) {
  if (index < 0) index += state->filled;
  assert(index >= 0 && index < state->slots);
  jack_value_t *value = state->stack[index];
  return value;
}

static jack_value_t* state_get_as(jack_state_t* state, jack_type_t type, int index) {
  jack_value_t *value = state_get(state, index);
  assert((value->type & JACK_TYPE_MASK) == type);
  return value;
}

// Append a value to the tail of a list.
static void list_push(jack_list_t* list, jack_value_t* value) {
  jack_list_node_t *node = malloc(sizeof(*node));
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
  jack_list_node_t *node = malloc(sizeof(*node));
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
  jack_list_node_t* tail = list->tail;
  if (!tail) return NULL;
  jack_list_node_t *prev = list->tail = tail->prev;
  jack_value_t* value = tail->value;
  free(tail);
  if (prev) prev->next = NULL;
  else list->head = NULL;
  list->length--;
  return value;
}

// Shift a value from the head of a list
static jack_value_t* list_shift(jack_list_t* list) {
  jack_list_node_t* head = list->head;
  if (!head) return NULL;
  jack_list_node_t* next = list->head = head->next;
  jack_value_t* value = head->value;
  free(head);
  if (next) next->prev = NULL;
  else list->tail = NULL;
  list->length--;
  return value;
}

static bool map_set(jack_map_t* map, jack_value_t* key, jack_value_t* value) {

  // Look for the key in the hash bucket
  jack_pair_t **parent = &(map->buckets[hash_value(key) % map->num_buckets]);
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

static jack_value_t* map_get(jack_map_t* map, jack_value_t* key) {
  // Look for the key in the hash bucket
  jack_pair_t *pair = map->buckets[hash_value(key) % map->num_buckets];
  while (pair) {
    // When the key is found, return the corresponding value.
    if (value_is_equal(key, pair->key)) return pair->value;
    // Otherwise keep looking.
    pair = pair->next;
  }
  // If the key is not found, return NULL.
  return NULL;
}

static bool map_delete(jack_map_t* map, jack_value_t* key) {
  // Look for the key in the hash bucket
  jack_pair_t **parent = &(map->buckets[hash_value(key) % map->num_buckets]);
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

////////////////////////////////////////////////////////////////////////////////
//   PUBLIC API
////////////////////////////////////////////////////////////////////////////////

jack_state_t* jack_new_state(int slots) {
  jack_state_t *state = malloc(sizeof(*state));
  state->slots = slots;
  state->filled = 0;
  state->stack = malloc(sizeof(jack_value_t*) * slots);
  return state;
}

void jack_dump_value(jack_value_t *value) {
  switch (value->type & JACK_TYPE_MASK) {
    case Symbol:
      printf(":%.*s", value->buffer->length, value->buffer->data);
      break;
    case Integer:
      printf("%ld", value->integer);
      break;
    case Buffer:
      printf("Buffer[%d] %p", value->buffer->length, value->buffer->data);
      break;
    case List: {
      jack_list_node_t *node = value->list->head;
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
    default:
      printf("Unknown(%d) %p", value->type & JACK_TYPE_MASK, value);
  }

}

void jack_dump_state(jack_state_t *state) {
  int i;
  printf("state: %p (%d/%d)", state, state->filled, state->slots);
  for (i = 0; i < state->filled; ++i) {
    printf("\n%d: ", i);
    jack_dump_value(state->stack[i]);
  }
  printf("\n");
}

void jack_new_integer(jack_state_t *state, intptr_t integer) {
  ref_value(state_push(state, new_integer(integer)));
};

char* jack_new_buffer(jack_state_t *state, size_t length, const char* data) {
  return ref_value(state_push(state, new_buffer(length, data)))->buffer->data;
};

void jack_new_symbol(jack_state_t *state, const char* symbol) {
  ref_value(state_push(state, new_symbol(strlen(symbol), symbol)));
};


void jack_new_list(jack_state_t *state) {
  ref_value(state_push(state, new_list()));
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

void jack_new_map(jack_state_t *state, int num_buckets) {
  ref_value(state_push(state, new_map(num_buckets)));
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
bool jack_map_get(jack_state_t *state, int index) {
  jack_map_t* map = state_get_as(state, Map, index)->map;
  jack_value_t* value = map_get(map, state_pop(state));
  state_push(state, value);
  return (bool)value;
}
bool jack_map_has(jack_state_t *state, int index) {
  jack_map_t* map = state_get_as(state, Map, index)->map;
  return (bool)map_get(map, state_pop(state));
}
bool jack_map_delete(jack_state_t *state, int index) {
  jack_map_t* map = state_get_as(state, Map, index)->map;
  return map_delete(map, state_pop(state));
}

void jack_pop(jack_state_t *state) {
  unref_value(state_pop(state));
}

void jack_dup(jack_state_t *state, int index) {
  ref_value(state_push(state, state_get(state, index)));
}

intptr_t jack_get_integer(jack_state_t *state, int index) {
  return state_get_as(state, Integer, index)->integer;
}
