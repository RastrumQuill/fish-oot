#!/usr/bin/env python3
"""The lake's rocks, each made for where it lies (fishmod/tools/rock_designs.py's E design, "made for its spot"): the
same placement as before (gen_scatter.py rocks_reworked), but each rock its own mesh, in the world, sized by what it
is. The triangles follow the size (a pebble needs few, a boulder more, an outcrop most), and a group of rocks is one
mesh (gen_scatter.py merges clusters into one broad rock: here an outcrop, three boulders under one hull, no faces
inside it).

  stone   (scale < 0.55): five sides, one ring above the base (turned half a side), a point on top
  boulder (scale < 1.2):  seven sides, the same
  outcrop (bigger):       three boulders in a row along the rock's length, one hull round them all: the middle one
                          seven-sided with two rings above its base, the ends five-sided with one

Every rock's base ring sits 3 units into the bed wherever it stands (it follows the slope: nothing hangs over, nothing
is buried deeper, no bottom face), its rings odd-sided and turned half a side (round from few vertices:
fishmod/docs/low-poly-shapes.md). The texture is projected by each vertex's main direction (the rock's texture is
noise: no seam shows), the colour is moss where it faces up and darker low down (the old rock's vertex colours).

The game's data (gen_scatter.py --c writes it into the scatter file) and its collision (gen_collision.py) are both
made from make(): the same rock from the same inputs (the placed prop's whole-unit position and 1/64 scale), so Link
stands on what he sees.
"""
import math
import random

import numpy as np

import gen_scatter as gs

BASE_SINK = 3.0   # the base ring this far into the bed
ROCK_W, ROCK_H = 60.0, 56.0  # the old rock's half-width and height at scale 1 (what each rock replaces)


def _bed(finder, x, z, y0):
    f = finder.floor(int(x), int(z), int(y0 + 400))
    return y0 if f is None else f


def make(x, y, z, scale, finder):
    """The rock standing at (x, y, z) (whole units: the prop's) at scale (a multiple of 1/64): (vertices [(x, y, z)]
    relative to (x, y, z), uvs [(u, v)] in texture repeats, colours [(r, g, b)] 0-1, triangles [(a, b, c)] counter-
    clockwise seen from outside)."""
    rnd = random.Random(x * 7 + z * 13)
    st = gs.rock_stretch(x, z)
    k = scale
    r = ROCK_W * k * (st[0] + st[2]) / 2
    h = ROCK_H * k * st[1]
    yaw = rnd.uniform(0, 2 * math.pi)

    def blob(ox, oz, rr, hh, n, lean):
        pts = []
        rings = ((1.0, None, 0.0), (0.92, 0.5, 0.5), (0.55, 0.9, 0.0))
        if lean:
            rings = ((1.0, None, 0.0), (0.8, 0.62, 0.5))  # (wide and low under a low top: a rock, not a cone)
        for ring, (fr, fh, tw) in enumerate(rings):
            m = n if ring < 2 else max(3, n - 2)
            for i in range(m):
                a = yaw + 2 * math.pi * (i + tw) / m
                rad = rr * fr * rnd.uniform(0.85, 1.12)
                px = x + ox + math.cos(a) * rad * 1.15
                pz = z + oz + math.sin(a) * rad * 0.9
                b = _bed(finder, px, pz, y)
                py = b - BASE_SINK if fh is None else b + hh * fh * rnd.uniform(0.85, 1.1)
                pts.append((px, py, pz))
        b = _bed(finder, x + ox, z + oz, y)
        top = (0.86, 0.96) if lean else (0.95, 1.08)
        pts.append((x + ox + rnd.uniform(-0.1, 0.1) * rr, b + hh * rnd.uniform(*top), z + oz))
        return pts

    if k < 0.55:
        pts = blob(0, 0, r, h * 0.55, 5, True)        # a stone
    elif k < 1.2:
        pts = blob(0, 0, r, h, 7, True)               # a boulder
    else:
        pts = []                                       # an outcrop: three boulders, one hull
        d = r * 0.55
        for dd, sz, n, lean in ((-d, 0.7, 5, True), (0.0, 1.0, 7, False), (d, 0.75, 5, True)):
            pts += blob(math.cos(yaw) * dd, math.sin(yaw) * dd, r * sz * 0.75, h * sz, n, lean)
    P = np.array(pts, float)
    from scipy.spatial import ConvexHull
    hull = ConvexHull(P)
    faces = []
    for simplex, eq in zip(hull.simplices, hull.equations):
        if eq[1] < -0.6:
            continue  # (the underside: on or in the bed)
        a, b, c = (int(i) for i in simplex)
        n = np.cross(P[b] - P[a], P[c] - P[a])
        faces.append((a, b, c) if np.dot(n, eq[:3]) > 0 else (a, c, b))
    used = sorted({i for f in faces for i in f})
    remap = {old: new for new, old in enumerate(used)}
    faces = [tuple(remap[i] for i in f) for f in faces]
    P = P[used]
    # each vertex's normal: its faces' (by area)
    N = np.zeros_like(P)
    for a, b, c in faces:
        N[[a, b, c]] += np.cross(P[b] - P[a], P[c] - P[a])
    N /= np.maximum(np.linalg.norm(N, axis=1), 1e-6)[:, None]
    ymin = P[:, 1].min()
    # the texture projected along each face's own main direction (no stretch across a face): a vertex for each point
    # and direction its faces use
    vs, uvs, cs, out_faces, index = [], [], [], [], {}
    for f in faces:
        fn = np.cross(P[f[1]] - P[f[0]], P[f[2]] - P[f[0]])
        ax = int(np.argmax(np.abs(fn)))
        tri = []
        for i in f:
            if (i, ax) not in index:
                p, n = P[i], N[i]
                q = (p[0] - x, p[1] - y, p[2] - z)  # (from the rock's own place: the coordinates stay small)
                u, v = ((q[2], q[1]), (q[0], q[2]), (q[0], q[1]))[ax]
                up = max(0.0, n[1])
                low = min(1.0, (p[1] - ymin) / max(h, 1.0))
                shade = 0.55 + 0.45 * low
                moss = up ** 1.5
                index[(i, ax)] = len(vs)
                cs.append((shade * (0.62 - 0.15 * moss), shade * (0.62 + 0.1 * moss), shade * (0.58 - 0.2 * moss)))
                vs.append(q)
                uvs.append((u / 64.0, -v / 64.0))
            tri.append(index[(i, ax)])
        out_faces.append(tuple(tri))
    faces = out_faces
    return vs, uvs, cs, faces


