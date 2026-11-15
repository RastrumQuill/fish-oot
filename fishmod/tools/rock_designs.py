#!/usr/bin/env python3
"""The lake's rocks drawn four ways, on the lake bed itself (its collision floors), for comparing by look and by the
RDP's cost (fishmod/tools/mesh_preview.py's rasterizer and cost model):
  A now:      vanilla's Fishing Pond rock (16 triangles), every one drawn, back faces culled (as the game draws it)
  B pruned:   each rock's triangles that lie wholly under the bed where it sits left out (the game would draw each
              rock with its own list: there are a few dozen)
  C cut:      B, and the triangles that go into the bed cut off just under it (2 units: no gap shows where the bed
              isn't flat): the buried part of a rock costs the RDP as much as the part that shows
  D new:      a rock of its own, an icosahedron (20 triangles) roughened, as big as the vanilla one, cut as C
Each rock is placed exactly as the game places it (gen_collision.py place: stretched, tilted, turned, scaled).

Usage: rock_designs.py OUT.png
"""
import math
import os
import random
import sys

import numpy as np
from PIL import Image, ImageDraw

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_collision as gc  # noqa: E402
import gen_scatter as gs  # noqa: E402
import gen_watermap as gw  # noqa: E402
import mesh_preview as mp  # noqa: E402

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
ROCK_OBJ = os.path.join(ROOT, "src", "fishmod_data", "habitat_imp", "rock_vc.obj")
ROCK_PNG = os.path.join(ROOT, "fishmod", "art", "habitat_rom", "rock_vc.png")
SPOT06 = os.path.join(ROOT, "extracted", "ntsc-1.2", "assets", "scenes", "overworld", "spot06")
CUT_BELOW = 2.0


def load_rock():
    v, vt, f, col = [], [], [], []
    for line in open(ROCK_OBJ):
        p = line.split()
        if not p:
            continue
        if p[0] == "v":
            v.append([float(x) for x in p[1:4]])
            col.append([float(x) for x in p[4:7]])
        elif p[0] == "vt":
            vt.append([float(p[1]), 1.0 - float(p[2])])
        elif p[0] == "f":
            f.append([tuple(int(i) - 1 for i in x.split("/")[:2]) for x in p[1:4]])
    return np.array(v), np.array(vt), np.array(col), f


def new_rock(seed):
    """an icosahedron roughened (each vertex out or in a little), flattened like the vanilla rock (56 tall, about 120
    across at scale 1), its base at y -5; UVs by angle round it and height, colour: moss on top, darker low"""
    p = (1 + 5 ** 0.5) / 2
    V = np.array([(-1, p, 0), (1, p, 0), (-1, -p, 0), (1, -p, 0), (0, -1, p), (0, 1, p), (0, -1, -p), (0, 1, -p),
                  (p, 0, -1), (p, 0, 1), (-p, 0, -1), (-p, 0, 1)], float)
    F = [(0, 11, 5), (0, 5, 1), (0, 1, 7), (0, 7, 10), (0, 10, 11), (1, 5, 9), (5, 11, 4), (11, 10, 2), (10, 7, 6),
         (7, 1, 8), (3, 9, 4), (3, 4, 2), (3, 2, 6), (3, 6, 8), (3, 8, 9), (4, 9, 5), (2, 4, 11), (6, 2, 10),
         (8, 6, 7), (9, 8, 1)]
    rnd = random.Random(seed)
    V /= np.linalg.norm(V, axis=1)[:, None]
    V *= np.array([[rnd.uniform(0.85, 1.12)] for _ in V])
    V *= np.array([60.0, 34.0, 58.0])
    V[:, 1] += 22.0  # (most of it above its base, as the vanilla rock)
    uv = np.c_[(np.arctan2(V[:, 2], V[:, 0]) / (2 * math.pi) + 0.5) * 2.0, (60.0 - V[:, 1]) / 60.0]
    up = (V[:, 1] - V[:, 1].min()) / np.ptp(V[:, 1])
    col = np.c_[0.45 + 0.25 * up, 0.5 + 0.3 * up, 0.42 + 0.15 * up]
    return V, uv, col, [((a, a), (b, b), (c, c)) for a, b, c in F]


