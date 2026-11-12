#!/usr/bin/env python3
"""Reads the view bench's frame log (fishmod/scripts/pcprof_views.inc.c): at each tour stop, the frozen scene drawn
with each setting in turn. Prints what each part of the mod costs (the shipped setting less the setting without it):
on average over the stops, then stop by stop for the RSP, the RDP and the CPU's draw and update.

Usage: fishmod/tools/viewbench.py FOLDER [--md OUT.md]
"""
import collections
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import framelog  # noqa: E402

VARIANTS = ["as shipped", "no fish", "no props", "no bed cards", "no water surface", "no sky", "no fish or props"]
PARTS = [("fish", 1), ("props", 2), ("bed cards (cost of drawing their clumps plant by plant)", 3),
         ("water surface", 4), ("sky", 5), ("fish and props", 6)]
METRICS = [("frame", 2), ("CPU update", 3), ("CPU draw", 4), ("RSP", 5), ("RDP", 7)]
# F3DEX3's counters (frame log version 2, words 52-55; the default set): counts, exact in an emulator too
COUNTS = [("vertices", lambda r: r[52] >> 16), ("tris in", lambda r: r[53] >> 14), ("tris drawn", lambda r: r[52] & 0xFFFF),
          ("FIFO stall us", lambda r: r[54] / 62.5)]


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    out = None
    if "--md" in sys.argv:
        out = sys.argv[sys.argv.index("--md") + 1]
        args = [a for a in args if a != out]
    recs = [r for r in framelog.records(args[0]) if r[1] & 1]
    stops = framelog.stop_names() if hasattr(framelog, "stop_names") else []
    cells = collections.defaultdict(list)
    for r in recs:
        cells[(r[1] >> 24, (r[1] >> 8) & 0xFF)].append(r)

    def mean(rs, i):
        return sum(r[i] for r in rs) / len(rs) / 1000.0 if rs else float("nan")

    lines = []
    p = lines.append
    stop_ids = sorted({k[0] for k in cells})
    p(f"# View bench: {len(recs)} frames counted, {len(stop_ids)} stops\n")
    p("## What each part costs, ms a frame (the stops' average)\n")
    p("| part | " + " | ".join(n for n, _ in METRICS) + " |")
    p("|---|" + "---|" * len(METRICS))
    base = {m: [mean(cells[(s, 0)], i) for s in stop_ids] for m, i in METRICS}
    p("| everything, as shipped | " + " | ".join(f"{sum(base[m]) / len(base[m]):.1f}" for m, _ in METRICS) + " |")
    for name, v in PARTS:
        row = []
        for m, i in METRICS:
            d = [mean(cells[(s, 0)], i) - mean(cells[(s, v)], i) for s in stop_ids if cells[(s, v)] and cells[(s, 0)]]
            row.append(f"{sum(d) / len(d):+.1f}" if d else "")
        p(f"| {name} | " + " | ".join(row) + " |")
    if any(r[52] for r in recs):
        p("\n## What each part draws (F3DEX3's counters, the stops' average a frame)\n")
        p("| part | " + " | ".join(n for n, _ in COUNTS) + " |")
        p("|---|" + "---|" * len(COUNTS))

        def cmean(rs, f):
            return sum(f(r) for r in rs) / len(rs) if rs else float("nan")
        p("| everything, as shipped | " + " | ".join(
            f"{sum(cmean(cells[(s, 0)], f) for s in stop_ids) / len(stop_ids):.0f}" for _, f in COUNTS) + " |")
        for name, v in PARTS:
            row = []
            for _, f in COUNTS:
                d = [cmean(cells[(s, 0)], f) - cmean(cells[(s, v)], f) for s in stop_ids if cells[(s, v)] and cells[(s, 0)]]
                row.append(f"{sum(d) / len(d):+.0f}" if d else "")
            p(f"| {name} | " + " | ".join(row) + " |")
    for m, i in [("RSP", 5), ("RDP", 7), ("CPU draw", 4), ("CPU update", 3), ("frame", 2)]:
        p(f"\n## {m}, stop by stop (ms): as shipped, then what each part costs\n")
        p("| stop | as shipped | " + " | ".join(n.split(" (")[0] for n, _ in PARTS) + " |")
        p("|---|---|" + "---|" * len(PARTS))
        for s in stop_ids:
            name = stops[s] if s < len(stops) else f"stop {s + 1}"
            b = mean(cells[(s, 0)], i)
            p(f"| {s + 1}. {name} | {b:.1f} | " +
              " | ".join(f"{b - mean(cells[(s, v)], i):+.1f}" if cells[(s, v)] else "" for _, v in PARTS) + " |")
    text = "\n".join(lines)
    print(text)
    if out:
        open(out, "w").write(text + "\n")


if __name__ == "__main__":
    main()
