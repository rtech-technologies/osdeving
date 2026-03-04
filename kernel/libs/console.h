#ifndef CONSOLE_H
#define CONSOLE_H

#include "../unice64/kernel.h"

void console_init();
void console_clear();
void console_set_color(uint32 fg, uint32 bg);
void print(const char* str);

#endif