def made_for(rock, finder, seed=None, lean=False):
    """E: a rock made for its spot (world coordinates): a stone, a boulder or an outcrop by size, its base ring 3 units
    into the bed wherever it stands (it follows the slope: nothing deeper, no bottom face), rings of odd counts turned
    half a side (round from few vertices: fishmod/docs/low-poly-shapes.md), an outcrop one hull round three boulders
    (no faces inside it). Texture projected by each face's direction (no stretch), moss on what faces up."""
    from scipy.spatial import ConvexHull
    rnd = random.Random(seed if seed is not None else rock["x"] * 7 + rock["z"] * 13)
    st = gs.rock_stretch(rock["x"], rock["z"])
    k = rock["scale"]
    r = 60.0 * k * (st[0] + st[2]) / 2
    h = 56.0 * k * st[1]
    cx, cz = rock["x"], rock["z"]
    yaw = rnd.uniform(0, 2 * math.pi)

    def bed(x, z):
        f = finder.floor(int(x), int(z), int(rock["y"] + 400))
        return rock["y"] if f is None else f

    def blob(ox, oz, rr, hh, n):
        pts = []
        rings = ((1.0, -3.0, 0.0), (0.92, 0.5, 0.5), (0.55, 0.9, 0.0))
        if lean:
            rings = ((1.0, -3.0, 0.0), (0.7, 0.75, 0.5))  # (the base, one ring turned half a side, the top)
        for ring, (fr, fh, tw) in enumerate(rings):
            m = n if (ring < 2 and not lean) or ring == 0 else max(3, n - 2)
            for i in range(m):
                a = yaw + 2 * math.pi * (i + tw) / m
                rad = rr * fr * rnd.uniform(0.85, 1.12)
                x, z = cx + ox + math.cos(a) * rad * 1.15, cz + oz + math.sin(a) * rad * 0.9
                b = bed(x, z)
                y = b - 3.0 if ring == 0 else b + hh * fh * rnd.uniform(0.85, 1.1)
                pts.append((x, y, z))
        b = bed(cx + ox, cz + oz)
        pts.append((cx + ox + rnd.uniform(-0.1, 0.1) * rr, b + hh * rnd.uniform(0.95, 1.08), cz + oz))
        return pts

    if k < 0.55:
        pts = blob(0, 0, r, h * 0.55, 5)       # a stone: low, five-sided
    elif k < 1.2:
        pts = blob(0, 0, r, h, 5 if lean else 7)  # a boulder
    else:
        pts = []                              # an outcrop: three boulders, one hull
        d = r * 0.55
        for j, (dd, sz) in enumerate(((-d, 0.7), (0.0, 1.0), (d, 0.75))):
            pts += blob(math.cos(yaw) * dd, math.sin(yaw) * dd, r * sz * 0.75, h * sz, 5 if lean else 7)
    P = np.array(pts, float)
    hull = ConvexHull(P)
    faces = []
    for simplex, eq in zip(hull.simplices, hull.equations):
        if eq[1] < -0.6:
            continue  # (the underside: on or in the bed)
        a, b, c = simplex
        n = np.cross(P[b] - P[a], P[c] - P[a])
        faces.append((a, b, c) if np.dot(n, eq[:3]) > 0 else (a, c, b))
    # per face: its own corners (the texture projected by its direction), moss by how much it faces up
    vs, uvs, cs, fs = [], [], [], []
    ymin = P[:, 1].min()
    for a, b, c in faces:
        n = np.cross(P[b] - P[a], P[c] - P[a])
        n /= np.linalg.norm(n)
        ax = int(np.argmax(np.abs(n)))
        base_i = len(vs)
        for i in (a, b, c):
            x, y, z = P[i]
            u, v = ((z, y), (x, z), (x, y))[ax]
            up = max(0.0, n[1])
            low = min(1.0, (y - ymin) / max(h, 1.0))
            shade = 0.55 + 0.45 * low
            moss = up ** 1.5
            cs.append((shade * (0.62 - 0.15 * moss), shade * (0.62 + 0.1 * moss), shade * (0.58 - 0.2 * moss)))
            vs.append((x, y, z))
            uvs.append((u / 64.0, -v / 64.0))
        fs.append((base_i, base_i + 1, base_i + 2))
    return np.array(vs), np.array(uvs), np.array(cs), fs


