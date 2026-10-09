#!/usr/bin/env python3
"""
Maps fishable water in a scene from the extracted collision data.

For every water box, samples a grid over it, finds the highest floor polygon
under each sample (ignoring walls/ceilings), and records the depth below the
water surface. Writes a PNG depth map and prints candidate fishing spots
(deep cells) and good standing spots (dry ground next to deep water).

Usage:
  fishmod/tools/water_map.py <scene dir> [--out map.png] [--step 20] [--min-depth 30]
e.g.
  fishmod/tools/water_map.py extracted/ntsc-1.2/assets/scenes/overworld/spot04
"""

import argparse
import glob
import os
import re
import sys

NUM = r"(-?\d+)"


def read(path):
    with open(path) as f:
        return f.read()


def parse_vtx(path):
    return [tuple(int(v) for v in m) for m in re.findall(r"\{\s*" + NUM + r",\s*" + NUM + r",\s*" + NUM + r"\s*\}", read(path))]


def parse_polys(path):
    polys = []
    text = read(path)
    for block in re.finditer(
        r"COLPOLY_VTX\((\d+),\s*(\d+)\).*?COLPOLY_VTX\((\d+),\s*(\d+)\).*?COLPOLY_VTX\((\d+),\s*(\d+)\).*?"
        r"COLPOLY_SNORMAL\(([-\d.]+)\).*?COLPOLY_SNORMAL\(([-\d.]+)\).*?COLPOLY_SNORMAL\(([-\d.]+)\)",
        text,
        re.S,
    ):
        g = block.groups()
        polys.append(((int(g[0]), int(g[2]), int(g[4])), (float(g[6]), float(g[7]), float(g[8]))))
    return polys


def parse_waterboxes(path):
    boxes = []
    for m in re.finditer(
        NUM + r", // xMin\s*" + NUM + r", // ySurface\s*" + NUM + r", // zMin\s*" + NUM + r", // xLength\s*" + NUM
        + r", // zLength",
        read(path),
    ):
        x, y, z, xl, zl = (int(v) for v in m.groups())
        boxes.append({"xMin": x, "y": y, "zMin": z, "xLen": xl, "zLen": zl})
    return boxes


def floor_height_at(x, z, tris):
    """Highest floor (normal.y > 0.5) at (x, z), or None."""
    best = None
    for (a, b, c), ny in tris:
        # barycentric test in xz
        d = (b[2] - c[2]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[2] - c[2])
        if d == 0:
            continue
        w1 = ((b[2] - c[2]) * (x - c[0]) + (c[0] - b[0]) * (z - c[2])) / d
        w2 = ((c[2] - a[2]) * (x - c[0]) + (a[0] - c[0]) * (z - c[2])) / d
        w3 = 1 - w1 - w2
        if w1 < -1e-6 or w2 < -1e-6 or w3 < -1e-6:
            continue
        y = w1 * a[1] + w2 * b[1] + w3 * c[1]
        if best is None or y > best:
            best = y
    return best


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("scene_dir")
    ap.add_argument("--out", default=None)
    ap.add_argument("--step", type=int, default=20)
    ap.add_argument("--min-depth", type=int, default=30)
    args = ap.parse_args()

    def one(pattern):
        hits = glob.glob(os.path.join(args.scene_dir, pattern))
        hits = [h for h in hits if "AltHeaders" not in h]
        if not hits:
            sys.exit(f"no {pattern} in {args.scene_dir}")
        return hits[0]

    vtx = parse_vtx(one("*_VtxList.inc.c"))
    polys = parse_polys(one("*_PolyList.inc.c"))
    boxes = parse_waterboxes(one("*_WaterBoxes.inc.c"))

    floors = []
    for (i0, i1, i2), (nx, ny, nz) in polys:
        if ny > 0.5:
            floors.append(((vtx[i0], vtx[i1], vtx[i2]), ny))

    print(f"{len(vtx)} vertices, {len(polys)} polys ({len(floors)} floors), {len(boxes)} water boxes")

    cells = []
    for bi, box in enumerate(boxes):
        print(f"box {bi}: x {box['xMin']}..{box['xMin'] + box['xLen']}  z {box['zMin']}..{box['zMin'] + box['zLen']}"
              f"  surface y={box['y']}")
        for x in range(box["xMin"], box["xMin"] + box["xLen"] + 1, args.step):
            for z in range(box["zMin"], box["zMin"] + box["zLen"] + 1, args.step):
                fy = floor_height_at(x, z, floors)
                depth = None if fy is None else box["y"] - fy
                cells.append((bi, x, z, fy, depth))

    deep = [c for c in cells if c[4] is not None and c[4] >= args.min_depth]
    print(f"{len(deep)} cells with depth >= {args.min_depth}")
    if deep:
        xs = [c[1] for c in deep]
        zs = [c[2] for c in deep]
        print(f"deep water extent: x {min(xs)}..{max(xs)}  z {min(zs)}..{max(zs)}  max depth {max(c[4] for c in deep):.0f}")
        deepest = sorted(deep, key=lambda c: -c[4])[:8]
        for c in deepest:
            print(f"  deep spot x={c[1]} z={c[2]} depth={c[4]:.0f}")

    # shore: dry cells (floor above surface) adjacent to deep cells
    lookup = {(c[1], c[2]): c for c in cells}
    shore = []
    for c in cells:
        if c[4] is not None and c[4] < 0:
            for dx, dz in ((args.step, 0), (-args.step, 0), (0, args.step), (0, -args.step)):
                n = lookup.get((c[1] + dx * 3, c[2] + dz * 3))
                if n and n[4] is not None and n[4] >= args.min_depth:
                    shore.append((c, n))
                    break
    print(f"{len(shore)} shore cells next to deep water; samples:")
    for c, n in shore[:: max(1, len(shore) // 8)][:8]:
        print(f"  stand x={c[1]} y={c[3]:.0f} z={c[2]}  facing deep x={n[1]} z={n[2]}")

    if args.out:
        try:
            from PIL import Image
        except ImportError:
            print("PIL not available, skipping image")
            return
        box_xs = [c[1] for c in cells]
        box_zs = [c[2] for c in cells]
        x0, z0 = min(box_xs), min(box_zs)
        w = (max(box_xs) - x0) // args.step + 1
        h = (max(box_zs) - z0) // args.step + 1
        img = Image.new("RGB", (w, h), (40, 40, 40))
        for _, x, z, fy, depth in cells:
            px, pz = (x - x0) // args.step, (z - z0) // args.step
            if depth is None:
                col = (0, 0, 0)
            elif depth <= 0:
                col = (90, 140, 60)
            else:
                t = min(depth / 150.0, 1.0)
                col = (int(120 * (1 - t)), int(180 * (1 - t) + 40), int(255 - 120 * t))
            img.putpixel((px, pz), col)
        img = img.resize((w * 4, h * 4), Image.NEAREST)
        img.save(args.out)
        print(f"wrote {args.out}")


if __name__ == "__main__":
    main()
