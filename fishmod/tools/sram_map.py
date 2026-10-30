#!/usr/bin/env python3
"""The SRAM map of fishmod's debug and bench builds, read from include/fishmod_sram.h (the one source).

    from sram_map import SRAM
    data[SRAM["HASHES"]:SRAM["HASHES"] + 0x40]

Run it to print the map.
"""
import os
import re

HEADER = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "..", "include", "fishmod_sram.h"))


def load(path=HEADER):
    out = {}
    with open(path) as f:
        for m in re.finditer(r"^#define FISHMOD_SRAM_(\w+) (0x[0-9A-Fa-f]+)", f.read(), re.M):
            out[m.group(1)] = int(m.group(2), 16)
    return out


SRAM = load()

if __name__ == "__main__":
    for name, addr in sorted(SRAM.items(), key=lambda kv: kv[1]):
        print(f"0x{addr:04X}  {name}")
