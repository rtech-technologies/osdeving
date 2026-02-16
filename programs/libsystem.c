#include "../include/system.h"
#include "../kernel/kernel.h"

EFI_SYSTEM_TABLE *ST_PTR;

int program_main();

void exit() {
}

void _start(EFI_SYSTEM_TABLE *ST) {
    ST_PTR = ST;
    program_main();
}
