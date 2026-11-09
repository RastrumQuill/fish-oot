#!/usr/bin/env python3
"""fish-oot: marks an overlay's relocation table (fado's .reloc.s) for standard ELF HI16/LO16 pairing: puts the
RELOC_ELF_PAIRING entry (include/libu64/overlay.h) first and counts it. The loader (Overlay_Relocate) then pairs each
HI16 with the next LO16 in the table, as the linker does, instead of by register. For overlays built with clang.

Usage: tools/reloc_elf_pairing.py OVERLAY.reloc.s   (edits it in place)
"""
import re
import sys

RELOC_ELF_PAIRING = 0x00454C46


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    path = sys.argv[1]
    s = open(path).read()
    if f"0x{RELOC_ELF_PAIRING:08X}" in s:
        return  # already marked
    m = re.search(r"^\.word (\d+) # relocCount\n", s, re.M)
    tail = re.search(r"((?:^\.word 0\n)*)\n?^\.word 0x([0-9A-Fa-f]+) # (\w+OverlayInfoOffset)\n?\Z", s, re.M)
    if not m or not tail:
        sys.exit(f"{path}: not fado's layout (relocCount, padding, OverlayInfoOffset)")
    # The table is padded to 16 bytes and ends with its own size (the loader finds it from the end): one entry more
    # changes both.
    count = int(m.group(1)) + 1
    words = 5 + count + 1  # the 4 sizes and the count, the relocations, the offset
    pad = -words % 4
    s = (s[:m.start()] + f".word {count} # relocCount\n\n.word 0x{RELOC_ELF_PAIRING:08X} # RELOC_ELF_PAIRING\n"
         + s[m.end():tail.start()] + ".word 0\n" * pad + f"\n.word 0x{(words + pad) * 4:08X} # {tail.group(3)}\n")
    open(path, "w").write(s)


if __name__ == "__main__":
    main()
