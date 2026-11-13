#!/usr/bin/env python3
"""Where the CPU's time lands, by the kind of instruction it was on (a console profile streamed over USB: one bucket per
instruction), to tell waiting on memory from computing.

A sample is the instruction the CPU was at when the timer interrupted it. Time spent waiting shows up as samples
piling on the instruction that waits:
- a data-cache miss: on the load or store, or the instruction right after a load (the load's result being waited for);
- an instruction-cache miss: on the first instruction of a 32-byte cache line (1 in 8 instructions: a share well over
  12.5% there means fetches from RDRAM);
- the slow arithmetic: division and square roots (29-37 cycles), integer multiply/divide results (mflo/mfhi).
The calibration loops (fishmod_debug_pcprof.inc.c, FISHMOD_PCPROF_CALIB: PcCalib_*) show what each looks like on the
console, with a known answer: read those first.

Usage: fishmod/tools/inst_profile.py FOLDER --elf ELF [--top N] [--func NAME]
"""
import argparse
import collections
import os
import re
import subprocess
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "n64prof", "tools"))
import n64prof  # noqa: E402

LOADS = {"lb", "lbu", "lh", "lhu", "lw", "lwl", "lwr", "lwu", "ld", "ldl", "ldr", "lwc1", "ldc1", "ll", "lld"}
STORES = {"sb", "sh", "sw", "swl", "swr", "sd", "sdl", "sdr", "swc1", "sdc1", "sc", "scd"}
SLOW_FP = {"div.s", "div.d", "sqrt.s", "sqrt.d"}
MULDIV = {"mult", "multu", "div", "divu", "dmult", "dmultu", "ddiv", "ddivu", "mflo", "mfhi"}
BRANCH_PREFIX = ("b", "j")
CATS = ["load", "store", "after a load", "slow FP (div, sqrt)", "int mul/div", "FP other", "branch/jump",
        "cache/sync/cop0", "other"]


def disassemble(elf):
    """address -> mnemonic, for every instruction in the ELF's code (the overlays at their link addresses, as the
    profile has them)."""
    exe = None
    for c in ("mips-linux-gnu-objdump", "mips64-elf-objdump", "llvm-objdump"):
        if subprocess.run(["which", c], capture_output=True).returncode == 0:
            exe = c
            break
    if exe is None:
        sys.exit("no MIPS objdump (mips-linux-gnu-objdump or llvm-objdump)")
    out = subprocess.run([exe, "-d", "--no-show-raw-insn", elf], capture_output=True, text=True).stdout
    insns = {}
    pat = re.compile(r"^\s*([0-9a-f]{8}):\s+([a-z0-9.]+)\s*(.*)$")
    for line in out.splitlines():
        m = pat.match(line)
        if m:
            insns[int(m.group(1), 16)] = m.group(2)
            TEXT[int(m.group(1), 16)] = (m.group(2) + " " + m.group(3)).strip()
    return insns


TEXT = {}  # address -> the instruction with its operands (disassemble)


