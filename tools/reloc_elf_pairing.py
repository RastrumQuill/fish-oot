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
    m = re.search(r"^\.word (\d+) # relocCount\n", s, re.M)
    if not m:
        sys.exit(f"{path}: no relocCount line")
    if f"0x{RELOC_ELF_PAIRING:08X}" in s:
        return  # already marked
    s = (s[:m.start()] + f".word {int(m.group(1)) + 1} # relocCount\n\n"
         f".word 0x{RELOC_ELF_PAIRING:08X} # RELOC_ELF_PAIRING\n" + s[m.end():])
    open(path, "w").write(s)


if __name__ == "__main__":
    main()
