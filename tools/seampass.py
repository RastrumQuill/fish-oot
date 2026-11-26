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
END_FRAC = 0.01   # ...and between its ends by this much of its length
DECAL_COS = 0.9   # a weld along both surfaces' normals (cosine) is a layer, not a crack
VTX_SLOTS = 32    # the vertex buffer the re-emitted lists load into (F3DEX2's; F3DEX3 has more)
SKIP_SCENES = set()  # scene directories left alone (by their last path part)
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

def weld(leaves, arrays):
    """moves the near-miss corners together (in the arrays); returns how many corners moved"""
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


def close_gaps(leaves, arrays):
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
                if layer:
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
        if ln < 4:
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


def rewrite(lf, arrays, splits):
    """the list's commands again, its triangles from a vertex array of its own (the split ones as their pieces);
    returns (commands, array) or None"""
    lits = lit_state(lf)
    tri_at = collections.defaultdict(list)
    for ti, (ci, k, t) in enumerate(lf.tris):
        tri_at[ci].append(ti)
    vname = lf.name + "_seamVtx"
    vtx, out, pending = [], [], []

    def flush():
        if pending:
            mp.emit_tris([(None, t) for t in pending], vname, vtx, out)
            pending.clear()

    cmds = lf.cmds
    skip_next_vertex = set()
    for ci, c in enumerate(cmds):
        n = mp.name_of(c) if "(" in c else c
        if n == "gsSPCullDisplayList" and ci > 0 and mp.name_of(cmds[ci - 1]) == "gsSPVertex":
            skip_next_vertex.add(ci - 1)
    for ci, c in enumerate(cmds):
        n = mp.name_of(c) if "(" in c else c
        if n == "gsSPVertex":
            if ci in skip_next_vertex:
                flush()
                out.append(c)  # (the cull's box: as it was)
            continue
        if n in ("gsSP1Triangle", "gsSP2Triangles", "gsSP1Quadrangle"):
            for ti in tri_at.get(ci, []):
                _, _, t = lf.tris[ti]
                vs = tuple(arrays[a][i] for a, i in t)
                sp = splits.get((lf.name, ti))
                if sp:
                    lit = lits[ci]
                    if lit is None:
                        lit = False
                    pending.extend(split_tri(vs, sp, lit))
                else:
                    pending.append(vs)
            continue
        if n == "gsSPCullDisplayList":
            out.append(c)
            continue
        flush()
        out.append(c)
    flush()
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
    nweld, _ = weld(ok, arrays)
    nweld += close_gaps(ok, arrays)
    splits = t_junctions(ok, arrays)
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
        cmds, vtx = rewrite(lf, arrays, splits)
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
