"""meshpack's remesh of the lake bed (20 Nov): the water fog's bands and the caustics' steps change along depth lines,
with few triangles.

The bed's fog (by band) and its caustics (by step) are set per triangle. Each whole triangle in the band of its middle
put the changes on triangle edges (seams); cutting the triangles at the depths put them on depth lines but made four
times the triangles. Here the bed is cut at every depth (the bands' tops, the steps' edges), and then decimated:

- **The near-flat bed under the water** (a triangle under the surface whose face points up more than BED_COS) gets its
  texture coordinates projected straight down (the same density and direction as the run's own, from its
  triangles), so the bed's patchwork of texture coordinates no longer splits it into separate pieces; copies of a
  vertex (one position, different coordinates) become one.
- **A vertex of it goes** when every point of the cut bed (all its corners, kept as samples) stays within POS_TOL of
  the surface left, and its shade (normal or colour) within SHADE_TOL. Inside one band its hole is filled again; on a
  depth line it goes only when both its neighbours along the line are on it too (at the same depth), so the line
  stays a depth line, filled again from both sides along the same edge (no crack).
- **What isn't the bed** (steep faces under water: the pit's walls, pillars; and everything over the water) keeps its
  vertices and coordinates. Its triangles under the water are cut at the same depths (so where it meets the bed the
  two share every vertex), and stay cut.

The runs (vertex loads and triangles between two state commands) are each done on their own, their state as it was.
"""
import collections

BED_COS = 0.15       # a face facing up at least this much (its normal's height share) is the bed
POS_TOL = 3.0        # units: how far the surface may move anywhere (the cut bed's corners as samples)
SHADE_TOL = 14.0     # a normal's (s8) or colour's steps
MIN_SIN = 0.004      # no new triangle thinner than this in the plan view (bands on gentle slopes are long and thin)
CELL = 64.0          # the samples' grid


def contours(water_y, depths):
    return sorted(set(depths))


def _i(x):
    return int(x, 0)


def band_of(tri, water_y, depths):
    d = water_y - sum(_i(v[1]) for v in tri) / 3.0
    b = -1
    for k, top in enumerate(depths):
        if d > top:
            b = k
    return b


def _area2_xz(a, b, c):
    return (_i(b[0]) - _i(a[0])) * (_i(c[2]) - _i(a[2])) - (_i(b[2]) - _i(a[2])) * (_i(c[0]) - _i(a[0]))


def _up_share(t):
    p = [[_i(v[k]) for k in range(3)] for v in t]
    u = [p[1][k] - p[0][k] for k in range(3)]
    w = [p[2][k] - p[0][k] for k in range(3)]
    n = (u[1] * w[2] - u[2] * w[1], u[2] * w[0] - u[0] * w[2], u[0] * w[1] - u[1] * w[0])
    ln = (n[0] ** 2 + n[1] ** 2 + n[2] ** 2) ** 0.5
    return (n[1] / ln) if ln > 0 else 0.0


def _sb(x):
    x = _i(x) & 0xFF
    return x - 256 if x > 127 else x


def cut_at(tri, water_y, depths, lit, clip_poly):
    """the triangle cut at each depth it crosses: [triangles] (each piece fanned: they're convex), its winding kept"""
    ys = [_i(v[1]) for v in tri]
    pieces = []
    poly = list(tri)
    for d in depths:
        y0 = water_y - d
        if min(_i(v[1]) for v in poly) < y0 < max(_i(v[1]) for v in poly):
            above, below = clip_poly(poly, y0, lit)
            if len(above) >= 3:
                pieces.append(above)
            poly = below
            if len(poly) < 3:
                break
    if len(poly) >= 3:
        pieces.append(poly)
    if len(pieces) == 1 and len(pieces[0]) == 3:
        return [tuple(tri)]
    out = []
    for pc in pieces:
        out += [(pc[0], pc[i], pc[i + 1]) for i in range(1, len(pc) - 1)]
    return [t for t in out if _area2_xz(*t) != 0 or True]


