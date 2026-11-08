#!/usr/bin/env python3
"""Occluder candidates for F3DEX3's occlusion plane, from each fishing scene's collision (at build time).

A candidate is a flat quad just inside solid ground or wall: everything the camera sees behind it, through it, is
hidden by the solid in front, so the RSP may drop it (fishmod_occlusion.c picks the best candidate each frame).
Found by grouping the collision's triangles into connected, nearly coplanar patches, then taking the biggest rectangle
each big patch covers completely (sampled on a grid), shrunk a little and pushed a few units into the solid.

Writes src/fishmod_data/fishmod_occluders.h (not committed: the collision is the game's).
Usage: fishmod/tools/gen_occluders.py [--report]
"""
import argparse
import math
import os
import re
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_watermap as gw  # noqa: E402

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "src", "fishmod_data", "fishmod_occluders.h")
SCENES = {"SCENE_LAKE_HYLIA": "overworld/spot06"}

ANGLE = math.radians(8)   # triangles this close in angle (and close in plane offset) are one patch
OFFSET = 12.0             # plane offset tolerance (units)
MIN_AREA = 40000.0        # patches smaller than this (units^2; 200 x 200) aren't worth an occluder
GRID = 16.0               # how finely a patch's cover is sampled
SHRINK = 12.0             # kept inside the patch's edges
INSET = 6.0               # pushed into the solid behind the surface
MAX_PER_SCENE = 48


def polys(scene_dir):
    """Every collision triangle: (three vertices, unit normal pointing out of the solid)."""
    vtx = gw.parse_vtx(gw.one(scene_dir, "*_VtxList.inc.c"))
    src = gw.read(gw.one(scene_dir, "*_PolyList.inc.c"))
    out = []
    for block in re.finditer(
            r"COLPOLY_VTX\((\d+),\s*(\d+)\).*?COLPOLY_VTX\((\d+),\s*(\d+)\).*?COLPOLY_VTX\((\d+),\s*(\d+)\).*?"
            r"COLPOLY_SNORMAL\(([-\d.]+)\).*?COLPOLY_SNORMAL\(([-\d.]+)\).*?COLPOLY_SNORMAL\(([-\d.]+)\)",
            src, re.S):
        g = block.groups()
        tri = np.array([vtx[int(g[0])], vtx[int(g[2])], vtx[int(g[4])]], dtype=float)
        n = np.array([float(g[6]), float(g[7]), float(g[8])])
        ln = np.linalg.norm(n)
        if ln < 1e-6:
            continue
        out.append((tri, n / ln))
    return out


def patches(tris):
    """Connected groups of nearly coplanar triangles (shared vertex = connected)."""
    n = len(tris)
    parent = list(range(n))

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i

    by_vert = {}
    for i, (t, _) in enumerate(tris):
        for v in t:
            by_vert.setdefault(tuple(v), []).append(i)
    cos_lim = math.cos(ANGLE)
    for idx in by_vert.values():
        for a in idx:
            for b in idx:
                if a < b and find(a) != find(b):
                    ta, na = tris[a]
                    tb, nb = tris[b]
                    if np.dot(na, nb) >= cos_lim and abs(np.dot(na, tb.mean(0)) - np.dot(na, ta.mean(0))) < OFFSET:
                        parent[find(a)] = find(b)
    groups = {}
    for i in range(n):
        groups.setdefault(find(i), []).append(i)
    return [[tris[i] for i in g] for g in groups.values()]


def tri_area(t):
    return 0.5 * np.linalg.norm(np.cross(t[1] - t[0], t[2] - t[0]))


def inside_tri2(p, a, b, c):
    d1 = (p[0] - b[0]) * (a[1] - b[1]) - (a[0] - b[0]) * (p[1] - b[1])
    d2 = (p[0] - c[0]) * (b[1] - c[1]) - (b[0] - c[0]) * (p[1] - c[1])
    d3 = (p[0] - a[0]) * (c[1] - a[1]) - (c[0] - a[0]) * (p[1] - a[1])
    neg = (d1 < 0) or (d2 < 0) or (d3 < 0)
    pos = (d1 > 0) or (d2 > 0) or (d3 > 0)
    return not (neg and pos)


