#!/usr/bin/env python3
"""Finds code that may be fighting the instruction cache: hot paths bigger than the cache can comfortably keep.

  hotcode.py PROFILE... --elf ELF --frames N [--hot-kb 4] [--item-kb 2] [--match REGEX] [--why] [-o OUT.md]

PROFILE: ares's exact profile (ARES_CPUPROF, with ARES_CPUPROF_OVL so overlays are placed), as for sysprof.py.
--frames: the frames the profile covers (a replay's FISHMOD-REPLAY END line says).

The VR4300's instruction cache is 16 KB, direct mapped. A routine doesn't have it to itself, so the lines here are
filters for what to look at, not targets:
  - "hot": a routine with a real share of the frame whose code per call is over --hot-kb (4 KB);
  - "per item": code that runs many times a frame (once per fish, actor, prop, collision polygon), a routine called
    that often or a loop inside one, whose code per item is over --item-kb (2 KB).
Candidates for a smaller inner path: the cold parts split off, a tighter rewrite, or assembly.

How the size is counted. From the profile, each instruction's run count; from the ELF, each direct call (jal) and its
target. A routine's code per call is its own instructions that run in at least a quarter of its calls, plus the same
for every routine it calls from such an instruction, and so on down. A loop is the instructions that run 8 or more
times per call; its code per pass is those plus what they call. Calls through pointers (jalr: an actor's update and
draw, callbacks) can't be followed, so each row says how many such calls its path makes; what they reach is listed
under its own name. A routine whose path is nearly all one listed callee's (a wrapper) is left out: --wrappers.

Columns: ms/frame is the routine's own time plus its callees' (shared out by call counts); "code fills" are the
instruction-cache line fills counted inside the path's routines (the direct evidence of trouble), per frame, and
as a share of the path's time at 48 cycles a fill. The fills are all of those routines' (a shared helper's fills
count in every path that uses it), so the share is a ceiling for wide paths and close for narrow ones.
"""
import argparse
import bisect
import collections
import os
import re
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import sysprof  # noqa: E402

CPU_HZ = sysprof.CPU_HZ


class Elf:
    """The executable sections of a big-endian ELF32: words by address."""

    def __init__(self, path):
        self.data = open(path, "rb").read()
        d = self.data
        shoff, = struct.unpack_from(">I", d, 0x20)
        shentsize, shnum = struct.unpack_from(">HH", d, 0x2E)
        self.secs = []
        for i in range(shnum):
            name, typ, flags, addr, off, size = struct.unpack_from(">6I", d, shoff + i * shentsize)
            if typ == 1 and (flags & 4) and size:
                self.secs.append((addr, addr + size, off))
        self.secs.sort()
        self.starts = [s[0] for s in self.secs]

    def word(self, addr):
        k = bisect.bisect_right(self.starts, addr) - 1
        if k < 0:
            return None
        lo, hi, off = self.secs[k]
        if addr >= hi:
            return None
        return struct.unpack_from(">I", self.data, off + addr - lo)[0]


