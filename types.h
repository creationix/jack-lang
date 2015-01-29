#ifndef JACK_TYPES_H
#define JACK_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

// REF_COUNT must be larger than the largest enum value below.
// Also it must be a power of two for the mask to work.
#define JACK_TYPE_MASK  7
#define JACK_REF_COUNT 8

struct jack_value_s;
struct jack_stack_s;

typedef struct {
  void* data;
  struct jack_stack_s *stack;
} jack_state_t;

typedef int (jack_call_t)(jack_state_t *state);

typedef enum {
  Boolean,
  Integer,
  Buffer,
  Symbol,
  List,
  Map,
  Function,
} jack_type_t;

// Nodes in the doubly linked list
typedef struct jack_node_s {
  struct jack_value_s *value;
  struct jack_node_s *prev;
  struct jack_node_s *next;
} jack_node_t;

// The list container
typedef struct {
  int length;
  jack_node_t *head;
  jack_node_t *tail;
} jack_list_t;

// Pairs in the singly-linked lists in the hastable buckets.
typedef struct jack_pair_s {
  struct jack_value_s *key;
  struct jack_value_s *value;
  struct jack_pair_s *next;
} jack_pair_t;

// Map container as a hash table of pair chains.
typedef struct {
  int length;
  int num_buckets;
  jack_pair_t* buckets[];
} jack_map_t;

typedef struct {
  int length;
  char data[];
} jack_buffer_t;

typedef struct {
  jack_call_t* call;
  jack_state_t* state;
} jack_function_t;

typedef struct jack_value_s {
  // type and ref_count are packed together.
  // count if left shifted 4 bits
  union {
    jack_type_t type;
    int ref_count;
  };
  union {
    bool boolean;
    intptr_t integer;
    jack_buffer_t *buffer;
    jack_list_t *list;
    jack_map_t *map;
    jack_function_t *function;
  };
} jack_value_t;

typedef struct jack_stack_s {
  int length; // Total number of slots in the stack.
  int top;    // Number of used slots / index to first empty slot.
  jack_value_t* values[]; // Inline array of value pointers.
} jack_stack_t;

#endif
