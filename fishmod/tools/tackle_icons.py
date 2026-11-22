#!/usr/bin/env python3
"""The tackle's item icons (the pond lure, the sinking lure, the sinker, the bobber), rendered as the game's icons are
(fishmod/tools/icon_render.py). The pond lure and the sinking lure are the Fishing Pond's own: its float lure's mesh
and texture, its hooks, the sinking lure's beads (its segment texture under its four corner colours, as the RSP draws
them). So this reads the extracted assets: run at build time (fishmod/tools/gen_item_page.py), never committed.

  python3 fishmod/tools/tackle_icons.py --sheet OUT.png   a contact sheet: each icon beside vanilla's, 1x and 4x
"""
import argparse
import math
import os
import re
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import icon_render as ir  # noqa: E402

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
FISH = os.path.join(ROOT, "extracted", "ntsc-1.2", "assets", "objects", "object_fish")
ICONS = os.path.join(ROOT, "extracted", "ntsc-1.2", "assets", "textures", "icon_item_static")


def _vtx(path):
    """VTX(x, y, z, s, t, nx, ny, nz, a) lines: [(pos, st, normal)]."""
    out = []
    for m in re.finditer(r"VTX\(([^)]*)\)", open(path).read()):
        v = [int(x.strip(), 0) for x in m.group(1).split(",")]
        n = [(c - 256 if c > 127 else c) / 127.0 for c in v[5:8]]
        out.append(((v[0], v[1], v[2]), (v[3] / 32.0, v[4] / 32.0), n))
    return out


def _tris(path):
    t = []
    for m in re.finditer(r"gsSP2Triangles\((\d+), (\d+), (\d+), 0, (\d+), (\d+), (\d+), 0\)", open(path).read()):
        v = list(map(int, m.groups()))
        t += [v[:3], v[3:]]
    for m in re.finditer(r"gsSP1Triangle\((\d+), (\d+), (\d+), 0\)", open(path).read()):
        t.append(list(map(int, m.groups())))
    return t


def _hook_mesh(at, scale, yaw=0.0):
    """The pond's hook (its texture on two crossed cards), hanging from `at` (its eye), in the lure's units."""
    tex = ir.load_texture(os.path.join(FISH, "gFishingLureHookTex.rgba16.png"))
    m = ir.Mesh(tex, gloss=0.7, shine=30, alpha_cut=0.4)
    m.lift = 0.25  # (the hook's grey texture lit up a little: steel)
    verts = _vtx(os.path.join(FISH, "gFishingLureHookVtx.inc.c"))
    tw, th = tex.shape[1], tex.shape[0]
    for k in range(2):
        a = yaw + k * math.pi / 2
        ca, sa = math.cos(a), math.sin(a)
        ids = []
        for (x, y, z), (s, t), _ in verts:
            # the hook's card lies in its x-z plane, its eye at z 27: hung down from the eye, turned by a
            px = x * scale
            py = (z - 27) * scale * 1.25
            ids.append(m.vert((at[0] + px * ca, at[1] + py, at[2] + px * sa), (sa, 0, ca), (s / tw, t / th)))
        m.tri(ids[0], ids[1], ids[2])
        m.tri(ids[0], ids[2], ids[3])
    return m


def pond_lure():
    tex = ir.load_texture(os.path.join(FISH, "gFishingLureFloatTex.rgba16.png"))
    m = ir.Mesh(tex, gloss=0.55, shine=30)
    tw, th = tex.shape[1], tex.shape[0]
    # (its depth a third more than the game's, as an icon's subject is drawn plain: a minnow, not a blade)
    ids = [m.vert((p[0], p[1] * 1.35, p[2] * 1.2), n, (s / tw, t / th))
           for p, (s, t), n in _vtx(os.path.join(FISH, "gFishingLureFloatVtx.inc.c"))]
    for a, b, c in _tris(os.path.join(FISH, "gFishingLureFloatDL.inc.c")):
        m.tri(ids[a], ids[b], ids[c])
    hooks = [_hook_mesh((150, -390, 0), 1.35, 0.5), _hook_mesh((1500, -260, 0), 1.35, 1.1)]
    return ir.render([m] + hooks, yaw=-0.35, pitch=0.25, roll=0.55, margin=0.03)


