#!/usr/bin/env python3
"""fish-oot: a collision pass over the extracted scenes (build.sh --colpack): fewer triangles, the same surfaces.

Neighbouring collision triangles in exactly the same stored plane (normal and distance) with the same surface type
and flags are merged: the region's outline is traced from the triangles' own edges (so it keeps their winding), the
vertices left in the middle of its straight edges are dropped (an exact integer test), and it's cut again into the
fewest triangles (ear clipping, exact integer orientation tests). The region covers exactly the same area: every
floor height and wall in it is where it was; only which triangle answers changes, and those carry the same plane,
surface and flags. A region is kept as it is when merging doesn't save a triangle, or when it has a hole, a pinch
(a vertex where its outline touches itself) or mixed winding; each new region's area is checked against the old
(exact integers) before it's written.

The original tree isn't touched: the output is a copy (hard links for every file the pass doesn't change), which the
build then reads (EXTRACTED_DIR), like tools/texpack.py.

Usage: tools/colpack.py SRC_TREE OUT_TREE [--report]
"""
import collections
import os
import re
import shutil
import sys

POLY = re.compile(
    r"(?P<indent>[ \t]*)\{\s*(?P<type>\d+), // type\s*\{\s*"
    r"COLPOLY_VTX\((?P<a>\d+), (?P<fa>[^)]*)\), // 0\s*"
    r"COLPOLY_VTX\((?P<b>\d+), (?P<fb>[^)]*)\), // 1\s*"
    r"COLPOLY_VTX\((?P<c>\d+), (?P<fc>[^)]*)\), // 2\s*"
    r"\}, // vtxData\s*\{\s*"
    r"COLPOLY_SNORMAL\((?P<nx>[^)]*)\), // x\s*COLPOLY_SNORMAL\((?P<ny>[^)]*)\), // y\s*"
    r"COLPOLY_SNORMAL\((?P<nz>[^)]*)\), // z\s*\}, // normal\s*(?P<dist>-?\d+), // dist\s*\}, // \d+\n")
VTX = re.compile(r"\{\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\s*\}")


def cross(o, a, b):
    """z of (a - o) x (b - o), 2D integer points"""
    return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])


def collinear3(V, i, j, k):
    a, b, c = V[i], V[j], V[k]
    u = (b[0] - a[0], b[1] - a[1], b[2] - a[2])
    w = (c[0] - a[0], c[1] - a[1], c[2] - a[2])
    return (u[1] * w[2] - u[2] * w[1], u[2] * w[0] - u[0] * w[2], u[0] * w[1] - u[1] * w[0]) == (0, 0, 0)


def project(V, normal, tris):
    """2D coordinates dropping the normal's largest axis, mirrored if need be so most of `tris` wind
    counter-clockwise"""
    ax = max(range(3), key=lambda k: abs(normal[k]))
    keep = [k for k in range(3) if k != ax]

    def raw(i):
        v = V[i]
        return (v[keep[0]], v[keep[1]])
    turn = sum((cross(raw(a), raw(b), raw(c)) > 0) - (cross(raw(a), raw(b), raw(c)) < 0) for a, b, c in tris)
    if turn >= 0:
        return raw

    def mirrored(i):
        x, y = raw(i)
        return (x, -y)
    return mirrored


def ear_clip(loop, P):
    """triangles (index triples) for a simple polygon `loop` (counter-clockwise in P), or None"""
    pts = list(loop)
    tris = []
    guard = 0
    while len(pts) > 3:
        guard += 1
        if guard > 10000:
            return None
        n = len(pts)
        for i in range(n):
            a, b, c = pts[i - 1], pts[i], pts[(i + 1) % n]
            if cross(P(a), P(b), P(c)) <= 0:
                continue  # reflex or flat: not an ear
            inside = False
            for q in pts:
                if q in (a, b, c):
                    continue
                pq = P(q)
                if cross(P(a), P(b), pq) >= 0 and cross(P(b), P(c), pq) >= 0 and cross(P(c), P(a), pq) >= 0:
                    inside = True
                    break
            if not inside:
                tris.append((a, b, c))
                del pts[i]
                break
        else:
            return None
    if cross(P(pts[0]), P(pts[1]), P(pts[2])) <= 0:
        return None
    tris.append(tuple(pts))
    return tris


def same_cover(old, new, P, samples=400):
    """random points in the region's box: strictly inside some old triangle exactly when strictly inside some new one
    (points on an edge of either are skipped: edges are where the tests' own tolerance decides)"""
    import random
    rng = random.Random(len(old) * 7919 + len(new))
    pts = [P(i) for t in old for i in t]
    x0, x1 = min(p[0] for p in pts), max(p[0] for p in pts)
    y0, y1 = min(p[1] for p in pts), max(p[1] for p in pts)

    def where(tris, q):
        inside = False
        for a, b, c in tris:
            d = (cross(P(a), P(b), q), cross(P(b), P(c), q), cross(P(c), P(a), q))
            if 0 in d and all(v >= 0 for v in d):
                return None  # on an edge
            if all(v > 0 for v in d):
                inside = True
        return inside
    for _ in range(samples):
        q = (rng.uniform(x0, x1), rng.uniform(y0, y1))
        a, b = where(old, q), where(new, q)
        if a is not None and b is not None and a != b:
            return False
    return True


def area2(tris, P):
    return sum(cross(P(a), P(b), P(c)) for a, b, c in tris)


