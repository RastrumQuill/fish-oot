#!/usr/bin/env python3
"""Bakes the water floor map of each fishing scene from its collision, at build time.

The fish ask for the floor under the water many times a frame. The game used to raycast each grid corner the first
time it was needed (about half a millisecond each on the N64) and keep it in a cache (fishmod_water.c). The scenery
never changes, so this does the same raycasts here and writes the grid to a ROM file per scene that the game loads
with the scene. The water surface isn't baked: it comes from the scene's water box at runtime, so water levels that
change (Lake Hylia drained or full) use the same map.

Same grid as the game's cache: corners every `cell` units over each water box, the cell doubled from 16 until all
the scene's boxes fit the corner budget; each corner is the floor height found by casting down from 60 above the
surface (the highest floor below that point), or "no floor".

Writes src/fishmod_data/fishmod_wm_<scene>.c and include/fishmod_watermap_scenes.h (the list of baked scenes).

Usage: fishmod/tools/gen_watermap.py        (needs the extracted assets)
"""
import glob
import json
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
SCENES = os.path.join(ROOT, "extracted", "ntsc-1.2", "assets", "scenes")
OUT_DIR = os.path.join(ROOT, "src", "fishmod_data")
OUT_HEADER = os.path.join(ROOT, "include", "fishmod_watermap_scenes.h")

# scene id name -> scene folder (relative to SCENES). Every scene with fish.
BAKED = [
    ("SCENE_KOKIRI_FOREST", "overworld/spot04"),
    ("SCENE_LAKE_HYLIA", "overworld/spot06"),
    ("SCENE_LAKESIDE_LABORATORY", "indoors/hylia_labo"),
]

# The game takes each box's cell size from the map, so these only need to fit its memory (Expansion Pak).
MAX_BOXES = 4
CORNER_BUDGET = 120000  # all boxes together, 2 bytes each
BOX_CORNERS_MAX = 45000  # finer than this per box isn't needed: coarser cells for big boxes (Lake Hylia: 32)
CELL_MIN = 16
CELL_MAX = 64
CAST_HEIGHT = 60
NO_FLOOR = -0x8000
BUCKET = 128

NUM = r"(-?\d+)"


def read(path):
    with open(path) as f:
        return f.read()


def one(scene_dir, pattern):
    hits = [h for h in glob.glob(os.path.join(scene_dir, pattern)) if "AltHeaders" not in h]
    if not hits:
        sys.exit(f"no {pattern} in {scene_dir}")
    return hits[0]


def parse_vtx(path):
    return [tuple(int(v) for v in m)
            for m in re.findall(r"\{\s*" + NUM + r",\s*" + NUM + r",\s*" + NUM + r"\s*\}", read(path))]


def parse_floors(path, vtx):
    floors = []
    for block in re.finditer(
            r"COLPOLY_VTX\((\d+),\s*(\d+)\).*?COLPOLY_VTX\((\d+),\s*(\d+)\).*?COLPOLY_VTX\((\d+),\s*(\d+)\).*?"
            r"COLPOLY_SNORMAL\(([-\d.]+)\).*?COLPOLY_SNORMAL\(([-\d.]+)\).*?COLPOLY_SNORMAL\(([-\d.]+)\)",
            read(path), re.S):
        g = block.groups()
        if float(g[7]) > 0.5:  # a floor (BgCheck's floor class)
            floors.append((vtx[int(g[0])], vtx[int(g[2])], vtx[int(g[4])]))
    return floors


def parse_waterboxes(path):
    boxes = []
    for m in re.finditer(NUM + r", // xMin\s*" + NUM + r", // ySurface\s*" + NUM + r", // zMin\s*" + NUM +
                         r", // xLength\s*" + NUM + r", // zLength", read(path)):
        x, y, z, xl, zl = (int(v) for v in m.groups())
        boxes.append({"xMin": x, "y": y, "zMin": z, "xLen": xl, "zLen": zl})
    return boxes


