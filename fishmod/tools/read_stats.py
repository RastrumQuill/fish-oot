#!/usr/bin/env python3
"""Reads the timing stats a debug build writes to the end of SRAM (see Fishmod_DebugTiming) from an emulator save
file (.ram/.sra/.sav, 32 KB, big-endian) and prints whole-run averages and worst frames in milliseconds.

Usage: fishmod/tools/read_stats.py save.ram [more.ram ...]
       fishmod/tools/read_stats.py --table run.txt [more.txt ...]   (a Markdown table of saved outputs)
       fishmod/tools/read_stats.py --prof save.ram   (the fish profile timers: where fish time goes)"""
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


PROF = [
    ("calibrate", "one timer on its own"),
    ("update (engine)", "the whole update call for fish"),
    ("  glide", "throttled in-between frames"),
    ("  AI", "deciding and moving"),
    ("    water queries", "surface/floor lookups (also counted in AI and debug)"),
    ("      raycasts", "collision raycasts (inside water queries)"),
    ("  swim wave", "body and fin angles"),
    ("  debug block", "debug readout, not in release"),
    ("draw (engine)", "the whole Actor_Draw for fish: lights, matrix, draw"),
    ("  setup", "render setup and texture segments"),
    ("  root matrix", "the fish's root matrix"),
    ("  skeleton", "limb matrices and display lists"),
    ("    limb callbacks", "part scales, tints, head position"),
    ("  whiskers", "catfish whiskers"),
    ("  billboard", "far fish drawn flat"),
    ("  AI: steering", "steering toward a target (in AI)"),
    ("  AI: spook", "splash and Link checks (in AI)"),
    ("  AI: see", "open water to a point (in AI)"),
    ("  AI: unstick", "the stuck watchdog (in AI)"),
    ("habitat props", "the scattered props (drawn by the manager)"),
    ("particles", "underwater bubbles and specks"),
]


def prof(path):
    data = open(path, "rb").read()
    off = 0x7E00
    magic, frames = struct.unpack(">II", data[off:off + 8])
    if magic != 0x464D5052 or frames == 0:
        print(f"{path}: no profile")
        return
    n = len(PROF)
    usec = struct.unpack(">%dI" % n, data[off + 8:off + 8 + 4 * n])
    calls = struct.unpack(">%dI" % n, data[off + 8 + 4 * n:off + 8 + 8 * n])
    timer = usec[0] / calls[0] if calls[0] else 0.0  # what one timed block costs by itself
    print(f"{path}: {frames} frames; one timer costs {timer:.2f} us (taken off each slot below)")
    print(f"  {'slot':22s} {'ms/frame':>9s} {'calls/frame':>12s} {'us/call':>8s}")
    for i, (name, _) in enumerate(PROF[1:], 1):
        if calls[i] == 0:
            continue
        net = max(0.0, usec[i] - timer * calls[i])
        print(f"  {name:22s} {net / frames / 1000:9.3f} {calls[i] / frames:12.1f} {net / calls[i]:8.2f}")


def main():
    if sys.argv[1:2] == ["--prof"]:
        for path in sys.argv[2:]:
            prof(path)
        return
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
