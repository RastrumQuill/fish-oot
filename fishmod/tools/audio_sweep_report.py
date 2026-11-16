#!/usr/bin/env python3
"""Reads one part of the audio sweep as fishmod/tools/audio_sweep_run.sh saved it (fishmod/docs/audio-sweep.md) and
writes what the sweep's checks need:

  OUT/aud.bin      the sound log (one record an output buffer: audlog_compare.py compares two builds')
  OUT/report.json  per leg: frames, audio updates, and cycles, data cache fills, write-backs and code fills for the
                   audio driver and for the rest of the game (the idle loop apart); the threads' states at the end;
                   the instructions of the audio driver that ran, by function and offset (the coverage)
  and prints the same as a table.

    audio_sweep_report.py RUN_DIR --elf ELF            one run
    audio_sweep_report.py --compare REF_DIR NEW_DIR    two builds' reports of the same part, leg by leg
    audio_sweep_report.py --cover ELF DIR [DIR...]     what of the audio driver no run in DIRs reached (the DIRs'
                                                       builds differ only by the script's options; ELF is one of them)

The audio driver: every function whose source is under src/audio/ or is audio_thread_manager.c. Cycles are the
emulator's (clocks / 2), the same from run to run for the same ROM.
"""
import argparse, bisect, collections, glob, json, os, re, struct, subprocess, sys

AUDIO_SRC = re.compile(r"/src/audio/|/audio_thread_manager\.c")
IDLE = re.compile(r"^(Idle_ThreadEntry|__osIdle|Fishmod_Idle)")


def run(*args):
    return subprocess.run(args, capture_output=True, text=True).stdout


def functions(elf):
    """[(address, name, source file)] of the ELF's functions, in address order."""
    syms = []
    for line in run("mips-linux-gnu-nm", "-n", "--defined-only", elf).splitlines():
        p = line.split()
        if len(p) == 3 and p[1] in "tT" and not p[2].startswith(("$", ".L", "L8")):
            syms.append((int(p[0], 16) & 0xFFFFFFFF, p[2]))
    files = run("mips-linux-gnu-addr2line", "-e", elf, *["0x%x" % a for a, _ in syms]).splitlines()
    return [(a, n, f.rsplit(":", 1)[0]) for (a, n), f in zip(syms, files)]


def symbols(elf):
    d = {}
    for line in run("mips-linux-gnu-nm", elf).splitlines():
        p = line.split()
        if len(p) == 3:
            d[p[2]] = int(p[0], 16) & 0xFFFFFFFF
    return d


def read_prof(path):
    d = open(path, "rb").read()
    out = {}
    for i in range(0, len(d) - 27, 28):
        pc, n, clocks, fills, wb, ifills = struct.unpack_from("<IIQIII", d, i)
        out[pc] = (n, clocks, fills, wb, ifills)
    return out


def entrance_names(root):
    names = []
    try:
        for line in open(os.path.join(root, "include/tables/entrance_table.h")):
            m = re.match(r"\s*/\* 0x[0-9A-F]+ \*/ DEFINE_ENTRANCE\((\w+)", line)
            if m:
                names.append(m.group(1))
    except OSError:
        pass
    return names


def threads(ram, sym):
    def r32(a):
        return struct.unpack_from(">I", ram, a - 0x80000000)[0]

    out = []
    t = r32(sym["__osActiveQueue"])
    n = 0
    while 0x80000000 <= t < 0x80800000 and n < 40:
        state = struct.unpack_from(">H", ram, t + 0x10 - 0x80000000)[0]
        out.append({"id": r32(t + 0x14), "state": state, "pc": r32(t + 0x11C), "cause": (r32(t + 0x120) >> 2) & 31,
                    "flags": struct.unpack_from(">H", ram, t + 0x12 - 0x80000000)[0]})
        t = r32(t + 0xC)
        n += 1
    return out