def best_rect(cover):
    """The largest axis-aligned rectangle of True cells (the classic histogram method). Returns (r0, c0, r1, c1)."""
    rows, cols = cover.shape
    h = np.zeros(cols, dtype=int)
    best = (0, None)
    for r in range(rows):
        h = np.where(cover[r], h + 1, 0)
        stack = []
        for c in range(cols + 1):
            cur = h[c] if c < cols else 0
            start = c
            while stack and stack[-1][1] >= cur:
                s, hh = stack.pop()
                area = hh * (c - s)
                if area > best[0]:
                    best = (area, (r - hh + 1, s, r, c - 1))
                start = s
            stack.append((start, cur))
    return best[1]


def occluder(patch):
    """The candidate quad for a patch (four world points, in order), or None."""
    area = sum(tri_area(t) for t, _ in patch)
    if area < MIN_AREA:
        return None
    normal = np.mean([n * tri_area(t) for t, n in patch], axis=0)
    normal /= np.linalg.norm(normal)
    pts = np.array([v for t, _ in patch for v in t])
    centre = pts.mean(0)
    # in-plane axes: the patch's principal directions (so long walls get long rectangles)
    rel = pts - centre
    rel -= np.outer(rel @ normal, normal)
    _, _, vt = np.linalg.svd(rel, full_matrices=False)
    ax_u = vt[0] - np.dot(vt[0], normal) * normal
    ax_u /= np.linalg.norm(ax_u)
    ax_v = np.cross(normal, ax_u)
    tri2 = [np.array([[np.dot(v - centre, ax_u), np.dot(v - centre, ax_v)] for v in t]) for t, _ in patch]
    allp = np.vstack(tri2)
    u0, v0 = allp.min(0)
    u1, v1 = allp.max(0)
    nu = int((u1 - u0) / GRID) + 1
    nv = int((v1 - v0) / GRID) + 1
    if nu < 2 or nv < 2 or nu * nv > 200000:
        return None
    cover = np.zeros((nv, nu), dtype=bool)
    for t in tri2:
        tu0, tv0 = t.min(0)
        tu1, tv1 = t.max(0)
        for j in range(max(0, int((tv0 - v0) / GRID)), min(nv, int((tv1 - v0) / GRID) + 2)):
            for i in range(max(0, int((tu0 - u0) / GRID)), min(nu, int((tu1 - u0) / GRID) + 2)):
                if not cover[j, i] and inside_tri2((u0 + i * GRID, v0 + j * GRID), t[0], t[1], t[2]):
                    cover[j, i] = True
    # a cell counts only if all four of its corners are covered (so the rectangle is inside the patch)
    full = cover[:-1, :-1] & cover[1:, :-1] & cover[:-1, 1:] & cover[1:, 1:]
    rect = best_rect(full)
    if rect is None:
        return None
    r0, c0, r1, c1 = rect
    ua, ub = u0 + c0 * GRID + SHRINK, u0 + (c1 + 1) * GRID - SHRINK
    va, vb = v0 + r0 * GRID + SHRINK, v0 + (r1 + 1) * GRID - SHRINK
    if (ub - ua) * (vb - va) < MIN_AREA * 0.5:
        return None
    base = centre - normal * INSET  # into the solid: the collision normal points out of it
    quad = [base + ax_u * u + ax_v * v for u, v in ((ua, va), (ub, va), (ub, vb), (ua, vb))]
    return quad, (ub - ua) * (vb - va), normal


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--report", action="store_true")
    args = ap.parse_args()
    lines = ["// Generated by fishmod/tools/gen_occluders.py from the scenes' collision. Do not edit.",
             "// Occluder candidates for the occlusion plane: scene, then four corners (x, y, z), then the quad's",
             "// outward normal (x 1000, the side the camera must be on)",
             "#define FISHMOD_OCCLUDERS(X) \\"]
    for scene, folder in SCENES.items():
        scene_dir = os.path.join(ROOT, "extracted", "ntsc-1.2", "assets", "scenes", folder)
        found = []
        for patch in patches(polys(scene_dir)):
            o = occluder(patch)
            if o:
                found.append(o)
        found.sort(key=lambda o: -o[1])
        for quad, area, n in found[:MAX_PER_SCENE]:
            q = [int(round(c)) for p in quad for c in p]
            nn = [int(round(c * 1000)) for c in n]
            lines.append(f"    X({scene}, {', '.join(map(str, q))}, {', '.join(map(str, nn))}) \\")
            if args.report:
                print(f"{scene}: area {area:8.0f}  normal {n.round(2)}  at {np.mean(quad, 0).round(0)}")
        print(f"{scene}: {min(len(found), MAX_PER_SCENE)} occluders (of {len(found)})")
    lines.append("")
    with open(OUT, "w") as f:
        f.write("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
