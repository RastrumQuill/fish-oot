#!/usr/bin/env python3
"""Draws the scene's placed habitat props (src/fishmod_data/fishmod_sc_<scene>.c, made by gen_scatter.py) from
above over the depth map, one colour per kind, for review.

Usage: fishmod/tools/scatter_map.py [out.png]   (default fishmod/docs/lake-hylia-props-map.png; run after a build)
"""
import importlib.util
import os
import re
import sys

from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
spec = importlib.util.spec_from_file_location("zone_map", os.path.join(ROOT, "fishmod", "tools", "zone_map.py"))
zm = importlib.util.module_from_spec(spec)
spec.loader.exec_module(zm)
gw = zm.gw

COLOURS = {
    "grass_tuft": (120, 200, 80), "ribbon_weed": (60, 160, 60), "tall_weed": (30, 120, 50), "broadleaf": (150, 210, 60),
    "bush_weed": (90, 130, 40), "reed_clump": (200, 190, 90), "pond_reed": (255, 255, 0), "cattails": (160, 110, 50), "old_posts": (110, 80, 60),
    "float_leaves": (170, 255, 140), "leaf_litter": (150, 80, 40), "pebble_patch": (200, 190, 160),
    "sunken_log": (90, 60, 30), "branch": (120, 90, 50), "drowned_tree": (70, 50, 30), "fallen_tree": (130, 70, 20),
    "rush_tuft": (220, 230, 120), "iris": (150, 120, 255), "big_pondweed": (40, 90, 30), "spatterdock": (240, 240, 60),
    "kelp_frond": (100, 140, 20), "vine_strand": (60, 200, 140), "vine_curtain": (20, 220, 180),
    "bank_roots": (180, 120, 90), "marigold": (255, 160, 0), "fan_weed": (180, 220, 160), "lily_flower": (255, 150, 220),
    "lily_centre": (255, 255, 255), "lily_pad": (0, 255, 90), "rock": (170, 170, 170),
}


def main():
    out_path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "fishmod", "docs", "lake-hylia-props-map.png")
    kinds = re.findall(r"X\(\w+, (\w+)\)", open(os.path.join(ROOT, "src", "fishmod_data",
                                                                  "fishmod_props_kinds.h")).read().split("#define FISHMOD_PROP_KINDS_GENERATED")[0])
    src = open(os.path.join(ROOT, "src", "fishmod_data", "fishmod_sc_spot06.c")).read()
    props = [tuple(map(int, m)) for m in re.findall(r"\{ (-?\d+), (-?\d+), (-?\d+), (\d+), (\d+), (\d+), (\d+) \}", src)]
    folder, (x0, x1, z0, z1), landmarks = zm.SCENES["SCENE_LAKE_HYLIA"]
    d = os.path.join(gw.SCENES, folder)
    vtx = gw.parse_vtx(gw.one(d, "*_VtxList.inc.c"))
    finder = gw.FloorFinder(gw.parse_floors(gw.one(d, "*_PolyList.inc.c"), vtx))
    boxes = gw.parse_waterboxes(gw.one(d, "*_WaterBoxes.inc.c"))
    step, scale = zm.STEP, zm.SCALE
    w, h = (x1 - x0) // step, (z1 - z0) // step
    base = Image.new("RGB", (w, h), (40, 40, 40))
    for j in range(h):
        for i in range(w):
            x, z = x0 + i * step, z0 + j * step
            surf = next((b["y"] for b in boxes if b["xMin"] <= x <= b["xMin"] + b["xLen"] and
                         b["zMin"] <= z <= b["zMin"] + b["zLen"]), None)
            f = finder.floor(x, z, (surf + 60) if surf is not None else 100000)
            if surf is None or f is None or f >= surf:
                if f is not None:
                    t = max(0, min(1, (f + 1400) / 900))
                    base.putpixel((i, j), (int(70 + 50 * t), int(80 + 40 * t), int(55 + 30 * t)))
                continue
            t = min((surf - f) / 1000.0, 1.0)
            base.putpixel((i, j), (int(60 * (1 - t) + 10), int(100 * (1 - t) + 25), int(140 - 70 * t)))
    img = base.resize((w * scale, h * scale), Image.BILINEAR)
    dr = ImageDraw.Draw(img)
    font = ImageFont.load_default()
    counts = {}
    big = {"sunken_log", "fallen_tree", "drowned_tree", "old_posts", "bank_roots", "rock"}
    for x, y, z, k, yaw, sc, tint in props:
        name = kinds[k]
        counts[name] = counts.get(name, 0) + 1
        px, pz = (x - x0) / step * scale, (z - z0) / step * scale
        r = 2.5 if name in big else 1.2
        dr.ellipse([px - r, pz - r, px + r, pz + r], fill=COLOURS.get(name, (255, 0, 255)))
    for name, x, z in landmarks:
        px, pz = (x - x0) / step * scale, (z - z0) / step * scale
        dr.text((px + 6, pz - 6), name, fill=(255, 255, 255), font=font)
    names = [k for k in kinds if k in counts]
    rows = (len(names) + 3) // 4
    leg = Image.new("RGB", (img.width, 18 * rows + 30), (20, 20, 20))
    ld = ImageDraw.Draw(leg)
    ld.text((8, 6), f"Habitat props from above: {len(props)} placed (north is up; water darker with depth)",
            fill=(255, 255, 255), font=font)
    for n, name in enumerate(names):
        cx, cy = 8 + (n % 4) * (img.width // 4), 26 + 18 * (n // 4)
        ld.rectangle([cx, cy + 2, cx + 10, cy + 12], fill=COLOURS.get(name, (255, 0, 255)))
        ld.text((cx + 16, cy), f"{name} ({counts[name]})", fill=(230, 230, 230), font=font)
    out = Image.new("RGB", (img.width, img.height + leg.height))
    out.paste(img, (0, 0))
    out.paste(leg, (0, img.height))
    out.save(out_path)
    print(f"wrote {out_path}")


if __name__ == "__main__":
    main()
