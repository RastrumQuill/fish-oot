#!/usr/bin/env python3
"""fish-oot: the scenes' seams closed (build.sh, before the mesh pass): cracks between surfaces that were meant to meet.

Two kinds, both from how the game's levels were made:
- Near misses: two corners meant to be one, a unit or two apart (the Temple of Time has 18 pairs exactly 1 unit
  apart). The N64's models are coarse: points of one scene that close are the same point. Welded: each group of
  corners within WELD units moved to the one most triangles use. Their texture coordinates and colours stay as they
  are. Not welded: a layer laid over a surface on purpose (a decal a unit above the floor, its corners straight above
  the floor's): the move would run along both surfaces' normal.
- T-junctions: a corner lying on another triangle's edge, between its ends (one side of a seam cut in two, the other
  not). The positions match, but the rasteriser fills the long edge and the two short ones differently: a line of
  pixel cracks along it. The long edge's triangle is split at the corner (a fan from its far corner), the new
  corner's texture coordinates and colour (or normal) taken along the edge it's on, so its picture doesn't move.
  Only where a surface beside it runs along the same line (a real seam, not a corner that happens to touch an edge).

Exact otherwise: every display list this changes keeps its commands in order; its triangles are drawn in the same
order with their own vertices (the split ones as their pieces), loaded afresh from an array of its own. A list holding
a command that reads the vertex buffer other than its triangles (a vertex edit, a depth branch) is left as it is; a
cull's box load before it is kept as it was.

The original tree isn't touched: the output is a copy (hard links for every file the pass doesn't change), which the
build then reads (EXTRACTED_DIR), like tools/meshpack.py.

Usage: tools/seampass.py SRC_TREE OUT_TREE [--report]
       tools/seampass.py --scan SRC_TREE SCENE_DIR...   (counts only, nothing written)
"""
import collections
import math
import os
import re
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import meshpack as mp  # noqa: E402  (the display list parser and the vertex interpolation)

WELD = 2.0        # corners this close (units) are one
ON_LINE = 0.6     # a corner this close to an edge's line lies on it
# The longest edge split (units). A split adds a corner, and the game works out fog (and light, on lit surfaces) at
# each corner and blends it between them: a long triangle split has its fog worked out at the new corner rather than
# blended there, which shifts the whole surface's shading (the castle courtyard's walkway came out darker). Short edges
# shift too little to see.
MAX_SPLIT_EDGE = float(os.environ.get("SEAMPASS_MAX_EDGE", "200"))
# ...and a triangle is split only if it's small (no edge longer than this) or its corners shade alike (within this, of
# 255, in every channel): see process()
SPLIT_SMALL = float(os.environ.get("SEAMPASS_SPLIT_SMALL", "64"))
SPLIT_SHADE_TOL = int(os.environ.get("SEAMPASS_SHADE_TOL", "8"))
END_FRAC = 0.01   # ...and between its ends by this much of its length
DECAL_COS = 0.9   # a weld along both surfaces' normals (cosine) is a layer, not a crack
VTX_SLOTS = 32    # the vertex buffer the re-emitted lists load into (F3DEX2's; F3DEX3 has more)
SKIP_SCENES = set()  # scene directories left alone (by their last path part)
# Decals: a surface laid on another (a render mode whose depth mode is the decal's, ZMODE_DEC). Neither it nor the
# surface under it is altered: a floor welded or split under its decal lifts or tilts out from under it, and the
# decal's depth test (equal within a tolerance) fails in patches. SEAMPASS_NO_DECAL_GUARD=1: as the pass was (to count
# what the guard keeps: --check)
DECAL_GUARD = os.environ.get("SEAMPASS_NO_DECAL_GUARD", "") in ("", "0")
DECAL_PLANE = 3.0  # a decal lies this close to its surface's plane (units)
_GBI = None


def is_decal_mode(c):
    """whether a render mode command sets the decal depth mode"""
    global _GBI
    n = mp.name_of(c)
    if n not in ("gsDPSetRenderMode", "gsDPSetOtherMode"):
        return False
    a = mp.args_of(c)
    expr = " | ".join("(%s)" % x for x in a) if n == "gsDPSetRenderMode" else "(%s)" % a[-1]
    try:
        import strippass as sp
        if _GBI is None:
            _GBI = sp._Gbi()
        v = eval(expr.replace("\n", " "), {}, _GBI)
        if v is not None:
            return (v & 0xC00) == 0xC00
    except Exception:
        pass
    return "DEC" in c


BUFFER_READERS = {"gsSPModifyVertex", "gsSPBranchLessZ", "gsSPBranchLessZraw", "gsSPBranchLessZrg",
                  "gsSPTriSnake", "gsSPTriStrip", "gsSPTriFan", "gsSPLineW3D", "gsSPLine3D"}


def pos(v):
    return (int(v[0], 0), int(v[1], 0), int(v[2], 0))


