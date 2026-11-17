#!/usr/bin/env python3
"""Where the CPU's time goes, by system: every part of the engine and the mod, each one's files, functions and lines.

  sysprof.py report PROFILE... --elf ELF [-o REPORT.md] [--json OUT.json] [--functions N] [--lines N]
  sysprof.py compare A.json B.json [--top N]        two reports' systems and functions side by side
  sysprof.py dump PROFILE... -o OUT.n64p            ares's exact profile as an n64prof dump (for `n64prof.py llvm`:
                                                    a compiler's sample profile from exact counts)

PROFILE is either
  - ares's exact profile (ARES_CPUPROF=<file>, fishmod/tools/ares-prof.patch): every instruction run, with its clocks,
    data cache fills, write-backs and code fills. With ARES_CPUPROF_OVL=<address of gFishmodOvlMap> the overlays' code
    (actors, Link, the pause menu) is counted where the ELF has it; without, it's reported as "overlay code, not placed";
  - a console profile: an n64prof dump or a folder of its streamed chunks (`sc64deployer debug`'s binaryout-*.bin):
    samples of the program counter every interrupt.
Several PROFILEs of the same kind are added together (the legs of a whole-game run).

The report: the run's totals; every system's share (idle apart, so the shares are of the time the CPU worked); for each
system its files, its costliest functions, and those functions' costliest source lines. --json keeps every function's
numbers, for `compare` after a change.

Systems are named by source file (SYSTEMS below): the first rule that matches wins.
"""
import argparse
import bisect
import collections
import json
import os
import re
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "n64prof", "tools"))
import n64prof  # noqa: E402

CPU_HZ = 93750000.0

