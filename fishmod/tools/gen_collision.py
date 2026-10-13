#!/usr/bin/env python3
"""Collision for the solid habitat props (rocks): baked at build time, added to the scene's own collision when it
loads (fishmod.c), so it costs nothing per frame beyond the lookups BgCheck already does.

Chunky on purpose (vanilla's collision is coarse): rocks that touch or nearly touch are one cluster, wrapped in one
convex hull; the rock garden (iwagumi boulders off the grave islet) is merged more coarsely. Points are snapped to a
grid first (fewer, larger faces), and faces nobody can touch are dropped: those facing down, and those wholly under
the lake bed.

Reads the placed props (src/fishmod_data/fishmod_sc_<scene>.c, from gen_scatter.py --c) and the vanilla rock model;
writes src/fishmod_data/fishmod_col_<scene>.c (the game's data) and fishmod_col_<scene>.json (the triangles, for
gen_watermap.py, so the fish's map of the bed has the rocks too). Generated at build time, not committed.

Usage: fishmod/tools/gen_collision.py [--report]
"""
import importlib.util
import json
import math
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
DATA = os.path.join(ROOT, "src", "fishmod_data")
ROCK_VTX = os.path.join(ROOT, "extracted", "ntsc-1.2", "assets", "objects", "object_fish", "gFishingRockModelVtx.inc.c")
SCENES = {"SCENE_LAKE_HYLIA": "overworld/spot06"}
SNAP = 10        # grid the hull points are snapped to (units): fewer, larger faces
GAP = 8          # rocks this close (edge to edge) share a hull
GARDEN_GAP = 40  # in the rock garden, coarser
SPAN_MAX = 360   # a merged cluster wider than this is split back into tighter clusters (no tents over open bed)


def load_tool(name):
    spec = importlib.util.spec_from_file_location(name, os.path.join(ROOT, "fishmod", "tools", name + ".py"))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def rock_points():
    src = open(ROCK_VTX).read()
    return [tuple(map(int, m)) for m in re.findall(r"VTX\(\s*(-?\d+),\s*(-?\d+),\s*(-?\d+),", src)]


def props(base):
    kinds = re.findall(r"X\(\w+, (\w+)\)", open(os.path.join(DATA, "fishmod_props_kinds.h")).read()
                       .split("#define FISHMOD_PROP_KINDS_GENERATED")[0])
    src = open(os.path.join(DATA, f"fishmod_sc_{base}.c")).read()
    out = []
    for m in re.findall(r"\{ (-?\d+), (-?\d+), (-?\d+), (\d+), (\d+), (\d+), (\d+) \}", src):
        x, y, z, k, yaw, sc, tint = map(int, m)
        out.append({"x": x, "y": y, "z": z, "kind": kinds[k], "yaw": yaw, "scale": sc / 64.0})
    return out


def place(p, pts):
    """The rock's points in the world, as the game draws it (fishmod_scatter.c, Scatter_Matrix)."""
    ang = ((p["yaw"] << 8) & 0xFFFF) * 2 * math.pi / 65536.0
    c, s = math.cos(ang), math.sin(ang)
    k = p["scale"]
    return [(p["x"] + (x * c + z * s) * k, p["y"] + y * k, p["z"] + (-x * s + z * c) * k) for x, y, z in pts]