# ---- reading a scene's rooms ----

class Leaf:
    """a display list with vertex loads and triangles: its commands, and each triangle's three vertices (as their
    argument strings) with the array slots they came from"""

    def __init__(self, dirpath, name, room):
        self.dir, self.name, self.room = dirpath, name, room
        self.path = os.path.join(dirpath, name + ".inc.c")
        self.cmds = mp.split_items(open(self.path).read())
        self.tris = []          # (command index, which of its triangles, ((array, index) x3))
        self.ok = True
        self.why = ""
        self.decal = False


def read_scene(dirpath, arrays):
    """the scene's leaf lists (by room) and the vertex arrays they load (name -> list of VTX argument tuples)"""
    leaves = []
    for f in sorted(os.listdir(dirpath)):
        m = re.match(r"(.+_room_(\d+)_.*_DL)\.inc\.c$", f)
        if not m:
            continue
        name, room = m.group(1), m.group(2)
        text = open(os.path.join(dirpath, f)).read()
        if "gsSPVertex" not in text:
            continue
        lf = Leaf(dirpath, name, room)
        buf = [None] * 64
        for ci, c in enumerate(lf.cmds):
            if "(" not in c:
                continue
            n = mp.name_of(c)
            if n in BUFFER_READERS:
                lf.ok, lf.why = False, n
            if is_decal_mode(c):
                lf.decal = True
                lf.ok, lf.why = False, "decal"
            if n == "gsDPSetRenderMode" and ("XLU" in c or "DECAL" in c):
                # (translucent: a sliver where a split triangle's pieces overlap is drawn twice, darker; a decal is
                # laid over another surface on purpose. Seams there don't show: left as they are)
                lf.ok, lf.why = False, "translucent or decal"
            if n == "gsSPVertex":
                a = mp.args_of(c)
                m2 = mp.VTX_REF.match(a[0])
                arr, idx = m2.group(1), int(m2.group(2) or 0)
                if arr not in arrays:
                    p = os.path.join(dirpath, arr + ".inc.c")
                    arrays[arr] = mp.load_vtx(dirpath, arr) if os.path.exists(p) else None
                if arrays[arr] is None:
                    lf.ok, lf.why = False, "an array not in the scene"
                    continue
                cnt, v0 = int(a[1], 0), int(a[2], 0)
                for k in range(cnt):
                    buf[v0 + k] = (arr, idx + k)
            elif n in ("gsSP1Triangle", "gsSP2Triangles", "gsSP1Quadrangle"):
                a = [int(x, 0) for x in mp.args_of(c)]
                sets = [a[0:3]] if n == "gsSP1Triangle" else (
                    [a[0:3], a[4:7]] if n == "gsSP2Triangles" else [[a[0], a[1], a[2]], [a[0], a[2], a[3]]])
                for k, s in enumerate(sets):
                    t = tuple(buf[i] for i in s)
                    if None in t:
                        lf.ok, lf.why = False, "a triangle before its load"
                    else:
                        lf.tris.append((ci, k, t))
        leaves.append(lf)
    return leaves


# ---- welding ----

