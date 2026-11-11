#!/usr/bin/env python3
"""Two frame logs (fishmod/tools/framelog.py's input) side by side on the same route: area by area (each leg, all
laps together), each part of the frame, and overall. Frames that sent a USB packet are left out of both (the
profiler's own cost), as framelog.py does.

Usage: fishmod/tools/framelog_vs.py A_FOLDER B_FOLDER [--names A,B]
"""
import argparse
import collections
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import framelog  # noqa: E402


def load(folder, sects):
    recs = framelog.records(folder)
    return [r for r in recs if r[16 + sects.index("fm_debug")] <= 5000]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("a")
    ap.add_argument("b")
    ap.add_argument("--names", default="A,B")
    args = ap.parse_args()
    na, nb = args.names.split(",")
    sects = framelog.enum_names("include/fishmod.h", "FishmodSect", "FISHMOD_SECT_")
    cats = framelog.enum_names("include/actor_profile.h", "ActorCategory", "ACTORCAT_")
    A, B = load(args.a, sects), load(args.b, sects)
    stops = framelog.stop_names()

    def m(rs, i):
        return sum(r[i] for r in rs) / max(len(rs), 1) / 1000

    def fps(rs):
        return len(rs) / (sum(r[2] for r in rs) / 1e6)

    def slow(rs):
        return 100 * sum(1 for r in rs if r[2] > framelog.SLOW_US) / max(len(rs), 1)
    print(f"# {na} against {nb}, the same route\n")
    print(f"| | {na} | {nb} | difference |\n|---|---|---|---|")
    print(f"| frames compared | {len(A)} | {len(B)} | |")
    print(f"| fps (frames compared) | {fps(A):.1f} | {fps(B):.1f} | {fps(B) - fps(A):+.1f} |")
    print(f"| frames over 52 ms | {slow(A):.0f}% | {slow(B):.0f}% | {slow(B) - slow(A):+.0f} |")
    rows = [("frame period", 2), ("CPU update", 3), ("CPU draw", 4), ("RSP graphics", 5), ("RDP", 7),
            ("fish update", 12), ("fish draw", 13), ("props draw", 14)]
    rows += [(f"update: {n}", 16 + i) for i, n in enumerate(sects)] + [(f"actors: {n}", 36 + i) for i, n in enumerate(cats)]
    print("\n## Each part, ms a frame\n")
    print(f"| part | {na} | {nb} | difference |\n|---|---|---|---|")
    for name, i in rows:
        a, b = m(A, i), m(B, i)
        if max(a, b) >= 0.3:
            print(f"| {name} | {a:.1f} | {b:.1f} | {b - a:+.1f} |")
    print("\n## By area (each leg, all laps): ms a frame, and frames over 52 ms\n")
    print(f"| area | {na} | {nb} | difference | {na} slow | {nb} slow |\n|---|---|---|---|---|---|")
    la, lb = collections.defaultdict(list), collections.defaultdict(list)
    for r in A:
        la[r[1] >> 24].append(r)
    for r in B:
        lb[r[1] >> 24].append(r)
    for leg in sorted(set(la) & set(lb)):
        if len(la[leg]) < 50 or len(lb[leg]) < 50:
            continue
        a, b = m(la[leg], 2), m(lb[leg], 2)
        name = f"{leg + 1}: {stops[leg % len(stops)]} -> {stops[(leg + 1) % len(stops)]}" if stops else f"leg {leg + 1}"
        print(f"| {name} | {a:.1f} | {b:.1f} | {b - a:+.1f} | {slow(la[leg]):.0f}% | {slow(lb[leg]):.0f}% |")


if __name__ == "__main__":
    main()