# ---------------------------------------------------------------------------------------------------------------
# A small 3D convex hull (incremental): fine for the few dozen points of a cluster


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def hull(points):
    pts = list(dict.fromkeys(points))
    if len(pts) < 4:
        return []
    # a starting tetrahedron from extreme points
    a = min(pts)
    b = max(pts, key=lambda p: dot(sub(p, a), sub(p, a)))
    ab = sub(b, a)
    c = max(pts, key=lambda p: dot(cross(ab, sub(p, a)), cross(ab, sub(p, a))))
    n = cross(ab, sub(c, a))
    d = max(pts, key=lambda p: abs(dot(n, sub(p, a))))
    if abs(dot(n, sub(d, a))) < 1e-6:
        return []  # flat
    faces = [(a, b, c), (a, c, d), (a, d, b), (b, d, c)]
    centre = tuple(sum(p[i] for p in (a, b, c, d)) / 4 for i in range(3))
    faces = [f if dot(cross(sub(f[1], f[0]), sub(f[2], f[0])), sub(f[0], centre)) > 0 else (f[0], f[2], f[1])
             for f in faces]
    for p in pts:
        visible = [f for f in faces if dot(cross(sub(f[1], f[0]), sub(f[2], f[0])), sub(p, f[0])) > 1e-6]
        if not visible:
            continue
        edges = {}
        for f in visible:
            for e in ((f[0], f[1]), (f[1], f[2]), (f[2], f[0])):
                edges[e] = edges.get(e, 0) + 1
        horizon = [e for e in edges if (e[1], e[0]) not in edges]
        faces = [f for f in faces if f not in visible] + [(e[0], e[1], p) for e in horizon]
    return faces


def volume(faces):
    return sum(dot(f[0], cross(f[1], f[2])) for f in faces) / 6.0


def proxy(points, keep_volume=0.88):
    """The rock simplified: its hull's points, dropping one at a time whichever loses the least volume, while the
    shape keeps at least `keep_volume` of the full hull's volume. Every cluster's hull is made of these few points."""
    pts = list({v for f in hull(points) for v in f})
    full = volume(hull(pts))
    while len(pts) > 6:
        best = None
        for i in range(len(pts)):
            v = volume(hull(pts[:i] + pts[i + 1:]))
            if best is None or v > best[0]:
                best = (v, i)
        if best[0] < full * keep_volume:
            break
        pts.pop(best[1])
    return pts


# ---------------------------------------------------------------------------------------------------------------


def bed_surface_type(d):
    """The commonest surface type among the scene's floors well under the water (the lake bed)."""
    src = open(next(os.path.join(d, f) for f in os.listdir(d) if f.endswith("_PolyList.inc.c"))).read()
    vsrc = open(next(os.path.join(d, f) for f in os.listdir(d) if f.endswith("_VtxList.inc.c"))).read()
    verts = [tuple(map(int, m)) for m in re.findall(r"\{\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\s*\}", vsrc)]
    counts = {}
    for m in re.finditer(r"(\d+), // type\s*\{\s*COLPOLY_VTX\((\d+).*?COLPOLY_SNORMAL\(([-\d.]+)\), // x\s*"
                         r"COLPOLY_SNORMAL\(([-\d.]+)\)", src, re.S):
        t, v0, nx, ny = int(m.group(1)), int(m.group(2)), float(m.group(3)), float(m.group(4))
        if ny > 0.7 and verts[v0][1] < -1340:
            counts[t] = counts.get(t, 0) + 1
    return max(counts, key=counts.get) if counts else 0


def clusters(rocks, gap_of):
    """Union-find: rocks whose footprints come within the gap of each other."""
    parent = list(range(len(rocks)))

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i

    for i, a in enumerate(rocks):
        for j in range(i + 1, len(rocks)):
            b = rocks[j]
            d = math.hypot(a["x"] - b["x"], a["z"] - b["z"])
            if d < a["r"] + b["r"] + max(gap_of(a), gap_of(b)):
                parent[find(i)] = find(j)
    groups = {}
    for i in range(len(rocks)):
        groups.setdefault(find(i), []).append(rocks[i])
    return list(groups.values())


