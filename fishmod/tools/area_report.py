#!/usr/bin/env python3
"""Reads the area bench (fishmod/scripts/hwbench_area.inc.c) from a save: at each spot, the frame time and every step
of the game's update, as shipped and with the mod's fish, props, water surface and sky off, in the iron boots and in
his own; and the costliest kinds of actor.

Usage: fishmod/tools/area_report.py save.sav [--md out.md]
"""
import argparse
import os
import re
import struct

SPOTS = ["Inflow mouth", "West of the bridge", "East of the bridge", "Rock garden", "Under the bridge",
         "On the bridge", "West weed flat", "River, upstream", "North beach (elsewhere)", "Drop-off (elsewhere)"]
VARIANTS = ["iron, as shipped", "iron, mod off", "own boots, as shipped", "own boots, mod off"]
HEAD = ["period", "cpu_update", "cpu_draw", "rsp", "rdp", "fish_upd", "fish_draw", "scatter", "actors_n"]
SECTS = ["fm_play", "fm_debug", "objects", "roomreq", "col_at", "col_oc", "col_damage", "col_clear", "actors",
         "dynapoly", "cutscene", "effects", "roomfx", "skybox", "message", "interface", "animtask", "sfxsrc", "camera",
         "env"]
CATS = ["switch", "bg", "player", "explosive", "npc", "enemy", "prop", "itemaction", "misc", "boss", "door", "chest"]


def actor_names():
    path = os.path.join(os.path.dirname(__file__), "..", "..", "include", "tables", "actor_table.h")
    names = {}
    for m in re.finditer(r"/\* 0x([0-9A-Fa-f]+) \*/ DEFINE_ACTOR\w*\((?:\w+, )?(\w+)", open(path).read()):
        names[int(m.group(1), 16)] = m.group(2).replace("ACTOR_", "")
    return names


def read(path):
    d = open(path, "rb").read()
    magic, version, spots, variants, sects, cats, top, hw = struct.unpack(">IBBBBBBB", d[:11])
    if magic != 0x46415245:
        raise SystemExit("no area bench results in this save")
    laps, spot, variant = struct.unpack(">III", d[12:24])
    nm = len(HEAD) + sects + cats
    o = 24
    cells = []
    for s in range(spots):
        row = []
        for v in range(variants):
            frames = struct.unpack(">I", d[o:o + 4])[0]
            sums = struct.unpack(f">{nm}I", d[o + 4:o + 4 + 4 * nm])
            maxp = struct.unpack(">I", d[o + 4 + 4 * nm:o + 8 + 4 * nm])[0]
            o2 = o + 8 + 4 * nm
            tops = []
            for k in range(top):
                aid, most, _, us = struct.unpack(">HBBI", d[o2 + 8 * k:o2 + 8 * k + 8])
                if us:
                    tops.append((aid, most, us))
            o = o2 + 8 * top
            row.append(dict(frames=frames, sums=sums, maxp=maxp, top=tops))
        cells.append(row)
    return dict(version=version, hw=hw, laps=laps, spot=spot, variant=variant, sects=sects, cats=cats, cells=cells)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("save")
    ap.add_argument("--md")
    a = ap.parse_args()
    r = read(a.save)
    names = actor_names()
    out = []
    p = out.append
    ns = r["sects"]
    p(f"# Area bench ({'original-hardware build' if r['hw'] else 'emulator build'})\n")
    p(f"{r['laps']} full laps (it had got to spot {r['spot'] + 1}, variant {r['variant']}). Times are ms per frame.\n")
    p("## Each spot\n")
    p("| spot | how | frames | fps | CPU update | CPU draw | RSP | RDP | fish upd | actors | worst |")
    p("|---|---|---|---|---|---|---|---|---|---|---|")
    for s, row in enumerate(r["cells"]):
        for v, c in enumerate(row):
            f = c["frames"]
            if not f:
                continue
            g = lambda i: c["sums"][i] / f / 1000
            p(f"| {SPOTS[s] if v == 0 else ''} | {VARIANTS[v]} | {f} | {1000 / g(0):.1f} | {g(1):.1f} | {g(2):.1f} | "
              f"{g(3):.1f} | {g(4):.1f} | {g(5):.1f} | {c['sums'][8] / f:.0f} | {c['maxp'] / 1000:.0f} |")
    p("\n## The update's steps (ms per frame; 'other' is the update less the steps)\n")
    p("| spot | how | " + " | ".join(SECTS) + " | other |")
    p("|---|---|" + "---|" * (len(SECTS) + 1))
    base = len(HEAD)
    for s, row in enumerate(r["cells"]):
        for v, c in enumerate(row):
            f = c["frames"]
            if not f:
                continue
            st = [c["sums"][base + i] / f / 1000 for i in range(ns)]
            # the actors' step includes dynapoly: "other" is what no step covers
            other = c["sums"][1] / f / 1000 - sum(x for i, x in enumerate(st) if SECTS[i] != "dynapoly")
            p(f"| {SPOTS[s] if v == 0 else ''} | {VARIANTS[v]} | " + " | ".join(f"{x:.1f}" for x in st) +
              f" | {other:.1f} |")
    p("\n## Actor updates by category (ms per frame)\n")
    p("| spot | how | " + " | ".join(CATS) + " |")
    p("|---|---|" + "---|" * len(CATS))
    for s, row in enumerate(r["cells"]):
        for v, c in enumerate(row):
            f = c["frames"]
            if not f:
                continue
            ct = [c["sums"][base + ns + i] / f / 1000 for i in range(r["cats"])]
            p(f"| {SPOTS[s] if v == 0 else ''} | {VARIANTS[v]} | " + " | ".join(f"{x:.1f}" for x in ct) + " |")
    p("\n## The costliest kinds of actor (ms per frame; most at once)\n")
    for s, row in enumerate(r["cells"]):
        for v, c in enumerate(row):
            f = c["frames"]
            if not f or not c["top"]:
                continue
            items = ", ".join(f"{names.get(aid, hex(aid))} {us / f / 1000:.2f} (x{most})" for aid, most, us in c["top"])
            p(f"- **{SPOTS[s]}, {VARIANTS[v]}**: {items}")
    text = "\n".join(out) + "\n"
    print(text)
    if a.md:
        open(a.md, "w").write(text)


if __name__ == "__main__":
    main()