# ---- the game's data: F3DEX2 / F3DEX3 commands (the same encoding for these), as 32-bit words

G_VTX, G_TRI1, G_TRI2, G_DL, G_ENDDL, G_SETENVCOLOR, G_RDPPIPESYNC = 0x01, 0x05, 0x06, 0xDE, 0xDF, 0xFB, 0xE7
VTX_LOAD = 32  # vertices loaded at a time
STEP_TOKEN = 0xF15C0000  # a call to the caustics' fade step (| the step): the game puts in its list's address


def vtx_words(v, uv, c):
    """one Vtx (16 bytes) as four words: x, y, z, flag, s, t (10.5 of a 32-texel texture), r, g, b, a"""
    def s16(n):
        n = int(round(n))
        if not -32768 <= n <= 32767:
            raise ValueError(n)
        return n & 0xFFFF
    s = s16(uv[0] * 32 * 32)
    t = s16(uv[1] * 32 * 32)
    r, g, b = (max(0, min(255, int(round(ch * 255)))) for ch in c)
    return [(s16(v[0]) << 16) | s16(v[1]), (s16(v[2]) << 16), (s << 16) | t, (r << 24) | (g << 16) | (b << 8) | 255]


def pack(nverts, faces):
    """the vertex loads: [(the vertices in load order, the triangles in the load's own numbers)], each of up to VTX_LOAD
    vertices (a vertex several loads use is in each)"""
    loads, cur, tris = [], [], []
    for f in faces:
        new = [i for i in dict.fromkeys(f) if i not in cur]
        if len(cur) + len(new) > VTX_LOAD:
            loads.append((cur, tris))
            cur, tris = [], []
            new = list(dict.fromkeys(f))
        cur += new
        tris.append(tuple(cur.index(i) for i in f))
    if tris:
        loads.append((cur, tris))
    return loads


def dl_words(loads, vtx_offset, prefix=(), vdata=None):
    """the rock's list: its prefix (commands as word pairs), its vertex loads (the loads' vertices one after another
    from vtx_offset in the data: relocated to an address at load) and triangles (as F3DEX3 triangle snakes where the
    build draws them: snake.py, vdata each load's vertices as (x, y, z, s, t, r, g, b) for which may be turned), the end"""
    import snake
    out = []
    for w0, w1 in prefix:
        out += [w0, w1]
    at = vtx_offset
    for verts, tris in loads:
        n = len(verts)
        out += [(G_VTX << 24) | (n << 12) | (n << 1), at]
        at += n * 16
        vd = vdata[loads.index((verts, tris))] if vdata else [(0, 0, 0, 0, 0, 0, 0, 0)] * n
        out += snake.words(snake.run(tris, vd, 1.0))
    out += [G_ENDDL << 24, 0]
    return out
