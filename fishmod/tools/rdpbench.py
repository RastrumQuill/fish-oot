#!/usr/bin/env python3
"""Reads the RDP calibration bench's run (fishmod_debug_rdpbench.inc.c, FISHMOD_RDP_BENCH): each test's RDP time and
counters from the frame log, less the empty frame's, and what that comes to per pixel and per primitive.

Usage: fishmod/tools/rdpbench.py FOLDER [--csv OUT.csv]
FOLDER: what `sc64deployer debug` saved (binaryout-*.bin; the tests' descriptions are the "RDPTEST" lines in the
debug.log beside them, or, from ares, in text packets among the binaryouts). The morning runner's packets.zip can be
unzipped there.

The frame log's RDP time is the scheduler's (from the graphics task's start to the RDP's end: the RSP's own work is
in it); the RDP's counters (62.5 MHz cycles) are its own: all cycles while it ran (clock), the command buffer busy,
the pipeline busy (drawing), and loading TMEM.
"""
import argparse
import os
import re
import statistics
import sys
import zipfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import framelog  # noqa: E402

KINDS = ["empty", "fill", "texrect", "tris", "loads", "syncs", "switch"]
RCP_MHZ = 62.5


def descriptions(folder):
    out = {}
    texts = []
    for f in sorted(os.listdir(folder)):
        p = os.path.join(folder, f)
        if f.endswith(".log") or f.startswith("binaryout"):
            try:
                texts.append(open(p, "rb").read().decode("utf-8", "replace"))
            except OSError:
                pass
    for t in texts:
        for m in re.finditer(r"RDPTEST (\d+) (\d+) (\d+) (\d+) (\d+) ([^\n\r]+)", t):
            n, kind, mode, count, size = (int(m.group(k)) for k in range(1, 6))
            out[n] = {"kind": KINDS[kind], "mode": m.group(6).strip(), "count": count, "size": size}
    return out


def pixels(d):
    side = d["size"]
    area = 320 * 240 if side >= 320 else side * side
    if d["kind"] == "empty":
        return 0
    return d["count"] * area


def prims(d):
    if d["kind"] == "tris":
        return d["count"] * 2
    return d["count"]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("folder")
    ap.add_argument("--csv")
    args = ap.parse_args()

    folder = args.folder
    for f in os.listdir(folder):
        if f == "packets.zip":
            zipfile.ZipFile(os.path.join(folder, f)).extractall(folder)
    desc = descriptions(folder)
    if not desc:
        sys.exit("no RDPTEST lines found (the debug.log beside the packets, or text packets among them)")
    recs = [r for r in framelog.records(folder) if (r[1] & 1)]
    by = {}
    for r in recs:
        by.setdefault(r[1] >> 24, []).append(r)

    def med(rs, f):
        return statistics.median(f(r) for r in rs) if rs else 0.0

    rows = []
    for n in sorted(desc):
        rs = by.get(n, [])
        rows.append({
            "test": n, **desc[n], "frames": len(rs),
            "rdp_us": med(rs, lambda r: r[7]), "rsp_us": med(rs, lambda r: r[5]),
            "clock_us": med(rs, lambda r: r[8] / RCP_MHZ), "buf_us": med(rs, lambda r: r[9] / RCP_MHZ),
            "pipe_us": med(rs, lambda r: r[10] / RCP_MHZ), "tmem_us": med(rs, lambda r: r[11] / RCP_MHZ),
            "tris_drawn": med(rs, lambda r: r[52] & 0xFFFF),
        })
    base = rows[0] if rows and rows[0]["kind"] == "empty" else None
    print(f"# RDP calibration bench: {len(recs)} counted frames, {len(rows)} tests")
    if base:
        print(f"Baseline (the frame setup alone): RDP {base['rdp_us']:.0f} us, pipeline {base['pipe_us']:.0f} us, "
              f"clock {base['clock_us']:.0f} us")
    print()
    print("| # | kind | mode | count x size | pixels | frames | RDP us | pipe us | tmem us | buf us | RSP us "
          "| ns/pixel (pipe) | ns/prim (pipe) |")
    print("|---|---|---|---|---|---|---|---|---|---|---|---|---|")
    for r in rows:
        px = pixels(r)
        dpipe = r["pipe_us"] - (base["pipe_us"] if base else 0)
        r["pixels"] = px
        r["d_pipe_us"] = dpipe
        r["ns_per_pixel"] = 1000 * dpipe / px if px else 0
        r["ns_per_prim"] = 1000 * dpipe / prims(r) if prims(r) else 0
        print(f"| {r['test']} | {r['kind']} | {r['mode']} | {r['count']} x {r['size']} | {px} | {r['frames']} | "
              f"{r['rdp_us']:.0f} | {r['pipe_us']:.0f} | {r['tmem_us']:.0f} | {r['buf_us']:.0f} | {r['rsp_us']:.0f} | "
              f"{r['ns_per_pixel']:.2f} | {r['ns_per_prim']:.0f} |")
    if args.csv:
        import csv
        with open(args.csv, "w", newline="") as f:
            w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
            w.writeheader()
            w.writerows(rows)


if __name__ == "__main__":
    main()
