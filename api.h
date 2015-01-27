#ifndef JACK_API_H
#define JACK_API_H

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
void jack_new_list(jack_state_t *state);
void jack_new_set(jack_state_t *state, int num_buckets);
void jack_new_map(jack_state_t *state, int num_buckets);
void jack_list_push(jack_state_t *state, int index);
void jack_set_add(jack_state_t *state, int index);

void jack_map_set(jack_state_t *state, int index);
void jack_map_get(jack_state_t *state, int index);

void jack_pop(jack_state_t *state);
void jack_dup(jack_state_t *state, int index);

intptr_t jack_get_integer(jack_state_t *state, int index);

#endif
