#!/usr/bin/env python3
"""A search through the built code itself (no profile, nothing run) for loops and routines whose code is large for
the instruction cache. It lists candidates; which are real problems takes reading them (fishmod/docs/code-size.md
keeps the reading).

  codescan.py --elf ELF [--loop-kb 2] [--call-kb 4] [--match REGEX] [--top N] [--why] [--all] [-o OUT.md]

For every routine in the ELF, from its instructions:
  - its loops (a branch back to an earlier instruction), each one's own size, the routines it calls (jal) and how many
    calls it makes through pointers (jalr);
  - a loop's path: its own code plus the routines it calls and what those call (--depth levels further, 1 by
    default), each routine counted once at its full size; error and debug-print routines left out. Only innermost
    loops (no loop with calls inside them) unless --outer: the innermost is the per-item code;
  - cache conflicts: the instruction cache is 16 KB and direct mapped, so two pieces of code 16 KB apart (or a multiple)
    use the same lines and push each other out. For a loop whose path fits the cache by size, the bytes of it that
    still land on lines another part of the same path uses ("conflict"), counted between pieces whose distance is
    fixed at link time (both in the main code, or both in the same overlay);
  - whether a frame can reach it: from the game's per-frame entry points by direct calls, or an overlay's routine
    named for a per-frame job (Update, Draw, ...). The rest (set-up, save, debug) is left out unless --all.

Lists: loops over --loop-kb (2 KB; per-item code: the loop runs once per something) and routines over --call-kb
(4 KB) with no such loop inside, most conflict first, then biggest path. Size alone isn't a fault: a loop that runs twice a frame can be
any size. That's the judgement the reading adds, and what hotcode.py (the same question asked of a profile) measures.
"""
import argparse
import bisect
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import sysprof  # noqa: E402
from hotcode import Elf, sized_functions  # noqa: E402

COLD = re.compile(r"printf|Printf|Fault|fault|Assert|assert|LogUtils|osSyncPrintf|is_proutSyncPrintf|__osError|Debug|"
                  r"DebugArena|rmon|_Putfld|_Litob|_Ldtob|GfxPrint|Fishmod_Debug|n64prof|PcProf|InputRec|Sc64")
FRAME_ROOTS = ["Play_Main", "Play_Update", "Play_Draw", "Graph_Update", "Graph_ThreadEntry", "AudioThread_UpdateImpl",
               "AudioThread_Update", "Sched_ThreadEntry", "PadMgr_ThreadEntry", "Actor_UpdateAll", "Actor_DrawAll",
               "func_800315AC", "FileSelect_Main", "ConsoleLogo_Main", "KaleidoScope_Update", "KaleidoScope_Draw",
               "AudioMgr_ThreadEntry", "IrqMgr_ThreadEntry", "Main"]
FRAME_NAME = re.compile(r"(Update|Draw|Main|Action|Step|Move|Wait|Walk|Attack|Idle|Fly|Swim|Tick|Think|Process|"
                        r"Collision|Collider|Limb|func_8)", re.I)
CACHE = 0x4000


def decode(elf, addr, size):
    """-> (calls [(pc, target)], pointer calls [pc], loops [(head, tail)]) of the routine at addr."""
    calls, pcalls, loops = [], [], []
    for pc in range(addr, addr + size, 4):
        w = elf.word(pc)
        if w is None:
            break
        op = w >> 26
        if op == 3:
            calls.append((pc, ((pc + 4) & 0xF0000000) | ((w & 0x03FFFFFF) << 2)))
        elif op == 0 and (w & 0x3F) == 9:
            pcalls.append(pc)
        elif op in (4, 5, 6, 7, 20, 21, 22, 23) or (op == 1 and ((w >> 16) & 0x1F) in (0, 1, 2, 3, 16, 17, 18, 19)) or \
                (op == 17 and ((w >> 21) & 0x1F) == 8):
            off = w & 0xFFFF
            if off & 0x8000:
                off -= 0x10000
            tgt = pc + 4 + off * 4
            if addr <= tgt <= pc:
                loops.append((tgt, pc + 8))
        elif op == 2:  # j: a loop written as a jump back
            tgt = ((pc + 4) & 0xF0000000) | ((w & 0x03FFFFFF) << 2)
            if addr <= tgt <= pc:
                loops.append((tgt, pc + 8))
    return calls, pcalls, loops


