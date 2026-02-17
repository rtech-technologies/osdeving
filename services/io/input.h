#ifndef INPUT_H
#define INPUT_H

#include "../../include/types.h"

void input_init();
void input(const char* prompt, char* buffer, uint64 size);
char read_key();

#endif
