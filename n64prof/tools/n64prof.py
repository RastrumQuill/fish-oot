#!/usr/bin/env python3
"""n64prof's PC side: reads a sample dump (n64prof.h's format) and maps it to the program's ELF.

  n64prof.py report DUMP --elf ELF [--top N]        where the time goes, by function (and the dump's own numbers)
  n64prof.py llvm DUMP --elf ELF -o OUT.prof         LLVM's text sample profile (for -fprofile-sample-use)
  n64prof.py compare DUMP_A DUMP_B --elf ELF         two runs side by side, by function (e.g. the console and ares)

DUMP is a file holding the dump; --offset says where in it the dump starts (e.g. a save file: default 0). Several
DUMPs (to report/llvm) are added together. The ELF needs line tables (-g) for the LLVM profile; functions alone
come from its symbols. Needs llvm-symbolizer (LLVM 14 or later) on PATH or in $LLVM_SYMBOLIZER.
"""
import argparse
import collections
import json
import os
import shutil
import struct
import subprocess
import sys

MAGIC = 0x4E363450  # 'N64P'
HEADER = ">16I"     # 0x40 bytes
FIELDS = ["magic", "version", "total", "dropped", "unsaved", "entries", "interval", "unmapped", "bucket_shift",
          "dumps", "u0", "u1", "u2", "u3", "u4", "u5"]


def read_dump(path, offset=0):
    data = open(path, "rb").read()[offset:]
    if len(data) < 0x40:
        sys.exit(f"{path}: too short for a dump")
    head = dict(zip(FIELDS, struct.unpack_from(HEADER, data, 0)))
    if head["magic"] != MAGIC:
        sys.exit(f"{path}: no n64prof dump at offset {offset:#x} (magic {head['magic']:#010x})")
    n = head["entries"]
    if 0x40 + 8 * n > len(data):
        sys.exit(f"{path}: the dump says {n} entries but the file ends first")
    buckets = {}
    for i in range(n):
        addr, count = struct.unpack_from(">II", data, 0x40 + 8 * i)
        buckets[addr] = buckets.get(addr, 0) + count
    return head, buckets


def merge(dumps):
    total = collections.Counter()
    heads = []
    for head, buckets in dumps:
        heads.append(head)
        total.update(buckets)
    return heads, dict(total)


def symbolizer():
    exe = os.environ.get("LLVM_SYMBOLIZER") or shutil.which("llvm-symbolizer")
    if exe is None:
        for v in range(20, 13, -1):
            exe = shutil.which(f"llvm-symbolizer-{v}")
            if exe:
                break
    if exe is None:
        sys.exit("llvm-symbolizer not found (set LLVM_SYMBOLIZER)")
    return exe


def symbolize(elf, addrs):
    """addr -> list of frames, innermost first: dicts with FunctionName, Line, StartLine, Discriminator, FileName."""
    addrs = sorted(set(addrs))
    out = {}
    exe = symbolizer()
    for i in range(0, len(addrs), 20000):
        chunk = addrs[i:i + 20000]
        res = subprocess.run([exe, "--obj", elf, "--output-style=JSON", "--inlining", "--functions=linkage"],
                             input="\n".join(f"{a:#x}" for a in chunk), capture_output=True, text=True, check=True)
        for line, a in zip(res.stdout.splitlines(), chunk):
            rec = json.loads(line)
            frames = [f for f in rec.get("Symbol", []) if f.get("FunctionName") not in (None, "", "??")]
            out[a] = frames
    return out


