#!/usr/bin/env python3
"""Where a frame's RDP time goes, triangle by triangle: last frame's display list walked out of a RAM dump as F3DEX3
runs it (segments, calls, the matrix stack, the vertex buffer, CULLDL), each triangle transformed, clipped to the near
plane and the screen, culled as the geometry mode says, and costed with the console's own calibration (rdpbench2:
releases/history/notes-rdpbench2.md): per render mode, a fixed cost a triangle, a cost per scanline and a cost per
pixel, fitted to the bench's quads of 2 to 128 pixels; texture loads and pipe syncs as measured. The cost goes to the
display list the triangle came from (by symbol: overlays mapped back to their link addresses), and to groups.

An estimate: the RDP's real cost depends on what's in the caches and how spans fall, and the RSP can starve it. Its use
is the split: which lists, kinds and modes take the time.

Usage: rdp_cost.py RAM.bin ELF [--top N] [--group REGEX ...] [--start ADDR]
  RAM.bin: 8 MB of RDRAM from 0x80000000, taken during a play frame's update (framedump.sh): last frame's list is
  the one the graphics task last ran (gfxCtx->task's data_ptr, found through the camera's PlayState pointer)
"""
import argparse
import bisect
import collections
import math
import os
import re
import struct
import subprocess
import sys

import numpy as np

W, H = 320, 240

# rdpbench2 (console, 12 November): pipeline ns per quad (two triangles, s scanlines, s*s pixels) by mode, s = 2/8/32/128
CAL = {
    "shade": [1735, 3796, 18805, 185572],
    "shade+z": [3062, 10094, 55264, 528872],
    "tex+z": [3520, 10624, 55404, 532736],
    "2cyc fog aa+z": [4632, 14464, 76561, 676668],
    "aa+z": [4636, 14375, 75589, 682233],
    "xlu+z": [3806, 11764, 61673, 562470],
}
# rectangles (rdpbench1/2): a fixed cost each, and per pixel by mode
RECT_NS = 2400.0
RECT_NS_PX = {"fill": 11.5, "copy": 6.0, "1cyc": 19.0, "1cyc rd": 38.2, "2cyc": 40.7}
LOAD_NS = 2500.0       # a texture load: fixed (rdpbench2 loads, rounded)
LOAD_NS_PER_KB = 7900.0
SYNC_NS = 870.0        # a pipe sync


def fit():
    """per mode: (a, b, c): a triangle costs a + b * scanlines + c * pixels (ns)"""
    out = {}
    for k, v in CAL.items():
        A = np.array([[2.0, 2.0 * s, s * s] for s in (2, 8, 32, 128)])
        x, *_ = np.linalg.lstsq(A, np.array(v, float), rcond=None)
        out[k] = tuple(float(t) for t in x)
    return out


def symbols(elf):
    out = subprocess.run(["mips-linux-gnu-nm", "-n", elf], capture_output=True, text=True).stdout
    addrs, names, var = [], [], {}
    for line in out.splitlines():
        p = line.split()
        if len(p) == 3:
            a = int(p[0], 16)
            var[p[2]] = a
            if p[1] in "dDrRtTbB":
                addrs.append(a)
                names.append(p[2])
    return addrs, names, var


