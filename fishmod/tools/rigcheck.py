#!/usr/bin/env python3
"""Checks the fish rig's cleaned display lists against the lists as they were (fishmod_rig.c, Rig_Peephole; debug
builds keep both: gFishmodRigCheck). Each list is walked with its own model of what the RDP and RSP hold: every state
command's value (the othermode words bit by bit, unknown bits apart), the tile descriptors, and texture memory (which
load last wrote each address). The two walks must give the same events in the same order: every vertex load, matrix,
call and triangle, each with the same state and texture memory. A difference means the cleaned list draws something
differently.

Usage: fishmod/tools/rigcheck.py RAM.bin ELF   (RAM.bin: RDRAM from 0x80000000, the game paused after fish were drawn)
"""
import struct
import subprocess
import sys

STATE = {0xFC: "combine", 0xFA: "prim", 0xFB: "env", 0xF8: "fog", 0xF9: "blend", 0xF7: "fill", 0xEE: "primdepth",
         0xED: "scissor", 0xD7: "texture", 0xFD: "timg"}
EVENTS = {0x01, 0x05, 0x06, 0x07, 0xDA, 0xD8, 0xDE, 0xE4, 0xE5, 0xF6, 0xE1, 0xF1, 0xDB, 0xDC, 0x0B, 0xEF}
IGNORED = {0xE7, 0xE6, 0xE8, 0x00}  # syncs and no-ops: not state


def walk(ram, start, end):
    st = {}
    om = {0xE2: [0, 0], 0xE3: [0, 0]}  # (known mask, value)
    geom = []  # the geometry commands since the start (idempotent repeats collapse)
    tiles = {}
    tmem = {}  # tmem address -> the load that last wrote it
    events = []
    p = start
    while p < end:
        w0, w1 = struct.unpack_from(">II", ram, p & 0x1FFFFFFF)
        p += 8
        op = w0 >> 24
        if op in STATE:
            st[STATE[op]] = (w0 & 0xFFFFFF, w1)
        elif op in (0xE2, 0xE3):
            ln = (w0 & 0xFF) + 1
            lo = 32 - ((w0 >> 8) & 0xFF) - ln
            mask = ((1 << ln) - 1) << lo
            om[op][0] |= mask
            om[op][1] = (om[op][1] & ~mask) | (w1 & mask)
        elif op == 0xD9:
            if not geom or geom[-1] != (w0, w1):
                geom.append((w0, w1))
        elif op == 0xF5:
            tiles[(w1 >> 24) & 7] = (w0, w1)
        elif op == 0xF2:
            tiles[("size", (w1 >> 24) & 7)] = (w0, w1)
        elif op in (0xF3, 0xF4, 0xF0):
            t = (w1 >> 24) & 7
            desc = tiles.get(t)
            addr = (desc[0] & 0x1FF) if desc else None
            tmem[addr] = (st.get("timg"), desc, w0, w1)
        elif op in IGNORED:
            pass
        else:
            if op in (0x05, 0x06, 0x07, 0xE4, 0xE5, 0xF6):
                snap = (tuple(sorted(st.items())), tuple(om[0xE2]), tuple(om[0xE3]), tuple(geom),
                        tuple(sorted(tiles.items(), key=str)), tuple(sorted(tmem.items(), key=str)))
                events.append((w0, w1, snap))
            else:
                events.append((w0, w1, None))
            if op not in EVENTS:
                events.append(("unknown", op))
    return events


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    ram = open(sys.argv[1], "rb").read()
    out = subprocess.run(["mips-linux-gnu-nm", sys.argv[2]], capture_output=True, text=True).stdout
    sym = {p[2]: int(p[0], 16) for p in (l.split() for l in out.splitlines()) if len(p) == 3}
    tab = sym["gFishmodRigCheck"] & 0x1FFFFFFF
    n = struct.unpack_from(">I", ram, sym["gFishmodRigCheckCount"] & 0x1FFFFFFF)[0]
    bad = 0
    for i in range(min(n, 16)):
        rs, re, cs, ce = struct.unpack_from(">4I", ram, tab + 16 * i)
        a = walk(ram, rs, re)
        b = walk(ram, cs, ce)
        same = a == b
        draws = sum(1 for e in a if len(e) == 3 and e[2] is not None)
        print("list %d: %d commands -> %d (%.0f%% fewer), %d draws: %s" % (
            i, (re - rs) // 8, (ce - cs) // 8, 100 * (1 - (ce - cs) / max(re - rs, 1)), draws,
            "the same" if same else "DIFFERENT"))
        if not same:
            bad += 1
            for k, (x, y) in enumerate(zip(a, b)):
                if x != y:
                    print("  first difference at event %d" % k)
                    break
    print("%d lists checked, %d different" % (min(n, 16), bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
