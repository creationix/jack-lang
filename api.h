#ifndef JACK_API_H
#define JACK_API_H

#include <stdint.h>
#include <stdbool.h>
#include "types.h"

jack_state_t* jack_new_state(int slots);
void jack_free_state(jack_state_t *state);

void jack_dump_value(jack_value_t *value);
void jack_dump_state(jack_state_t *state);

void jack_new_integer(jack_state_t *state, intptr_t integer);
void jack_new_boolean(jack_state_t *state, bool boolean);
char* jack_new_buffer(jack_state_t *state, size_t length, const char* data);
void jack_new_symbol(jack_state_t *state, const char* symbol);


void jack_new_native(jack_state_t *state, jack_native_t *native);
// Call native function.  Function is at top of stack right below arguments.
// Arguments are popped in reverse order, pushed in normal order.
// The native function gets it's own state and stack.
// [-n,+m] Pop's argc + 1 items from stack and pushes return value number of items back on.
int jack_native_call(jack_state_t *state, int argc);

// Create a new exec value with stateful area and C behavior area.
void* jack_new_exec(jack_state_t *state, jack_function_t *function, size_t size);
// Works same as jack_native_call, but on stateful exec value.
int jack_exec_call(jack_state_t *state, int argc);

// List is a doubly linked list of jack values
// All operations work with list at stack[index].
// If the list if empty when reading, NULL is put on stack.

// Create a new empty list and place at top of stack.
// [0,+1] Pushes new list on stack.
void jack_new_list(jack_state_t *state);
// Read the length of the list quickly.
// [0,0] No changes to stack.
int jack_list_length(jack_state_t *state, int index);
// Move from top of stack to tail of list.  Returns new length.
// [-1,0] Pops value from stack.
int jack_list_push(jack_state_t *state, int index);
// Move from top of stack to head of list.  Returns new length.
// [-1,0] Pops value from stack.
int jack_list_insert(jack_state_t *state, int index);
// Move from tail of list to top of stack.  Returns new length.
// [0,+1] Pushes value on stack.
int jack_list_pop(jack_state_t *state, int index);
// Move from head of list to top of stack.  Returns new length.
// [0,+1] Pushes value on stack.
int jack_list_shift(jack_state_t *state, int index);


// Map is an unordered collection of unique keys with associated values.
// All operations work with map at stack[index] and value at top.

// Create a new empty map with `num_buckets` sized hashtable.
// [0,+1] Pushes map on stack
void jack_new_map(jack_state_t *state, int num_buckets);
// Read the length of the map quickly.
// [0,0] No changes to stack
int jack_map_length(jack_state_t *state, int index);
// Add or replace entry.
// Returns true if the key wasn't in the map yet.
// [-2,0] Pops value and then key from stack (key must be pushed first).
bool jack_map_set(jack_state_t *state, int index);
// [-1,0] Pops value from stack. Creates symbol on the fly.
bool jack_map_set_symbol(jack_state_t *state, int index, const char* symbol);
// Pop the key from the stack and push the associated value from the map.
// Pushed NULL if there is no such key.
// Returns true if the key was found.
// [-1,+1] Pops key from stack, pushes value on stack.
bool jack_map_get(jack_state_t *state, int index);
// [0,+1] Pushes value on stack. Creates symbol on the fly.
bool jack_map_get_symbol(jack_state_t *state, int index, const char* symbol);
// Remove an item from the map by key, returns true if it was there.
// [-1,0] Pops key from stack.
bool jack_map_delete(jack_state_t *state, int index);
// [0,0] Doesn't affect stack.  Creates symbol on the fly.
bool jack_map_delete_symbol(jack_state_t *state, int index, const char* symbol);
// Returns true if key at top of stack is in map.
// [-1,0] Pops key from stack.
bool jack_map_has(jack_state_t *state, int index);
// [0,0] Doesn't affect stack.  Creates symbol on the fly.
bool jack_map_has_symbol(jack_state_t *state, int index, const char* symbol);

// Pop and discard the top value in the stack
// [-1,0] Pops value from stack.
void jack_pop(jack_state_t *state);
// Duplicate value in stack at [index] and push to top of stack.
// [0,+1] Pushes duplicate on stack.
void jack_dup(jack_state_t *state, int index);

intptr_t jack_get_integer(jack_state_t *state, int index);

#endif
