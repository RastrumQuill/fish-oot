#!/usr/bin/env python3
"""Lists a build's functions for icache_layout: hex address, size, 1 if the linker can place it anywhere in the audio
driver's block (the driver's own, src/audio/, each in its own section; and the scheduler's functions that are linked
there, fishmod/data/sched_text_slots.txt), name.   layout_funcs.py ELF > FUNCS"""
import os, subprocess, sys

moved = set()
for line in open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "../../data/sched_text_slots.txt")):
    p = line.split()
    if len(p) == 2 and not line.startswith("#"):
        moved.add(p[0])

elf = sys.argv[1]
syms = []
for line in subprocess.run(["mips-linux-gnu-nm", "-n", "--defined-only", elf], capture_output=True, text=True).stdout.splitlines():
    p = line.split()
    if len(p) == 3 and p[1] in "tT" and not p[2].startswith(("$", ".L", "L8")):
        a = int(p[0], 16) & 0xFFFFFFFF
        if syms and syms[-1][0] == a:
            continue
        syms.append((a, p[2]))
files = subprocess.run(["mips-linux-gnu-addr2line", "-e", elf] + ["0x%x" % a for a, _ in syms], capture_output=True,
                       text=True).stdout.splitlines()
for i, (a, name) in enumerate(syms):
    size = syms[i + 1][0] - a if i + 1 < len(syms) else 0
    print("%08x %d %d %s" % (a, size, 1 if (("/src/audio/" in files[i] and not name.startswith("Fishmod_AudioPin")) or name in moved) else 0, name))
