#ifndef JACK_TYPES_H
#define JACK_TYPES_H

#include <stdint.h>
#include <stdlib.h>

// REF_COUNT must be larger than the largest enum value below.
// Also it must be a power of two for the mask to work.
#define JACK_TYPE_MASK  7
#define JACK_REF_COUNT 8

typedef enum {
  Integer,
  Buffer,
  String,
  Symbol,
  List,
  Set,
  Map,
  Function,
} jack_type_t;

struct jack_value_s;

typedef struct jack_node_s {
  struct jack_value_s *value;
  struct jack_node_s *next;
} jack_node_t;

typedef struct jack_pair_s {
  struct jack_value_s *key;
  struct jack_value_s *value;
  struct jack_pair_s *next;
} jack_pair_t;

typedef struct {
  jack_node_t *head;
  jack_node_t *tail;
} jack_list_t;

typedef struct {
  int num_buckets;
  jack_node_t* buckets[];
} jack_set_t;

typedef struct {
  int num_buckets;
  jack_pair_t* buckets[];
} jack_map_t;

typedef struct {
  int length;
  char data[];
} jack_buffer_t;

typedef struct {
  // TODO: design
} jack_function_t;

typedef struct jack_value_s {
  // type and ref_count are packed together.
  // count if left shifted 4 bits
  union {
    jack_type_t type;
    int ref_count;
  };
  union {
    intptr_t integer;
    jack_buffer_t *buffer;
    jack_list_t *list;
    jack_set_t *set;
    jack_map_t *map;
    jack_function_t *function;
  };
} jack_value_t;

#endif