def report(a):
    funcs = functions(a.elf)
    addrs = [f[0] for f in funcs]
    sym = symbols(a.elf)
    names = entrance_names(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))

    def klass(pc):
        k = bisect.bisect_right(addrs, pc) - 1
        if k < 0:
            return "rest", None
        _, name, src = funcs[k]
        if IDLE.match(name):
            return "idle", None
        if AUDIO_SRC.search(src):
            return "audio", (name, pc - addrs[k])
        return "rest", None

    cuts = sorted(glob.glob(os.path.join(a.run, "cpu.bin.seg*")))
    cuts = [c for c in cuts if re.search(r"\.seg\d+\.\d+$", c)]
    upd, frame = sym.get("AudioThread_UpdateImpl"), sym.get("Graph_Update")
    prev = {}
    legs = []
    cover = collections.defaultdict(set)
    for c in cuts:
        leg = int(c.rsplit(".", 1)[1])
        cur = read_prof(c)
        tot = {"audio": [0, 0, 0, 0, 0], "rest": [0, 0, 0, 0, 0], "idle": [0, 0, 0, 0, 0]}
        for pc, v in cur.items():
            p = prev.get(pc, (0, 0, 0, 0, 0))
            if v[0] == p[0]:
                continue
            k, where = klass(pc)
            t = tot[k]
            for j in range(5):
                t[j] += v[j] - p[j]
            if where:
                cover[where[0]].add(where[1])
        n = lambda pc: (cur.get(pc, (0,))[0] - prev.get(pc, (0,))[0]) if pc else 0
        if leg < 0xFFFF:
            legs.append({"leg": leg, "name": (names[leg - 1] if 0 < leg <= len(names) else "start"),
                         "frames": n(frame), "updates": n(upd),
                         "audio": {"cycles": tot["audio"][1] // 2, "fills": tot["audio"][2], "wbacks": tot["audio"][3],
                                   "ifills": tot["audio"][4], "instr": tot["audio"][0]},
                         "rest": {"cycles": tot["rest"][1] // 2, "fills": tot["rest"][2], "wbacks": tot["rest"][3],
                                  "ifills": tot["rest"][4], "instr": tot["rest"][0]}})
        prev = cur
    ended = any(c.endswith(".65535") for c in cuts)
    out = {"legs": legs, "ended": ended, "cover": {k: sorted(v) for k, v in cover.items()}}
    rampath = os.path.join(a.run, "ram.bin")
    if os.path.exists(rampath):
        ram = open(rampath, "rb").read()
        r32 = lambda x: struct.unpack_from(">I", ram, x - 0x80000000)[0]
        th = threads(ram, sym)
        for t in th:
            k = bisect.bisect_right(addrs, t["pc"]) - 1
            t["at"] = "%s+0x%x" % (funcs[k][1], t["pc"] - addrs[k]) if k >= 0 else "0x%08x" % t["pc"]
        out["threads"] = th
        # (a thread the fault handler took: stopped with the fault flag, osThread.flags & 2, or an exception's cause)
        out["stopped"] = [t for t in th if (t["flags"] & 2) or (t["state"] == 1 and t["cause"] not in (0, 8, 9))]
        p, cnt = r32(sym["gFishmodAudLog"]), r32(sym["gFishmodAudLogCount"])
        out["buffers"] = cnt
        out["buffers_max"] = r32(sym["gFishmodAudLogMax"])
        if 0x80000000 <= p < 0x80800000:
            open(os.path.join(a.run, "aud.bin"), "wb").write(ram[p - 0x80000000:p - 0x80000000 + cnt * 16])
    json.dump(out, open(os.path.join(a.run, "report.json"), "w"))
    print("%s: %d legs, end %s, %s buffers%s" % (a.run, len(legs), "reached" if ended else "NOT REACHED",
          out.get("buffers", "?"), " (LOG FULL)" if out.get("buffers", 0) >= out.get("buffers_max", 1) else ""))
    for t in out.get("stopped", []):
        print("  STOPPED: thread %d at %s, cause %d" % (t["id"], t["at"], t["cause"]))
    print("%-44s %6s %9s %7s %7s %9s %7s %7s" % ("leg (per game frame)", "frames", "aud cyc", "fills", "wbacks",
                                               "rest cyc", "fills", "wbacks"))
    for l in legs:
        f = max(l["frames"], 1)
        print("%-44s %6d %9.0f %7.1f %7.1f %9.0f %7.1f %7.1f" % (l["name"][:44], l["frames"], l["audio"]["cycles"] / f,
              l["audio"]["fills"] / f, l["audio"]["wbacks"] / f, l["rest"]["cycles"] / f, l["rest"]["fills"] / f,
              l["rest"]["wbacks"] / f))