def orient_out(P, faces, centre):
    out = []
    for f in faces:
        a, b, c = (P[i] for i, _ in f)
        n = np.cross(b - a, c - a)
        out.append(f if np.dot(n, (a + b + c) / 3 - centre) >= 0 else (f[0], f[2], f[1]))
    return out


def clip_tri(P, UV, C, H):
    """the part of a triangle above h = 0 (h: height over the cut, per vertex), as a polygon of (pos, uv, col)"""
    pts = list(zip(P, UV, C, H))
    out = []
    for i in range(3):
        a, b = pts[i], pts[(i + 1) % 3]
        if a[3] >= 0:
            out.append(a[:3])
        if (a[3] >= 0) != (b[3] >= 0):
            t = a[3] / (a[3] - b[3])
            out.append(tuple(a[k] + (b[k] - a[k]) * t for k in range(3)))
    return out


def rock_mesh(rock, design, finder, base):
    """one rock's world triangles for a design: (verts, uvs, cols, faces)"""
    if design == "E":
        return made_for(rock, finder)
    if design == "F":
        return made_for(rock, finder, lean=True)
    if design == "D":
        V, UV, C, F = new_rock(rock["x"] * 31 + rock["z"])
    else:
        V, UV, C, F = base
    P = np.array(gc.place(rock, [tuple(v) for v in V]), float)
    centre = P.mean(axis=0)
    F = orient_out(P, F, centre)
    floor = np.array([(finder.floor(int(x), int(z), int(y) + 400) or -1e9) for x, y, z in P], float)
    H = P[:, 1] - (floor - CUT_BELOW)
    vs, uvs, cs, fs = [], [], [], []
    for f in F:
        idx = [i for i, _ in f]
        tdx = [t for _, t in f]
        if design in ("B", "C", "D") and all(H[i] < 0 for i in idx):
            continue  # (wholly under the bed)
        if design in ("C", "D") and any(H[i] < 0 for i in idx):
            poly = clip_tri([P[i] for i in idx], [UV[t] for t in tdx], [C[i] for i in idx], [H[i] for i in idx])
            if len(poly) < 3:
                continue
            base_i = len(vs)
            for p, uv, c in poly:
                vs.append(p)
                uvs.append(uv)
                cs.append(c)
            for k in range(1, len(poly) - 1):
                fs.append((base_i, base_i + k, base_i + k + 1))
            continue
        base_i = len(vs)
        for i, t in zip(idx, tdx):
            vs.append(P[i])
            uvs.append(UV[t])
            cs.append(C[i])
        fs.append((base_i, base_i + 1, base_i + 2))
    return np.array(vs), np.array(uvs), np.array(cs), fs


def bed_near(floors, cx, cz, r, edge=60.0):
    """the bed's floor triangles round a point, split into pieces no longer than `edge` (the preview's rasterizer
    drops a triangle that reaches behind the camera instead of clipping it)"""
    vs, fs = [], []

    def add(t):
        a, b, c = (np.array(v, float) for v in t)
        lens = [np.linalg.norm(b - a), np.linalg.norm(c - b), np.linalg.norm(a - c)]
        if max(lens) > edge:
            k = int(np.argmax(lens))
            p, q, o = [(a, b, c), (b, c, a), (c, a, b)][k]
            m = (p + q) / 2
            add((p, m, o))
            add((m, q, o))
            return
        if all((v[0] - cx) ** 2 + (v[2] - cz) ** 2 > r * r for v in (a, b, c)):
            return
        n = len(vs)
        vs.extend([a, b, c])
        fs.append((n, n + 1, n + 2))
    for t in floors:
        xs, zs = [v[0] for v in t], [v[2] for v in t]
        if (min(xs) - r <= cx <= max(xs) + r) and (min(zs) - r <= cz <= max(zs) + r):
            add(t)
    return np.array(vs), fs