class FloorFinder:
    """The highest floor at or below a height, at a point: what a downward raycast finds."""

    def __init__(self, tris):
        self.buckets = {}
        for t in tris:
            xs = [v[0] for v in t]
            zs = [v[2] for v in t]
            for bx in range(min(xs) // BUCKET, max(xs) // BUCKET + 1):
                for bz in range(min(zs) // BUCKET, max(zs) // BUCKET + 1):
                    self.buckets.setdefault((bx, bz), []).append(t)

    def floor(self, x, z, top):
        best = None
        for a, b, c in self.buckets.get((x // BUCKET, z // BUCKET), ()):
            d = (b[2] - c[2]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[2] - c[2])
            if d == 0:
                continue
            w1 = ((b[2] - c[2]) * (x - c[0]) + (c[0] - b[0]) * (z - c[2])) / d
            w2 = ((c[2] - a[2]) * (x - c[0]) + (a[0] - c[0]) * (z - c[2])) / d
            w3 = 1 - w1 - w2
            if w1 < -1e-6 or w2 < -1e-6 or w3 < -1e-6:
                continue
            y = w1 * a[1] + w2 * b[1] + w3 * c[1]
            if y <= top and (best is None or y > best):
                best = y
        return best


def grid_for(box, corners_left):
    """Cell size for a box: 16, doubled while it has too many corners."""
    cell = CELL_MIN
    while True:
        width = int(box["xLen"] / cell) + 2
        depth = int(box["zLen"] / cell) + 2
        if width * depth <= min(corners_left, BOX_CORNERS_MAX) or cell >= CELL_MAX:
            break
        cell *= 2
    if width * depth > corners_left:
        return None
    return cell, width, depth


def bake(scene_name, folder):
    scene_dir = os.path.join(SCENES, folder)
    vtx = parse_vtx(one(scene_dir, "*_VtxList.inc.c"))
    floors = parse_floors(one(scene_dir, "*_PolyList.inc.c"), vtx)
    # the habitat props' collision (rocks, from gen_collision.py), which the game adds to the scene's: the fish see
    # the rocks as bed too
    extra = os.path.join(os.path.dirname(__file__), "..", "..", "src", "fishmod_data",
                         "fishmod_col_" + os.path.basename(folder) + ".json")
    if os.path.exists(extra):
        for tri in json.load(open(extra)):
            a, b, c = tri
            ny = (b[2] - a[2]) * (c[0] - a[0]) - (b[0] - a[0]) * (c[2] - a[2])
            ln = sum(v * v for v in ((b[1] - a[1]) * (c[2] - a[2]) - (b[2] - a[2]) * (c[1] - a[1]), ny,
                                     (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]))) ** 0.5
            if ln > 0 and ny / ln > 0.5:  # floors only, as above
                floors.append(tuple(tuple(v) for v in tri))
    finder = FloorFinder(floors)
    boxes = parse_waterboxes(one(scene_dir, "*_WaterBoxes.inc.c"))
    left = CORNER_BUDGET
    out = []
    for box in boxes[:MAX_BOXES]:
        g = grid_for(box, left)
        if g is None:
            continue
        cell, width, depth = g
        top = box["y"] + CAST_HEIGHT
        floors = []
        for iz in range(depth):
            for ix in range(width):
                f = finder.floor(box["xMin"] + ix * cell, box["zMin"] + iz * cell, top)
                floors.append(NO_FLOOR if f is None else max(-0x7FF0, min(0x7FF0, int(f))))
        out.append((box, cell, width, depth, floors))
        left -= width * depth
    return out


def write_scene(scene_name, folder, grids):
    base = os.path.basename(folder)
    total = sum(len(g[4]) for g in grids)
    lines = [
        "// Generated by fishmod/tools/gen_watermap.py from the scene's collision. Do not edit.",
        '#include "fishmod_watermap.h"',
        "",
        "typedef struct {",
        "    FishmodWaterMapHeader header;",
        f"    s16 floors[{max(total, 1)}];",
        f"}} FishmodWaterMap_{base};",
        "",
        f"FishmodWaterMap_{base} gFishmodWaterMap_{base} = {{",
        f"    {{ FISHMOD_WATERMAP_MAGIC, {len(grids)}, {total}, {{",
    ]
    offset = 0
    for box, cell, width, depth, floors in grids:
        lines.append(f"        {{ {box['xMin']}, {box['zMin']}, {box['xLen']}, {box['zLen']}, {cell}, {width}, {depth}, "
                     f"{offset} }},")
        offset += len(floors)
    lines.append("    } },")
    lines.append("    {")
    allf = [v for g in grids for v in g[4]]
    for i in range(0, len(allf), 16):
        lines.append("        " + ", ".join(str(v) for v in allf[i:i + 16]) + ",")
    if not allf:
        lines.append("        0,")
    lines.append("    },")
    lines.append("};")
    os.makedirs(OUT_DIR, exist_ok=True)
    with open(os.path.join(OUT_DIR, f"fishmod_wm_{base}.c"), "w") as f:
        f.write("\n".join(lines) + "\n")
    size = 12 + 20 * MAX_BOXES + 2 * total
    print(f"{scene_name}: {len(grids)} boxes, {total} corners, {size / 1024:.1f} KB")


def main():
    header = [
        "// Generated by fishmod/tools/gen_watermap.py. Do not edit.",
        "// The scenes with a baked water floor map: FISHMOD_WATERMAP(sceneId, segment name)",
    ]
    for scene_name, folder in BAKED:
        write_scene(scene_name, folder, bake(scene_name, folder))
        header.append(f"FISHMOD_WATERMAP({scene_name}, fishmod_wm_{os.path.basename(folder)})")
    with open(OUT_HEADER, "w") as f:
        f.write("\n".join(header) + "\n")


if __name__ == "__main__":
    main()
