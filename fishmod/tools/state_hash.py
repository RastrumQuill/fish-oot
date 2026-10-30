#!/usr/bin/env python3
"""Compares the state fingerprints two script runs wrote to SRAM (fishmod_debug.c, StateHash_Update): every 200
frames, a hash of every actor's id, params, position and rotation. Runs of the same script are deterministic in ares
(fixed seed, scripted input), so a refactor that shouldn't change behaviour must give the same hashes.

Usage: state_hash.py <a.ram> [<b.ram> ...]    (one file: print its hashes; more: compare each with the first)
Exit status 1 if any differs.
"""
import struct
import sys


def read(path):
    data = open(path, "rb").read()[0x7A00:0x7A40]
    magic, count = struct.unpack(">II", data[:8])
    if magic != 0x46485348:
        return None
    return list(struct.unpack(">14I", data[8:]))[:count]


def main():
    runs = [(p, read(p)) for p in sys.argv[1:]]
    for p, h in runs:
        if h is None:
            print(f"{p}: no fingerprints")
    base_path, base = runs[0]
    bad = False
    for p, h in runs[1:]:
        if base is None or h is None:
            bad = True
            continue
        n = min(len(base), len(h))
        first = next((i for i in range(n) if base[i] != h[i]), None)
        if first is None and n > 0:
            # (runs last a fixed wall time, so one may get further than the other: the frames both reached count)
            print(f"{p}: same as {base_path} ({n} fingerprints in common, of {len(h)} and {len(base)})")
        else:
            bad = True
            where = f"from frame {(first + 1) * 200}" if first is not None else "(no fingerprints in common)"
            print(f"{p}: DIFFERS from {base_path} {where} ({len(h)} vs {len(base)} fingerprints)")
    if len(runs) == 1 and base is not None:
        for i, v in enumerate(base):
            print(f"frame {(i + 1) * 200:5d}: {v:08x}")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
