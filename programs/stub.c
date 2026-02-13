#include <efi.h>

EFI_SYSTEM_TABLE *ST;
extern int program_main();

int _start(EFI_SYSTEM_TABLE *SystemTable) {
    ST = SystemTable;
    // We also need to initialize the console/memory if they need more than just ST
    // But for now, they just use ST.
    return program_main();
}