class Frame:
    def __init__(self, ram, elf):
        self.d = ram
        self.addrs, self.names, self.var = symbols(elf)
        self.seg = [0] * 16
        self.via = {}
        self.lab = {}
        # the actors' overlays where they're loaded: RAM -> link address
        self.ovl = []
        t = self.var.get("gActorOverlayTable")
        if t:
            for i in range(0x300):
                p = (t & 0xFFFFFF) + 0x20 * i
                if p + 0x20 > len(ram):
                    break
                vs, ve, ld = struct.unpack_from(">III", ram, p + 8)
                if ld and vs and ve > vs and (ld >> 24) == 0x80:
                    self.ovl.append((ld & 0x1FFFFFFF, (ld & 0x1FFFFFFF) + (ve - vs), vs))

    def u32(self, p):
        return struct.unpack_from(">I", self.d, p)[0]

    def phys(self, a):
        if a >= 0x80000000:
            return a & 0x1FFFFFFF
        return (self.seg[(a >> 24) & 0xF] + (a & 0xFFFFFF)) & 0x1FFFFFFF

    def name(self, p):
        """the symbol of a physical address (overlays at their link address)"""
        a = 0x80000000 | p
        for lo, hi, vs in self.ovl:
            if lo <= p < hi:
                a = vs + (p - lo)
                break
        i = bisect.bisect_right(self.addrs, a) - 1
        if i < 0 or a - self.addrs[i] > 0x20000:
            return "%08X" % a
        return self.names[i]

    def label(self, chain):
        """the innermost list with a symbol; an unnamed one by the segment it was reached through (3: the room, 6:
        an object), else under its nearest named caller"""
        k = self.lab.get(chain[-1])
        if k is not None:
            return k
        n = self.name(chain[-1])
        if not re.match(r"[0-9A-F]{8}$", n):
            k = n
        elif self.via.get(chain[-1]) == 3:
            k = "room %s" % n
        elif self.via.get(chain[-1]):
            k = "seg%X %s" % (self.via[chain[-1]], n)
        else:
            k = None
            for c in reversed(chain[:-1]):
                m = self.name(c)
                if not re.match(r"[0-9A-F]{8}$", m) and m != "gGfxPools":
                    k = "(under %s) %s" % (m, n)
                    break
                if self.via.get(c) == 3:
                    k = "room %s" % m
                    break
            if k is None:
                k = n
        self.lab[chain[-1]] = k
        return k

    def last_list(self):
        play = self.u32(self.var["D_8015BD7C"] & 0xFFFFFF) & 0x1FFFFFFF
        gfx = self.u32(play) & 0x1FFFFFFF
        return self.u32(gfx + 0x78 + 0x10 + 0x30)  # gfxCtx->task.list.t.data_ptr


