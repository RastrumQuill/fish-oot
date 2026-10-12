#!/usr/bin/env python3
"""Draws a scene's sub-biome zones (fishmod/data/<scene>_zones.json) over its depth map, for review.

Depth comes from the scene's collision (the same floor finder as gen_watermap.py): light blue is shallow, dark blue
deep; orange marks steep drop-offs (found automatically). Landmarks are labelled.

Usage: fishmod/tools/zone_map.py [zones.json] [out.png]
       (default: fishmod/data/lake_hylia_zones.json -> fishmod/docs/lake-hylia-zones-draft.png)
"""
import importlib.util
import json
import math
import os
import sys

from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
spec = importlib.util.spec_from_file_location("gen_watermap", os.path.join(ROOT, "fishmod", "tools", "gen_watermap.py"))
gw = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gw)

SCENES = {"SCENE_LAKE_HYLIA": ("overworld/spot06", (-4800, 3300, 2100, 9400), [
    ("lab", -2721, 3835), ("pond house", 1592, 3893), ("scarecrows", 12, 2664), ("main island", -900, 7120),
    ("grave islet", -3045, 6077), ("Link (bench)", -649, 3121)])}
STEP = 25
SCALE = 3
COLOURS = [(255, 80, 80), (255, 200, 60), (255, 255, 120), (120, 255, 120), (200, 120, 255), (120, 220, 255),
           (255, 140, 0), (40, 40, 255), (255, 0, 255), (255, 160, 200), (160, 255, 200), (200, 255, 60),
           (255, 255, 255), (255, 120, 40), (90, 255, 255)]


def main():
    zones_path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "fishmod", "data", "lake_hylia_zones.json")
    out_path = sys.argv[2] if len(sys.argv) > 2 else os.path.join(ROOT, "fishmod", "docs", "lake-hylia-zones-draft.png")
    data = json.load(open(zones_path))
    folder, (x0, x1, z0, z1), landmarks = SCENES[data["scene"]]
    d = os.path.join(gw.SCENES, folder)
    vtx = gw.parse_vtx(gw.one(d, "*_VtxList.inc.c"))
    finder = gw.FloorFinder(gw.parse_floors(gw.one(d, "*_PolyList.inc.c"), vtx))
    boxes = gw.parse_waterboxes(gw.one(d, "*_WaterBoxes.inc.c"))
    w, h = (x1 - x0) // STEP, (z1 - z0) // STEP
    base = Image.new("RGB", (w, h), (40, 40, 40))
    depth = {}
    for j in range(h):
        for i in range(w):
            x, z = x0 + i * STEP, z0 + j * STEP
            surf = next((b["y"] for b in boxes if b["xMin"] <= x <= b["xMin"] + b["xLen"] and
                         b["zMin"] <= z <= b["zMin"] + b["zLen"]), None)
            f = finder.floor(x, z, (surf + 60) if surf is not None else 100000)
            if surf is None or f is None or f >= surf:
                if f is not None:
                    t = max(0, min(1, (f + 1400) / 900))
                    base.putpixel((i, j), (int(100 + 80 * t), int(120 + 60 * t), int(70 + 40 * t)))
                continue
            depth[(i, j)] = surf - f
            t = min((surf - f) / 1000.0, 1.0)
            base.putpixel((i, j), (int(150 * (1 - t) ** 2 + 10), int(200 * (1 - t) + 40 * t), int(230 - 100 * t)))
    img = base.resize((w * SCALE, h * SCALE), Image.NEAREST).convert("RGBA")
    ov = Image.new("RGBA", img.size, (0, 0, 0, 0))
    dr = ImageDraw.Draw(ov)
    font = ImageFont.load_default()

    def P(x, z):
        return ((x - x0) / STEP * SCALE, (z - z0) / STEP * SCALE)

    for (i, j), dp in depth.items():
        n = [depth.get((i + a, j + b)) for a, b in ((1, 0), (-1, 0), (0, 1), (0, -1))]
        n = [v for v in n if v is not None]
        if n and max(abs(dp - v) for v in n) > 14 and dp > 120:
            dr.rectangle([i * SCALE, j * SCALE, i * SCALE + SCALE - 1, j * SCALE + SCALE - 1], fill=(255, 140, 0, 150))
    for k, zn in enumerate(data["zones"]):
        c = COLOURS[k % len(COLOURS)]
        sh = zn["shape"]
        if isinstance(sh, str):
            continue
        if sh[0] == "circle":
            cx, cz = P(sh[1], sh[2])
            r = sh[3] / STEP * SCALE
            dr.ellipse([cx - r, cz - r, cx + r, cz + r], outline=c + (255,), width=3)
            lx, lz = cx, cz
        else:
            ax, az = P(sh[1], sh[2])
            bx, bz = P(sh[3], sh[4])
            r = sh[5] / STEP * SCALE
            length = math.hypot(bx - ax, bz - az)
            nx, nz = -(bz - az) / length, (bx - ax) / length
            dr.line([(ax + nx * r, az + nz * r), (bx + nx * r, bz + nz * r)], fill=c + (255,), width=3)
            dr.line([(ax - nx * r, az - nz * r), (bx - nx * r, bz - nz * r)], fill=c + (255,), width=3)
            dr.ellipse([ax - r, az - r, ax + r, az + r], outline=c + (255,), width=3)
            dr.ellipse([bx - r, bz - r, bx + r, bz + r], outline=c + (255,), width=3)
            lx, lz = (ax + bx) / 2, (az + bz) / 2
        dr.rectangle([lx - 9, lz - 8, lx + 9, lz + 8], fill=(0, 0, 0, 200))
        dr.text((lx - 6, lz - 6), str(zn["id"]), fill=c + (255,), font=font)
    for name, x, z in landmarks:
        px, pz = P(x, z)
        dr.ellipse([px - 4, pz - 4, px + 4, pz + 4], fill=(0, 0, 0, 255))
        dr.text((px + 6, pz - 6), name, fill=(255, 255, 255, 255), font=font)
    img = Image.alpha_composite(img, ov)
    leg = Image.new("RGBA", (img.width, 22 * len(data["zones"]) + 60), (20, 20, 20, 255))
    ld = ImageDraw.Draw(leg)
    ld.text((8, 6), "Sub-biomes (draft) - north is up; light blue = shallow, dark = deep; orange = steep drop-off "
            "(auto). Shapes only mark places: zones blend in the game.", fill=(255, 255, 255), font=font)
    for k, zn in enumerate(data["zones"]):
        ld.text((8, 30 + 22 * k), f"{zn['id']:2d}. {zn['name']}: {zn['props']} | {zn['fish']}",
                fill=COLOURS[k % len(COLOURS)], font=font)
    out = Image.new("RGBA", (img.width, img.height + leg.height))
    out.paste(img, (0, 0))
    out.paste(leg, (0, img.height))
    out.convert("RGB").save(out_path)
    print(f"wrote {out_path}")


if __name__ == "__main__":
    main()
