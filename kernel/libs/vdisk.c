#include "vdisk.h"
#include "kutils.h"
#include "console.h"

static vdisk_t vdisk_registry[MAX_VDISKS];
static uint32 vdisk_count = 0;

void vdisk_init() {
    vdisk_count = 0;
    memset(vdisk_registry, 0, sizeof(vdisk_registry));
    print("VDisk: Storage Virtualization Layer initialized.\n");
}

vdisk_t* vdisk_open(const char* name) {
    for (uint32 i = 0; i < vdisk_count; i++) {
        if (strcmp(vdisk_registry[i].name, name) == 0) {
            return &vdisk_registry[i];
        }
    }

    /* Auto-create VDISK on RAM0 if not found for v0 compatibility */
    if (vdisk_count < MAX_VDISKS) {
        strcpy(vdisk_registry[vdisk_count].name, name);
        vdisk_registry[vdisk_count].physical = connect_get_device("/CONNECT/RAM0/");
        vdisk_registry[vdisk_count].start_lba = 0;
        vdisk_registry[vdisk_count].end_lba = vdisk_registry[vdisk_count].physical ? vdisk_registry[vdisk_count].physical->total_lba - 1 : 0;
        vdisk_registry[vdisk_count].is_mounted = 0;
        return &vdisk_registry[vdisk_count++];
    }

    return (void*)0;
}

int vdisk_mount_verify(vdisk_t* vd) {
    if (!vd || !vd->physical || !vd->physical->is_active) {
        print("VDISK: CANNOT FIND DISK (Hardware Offline)\n");
        return 0;
    }

    uint32 signature = 0;
    if (vdisk_read(vd, 0, 1, &signature)) {
        if (signature == 0xDEADBEEF) {
            vd->is_mounted = 1;
            return 1;
        }
    }

    print("VDISK: Signature mismatch. Access denied.\n");
    return 0;
}

int vdisk_read(vdisk_t* vd, uint64 lba, uint32 count, void* buffer) {
    if (!vd || !vd->physical || !vd->physical->is_active) return 0;
    uint64 phys_lba = vd->start_lba + lba;
    if (phys_lba + count - 1 > vd->end_lba) return 0;

    if (vd->physical->type == DEVICE_TYPE_RAM) {
        uint64 offset = phys_lba * vd->physical->sector_size;
        uint64 size = (uint64)count * vd->physical->sector_size;
        memcpy(buffer, (uint8*)vd->physical->base_addr + offset, size);
        return 1;
    }
    return 0;
}

int vdisk_write(vdisk_t* vd, uint64 lba, uint32 count, const void* buffer) {
    if (!vd || !vd->physical || !vd->physical->is_active) return 0;
    uint64 phys_lba = vd->start_lba + lba;
    if (phys_lba + count - 1 > vd->end_lba) return 0;

    if (vd->physical->type == DEVICE_TYPE_RAM) {
        uint64 offset = phys_lba * vd->physical->sector_size;
        uint64 size = (uint64)count * vd->physical->sector_size;
        memcpy((uint8*)vd->physical->base_addr + offset, buffer, size);
        return 1;
    }
    return 0;
}