def sinking_lure():
    seg = np.asarray(Image.open(os.path.join(FISH, "gFishingSinkingLureSegmentTex.ia8.png")).convert("LA"), float) / 255
    h, w = seg.shape[:2]
    # the RSP's (1 - SHADE) x TEXEL0 + SHADE, its shade the quad's four corner colours (gFishingSinkingLureSegmentModelVtx)
    corners = {(1, 1): (255, 0, 100), (0, 1): (0, 150, 255), (0, 0): (255, 150, 0), (1, 0): (0, 255, 150)}
    vv, uu = np.mgrid[0:h, 0:w] / np.array([[h - 1]]), np.mgrid[0:h, 0:w][1] / (w - 1)
    vv = 1.0 - np.mgrid[0:h, 0:w][0] / (h - 1)

    def corner(u, v):
        return np.array(corners[(u, v)], float) / 255

    shade = (corner(0, 0) * ((1 - uu) * (1 - vv))[..., None] + corner(1, 0) * (uu * (1 - vv))[..., None] +
             corner(0, 1) * ((1 - uu) * vv)[..., None] + corner(1, 1) * (uu * vv)[..., None])
    I = seg[..., :1]
    # (the game blends them half see-through over the water, a shimmer; on the icon's dark ground they're made
    # solid enough to read, the colours the game's)
    bead = np.dstack([(1 - shade) * I + shade, np.clip(seg[..., 1] * 2.6, 0, 1)])
    sizes = [1.0, 1.5, 1.8, 2.0, 1.8, 1.6, 1.4, 1.2, 1.0, 1.0, 0.9, 0.85, 0.8, 0.7, 0.8, 1.0, 1.2, 1.1, 1.0, 0.8]
    sprites = []
    # an S along the icon's diagonal, 0.85 between beads as the pond's
    pts = []
    x, y, a = 0.0, 0.0, -0.35
    for i in range(len(sizes)):
        pts.append((x, y))
        a -= 0.36 * math.cos(i * 0.33)  # (a hooked S, its head up at the left, its tail curling down)
        x += 0.85 * math.cos(a)
        y += 0.85 * math.sin(a)
    for (x, y), s in zip(pts, sizes):
        sprites.append((x, y, 0.0, 0.64 * s * 1.45, bead))  # (fuller than the game's thin chain: an icon's subject)
    hook = _hook_mesh((pts[0][0] - 0.6, pts[0][1] + 0.9, 0.3), 0.0042, 0.6)
    return ir.render([hook], sprites=sprites, margin=0.05)


def sinker():
    lead = ir.Mesh(gloss=0.55, shine=26)
    prof = [(0.0, 1.05), (0.07, 1.0), (0.13, 0.85), (0.22, 0.62), (0.32, 0.38), (0.40, 0.15), (0.43, 0.0),
            (0.42, -0.12), (0.37, -0.24), (0.27, -0.33), (0.14, -0.38), (0.0, -0.40)]
    lead.lathe(prof, 28, lambda t, y, a: (0.50, 0.53, 0.60))
    ring = ir.Mesh(gloss=0.9, shine=40)
    ring.torus((0, 1.17, 0), (0.0, 1.0, 0.0), (1.0, 0.0, 0.0), 0.12, 0.035, 20, (0.92, 0.72, 0.28))
    swivel = ir.Mesh(gloss=0.9, shine=40)
    swivel.lathe([(0.0, 1.55), (0.045, 1.5), (0.06, 1.42), (0.045, 1.34), (0.0, 1.3)], 12,
                 lambda t, y, a: (0.92, 0.72, 0.28))
    ring2 = ir.Mesh(gloss=0.9, shine=40)
    ring2.torus((0, 1.66, 0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0), 0.1, 0.03, 20, (0.92, 0.72, 0.28))
    return ir.render([lead, ring, swivel, ring2], yaw=0.3, pitch=0.25, roll=-0.38, margin=0.05)


def bobber():
    body = ir.Mesh(gloss=0.65, shine=36)
    prof = []
    for k in range(19):
        a = math.pi * k / 18
        prof.append((0.5 * math.sin(a), 0.5 * math.cos(a)))

    def col(t, y, a):
        if abs(y) < 0.035:
            return (0.12, 0.1, 0.1)
        return (0.88, 0.1, 0.08) if y > 0 else (0.95, 0.94, 0.88)

    body.lathe(prof, 28, col)
    stick = ir.Mesh(gloss=0.4, shine=20)
    stick.lathe([(0.0, 1.0), (0.05, 0.97), (0.055, 0.5), (0.055, -0.5), (0.045, -0.9), (0.0, -0.95)], 10,
                lambda t, y, a: (0.9, 0.1, 0.08) if y > 0.8 else (0.96, 0.95, 0.9))
    return ir.render([body, stick], yaw=0.2, pitch=0.3, roll=-0.42, margin=0.05)


ICON = {"lure_pond": pond_lure, "lure_sink": sinking_lure, "sinker": sinker, "bobber": bobber}


def icon(kind):
    """32x32 RGBA (PIL), as gen_item_page.py wants it."""
    return ICON[kind]()


def sheet(path, extra=()):
    """Each icon beside the game's own (the rod, the hookshot, the boomerang, the bug bottle, a bean), on the item
    page's red, at 1x and 4x."""
    refs = ["gItemIconFishingPoleTex", "gItemIconHookshotTex", "gItemIconBoomerangTex", "gItemIconBottleBugTex"]
    ims = [Image.open(os.path.join(ICONS, r + ".rgba32.png")).convert("RGBA") for r in refs]
    ims += [icon(k) for k in ICON] + [Image.open(p).convert("RGBA") for p in extra]
    c = Image.new("RGBA", (len(ims) * 136, 182), (72, 22, 22, 255))
    for i, im in enumerate(ims):
        c.alpha_composite(im.resize((128, 128), Image.NEAREST), (4 + i * 136, 4))
        c.alpha_composite(im, (4 + i * 136 + 48, 142))
    c.save(path)


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--sheet")
    ap.add_argument("--big", help="also each icon at 8x before shrinking, to this folder")
    ap.add_argument("extra", nargs="*", help="more icons for the sheet (the bait bottles)")
    a = ap.parse_args()
    if a.sheet:
        sheet(a.sheet, a.extra)