def category(insns, pc):
    op = insns.get(pc)
    if op is None:
        return None
    prev = insns.get(pc - 4)
    if op in LOADS:
        return "load"
    if op in STORES:
        return "store"
    if prev in LOADS:
        return "after a load"
    if op in SLOW_FP:
        return "slow FP (div, sqrt)"
    if op in MULDIV:
        return "int mul/div"
    if "." in op and op.split(".")[-1] in ("s", "d", "w", "l"):
        return "FP other"
    if op.startswith(BRANCH_PREFIX) and op not in ("break",):
        return "branch/jump"
    if op in ("cache", "sync", "mtc0", "mfc0", "eret", "tlbwi", "tlbp", "tlbr"):
        return "cache/sync/cop0"
    return "other"


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("folder")
    ap.add_argument("--elf", required=True)
    ap.add_argument("--top", type=int, default=20)
    ap.add_argument("--func", help="one function's instructions, hottest first")
    ap.add_argument("--stalls", type=int, metavar="N",
                    help="the N loads with the most time stalled after them (a data-cache miss: the console's "
                         "calibration puts its wait on the next instruction), each with its source line")
    args = ap.parse_args()
    for f in os.listdir(args.folder):
        if f == "packets.zip" or (f.startswith("packets-") and f.endswith(".zip")):
            zipfile.ZipFile(os.path.join(args.folder, f)).extractall(args.folder)
    dumps = n64prof.load([args.folder])
    heads, buckets = n64prof.merge(dumps)
    shift = heads[-1]["bucket_shift"]
    if shift != 2:
        print(f"warning: buckets of {1 << shift} bytes, not one per instruction: the kinds below are approximate")
    pcs = n64prof.expand(buckets, shift)
    insns = disassemble(args.elf)
    frames = n64prof.symbolize(args.elf, pcs.keys())
    total = sum(pcs.values())

    per_func = collections.defaultdict(collections.Counter)
    line_start = collections.Counter()
    func_total = collections.Counter()
    overall = collections.Counter()
    unknown = 0
    for pc, c in pcs.items():
        cat = category(insns, pc)
        fr = frames.get(pc)
        name = fr[-1]["FunctionName"] if fr else "(no symbol)"
        if cat is None:
            unknown += c
            continue
        overall[cat] += c
        per_func[name][cat] += c
        func_total[name] += c
        if (pc & 0x1F) == 0:
            line_start[name] += c

    known = total - unknown
    print(f"{total} samples, {unknown} outside the disassembly ({100 * unknown / max(total, 1):.1f}%)\n")
    print("## All the code\n")
    print("| kind of instruction | share of the time |")
    print("|---|---|")
    for cat in CATS:
        print(f"| {cat} | {100 * overall[cat] / max(known, 1):.1f}% |")
    print(f"\nOn the first instruction of a 32-byte cache line: "
          f"{100 * sum(line_start.values()) / max(known, 1):.1f}% (12.5% if fetching cost nothing)\n")

    if args.stalls:
        # the time a load's next instruction holds beyond the load's own (a hit gives them about the same)
        rows = []
        for pc, c in pcs.items():
            if insns.get(pc - 4) in LOADS:
                excess = c - pcs.get(pc - 4, 0)
                if excess > 0:
                    rows.append((excess, pc - 4))
        rows.sort(reverse=True)
        stalled = sum(e for e, _ in rows)
        print(f"## Stalled loads: {100 * stalled / max(known, 1):.1f}% of the time in all\n")
        print("| share | function | load | source |")
        print("|---|---|---|---|")
        for e, pc in rows[:args.stalls]:
            fr = frames.get(pc + 4) or frames.get(pc)
            name = fr[-1]["FunctionName"] if fr else "?"
            src = f"{os.path.basename(fr[0]['FileName'])}:{fr[0]['Line']}" if fr else "?"
            print(f"| {100 * e / max(known, 1):.2f}% | {name} | `{TEXT.get(pc, '?')}` | {src} |")
        return

    if args.func:
        print(f"## {args.func}, hottest instructions\n")
        rows = sorted(((c, pc) for pc, c in pcs.items()
                       if frames.get(pc) and frames[pc][-1]["FunctionName"] == args.func), reverse=True)
        tot = sum(c for c, _ in rows)
        for c, pc in rows[:args.top]:
            print(f"  {pc:08x} {insns.get(pc, '?'):8s} {100 * c / max(tot, 1):5.1f}%  {category(insns, pc)}"
                  f"{'  (line start)' if (pc & 0x1F) == 0 else ''}")
        return

    print(f"## The {args.top} costliest functions\n")
    print("| function | share | load | store | after a load | slow FP | int mul/div | line starts |")
    print("|---|---|---|---|---|---|---|---|")
    for name, t in func_total.most_common(args.top):
        c = per_func[name]
        pct = lambda k: f"{100 * c[k] / t:.0f}%"
        print(f"| {name} | {100 * t / max(known, 1):.1f}% | {pct('load')} | {pct('store')} | {pct('after a load')} | "
              f"{pct('slow FP (div, sqrt)')} | {pct('int mul/div')} | {100 * line_start[name] / t:.0f}% |")


if __name__ == "__main__":
    main()
