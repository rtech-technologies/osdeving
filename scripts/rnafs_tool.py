import struct
import sys
import os

# RNAFS v1 Structure
BLOCK_SIZE = 512
MAGIC = b"RNAFS\0\0\0" # Padded to 8 bytes

# Layout:
# block 0: Superblock
# block 1: Bitmap (1 block = 4096 files/blocks supported)
# block 2-5: Directory Entries (64 entries per block? No, 64 entries total in v0)
# block 6+: Data

class RNAFSTool:
    def __init__(self, filename):
        self.filename = filename

    def format(self, size_mb=16):
        total_blocks = (size_mb * 1024 * 1024) // BLOCK_SIZE
        print(f"Formatting {self.filename} with {total_blocks} blocks...")

        with open(self.filename, "wb") as f:
            # Superblock
            # magic(8), total(8), bitmap_st(8), bitmap_bl(8), dir_st(8), dir_bl(8), data_st(8)
            sb = struct.pack("<8sQQQQQQ", MAGIC, total_blocks, 1, 1, 2, 4, 6)
            f.write(sb.ljust(BLOCK_SIZE, b"\0"))

            # Bitmap (mark first 6 blocks used)
            bitmap = bytearray(BLOCK_SIZE)
            bitmap[0] = 0x3F # binary 00111111
            f.write(bitmap)

            # Directory (4 blocks, all empty)
            f.write(b"\0" * (4 * BLOCK_SIZE))

            # Rest is data
            f.write(b"\0" * ((total_blocks - 6) * BLOCK_SIZE))

    def add_file(self, host_path, guest_name):
        if not os.path.exists(host_path):
            print(f"Error: {host_path} not found")
            return

        with open(host_path, "rb") as src:
            data = src.read()

        size = len(data)
        blocks_needed = (size + BLOCK_SIZE - 1) // BLOCK_SIZE

        with open(self.filename, "r+b") as f:
            # Read SB
            f.seek(0)
            sb = struct.unpack("<8sQQQQQQ", f.read(56))
            data_start = sb[6]
            total_blocks = sb[1]

            # Find free space in bitmap (very simple contiguous find)
            f.seek(BLOCK_SIZE)
            bitmap = bytearray(f.read(BLOCK_SIZE))

            start_block = -1
            for i in range(data_start, total_blocks - blocks_needed):
                found = True
                for j in range(blocks_needed):
                    byte_idx = (i + j) // 8
                    bit_idx = (i + j) % 8
                    if bitmap[byte_idx] & (1 << bit_idx):
                        found = False
                        break
                if found:
                    start_block = i
                    break

            if start_block == -1:
                print("Error: No contiguous space found")
                return

            # Mark bitmap
            for i in range(start_block, start_block + blocks_needed):
                bitmap[i // 8] |= (1 << (i % 8))
            f.seek(BLOCK_SIZE)
            f.write(bitmap)

            # Find empty dir entry
            dir_start = sb[4]
            f.seek(dir_start * BLOCK_SIZE)
            entries = f.read(4 * BLOCK_SIZE)

            entry_size = 128 # name(64), start(8), size(8), flags(4), pad(44)
            found_entry = -1
            for i in range(0, len(entries), entry_size):
                if entries[i] == 0:
                    found_entry = i
                    break

            if found_entry == -1:
                print("Error: Directory full")
                return

            # Write entry
            # name(64s), start(Q), size(Q), flags(I)
            name_bytes = guest_name.encode("ascii")[:63]
            new_entry = struct.pack("<64sQQI", name_bytes, start_block, size, 0)
            f.seek(dir_start * BLOCK_SIZE + found_entry)
            f.write(new_entry)

            # Write data
            f.seek(start_block * BLOCK_SIZE)
            f.write(data)
            print(f"Added {guest_name} at block {start_block} ({size} bytes)")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: rnafs_tool.py <image> <command> [args]")
        print("Commands: format, add <host_path> <guest_name>")
        sys.exit(1)

    tool = RNAFSTool(sys.argv[1])
    cmd = sys.argv[2]
    if cmd == "format": tool.format()
    elif cmd == "add": tool.add_file(sys.argv[3], sys.argv[4])
