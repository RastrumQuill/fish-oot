"""meshpack's merge pass: after the lake bed is cut along its depth lines (the water fog's bands, the caustics' steps),
the pieces merged back into as few triangles as the surface allows.

A vertex goes when nothing it carries is lost: the triangles around it lie in one plane, and its texture coordinates,
colour (or normal) and alpha are what the triangles left after it give at its place (within a fraction of a texel,
a few steps of colour). Inside one group (a material's state, a culling area, a fog band) its hole is triangulated
again; on the line between two groups (a depth line) it goes only where that line runs straight through it, from both
sides at once, so the two sides keep the same edge (no crack). A vertex on a mesh's border (an edge one triangle
uses: the piece's outline, a texture seam) stays: other pieces may meet it there.

The winding of every triangle is kept (the bed is drawn with back faces culled).
"""
import heapq

POS_TOL = 0.75     # units: how far the surface may move at a vertex that goes
ST_TOL = 8.0       # texture coordinates (s10.5: 32 a texel): a quarter of a texel
COL_TOL = 3.0      # colour steps (unlit)
NRM_TOL = 4.0      # normal steps (lit, s8)
A_TOL = 2.0        # alpha
COLLINEAR_TOL = 0.35
MIN_SIN = 0.02     # no new triangle thinner than this (the sine of its smallest angle)


def _p(v):
    return (float(int(v[0], 0)), float(int(v[1], 0)), float(int(v[2], 0)))


def _sb(x):
    x = int(x, 0) & 0xFF
    return x - 256 if x > 127 else x


def _attr(v, lit):
    a = [float(int(v[3], 0)), float(int(v[4], 0))]
    if lit:
        a += [float(_sb(x)) for x in v[5:8]]
    else:
        a += [float(int(x, 0) & 0xFF) for x in v[5:8]]
    a.append(float(int(v[8], 0) & 0xFF))
    return a


def _sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def _cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def _dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def _len(a):
    return _dot(a, a) ** 0.5


def _normal(t):
    return _cross(_sub(_p(t[1]), _p(t[0])), _sub(_p(t[2]), _p(t[0])))


def _min_sin(t):
    ps = [_p(v) for v in t]
    best = 1.0
    for i in range(3):
        a, b, c = ps[i], ps[(i + 1) % 3], ps[(i + 2) % 3]
        u, w = _sub(b, a), _sub(c, a)
        lu, lw = _len(u), _len(w)
        if lu < 1e-9 or lw < 1e-9:
            return 0.0
        best = min(best, _len(_cross(u, w)) / (lu * lw))
    return best


def _triangulate(chain, closed, nrm):
    """ear clipping of the polygon chain (closed: a ring; else the open chain closed by its ends' edge), in the
    winding nrm gives; the ear with the widest smallest angle first. None when it can't."""
    poly = list(chain)
    out = []
    while len(poly) > 3:
        best, best_i = -1.0, -1
        n = len(poly)
        for i in range(n):
            a, b, c = poly[i - 1], poly[i], poly[(i + 1) % n]
            t = (a, b, c)
            nn = _normal(t)
            if _dot(nn, nrm) <= 0:
                continue  # (reflex there)
            pa, pb, pc = _p(a), _p(b), _p(c)
            inside = False
            for q in poly:
                if q in t:
                    continue
                pq = _p(q)
                if all(_dot(_cross(_sub(y, x), _sub(pq, x)), nrm) >= -1e-6
                       for x, y in ((pa, pb), (pb, pc), (pc, pa))):
                    inside = True
                    break
            if inside:
                continue
            s = _min_sin(t)
            if s > best:
                best, best_i = s, i
        if best_i < 0:
            return None
        n = len(poly)
        out.append((poly[best_i - 1], poly[best_i], poly[(best_i + 1) % n]))
        del poly[best_i]
    if _dot(_normal(tuple(poly)), nrm) <= 0:
        return None
    out.append(tuple(poly))
    return out


def _interp(v, tris, lit):
    """v's place on the triangles (the one it falls in, along their plane): (the distance off it, the attributes'
    largest error against v's own by kind) or None when it falls in none"""
    pv = _p(v)
    av = _attr(v, lit)
    for t in tris:
        ps = [_p(x) for x in t]
        n = _normal(t)
        ln = _len(n)
        if ln < 1e-9:
            continue
        n = (n[0] / ln, n[1] / ln, n[2] / ln)
        d = _dot(_sub(pv, ps[0]), n)
        q = (pv[0] - n[0] * d, pv[1] - n[1] * d, pv[2] - n[2] * d)
        w = []
        for i in range(3):
            a, b = ps[(i + 1) % 3], ps[(i + 2) % 3]
            w.append(_dot(_cross(_sub(b, a), _sub(q, a)), n) / ln)
        s = sum(w)
        if s <= 0:
            continue
        w = [x / s for x in w]
        if min(w) < -0.02:
            continue
        at = [_attr(x, lit) for x in t]
        e = [sum(w[i] * at[i][k] for i in range(3)) - av[k] for k in range(len(av))]
        return abs(d), e
    return None


def _ok(v, old, new, lit):
    """v can go: the new triangles keep the surface and what v carried"""
    if not new or any(_min_sin(t) < MIN_SIN for t in new):
        return False
    if abs(sum(_len(_normal(t)) for t in new) - sum(_len(_normal(t)) for t in old)) > \
            0.002 * sum(_len(_normal(t)) for t in old) + 2.0:
        return False  # (the area changed: folded or not flat)
    r = _interp(v, new, lit)
    if r is None:
        return False
    d, e = r
    if d > POS_TOL:
        return False
    tols = [ST_TOL, ST_TOL] + [NRM_TOL if lit else COL_TOL] * 3 + [A_TOL]
    if any(abs(e[k]) > tols[k] for k in range(len(e))):
        return False
    # every old triangle's corners on the new triangles' plane too (the hole flat, not only at v)
    for t in old:
        for x in t:
            if x is v:
                continue
            r = _interp(x, new, lit)
            if r is not None and r[0] > POS_TOL:
                return False
    return True


