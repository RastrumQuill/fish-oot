#!/usr/bin/env python3
"""Compares two collision logs (fishmod/tools/collog.sh): frame by frame, the hash of every collision question and
answer. The same hashes: the two builds' collision answered the same, for every question, on every frame both reached.
The first frame that differs is where the answers (or the game) part.

Usage: fishmod/tools/collog_compare.py A.bin [B.bin]   (one file: just its question counts)
"""
import struct
import sys

KINDS = ["floor", "wall", "ceiling", "line", "sphere"]


def load(path):
    d = open(path, "rb").read()
    return [struct.unpack_from(">8I", d, i) for i in range(0, len(d) - 31, 32)]


def counts(name, recs):
    n = len(recs)
    if not n:
        print(f"{name}: empty")
        return
    tot = [sum(r[1 + k] for r in recs) for k in range(5)]
    print(f"{name}: {n} frames; collision questions a frame: " +
          ", ".join(f"{KINDS[k]} {tot[k] / n:.1f}" for k in range(5)) + f"; all {sum(tot) / n:.1f}")
    rep = sum(r[7] for r in recs)
    if rep:
        print(f"  repeats (floor and wall questions asked exactly as the frame before): {rep / n:.1f} a frame, "
              f"{100 * rep / max(tot[0] + tot[1], 1):.0f}% of floor and wall")


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    a = load(sys.argv[1])
    counts(sys.argv[1], a)
    if len(sys.argv) < 3:
        return
    b = load(sys.argv[2])
    counts(sys.argv[2], b)
    bf = {r[6]: r for r in b}
    same = 0
    for r in a:
        o = bf.get(r[6])
        if o is None:
            continue
        if r[:6] != o[:6]:
            print(f"DIFFERENT from frame {r[6]} (after {same} frames the same): calls {r[1:6]} vs {o[1:6]}")
            return 1
        same += 1
    print(f"SAME: {same} frames, every collision question answered alike")
    return 0


if __name__ == "__main__":
    sys.exit(main())
