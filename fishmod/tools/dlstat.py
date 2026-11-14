#!/usr/bin/env python3
"""What a frame's display list asks of the RSP and the RDP: every command it runs, walked out of a RAM dump the way
F3DEX3 walks it (segments resolved, called and branched lists followed), counted by kind; the RDP state commands that
change nothing (the same value again) or are overwritten before anything is drawn with them; the matrices, vertices
and triangles; and the lists the commands come from, by symbol where the list is the game's own.

Usage: fishmod/tools/dlstat.py RAM.bin ELF [--start ADDR] [--end ADDR] [--top N]
RAM.bin: RDRAM from 0x80000000 (gdb: dump binary memory RAM.bin 0x80000000 0x80800000, the game paused). ADDR: the
list to start from (default: the graphics pool whose list is complete: the one the game isn't building, gGfxPools).
"""
import argparse
import bisect
import collections
import struct
import subprocess
import sys

NAMES = {
    0x00: "NOOP", 0x01: "VTX", 0x02: "MODIFYVTX", 0x03: "CULLDL", 0x04: "BRANCH_Z", 0x05: "TRI1", 0x06: "TRI2",
    0x07: "QUAD", 0x08: "TRISTRIP", 0x09: "TRIFAN", 0x0A: "LIGHTTORDP", 0x0B: "SEGMENT", 0xD3: "SPECIAL_3", 0xD4: "SPECIAL_2",
    0xD5: "MEMSET", 0xD6: "DMA_IO", 0xD7: "TEXTURE", 0xD8: "POPMTX", 0xD9: "GEOMETRYMODE", 0xDA: "MTX",
    0xDB: "MOVEWORD", 0xDC: "MOVEMEM", 0xDD: "LOAD_UCODE", 0xDE: "DL", 0xDF: "ENDDL", 0xE0: "SPNOOP",
    0xE1: "RDPHALF_1", 0xE2: "SETOTHERMODE_L", 0xE3: "SETOTHERMODE_H", 0xE4: "TEXRECT", 0xE5: "TEXRECTFLIP",
    0xE6: "RDPLOADSYNC", 0xE7: "RDPPIPESYNC", 0xE8: "RDPTILESYNC", 0xE9: "RDPFULLSYNC", 0xEA: "SETKEYGB",
    0xEB: "SETKEYR", 0xEC: "SETCONVERT", 0xED: "SETSCISSOR", 0xEE: "SETPRIMDEPTH", 0xEF: "RDPSETOTHERMODE",
    0xF0: "LOADTLUT", 0xF1: "RDPHALF_2", 0xF2: "SETTILESIZE", 0xF3: "LOADBLOCK", 0xF4: "LOADTILE", 0xF5: "SETTILE",
    0xF6: "FILLRECT", 0xF7: "SETFILLCOLOR", 0xF8: "SETFOGCOLOR", 0xF9: "SETBLENDCOLOR", 0xFA: "SETPRIMCOLOR",
    0xFB: "SETENVCOLOR", 0xFC: "SETCOMBINE", 0xFD: "SETTIMG", 0xFE: "SETZIMG", 0xFF: "SETCIMG",
}
DRAWS = {0x05, 0x06, 0x07, 0x08, 0x09, 0xE4, 0xE5, 0xF6}
# RDP state that a draw uses: the key each command sets (so one setting another's key overwrites it)
STATE_KEYS = {0xFC: "combine", 0xFA: "prim", 0xFB: "env", 0xF8: "fog", 0xF9: "blend", 0xF7: "fill",
              0xEE: "primdepth", 0xED: "scissor", 0xD7: "texture"}


class Ram:
    def __init__(self, data):
        self.d = data
        self.seg = [0] * 16

    def phys(self, addr):
        if addr >= 0x80000000:
            return addr & 0x1FFFFFFF
        return (self.seg[(addr >> 24) & 0xF] + (addr & 0xFFFFFF)) & 0x1FFFFFFF

    def word2(self, p):
        if p + 8 > len(self.d):
            return None
        return struct.unpack_from(">II", self.d, p)


