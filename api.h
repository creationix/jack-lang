#ifndef JACK_API_H
#define JACK_API_H

#include <stdint.h>
#include "types.h"

typedef struct {
  int slots;  // Total number of slots in stack
  int filled; // Number of filled slots in stack
  jack_value_t** stack;
} jack_state_t;

jack_state_t* jack_new_state(int slots);

void jack_dump_value(jack_value_t *value);
void jack_dump_state(jack_state_t *state);

void jack_new_integer(jack_state_t *state, intptr_t integer);
char* jack_new_buffer(jack_state_t *state, size_t length, const char* data);
void jack_new_string(jack_state_t *state, size_t length, const char* string);
void jack_new_cstring(jack_state_t *state, const char* string);
void jack_new_symbol(jack_state_t *state, const char* symbol);
void jack_new_set(jack_state_t *state, int num_buckets);
void jack_new_map(jack_state_t *state, int num_buckets);

// List is a doubly linked list of jack values
// All operations work with list at stack[index].
// If the list if empty when reading, NULL is put on stack.

// Create a new empty list and place at top of stack.
void jack_new_list(jack_state_t *state);
// Read the length of the list quickly.
int jack_list_length(jack_state_t *state, int index);
// Move from top of stack to tail of list.
void jack_list_push(jack_state_t *state, int index);
// Move from top of stack to head of list.
void jack_list_insert(jack_state_t *state, int index);
// Move from tail of list to top of stack.
void jack_list_pop(jack_state_t *state, int index);
// Move from head of list to top of stack.
void jack_list_shift(jack_state_t *state, int index);


void jack_set_add(jack_state_t *state, int index);

void jack_map_set(jack_state_t *state, int index);
void jack_map_get(jack_state_t *state, int index);

void jack_pop(jack_state_t *state);
void jack_dup(jack_state_t *state, int index);

intptr_t jack_get_integer(jack_state_t *state, int index);

#endif