def _planar(tris):
    """the run's texture direction and density: each bed triangle's s and t as a function of x and z, the medians"""
    gs = [[], [], [], []]
    for t in tris:
        x = [_i(v[0]) for v in t]
        z = [_i(v[2]) for v in t]
        s = [_i(v[3]) for v in t]
        tt = [_i(v[4]) for v in t]
        a, b = x[1] - x[0], z[1] - z[0]
        c, d = x[2] - x[0], z[2] - z[0]
        det = a * d - b * c
        if abs(det) < 1e-6:
            continue
        for k, f in enumerate((s, tt)):
            df1, df2 = f[1] - f[0], f[2] - f[0]
            gx = (df1 * d - df2 * b) / det
            gz = (a * df2 - c * df1) / det
            gs[2 * k].append(gx)
            gs[2 * k + 1].append(gz)
    if not gs[0]:
        return None
    med = []
    for g in gs:
        g = sorted(g)
        med.append(g[len(g) // 2])
    return med


def remesh_run(tris, lit, water_y, depths, clip_poly, origin):
    """one run's triangles (vertex tuples, wound as drawn): the run again, the bed remeshed. origin: ORIGIN (a new
    vertex's source in the room). Returns (triangles, how many the bed had cut, how many it has)"""
    under = [t for t in tris if min(_i(v[1]) for v in t) < water_y]
    if not under:
        return tris, 0, 0
    keep = [t for t in tris if min(_i(v[1]) for v in t) >= water_y]
    bed_src = [t for t in under if _up_share(t) >= BED_COS]
    pl = _planar(bed_src)
    pieces = []
    for t in under:
        for p in cut_at(t, water_y, depths, lit, clip_poly):
            pieces.append(p)
    bed = [p for p in pieces if _up_share(p) >= BED_COS and pl is not None]
    other = [p for p in pieces if not (_up_share(p) >= BED_COS and pl is not None)]
    if not bed:
        return keep + other, 0, 0
    # the bed's vertices: one per position, texture coordinates projected straight down, the shade averaged
    sx, sz, tx, tz = pl
    xs = [_i(v[0]) for p in bed for v in p]
    zs = [_i(v[2]) for p in bed for v in p]
    mx, mz = sorted(xs)[len(xs) // 2], sorted(zs)[len(zs) // 2]
    so = -round((sx * mx + sz * mz) / 4096.0) * 4096
    to = -round((tx * mx + tz * mz) / 4096.0) * 4096
    by_pos = collections.defaultdict(list)
    for p in bed:
        for v in p:
            by_pos[(v[0], v[1], v[2])].append(v)
    canon = {}
    for pos, vs in by_pos.items():
        x, z = _i(pos[0]), _i(pos[2])
        s = int(round(sx * x + sz * z + so))
        t = int(round(tx * x + tz * z + to))
        uniq = list(dict.fromkeys(vs))
        if lit:
            n = [sum(_sb(v[5 + k]) for v in uniq) / len(uniq) for k in range(3)]
            ln = max(1e-6, sum(c * c for c in n) ** 0.5)
            want = max(sum(_sb(v[5 + k]) ** 2 for k in range(3)) ** 0.5 for v in uniq)
            sh = [str(int(round(c * want / ln)) & 0xFF) for c in n]
        else:
            sh = [str(int(round(sum(_i(v[5 + k]) & 0xFF for v in uniq) / len(uniq)))) for k in range(3)]
        a = str(int(round(sum(_i(v[8]) & 0xFF for v in uniq) / len(uniq))))
        nv = (pos[0], pos[1], pos[2], str(s), str(t), sh[0], sh[1], sh[2], a)
        origin.setdefault(nv, origin.get(uniq[0]))
        canon[pos] = nv
    bed = [tuple(canon[(v[0], v[1], v[2])] for v in p) for p in bed]
    ncut = len(bed)
    bed = _decimate(bed, lit, water_y, depths)
    return keep + other + bed, ncut, len(bed)


def _shade(v, lit):
    return [float(_sb(x)) if lit else float(_i(x) & 0xFF) for x in v[5:8]]


def _decimate(bed, lit, water_y, depths):
    tris = {i: (band_of(t, water_y, depths), t) for i, t in enumerate(bed)}
    at = collections.defaultdict(set)
    for i, (g, t) in tris.items():
        for v in t:
            at[v].add(i)
    nid = [len(bed)]
    sign = 1 if sum(_area2_xz(*t) for t in bed) > 0 else -1
    # the samples: every corner of the cut bed (position, shade), by cell
    grid = collections.defaultdict(list)
    for v in at:
        x, z = _i(v[0]), _i(v[2])
        grid[(int(x // CELL), int(z // CELL))].append((x, z, _i(v[1]), _shade(v, lit)))

    def xz(v):
        return (_i(v[0]), _i(v[2]))

    def min_sin(t):
        p = [xz(v) for v in t]
        best = 1.0
        for k in range(3):
            a, b, c = p[k], p[(k + 1) % 3], p[(k + 2) % 3]
            u = (b[0] - a[0], b[1] - a[1])
            w = (c[0] - a[0], c[1] - a[1])
            lu = (u[0] ** 2 + u[1] ** 2) ** 0.5
            lw = (w[0] ** 2 + w[1] ** 2) ** 0.5
            if lu == 0 or lw == 0:
                return 0.0
            best = min(best, abs(u[0] * w[1] - u[1] * w[0]) / (lu * lw))
        return best

    def ear_clip(poly):
        """the polygon (in the run's winding, plan view) in triangles, widest ears first; None if it can't"""
        poly = list(poly)
        out = []
        while len(poly) > 3:
            best, bi = -1.0, -1
            n = len(poly)
            for k in range(n):
                a, b, c = poly[k - 1], poly[k], poly[(k + 1) % n]
                if _area2_xz(a, b, c) * sign <= 0:
                    continue
                pa, pb, pc = xz(a), xz(b), xz(c)
                bad = False
                for q in poly:
                    if q in (a, b, c):
                        continue
                    pq = xz(q)
                    s1 = ((pb[0] - pa[0]) * (pq[1] - pa[1]) - (pb[1] - pa[1]) * (pq[0] - pa[0])) * sign
                    s2 = ((pc[0] - pb[0]) * (pq[1] - pb[1]) - (pc[1] - pb[1]) * (pq[0] - pb[0])) * sign
                    s3 = ((pa[0] - pc[0]) * (pq[1] - pc[1]) - (pa[1] - pc[1]) * (pq[0] - pc[0])) * sign
                    if s1 >= 0 and s2 >= 0 and s3 >= 0:
                        bad = True
                        break
                if bad:
                    continue
                m = min_sin((a, b, c))
                if m > best:
                    best, bi = m, k
            if bi < 0:
                return None
            n = len(poly)
            out.append((poly[bi - 1], poly[bi], poly[(bi + 1) % n]))
            del poly[bi]
        if _area2_xz(*poly) * sign <= 0:
            return None
        out.append(tuple(poly))
        return out

    def fits(new):
        """every sample under the new triangles within the tolerances"""
        xs = [_i(v[0]) for t in new for v in t]
        zs = [_i(v[2]) for t in new for v in t]
        for cx in range(int(min(xs) // CELL), int(max(xs) // CELL) + 1):
            for cz in range(int(min(zs) // CELL), int(max(zs) // CELL) + 1):
                for (x, z, y, sh) in grid.get((cx, cz), ()):
                    for t in new:
                        p = [(_i(v[0]), _i(v[2]), _i(v[1])) for v in t]
                        d = (p[1][0] - p[0][0]) * (p[2][1] - p[0][1]) - (p[1][1] - p[0][1]) * (p[2][0] - p[0][0])
                        if d == 0:
                            continue
                        w1 = ((x - p[0][0]) * (p[2][1] - p[0][1]) - (z - p[0][1]) * (p[2][0] - p[0][0])) / d
                        w2 = ((p[1][0] - p[0][0]) * (z - p[0][1]) - (p[1][1] - p[0][1]) * (x - p[0][0])) / d
                        w0 = 1.0 - w1 - w2
                        if min(w0, w1, w2) < -1e-6:
                            continue
                        yy = w0 * p[0][2] + w1 * p[1][2] + w2 * p[2][2]
                        if abs(yy - y) > POS_TOL:
                            return False
                        ss = [_shade(v, lit) for v in t]
                        for k in range(3):
                            if abs(w0 * ss[0][k] + w1 * ss[1][k] + w2 * ss[2][k] - sh[k]) > SHADE_TOL:
                                return False
                        break
        return True

    def try_remove(v):
        ids = list(at.get(v, ()))
        if len(ids) < 3:
            return None
        nxt = {}
        for i in ids:
            t = tris[i][1]
            k = t.index(v)
            a, b = t[(k + 1) % 3], t[(k + 2) % 3]
            if a in nxt:
                return None
            nxt[a] = (b, i)
        if any(b not in nxt for b, _ in nxt.values()):
            return None  # (on the bed's border: kept)
        start = next(iter(nxt))
        ring, rids, cur = [], [], start
        for _ in range(len(nxt)):
            ring.append(cur)
            b, i = nxt[cur]
            rids.append(i)
            cur = b
        if cur != start or len(set(ring)) != len(ring):
            return None
        groups = [tris[i][0] for i in rids]
        gset = set(groups)
        if len(gset) == 1:
            new = ear_clip(ring)
            if new is None or any(min_sin(t) < MIN_SIN for t in new) or not fits(new):
                return None
            return ids, [(groups[0], t) for t in new]
        if len(gset) != 2:
            return None
        cuts = [j for j in range(len(ring)) if groups[j] != groups[j - 1]]
        if len(cuts) != 2:
            return None
        a, b = ring[cuts[0]], ring[cuts[1]]
        if not (a[1] == b[1] == v[1]):
            return None  # (the line through v isn't a depth line there)
        out = []
        for c0, c1 in ((cuts[0], cuts[1]), (cuts[1], cuts[0] + len(ring))):
            side = [ring[j % len(ring)] for j in range(c0, c1 + 1)]
            g = groups[c0 % len(ring)]
            if len(side) < 3:
                return None
            new = ear_clip(side)
            if new is None or any(min_sin(t) < MIN_SIN for t in new):
                return None
            out += [(g, t) for t in new]
        if not fits([t for _, t in out]):
            return None
        return ids, out

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
            g, t = tris.pop(i)
            for x in t:
                at[x].discard(i)
                touched.add(x)
        for g, t in new:
            i = nid[0]
            nid[0] += 1
            tris[i] = (g, t)
            for x in t:
                at[x].add(i)
        if not at[v]:
            del at[v]
        for x in touched:
            if x != v and x in at and x not in queued:
                queue.append(x)
                queued.add(x)
    return [tris[i][1] for i in sorted(tris)]