def sized_functions(elf):
    out = subprocess.run(["mips-linux-gnu-nm", "-n", "-S", "--defined-only", elf], capture_output=True, text=True).stdout
    syms = []
    for line in out.splitlines():
        p = line.split()
        if len(p) == 4 and p[2] in "tT":
            syms.append((int(p[0], 16), int(p[1], 16), p[3]))
        elif len(p) == 3 and p[1] in "tT" and not p[2].startswith(("$", ".L", "L8")):
            syms.append((int(p[0], 16), 0, p[2]))
    syms.sort()
    res = []
    for i, (a, size, name) in enumerate(syms):
        if size == 0:
            nxt = syms[i + 1][0] if i + 1 < len(syms) else a + 4
            size = min(max(nxt - a, 4), 0x4000)
        res.append((a, size, name))
    return res


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("profiles", nargs="+")
    ap.add_argument("--elf", required=True)
    ap.add_argument("--build")
    ap.add_argument("--frames", type=int, required=True)
    ap.add_argument("--hot-kb", type=float, default=4.0)
    ap.add_argument("--item-kb", type=float, default=2.0)
    ap.add_argument("--hot-share", type=float, default=0.25, help="least share of working time (%%) for a 'hot' row")
    ap.add_argument("--item-calls", type=float, default=8.0, help="calls (or loop passes) a frame that make code per-item")
    ap.add_argument("--item-ms", type=float, default=0.01, help="least ms a frame for a 'per item' row")
    ap.add_argument("--match", default="", help="only rows whose routine, file or system matches (a search)")
    ap.add_argument("--all", action="store_true", help="with --match: every matching routine, over the lines or not")
    ap.add_argument("--wrappers", action="store_true",
                    help="also list routines whose path is nearly all (85%%) one listed callee's")
    ap.add_argument("--why", action="store_true", help="under each row, the routines its path is made of, by size")
    ap.add_argument("-o", "--output")
    a = ap.parse_args()

    total = {}
    for p in a.profiles:
        for pc, e in sysprof.read_cpuprof(p).items():
            t = total.setdefault(pc, [0, 0, 0, 0, 0])
            for k in range(5):
                t[k] += e[k]
    elf = Elf(a.elf)
    syms = sized_functions(a.elf)
    addrs = [s[0] for s in syms]
    by_addr = {s[0]: i for i, s in enumerate(syms)}

    # per function: its instructions' counts
    F = {}
    for pc, e in total.items():
        k = bisect.bisect_right(addrs, pc) - 1
        if k < 0 or pc >= syms[k][0] + syms[k][1]:
            continue
        f = F.get(k)
        if f is None:
            f = F[k] = {"k": k, "ins": {}, "w": [0, 0, 0, 0, 0]}
        f["ins"][pc] = e
        for j in range(5):
            f["w"][j] += e[j]

    for f in F.values():
        addr = syms[f["k"]][0]
        f["name"] = syms[f["k"]][2]
        f["size"] = syms[f["k"]][1]
        f["calls"] = f["ins"].get(addr, [0, 0])[1]
        f["sites"] = []     # (pc, count, callee k)
        f["indirect"] = []  # (pc, count)
        for pc, e in f["ins"].items():
            w = elf.word(pc)
            if w is None:
                continue
            if (w >> 26) == 3:
                tgt = ((pc + 4) & 0xF0000000) | ((w & 0x03FFFFFF) << 2)
                kk = by_addr.get(tgt)
                if kk is not None and kk in F and kk != f["k"]:
                    f["sites"].append((pc, e[1], kk))
            elif (w >> 26) == 0 and (w & 0x3F) == 9:
                f["indirect"].append((pc, e[1]))

    # files and systems (the functions' first instructions symbolized; the rest from the objects)
    build_dir = a.build or os.path.dirname(os.path.abspath(a.elf))
    if not os.path.isdir(os.path.join(build_dir, "src")):
        build_dir = os.path.join(HERE, "..", "..", "build", "ntsc-1.2")
    sources = sysprof.object_sources(build_dir) if os.path.isdir(os.path.join(build_dir, "src")) else {}
    frames = sysprof.n64prof.symbolize(a.elf, [syms[k][0] for k in F])
    for k, f in F.items():
        fr = frames.get(syms[k][0])
        path = (fr[-1].get("FileName", "") if fr else "") or sources.get(f["name"], "")
        f["file"] = sysprof.rel_file(path) if path else ""
        f["system"], f["sub"] = sysprof.classify(path, f["name"])

    # the working time: the game's own (idle and the test harness's code left out, here and in every path's time)
    for f in F.values():
        if f["system"] in ("(idle)", sysprof.HARNESS):
            f["w"] = [0, 0, 0, 0, 0]
            f["skip"] = True
    busy = max(sum(f["w"][0] for f in F.values()), 1)

    # inclusive time: own plus each callee's, shared out by the calls made from here
    incl = {}

    def inclusive(k, stack):
        if k in incl:
            return incl[k]
        if k in stack:
            return 0.0
        f = F[k]
        stack.add(k)
        t = float(f["w"][0])
        for pc, n, kk in f["sites"]:
            c = F[kk]["calls"]
            if c:
                t += inclusive(kk, stack) * min(n / c, 1.0)
        stack.discard(k)
        incl[k] = t
        return t

    sys.setrecursionlimit(10000)

    def path_of(k, pcs, memo_key=None):
        """The routines reached from the instructions `pcs` of routine k: {routine: bytes that run per call}."""
        out = {}
        seen = set()

        def body(kk):
            g = F[kk]
            c = max(g["calls"], 1)
            return {pc for pc, e in g["ins"].items() if e[1] >= 0.25 * c}

        def walk(kk, hot):
            g = F[kk]
            out[kk] = out.get(kk, 0) + 4 * len(hot)
            for pc, n, callee in g["sites"]:
                if pc in hot and callee not in seen:
                    seen.add(callee)
                    walk(callee, body(callee))

        seen.add(k)
        walk(k, pcs)
        return out

    rows = []
    for k, f in F.items():
        if f.get("skip"):
            continue
        c = f["calls"]
        if c == 0:
            continue
        t = inclusive(k, set())
        ms = t / 2 / CPU_HZ / a.frames * 1000
        share = 100.0 * t / busy
        hot = {pc for pc, e in f["ins"].items() if e[1] >= 0.25 * c}
        p = path_of(k, hot)
        kb = sum(p.values()) / 1024.0
        per_frame = c / a.frames
        kind = None
        if per_frame >= a.item_calls:
            if kb >= a.item_kb and ms >= a.item_ms:
                kind = "per item"
        elif kb >= a.hot_kb and share >= a.hot_share:
            kind = "hot"
        ind = sum(1 for pc, n in f["indirect"] if pc in hot)
        fills = sum(F[kk]["w"][4] for kk in p)
        rows.append({"kind": kind, "f": f, "what": "call", "per_frame": per_frame, "kb": kb, "path": p, "ms": ms,
                     "share": share, "ind": ind, "fills": fills})
        # a loop inside it
        loop = {pc for pc, e in f["ins"].items() if e[1] >= 8 * c}
        if len(loop) >= 4:
            counts = sorted(f["ins"][pc][1] for pc in loop)
            passes = counts[len(counts) // 2]
            lp = path_of(k, loop)
            lkb = sum(lp.values()) / 1024.0
            lt = sum(f["ins"][pc][0] for pc in loop)
            for pc, n, kk in f["sites"]:
                if pc in loop and F[kk]["calls"]:
                    lt += inclusive(kk, set()) * min(n / F[kk]["calls"], 1.0)
            lms = lt / 2 / CPU_HZ / a.frames * 1000
            lkind = "per item" if (passes / a.frames >= a.item_calls and lkb >= a.item_kb and lms >= a.item_ms) else None
            rows.append({"kind": lkind, "f": f, "what": "loop", "per_frame": passes / a.frames, "kb": lkb, "path": lp,
                         "ms": lms, "share": 100.0 * lt / busy, "ind": sum(1 for pc, n in f["indirect"] if pc in loop),
                         "fills": sum(F[kk]["w"][4] for kk in lp)})

    # a wrapper whose path is nearly all one listed callee's says nothing new: the callee's row stands for it
    if not a.wrappers:
        listed = {r["f"]["k"]: r for r in rows if r["kind"] and r["what"] == "call"}
        for r in rows:
            if r["kind"] != "hot":
                continue
            for kk in r["path"]:
                r2 = listed.get(kk)
                if r2 is not None and r2 is not r and r2["kb"] >= 0.85 * r["kb"]:
                    r["kind"] = None
                    r["wrapper"] = True
                    break
        if not a.match:
            rows = [r for r in rows if not r.get("wrapper")]

    def matches(r):
        f = r["f"]
        return re.search(a.match, f["name"] + " " + f["file"] + " " + f["system"], re.I) is not None

    if a.match:
        rows = [r for r in rows if matches(r) and (a.all or r["kind"])]
    else:
        rows = [r for r in rows if r["kind"]]
    rows.sort(key=lambda r: (-r["ms"]))

    out = []
    w = out.append
    w("# Hot code by size\n")
    w(f"- {a.frames} frames; working time {busy / 2 / CPU_HZ / a.frames * 1000:.2f} ms a frame on the emulator's clock")
    w(f"- lines (filters, not targets): hot over {a.hot_kb:g} KB a call with at least {a.hot_share:g}% of the working "
      f"time; per item (at least {a.item_calls:g} calls or loop passes a frame) over {a.item_kb:g} KB")
    w(f"- the instruction cache is 16 KB; code fills are its line fills inside the path's routines, per frame\n")
    for kind, title in (("per item", "Per item: over %g KB each time" % a.item_kb),
                        ("hot", "Hot: over %g KB a call" % a.hot_kb), (None, "Matching, under the lines")):
        sel = [r for r in rows if r["kind"] == kind]
        if not sel:
            continue
        w(f"## {title}\n")
        w("| routine | what | times/frame | code KB | routines | calls by pointer | ms/frame | share | code fills/frame | "
          "fills' share of its time | system | file |")
        w("|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---|---|")
        for r in sel:
            f = r["f"]
            w(f"| `{f['name']}` | {r['what']} | {r['per_frame']:,.1f} | {r['kb']:.1f} | {len(r['path'])} | {r['ind']} | "
              f"{r['ms']:.3f} | {r['share']:.2f}% | {r['fills'] / a.frames:,.1f} | "
              f"{min(100.0, 100.0 * r['fills'] * 48 / max(r['ms'] / 1000 * CPU_HZ * a.frames, 1)):.0f}% | {f['system']} | "
              f"{f['file']} |")
            if a.why:
                parts = sorted(r["path"].items(), key=lambda kv: -kv[1])[:12]
                w("| | " + ", ".join(f"`{F[kk]['name']}` {b / 1024:.2f}" for kk, b in parts)
                  + (f", +{len(r['path']) - 12} more" if len(r["path"]) > 12 else "") + " | | | | | | | | | | |")
        w("")
    text = "\n".join(out) + "\n"
    if a.output:
        open(a.output, "w").write(text)
        print(f"{a.output}: {len(rows)} rows")
    else:
        sys.stdout.write(text)


if __name__ == "__main__":
    main()
