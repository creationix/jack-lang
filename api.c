
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "api.h"
#include "intern.h"

static jack_value_t* get_slot(jack_state_t *state, jack_type_t type) {
  assert(state->filled < state->slots);
  jack_value_t *slot = &state->stack[state->filled++];
  memset(slot, 0, sizeof(*slot));
  // Ref count is zero initially on the stack slot.
  slot->type = type;
  return slot;
}

// Copy a value, but with it's ref-count reset to 1
static jack_value_t* duplicate_value(jack_value_t *original) {
  jack_value_t *copy = malloc(sizeof(jack_value_t));
  memcpy(copy, original, sizeof(jack_value_t));
  copy->type = (copy->type & JACK_TYPE_MASK) | JACK_REF_COUNT;
  return copy;
}

// Constant is the nearest prime to 2^64 / phi
// This is a 64-bit modification of knuth's integer hash.
// The xor shift is to mix entropy to lower bits where it's lacking.
static uint64_t value_hash(jack_value_t *value) {
  uint64_t integer = value->integer ^ (value->type & JACK_TYPE_MASK);
  return 11400714819674759057UL * (integer ^ integer >> 3);
}

// Equality is defined as the same type and same value.  Since strings
// and symbols are interned, this works for them too.
static int value_is_equal(jack_value_t *one, jack_value_t *two) {
  return (one->type & JACK_TYPE_MASK) == (two->type & JACK_TYPE_MASK) &&
         one->buffer == two->buffer;
}


////////////////////////////////////////////////////////////////////////////////

jack_state_t* jack_new_state(int slots) {
  jack_state_t *state = malloc(sizeof(*state));
  state->slots = slots;
  state->filled = 0;
  state->stack = malloc(sizeof(jack_value_t) * slots);
  return state;
}

void jack_dump_value(jack_value_t *value) {
  switch (value->type & JACK_TYPE_MASK) {
    case String:
      printf("'%.*s'", value->buffer->length, value->buffer->data);
      break;
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
    case Set: {
      jack_set_t *set = value->set;
      printf("(");
      int i, count = 0;
      for (i = 0; i < set->num_buckets; ++i) {
        jack_node_t *node = set->buckets[i];
        while (node) {
          if (count++) printf(", ");
          jack_dump_value(node->value);
          node = node->next;
        }
      }
      printf(")");
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
  size_t i;
  printf("state: %p", state);
  for (i = 0; i < state->filled; ++i) {
    printf("\n%lu: ", i);
    jack_dump_value(&state->stack[i]);
  }
  printf("\n");
}

void jack_push_integer(jack_state_t *state, int integer) {
  jack_value_t *slot = get_slot(state, Integer);
  slot->integer = integer;
};

char* jack_push_buffer(jack_state_t *state, int length, char* data) {
  jack_value_t *slot = get_slot(state, Buffer);
  slot->buffer = malloc(sizeof(jack_buffer_t) + length);
  slot->buffer->length = length;
  if (data) {
    memcpy(slot->buffer->data, data, length);
  }
  else {
    memset(slot->buffer->data, 0, length);
  }
  return slot->buffer->data;
};

void jack_push_string(jack_state_t *state, int length, const char* string) {
  jack_value_t *slot = get_slot(state, String);
  slot->buffer = jack_intern(length, string);
};

void jack_push_cstring(jack_state_t *state, const char* string) {
  return jack_push_string(state, strlen(string), string);
};

void jack_push_symbol(jack_state_t *state, const char* symbol) {
  jack_value_t *slot = get_slot(state, Symbol);
  slot->buffer = jack_intern(strlen(symbol), symbol);
};

void jack_push_list(jack_state_t *state) {
  jack_value_t *slot = get_slot(state, List);
  jack_list_t *list = slot->list = malloc(sizeof(*list));
  memset(list, 0, sizeof(*list));
}

void jack_push_set(jack_state_t *state, int num_buckets) {
  jack_value_t *slot = get_slot(state, Set);
  size_t size = sizeof(jack_set_t) + sizeof(jack_node_t*) * num_buckets;
  jack_set_t *set = slot->set = malloc(size);
  memset(set, 0, size);
  set->num_buckets = num_buckets;
}

void jack_push_map(jack_state_t *state, int num_buckets) {
  jack_value_t *slot = get_slot(state, Map);
  size_t size = sizeof(jack_map_t) + sizeof(jack_pair_t*) * num_buckets;
  jack_map_t *map = slot->map = malloc(size);
  memset(map, 0, size);
  map->num_buckets = num_buckets;
}

// Pop the value from the top of the stack and push on the list at [index]
void jack_list_push(jack_state_t *state, int index) {
  if (index < 0) index += state->filled;

  jack_value_t *value;

  // Read the list from the stack
  value = &state->stack[index];
  assert((value->type & JACK_TYPE_MASK) == List);
  jack_list_t *list = value->list;

  // Pop the value from the stack
  assert(state->filled >= 1);
  value = &state->stack[--state->filled];

  // Append the value to the list
  jack_node_t *node = malloc(sizeof(*node));
  node->value = duplicate_value(value);
  node->next = NULL;
  if (!list->head) list->head = node;
  if (list->tail) list->tail->next = node;
  list->tail = node;
}

// Pop the value from the top of the stack and add it to the set at [index].
void jack_set_add(jack_state_t *state, int index) {
  if (index < 0) index += state->filled;

  jack_value_t *value;

  // Read the set from the stack
  value = &state->stack[index];
  assert((value->type & JACK_TYPE_MASK) == Set);
  jack_set_t *set = value->set;

  // Pop the value from the stack
  assert(state->filled >= 1);
  value = &state->stack[--state->filled];

  // Look for the value in the hash bucket
  jack_node_t **parent = &(set->buckets[value_hash(value) % set->num_buckets]);
  jack_node_t *node = *parent;
  while (node) {
    // If the value is already in the slot, do nothing.
    if (value_is_equal(value, node->value)) return;
    // Otherwise keep looking.
    parent = &node->next;
    node = *parent;
  }

  // It wasn't there, append to the list.
  node = malloc(sizeof(*node));
  node->value = duplicate_value(value);
  node->next = NULL;
  *parent = node;
}

// pop the key and value from the stack and set into map at [index]
void jack_map_set(jack_state_t *state, int index) {
  if (index < 0) index += state->filled;

  jack_value_t *key, *value;

  // Read the map from the stack
  value = &state->stack[index];
  assert((value->type & JACK_TYPE_MASK) == Map);
  jack_map_t *map = value->map;

  // Pop the value and key from the stack
  assert(state->filled >= 2);
  value = &state->stack[--state->filled];
  key = &state->stack[--state->filled];

  // Look for the key in the hash bucket
  jack_pair_t **parent = &(map->buckets[value_hash(key) % map->num_buckets]);
  jack_pair_t *pair = *parent;
  while (pair) {
    // If the key is already in the slot,
    if (value_is_equal(key, pair->key)) {
      // If the value also matches, do nothing.
      if (value_is_equal(value, pair->value)) return;
      // Otherwise, set the value
      pair->value = duplicate_value(value);
      return;
    }
    // Otherwise keep looking.
    parent = &pair->next;
    pair = *parent;
  }

  // If it wasn't there, append to the list.
  pair = malloc(sizeof(*pair));
  pair->key = duplicate_value(key);
  pair->value = duplicate_value(value);
  pair->next = NULL;
  *parent = pair;
}

