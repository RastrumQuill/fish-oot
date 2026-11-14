#!/usr/bin/env python3
"""The plant cards drawn three ways, for comparing by look and by the RDP's cost (fishmod/tools/mesh_preview.py's
rasterizer and cost model): each kind as a small bed of nine under the water, near and far.
  now:   cut to the texture's outline, the outline split into triangles by ear clipping (gen_habitat.py as it is)
  low:   the same outline, split into the triangles of least total height (low_triangulate: the RDP pays a cost per
         scanline per triangle, so tall triangles fanned across the card cost most): the same pixels
  whole: the uncut cards (two triangles each), for reference

Usage: prop_designs.py OUT.png [--kinds bush_weed,broadleaf]
"""
import math
import os
import random
import sys

import numpy as np
from PIL import Image, ImageDraw

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_habitat as gh  # noqa: E402
import mesh_preview as mp  # noqa: E402

KINDS = ["bush_weed", "grass_tuft", "broadleaf", "big_pondweed", "ribbon_weed", "tall_weed", "kelp_frond",
         "vine_curtain"]
TRI_CONST = 0.04  # a triangle's fixed cost in card heights (about 2.3 scanlines of a card 60 scanlines tall)


low_triangulate = gh.low_triangulate

def find_maker(kind):
    for f in gh.MESHES:
        try:
            m = f()
        except Exception:
            continue
        if m.name == kind:
            return f
    raise KeyError(kind)


def bed(V, UV, C, F, n=9, spacing=34.0, seed=1):
    rnd = random.Random(seed)
    vs, uvs, cs, fs = [], [], [], []
    for i in range(n):
        a = rnd.uniform(0, 2 * math.pi)
        s = rnd.uniform(0.85, 1.15)
        R = np.array([[math.cos(a), 0, math.sin(a)], [0, 1, 0], [-math.sin(a), 0, math.cos(a)]])
        off = np.array([(i % 3 - 1) * spacing + rnd.uniform(-8, 8), 0, (i // 3 - 1) * spacing + rnd.uniform(-8, 8)])
        base = sum(len(x) for x in vs)
        vs.append(V @ R.T * s + off)
        uvs.append(UV)
        cs.append(C)
        fs += [(a_ + base, b + base, c + base) for a_, b, c in F]
    return np.concatenate(vs), np.concatenate(uvs), np.concatenate(cs), fs


VIEWS = [("near (about 120)", (0, 35, -120), (0, 20, 0)), ("far (about 380)", (60, 60, -370), (0, 15, 0))]
FOG = ((60, 95, 90), 60.0, 650.0)


def main():
    out = sys.argv[1]
    kinds = KINDS
    if "--kinds" in sys.argv:
        kinds = sys.argv[sys.argv.index("--kinds") + 1].split(",")
    hows = ["now", "low", "whole"]
    makers = {k: find_maker(k) for k in kinds}
    scale = 1
    cw, ch = mp.W * scale, mp.H * scale
    sheet = Image.new("RGB", (160 + cw * len(hows) * len(VIEWS), 20 + len(kinds) * (ch + 2)), (30, 30, 30))
    d = ImageDraw.Draw(sheet)
    for j, how in enumerate(hows):
        for vi, (vn, _, _) in enumerate(VIEWS):
            d.text((160 + (vi * len(hows) + j) * cw + 4, 4), "%s, %s" % (how, vn), fill=(255, 255, 255))
    print("| kind | how | tris a plant | near: us (tris drawn) | far: us |")
    print("|---|---|---|---|---|")
    for i, k in enumerate(kinds):
        y = 20 + i * (ch + 2)
        d.text((4, y + 4), k, fill=(255, 255, 0))
        for j, how in enumerate(hows):
            m = makers[k]()
            if how == "whole":
                v, vt, f = m.v, m.vt, m.f
            else:
                se = gh.ear_clip
                if how == "low":
                    gh.ear_clip = low_triangulate
                try:
                    t = m.trimmed()
                finally:
                    gh.ear_clip = se
                v, vt, f = (m.v, m.vt, m.f) if t is None else (t[0], t[1], t[2])
            V = np.array([p[:3] for p in v], float)
            C = np.array([p[3:6] for p in v], float)
            UV = np.array(vt, float)
            F = [(a - 1, b - 1, c - 1) for a, b, c in f]
            T = np.asarray(Image.open(gh.texture_path(m.texture)).convert("RGBA"))
            bv, buv, bc, bf = bed(V, UV, C, F)
            costs = []
            for vi, (vn, eye, at) in enumerate(VIEWS):
                c = mp.Canvas(sky=FOG[0])
                c.draw(mp.look_at(eye, at), bv, buv, bc, bf, T, cull=False, fog=FOG)
                sheet.paste(c.image(scale), (160 + (vi * len(hows) + j) * cw, y))
                d.text((160 + (vi * len(hows) + j) * cw + 4, y + ch - 14), "%.0f us, %d tris" % (c.cost_ns / 1e3,
                                                                                               c.tris),
                       fill=(255, 255, 255))
                costs.append((c.cost_ns / 1e3, c.tris))
            print("| %s | %s | %d | %.0f (%d) | %.0f |" % (k, how, len(F), costs[0][0], costs[0][1], costs[1][0]))
    sheet.save(out)


if __name__ == "__main__":
    main()
