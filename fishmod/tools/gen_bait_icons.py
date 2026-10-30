#!/usr/bin/env python3
"""Generates the bait bottle icons (32x32 RGBA32, like the vanilla bottle icons) from the vanilla empty bottle: bee
larvae suspended in honey, as in Twilight Princess, with the honey at 3/3, 2/3 and 1/3 for the three uses left.
Made from the game's own icon, so generated at build time, never committed: writes the PNGs and an asset manifest
to src/fishmod_data/ (4x previews to fishmod/out/), and the build turns them into C with fishmod/tools/assets.py
(src/fishmod_data/fishmod_bait_icons.inc.c). Needs the extracted assets and Pillow."""
import json
import math
import os
import random

from PIL import Image

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
BOTTLE = os.path.join(ROOT, "extracted", "ntsc-1.2", "assets", "textures", "icon_item_static",
                      "gItemIconBottleEmptyTex.rgba32.png")
OUT_ART = os.path.join(ROOT, "src", "fishmod_data", "art")
MANIFEST = os.path.join(ROOT, "src", "fishmod_data", "bait_manifest.json")
OUT_C = "src/fishmod_data/fishmod_bait_icons.inc.c"

# The inside of the bottle's body: rows 12-25 (the glass base starts at 26). The honey's surface for each level.
BODY_TOP = 12
BODY_BOTTOM = 25
SURFACE = {3: 13.0, 2: 17.6, 1: 21.6}

HONEY_TOP = (255, 196, 60)     # light amber near the surface
HONEY_DEEP = (196, 112, 18)    # darker toward the bottom
HONEY_SURFACE = (255, 232, 140)
LARVA = (250, 242, 214)
LARVA_SHADE = (222, 196, 140)


def inside_mask(base):
    """The see-through pixels between the bottle's walls, per row of the body."""
    w, h = base.size
    mask = set()
    for y in range(BODY_TOP, BODY_BOTTOM + 1):
        row = [base.getpixel((x, y))[3] for x in range(w)]
        solid = [x for x in range(w) if row[x] > 200]
        if not solid:
            continue
        # the left wall is the first solid run, the right wall the last solid run on the far side of the middle
        left = solid[0]
        while left + 1 < w and row[left + 1] > 200:
            left += 1
        right = [x for x in solid if x > 16][0] if [x for x in solid if x > 16] else w - 1
        for x in range(left + 1, right):
            if 0 < row[x] <= 200:
                mask.add((x, y))
    return mask


def larvae_layer(level, seed):
    """Pale larvae suspended in the honey, drawn soft at 4x and scaled down: a vague mass, not individual grubs."""
    s = 4
    layer = Image.new("RGBA", (32 * s, 32 * s), (0, 0, 0, 0))
    px = layer.load()
    rnd = random.Random(seed)
    top = SURFACE[level] + 1.2
    count = {3: 13, 2: 9, 1: 5}[level]
    for _ in range(count):
        # mostly in the lower part of the honey, curled ovals of a couple of pixels
        cx = rnd.uniform(10.5, 21.0)
        cy = rnd.uniform(top, BODY_BOTTOM - 0.3) if rnd.random() < 0.5 else rnd.uniform((top + BODY_BOTTOM) / 2,
                                                                                        BODY_BOTTOM - 0.3)
        rx = rnd.uniform(0.9, 1.4)
        ry = rx * rnd.uniform(0.55, 0.75)
        ang = rnd.uniform(0, math.pi)
        depth = rnd.uniform(0.6, 0.95)  # deeper in the honey = fainter
        ca, sa = math.cos(ang), math.sin(ang)
        for yy in range(int((cy - 2) * s), int((cy + 2) * s)):
            for xx in range(int((cx - 2) * s), int((cx + 2) * s)):
                dx = (xx + 0.5) / s - cx
                dy = (yy + 0.5) / s - cy
                u = (dx * ca + dy * sa) / rx
                v = (-dx * sa + dy * ca) / ry
                d = u * u + v * v
                if d < 1.0 and 0 <= xx < 32 * s and 0 <= yy < 32 * s:
                    shade = LARVA if v < 0.2 else LARVA_SHADE
                    a = int(255 * depth * min(1.0, (1.0 - d) * 2.5))
                    old = px[xx, yy]
                    if a > old[3]:
                        px[xx, yy] = shade + (a,)
    return layer.resize((32, 32), Image.LANCZOS)


def icon(level):
    base = Image.open(BOTTLE).convert("RGBA")
    out = base.copy()
    px = out.load()
    surface = SURFACE[level]
    larvae = larvae_layer(level, 1000 + level).load()
    inside = inside_mask(base)

    for y in range(int(surface), BODY_BOTTOM + 3):
        for x in range(32):
            r, g, b, a = base.getpixel((x, y))
            if a == 0:
                continue
            lum = (r * 299 + g * 587 + b * 114) / 1000
            if (x, y) in inside and y + 1 > surface:
                # the honey: translucent amber, deeper colour lower down, a light line at the surface
                t = (y - surface) / max(1.0, BODY_BOTTOM - surface)
                hc = [HONEY_TOP[i] + (HONEY_DEEP[i] - HONEY_TOP[i]) * t for i in range(3)]
                if y < surface + 1.0:
                    edge = 1.0 - min(1.0, abs(x - 15.5) / 7.0)  # brighter in the middle, like a meniscus
                    hc = [hc[i] + (HONEY_SURFACE[i] - hc[i]) * (0.4 + 0.6 * edge) for i in range(3)]
                lr, lg, lb, la = larvae[x, y]
                k = la / 255.0
                hc = [hc[i] * (1 - k) + (lr, lg, lb)[i] * k for i in range(3)]
                # see the glass's shading through it, and keep its highlights on top
                mix = 0.8
                col = [hc[i] * mix + (r, g, b)[i] * (1 - mix) for i in range(3)]
                hl = max(0.0, (lum - 120) / 135.0)
                col = [col[i] * (1 - hl) + (r, g, b)[i] * hl for i in range(3)]
                px[x, y] = tuple(int(min(255, c)) for c in col) + (max(a, 236),)
            elif (y + 1 > surface) and (y <= BODY_BOTTOM + 2) and lum < 150:
                # the glass walls and base around the honey pick up its colour
                tint = (1.0, 0.72, 0.32)
                col = [(r, g, b)[i] * (0.55 + 0.45 * tint[i] * 1.35) for i in range(3)]
                px[x, y] = tuple(int(min(255, c)) for c in col) + (a,)
    return out


def main():
    os.makedirs(OUT_ART, exist_ok=True)
    for count in (3, 2, 1):
        img = icon(count)
        img.save(os.path.join(OUT_ART, f"bottle_larva_{count}.png"))
        os.makedirs(os.path.join(ROOT, "fishmod", "out"), exist_ok=True)
        img.resize((128, 128), Image.NEAREST).save(os.path.join(ROOT, "fishmod", "out", f"bottle_larva_{count}_x4.png"))
    entries = [{"name": f"gFishmodBottleLarva{c}Tex", "src": f"src/fishmod_data/art/bottle_larva_{c}.png",
                "format": "rgba32", "out": OUT_C} for c in (3, 2, 1)]
    with open(MANIFEST, "w") as f:
        json.dump({"textures": entries, "models": []}, f, indent=2)


if __name__ == "__main__":
    main()