# (regex on the source path, system, subsystem or None for the file's own name)
SYSTEMS = [
    (r"n64prof/|fishmod_debug", "Profiler and test harness (not in a release)", None),
    (r"src/audio/internal/synthesis", "Audio", "mixing (synthesis.c)"),
    (r"src/audio/internal/seqplayer", "Audio", "sequence player"),
    (r"src/audio/internal/playback", "Audio", "note playback"),
    (r"src/audio/internal/effects", "Audio", "note effects"),
    (r"src/audio/internal/load", "Audio", "loading (fonts, samples, sequences)"),
    (r"src/audio/internal/heap", "Audio", "heap and caches"),
    (r"src/audio/internal/", "Audio", "thread and tasks"),
    (r"src/audio/game/|audio_stop_all_sfx|z_sfx_source", "Audio", "game side (sound effects, music commands)"),
    (r"src/audio/|audio_thread_manager", "Audio", "manager"),
    (r"src/libultra/audio", "Audio", "libultra audio"),
    (r"fishmod_scatter", "Fishing mod", "props (scatter)"),
    (r"fishmod_manager", "Fishing mod", "fish manager"),
    (r"fishmod_rig", "Fishing mod", "fish rigs"),
    (r"ovl_En_Fishmod/", "Fishing mod", None),
    (r"fishmod|src/fishmod_data", "Fishing mod", None),
    (r"z_bgcheck|z_bg_collect|z_bg_item", "Collision", "floors, walls, ceilings (z_bgcheck.c)"),
    (r"z_collision_check|z_collision_btltbls", "Collision", "colliders (z_collision_check.c)"),
    (r"sys_math3d", "Collision", "geometry tests (sys_math3d.c)"),
    (r"ovl_player_actor|z_player_lib|z_player_call", "Link", None),
    (r"z_horse|ovl_En_Horse", "Actors", "horses"),
    (r"src/overlays/actors/ovl_(Boss_\w+|En_Fd|En_Goma|Door_Warp1)", "Actors", None),
    (r"src/overlays/actors/", "Actors", None),
    (r"z_en_item00|z_en_a_keep", "Actors", None),
    (r"z_actor_dlftbls|z_actor\.c|z_cheap_proc", "Actor framework (z_actor.c)", None),
    (r"z_skelanime|z_skin|z_fcurve", "Animation", None),
    (r"z_camera|db_camera|z_onepointdemo|z_view|z_quake|shrink_window|letterbox", "Camera and view", None),
    (r"z_demo|z_cutscene", "Cutscenes", None),
    (r"z_kankyo|z_vr_box|z_lights|z_env_flags", "Sky, weather and lights", None),
    (r"src/overlays/effects/|z_eff|z_effect", "Effects", None),
    (r"z_fbdemo|src/overlays/misc/ovl_fbdemo|z_fbdemo", "Screen transitions", None),
    (r"ovl_kaleido_scope|z_kaleido|z_jpeg|jpeg|PreRender|z_vismono|z_viscvg|z_viszbuffer", "Pause menu and screen effects", None),
    (r"z_message|z_kanfont|z_moji|z_elf_message|z_face_reaction", "Text", None),
    (r"z_parameter|z_construct|z_lifemeter|z_map_exp|z_map_mark|z_map_data|ovl_map_mark|speed_meter", "HUD and map", None),
    (r"src/overlays/gamestates/", "Title, file select, menus", None),
    (r"z_room|z_scene|object_table|z_path", "Scenes, rooms and objects", None),
    (r"z_rcp|z_draw|gfxprint|gfxalloc|sys_ucode|ucode_disas|gDPLoad|z_vimode|sys_cfb|TwoHeadGfxArena", "Display list setup", None),
    (r"sys_matrix|sys_math|z_lib|z_olib|z_skin_matrix|qrand|src/libultra/gu|src/libultra/mgu|libc/math|sinf|cosf|sqrt",
     "Maths (matrices, vectors, trig, random)", None),
    (r"z_std_dma|yaz0|src/boot/z_locale|z_DLF|loadfragment|relocation|src/libu64/load", "Loading (DMA, decompression, overlays)",
     None),
    (r"malloc|TwoHeadArena|gamealloc|listalloc|z_memory_utils|debug_arena|system_heap|__osMalloc", "Memory", None),
    (r"z_sram|z_ss_sram|z_common_data|z_inventory|mempak|flg_set", "Save data and inventory", None),
    (r"sched|graph|irqmgr|padmgr|main\.c|src/boot/|fault|sys_rumble|z_rumble|sys_freeze|z_prenmi|stackcheck|rcp_utils|"
     r"z_nulltask|logutils|sys_debug|z_debug|code_n64dd|src/n64dd|src/libu64|src/libc64", "Scheduler, threads and system", None),
    (r"game\.c|z_play|z_game_over|z_frame_advance|title_setup|z_game_dlftbls|z_sample", "Game loop (game.c, z_play.c)", None),
    (r"src/libultra/libc|src/libc/", "C library (memory copies, printf, 64-bit maths)", None),
    (r"src/libultra/", "libultra (OS: threads, interrupts, PI, SI, VI, timers)", None),
    (r"src/code/|src/", "Other engine code", None),
]
# functions with no line tables (assembly), by name
ASM_SYSTEMS = [
    (r"^(bcopy|bzero|bcmp|memcpy|memset|memmove|__ll|__ull|__d_to|__f_to|__u?divdi|__u?moddi|_Printf|_Litob|_Ldtob|str)",
     "C library (memory copies, printf, 64-bit maths)", "assembly"),
    (r"^(guMtx|guS|guN|guP|guO|guR|guT|guL|sinf|cosf|sins|coss|sqrtf|__sin|__cos|Math_)", "Maths (matrices, vectors, trig, random)",
     "assembly"),
    (r"^(__os|os|__rmon|__exception|send_mesg|handle_|__dispatch|_VirtualToPhysicalTask|__sc|__vi|__pi|__si|vi|pi|si)",
     "libultra (OS: threads, interrupts, PI, SI, VI, timers)", "assembly"),
]
IDLE_FUNCS = {"Idle_ThreadEntry"}
OVERLAY_LO = 0x80800000


def classify(path, func):
    if func in IDLE_FUNCS:
        return ("(idle)", "idle thread")
    if path:
        p = path.replace("\\", "/")
        i = p.find("/src/")
        j = p.find("/n64prof/")
        rel = p[i + 1:] if i >= 0 else (p[j + 1:] if j >= 0 else p)
        for rx, system, sub in SYSTEMS:
            if re.search(rx, rel):
                if sub is None:
                    m = re.search(r"src/overlays/\w+/(ovl_\w+)/", rel)
                    sub = m.group(1) if m else os.path.basename(rel)
                return (system, sub)
        return ("Other engine code", os.path.basename(rel))
    for rx, system, sub in ASM_SYSTEMS:
        if re.search(rx, func):
            return (system, sub)
    return ("Other (no source)", "assembly and unknown")


