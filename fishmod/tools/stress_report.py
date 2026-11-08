#!/usr/bin/env python3
"""Reads a stress bench's save (fishmod/scripts/stress_fish.inc.c, stress_plants.inc.c; fishmod_debug_stress.inc.c):
per step, the count of fish or plants and the frame rate, CPU, RSP and RDP times, averaged over every pass, and the
largest count that held 20 fps.

Usage: fishmod/tools/stress_report.py SAVE
"""
import struct
import sys

FULL_SPEED_US = 50800  # the bench's own test: a step's mean frame no longer than this (19.7 fps), and no more than
# 1 frame in 20 over 52 ms (the mean alone hides a stutter)
FIELDS = ["count", "frames", "period", "cpuUpdate", "cpuDraw", "rsp", "rdp", "fishUpdate", "fishDraw", "worst",
          "slow"]


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    d = open(sys.argv[1], "rb").read()
    magic, kind, passes, steps, version = struct.unpack(">IBBBB", d[:8])
    if magic != 0x46535452:
        sys.exit(f"no stress bench results in {sys.argv[1]}")
    what = {1: "fish", 2: "plants"}.get(kind, f"kind {kind}")
    print(f"# Stress bench: {what} ({passes} passes done, {steps} steps reached)\n")
    print(f"| {what} | fps | frame ms | CPU update | CPU draw | RSP | RDP | fish update | fish draw | worst frame | "
          f"frames over 52 ms |")
    print("|---|---|---|---|---|---|---|---|---|---|---|")
    best = None
    for i in range(steps):
        v = dict(zip(FIELDS, struct.unpack(">11I", d[8 + 44 * i:8 + 44 * (i + 1)])))
        n = v["frames"]
        if n == 0:
            continue
        ms = lambda k: v[k] / n / 1000.0  # noqa: E731
        period = v["period"] / n
        full = (period <= FULL_SPEED_US) and (v["slow"] * 20 <= n)
        if full:
            best = v["count"]
        print(f"| {v['count']} | {1e6 / period:.1f} | {period / 1000:.1f} | {ms('cpuUpdate'):.1f} | "
              f"{ms('cpuDraw'):.1f} | {ms('rsp'):.1f} | {ms('rdp'):.1f} | {ms('fishUpdate'):.1f} | "
              f"{ms('fishDraw'):.1f} | {v['worst'] / 1000:.0f} ms | {100.0 * v['slow'] / n:.0f}% |")
    print()
    if best is not None:
        print(f"**Most {what} at full speed (20 fps): {best}.**")
    else:
        print("No step held full speed.")
    print("\nThe frame is capped at 50 ms (20 fps): the CPU, RSP and RDP times say which one runs out first.")


if __name__ == "__main__":
    main()
