#include <stdint.h>

struct jack_buffer {
  uint64_t size;
  char data[];
};

struct jack_symbol {
  uint64_t hash;
  uint64_t refcount;
  uint64_t size;
  char data[];
};

// Nodes in the doubly linked list
struct jack_node {
  struct jack_value* value;
  struct jack_node* prev;
  struct jack_node* next;
};

// The list container
struct jack_list {
  uint64_t length;
  struct jack_node* head;
  struct jack_node* tail;
};

// Pairs in the singly-linked lists in the hastable buckets.
struct jack_pair {
  struct jack_value* key;
  struct jack_value* value;
  struct jack_pair* next;
};

// Map container as a hash table of pair chains.
struct jack_map {
  uint64_t length;
  uint64_t num_buckets;
  struct jack_pair* buckets[];
};

struct jack_function {

};

struct jack_thread {


};

enum jack_type {
  Primitive, // nil, false, true
  Number, // rational number
  Buffer, // mutable buffer
  Symbol, // immutable interned string
  List, // linked list of values
  Map, // map of values
  Function, // block of code with arguments
  Thread, // thread of execution
};

// 64 bits overall for all types including flags and refcount.
struct jack_value {
  enum jack_type type : 4;
  unsigned int gc : 4; // reserved bits for GC flags
  uint64_t refcount : 56;
  union {
    int64_t value;
    // Boxed types with refcount and pointer to heap allocated data
    union {
      struct jack_buffer *buffer;
      struct jack_symbol *symbol;
      struct jack_list *list;
      struct jack_map *map;
      struct jack_function *function;
      struct jack_thread *thread;
    };
  };
};