def nm_functions(elf):
    out = subprocess.run(["mips-linux-gnu-nm", "-n", "--defined-only", elf], capture_output=True, text=True).stdout
    syms = []
    for line in out.splitlines():
        p = line.split()
        if len(p) == 3 and p[1] in "tT" and not p[2].startswith(("$", ".L", "L8")):
            syms.append((int(p[0], 16) & 0xFFFFFFFF, p[2]))
    return syms


def object_sources(build):
    """function name -> source path, from the build's object files (for code compiled without line tables)."""
    objs = []
    for root, _, files in os.walk(os.path.join(build, "src")):
        objs += [os.path.join(root, f) for f in files if f.endswith(".o")]
    out = {}
    for i in range(0, len(objs), 400):
        res = subprocess.run(["mips-linux-gnu-nm", "-A", "--defined-only"] + objs[i:i + 400], capture_output=True,
                             text=True).stdout
        for line in res.splitlines():
            m = re.match(r"(.*?\.o):\S+ [tT] (\S+)$", line)
            if m:
                rel = os.path.relpath(m.group(1), build)
                out.setdefault(m.group(2), "/" + rel[:-2] + ".c")
    return out


def read_cpuprof(path):
    """pc -> [clocks, instructions, data fills, write-backs, code fills]"""
    d = open(path, "rb").read()
    out = {}
    for i in range(0, len(d) - 27, 28):
        pc, n, clocks, fills, wb, ifills = struct.unpack_from("<IIQIII", d, i)
        e = out.setdefault(pc, [0, 0, 0, 0, 0])
        e[0] += clocks
        e[1] += n
        e[2] += fills
        e[3] += wb
        e[4] += ifills
    return out


def is_n64prof(path):
    if os.path.isdir(path):
        return True
    with open(path, "rb") as f:
        return f.read(4) == struct.pack(">I", n64prof.MAGIC)


def load_profiles(paths):
    """-> (kind, {pc: [weight, instructions, dfills, wbacks, ifills]}, notes). weight is clocks (ares) or samples."""
    kinds = {is_n64prof(p) for p in paths}
    if len(kinds) != 1:
        sys.exit("the profiles are of two kinds (ares's and the console's): report them apart")
    if kinds.pop():
        dumps = n64prof.load(paths)
        heads, buckets = n64prof.merge(dumps)
        shift = heads[0]["bucket_shift"]
        pcs = n64prof.expand(buckets, shift)
        return "samples", {pc: [c, 0, 0, 0, 0] for pc, c in pcs.items()}, [n64prof.describe(heads)], heads
    total = {}
    for p in paths:
        for pc, e in read_cpuprof(p).items():
            t = total.setdefault(pc, [0, 0, 0, 0, 0])
            for k in range(5):
                t[k] += e[k]
    return "clocks", total, [f"{len(paths)} exact profile(s) from ares"], None


def fmt_time(weight, kind, heads):
    if kind == "clocks":
        return f"{weight / 2 / CPU_HZ:9.2f} s"
    return f"{weight:9d}"


