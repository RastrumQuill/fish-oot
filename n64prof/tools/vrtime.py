#!/usr/bin/env python3
"""Reads vrtime's results (n64prof/vrtime/: the VR4300 timing tests) and prints CPU cycles per operation.

  vrtime.py SAVE [--offset N] [--vs OTHER_SAVE] [--json OUT]

Each test's time is its best run, less the empty loop's (the loop counter and branch); the Count register ticks once
every two CPU cycles. --vs puts a second run beside it (e.g. the console and ares). --json writes the numbers for
other tools (a compiler's cost tables).
"""
import argparse
import json
import struct
import sys

MAGIC = 0x56525431  # 'VRT1'

NOTES = {
    "nop": "the pipeline's pace: 1 if nothing stalls",
    "alu.dep": "integer ALU result to the next instruction",
    "lw.use0": "load then use at once (less lw.ind and alu.ind: the load-use stall)",
    "lw.chain": "a load whose address is the last load's result (cache hit latency)",
    "sw+lw.use": "store, load the same word, use it",
    "mult.lat": "mult then mflo: the multiply's latency",
    "div.lat": "div then mflo",
    "add.s.lat": "FPU add, each needing the last",
    "add.s.thr": "FPU adds that don't depend on each other",
    "b.taken": "a taken branch (with its delay slot)",
    "bl.not": "a branch-likely not taken (its delay slot cancelled)",
    "dc.chase": "a data-cache miss each load (64 KB, shuffled)",
    "dc.seq.read": "loads streaming through new lines (16 bytes apart)",
    "dc.seq.write": "stores streaming through new lines (write-back of the dirty lines they evict)",
    "uncached.chase": "uncached loads (KSEG1), each needing the last",
    "ic.miss": "straight code through 64 KB (per instruction; every 8th starts a new line)",
}


def read(path, offset):
    data = open(path, "rb").read()[offset:]
    magic, count, runs = struct.unpack_from(">III", data, 0)
    if magic != MAGIC:
        sys.exit(f"{path}: no vrtime results at {offset:#x}")
    out = {}
    for i in range(count):
        name, loops, ops, best, worst = struct.unpack_from(">16sIIII", data, 0x20 + 0x20 * i)
        out[name.split(b"\0")[0].decode()] = dict(loops=loops, ops=ops, best=best, worst=worst)
    return out, runs


def cycles(res):
    """Test -> CPU cycles per operation (the empty loop's cost per pass taken off)."""
    base = res["loop"]["best"] / res["loop"]["loops"]
    out = {}
    for name, r in res.items():
        if name == "loop":
            continue
        ticks = r["best"] - base * r["loops"]
        out[name] = 2.0 * ticks / (r["loops"] * r["ops"])
        out[name + ":spread"] = (r["worst"] - r["best"]) / max(r["best"], 1)
    out["loop.pass"] = 2.0 * base  # cycles for the loop's own counter and branch, once a pass
    return out


def derived(c):
    d = {}
    if "lw.use0" in c and "lw.ind" in c and "alu.ind" in c:
        d["load-use stall"] = c["lw.use0"] - (c["lw.ind"] + c["alu.ind"])
    if "dc.chase" in c and "lw.chain" in c:
        d["D-cache miss (load)"] = c["dc.chase"] - c["lw.chain"]
    if "uncached.chase" in c and "lw.chain" in c:
        d["uncached load"] = c["uncached.chase"]
    if "ic.miss" in c and "nop" in c:
        d["I-cache line miss"] = (c["ic.miss"] - c["nop"]) * 8
    if "dc.seq.write" in c and "dc.seq.read" in c:
        d["dirty line write-back"] = c["dc.seq.write"] - c["dc.seq.read"]
    return d


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("save")
    ap.add_argument("--offset", type=lambda s: int(s, 0), default=0)
    ap.add_argument("--vs")
    ap.add_argument("--json")
    args = ap.parse_args()
    res, runs = read(args.save, args.offset)
    c = cycles(res)
    cv = cycles(read(args.vs, args.offset)[0]) if args.vs else None
    print(f"{len(res)} tests, best of {runs} runs; cycles per operation (the loop's {c['loop.pass']:.1f} cycles a "
          f"pass taken off)")
    head = f"{'test':<16} {'cycles':>7}" + (f" {'vs':>7}" if cv else "") + "  spread  note"
    print(head)
    for name in res:
        if name == "loop":
            continue
        line = f"{name:<16} {c[name]:7.2f}"
        if cv:
            line += f" {cv.get(name, float('nan')):7.2f}"
        spread = c[name + ":spread"]
        line += f"  {100 * spread:5.1f}%  {NOTES.get(name, '')}"
        print(line)
    print("\nderived (cycles):")
    dv = derived(cv) if cv else {}
    for k, v in derived(c).items():
        print(f"  {k:<24} {v:7.1f}" + (f" {dv.get(k, float('nan')):7.1f}" if cv else ""))
    if args.json:
        with open(args.json, "w") as f:
            json.dump({"cycles": {k: v for k, v in c.items() if ":" not in k}, "derived": derived(c)}, f, indent=1)


if __name__ == "__main__":
    main()
