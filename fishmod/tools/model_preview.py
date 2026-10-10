#!/usr/bin/env python3
"""
Offline preview of an extracted skeleton model (bind pose, no animation).

Parses the limb list, display lists and vertices from extracted object assets and
draws a side and top view with each limb in its own color, labeled with its index.
Used to identify fish limbs (head, jaw, fins, tail) for per-limb tint and scale.

Usage:
  fishmod/tools/model_preview.py <object dir> <LimbsArrayPrefix> <out.png>
e.g.
  fishmod/tools/model_preview.py extracted/ntsc-1.2/assets/objects/object_fish gFishingFish out.png
"""

import glob
import os
import re
import sys

from PIL import Image, ImageDraw

NUM = r"(-?\d+)"


def read(p):
    with open(p) as f:
        return f.read()


def parse_vtx_files(objdir):
    vtx = {}
    for p in glob.glob(os.path.join(objdir, "*Vtx*.inc.c")):
        name = os.path.basename(p).replace(".inc.c", "")
        pts = []
        for m in re.finditer(r"VTX\(\s*" + NUM + r",\s*" + NUM + r",\s*" + NUM, read(p)):
            pts.append(tuple(int(v) for v in m.groups()))
        if not pts:
            for m in re.finditer(r"\{\s*\{\s*" + NUM + r",\s*" + NUM + r",\s*" + NUM + r"\s*\}", read(p)):
                pts.append(tuple(int(v) for v in m.groups()))
        vtx[name] = pts
    return vtx


def parse_object_c(objdir):
    """Map symbol name -> included file for Vtx arrays declared in the object .c file."""
    c = [p for p in glob.glob(os.path.join(objdir, "*.c")) if not p.endswith(".inc.c")]
    syms = {}
    for p in c:
        for m in re.finditer(r"Vtx\s+(\w+)\[\]\s*=\s*\{\s*#include\s+\"([^\"]+)\"", read(p)):
            syms[m.group(1)] = os.path.basename(m.group(2)).replace(".inc.c", "")
    return syms


def parse_dl(path, vtx, vtxsyms):
    """Return a list of triangles (3 vertex tuples) for a display list file."""
    buf = [None] * 64
    tris = []
    text = read(path)
    for line in re.finditer(r"gsSP(Vertex|1Triangle|2Triangles)\(([^;]*?)\)\s*,\s*$", text, re.M):
        kind, args = line.groups()
        if kind == "Vertex":
            m = re.match(r"&(\w+)\[(\d+)\],\s*(\d+),\s*(\d+)", args.strip())
            sym, off, n, v0 = m.group(1), int(m.group(2)), int(m.group(3)), int(m.group(4))
            arr = vtx.get(vtxsyms.get(sym, sym)) or vtx.get(sym)
            if arr is None:
                continue
            for i in range(n):
                buf[v0 + i] = arr[off + i]
        else:
            idx = [int(v) for v in re.findall(r"\d+", args)]
            if kind == "1Triangle":
                groups = [idx[0:3]]
            else:
                groups = [idx[0:3], idx[4:7]]
            for g in groups:
                if all(buf[i] is not None for i in g):
                    tris.append(tuple(buf[i] for i in g))
    return tris


def parse_limbs(objdir, prefix):
    order = read(os.path.join(objdir, prefix + "Limbs.inc.c"))
    names = re.findall(r"&(\w+)", order)
    limbs = []
    for n in names:
        t = read(os.path.join(objdir, n + ".inc.c"))
        pos = [int(v) for v in re.findall(NUM + r",\s*// [xyz]", t)]
        child = re.search(r"/\* *(\d+) *\*/[^,]*,\s*// child", t)
        sib = re.search(r"/\* *(\d+) *\*/[^,]*,\s*// sibling", t)
        dl = re.search(r"(\w+|NULL),\s*// dList", t).group(1)
        limbs.append({"pos": pos, "child": int(child.group(1)), "sibling": int(sib.group(1)), "dl": dl})
    return limbs


def world_positions(limbs):
    out = [None] * len(limbs)

    def walk(i, base):
        if i == 255 or i >= len(limbs):
            return
        p = [base[k] + limbs[i]["pos"][k] for k in range(3)]
        out[i] = p
        walk(limbs[i]["child"], p)
        walk(limbs[i]["sibling"], base)

    walk(0, [0, 0, 0])
    return out


COLORS = [
    (230, 60, 60), (60, 160, 230), (80, 200, 90), (240, 180, 40), (170, 90, 220), (40, 200, 200),
    (240, 110, 180), (140, 140, 140), (200, 120, 60), (120, 220, 160), (90, 90, 230), (230, 230, 90),
    (150, 60, 90), (60, 120, 60), (230, 150, 120), (100, 180, 250),
]


def render(objdir, prefix, out):
    vtx = parse_vtx_files(objdir)
    vtxsyms = parse_object_c(objdir)
    limbs = parse_limbs(objdir, prefix)
    wpos = world_positions(limbs)

    polys = []  # (limb, [(x,y,z)...])
    for i, l in enumerate(limbs):
        if l["dl"] == "NULL":
            continue
        tris = parse_dl(os.path.join(objdir, l["dl"] + ".inc.c"), vtx, vtxsyms)
        for t in tris:
            polys.append((i, [tuple(v[k] + wpos[i][k] for k in range(3)) for v in t]))

    if not polys:
        sys.exit("no geometry parsed")

    xs = [p[0] for _, t in polys for p in t]
    ys = [p[1] for _, t in polys for p in t]
    zs = [p[2] for _, t in polys for p in t]
    print(f"bounds x {min(xs)}..{max(xs)} y {min(ys)}..{max(ys)} z {min(zs)}..{max(zs)}")

    W, H, pad = 900, 330, 20
    img = Image.new("RGB", (W, H * 2), (245, 245, 240))
    d = ImageDraw.Draw(img)
    sx = (W - 2 * pad) / (max(xs) - min(xs) + 1)
    s = min(sx, (H - 2 * pad) / (max(ys) - min(ys) + 1), (H - 2 * pad) / (max(zs) - min(zs) + 1))

    def proj(p, view):
        x = pad + (p[0] - min(xs)) * s
        if view == 0:  # side: x right, y up
            y = H - pad - (p[1] - min(ys)) * s
        else:  # top: x right, z down
            y = H + pad + (p[2] - min(zs)) * s
        return (x, y)

    for view, depth_axis in ((0, 2), (1, 1)):
        order = sorted(polys, key=lambda lp: sum(v[depth_axis] for v in lp[1]) * (1 if view == 0 else 1))
        for limb, t in order:
            d.polygon([proj(v, view) for v in t], fill=COLORS[limb % len(COLORS)], outline=(30, 30, 30))
        for i, p in enumerate(wpos):
            if p is None:
                continue
            x, y = proj(p, view)
            d.text((x + 2, y - 10), str(i), fill=(0, 0, 0))
    d.text((5, 5), f"{prefix} side view (x right, y up)", fill=(0, 0, 0))
    d.text((5, H + 5), f"{prefix} top view (x right, z down)", fill=(0, 0, 0))
    img.save(out)
    for i, l in enumerate(limbs):
        cnt = sum(1 for li, _ in polys if li == i)
        print(f"limb {i:2} dl={l['dl']:<24} tris={cnt:3} pos={wpos[i]}")


if __name__ == "__main__":
    render(sys.argv[1], sys.argv[2], sys.argv[3])