def build(args):
    kind, pcs, notes, heads = load_profiles(args.profiles)
    syms = nm_functions(args.elf)
    addrs = [s[0] for s in syms]
    code_end = max((a for a, n in syms if a < OVERLAY_LO), default=0)
    frames = n64prof.symbolize(args.elf, pcs.keys())

    funcs = {}   # name -> record
    unplaced = [0, 0, 0, 0, 0]
    for pc, e in pcs.items():
        fr = frames.get(pc)
        k = bisect.bisect_right(addrs, pc) - 1
        name = None
        path = ""
        line = 0
        if fr:
            name = fr[-1]["FunctionName"]
            path = fr[-1].get("FileName", "") or ""
            line = fr[-1].get("Line", 0) or 0
        elif k >= 0 and pc - addrs[k] < 0x8000 and (pc < code_end + 0x2000 or pc >= OVERLAY_LO):
            name = syms[k][1]
        if name is None:
            for j in range(5):
                unplaced[j] += e[j]
            continue
        f = funcs.get(name)
        if f is None:
            f = funcs[name] = {"name": name, "file": path, "w": [0, 0, 0, 0, 0], "calls": 0,
                               "lines": collections.defaultdict(lambda: [0, 0, 0, 0, 0])}
        if not f["file"] and path:
            f["file"] = path
        for j in range(5):
            f["w"][j] += e[j]
            f["lines"][line][j] += e[j]
        if k >= 0 and pc == addrs[k] and syms[k][1] == name:
            f["calls"] += e[1]
    build_dir = args.build or os.path.dirname(os.path.abspath(args.elf))
    if not os.path.isdir(os.path.join(build_dir, "src")):
        build_dir = os.path.join(HERE, "..", "..", "build", "ntsc-1.2")
    sources = object_sources(build_dir) if os.path.isdir(os.path.join(build_dir, "src")) else {}
    for f in funcs.values():
        if not f["file"]:
            f["file"] = sources.get(f["name"], "")
        f["system"], f["sub"] = classify(f["file"], f["name"])
    if unplaced[0]:
        funcs["(not placed)"] = {"name": "(code at addresses the ELF doesn't have)", "file": "", "w": unplaced, "calls": 0,
                                 "lines": {}, "system": "Overlay code, not placed",
                                 "sub": "run with ARES_CPUPROF_OVL to place it"}
    return kind, funcs, notes, heads


def add(a, b):
    for j in range(5):
        a[j] += b[j]


def rel_file(path):
    p = path.replace("\\", "/")
    i = p.find("/src/")
    return p[i + 1:] if i >= 0 else (p[1:] if p.startswith("/src/") else os.path.basename(p))