def conflicts(pieces):
    """Bytes of the pieces [(lo, hi, group)] on cache lines another piece of the same group also uses."""
    total = 0
    groups = {}
    for lo, hi, g in pieces:
        groups.setdefault(g, []).append((lo, hi))
    for g, ps in groups.items():
        if len(ps) < 2:
            continue
        use = {}
        for n, (lo, hi) in enumerate(ps):
            for line in range(lo >> 5, (hi + 31) >> 5):
                use.setdefault(line % (CACHE >> 5), set()).add(n)
        total += 32 * sum(len(v) - 1 for v in use.values() if len(v) > 1)
    return total


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--elf", required=True)
    ap.add_argument("--build")
    ap.add_argument("--loop-kb", type=float, default=2.0)
    ap.add_argument("--call-kb", type=float, default=4.0)
    ap.add_argument("--depth", type=int, default=1, help="calls followed below a loop's own calls (0: its callees only)")
    ap.add_argument("--outer", action="store_true", help="also loops that have another loop with calls inside")
    ap.add_argument("--match", default="")
    ap.add_argument("--top", type=int, default=150)
    ap.add_argument("--all", action="store_true", help="also code no frame reaches by a direct call")
    ap.add_argument("--why", action="store_true")
    ap.add_argument("-o", "--output")
    a = ap.parse_args()

    elf = Elf(a.elf)
    syms = sized_functions(a.elf)
    by_addr = {s[0]: i for i, s in enumerate(syms)}
    F = []
    for i, (addr, size, name) in enumerate(syms):
        calls, pcalls, loops = decode(elf, addr, size)
        F.append({"i": i, "addr": addr, "size": size, "name": name, "pcalls": pcalls, "loops": loops,
                  "calls": [(pc, by_addr[t]) for pc, t in calls if t in by_addr and by_addr[t] != i],
                  "cold": bool(COLD.search(name))})

    def group(f):
        return "code" if f["addr"] < 0x80800000 else f["sub"]

    # files and systems
    build_dir = a.build or os.path.dirname(os.path.abspath(a.elf))
    if not os.path.isdir(os.path.join(build_dir, "src")):
        build_dir = os.path.join(HERE, "..", "..", "build", "ntsc-1.2")
    sources = sysprof.object_sources(build_dir) if os.path.isdir(os.path.join(build_dir, "src")) else {}
    for f in F:
        path = sources.get(f["name"], "")
        f["file"] = sysprof.rel_file(path) if path else ""
        f["system"], f["sub"] = sysprof.classify(path, f["name"])
        if not path and f["addr"] >= 0x80800000:
            f["sub"] = "overlay@%x" % (f["addr"] >> 16)

    # what a frame reaches
    reach = set()
    work = [by_addr_name for by_addr_name in (next((f["i"] for f in F if f["name"] == r), None) for r in FRAME_ROOTS)
            if by_addr_name is not None]
    work += [f["i"] for f in F if f["addr"] >= 0x80800000 and FRAME_NAME.search(f["name"]) and
             not re.search(r"Init|Destroy|Setup$", f["name"])]
    while work:
        i = work.pop()
        if i in reach or F[i]["cold"]:
            continue
        reach.add(i)
        work += [k for _, k in F[i]["calls"]]

    closure_memo = {}

    def closure(i):
        """The routines reachable from routine i within --depth direct calls (itself included), error paths left
        out. (Followed without limit, nearly every loop reaches most of the engine: a call that's possible isn't a
        call that's made each pass.)"""
        if i in closure_memo:
            return closure_memo[i]
        seen = {}
        stack = [(i, 0)]
        while stack:
            k, d = stack.pop()
            if F[k]["cold"] or seen.get(k, 99) <= d:
                continue
            seen[k] = d
            if d < a.depth:
                stack += [(c, d + 1) for _, c in F[k]["calls"]]
        closure_memo[i] = set(seen)
        return closure_memo[i]

    rows = []
    for f in F:
        if f["cold"] or (not a.all and f["i"] not in reach):
            continue
        # loops: merged by head (several branches back to one head are one loop), outermost dropped when an inner one
        # with calls exists is NOT done: each loop is its own row, the reader sees the nesting by the sizes
        heads = {}
        for lo, hi in f["loops"]:
            heads[lo] = max(heads.get(lo, 0), hi)
        best = None
        for lo, hi in heads.items():
            inside = [(pc, k) for pc, k in f["calls"] if lo <= pc < hi]
            reached = set()
            for _, k in inside:
                reached |= closure(k)
            reached.discard(f["i"])
            path = (hi - lo) + sum(F[k]["size"] for k in reached)
            if path < a.loop_kb * 1024:
                continue
            pieces = [(lo, hi, group(f))] + [(F[k]["addr"], F[k]["addr"] + F[k]["size"], group(F[k])) for k in reached]
            row = {"f": f, "what": "loop", "at": lo, "own": hi - lo, "path": path, "n": len(reached) + 1,
                   "direct": len(inside), "ptr": sum(1 for pc in f["pcalls"] if lo <= pc < hi),
                   "conflict": conflicts(pieces) if path <= CACHE else None, "members": reached,
                   "nested": sum(1 for l2, h2 in heads.items() if lo < l2 and h2 <= hi)}
            row["inner"] = not any(lo <= l2 and h2 <= hi and (l2, h2) != (lo, hi) and
                                   any(l2 <= pc < h2 for pc, _ in f["calls"]) for l2, h2 in heads.items())
            best = row
            if row["inner"] or a.outer:
                rows.append(row)
        if best is None:
            cl = closure(f["i"])
            path = sum(F[k]["size"] for k in cl)
            if path >= a.call_kb * 1024:
                pieces = [(F[k]["addr"], F[k]["addr"] + F[k]["size"], group(F[k])) for k in cl]
                rows.append({"f": f, "what": "call", "at": f["addr"], "own": f["size"], "path": path, "n": len(cl),
                             "direct": len(f["calls"]), "ptr": len(f["pcalls"]),
                             "conflict": conflicts(pieces) if path <= CACHE else None, "members": cl - {f["i"]},
                             "nested": 0})
    if a.match:
        rows = [r for r in rows if re.search(a.match, r["f"]["name"] + " " + r["f"]["file"] + " " + r["f"]["system"], re.I)]
    rows.sort(key=lambda r: (-(r["conflict"] or 0), -r["path"]))
    shown = rows[:a.top]
    lines = sysprof.n64prof.symbolize(a.elf, [r["at"] for r in shown])

    out = []
    w = out.append
    w("# Code size scan (static)\n")
    w(f"- {len(F)} routines; {len(reach)} a frame can reach by direct calls; {len(rows)} rows over the lines "
      f"(loops over {a.loop_kb:g} KB, routines over {a.call_kb:g} KB), the {len(shown)} biggest shown")
    w("- path: the loop's (or routine's) own code plus the routines it calls, to the depth given, each once, whole")
    w("- conflict: for a path that fits the 16 KB cache, its bytes on lines another part of it also uses\n")
    for what, title in (("loop", "Loops"), ("call", "Routines without such a loop")):
        sel = [r for r in shown if r["what"] == what]
        if not sel:
            continue
        w(f"## {title}\n")
        w("| routine | line | own bytes | path KB | routines | calls in it | by pointer | loops inside | conflict | system | file |")
        w("|---|---:|---:|---:|---:|---:|---:|---:|---:|---|---|")
        for r in sel:
            f = r["f"]
            fr = lines.get(r["at"])
            line = fr[0].get("Line", 0) if fr else 0
            w(f"| `{f['name']}` | {line or ''} | {r['own']:,} | {r['path'] / 1024:.1f} | {r['n']} | {r['direct']} | {r['ptr']} | "
              f"{r['nested']} | {'' if r['conflict'] is None else format(r['conflict'], ',')} | {f['system']} | {f['file']} |")
            if a.why:
                parts = sorted(r["members"], key=lambda k: -F[k]["size"])[:12]
                w("| | " + ", ".join(f"`{F[k]['name']}` {F[k]['size'] / 1024:.2f}" for k in parts)
                  + (f", +{len(r['members']) - 12} more" if len(r["members"]) > 12 else "") + " | | | | | | | | | |")
        w("")
    text = "\n".join(out) + "\n"
    if a.output:
        open(a.output, "w").write(text)
        print(f"{a.output}: {len(rows)} rows")
    else:
        sys.stdout.write(text)


if __name__ == "__main__":
    main()