def symbols(elf):
    out = subprocess.run(["mips-linux-gnu-nm", "-n", elf], capture_output=True, text=True).stdout
    addrs, names = [], []
    for line in out.splitlines():
        p = line.split()
        if len(p) == 3:
            try:
                addrs.append(int(p[0], 16))
                names.append(p[2])
            except ValueError:
                pass
    return addrs, names


def sym_of(syms, addr):
    addrs, names = syms
    i = bisect.bisect_right(addrs, addr) - 1
    if i < 0:
        return "?"
    off = addr - addrs[i]
    return names[i] if off == 0 else "%s+0x%X" % (names[i], off) if off < 0x4000 else "%08X" % addr


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("ram")
    ap.add_argument("elf")
    ap.add_argument("--start", type=lambda s: int(s, 16))
    ap.add_argument("--top", type=int, default=25)
    ap.add_argument("--end", type=lambda s: int(s, 16), help="stop when the starting list reaches this address")
    args = ap.parse_args()
    ram = Ram(open(args.ram, "rb").read())
    syms = symbols(args.elf)
    start = args.start
    if start is None:
        sys.exit("--start: the list to walk (the pool's workBuffer)")

    count = collections.Counter()
    by_list = collections.Counter()  # commands run, by the list they're in (its start address)
    redundant = collections.Counter()  # the same value again
    dead = collections.Counter()  # overwritten before any draw
    dead_by_list = collections.Counter()
    red_by_list = collections.Counter()
    state = {}  # key -> (value, the list it came from, used since)
    othermode = {0xE2: {}, 0xE3: {}}  # the othermode words by bit range
    vtx = mtx = tris = 0
    vtx_by_list = collections.Counter()
    drawn_since_sync = True
    idle_syncs = collections.Counter()  # pipe syncs with nothing drawn since the last
    last_load = None  # (the image, the load command) of the last TMEM load
    repeat_loads = collections.Counter()  # the same image loaded the same way again, nothing else loaded between
    timg = None
    stack = []  # return addresses (phys) of called lists
    pc = ram.phys(start)
    cur_list = pc
    steps = 0
    end = ram.phys(args.end) if args.end else None
    if args.end:
        # a walk from the middle of a frame: the segments as the game's own table has them (gSegments)
        out = subprocess.run(["mips-linux-gnu-nm", args.elf], capture_output=True, text=True).stdout
        seg_at = [int(l.split()[0], 16) for l in out.splitlines() if l.endswith(" gSegments")]
        if seg_at:
            for i in range(16):
                ram.seg[i] = struct.unpack_from(">I", ram.d, (seg_at[0] & 0x1FFFFFFF) + 4 * i)[0] & 0x1FFFFFFF
    while steps < 2000000:
        steps += 1
        if end is not None and not stack and pc >= end:
            break
        w = ram.word2(pc)
        if w is None:
            print("ran off the dump at %08X" % pc)
            break
        w0, w1 = w
        op = w0 >> 24
        count[op] += 1
        by_list[cur_list] += 1
        nxt = pc + 8
        if op == 0xDE:  # DL: w0 bit 16: 1 = branch (no return)
            tgt = ram.phys(w1)
            if not ((w0 >> 16) & 1):
                stack.append((nxt, cur_list))
            pc = tgt
            cur_list = tgt
            continue
        if op == 0xDF:
            if not stack:
                break
            pc, cur_list = stack.pop()
            continue
        if op == 0x04:  # BRANCH_Z: the half-1 before it holds the target; taken or not, unknown: not followed
            pass
        if op in (0xDB, 0x0B):  # MOVEWORD (F3DEX3 also 0x0B, its segment command): segments (index 6)
            index = (w0 >> 16) & 0xFF
            offset = w0 & 0xFFFF
            if index == 0x06:
                ram.seg[(offset // 4) & 0xF] = ram.phys(w1) if (w1 >> 24) & 0xF else (w1 & 0x1FFFFFFF)
        if op == 0x01:
            vtx += (w0 >> 12) & 0xFF
            vtx_by_list[cur_list] += (w0 >> 12) & 0xFF
        elif op == 0xDA:
            mtx += 1
        elif op == 0x05:
            tris += 1
        elif op in (0x06, 0x07):
            tris += 2
        if op == 0xE7:
            if not drawn_since_sync:
                idle_syncs[cur_list] += 1
            drawn_since_sync = False
        if op == 0xFD:
            timg = (w0, w1)
        if op in (0xF3, 0xF4, 0xF0):
            sig = (timg, op, w0, w1)
            if sig == last_load:
                repeat_loads[cur_list] += 1
            last_load = sig
        if op in DRAWS:
            drawn_since_sync = True
            for k in state:
                v, src, used = state[k]
                state[k] = (v, src, True)
            for om in othermode.values():
                for k in om:
                    v, src, used = om[k]
                    om[k] = (v, src, True)
        elif op in STATE_KEYS:
            k = STATE_KEYS[op]
            old = state.get(k)
            if old is not None:
                if old[0] == (w0 & 0x00FFFFFF, w1):
                    redundant[op] += 1
                    red_by_list[cur_list] += 1
                elif not old[2]:
                    dead[op] += 1
                    dead_by_list[old[1]] += 1
            state[k] = ((w0 & 0x00FFFFFF, w1), cur_list, False)
        elif op in (0xE2, 0xE3):
            shift = 32 - ((w0 >> 8) & 0xFF) - 1 - (w0 & 0xFF)  # (F3DEX2 encoding: sft and len-1)
            length = (w0 & 0xFF) + 1
            key = (shift, length)
            om = othermode[op]
            old = om.get(key)
            if old is not None:
                if old[0] == w1:
                    redundant[op] += 1
                    red_by_list[cur_list] += 1
                elif not old[2]:
                    dead[op] += 1
                    dead_by_list[old[1]] += 1
            om[key] = (w1, cur_list, False)
        pc = nxt

    total = sum(count.values())
    print("%d commands run, %d vertices loaded, %d matrices, %d triangles sent" % (total, vtx, mtx, tris))
    print("\n| command | count | the same value again | overwritten before a draw |")
    print("|---|---|---|---|")
    for op, c in count.most_common():
        print("| %s | %d | %s | %s |" % (NAMES.get(op, "0x%02X" % op), c, redundant.get(op, ""), dead.get(op, "")))
    r = sum(redundant.values())
    d = sum(dead.values())
    print("\nRDP state commands that change nothing: %d; set and overwritten unused: %d (of %d commands)" % (r, d, total))
    print("Pipe syncs with nothing drawn since the last: %d; texture loads repeating the last load exactly: %d"
          % (sum(idle_syncs.values()), sum(repeat_loads.values())))
    print("\n## The lists that run the most commands\n")
    print("| list | commands | the same value again | its state overwritten unused |")
    print("|---|---|---|---|")
    for p, c in by_list.most_common(args.top):
        print("| %s | %d | %d | %d |" % (sym_of(syms, 0x80000000 | p), c, red_by_list.get(p, 0), dead_by_list.get(p, 0)))
    print("\n## Vertices loaded, by list\n")
    print("| list | vertices | commands |")
    print("|---|---|---|")
    for p, c in vtx_by_list.most_common(args.top):
        print("| %s | %d | %d |" % (sym_of(syms, 0x80000000 | p), c, by_list[p]))
    print("\n## Idle pipe syncs and repeated texture loads, by list\n")
    print("| list | idle pipe syncs | repeated loads | commands |")
    print("|---|---|---|---|")
    for p, c in (idle_syncs + repeat_loads).most_common(args.top):
        print("| %s | %d | %d | %d |" % (sym_of(syms, 0x80000000 | p), idle_syncs.get(p, 0), repeat_loads.get(p, 0),
                                       by_list[p]))
    print("\n## The lists with the most commands that change nothing\n")
    print("| list | the same value again | commands |")
    print("|---|---|---|")
    for p, c in red_by_list.most_common(args.top):
        print("| %s | %d | %d |" % (sym_of(syms, 0x80000000 | p), c, by_list[p]))


if __name__ == "__main__":
    main()
