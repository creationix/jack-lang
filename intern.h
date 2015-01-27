#ifndef JACK_INTERN_H
#define JACK_INTERN_H

#include "types.h"

#ifndef JACK_INTERNMENT_SIZE
#define JACK_INTERNMENT_SIZE 1024
#endif

jack_buffer_t* jack_intern(int len, const char *string);
void jack_unintern(jack_buffer_t *buffer);
void jack_dump_internment();

#endif