def merge_file(poly_text, vtx_text):
    V = [tuple(map(int, m.groups())) for m in VTX.finditer(vtx_text)]
    polys = list(POLY.finditer(poly_text))
    if not polys or "".join(m.group(0) for m in polys) != poly_text[polys[0].start():polys[-1].end()]:
        return None, 0, 0  # (a layout this pass doesn't know: left as it is)
    groups = collections.defaultdict(list)
    for i, m in enumerate(polys):
        key = (m["type"], m["fa"], m["fb"], m["fc"], m["nx"], m["ny"], m["nz"], m["dist"])
        groups[key].append(i)
    replace = {}  # first poly of a region -> its new triangles; the others of the region -> []
    saved = 0
    for key, ids in groups.items():
        normal = tuple(float(key[k]) for k in (4, 5, 6))
        if normal == (0.0, 0.0, 0.0):
            continue
        tri = {i: (int(polys[i]["a"]), int(polys[i]["b"]), int(polys[i]["c"])) for i in ids}
        P = project(V, normal, tri.values())
        # triangles that are degenerate in this projection stay as they are
        live = [i for i in ids if cross(P(tri[i][0]), P(tri[i][1]), P(tri[i][2])) > 0]
        edge_owner = collections.defaultdict(list)
        for i in live:
            a, b, c = tri[i]
            for e in ((a, b), (b, c), (c, a)):
                edge_owner[frozenset(e)].append(i)
        parent = {i: i for i in live}

        def find(x):
            while parent[x] != x:
                parent[x] = parent[parent[x]]
                x = parent[x]
            return x
        for owners in edge_owner.values():
            for o in owners[1:]:
                parent[find(o)] = find(owners[0])
        comps = collections.defaultdict(list)
        for i in live:
            comps[find(i)].append(i)
        for comp in comps.values():
            if len(comp) < 2:
                continue
            directed = collections.Counter()
            for i in comp:
                a, b, c = tri[i]
                for e in ((a, b), (b, c), (c, a)):
                    directed[e] += 1
            if any(n > 1 for n in directed.values()):
                continue  # mixed winding or a doubled triangle
            boundary = [e for e in directed if (e[1], e[0]) not in directed]
            nxt = {}
            ok = True
            for a, b in boundary:
                if a in nxt:
                    ok = False  # a pinch
                    break
                nxt[a] = b
            if not ok:
                continue
            start = boundary[0][0]
            loop = [start]
            cur = nxt[start]
            while cur != start and len(loop) <= len(boundary):
                loop.append(cur)
                cur = nxt.get(cur)
                if cur is None:
                    break
            if cur != start or len(loop) != len(boundary):
                continue  # more than one outline: a hole (or islands)
            # drop the vertices in the middle of straight edges (exactly on the line through their neighbours)
            changed = True
            while changed and len(loop) > 3:
                changed = False
                for j in range(len(loop)):
                    if collinear3(V, loop[j - 1], loop[j], loop[(j + 1) % len(loop)]):
                        del loop[j]
                        changed = True
                        break
            need = len(loop) - 2
            if need >= len(comp):
                continue
            new = ear_clip(loop, P)
            if new is None or len(new) >= len(comp):
                continue
            if area2(new, P) != area2([tri[i] for i in comp], P):
                continue  # (never expected: the guard against a bad outline)
            if not same_cover([tri[i] for i in comp], new, P):
                continue  # (likewise)
            first = min(comp)
            replace[first] = new
            for i in comp:
                if i != first:
                    replace[i] = []
            saved += len(comp) - len(new)
    if not replace:
        return None, len(polys), 0
    out = []
    n_out = 0
    for i, m in enumerate(polys):
        if i not in replace:
            text = m.group(0)
            out.append(re.sub(r"\}, // \d+\n$", f"}}, // {n_out}\n", text))
            n_out += 1
            continue
        for a, b, c in replace[i]:
            text = m.group(0)
            text = re.sub(r"COLPOLY_VTX\(\d+, ", lambda mm, it=iter((a, b, c)): f"COLPOLY_VTX({next(it)}, ", text)
            out.append(re.sub(r"\}, // \d+\n$", f"}}, // {n_out}\n", text))
            n_out += 1
    head = poly_text[:polys[0].start()]
    tail = poly_text[polys[-1].end():]
    return head + "".join(out) + tail, len(polys), saved


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    report = "--report" in sys.argv
    if len(args) != 2:
        sys.exit(__doc__)
    src, out = args
    changes = {}
    total = saved = 0
    for dp, _, files in os.walk(os.path.join(src, "assets")):
        for f in files:
            if not f.endswith("_PolyList.inc.c"):
                continue
            p = os.path.join(dp, f)
            vp = p.replace("_PolyList.inc.c", "_VtxList.inc.c")
            if not os.path.exists(vp):
                continue
            new, n, s = merge_file(open(p).read(), open(vp).read())
            total += n
            if new is not None and s:
                changes[os.path.relpath(p, src)] = new
                saved += s
    print(f"colpack: {saved} of {total} collision triangles merged away ({100 * saved / max(total, 1):.1f}%), "
          f"{len(changes)} files")
    if report:
        return
    if os.path.exists(out):
        shutil.rmtree(out)
    shutil.copytree(src, out, copy_function=os.link)
    for rel, text in changes.items():
        p = os.path.join(out, rel)
        os.unlink(p)
        with open(p, "w") as fh:
            fh.write(text)


if __name__ == "__main__":
    main()