def expand(buckets, shift):
    """Spread each bucket's samples over its instructions (4 bytes each), so lines within it get their share."""
    per = 1 << shift
    n = max(1, per // 4)
    out = collections.Counter()
    for addr, count in buckets.items():
        base, extra = divmod(count, n)
        for k in range(n):
            c = base + (1 if k < extra else 0)
            if c:
                out[addr + 4 * k] += c
    return out


def by_function(pcs, frames):
    funcs = collections.Counter()
    unknown = 0
    for pc, c in pcs.items():
        f = frames.get(pc)
        if f:
            funcs[f[-1]["FunctionName"]] += c  # the outermost frame: the function the code is in
        else:
            unknown += c
    return funcs, unknown


def describe(heads):
    total = sum(h["total"] for h in heads)
    lost = sum(h["dropped"] + h["unsaved"] for h in heads)
    unmapped = sum(h["unmapped"] for h in heads)
    return (f"{len(heads)} dump(s): {total} samples, {lost} not in the dump ({100.0 * lost / max(total, 1):.1f}%: "
            f"table full or too cold), {unmapped} outside the code and overlays")


def cmd_report(args):
    dumps = [read_dump(p, args.offset) for p in args.dumps]
    heads, buckets = merge(dumps)
    shift = heads[0]["bucket_shift"]
    pcs = expand(buckets, shift)
    frames = symbolize(args.elf, pcs.keys())
    funcs, unknown = by_function(pcs, frames)
    total = sum(pcs.values())
    print(describe(heads))
    print(f"{'share':>7}  {'samples':>9}  function")
    for name, c in funcs.most_common(args.top):
        print(f"{100.0 * c / total:6.2f}%  {c:9d}  {name}")
    if unknown:
        print(f"{100.0 * unknown / total:6.2f}%  {unknown:9d}  (no symbol)")


def cmd_compare(args):
    a_heads, a = merge([read_dump(args.a, args.offset)])
    b_heads, b = merge([read_dump(args.b, args.offset)])
    pa = expand(a, a_heads[0]["bucket_shift"])
    pb = expand(b, b_heads[0]["bucket_shift"])
    frames = symbolize(args.elf, set(pa) | set(pb))
    fa, _ = by_function(pa, frames)
    fb, _ = by_function(pb, frames)
    ta, tb = sum(pa.values()), sum(pb.values())
    print("A:", describe(a_heads))
    print("B:", describe(b_heads))
    names = sorted(set(fa) | set(fb), key=lambda n: -(fa[n] / ta + fb[n] / tb))
    print(f"{'A':>7}  {'B':>7}  {'B/A':>6}  function")
    for n in names[:args.top]:
        sa, sb = 100.0 * fa[n] / ta, 100.0 * fb[n] / tb
        ratio = f"{sb / sa:6.2f}" if sa > 0 else "   new"
        print(f"{sa:6.2f}%  {sb:6.2f}%  {ratio}  {n}")


class Node:
    """A function's samples in LLVM's sense: body counts by (line offset, discriminator), and inlined callees by call
    site."""

    def __init__(self, name):
        self.name = name
        self.body = collections.Counter()
        self.calls = {}  # (offset, disc, callee name) -> Node

    def total(self):
        return sum(self.body.values()) + sum(n.total() for n in self.calls.values())

    def write(self, out, indent):
        pad = " " * indent
        for (off, disc), c in sorted(self.body.items()):
            key = f"{off}.{disc}" if disc else f"{off}"
            out.append(f"{pad}{key}: {c}")
        for (off, disc, callee), node in sorted(self.calls.items()):
            key = f"{off}.{disc}" if disc else f"{off}"
            out.append(f"{pad}{key}: {callee}:{node.total()}")
            node.write(out, indent + 1)


def cmd_llvm(args):
    dumps = [read_dump(p, args.offset) for p in args.dumps]
    heads, buckets = merge(dumps)
    pcs = expand(buckets, heads[0]["bucket_shift"])
    frames = symbolize(args.elf, pcs.keys())
    tops = {}
    skipped = 0
    no_lines = 0
    for pc, c in pcs.items():
        chain = frames.get(pc)
        if not chain:
            skipped += c
            continue
        if any(not f.get("Line") or not f.get("StartLine") for f in chain):
            no_lines += c  # no line table (e.g. code from another compiler): still in the function report
            continue
        outer = chain[-1]
        node = tops.setdefault(outer["FunctionName"], Node(outer["FunctionName"]))
        # walk inward: frame k's Line is where it calls (or inlines) frame k-1
        for k in range(len(chain) - 1, 0, -1):
            f = chain[k]
            off = max(0, f["Line"] - f["StartLine"])
            callee = chain[k - 1]["FunctionName"]
            key = (off, f.get("Discriminator", 0), callee)
            node = node.calls.setdefault(key, Node(callee))
        inner = chain[0]
        node.body[(max(0, inner["Line"] - inner["StartLine"]), inner.get("Discriminator", 0))] += c
    out = []
    for name, node in sorted(tops.items(), key=lambda kv: -kv[1].total()):
        out.append(f"{name}:{node.total()}:0")
        node.write(out, 1)
    with open(args.output, "w") as f:
        f.write("\n".join(out) + "\n")
    total = sum(pcs.values())
    print(describe(heads))
    print(f"wrote {args.output}: {len(tops)} functions; {skipped} samples with no symbol, {no_lines} in code with no "
          f"line table (of {total})")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("report")
    r.add_argument("dumps", nargs="+")
    r.add_argument("--elf", required=True)
    r.add_argument("--top", type=int, default=40)
    r.add_argument("--offset", type=lambda s: int(s, 0), default=0)
    r.set_defaults(fn=cmd_report)
    lv = sub.add_parser("llvm")
    lv.add_argument("dumps", nargs="+")
    lv.add_argument("--elf", required=True)
    lv.add_argument("-o", "--output", required=True)
    lv.add_argument("--offset", type=lambda s: int(s, 0), default=0)
    lv.set_defaults(fn=cmd_llvm)
    c = sub.add_parser("compare")
    c.add_argument("a")
    c.add_argument("b")
    c.add_argument("--elf", required=True)
    c.add_argument("--top", type=int, default=40)
    c.add_argument("--offset", type=lambda s: int(s, 0), default=0)
    c.set_defaults(fn=cmd_compare)
    args = ap.parse_args()
    args.fn(args)


if __name__ == "__main__":
    main()
