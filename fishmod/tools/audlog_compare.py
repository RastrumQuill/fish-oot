#!/usr/bin/env python3
"""Compares two audio check logs (fishmod/tools/collog.sh --audio, the audlog script): output buffer by output buffer,
the hash of its samples. The same hashes: the two builds' audio made the same sound, sample for sample, for every buffer
both reached. The first buffer that differs is where they part.

Usage: fishmod/tools/audlog_compare.py A.bin [B.bin]   (one file: just its counts)
"""
import struct
import sys


def load(path):
    d = open(path, "rb").read()
    return [struct.unpack_from(">4I", d, i) for i in range(0, len(d) - 15, 16)]


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    a = load(sys.argv[1])
    print(f"{sys.argv[1]}: {len(a)} buffers, {sum(r[1] for r in a)} stereo samples, "
          f"{len(set(r[0] for r in a))} different")
    if len(sys.argv) < 3:
        return 0
    b = load(sys.argv[2])
    print(f"{sys.argv[2]}: {len(b)} buffers")
    n = min(len(a), len(b))
    for i in range(n):
        if a[i][:3] != b[i][:3]:
            print(f"DIFFERENT from buffer {i} (update {a[i][2]} vs {b[i][2]}; length {a[i][1]} vs {b[i][1]}), "
                  f"after {i} the same")
            return 1
    print(f"SAME: {n} buffers, every sample alike")
    return 0


if __name__ == "__main__":
    sys.exit(main())