def weld(leaves, arrays, frozen=()):
    """moves the near-miss corners together (in the arrays); returns how many corners moved. frozen: positions that
    stay where they are, and that nothing's moved onto (a decal's corners and its surface's)"""
    uses = collections.Counter()
    normals = collections.defaultdict(list)  # position -> the normals of its triangles
    for lf in leaves:
        for _, _, t in lf.tris:
            ps = [pos(arrays[a][i]) for a, i in t]
            for p in ps:
                uses[p] += 1
            u = [ps[1][k] - ps[0][k] for k in range(3)]
            w = [ps[2][k] - ps[0][k] for k in range(3)]
            n = (u[1] * w[2] - u[2] * w[1], u[2] * w[0] - u[0] * w[2], u[0] * w[1] - u[1] * w[0])
            ln = math.sqrt(sum(x * x for x in n))
            if ln > 0:
                for p in ps:
                    normals[p].append(tuple(x / ln for x in n))
    pts = list(uses)
    grid = collections.defaultdict(list)
    cell = 4
    for p in pts:
        grid[(p[0] // cell, p[1] // cell, p[2] // cell)].append(p)
    parent = {p: p for p in pts}

    def find(p):
        while parent[p] != p:
            parent[p] = parent[parent[p]]
            p = parent[p]
        return p

    def along_normals(p, q):
        d = [q[k] - p[k] for k in range(3)]
        ln = math.sqrt(sum(x * x for x in d))
        for np_ in normals[p]:
            for nq in normals[q]:
                if abs(sum(np_[k] * nq[k] for k in range(3))) > 0.95 and \
                        abs(sum(np_[k] * d[k] for k in range(3))) / ln > DECAL_COS:
                    return True
        return False

    for p in pts:
        g = (p[0] // cell, p[1] // cell, p[2] // cell)
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                for dz in (-1, 0, 1):
                    for q in grid[(g[0] + dx, g[1] + dy, g[2] + dz)]:
                        if q <= p or math.dist(p, q) > WELD or along_normals(p, q):
                            continue
                        if p in frozen or q in frozen:
                            continue
                        parent[find(q)] = find(p)
    groups = collections.defaultdict(list)
    for p in pts:
        groups[find(p)].append(p)
    target = {}
    for g in groups.values():
        if len(g) < 2:
            continue
        best = max(g, key=lambda p: (uses[p], -p[0], -p[1], -p[2]))
        for p in g:
            if p != best:
                target[p] = best
    # never two corners of one triangle onto one point
    for lf in leaves:
        for _, _, t in lf.tris:
            ps = [pos(arrays[a][i]) for a, i in t]
            moved = [target.get(p, p) for p in ps]
            if len(set(moved)) < 3:
                for p in ps:
                    target.pop(p, None)
    n = 0
    for arr, vs in arrays.items():
        if vs is None:
            continue
        for i, v in enumerate(vs):
            p = pos(v)
            if p in target:
                q = target[p]
                vs[i] = (str(q[0]), str(q[1]), str(q[2])) + tuple(v[3:])
                n += 1
    return n, target


# ---- open-edge gaps ----

OPEN_GAP = 1.5  # two surfaces' open edges side by side this close (units) were meant to meet


def close_gaps(leaves, arrays, frozen=()):
    """two surfaces' borders running side by side a hair apart (their corners not lining up, so neither a weld nor a
    T-junction): each corner of the shorter border moved onto the longer one's line (the T-junction step then splits
    the longer at it). Not a decal's border over the surface it's laid on (the gap along both normals). Returns the
    corners moved."""
    ec = collections.Counter()
    normals = collections.defaultdict(list)
    tri_pts = []
    for lf in leaves:
        for _, _, t in lf.tris:
            ps = [pos(arrays[a][i]) for a, i in t]
            if len(set(ps)) < 3:
                continue
            tri_pts.append(ps)
            u = [ps[1][k] - ps[0][k] for k in range(3)]
            w = [ps[2][k] - ps[0][k] for k in range(3)]
            n = (u[1] * w[2] - u[2] * w[1], u[2] * w[0] - u[0] * w[2], u[0] * w[1] - u[1] * w[0])
            ln = math.sqrt(sum(x * x for x in n))
            for k in range(3):
                a, b = ps[k], ps[(k + 1) % 3]
                e = (min(a, b), max(a, b))
                ec[e] += 1
                if ln > 0:
                    normals[e].append(tuple(x / ln for x in n))
    edges = [e for e, c in ec.items() if c == 1 and math.dist(*e) >= 4]
    grid = collections.defaultdict(list)
    cell = 32
    for i, (a, b) in enumerate(edges):
        for p in (a, b, tuple((a[j] + b[j]) / 2 for j in range(3))):
            grid[tuple(int(p[j] // cell) for j in range(3))].append(i)
    target = {}
    for i, (a, b) in enumerate(edges):
        L = math.dist(a, b)
        u = [(b[j] - a[j]) / L for j in range(3)]
        cand = set()
        for p in (a, b, tuple((a[j] + b[j]) / 2 for j in range(3))):
            g = tuple(int(p[j] // cell) for j in range(3))
            for dx in (-1, 0, 1):
                for dy in (-1, 0, 1):
                    for dz in (-1, 0, 1):
                        cand.update(grid.get((g[0] + dx, g[1] + dy, g[2] + dz), ()))
        for k in cand:
            if k == i:
                continue
            c, d = edges[k]
            L2 = math.dist(c, d)
            if L2 > L or (L2 == L and k < i):
                continue  # (the shorter border's corners go onto the longer's line)
            w = [(d[j] - c[j]) / L2 for j in range(3)]
            if abs(sum(u[j] * w[j] for j in range(3))) < 0.995:
                continue
            for p in (c, d):
                if p in (a, b):
                    continue
                f = sum((p[j] - a[j]) * u[j] for j in range(3)) / L
                if f < -0.001 or f > 1.001:
                    continue
                q = [a[j] + f * L * u[j] for j in range(3)]
                gap = math.dist(p, q)
                if gap < 0.3 or gap > OPEN_GAP:
                    continue
                g = [(q[j] - p[j]) / gap for j in range(3)]
                layer = any(abs(sum(n1[j] * n2[j] for j in range(3))) > 0.95 and
                            abs(sum(n1[j] * g[j] for j in range(3))) > DECAL_COS
                            for n1 in normals[(a, b)] for n2 in normals[(c, d)])
                if layer or p in frozen:
                    continue
                target.setdefault(p, tuple(int(round(x)) for x in q))
    # never two corners of one triangle onto one point
    for ps in tri_pts:
        moved = [target.get(p, p) for p in ps]
        if len(set(moved)) < 3:
            for p in ps:
                target.pop(p, None)
    n = 0
    for arr, vs in arrays.items():
        if vs is None:
            continue
        for i, v in enumerate(vs):
            p = pos(v)
            if p in target and target[p] != p:
                q = target[p]
                vs[i] = (str(q[0]), str(q[1]), str(q[2])) + tuple(v[3:])
                n += 1
    return n


# ---- T-junctions ----

def layered(leaves, arrays):
    """(leaf name, triangle number) of the triangles with another lying on them in the same plane (a surface drawn
    twice, a second layer on the first: which one shows is decided by equal depths, and a split one's depths round a
    little differently, so the other layer would show through in places: the castle courtyard's darker floor)"""
    tris = []
    for lf in leaves:
        for ti, (_, _, t) in enumerate(lf.tris):
            ps = [pos(arrays[a][i]) for a, i in t]
            u = [ps[1][k] - ps[0][k] for k in range(3)]
            w = [ps[2][k] - ps[0][k] for k in range(3)]
            n = (u[1] * w[2] - u[2] * w[1], u[2] * w[0] - u[0] * w[2], u[0] * w[1] - u[1] * w[0])
            ln = math.sqrt(sum(x * x for x in n))
            if ln == 0:
                continue
            n = tuple(x / ln for x in n)
            tris.append(((lf.name, ti), ps, n, sum(n[k] * ps[0][k] for k in range(3))))
    grid = collections.defaultdict(list)
    cell = 64
    for i, (_, ps, _, _) in enumerate(tris):
        c = [sum(p[k] for p in ps) / 3 for k in range(3)]
        grid[tuple(int(c[k] // cell) for k in range(3))].append(i)

    def inside(p, ps, n):
        # p in the triangle ps (in its plane), by the signs of the edges' cross products along the normal
        s = []
        for k in range(3):
            a, b = ps[k], ps[(k + 1) % 3]
            e = [b[j] - a[j] for j in range(3)]
            q = [p[j] - a[j] for j in range(3)]
            c = (e[1] * q[2] - e[2] * q[1], e[2] * q[0] - e[0] * q[2], e[0] * q[1] - e[1] * q[0])
            s.append(sum(c[j] * n[j] for j in range(3)))
        return all(x >= -1e-6 for x in s) or all(x <= 1e-6 for x in s)

    out = set()
    for i, (key, ps, n, d) in enumerate(tris):
        c = [sum(p[k] for p in ps) / 3 for k in range(3)]
        g = tuple(int(c[k] // cell) for k in range(3))
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                for dz in (-1, 0, 1):
                    for j in grid.get((g[0] + dx, g[1] + dy, g[2] + dz), ()):
                        if j == i:
                            continue
                        key2, ps2, n2, d2 = tris[j]
                        if abs(sum(n[k] * n2[k] for k in range(3))) < 0.999:
                            continue
                        if abs(sum(n[k] * ps2[0][k] for k in range(3)) - d) > 1.0:
                            continue
                        c2 = [sum(p[k] for p in ps2) / 3 for k in range(3)]
                        if inside(c2, ps, n) or inside(c, ps2, n):
                            out.add(key)
                            out.add(key2)
    return out


def under_decals(decals, leaves, arrays):
    """the surfaces decals lie on: ({(leaf name, triangle number)} of `leaves`' triangles a decal triangle overlaps (in
    the same plane within DECAL_PLANE, their areas overlapping), {positions of those triangles' corners and of the
    decals' own})"""
    def plane(ps):
        u = [ps[1][k] - ps[0][k] for k in range(3)]
        w = [ps[2][k] - ps[0][k] for k in range(3)]
        n = (u[1] * w[2] - u[2] * w[1], u[2] * w[0] - u[0] * w[2], u[0] * w[1] - u[1] * w[0])
        ln = math.sqrt(sum(x * x for x in n))
        return None if ln == 0 else tuple(x / ln for x in n)

    def inside(p, ps, n):
        s = []
        for k in range(3):
            a, b = ps[k], ps[(k + 1) % 3]
            e = [b[j] - a[j] for j in range(3)]
            q = [p[j] - a[j] for j in range(3)]
            c = (e[1] * q[2] - e[2] * q[1], e[2] * q[0] - e[0] * q[2], e[0] * q[1] - e[1] * q[0])
            s.append(sum(c[j] * n[j] for j in range(3)))
        return all(x >= -1e-6 for x in s) or all(x <= 1e-6 for x in s)

    def crosses(a, b, c, d, n):
        # the segments ab and cd cross (seen along n)
        def side(p, q, r):
            e = [q[j] - p[j] for j in range(3)]
            f = [r[j] - p[j] for j in range(3)]
            x = (e[1] * f[2] - e[2] * f[1], e[2] * f[0] - e[0] * f[2], e[0] * f[1] - e[1] * f[0])
            return sum(x[j] * n[j] for j in range(3))
        return side(a, b, c) * side(a, b, d) < 0 and side(c, d, a) * side(c, d, b) < 0

    keys, frozen = set(), set()
    dec = []
    for lf in decals:
        for _, _, t in lf.tris:
            if any(arrays.get(a) is None for a, _ in t):
                continue
            ps = [pos(arrays[a][i]) for a, i in t]
            frozen.update(ps)
            n = plane(ps)
            if n is not None:
                dec.append((ps, n))
    if not dec:
        return keys, frozen
    cell = 128
    grid = collections.defaultdict(list)
    for i, (ps, n) in enumerate(dec):
        lo = [min(p[k] for p in ps) for k in range(3)]
        hi = [max(p[k] for p in ps) for k in range(3)]
        for x in range(int(lo[0] // cell), int(hi[0] // cell) + 1):
            for y in range(int(lo[1] // cell), int(hi[1] // cell) + 1):
                for z in range(int(lo[2] // cell), int(hi[2] // cell) + 1):
                    grid[(x, y, z)].append(i)
    for lf in leaves:
        for ti, (_, _, t) in enumerate(lf.tris):
            ps = [pos(arrays[a][i]) for a, i in t]
            n = plane(ps)
            if n is None:
                continue
            lo = [min(p[k] for p in ps) - DECAL_PLANE for k in range(3)]
            hi = [max(p[k] for p in ps) + DECAL_PLANE for k in range(3)]
            cand = set()
            for x in range(int(lo[0] // cell), int(hi[0] // cell) + 1):
                for y in range(int(lo[1] // cell), int(hi[1] // cell) + 1):
                    for z in range(int(lo[2] // cell), int(hi[2] // cell) + 1):
                        cand.update(grid.get((x, y, z), ()))
            d = sum(n[k] * ps[0][k] for k in range(3))
            for j in cand:
                ps2, n2 = dec[j]
                if abs(sum(n[k] * n2[k] for k in range(3))) < 0.99:
                    continue
                if max(abs(sum(n[k] * p[k] for k in range(3)) - d) for p in ps2) > DECAL_PLANE:
                    continue
                c1 = [sum(p[k] for p in ps) / 3 for k in range(3)]
                c2 = [sum(p[k] for p in ps2) / 3 for k in range(3)]
                if (inside(c2, ps, n) or inside(c1, ps2, n) or any(inside(p, ps, n) for p in ps2) or
                        any(inside(p, ps2, n) for p in ps) or
                        any(crosses(ps[a], ps[(a + 1) % 3], ps2[b], ps2[(b + 1) % 3], n)
                            for a in range(3) for b in range(3))):
                    keys.add((lf.name, ti))
                    frozen.update(ps)
                    break
    return keys, frozen


def t_junctions(leaves, arrays):
    """{(leaf name, triangle number): [(edge k, points along it: (fraction, position))]} for the triangles to split"""
    pts_lines = collections.defaultdict(set)  # position -> directions of the edges through it (unit, signless)
    allpts = set()
    edges = []
    for lf in leaves:
        for ti, (_, _, t) in enumerate(lf.tris):
            ps = [pos(arrays[a][i]) for a, i in t]
            allpts.update(ps)
            for k in range(3):
                a, b = ps[k], ps[(k + 1) % 3]
                d = [b[j] - a[j] for j in range(3)]
                ln = math.sqrt(sum(x * x for x in d))
                if ln == 0:
                    continue
                u = tuple(x / ln for x in d)
                if u < tuple(-x for x in u):
                    u = tuple(-x for x in u)
                pts_lines[a].add(u)
                pts_lines[b].add(u)
                edges.append((lf.name, ti, k, a, b, ln, u))
    grid = collections.defaultdict(list)
    cell = 16
    for p in allpts:
        grid[(p[0] // cell, p[1] // cell, p[2] // cell)].append(p)
    out = collections.defaultdict(list)
    for name, ti, k, a, b, ln, u in edges:
        if ln < 4 or ln > MAX_SPLIT_EDGE:
            continue
        lo = [min(a[j], b[j]) - 1 for j in range(3)]
        hi = [max(a[j], b[j]) + 1 for j in range(3)]
        found = []
        for gx in range(int(lo[0] // cell), int(hi[0] // cell) + 1):
            for gy in range(int(lo[1] // cell), int(hi[1] // cell) + 1):
                for gz in range(int(lo[2] // cell), int(hi[2] // cell) + 1):
                    for p in grid.get((gx, gy, gz), ()):
                        if p == a or p == b:
                            continue
                        f = sum((p[j] - a[j]) * (b[j] - a[j]) for j in range(3)) / (ln * ln)
                        if f <= END_FRAC or f >= 1 - END_FRAC:
                            continue
                        c = [a[j] + f * (b[j] - a[j]) for j in range(3)]
                        if math.dist(p, c) > ON_LINE:
                            continue
                        # a seam: an edge through p runs along this one's line
                        if not any(abs(sum(u[j] * w[j] for j in range(3))) > 0.995 for w in pts_lines[p]):
                            continue
                        found.append((f, p))
        if found:
            out[(name, ti)].append((k, sorted(found)))
    return out


def split_tri(vtxs, splits, lit):
    """the triangle (three VTX tuples) as its pieces, its edges' points (edge k: [(fraction, position)]) put in"""
    # the outline: each corner, then the points along the edge from it
    ring = []
    for k in range(3):
        ring.append(vtxs[k])
        for f, p in dict(splits).get(k, []):
            v = mp.lerp_vtx(vtxs[k], vtxs[(k + 1) % 3], f, lit)
            v = (str(p[0]), str(p[1]), str(p[2])) + tuple(v[3:])  # (exactly on the corner it meets)
            ring.append(v)
    # a fan from the corner with the most points on the opposite side: from a corner with none on its own edges
    best = 0
    for k in range(3):
        if not dict(splits).get(k) and not dict(splits).get((k + 2) % 3):
            best = ring.index(vtxs[k])
            break
    else:
        # points on every edge: fan from the outline's centre (a vertex of its own)
        c = tuple(str(round(sum(int(v[j], 0) for v in vtxs) / 3)) for j in range(3))
        c = c + tuple(mp.lerp_vtx(mp.lerp_vtx(vtxs[0], vtxs[1], 0.5, lit), vtxs[2], 1 / 3, lit)[3:])
        return [(c, ring[i], ring[(i + 1) % len(ring)]) for i in range(len(ring))]
    r = ring[best:] + ring[:best]
    return [(r[0], r[i], r[i + 1]) for i in range(1, len(r) - 1)]


def lit_state(lf):
    """each command's G_LIGHTING as the list sets it (None until it does: then by the vertices, see lerp)"""
    lit, out = None, []
    for c in lf.cmds:
        if "(" in c:
            n = mp.name_of(c)
            if n in ("gsSPSetGeometryMode", "gsSPClearGeometryMode", "gsSPLoadGeometryMode") and \
                    "G_LIGHTING" in [x.strip() for x in mp.args_of(c)[0].split("|")]:
                lit = n != "gsSPClearGeometryMode"
        out.append(lit)
    return out


# commands that change what a vertex load computes (its lighting, fog, texture coordinates, transform): a load moved
# past one of them would compute its vertices differently (the game sometimes loads with lighting on and draws with it
# off: the floor of the castle courtyard keeps that light)
LOAD_STATE = {"gsSPSetGeometryMode", "gsSPClearGeometryMode", "gsSPLoadGeometryMode", "gsSPGeometryMode",
              "gsSPTexture", "gsSPMatrix", "gsSPPopMatrix", "gsSPSetLights0", "gsSPSetLights1", "gsSPSetLights2",
              "gsSPSetLights3", "gsSPSetLights4", "gsSPSetLights5", "gsSPSetLights6", "gsSPSetLights7", "gsSPLight",
              "gsSPLightColor", "gsSPNumLights", "gsSPFogPosition", "gsSPFogFactor", "gsSPSegment", "gsSPDisplayList",
              "gsSPLookAt", "gsSPLookAtX", "gsSPLookAtY", "gsSPPerspNormalize", "gsSPViewport", "gsMoveWd", "gsMoveMem",
              "gsSPAmbOcclusion", "gsSPSetLights"}


def rewrite(lf, arrays, splits):
    """the list's commands again, its triangles from a vertex array of its own (the split ones as their pieces), each
    original load's triangles loaded where that load was (under the same state); None if a load's triangles, split,
    need more than one load and a command between them changes what a load computes"""
    lits = lit_state(lf)
    tri_at = collections.defaultdict(list)
    for ti, (ci, k, t) in enumerate(lf.tris):
        tri_at[ci].append(ti)
    vname = lf.name + "_seamVtx"
    vtx, out = [], []
    cmds = lf.cmds
    cull_box = set()
    for ci, c in enumerate(cmds):
        if "(" in c and mp.name_of(c) == "gsSPCullDisplayList" and ci > 0 and mp.name_of(cmds[ci - 1]) == "gsSPVertex":
            cull_box.add(ci - 1)
    # the segments: from each load to the next (its triangles and the commands between them)
    segs = []
    cur = None
    for ci, c in enumerate(cmds):
        n = mp.name_of(c) if "(" in c else c
        if n == "gsSPVertex" and ci not in cull_box:
            cur = {"items": [], "lit": bool(lits[ci])}  # (a split corner's colour or normal: as the load reads it)
            segs.append(cur)
            continue
        if cur is None or ci in cull_box:
            out.append(c) if cur is None else cur["items"].append(("cmd", c))
            continue
        if n in ("gsSP1Triangle", "gsSP2Triangles", "gsSP1Quadrangle"):
            for ti in tri_at.get(ci, []):
                _, _, t = lf.tris[ti]
                vs = tuple(arrays[a][i] for a, i in t)
                sp_ = splits.get((lf.name, ti))
                if cur["lit"]:
                    sp_ = None  # (lit: a split corner would be lit by its own normal where the game blended the light
                    #             between the corners: the Ice Cavern's walls came out darker; left as it was)
                pieces = split_tri(vs, sp_, cur["lit"]) if sp_ else [vs]
                for pc in pieces:
                    cur["items"].append(("tri", pc))
            continue
        cur["items"].append(("cmd", c))
    for seg in segs:
        tris = [x[1] for x in seg["items"] if x[0] == "tri"]
        uniq = list(dict.fromkeys(v for t in tris for v in t))
        if len(uniq) <= VTX_SLOTS:
            # one load, where the original was; the commands and triangles after it as they were
            start = len(vtx)
            vtx.extend(uniq)
            if uniq:
                out.append("gsSPVertex(&%s[%d], %d, 0)" % (vname, start, len(uniq)))
            pend = []

            def flush():
                k = 0
                while k < len(pend):
                    if k + 1 < len(pend):
                        out.append("gsSP2Triangles(%d, %d, %d, 0, %d, %d, %d, 0)" % (pend[k] + pend[k + 1]))
                        k += 2
                    else:
                        out.append("gsSP1Triangle(%d, %d, %d, 0)" % pend[k])
                        k += 1
                pend.clear()
            for kind, x in seg["items"]:
                if kind == "tri":
                    pend.append(tuple(uniq.index(v) for v in x))
                else:
                    flush()
                    out.append(x)
            flush()
        else:
            # more than one load: only if nothing between them changes what a load computes
            if any(kind == "cmd" and "(" in x and mp.name_of(x) in LOAD_STATE for kind, x in seg["items"]):
                return None
            pend = []
            for kind, x in seg["items"]:
                if kind == "tri":
                    pend.append((None, x))
                else:
                    mp.emit_tris(pend, vname, vtx, out)
                    pend = []
                    out.append(x)
            mp.emit_tris(pend, vname, vtx, out)
    return out, vtx


# ---- the pass ----

def scenes(tree):
    base = os.path.join(tree, "assets", "scenes")
    for group in sorted(os.listdir(base)):
        for s in sorted(os.listdir(os.path.join(base, group))):
            d = os.path.join(base, group, s)
            if os.path.isdir(d) and s not in SKIP_SCENES:
                yield os.path.join("assets", "scenes", group, s)


def process(dirpath, write):
    arrays = {}
    leaves = read_scene(dirpath, arrays)
    ok = [lf for lf in leaves if lf.ok]
    nweld = 0
    under, frozen = under_decals([lf for lf in leaves if lf.decal], ok, arrays) if DECAL_GUARD else (set(), set())
    if not os.environ.get("SEAMPASS_NO_WELD"):
        nweld, _ = weld(ok, arrays, frozen)
        nweld += close_gaps(ok, arrays, frozen)
    splits = {} if os.environ.get("SEAMPASS_NO_SPLIT") else t_junctions(ok, arrays)
    lay = layered(ok, arrays)
    splits = {k: v for k, v in splits.items() if k not in lay and k not in under}
    # only triangles whose new shape can't show: small ones (the near plane hardly ever cuts them) or ones whose corners
    # shade alike. A split triangle is drawn as new triangles from other first corners, and a cut one is fanned from
    # its first corner, its shading following where the fan starts (the Ice Cavern's wall by the camera, darker)
    by_name = {lf.name: lf for lf in ok}

    def keep(key):
        lf = by_name[key[0]]
        _, _, t = lf.tris[key[1]]
        vs = [arrays[a][i] for a, i in t]
        ps = [pos(v) for v in vs]
        small = all(math.dist(ps[k], ps[(k + 1) % 3]) <= SPLIT_SMALL for k in range(3))
        alike = all(max(int(v[c], 0) for v in vs) - min(int(v[c], 0) for v in vs) <= SPLIT_SHADE_TOL
                    for c in range(5, 9))
        return small or alike
    splits = {k: v for k, v in splits.items() if keep(k)}
    only = os.environ.get("SEAMPASS_SPLIT_ONLY")  # (a bisection's aid: split only in lists whose names hold one of these)
    if only:
        keys = only.split(",")
        splits = {k: v for k, v in splits.items() if any(x in k[0] for x in keys)}
    nsplit = len(splits)
    if not write:
        return nweld, nsplit, 0
    changed_arrays = set()
    # the welded arrays written back
    for arr, vs in arrays.items():
        if vs is None:
            continue
        orig = mp.load_vtx(dirpath, arr)
        if orig != vs:
            changed_arrays.add(arr)
            p = os.path.join(dirpath, arr + ".inc.c")
            os.remove(p)  # (a hard link: written fresh)
            open(p, "w").write("".join("    VTX(%s),\n" % ", ".join(v) for v in vs))
    new_arrays = collections.defaultdict(list)  # room file -> (array name, vertices)
    rewritten = 0
    for lf in ok:
        if not any(k[0] == lf.name for k in splits):
            continue
        res = rewrite(lf, arrays, splits)
        if res is None:
            continue  # (left unsplit: its loads couldn't stay where they were)
        cmds, vtx = res
        os.remove(lf.path)
        open(lf.path, "w").write("".join("    %s,\n" % c for c in cmds))
        room = re.match(r"(.+_room_\d+)_", lf.name).group(1)
        mp.unsize(dirpath, room, lf.name)
        vname = lf.name + "_seamVtx"
        open(os.path.join(dirpath, vname + ".inc.c"), "w").write(
            "".join("    VTX(%s),\n" % ", ".join(v) for v in vtx))
        new_arrays[room].append(vname)
        rewritten += 1
    rel = os.path.relpath(dirpath, TREE)
    for room, names in new_arrays.items():
        rp = os.path.join(dirpath, room + ".c")
        text = open(rp).read()
        os.remove(rp)
        first_def = re.search(r"^[A-Za-z_][\w ]*\*?\s+\w+\[\w*\]\s*=", text, re.M).start()
        ext = "".join("extern Vtx %s[];\n" % n for n in names)
        defs = "".join('Vtx %s[] = {\n#include "%s/%s.inc.c"\n};\n\n' % (n, rel, n) for n in names)
        text = text[:first_def] + "// fish-oot: tools/seampass.py's lists' vertices\n" + ext + "\n" + text[first_def:]
        text += "\n// fish-oot: tools/seampass.py's lists' vertices\n" + defs
        open(rp, "w").write(text)
    return nweld, nsplit, rewritten


TREE = None


def main():
    global TREE
    if len(sys.argv) >= 3 and sys.argv[1] == "--scan":
        TREE = sys.argv[2]
        for s in sys.argv[3:]:
            w, t, _ = process(os.path.join(TREE, s), False)
            print("%-45s %4d corners welded, %4d triangles split" % (s, w, t))
        return
    if len(sys.argv) >= 4 and sys.argv[1] == "--check":
        # the decals' triangles and the surfaces under them, before (SRC) and after (OUT): any changed
        src, out = sys.argv[2], sys.argv[3]
        ndec = nund = bad_dec = bad_und = 0
        for s in scenes(src):
            a0, a1 = {}, {}
            l0 = read_scene(os.path.join(src, s), a0)
            l1 = {lf.name: lf for lf in read_scene(os.path.join(out, s), a1)}
            dec = [lf for lf in l0 if lf.decal]
            if not dec:
                continue
            keys, _ = under_decals(dec, [lf for lf in l0 if not lf.decal and all(
                a0.get(a) is not None for _, _, t in lf.tris for a, _ in t)], a0)

            def tris(lf, arrays):
                return sorted(tuple(pos(arrays[a][i]) for a, i in t) for _, _, t in lf.tris
                              if all(arrays.get(a) is not None for a, _ in t))
            for lf in dec:
                ndec += len(lf.tris)
                if lf.name not in l1 or tris(lf, a0) != tris(l1[lf.name], a1):
                    bad_dec += len(lf.tris)
            by = {lf.name: lf for lf in l0}
            for name, ti in keys:
                nund += 1
                lf = by[name]
                want = tuple(pos(a0[a][i]) for a, i in lf.tris[ti][2])
                have = set(tuple(pos(a1[a][i]) for a, i in t) for _, _, t in l1[name].tris) if name in l1 else set()
                if want not in have:
                    bad_und += 1
        print("seampass --check: %d decal triangles, %d changed; %d triangles under decals, %d changed" % (
            ndec, bad_dec, nund, bad_und))
        sys.exit(1 if (bad_dec or bad_und) else 0)
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    src, out = sys.argv[1], sys.argv[2]
    report = "--report" in sys.argv
    if os.path.exists(out):
        shutil.rmtree(out)
    shutil.copytree(src, out, copy_function=os.link)
    TREE = out
    tw = ts = tr = 0
    for s in scenes(out):
        w, t, r = process(os.path.join(out, s), True)
        tw, ts, tr = tw + w, ts + t, tr + r
        if report and (w or t):
            print("seampass: %-45s %4d corners welded, %4d triangles split (%d lists)" % (s, w, t, r))
    print("seampass: %d corners welded, %d triangles split, %d lists rewritten" % (tw, ts, tr))


if __name__ == "__main__":
    main()
