#include <stdio.h>
#include <assert.h>
#include "types-dual.h"

int main() {
  printf("struct jack_value = %lu\n", sizeof(struct jack_value));
  printf("struct jack_node = %lu\n", sizeof(struct jack_node));
  printf("struct jack_list = %lu\n", sizeof(struct jack_list));
  printf("struct jack_pair = %lu\n", sizeof(struct jack_pair));
  printf("struct jack_map = %lu\n", sizeof(struct jack_map));
  printf("struct jack_function = %lu\n", sizeof(struct jack_function));
  printf("struct jack_thread = %lu\n", sizeof(struct jack_thread));
  assert(sizeof(struct jack_value) == sizeof(void*)*2);
  return 0;
}