def report(args):
    kind, funcs, notes, heads = build(args)
    out = []
    w = out.append
    total = [0, 0, 0, 0, 0]
    systems = collections.defaultdict(lambda: {"w": [0, 0, 0, 0, 0], "subs": collections.defaultdict(lambda: [0, 0, 0, 0, 0]),
                                               "funcs": []})
    for f in funcs.values():
        add(total, f["w"])
        s = systems[f["system"]]
        add(s["w"], f["w"])
        add(s["subs"][f["sub"]], f["w"])
        s["funcs"].append(f)
    idle = systems.get("(idle)", {"w": [0] * 5})["w"][0]
    busy = max(total[0] - idle, 1)
    exact = kind == "clocks"

    def share(x):
        return 100.0 * x / busy

    w(f"# CPU profile by system\n")
    for n in notes:
        w(f"- {n}")
    if exact:
        w(f"- {total[1]:,} instructions, {total[0] / 2 / CPU_HZ:.1f} s of CPU time on the emulator's clock "
          f"({total[0] / 2:,.0f} cycles); idle {100.0 * idle / max(total[0], 1):.1f}% of it")
        w(f"- data cache: {total[2]:,} line fills, {total[3]:,} write-backs; code cache: {total[4]:,} line fills "
          f"(a fill is about 40-48 cycles: {100.0 * (total[2] * 40 + total[3] * 40 + total[4] * 48) / max(total[0] / 2, 1):.1f}% "
          f"of all cycles)")
        if args.frames:
            w(f"- per frame ({args.frames} frames): {busy / 2 / args.frames:,.0f} cycles working "
              f"({busy / 2 / args.frames / CPU_HZ * 1000:.2f} ms)")
    else:
        iv = heads[0]["interval"] if heads else 0
        w(f"- {total[0]:,} samples; idle {100.0 * idle / max(total[0], 1):.1f}% of them")
    w(f"- shares below are of the time the CPU was working (idle left out)\n")

    w("## Systems\n")
    if exact:
        w("| system | share | CPU s | instructions | cycles/instr | data fills | code fills | cache share of its time |")
        w("|---|---:|---:|---:|---:|---:|---:|---:|")
    else:
        w("| system | share | samples |")
        w("|---|---:|---:|")
    order = sorted(systems.items(), key=lambda kv: -kv[1]["w"][0])
    for name, s in order:
        if name == "(idle)":
            continue
        x = s["w"]
        if exact:
            cyc = x[0] / 2
            w(f"| {name} | {share(x[0]):.2f}% | {cyc / CPU_HZ:.2f} | {x[1]:,} | {cyc / max(x[1], 1):.2f} | {x[2]:,} | {x[4]:,} | "
              f"{100.0 * (x[2] * 40 + x[3] * 40 + x[4] * 48) / max(cyc, 1):.0f}% |")
        else:
            w(f"| {name} | {share(x[0]):.2f}% | {x[0]:,} |")
    w("")

    for name, s in order:
        if name == "(idle)":
            continue
        w(f"## {name}: {share(s['w'][0]):.2f}%\n")
        subs = sorted(s["subs"].items(), key=lambda kv: -kv[1][0])
        if len(subs) > 1:
            w("| part | share | of the system |" + (" data fills | code fills |" if exact else ""))
            w("|---|---:|---:|" + ("---:|---:|" if exact else ""))
            for sub, x in subs[:args.parts]:
                w(f"| {sub} | {share(x[0]):.2f}% | {100.0 * x[0] / max(s['w'][0], 1):.1f}% |"
                  + (f" {x[2]:,} | {x[4]:,} |" if exact else ""))
            if len(subs) > args.parts:
                rest = sum(x[0] for _, x in subs[args.parts:])
                w(f"| ({len(subs) - args.parts} more) | {share(rest):.2f}% | {100.0 * rest / max(s['w'][0], 1):.1f}% |"
                  + (" | |" if exact else ""))
            w("")
        fs = sorted(s["funcs"], key=lambda f: -f["w"][0])
        if exact:
            w("| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |")
            w("|---|---:|---:|---:|---:|---:|---:|---:|---|")
        else:
            w("| function | share | of the system | samples | file |")
            w("|---|---:|---:|---:|---|")
        for f in fs[:args.functions]:
            x = f["w"]
            if exact:
                calls = f["calls"]
                w(f"| `{f['name']}` | {share(x[0]):.2f}% | {100.0 * x[0] / max(s['w'][0], 1):.1f}% | {calls:,} | "
                  f"{(x[0] / 2 / calls) if calls else 0:,.0f} | {(x[1] / calls) if calls else 0:,.0f} | {x[2]:,} | {x[4]:,} | "
                  f"{rel_file(f['file'])} |")
            else:
                w(f"| `{f['name']}` | {share(x[0]):.2f}% | {100.0 * x[0] / max(s['w'][0], 1):.1f}% | {x[0]:,} | "
                  f"{rel_file(f['file'])} |")
        if len(fs) > args.functions:
            rest = sum(f["w"][0] for f in fs[args.functions:])
            w(f"| ({len(fs) - args.functions} more functions) | {share(rest):.2f}% | "
              f"{100.0 * rest / max(s['w'][0], 1):.1f}% |" + (" | | | | | |" if exact else " | |"))
        w("")
        if args.lines:
            shown = 0
            for f in fs[:args.line_functions]:
                if share(f["w"][0]) < args.line_min or not f["lines"]:
                    continue
                lines = sorted(f["lines"].items(), key=lambda kv: -kv[1][0])[:args.lines]
                if not lines or (len(lines) == 1 and lines[0][0] == 0):
                    continue
                if not shown:
                    w("Costliest lines (the function's own code, callees not counted; share of the function):\n")
                shown += 1
                cells = []
                for line, x in lines:
                    extra = f", {x[2]:,} data fills" if exact and x[2] * 40 > x[0] / 2 * 0.15 else ""
                    cells.append(f"{line}: {100.0 * x[0] / max(f['w'][0], 1):.0f}%{extra}")
                w(f"- `{f['name']}` ({rel_file(f['file'])}): " + "; ".join(cells))
            if shown:
                w("")
    text = "\n".join(out) + "\n"
    if args.output:
        open(args.output, "w").write(text)
        print(f"{args.output}: {len(systems)} systems, {len(funcs)} functions")
    else:
        sys.stdout.write(text)
    if args.json:
        data = {"kind": kind, "total": total, "idle": idle, "frames": args.frames,
                "functions": {f["name"]: {"system": f["system"], "sub": f["sub"], "file": rel_file(f["file"]), "w": f["w"],
                                          "calls": f["calls"]} for f in funcs.values()}}
        json.dump(data, open(args.json, "w"))


