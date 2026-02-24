#ifndef CONSOLE_H
#define CONSOLE_H

#include "../../kernel/kernel.h"

void console_init();
void print(const char* str);
void input(const char* prompt, char* buffer, uint64 size);

#endif
