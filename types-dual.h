#include <stdint.h>
#include <stdbool.h>

enum jack_type {
  Nil      = 0,  // nil
  Number   = 1,  // rational number
  Boolean  = 2,  // false, true
  Buffer   = 4,  // mutable buffer
  Symbol   = 6,  // immutable interned string
  List     = 8,  // linked list of values
  Map      = 10, // map of values
  Function = 12, // block of code with arguments
  Thread   = 14, // thread of execution
};



struct jack_value {
  // If bits are all zero then it's Nil
  // else If last bit is 1 then then rshift 1 to get denominator.
  // else rshift 1 to get enum type.
  uintptr_t type;
  union {
    bool boolean;
    intptr_t numerator;
    union jack_box *box;
  };
};

struct jack_buffer {
  uintptr_t size;
  char data[];
};

struct jack_symbol {
  uint64_t hash;
  uintptr_t refcount;
  uintptr_t size;
  char data[];
};

// Nodes in the doubly linked list
struct jack_node {
  struct jack_node* prev;
  struct jack_node* next;
  struct jack_value value;
};

// The list container
struct jack_list {
  uintptr_t length;
  struct jack_node* head;
  struct jack_node* tail;
};

// Pairs in the singly-linked lists in the hastable buckets.
struct jack_pair {
  struct jack_value key;
  struct jack_value value;
  struct jack_pair* next;
};

// Map container as a hash table of pair chains.
struct jack_map {
  uintptr_t length;
  uintptr_t num_buckets;
  struct jack_pair* buckets[];
};

struct jack_function {

};

struct jack_thread {


};

union jack_box {
  struct jack_buffer buffer;
  struct jack_symbol symbol;
  struct jack_list list;
  struct jack_map map;
  struct jack_function function;
  struct jack_thread thread;
};

enum jack_type jack_typeof(struct jack_value value) {
  return value.type & 1 ? Number : value.type;
}

bool jack_tobool(struct jack_value value) {
  return value.type == Nil ? false :
         value.type == Boolean ? value.boolean :
         true;
}
