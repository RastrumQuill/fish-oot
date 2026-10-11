#!/usr/bin/env python3
"""Reads the timing stats a debug build writes to the end of SRAM (see Fishmod_DebugTiming) from an emulator save
file (.ram/.sra/.sav, 32 KB, big-endian) and prints whole-run averages and worst frames in milliseconds.

Usage: fishmod/tools/read_stats.py save.ram [more.ram ...]
       fishmod/tools/read_stats.py --table run.txt [more.txt ...]   (a Markdown table of saved outputs)"""
import os
import re
import struct
import sys

NAMES = ["RSP gfx", "RDP", "CPU update", "CPU draw", "fish update", "fish draw", "rod update", "rod draw", "frame", "RSP audio"]


def read(path):
    data = open(path, "rb").read()
    off = 0x7F00
    magic, frames = struct.unpack(">II", data[off:off + 8])
    if magic != 0x464D5354 or frames == 0:
        return None
    n = len(NAMES)
    sums = struct.unpack(">%dI" % n, data[off + 8:off + 8 + 4 * n])
    maxs = struct.unpack(">%dI" % n, data[off + 8 + 4 * n:off + 8 + 8 * n])
    return frames, [s / frames / 1000.0 for s in sums], [m / 1000.0 for m in maxs]


def table(paths):
    cols = ["CPU update", "CPU draw", "fish update", "fish draw", "frame", "RSP gfx"]
    print("| run | frames | " + " | ".join(cols) + " |")
    print("|---|---:|" + "---:|" * len(cols))
    for path in paths:
        label = os.path.splitext(os.path.basename(path))[0]
        text = open(path).read() if os.path.exists(path) else ""
        m = re.search(r"(\d+) frames", text)
        if not m:
            print(f"| {label} | no stats |" + " |" * len(cols))
            continue
        cells = []
        for c in cols:
            v = re.search(re.escape(c) + r"\s+avg\s+([\d.]+) ms\s+worst\s+([\d.]+) ms", text)
            cells.append(f"{v.group(1)} ({v.group(2)})" if v else "")
        print(f"| {label} | {m.group(1)} | " + " | ".join(cells) + " |")


def main():
    if sys.argv[1:2] == ["--table"]:
        table(sys.argv[2:])
        return
    for path in sys.argv[1:]:
        r = read(path)
        if r is None:
            print(f"{path}: no stats")
            continue
        frames, avg, mx = r
        print(f"{path}: {frames} frames")
        for name, a, m in zip(NAMES, avg, mx):
            print(f"  {name:12s} avg {a:6.2f} ms   worst {m:6.2f} ms")


if __name__ == "__main__":
    main()
