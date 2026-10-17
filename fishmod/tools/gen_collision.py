#!/usr/bin/env python3
"""Collision for the solid habitat props (rocks): baked at build time, added to the scene's own collision when it
loads (fishmod.c), so it costs nothing per frame beyond the lookups BgCheck already does.

Each rock's collision is its own drawn mesh (the vanilla rock's 16 triangles, placed as the game draws it), so Link
stands on what he sees: the earlier convex hulls over clusters of rocks floated him over gaps and let him into the
rocks from below (test 5). Triangles wholly under the lake bed are dropped (nobody can touch them).

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
ROCK_DL = os.path.join(ROOT, "extracted", "ntsc-1.2", "assets", "objects", "object_fish", "gFishingRockModelDL.inc.c")
SCENES = {"SCENE_LAKE_HYLIA": "overworld/spot06"}


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
    for m in re.findall(r"\{ (-?\d+), (-?\d+), (-?\d+), (\d+), (\d+), (\d+), (\d+), -?\d+, -?\d+ \}", src):
        x, y, z, k, yaw, sc, tint = map(int, m)
        out.append({"x": x, "y": y, "z": z, "kind": kinds[k], "yaw": yaw, "scale": sc / 64.0})
    return out


def place(p, pts):
    """The rock's points in the world, as the game draws it (fishmod_scatter.c, Scatter_Matrix)."""
    ang = ((p["yaw"] << 8) & 0xFFFF) * 2 * math.pi / 65536.0
    c, s = math.cos(ang), math.sin(ang)
    k = p["scale"]
    return [(p["x"] + (x * c + z * s) * k, p["y"] + y * k, p["z"] + (-x * s + z * c) * k) for x, y, z in pts]


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def rock_faces():
    """The vanilla rock's triangles, as indices into rock_points(), from its display list."""
    src = open(ROCK_DL).read()
    out = []
    for m in re.finditer(r"gsSP2Triangles\((\d+), (\d+), (\d+), 0, (\d+), (\d+), (\d+), 0\)", src):
        v = list(map(int, m.groups()))
        out += [tuple(v[:3]), tuple(v[3:])]
    for m in re.finditer(r"gsSP1Triangle\((\d+), (\d+), (\d+), 0\)", src):
        out.append(tuple(map(int, m.groups())))
    return out


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


BRIDGE_SURFACE = 23  # Lake Hylia's bridges' surface type (SURFACE_MATERIAL_BRIDGE: wooden footsteps)


def box_tris(x0, x1, y0, y1, z0, z1, stype, top=True):
    """An axis-aligned box's faces (no bottom), outward, as (tri, normal, type)."""
    quads = [((x0, y0, z0), (x0, y1, z0), (x1, y1, z0), (x1, y0, z0)),  # -z
             ((x1, y0, z1), (x1, y1, z1), (x0, y1, z1), (x0, y0, z1)),  # +z
             ((x0, y0, z1), (x0, y1, z1), (x0, y1, z0), (x0, y0, z0)),  # -x
             ((x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1))]  # +x
    if top:
        quads.append(((x0, y1, z0), (x0, y1, z1), (x1, y1, z1), (x1, y1, z0)))
    out = []
    for a, b, c, d in quads:
        for t in ((a, b, c), (a, c, d)):
            n = cross(sub(t[1], t[0]), sub(t[2], t[0]))
            ln = math.sqrt(dot(n, n))
            out.append((t, (n[0] / ln, n[1] / ln, n[2] / ln), stype))
    return out


def dock_tris(scene):
    """The north-beach dock's collision, from the same numbers its mesh is made from (gen_habitat.py)."""
    gh = load_tool("gen_habitat")
    if gh.DOCK["scene"] != scene:
        return []
    ox, oy, oz = gh.DOCK["x"], gh.DOCK["y"], gh.DOCK["z"]
    hw = gh.DOCK_WIDTH / 2
    out = box_tris(ox - hw, ox + hw, oy - gh.DOCK_THICK, oy, oz, oz + gh.DOCK_LENGTH, BRIDGE_SURFACE)
    r = gh.DOCK_PILING_R
    for z in gh.DOCK_PILING_ROWS:
        for x in (-gh.DOCK_PILING_X, gh.DOCK_PILING_X):
            out += box_tris(ox + x - r, ox + x + r, oy - gh.DOCK_PILING_DEPTH, oy - gh.DOCK_THICK, oz + z - r,
                            oz + z + r, BRIDGE_SURFACE, top=False)
    mx, mz, mh = gh.DOCK_MOORING
    out += box_tris(ox + mx - 6, ox + mx + 6, oy - gh.DOCK_PILING_DEPTH, oy + mh, oz + mz - 6, oz + mz + 6,
                    BRIDGE_SURFACE)
    return out


