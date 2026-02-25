#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 2 ]; then
  echo "Usage: $0 <source.c> <out.rsl>"
  exit 2
fi

SRC="$1"
OUT="$2"
TMPDIR="/tmp/osx2_rsl_build_$$"
mkdir -p "$TMPDIR"

OBJ="$TMPDIR/tmp.o"
ELF="$TMPDIR/tmp.elf"
BIN="$TMPDIR/tmp.bin"

gcc -ffreestanding -fno-builtin -nostdlib -c "$SRC" -o "$OBJ"
ld -Ttext=0x0 "$OBJ" -o "$ELF"
objcopy -O binary "$ELF" "$BIN"

TOOLS_DIR="$(dirname "$0")"
if [ ! -x "$TOOLS_DIR/mk_rsl" ]; then
  if [ -f "$TOOLS_DIR/mk_rsl.c" ]; then
    gcc "$TOOLS_DIR/mk_rsl.c" -o "$TOOLS_DIR/mk_rsl"
  else
    echo "mk_rsl tool not found in $TOOLS_DIR" >&2
    exit 3
  fi
fi

"$TOOLS_DIR/mk_rsl" "$BIN" "$OUT" 20 0

echo "Built $OUT"

rm -rf "$TMPDIR"