def todump(args):
    """ares's exact profile as an n64prof dump (one bucket an instruction, its run count as the samples), so
    n64prof.py's tools read it: `n64prof.py llvm` makes a compiler's sample profile from exact counts."""
    total = {}
    for p in args.profiles:
        for pc, e in read_cpuprof(p).items():
            total[pc] = total.get(pc, 0) + e[1]
    top = max(total.values(), default=1)
    scale = max(1, (top >> 31) + 1)  # (counts are 32 bits in a dump)
    rows = [(pc, c // scale) for pc, c in sorted(total.items()) if c // scale]
    head = [n64prof.MAGIC, 1, min(sum(c for _, c in rows), 0xFFFFFFFF), 0, 0, len(rows), 0, 0, 2, 1, 0, 0, 0, 0, 0, 0]
    with open(args.output, "wb") as f:
        f.write(struct.pack(n64prof.HEADER, *head))
        for pc, c in rows:
            f.write(struct.pack(">II", pc, c))
    print(f"{args.output}: {len(rows)} instructions' counts" + (f" (divided by {scale})" if scale > 1 else ""))


def compare(args):
    a = json.load(open(args.a))
    b = json.load(open(args.b))
    if a["kind"] != b["kind"]:
        sys.exit("the two reports are of different kinds of profile")
    # per frame if both say how many frames they cover, else as shares of the working time
    per = bool(a.get("frames") and b.get("frames"))

    def norm(d):
        busy = max(d["total"][0] - d["idle"], 1)
        k = (1.0 / 2 / d["frames"]) if per else (100.0 / busy)
        return k

    ka, kb = norm(a), norm(b)
    unit = "cycles/frame" if per else "% of working time"
    sa, sb = collections.Counter(), collections.Counter()
    for n, f in a["functions"].items():
        sa[f["system"]] += f["w"][0] * ka
    for n, f in b["functions"].items():
        sb[f["system"]] += f["w"][0] * kb
    print(f"({unit}; A = {args.a}, B = {args.b})\n")
    print(f"{'system':58s} {'A':>12s} {'B':>12s} {'B-A':>12s}")
    for s in sorted(set(sa) | set(sb), key=lambda s: -max(sa[s], sb[s])):
        if s == "(idle)":
            continue
        print(f"{s[:58]:58s} {sa[s]:12.2f} {sb[s]:12.2f} {sb[s] - sa[s]:+12.2f}")
    rows = []
    for n in set(a["functions"]) | set(b["functions"]):
        x = a["functions"].get(n, {"w": [0]})["w"][0] * ka
        y = b["functions"].get(n, {"w": [0]})["w"][0] * kb
        rows.append((y - x, x, y, n))
    rows.sort(key=lambda r: -abs(r[0]))
    print(f"\n{'function':58s} {'A':>12s} {'B':>12s} {'B-A':>12s}")
    for d, x, y, n in rows[:args.top]:
        print(f"{n[:58]:58s} {x:12.2f} {y:12.2f} {d:+12.2f}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("report")
    r.add_argument("profiles", nargs="+")
    r.add_argument("--elf", required=True)
    r.add_argument("--build", help="the build folder (its src/**.o name the files of code without line tables)")
    r.add_argument("-o", "--output")
    r.add_argument("--json")
    r.add_argument("--frames", type=int, default=0, help="the frames the profile covers, for per-frame numbers")
    r.add_argument("--parts", type=int, default=25, help="files listed per system")
    r.add_argument("--functions", type=int, default=25, help="functions listed per system")
    r.add_argument("--lines", type=int, default=5, help="lines listed per function (0: none)")
    r.add_argument("--line-functions", type=int, default=8, help="functions per system whose lines are listed")
    r.add_argument("--line-min", type=float, default=0.2, help="only functions with at least this share (%%)")
    r.set_defaults(fn=report)
    d = sub.add_parser("dump")
    d.add_argument("profiles", nargs="+")
    d.add_argument("-o", "--output", required=True)
    d.set_defaults(fn=todump)
    c = sub.add_parser("compare")
    c.add_argument("a")
    c.add_argument("b")
    c.add_argument("--top", type=int, default=40)
    c.set_defaults(fn=compare)
    args = ap.parse_args()
    args.fn(args)


if __name__ == "__main__":
    main()