def merge(items, lit_of):
    """items: [(group, (v0, v1, v2), payload)]; lit_of(group) says whether the group's vertices carry normals.
    Returns the same with vertices removed where they can be, and how many triangles went."""
    tris = {}
    for i, (g, t, pay) in enumerate(items):
        tris[i] = (g, tuple(t), pay)
    at = {}
    for i, (g, t, _) in tris.items():
        for v in t:
            at.setdefault(v, set()).add(i)
    nid = [len(items)]
    before = len(tris)

    def edges_of(i):
        t = tris[i][1]
        return [(t[0], t[1]), (t[1], t[2]), (t[2], t[0])]

    def try_remove(v):
        ids = list(at.get(v, ()))
        if len(ids) < 3:
            return None
        groups = {tris[i][0] for i in ids}
        if len(groups) > 2:
            return None
        lit = lit_of(next(iter(groups)))
        if any(lit_of(g) != lit for g in groups):
            return None
        # the fan around v: each triangle (v, a, b) in its winding gives the edge a -> b
        nxt = {}
        for i in ids:
            t = tris[i][1]
            k = t.index(v)
            a, b = t[(k + 1) % 3], t[(k + 2) % 3]
            if a in nxt:
                return None
            nxt[a] = (b, i)
        # closed ring (v inside the mesh): every a's b is some other a
        if any(b not in nxt for b, _ in nxt.values()):
            return None  # (v on a border: kept)
        start = next(iter(nxt))
        ring, ring_ids, cur = [], [], start
        for _ in range(len(nxt)):
            ring.append(cur)
            b, i = nxt[cur]
            ring_ids.append(i)
            cur = b
        if cur != start or len(set(ring)) != len(ring):
            return None
        old = [tris[i][1] for i in ids]
        nrm = (0.0, 0.0, 0.0)
        for t in old:
            n = _normal(t)
            nrm = (nrm[0] + n[0], nrm[1] + n[1], nrm[2] + n[2])
        if len(groups) == 1:
            new = _triangulate(ring, True, nrm)
            if new is None or not _ok(v, old, new, lit):
                return None
            g = tris[ids[0]][0]
            pay = tris[ids[0]][2]
            return ids, [(g, t, pay) for t in new]
        # two groups: the ring's triangles in two runs, split at a and b (the line between them through v)
        gs = [tris[i][0] for i in ring_ids]
        cuts = [j for j in range(len(ring)) if gs[j] != gs[j - 1]]
        if len(cuts) != 2:
            return None
        a, b = ring[cuts[0]], ring[cuts[1]]
        pa, pb, pv = _p(a), _p(b), _p(v)
        ab = _sub(pb, pa)
        lab = _len(ab)
        if lab < 1e-9:
            return None
        if _len(_cross(ab, _sub(pv, pa))) / lab > COLLINEAR_TOL:
            return None
        f = _dot(_sub(pv, pa), ab) / (lab * lab)
        if not (0.0 < f < 1.0):
            return None
        # (v's attributes on the line: what a and b give there, so the line itself loses nothing)
        aa, ba, va = _attr(a, lit), _attr(b, lit), _attr(v, lit)
        tols = [ST_TOL, ST_TOL] + [NRM_TOL if lit else COL_TOL] * 3 + [A_TOL]
        if any(abs(aa[k] + (ba[k] - aa[k]) * f - va[k]) > tols[k] for k in range(len(va))):
            return None
        out = []
        for c0, c1 in ((cuts[0], cuts[1]), (cuts[1], cuts[0] + len(ring))):
            side = [ring[j % len(ring)] for j in range(c0, c1 + 1)]   # a ... b (or b ... a)
            side_ids = [ring_ids[j % len(ring)] for j in range(c0, c1)]
            sold = [tris[i][1] for i in side_ids]
            snrm = (0.0, 0.0, 0.0)
            for t in sold:
                n = _normal(t)
                snrm = (snrm[0] + n[0], snrm[1] + n[1], snrm[2] + n[2])
            if len(side) < 3:
                return None
            new = _triangulate(side, False, snrm) if len(side) > 3 else [tuple(side)]
            if new is None or _dot(_normal(new[0]) if len(new) == 1 else snrm, snrm) <= 0:
                return None
            if not _ok(v, sold, new, lit):
                return None
            g = tris[side_ids[0]][0]
            pay = tris[side_ids[0]][2]
            out += [(g, t, pay) for t in new]
        return ids, out

    # greedy: every vertex tried; a removal changes its neighbours, which are tried again
    queue = list(at.keys())
    queued = set(queue)
    while queue:
        v = queue.pop()
        queued.discard(v)
        if v not in at:
            continue
        r = try_remove(v)
        if r is None:
            continue
        ids, new = r
        touched = set()
        for i in ids:
            g, t, _ = tris.pop(i)
            for x in t:
                at[x].discard(i)
                touched.add(x)
        for g, t, pay in new:
            i = nid[0]
            nid[0] += 1
            tris[i] = (g, t, pay)
            for x in t:
                at.setdefault(x, set()).add(i)
        if not at[v]:
            del at[v]
        for x in touched:
            if x != v and x in at and x not in queued:
                queue.append(x)
                queued.add(x)
    out = [tris[i] for i in sorted(tris)]
    return out, before - len(out)
