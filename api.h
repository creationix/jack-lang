#ifndef JACK_API_H
#define JACK_API_H

#include "types.h"

typedef struct {
  size_t slots;  // Total number of slots in stack
  size_t filled; // Number of filled slots in stack
  jack_value_t* stack;
} jack_state_t;

jack_state_t* jack_new_state(int slots);

void jack_dump_value(jack_value_t *value);
void jack_dump_state(jack_state_t *state);

void jack_push_integer(jack_state_t *state, int integer);
char* jack_push_buffer(jack_state_t *state, int length, char* data);
void jack_push_string(jack_state_t *state, int length, const char* string);
void jack_push_cstring(jack_state_t *state, const char* string);
void jack_push_symbol(jack_state_t *state, const char* symbol);
void jack_push_list(jack_state_t *state);
void jack_push_set(jack_state_t *state, int num_buckets);
void jack_push_map(jack_state_t *state, int num_buckets);
void jack_list_push(jack_state_t *state, int index);
void jack_set_add(jack_state_t *state, int index);
void jack_map_set(jack_state_t *state, int index);


#endif
