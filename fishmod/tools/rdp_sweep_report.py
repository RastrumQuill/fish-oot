#!/usr/bin/env python3
"""The RDP sweep's report (fishmod/scripts/rdp_sweep.inc.c): per leg, what each thing left out saves, against the
shipped laps on either side of its lap (the night's drift cancelled): the RDP's frame time, its drawing (pipeline
busy), its texture loading (TMEM busy), the hitches; then the whole route.

Usage: fishmod/tools/rdp_sweep_report.py RUN_FOLDER   (the run's folder: packets-NNN.zip)
"""
import collections
import glob
import statistics as st
import struct
import sys
import zipfile

SWEEP = ["shipped", "no props", "shipped", "no fish", "shipped", "no water surface", "shipped", "no sky", "shipped",
         "no HUD", "shipped", "none of them"]


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
    rs = [r for r in records(sys.argv[1]) if r[1] & 1]  # (the counted frames)
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
        cond = SWEEP[lap % 12]
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
        if SWEEP[lap % 12] == "shipped":
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
        n = len({lap for lap in laps if SWEEP[lap % 12] == cond})
        print(f"| {cond} | {n} | {m['rdp']:.2f} | {m['pipe']:.2f} | {m['tmem']:.2f} | {m['frame']:.2f} | {m['late']:+.1f} |")
    print("\n## By leg: RDP ms saved (spread across its laps)\n")
    print("| leg | " + " | ".join(SWEEP[1::2]) + " |")
    print("|---|" + "---|" * 6)
    for leg in legs:
        row = []
        for cond in SWEEP[1::2]:
            d = [x["rdp"] for x in saved[cond][leg]]
            row.append(f"{st.mean(d):.1f} ±{(st.stdev(d) if len(d) > 1 else 0):.1f}" if d else "-")
        print(f"| {leg + 1} | " + " | ".join(row) + " |")


if __name__ == "__main__":
    main()
