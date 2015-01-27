#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "intern.h"

// bucket for string interning with ref-count
struct bucket {
  int count;
  unsigned long hash;
  struct bucket *next;
  jack_buffer_t buffer;
};

static struct bucket* internment[JACK_INTERNMENT_SIZE];

// djb2 with xor modification http://www.cse.yorku.ca/~oz/hash.html
static unsigned long string_hash(int length, const char* string) {
  unsigned long hash = 5381;
  int i;
  for (i = 0; i < length; ++i) {
    // hash * 33 xor c
    hash = ((hash << 5) + hash) ^ string[i];
  }
  return hash;
}

jack_buffer_t* jack_intern(int len, const char *string) {
  unsigned long hash = string_hash(len, string);
  int index = hash % JACK_INTERNMENT_SIZE;
  struct bucket *bucket = internment[index];
  while (bucket) {
    // TODO: add string comparison recheck if hash collisions happen
    if (bucket->hash == hash) {
      bucket->count++;
      return &bucket->buffer;
    }
    if (!bucket->next) break;
    bucket = bucket->next;
  }
  struct bucket *new_bucket = malloc(sizeof(*bucket) + len);
  new_bucket->count = 1;
  memcpy(new_bucket->buffer.data, string, len);
  new_bucket->buffer.length = len;
  new_bucket->hash = hash;
  new_bucket->next = NULL;
  if (bucket) {
    bucket->next = new_bucket;
  }
  else {
    internment[index] = new_bucket;
  }
  return &new_bucket->buffer;
}

void jack_unintern(jack_buffer_t *buffer) {
  unsigned long hash = string_hash(buffer->length, buffer->data);
  int index = hash % JACK_INTERNMENT_SIZE;
  struct bucket **parent = &(internment[index]);
  struct bucket *bucket = *parent;
  while (bucket) {
    if (bucket->hash == hash) {
      if (!--bucket->count) {
        *parent = bucket->next;
      }
      return;
    }
    parent = &bucket->next;
    bucket = bucket->next;
  }
  assert(0); // No such string!
}

void jack_dump_internment() {
  int i;
  for (i = 0; i < JACK_INTERNMENT_SIZE; ++i) {
    struct bucket *bucket = internment[i];
    printf("%d: ", i);
    while (bucket) {
      printf("%.*s(%d) ", bucket->buffer.length, bucket->buffer.data, bucket->count);
      bucket = bucket->next;
    }
    printf("\n");
  }
}