def main():
    out = sys.argv[1]
    d = os.path.join(gw.SCENES, gs.SCENES["SCENE_LAKE_HYLIA"])
    floors = gw.parse_floors(gw.one(d, "*_PolyList.inc.c"), gw.parse_vtx(gw.one(d, "*_VtxList.inc.c")))
    finder = gw.FloorFinder(floors)
    rocks = [p for p in gc.props("spot06") if p["kind"] == "rock"]
    base = load_rock()
    tex = np.asarray(Image.open(ROCK_PNG).convert("RGBA"))
    bed_tex = np.asarray(Image.open(os.path.join(SPOT06, "spot06_scene_0000C2B8_Tex.rgba16.png")).convert("RGBA"))
    # whole lake: triangles, and how much of the rocks is under the bed
    print("%d rocks" % len(rocks))
    for design in "AEF":
        tris = sum(len(rock_mesh(r, design, finder, base)[3]) for r in rocks)
        print("  %s: %d triangles in all" % (design, tris))
    # three views: the densest cluster, a big lone rock, a shallow one
    rocks.sort(key=lambda r: -r["scale"])
    picks = [rocks[0], rocks[len(rocks) // 2], rocks[-1]]
    designs = [("A now", "A"), ("E made for its spot", "E"), ("F the same, lean", "F")]
    cw, ch = mp.W * 2, mp.H * 2
    sheet = Image.new("RGB", (cw * len(designs), 20 + (ch + 2) * len(picks)), (30, 30, 30))
    dr = ImageDraw.Draw(sheet)
    for j, (name, _) in enumerate(designs):
        dr.text((j * cw + 6, 4), name, fill=(255, 255, 0))
    fog = ((60, 95, 90), 80.0, 900.0)
    for i, focus in enumerate(picks):
        near = [r for r in rocks if (r["x"] - focus["x"]) ** 2 + (r["z"] - focus["z"]) ** 2 < 500 ** 2]
        bv, bf = bed_near(floors, focus["x"], focus["z"], 700)
        reach = 60 + 70 * focus["scale"]
        # the camera: under the water (the full lake's surface at -1313), above the bed where it stands, from the
        # side with the most water above the bed, looking at the rock a little from above
        best = None
        for k in range(8):
            a = 2 * math.pi * k / 8
            ex, ez = focus["x"] + math.cos(a) * reach * 3.0, focus["z"] + math.sin(a) * reach * 3.0
            fl = finder.floor(int(ex), int(ez), -1313)
            if fl is None:
                continue
            room = -1320 - fl
            if best is None or room > best[0]:
                best = (room, ex, ez, fl)
        _, ex, ez, fl = best
        eye = (ex, min(-1320.0, max(fl + 60.0, focus["y"] + reach * 0.8)), ez)
        at = (focus["x"], focus["y"] + 10 * focus["scale"], focus["z"])
        mvp = mp.look_at(eye, at)
        for j, (name, dz) in enumerate(designs):
            c = mp.Canvas(sky=fog[0])
            c.draw(mvp, bv, np.c_[bv[:, 0] / 64.0, bv[:, 2] / 64.0], np.full((len(bv), 3), 0.8), bf, bed_tex,
                   cull=False, fog=fog, count=False)
            for r in near:
                v, uv, col, f = rock_mesh(r, dz, finder, base)
                if len(f):
                    c.draw(mvp, v, uv, col, f, tex, cull=True, fog=fog)
            sheet.paste(c.image(2), (j * cw, 20 + i * (ch + 2)))
            dr.text((j * cw + 6, 20 + i * (ch + 2) + ch - 16),
                    "%d rocks: %.0f us of RDP, %d tris drawn" % (len(near), c.cost_ns / 1e3, c.tris), fill=(255, 255, 255))
            print("view %d %-18s %.0f us, %d tris, %.0f px" % (i, name, c.cost_ns / 1e3, c.tris, c.px))
    sheet.save(out)


if __name__ == "__main__":
    main()
