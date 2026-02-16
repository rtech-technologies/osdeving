#ifndef POWER_H
#define POWER_H

#include "types.h"

void power_init();
void reboot();
void shutdown();
void reboot_prompt();
void shutdown_prompt();

void shell_shutdown();
void shell_reboot();

#endif
