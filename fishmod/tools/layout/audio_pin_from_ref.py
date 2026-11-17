#!/usr/bin/env python3
"""Writes fishmod/data/audio_pin.txt's phases anew from a reference build (the tree without the audio work:
fishmod/tools/audio_ref_tree.sh) and a build of the current tree with the file as it is: for each section, how far
what follows the driver is from where the reference has it, modulo the cache, taken off the phase. Needed when code
or data before the driver changes size for other reasons (a merge), which moves the reference itself.

    audio_pin_from_ref.py REF_ELF NEW_ELF        (rewrites the file; then link again and run check_outside.py)"""
import os, re, subprocess, sys


def read(elf):
    syms, count = [], {}
    for line in subprocess.run(["mips-linux-gnu-nm", "-n", "--defined-only", elf], capture_output=True, text=True).stdout.splitlines():
        p = line.split()
        if len(p) == 3 and not p[2].startswith(("$", ".L", "L8", "_")):
            syms.append((int(p[0], 16) & 0xFFFFFFFF, p[2]))
            count[p[2]] = count.get(p[2], 0) + 1
    return [s for s in syms if count[s[1]] == 1]


ref, new = read(sys.argv[1]), read(sys.argv[2])
rn, nn = dict((n, a) for a, n in ref), dict((n, a) for a, n in new)
path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "../../data/audio_pin.txt")
pins = {"text": "Fishmod_AudioPinText", "data": "gFishmodAudioPinData", "rodata": "gFishmodAudioPinRodata", "bss": "gFishmodAudioPinBss"}
out = []
for line in open(path):
    m = re.match(r"(\w+)\s+0x([0-9A-Fa-f]+)\s+0x([0-9A-Fa-f]+)", line)
    if not m:
        out.append(line)
        continue
    sec, mod, phase = m.group(1), int(m.group(2), 16), int(m.group(3), 16)
    after = [(a, n) for a, n in new if a > nn[pins[sec]] and n in rn and not n.startswith("gFishmod")]
    # (several symbols must agree: one alone could be a function or table that itself changed)
    deltas = [(a - rn[n]) % mod for a, n in after[:12]]
    delta = max(set(deltas), key=deltas.count)
    print("%s: %s is %#x from the reference's place: phase %#x -> %#x" % (sec, after[0][1], delta, phase, (phase - delta) % mod))
    out.append("%-6s 0x%X 0x%04X\n" % (sec, mod, (phase - delta) % mod))
open(path, "w").write("".join(out))