def main():
    report = "--report" in sys.argv
    zm = load_tool("zone_map")
    gw = zm.gw
    gs = load_tool("gen_scatter")
    model = proxy(rock_points())
    model_r = max(math.hypot(x, z) for x, _, z in model)
    for scene, folder in SCENES.items():
        base = os.path.basename(folder)
        if not os.path.exists(os.path.join(DATA, f"fishmod_sc_{base}.c")):
            print(f"no scatter for {scene}: run gen_scatter.py --c first")
            continue
        d = os.path.join(gw.SCENES, folder)
        finder = gw.FloorFinder(gw.parse_floors(gw.one(d, "*_PolyList.inc.c"), gw.parse_vtx(gw.one(d, "*_VtxList.inc.c"))))
        zones = json.load(open(os.path.join(ROOT, "fishmod", "data", "lake_hylia_zones.json")))["zones"]
        garden = next(z for z in zones if z["id"] == 5)
        rocks = [dict(p, r=model_r * p["scale"]) for p in props(base) if p["kind"] == "rock"]
        for r in rocks:
            r["garden"] = gs.in_zone(garden, r["x"], r["z"]) > 0.0

        groups = []
        for g in clusters(rocks, lambda r: GARDEN_GAP if r["garden"] else GAP):
            xs = [r["x"] for r in g]
            zs = [r["z"] for r in g]
            if len(g) > 1 and max(max(xs) - min(xs), max(zs) - min(zs)) > SPAN_MAX:
                groups += clusters(g, lambda r: GAP)  # too wide for one hull: back to tight clusters
            else:
                groups.append(g)

        tris = []
        for g in groups:
            pts = []
            for r in g:
                pts += place(r, model)
            snapped = [tuple(round(c / SNAP) * SNAP for c in p) for p in pts]
            for f in hull(snapped):
                n = cross(sub(f[1], f[0]), sub(f[2], f[0]))
                ln = math.sqrt(dot(n, n))
                if ln < 1e-3:
                    continue
                n = (n[0] / ln, n[1] / ln, n[2] / ln)
                if n[1] < -0.3:
                    continue  # faces down: under the rock
                beds = [finder.floor(int(v[0]), int(v[2]), v[1] + 200) for v in f]
                if all(bd is not None and v[1] < bd - 2 for v, bd in zip(f, beds)):
                    continue  # wholly under the lake bed
                tris.append((f, n))

        # the lake bed's surface type: the commonest among the scene's underwater floors
        bed_type = bed_surface_type(d)

        # the game's data: shared vertices, polys with normals and plane distances
        vindex = {}
        verts = []
        polys = []
        for f, n in tris:
            ids = []
            for v in f:
                key = tuple(int(round(c)) for c in v)
                if key not in vindex:
                    vindex[key] = len(verts)
                    verts.append(key)
                ids.append(vindex[key])
            nx, ny, nz = (int(round(c * 0x7FFF)) for c in n)
            dist = int(round(-(n[0] * verts[ids[0]][0] + n[1] * verts[ids[0]][1] + n[2] * verts[ids[0]][2])))
            polys.append((ids, (nx, ny, nz), dist))

        os.makedirs(DATA, exist_ok=True)
        with open(os.path.join(DATA, f"fishmod_col_{base}.c"), "w") as fo:
            fo.write("// Generated by fishmod/tools/gen_collision.py from the placed props. Do not edit.\n")
            fo.write('#include "fishmod_collision.h"\n\n')
            fo.write(f"typedef struct {{\n    FishmodColHeader header;\n    Vec3s vtx[{len(verts)}];\n"
                     f"    FishmodColPoly polys[{len(polys)}];\n}} FishmodCol_{base};\n\n")
            fo.write(f"FishmodCol_{base} gFishmodCol_{base} = {{\n")
            fo.write(f"    {{ FISHMOD_COL_MAGIC, {len(verts)}, {len(polys)}, {bed_type}, 0 }},\n    {{\n")
            for v in verts:
                fo.write(f"        {{ {v[0]}, {v[1]}, {v[2]} }},\n")
            fo.write("    },\n    {\n")
            for ids, n, dist in polys:
                fo.write(f"        {{ {{ {ids[0]}, {ids[1]}, {ids[2]} }}, {{ {n[0]}, {n[1]}, {n[2]} }}, {dist} }},\n")
            fo.write("    },\n};\n")
        with open(os.path.join(DATA, f"fishmod_col_{base}.json"), "w") as fo:
            json.dump([[list(v) for v in f] for f, _ in tris], fo)
        if report or True:
            print(f"rock stand-in: {len(model)} points")
            print(f"{scene}: {len(rocks)} rocks -> {len(groups)} clusters "
                  f"({sum(1 for g in groups if g[0]['garden'])} in the rock garden), "
                  f"{len(polys)} triangles, {len(verts)} vertices")


if __name__ == "__main__":
    main()