def main():
    report = "--report" in sys.argv
    zm = load_tool("zone_map")
    gw = zm.gw
    pts = rock_points()
    faces = rock_faces()
    for scene, folder in SCENES.items():
        base = os.path.basename(folder)
        if not os.path.exists(os.path.join(DATA, f"fishmod_sc_{base}.c")):
            print(f"no scatter for {scene}: run gen_scatter.py --c first")
            continue
        d = os.path.join(gw.SCENES, folder)
        finder = gw.FloorFinder(gw.parse_floors(gw.one(d, "*_PolyList.inc.c"), gw.parse_vtx(gw.one(d, "*_VtxList.inc.c"))))
        rocks = [p for p in props(base) if p["kind"] == "rock"]
        bed_type_rock = bed_surface_type(d)
        tris = []
        buried = 0
        for r in rocks:
            world = [tuple(int(round(c)) for c in v) for v in place(r, pts)]
            for a, b, c in faces:
                f = (world[a], world[b], world[c])
                n = cross(sub(f[1], f[0]), sub(f[2], f[0]))
                ln = math.sqrt(dot(n, n))
                if ln < 1e-3:
                    continue  # (a small rock's sliver, gone to nothing on the integer grid)
                n = (n[0] / ln, n[1] / ln, n[2] / ln)
                beds = [finder.floor(v[0], v[2], v[1] + 200) for v in f]
                if all(bd is not None and v[1] < bd - 2 for v, bd in zip(f, beds)):
                    buried += 1
                    continue  # wholly under the lake bed
                tris.append((f, n, bed_type_rock))

        rock_tris = len(tris)
        dock_polys = dock_tris(scene)
        tris += dock_polys

        # the game's data: shared vertices, polys with normals and plane distances
        vindex = {}
        verts = []
        polys = []
        for f, n, stype in tris:
            ids = []
            for v in f:
                key = tuple(int(round(c)) for c in v)
                if key not in vindex:
                    vindex[key] = len(verts)
                    verts.append(key)
                ids.append(vindex[key])
            nx, ny, nz = (int(round(c * 0x7FFF)) for c in n)
            dist = int(round(-(n[0] * verts[ids[0]][0] + n[1] * verts[ids[0]][1] + n[2] * verts[ids[0]][2])))
            polys.append((stype, ids, (nx, ny, nz), dist))

        os.makedirs(DATA, exist_ok=True)
        with open(os.path.join(DATA, f"fishmod_col_{base}.c"), "w") as fo:
            fo.write("// Generated by fishmod/tools/gen_collision.py from the placed props. Do not edit.\n")
            fo.write('#include "fishmod_collision.h"\n\n')
            fo.write(f"typedef struct {{\n    FishmodColHeader header;\n    Vec3s vtx[{len(verts)}];\n"
                     f"    FishmodColPoly polys[{len(polys)}];\n}} FishmodCol_{base};\n\n")
            fo.write(f"FishmodCol_{base} gFishmodCol_{base} = {{\n")
            fo.write(f"    {{ FISHMOD_COL_MAGIC, {len(verts)}, {len(polys)} }},\n    {{\n")
            for v in verts:
                fo.write(f"        {{ {v[0]}, {v[1]}, {v[2]} }},\n")
            fo.write("    },\n    {\n")
            for stype, ids, n, dist in polys:
                fo.write(f"        {{ {stype}, {{ {ids[0]}, {ids[1]}, {ids[2]} }}, {{ {n[0]}, {n[1]}, {n[2]} }}, {dist} }},\n")
            fo.write("    },\n};\n")
        with open(os.path.join(DATA, f"fishmod_col_{base}.json"), "w") as fo:
            # the rocks only: the fish's map of the bed (the dock is above the water)
            json.dump([[list(v) for v in f] for f, _, _ in tris[:rock_tris]], fo)
        if report or True:
            print(f"{scene}: dock {len(dock_polys)} triangles")
            print(f"{scene}: {len(rocks)} rocks, {rock_tris} triangles ({buried} under the bed dropped); "
                  f"{len(polys)} triangles, {len(verts)} vertices in all")


if __name__ == "__main__":
    main()
