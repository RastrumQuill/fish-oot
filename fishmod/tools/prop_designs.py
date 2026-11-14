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


def low_triangulate(poly, tri_const=TRI_CONST):
    """Triangles (index triples) for a simple polygon of (u, v) points, chosen to make the sum of the triangles'
    heights in v (plus a constant per triangle) least: a minimum-weight triangulation, by dynamic programming over
    the polygon's diagonals (those inside it only)."""
    n = len(poly)
    if n < 3:
        return []
    P = [tuple(map(float, p)) for p in poly]

    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    area = sum(P[i][0] * P[(i + 1) % n][1] - P[(i + 1) % n][0] * P[i][1] for i in range(n))
    sign = 1 if area > 0 else -1

    def seg_cross(p, q, r, s):
        d1, d2 = cross(p, q, r), cross(p, q, s)
        d3, d4 = cross(r, s, p), cross(r, s, q)
        return (d1 * d2 < 0) and (d3 * d4 < 0)

    def inside(x, y):
        c = False
        for i in range(n):
            (x1, y1), (x2, y2) = P[i], P[(i + 1) % n]
            if (y1 > y) != (y2 > y) and x < x1 + (y - y1) * (x2 - x1) / (y2 - y1):
                c = not c
        return c

    ok = [[False] * n for _ in range(n)]
    for i in range(n):
        for j in range(i + 1, n):
            if j == i + 1 or (i == 0 and j == n - 1):
                ok[i][j] = ok[j][i] = True
                continue
            if any(seg_cross(P[i], P[j], P[k], P[(k + 1) % n]) for k in range(n)
                   if len({i, j, k, (k + 1) % n}) == 4):
                continue
            mx, my = (P[i][0] + P[j][0]) / 2, (P[i][1] + P[j][1]) / 2
            if inside(mx, my):
                ok[i][j] = ok[j][i] = True
    INF = float("inf")
    cost = [[0.0] * n for _ in range(n)]
    pick = [[-1] * n for _ in range(n)]
    for gap in range(2, n):
        for i in range(0, n - gap):
            j = i + gap
            best, bk = INF, -1
            if not ok[i][j]:
                cost[i][j] = INF
                continue
            for k in range(i + 1, j):
                if not (ok[i][k] and ok[k][j]):
                    continue
                if cross(P[i], P[k], P[j]) * sign <= 0:
                    continue  # (a triangle turned the wrong way: outside the polygon)
                ys = (P[i][1], P[k][1], P[j][1])
                c = cost[i][k] + cost[k][j] + tri_const + (max(ys) - min(ys))
                if c < best:
                    best, bk = c, k
            cost[i][j], pick[i][j] = best, bk
    if cost[0][n - 1] == INF:
        return gh.ear_clip(poly)  # (no valid split found: as before)
    out = []

    def walk(i, j):
        if j - i < 2:
            return
        k = pick[i][j]
        out.append((i, k, j))
        walk(i, k)
        walk(k, j)
    walk(0, n - 1)
    return out


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
