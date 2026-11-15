#!/usr/bin/env python3
"""The anatomy sweep's report (fishmod/scripts/anatomy_sweep.inc.c): per leg, what each part of the frame left out
saves, against the shipped laps on either side of its lap: the RDP's frame time, its drawing (pipeline busy), its
texture loading (TMEM busy), the hitches; then the whole route. And the RDP's idle time (its frame time less its
pipeline's busy time: waiting for commands from the RSP, or for the frame to start), as shipped and with each part out.

Usage: fishmod/tools/anatomy_sweep_report.py RUN_FOLDER [--terrain]   (the run's folder: packets-NNN.zip)
  --terrain: the terrain sweep's laps instead (fishmod/scripts/terrain_sweep.inc.c, FISHMOD_TERRAIN_SWEEP)
"""
import collections
import glob
import statistics as st
import struct
import sys
import zipfile

# fishmod_debug_bench.inc.c, FISHMOD_ANATOMY_SWEEP's sSweep: the lap's condition is lap % len(SWEEP)
SWEEP = ["shipped", "no terrain", "shipped", "no Link", "shipped", "no other actors", "shipped", "no effects", "shipped",
         "nothing (all sweeps' parts out)"]
# FISHMOD_TERRAIN_SWEEP's (lap % 6): the terrain anti-aliased without reading back, the sky last, both
if "--terrain" in sys.argv:
    SWEEP = ["shipped", "terrain RA", "shipped", "sky last", "shipped", "RA + sky last"]


def records(folder):
    out = []
    for z in sorted(glob.glob(folder + "/packets*.zip")):
        zf = zipfile.ZipFile(z)
        for n in sorted(zf.namelist()):
            b = zf.read(n)
            if b[:4] != b"FFRM":
                continue
            _, ver, cnt, _ = struct.unpack(">4I", b[:16])
            w = 60 if ver & 0x20000 else 58 if ver & 0x10000 else 56
            for i in range(cnt):
                out.append(struct.unpack_from(">%dI" % w, b, 16 + 4 * w * i))
    return out


def main():
    rs = [r for r in records([a for a in sys.argv[1:] if not a.startswith("--")][0]) if r[1] & 1]  # (the counted frames)
    # the lap number: word 1 bits 16-23, counted on (it wraps at 256)
    by = collections.defaultdict(list)  # (lap, leg) -> records
    for r in rs:
        by[((r[1] >> 16) & 0xFF, r[1] >> 24)].append(r)
    laps = sorted({k[0] for k in by})
    legs = sorted({k[1] for k in by})

    def stat(x):
        if not x:
            return None
        pics = sum(r[56] >> 16 for r in x)
        return {"rdp": st.mean(r[7] for r in x) / 1000, "pipe": st.mean(r[10] for r in x) / 62500,
                "tmem": st.mean(r[11] for r in x) / 62500, "frame": st.mean(r[2] for r in x) / 1000,
                "late": 100 * sum(1 for r in x if (r[56] & 0xFFFF) >= 4) / max(pics, 1)}

    keys = ("rdp", "pipe", "tmem", "frame", "late")
    saved = collections.defaultdict(lambda: collections.defaultdict(list))  # cond -> leg -> [deltas dict]
    drift = []
    for lap in laps:
        cond = SWEEP[lap % len(SWEEP)]
        if cond == "shipped":
            continue
        for leg in legs:
            s = stat(by.get((lap, leg)))
            ref = [stat(by.get((lap + d, leg))) for d in (-1, 1)]
            ref = [x for x in ref if x]
            if not s or not ref:
                continue
            base = {k: st.mean(x[k] for x in ref) for k in keys}
            saved[cond][leg].append({k: base[k] - s[k] for k in keys})
    for lap in laps:
        if SWEEP[lap % len(SWEEP)] == "shipped":
            x = [r for leg in legs for r in by.get((lap, leg), [])]
            if x:
                drift.append((lap, stat(x)["rdp"]))
    print(f"{len(rs)} frames, laps {laps[0]}-{laps[-1]}; shipped laps' RDP from {drift[0][1]:.1f} to {drift[-1][1]:.1f} ms"
          f" (first and last: the night's drift)")
    print("\n## Saved by leaving it out (ms a frame, mean over its laps; RDP / drawing / texture loading; hitch points)\n")
    print("| left out | laps | RDP | drawing | loading | frame | hitches |")
    print("|---|---|---|---|---|---|---|")
    for cond in SWEEP[1::2]:
        all_ = [d for leg in saved[cond] for d in saved[cond][leg]]
        if not all_:
            continue
        m = {k: st.mean(d[k] for d in all_) for k in keys}
        n = len({lap for lap in laps if SWEEP[lap % len(SWEEP)] == cond})
        print(f"| {cond} | {n} | {m['rdp']:.2f} | {m['pipe']:.2f} | {m['tmem']:.2f} | {m['frame']:.2f} | {m['late']:+.1f} |")
    print("\n## The RDP's idle time (frame time less pipeline busy), ms a frame\n")
    print("| condition | RDP | pipeline busy | idle |")
    print("|---|---|---|---|")
    for cond in sorted(set(SWEEP), key=SWEEP.index):
        x = [r for lap in laps if SWEEP[lap % len(SWEEP)] == cond for leg in legs for r in by.get((lap, leg), [])]
        if x:
            s_ = stat(x)
            print(f"| {cond} | {s_['rdp']:.1f} | {s_['pipe']:.1f} | {s_['rdp'] - s_['pipe']:.1f} |")
    print("\n## By leg: RDP ms saved (spread across its laps)\n")
    print("| leg | " + " | ".join(SWEEP[1::2]) + " |")
    print("|---|" + "---|" * 5)
    for leg in legs:
        row = []
        for cond in SWEEP[1::2]:
            d = [x["rdp"] for x in saved[cond][leg]]
            row.append(f"{st.mean(d):.1f} ±{(st.stdev(d) if len(d) > 1 else 0):.1f}" if d else "-")
        print(f"| {leg + 1} | " + " | ".join(row) + " |")


if __name__ == "__main__":
    main()