def compare(a):
    ref = json.load(open(os.path.join(a.compare[0], "report.json")))
    new = json.load(open(os.path.join(a.compare[1], "report.json")))
    print("%-40s %6s %6s | %9s %9s %7s | %9s %9s %7s" % ("leg (cycles per game frame)", "frames", "new", "audio ref",
                                                        "new", "", "rest ref", "new", ""))
    tot = [0, 0, 0, 0, 0, 0]
    bad = 0
    for r, n in zip(ref["legs"], new["legs"]):
        fr, fn = max(r["frames"], 1), max(n["frames"], 1)
        ar, an = r["audio"]["cycles"] / fr, n["audio"]["cycles"] / fn
        rr, rn = r["rest"]["cycles"] / fr, n["rest"]["cycles"] / fn
        flag = ""
        if r["leg"] != n["leg"] or r["frames"] != n["frames"]:
            flag += " FRAMES-DIFFER"
        if rn > rr * 1.005:
            flag += " REST-SLOWER"
        if an > ar * 1.005:
            flag += " AUDIO-SLOWER"
        bad += bool(flag)
        for j, v in enumerate((r["audio"]["cycles"], n["audio"]["cycles"], r["rest"]["cycles"], n["rest"]["cycles"],
                               r["frames"], n["frames"])):
            tot[j] += v
        print("%-40s %6d %6d | %9.0f %9.0f %+6.1f%% | %9.0f %9.0f %+6.1f%%%s" % (
            r["name"][:40], r["frames"], n["frames"], ar, an, 100 * (an / ar - 1) if ar else 0, rr, rn,
            100 * (rn / rr - 1) if rr else 0, flag))
    if tot[4] and tot[5]:
        ar, an, rr, rn = tot[0] / tot[4], tot[1] / tot[5], tot[2] / tot[4], tot[3] / tot[5]
        print("%-40s %6d %6d | %9.0f %9.0f %+6.1f%% | %9.0f %9.0f %+6.1f%%" % ("all", tot[4], tot[5], ar, an,
              100 * (an / ar - 1), rr, rn, 100 * (rn / rr - 1)))
    for k in ("ref", "new"):
        d = ref if k == "ref" else new
        if not d["ended"] or d.get("stopped"):
            print("%s: %s %s" % (k, "" if d["ended"] else "END NOT REACHED", d.get("stopped") or ""))
            bad += 1
    print("legs flagged: %d" % bad)


def cover(a):
    elf, dirs = a.cover[0], a.cover[1:]
    funcs = functions(elf)
    seen = collections.defaultdict(set)
    for d in dirs:
        for k, v in json.load(open(os.path.join(d, "report.json")))["cover"].items():
            seen[k].update(v)
    lo = min(f[0] for f in funcs if AUDIO_SRC.search(f[2]))
    hi = max(f[0] for f in funcs if AUDIO_SRC.search(f[2])) + 0x4000
    dis = run("mips-linux-gnu-objdump", "-d", "--no-show-raw-insn", "--start-address=0x%x" % lo,
              "--stop-address=0x%x" % hi, elf)
    insns = {}
    for line in dis.splitlines():
        m = re.match(r"^([0-9a-f]{8}):\s+(\S+)", line)
        if m:
            insns[int(m.group(1), 16)] = m.group(2)
    addrs = [f[0] for f in funcs]
    total = hit = 0
    never, partial = [], []
    for i, (addr, name, src) in enumerate(funcs):
        if not AUDIO_SRC.search(src):
            continue
        end = funcs[i + 1][0] if i + 1 < len(funcs) else addr
        offs = [pc - addr for pc in range(addr, end, 4) if insns.get(pc, "nop") != "nop"]
        got = [o for o in offs if o in seen.get(name, ())]
        total += len(offs)
        hit += len(got)
        if offs and not got:
            never.append((len(offs), name, src))
        elif len(got) < len(offs):
            partial.append((len(offs) - len(got), len(offs), name, src, addr, [o for o in offs if o not in seen[name]]))
    print("audio driver: %d of %d instructions reached (%.1f%%)" % (hit, total, 100.0 * hit / max(total, 1)))
    print("\nfunctions never entered (%d):" % len(never))
    for n, name, src in sorted(never, reverse=True):
        print("  %5d  %-44s %s" % (n, name, os.path.relpath(src) if os.path.isabs(src) else src))
    print("\nfunctions with instructions not reached (%d): missing/all, then the source lines" % len(partial))
    for miss, n, name, src, addr, offs in sorted(partial, reverse=True):
        lines = run("mips-linux-gnu-addr2line", "-e", elf, *["0x%x" % (addr + o) for o in offs]).split()
        nums = sorted(set(int(l.rsplit(":", 1)[1].split()[0]) for l in lines if l.rsplit(":", 1)[1].split()[0].isdigit()))
        print("  %4d/%-5d %-40s %s: %s" % (miss, n, name, os.path.basename(src), " ".join(map(str, nums))[:400]))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("run", nargs="?")
    ap.add_argument("--elf")
    ap.add_argument("--compare", nargs=2)
    ap.add_argument("--cover", nargs="+")
    a = ap.parse_args()
    if a.compare:
        compare(a)
    elif a.cover:
        cover(a)
    else:
        report(a)


if __name__ == "__main__":
    main()
