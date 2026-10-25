#!/usr/bin/env python3
"""Reads the play log (fishmod/scripts/playlog.inc.c) from a save: frame times at Lake Hylia per situation, and the
slowest seconds with where Link and the camera were and what was going on.

Usage: fishmod/tools/playlog_report.py save.sav [--md out.md]
"""
import argparse
import struct

ADDR = 0x7A40
SITS = 12
WORST = 24
WEATHER = ["clear", "overcast", "rain"]
FLAGS = [(1, "under"), (2, "rod"), (4, "swim"), (8, "hooked")]


def clock(t):
    m = t * 24 * 60 // 0x10000
    return f"{m // 60:02d}:{m % 60:02d}"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("save")
    ap.add_argument("--md")
    a = ap.parse_args()
    d = open(a.save, "rb").read()
    magic, version, count, frames, samples = struct.unpack(">IHHII", d[ADDR:ADDR + 16])
    if magic != 0x46504C47:
        raise SystemExit("no play log in this save")
    out = []
    p = out.append
    p(f"Play log v{version}: {frames} frames at Lake Hylia ({samples} seconds of {20} frames).\n")
    p("## By situation\n")
    p("| camera | weather | rod | frames | fps | slow (<18 fps) | period | CPU update | CPU draw | RSP | RDP |")
    p("|---|---|---|---|---|---|---|---|---|---|---|")
    off = ADDR + 16
    for i in range(SITS):
        f, slow, *s = struct.unpack(">7I", d[off:off + 28])
        off += 28
        if not f:
            continue
        under, rest = divmod(i, 6)
        weather, rod = divmod(rest, 2)
        ms = [x * 10 / f / 1000 for x in s]
        p(f"| {'under' if under else 'above'} | {WEATHER[weather]} | {'out' if rod else '-'} | {f} | "
          f"{1000 / ms[0]:.1f} | {100 * slow / f:.0f}% | " + " | ".join(f"{x:.1f}" for x in ms) + " |")
    p("\n## The slowest seconds (one per spot; times in ms)\n")
    p("| # | fps | worst frame | CPU upd | CPU draw | RSP | RDP | fish upd | fish draw | props (ms/n) | Link | camera "
      "(eye, yaw/pitch) | time | weather | state | fish/actors | at (s) |")
    p("|" + "---|" * 17)
    rows = []
    for i in range(count):
        o = off + i * 0x2C
        v = struct.unpack(">10H3h3h2hHHBBBB", d[o:o + 0x2C])
        rows.append(v)
    rows.sort(key=lambda v: -v[0])
    for n, v in enumerate(rows):
        per, pmax, upd, drw, rsp, rdp, fu, fd, sc, props = v[:10]
        link, eye = v[10:13], v[13:16]
        yaw, pitch, day, sec, weather, flags, fish, actors = v[16:]
        state = ",".join(name for bit, name in FLAGS if flags & bit) or "-"
        p(f"| {n + 1} | {10000 / per:.1f} | {pmax / 10:.1f} | {upd / 10:.1f} | {drw / 10:.1f} | {rsp / 10:.1f} | "
          f"{rdp / 10:.1f} | {fu / 10:.1f} | {fd / 10:.1f} | {sc / 10:.1f}/{props} | {link[0]},{link[1]},{link[2]} | "
          f"{eye[0]},{eye[1]},{eye[2]} {yaw * 360 // 65536 % 360}/{pitch * 360 // 65536} | {clock(day)} | "
          f"{WEATHER[min(weather, 2)]} | {state} | {fish}/{actors} | {sec} |")
    text = "\n".join(out) + "\n"
    print(text)
    if a.md:
        open(a.md, "w").write(text)


if __name__ == "__main__":
    main()
