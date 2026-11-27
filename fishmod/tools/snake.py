"""F3DEX3 triangle snakes for the mod's own generated models (the props: assets.py; the rocks: rock_mesh.py), as
tools/strippass.py makes them for the game's: the same chains, the same rule for which triangles may be turned (small,
or their corners shading alike; otherwise each is drawn from its own first corner, as the near plane's cut fans from
it). Only in a build whose microcode draws them: FISHMOD_STRIPS=1 in the environment (fishmod/build.sh sets it from
--strips / --no-strips, on by default with F3DEX3).

  run(tris, verts, scale)  -> [("snake", first, seq) | ("tri2", a, b) | ("tri1", a)] for one vertex load's triangles
  c_lines(items)           -> the commands as C (gsSPTriSnake, gsSPContinueSnake, gsSP2Triangles, gsSP1Triangle)
  words(items)             -> the commands as 32-bit words (the rocks' binary lists)
"""
import math
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "tools"))
import strippass  # noqa: E402  (the chains and their encoding)

ENABLED = os.environ.get("FISHMOD_STRIPS") == "1"
SMALL_EDGE = strippass.SMALL_EDGE    # (in the world's units)
SHADE_TOL = strippass.SHADE_TOL
G_TRI1, G_TRI2, G_TRISNAKE = 0x05, 0x06, 0x08


def free_of(tri, verts, scale):
    """may this triangle be turned (its corners' order changed) to join a chain: small in the world (scale: the
    model's units to the world's), or its corners shading alike. verts: (x, y, z, ..., r, g, b[, a]) per vertex, the
    shade from index 5"""
    ps = [verts[i][0:3] for i in tri]
    small = all(math.dist(ps[k], ps[(k + 1) % 3]) * scale <= SMALL_EDGE for k in range(3))
    if small:
        return True
    n = min(len(verts[i]) for i in tri)
    return all(max(verts[i][c] for i in tri) - min(verts[i][c] for i in tri) <= SHADE_TOL for c in range(5, n))


def run(tris, verts, scale=1.0):
    """one load's triangles (in the load's own vertex numbers; verts the same numbering) as chains and pairs"""
    if not ENABLED:
        out = []
        for k in range(0, len(tris) - 1, 2):
            out.append(("tri2", tuple(tris[k]), tuple(tris[k + 1])))
        if len(tris) % 2:
            out.append(("tri1", tuple(tris[-1])))
        return out
    tris = [tuple(t) for t in tris]
    free = {k: free_of(t, verts, scale) for k, t in enumerate(tris)}
    out, small = [], []
    for first, seq in strippass.snakes(tris, free):
        if len(seq) >= 2:
            out.append(("snake", first, seq))
        else:
            i1, i2, i3 = first
            small.append((i3, i1, i2))
            if seq:
                n, d = seq[0]
                small.append((n, i3, i2) if d == 0 else (n, i1, i3))
    for k in range(0, len(small) - 1, 2):
        out.append(("tri2", small[k], small[k + 1]))
    if len(small) % 2:
        out.append(("tri1", small[-1]))
    return out


def c_lines(items):
    out = []
    for it in items:
        if it[0] == "snake":
            out += strippass.encode(it[1], it[2])
        elif it[0] == "tri2":
            out.append("gsSP2Triangles(%d, %d, %d, 0, %d, %d, %d, 0)" % (it[1] + it[2]))
        else:
            out.append("gsSP1Triangle(%d, %d, %d, 0)" % it[1])
    return out


def words(items):
    out = []
    for it in items:
        if it[0] == "tri2":
            a, b = it[1], it[2]
            out += [(G_TRI2 << 24) | (a[0] * 2 << 16) | (a[1] * 2 << 8) | a[2] * 2,
                    (b[0] * 2 << 16) | (b[1] * 2 << 8) | b[2] * 2]
        elif it[0] == "tri1":
            a = it[1]
            out += [(G_TRI1 << 24) | (a[0] * 2 << 16) | (a[1] * 2 << 8) | a[2] * 2, 0]
        else:
            (i1, i2, i3), seq = it[1], it[2]
            # each index byte: index * 2 | its direction (1 left, 0 right), 0x80 on the last
            idx = [(i3, 1)] + [(n, d) for n, d in seq]
            b = [(i * 2) | d for i, d in idx]
            b[-1] |= 0x80
            data = b[1:]
            data += [0] * ((-(len(data) - 4)) % 8 if len(data) > 4 else 4 - len(data))
            out += [(G_TRISNAKE << 24) | (i2 * 2 << 16) | (i1 * 2 << 8) | b[0],
                    (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]]
            rest = data[4:]
            for k in range(0, len(rest), 8):
                c = rest[k:k + 8]
                out += [(c[0] << 24) | (c[1] << 16) | (c[2] << 8) | c[3], (c[4] << 24) | (c[5] << 16) | (c[6] << 8) | c[7]]
    return out
