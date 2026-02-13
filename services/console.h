#ifndef CONSOLE_H
#define CONSOLE_H

#include "../include/types.h"

void console_init();
void console_print(const char* str);
void console_wait_for_key();
char console_read_key();

#endif