def mtx_at(fr, p):
    v = struct.unpack_from(">16h16H", fr.d, p)
    m = np.zeros((4, 4))
    for i in range(16):
        m[i // 4][i % 4] = ((v[i] << 16) | v[16 + i]) / 65536.0
    return m


def clip_poly(pts, inside, inter):
    out = []
    n = len(pts)
    for i in range(n):
        a, b = pts[i], pts[(i + 1) % n]
        ia, ib = inside(a), inside(b)
        if ia:
            out.append(a)
        if ia != ib:
            out.append(inter(a, b))
    return out


def tri_cost_geom(c0, c1, c2):
    """screen area and scanlines of a triangle given clip coords (x, y, z, w); None if nothing on screen"""
    poly = [c0, c1, c2]
    eps = 1e-4
    poly = clip_poly(poly, lambda v: v[3] > eps,
                     lambda a, b: a + (b - a) * ((eps - a[3]) / (b[3] - a[3])))
    if len(poly) < 3:
        return None
    sp = [(W / 2 + W / 2 * v[0] / v[3], H / 2 - H / 2 * v[1] / v[3]) for v in poly]
    # the signed area (screen y down), before clipping to the screen, for culling
    sa = 0.0
    for i in range(len(sp)):
        x0, y0 = sp[i]
        x1, y1 = sp[(i + 1) % len(sp)]
        sa += x0 * y1 - x1 * y0
    for axis, lim, keep in ((0, 0.0, 1), (0, W, -1), (1, 0.0, 1), (1, H, -1)):
        def inside(p, axis=axis, lim=lim, keep=keep):
            return (p[axis] - lim) * keep >= 0

        def inter(a, b, axis=axis, lim=lim):
            t = (lim - a[axis]) / (b[axis] - a[axis])
            return (a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t)
        sp = clip_poly(sp, inside, inter)
        if len(sp) < 3:
            return None
    area = 0.0
    for i in range(len(sp)):
        x0, y0 = sp[i]
        x1, y1 = sp[(i + 1) % len(sp)]
        area += x0 * y1 - x1 * y0
    area = abs(area) / 2
    ys = [p[1] for p in sp]
    lines = max(0, math.ceil(max(ys)) - math.floor(min(ys)))
    if area < 0.05:
        return None
    return area, lines, sa


def mode_of(om_h, om_l, tex_on):
    cyc = (om_h >> 20) & 3
    if cyc >= 2:
        return "shade"  # copy/fill: rectangles, not triangles
    zc = om_l & 0x30
    imrd = om_l & 0x40
    xlu = (om_l & 0x0C00) == 0x0800 or ((om_l & 0x4000) and imrd)
    if cyc == 1:
        return "2cyc fog aa+z" if zc else "shade"
    if xlu and imrd:
        return "xlu+z"
    if imrd:
        return "aa+z"
    if zc:
        return "tex+z" if tex_on else "shade+z"
    return "shade"


def walk(fr, start, fits, culldir):
    stack = []
    pc = fr.phys(start)
    cur = [pc]  # the call chain (list starts)
    mv = np.eye(4)
    mvstack = []
    vp = np.eye(4)
    vtx = [None] * 128
    geom = 0
    om_h = om_l = 0
    tex_on = False
    timg = (0, 0)
    per = collections.defaultdict(lambda: [0.0, 0, 0.0, 0, 0.0])  # ns, tris, pixels, loads, load ns
    by_mode = collections.Counter()
    unknown = collections.Counter()
    steps = 0
    while steps < 3000000:
        steps += 1
        w0, w1 = struct.unpack_from(">II", fr.d, pc)
        op = w0 >> 24
        nxt = pc + 8
        if op == 0xDE:
            if not ((w0 >> 16) & 1):
                stack.append((nxt, list(cur)))
            pc = fr.phys(w1)
            seg = (w1 >> 24) & 0xF if w1 < 0x80000000 else 0
            if seg and pc not in fr.via:
                fr.via[pc] = seg
            cur = cur + [pc] if not ((w0 >> 16) & 1) else cur[:-1] + [pc]
            continue
        if op == 0xDF:
            if not stack:
                break
            pc, cur = stack.pop()
            continue
        key = fr.label(cur)
        if op == 0x03:  # CULLDL: every vertex in the range off one side of the view: the list ends
            a, b = (w0 & 0xFFFF) // 2, (w1 & 0xFFFF) // 2
            vs = [vtx[i] for i in range(a, b + 1) if vtx[i] is not None]
            if vs:
                V = np.array(vs)
                out = False
                for ax in (0, 1, 2):
                    if np.all(V[:, ax] > V[:, 3]) or np.all(V[:, ax] < -V[:, 3]):
                        out = True
                if out:
                    if not stack:
                        break
                    pc, cur = stack.pop()
                    continue
        elif op in (0xDB, 0x0B) and ((w0 >> 16) & 0xFF) == 6:
            fr.seg[((w0 & 0xFFFF) // 4) & 0xF] = fr.phys(w1) if (w1 >> 24) & 0xF else (w1 & 0x1FFFFFFF)
        elif op == 0xDA:
            flags = (w0 & 0xFF) ^ 3  # (F3DEX3: push and load sent flipped)
            m = mtx_at(fr, fr.phys(w1))
            if flags & 4:
                vp = m if flags & 2 else m @ vp
            else:
                if flags & 1:
                    mvstack.append(mv)
                mv = m if flags & 2 else m @ mv
        elif op == 0xD8:
            for _ in range(max(1, w1 // 64)):
                if mvstack:
                    mv = mvstack.pop()
        elif op == 0x01:
            n = (w0 >> 12) & 0xFF
            end = (w0 >> 1) & 0x7F
            p = fr.phys(w1)
            M = mv @ vp
            for i in range(n):
                x, y, z = struct.unpack_from(">3h", fr.d, p + 16 * i)
                vtx[end - n + i] = np.array([x, y, z, 1.0]) @ M
        elif op == 0xD9:
            geom = (geom & (w0 & 0xFFFFFF | 0xFF000000)) | w1
        elif op == 0xD7:
            tex_on = bool((w0 >> 1) & 1) or bool(w0 & 0xFE)
        elif op in (0xE2, 0xE3):
            ln = (w0 & 0xFF) + 1
            sft = 32 - ((w0 >> 8) & 0xFF) - ln
            mask = ((1 << ln) - 1) << sft
            if op == 0xE2:
                om_l = (om_l & ~mask) | (w1 & mask)
            else:
                om_h = (om_h & ~mask) | (w1 & mask)
        elif op == 0xEF:
            om_h, om_l = w0 & 0xFFFFFF, w1
        elif op == 0xFD:
            timg = (w0, w1)
        elif op in (0xF3, 0xF4, 0xF0):
            siz = (timg[0] >> 19) & 3
            if op == 0xF3:
                b = ((((w1 >> 12) & 0xFFF) + 1) << siz) >> 1
            elif op == 0xF4:
                uls, ult = (w0 >> 12) & 0xFFF, w0 & 0xFFF
                lrs, lrt = (w1 >> 12) & 0xFFF, w1 & 0xFFF
                b = ((((lrs >> 2) - (uls >> 2) + 1) << siz) >> 1) * ((lrt >> 2) - (ult >> 2) + 1)
            else:
                b = ((((w1 >> 12) & 0xFFF) >> 2) + 1) * 2
            ns = LOAD_NS + LOAD_NS_PER_KB * b / 1024.0
            for k in (key,):
                per[k][3] += 1
                per[k][4] += ns
        elif op == 0xE7:
            per[key][4] += SYNC_NS
        elif op in (0x05, 0x06, 0x07):
            tris = [((w0 >> 16) & 0xFF, (w0 >> 8) & 0xFF, w0 & 0xFF)]
            if op != 0x05:
                tris.append(((w1 >> 16) & 0xFF, (w1 >> 8) & 0xFF, w1 & 0xFF))
            mode = mode_of(om_h, om_l, tex_on)
            a, b, c = fits[mode]
            for t in tris:
                vs = [vtx[i // 2] for i in t]
                if any(v is None for v in vs):
                    continue
                g = tri_cost_geom(*vs)
                if g is None:
                    continue
                area, lines, sa = g
                if (geom & 0x400) and sa * culldir < 0:
                    continue
                if (geom & 0x200) and sa * culldir > 0:
                    continue
                ns = a + b * lines + c * area
                e = per[key]
                e[0] += ns
                e[1] += 1
                e[2] += area
                by_mode[mode] += ns
        elif op in (0xE4, 0xE5, 0xF6):
            lrx, lry = ((w0 >> 12) & 0xFFF) / 4.0, (w0 & 0xFFF) / 4.0
            ulx, uly = ((w1 >> 12) & 0xFFF) / 4.0, (w1 & 0xFFF) / 4.0
            cyc = (om_h >> 20) & 3
            px = max(0.0, min(lrx, W) - max(ulx, 0)) * max(0.0, min(lry, H) - max(uly, 0))
            if op == 0xF6 or cyc == 3:
                rm = "fill"
            elif cyc == 2:
                rm = "copy"
            elif cyc == 1:
                rm = "2cyc"
            else:
                rm = "1cyc rd" if om_l & 0x40 else "1cyc"
            ns = RECT_NS + RECT_NS_PX[rm] * px
            e = per[key]
            e[0] += ns
            e[2] += px
            by_mode["rect " + rm] += ns
            if op != 0xF6:
                nxt += 16  # (its two RDPHALF words)
        elif op in (0x08, 0x04, 0x02):
            unknown[op] += 1
        pc = nxt
    return per, by_mode, unknown


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("ram")
    ap.add_argument("elf")
    ap.add_argument("--top", type=int, default=40)
    ap.add_argument("--start", type=lambda s: int(s, 16))
    ap.add_argument("--group", action="append", default=[], help="NAME=REGEX: also total the lists matching")
    ap.add_argument("--cull", type=int, default=-1, help="which signed area is the back (-1: as F3DEX3 culls)")
    ap.add_argument("--json", help="also write every list's numbers here")
    args = ap.parse_args()
    fr = Frame(open(args.ram, "rb").read(), args.elf)
    start = args.start or fr.last_list()
    fits = fit()
    per, by_mode, unknown = walk(fr, start, fits, args.cull)
    tot = sum(e[0] + e[4] for e in per.values())
    if args.json:
        import json
        json.dump({k: {"ns": e[0] + e[4], "tris": e[1], "px": e[2], "loads": e[3]} for k, e in per.items()},
                  open(args.json, "w"))
    print("list %08X: estimated RDP drawing %.2f ms (triangles %.2f, loads and syncs %.2f); %d triangles drawn, %.0f "
          "pixels (%.2f screens)" % (start, tot / 1e6, sum(e[0] for e in per.values()) / 1e6,
                                     sum(e[4] for e in per.values()) / 1e6, sum(e[1] for e in per.values()),
                                     sum(e[2] for e in per.values()), sum(e[2] for e in per.values()) / (W * H)))
    print("by mode: " + ", ".join("%s %.2f ms" % (k, v / 1e6) for k, v in by_mode.most_common()))
    if unknown:
        print("not walked: " + ", ".join("%02X x%d" % kv for kv in unknown.items()))
    for g in args.group:
        name, rx = g.split("=", 1)
        r = re.compile(rx)
        sel = [e for k, e in per.items() if r.search(k)]
        print("group %s: %.2f ms, %d triangles, %.0f pixels, %d loads" % (
            name, sum(e[0] + e[4] for e in sel) / 1e6, sum(e[1] for e in sel), sum(e[2] for e in sel),
            sum(e[3] for e in sel)))
    print("\n| list | ms | triangles | pixels | px/tri | loads |")
    print("|---|---|---|---|---|---|")
    for k, e in sorted(per.items(), key=lambda kv: -(kv[1][0] + kv[1][4]))[:args.top]:
        print("| %s | %.3f | %d | %.0f | %.0f | %d |" % (k, (e[0] + e[4]) / 1e6, e[1], e[2], e[2] / max(e[1], 1),
                                                       e[3]))


if __name__ == "__main__":
    main()
