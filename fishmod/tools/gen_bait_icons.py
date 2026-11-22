#!/usr/bin/env python3
"""Generates the bait bottle icons (32x32 RGBA32, like the vanilla bottle icons): bee larvae suspended in honey, as in
Twilight Princess, with the honey at 3/3, 2/3 and 1/3 for the three uses left. Made from the vanilla red potion's icon
(its liquid, its glass and its highlights, the liquid made honey) and the empty bottle's (the glass above a lower
honey), the larvae rendered at 8x and set into the honey.
Made from the game's own icon, so generated at build time, never committed: writes the PNGs and an asset manifest
to src/fishmod_data/ (4x previews to fishmod/out/), and the build turns them into C with fishmod/tools/assets.py
(src/fishmod_data/fishmod_bait_icons.inc.c). Needs the extracted assets and Pillow."""
import json
import math
import os
import random

import numpy as np

from PIL import Image

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
BOTTLE = os.path.join(ROOT, "extracted", "ntsc-1.2", "assets", "textures", "icon_item_static",
                      "gItemIconBottleEmptyTex.rgba32.png")
OUT_ART = os.path.join(ROOT, "src", "fishmod_data", "art")
MANIFEST = os.path.join(ROOT, "src", "fishmod_data", "bait_manifest.json")
OUT_C = "src/fishmod_data/fishmod_bait_icons.inc.c"

POTION = os.path.join(os.path.dirname(BOTTLE), "gItemIconBottlePotionRedTex.rgba32.png")

# The red potion's liquid spans rows 11-26 of its icon (a full bottle); the honey's surface for each level
LIQ_TOP, LIQ_BOTTOM = 11, 26
SURFACE = {3: 11.0, 2: 16.3, 1: 21.4}
HONEY_DARK = np.array([112.0, 44.0, 0.0])     # the honey in shadow (the potion's darkest red)
HONEY_MID = np.array([214.0, 128.0, 16.0])    # its body: amber
HONEY_LIGHT = np.array([250.0, 196.0, 74.0])  # and where the light comes through it
MENISCUS = np.array([255.0, 236.0, 160.0])
LARVA = np.array([252.0, 244.0, 214.0])       # TP's bee larvae: fat, pale, cream
LARVA_DARK = np.array([196.0, 150.0, 86.0])
# the grubs in the honey: (x, y, size, curl angle, depth 0 near the glass - 1 deep), filled bottle first
LARVAE = [
    (12.6, 22.2, 1.30, 0.4, 0.10), (19.6, 20.6, 1.25, 2.6, 0.22), (16.0, 16.8, 1.10, 4.4, 0.40),
    (10.8, 17.4, 0.95, 1.5, 0.55), (20.6, 24.4, 1.00, 3.6, 0.50),
]


def liquid_mask(potion):
    """The potion's liquid: its red pixels (the glass round it is grey), within the body's rows."""
    a = potion.astype(float)
    r, g, b = a[..., 0], a[..., 1], a[..., 2]
    m = (r > g * 1.35 + 8) & (a[..., 3] > 0)
    m[:LIQ_TOP] = False
    m[LIQ_BOTTOM + 1:] = False
    return m


def honey(potion, mask):
    """The potion's liquid made honey: its brightness (its red) mapped dark amber to light gold, its glass sheen (the
    white in it, the least of green and blue) kept on top."""
    out = potion.astype(float).copy()
    r = out[..., 0]
    sheen = np.minimum(out[..., 1], out[..., 2])
    # (the potion's own range of red: its darkest to its brightest, so the honey keeps all of its depth)
    lo, hi = np.percentile(r[mask], 3), np.percentile(r[mask], 97)
    t = np.clip((r - lo) / max(hi - lo, 1.0), 0.0, 1.0)
    t2 = t[..., None]
    col = np.where(t2 < 0.6, HONEY_DARK + (HONEY_MID - HONEY_DARK) * (t2 / 0.6),
                   HONEY_MID + (HONEY_LIGHT - HONEY_MID) * ((t2 - 0.6) / 0.4))
    col = col + sheen[..., None] * np.array([1.0, 0.95, 0.8])[None, None]
    out[..., :3] = np.where(mask[..., None], np.clip(col, 0, 255), out[..., :3])
    return out


def grub_layer(level):
    """The larvae at 8x, each a curled chain of round segments (fat at the head, thinning to the tail), lit from the
    upper left; returned 32x32 with each pixel's depth (how much honey is in front of it)."""
    S = 8
    n = {3: 5, 2: 4, 1: 2}[level]
    col = np.zeros((32 * S, 32 * S, 3))
    cov = np.zeros((32 * S, 32 * S))
    dep = np.zeros((32 * S, 32 * S))
    yy, xx = np.mgrid[0:32 * S, 0:32 * S]
    surface = SURFACE[level]
    for gx, gy, size, ang, depth in LARVAE[:n]:
        if gy - 1.5 < surface + 0.8:
            continue
        segs = 5
        for k in range(segs):  # (drawn tail first, head over it)
            kk = segs - 1 - k
            a = ang + kk * 0.5  # the curl: a fat C, a kidney bean
            rad = 0.8 * size
            cx = (gx + math.cos(a) * rad) * S
            cy = (gy + math.sin(a) * rad * 0.85) * S
            rr = (1.0 - 0.08 * kk) * size * S
            d2 = ((xx + 0.5 - cx) ** 2 + (yy + 0.5 - cy) ** 2) / (rr * rr)
            inside = d2 < 1.0
            nz = np.sqrt(np.clip(1.0 - d2, 0, 1))
            nx = (xx + 0.5 - cx) / rr
            ny = (yy + 0.5 - cy) / rr
            lam = np.clip(-0.55 * nx - 0.6 * ny + 0.58 * nz, 0, 1)
            spec = np.clip(lam, 0, 1) ** 12 * 0.35
            c = LARVA_DARK[None, None] + (LARVA - LARVA_DARK)[None, None] * (0.25 + 0.75 * lam)[..., None]
            c = c + spec[..., None] * 255
            # a darker crease between segments
            c = c * (0.86 + 0.14 * np.clip(nz * 1.6, 0, 1))[..., None]
            col[inside] = c[inside]
            cov[inside] = 1.0
            dep[inside] = depth
    # down to 32x32 (coverage-weighted)
    def down(x):
        return x.reshape(32, S, 32, S, *x.shape[2:]).mean(axis=(1, 3))
    c32 = down(col * cov[..., None])
    a32 = down(cov)
    d32 = down(dep * cov)
    with np.errstate(invalid="ignore", divide="ignore"):
        c32 = np.where(a32[..., None] > 0, c32 / a32[..., None], 0)
        d32 = np.where(a32 > 0, d32 / a32, 0)
    return c32, a32, d32


def icon(level):
    potion = np.asarray(Image.open(POTION).convert("RGBA"))
    empty = np.asarray(Image.open(BOTTLE).convert("RGBA")).astype(float)
    mask = liquid_mask(potion)
    out = honey(potion, mask)
    surface = SURFACE[level]
    rows = np.arange(32)[:, None] + 0.5
    # above the honey's surface: the empty bottle's glass inside
    above = mask & (rows < surface)
    out[above] = empty[above]
    # the meniscus: a light line along the surface, brightest in the middle, the honey's curve up at the walls
    if level < 3:
        cols = np.arange(32)[None, :] + 0.5
        xs = np.where(mask.any(axis=0))[0]
        mid = (xs.min() + xs.max() + 1) / 2.0
        half = (xs.max() + 1 - xs.min()) / 2.0
        rise = 0.6 * ((cols - mid) / half) ** 2  # (higher at the walls)
        line = mask & (np.abs(rows - (surface + 0.5 - rise)) < 0.75)
        k = 0.55 * (1.0 - 0.5 * np.abs((cols - mid) / half))
        k = np.broadcast_to(k, line.shape)
        out[..., :3] = np.where(line[..., None], out[..., :3] * (1 - k[..., None]) + MENISCUS * k[..., None],
                                out[..., :3])
    # the larvae, seen through the honey: tinted by it, more so deeper
    c32, a32, d32 = grub_layer(level)
    inl = mask & (rows >= surface + 0.7)
    tint = out[..., :3]
    lar = c32 * (1.0 - 0.5 * d32[..., None]) + tint * (0.5 * d32[..., None])
    # (the honey's own shading still darkens them where it's dark: multiply in a little of it)
    shade = np.clip(tint.mean(axis=2, keepdims=True) / 200.0, 0.55, 1.1)
    lar = lar * (0.82 + 0.18 * shade)
    k = np.clip(a32 * 1.3, 0, 1)[..., None] * inl[..., None] * 0.95
    out[..., :3] = out[..., :3] * (1 - k) + lar * k
    out[..., 3] = np.where(mask, np.maximum(out[..., 3], 240), out[..., 3])
    return Image.fromarray(np.clip(out + 0.5, 0, 255).astype(np.uint8), "RGBA")


SOIL_DARK = np.array([34.0, 20.0, 10.0])
SOIL_MID = np.array([86.0, 56.0, 30.0])
SOIL_LIGHT = np.array([140.0, 102.0, 62.0])
WORM = np.array([236.0, 128.0, 134.0])
WORM_DARK = np.array([150.0, 60.0, 70.0])
SOIL_TOP = 14.6
# the worms: paths in icon pixels (against the glass, one over the top of the soil)
WORMS = [
    [(9.5, 23.5), (11.5, 21.8), (13.8, 22.6), (15.6, 24.4), (17.8, 24.0)],
    [(14.0, 14.2), (15.6, 12.6), (17.6, 12.9), (18.8, 14.6), (20.6, 15.6), (22.2, 15.0)],
    [(19.0, 19.0), (20.8, 20.6), (21.4, 22.8), (20.0, 24.4)],
]


def worm_layer():
    """The worms at 8x: round, ringed, lit from the upper left; 32x32 colour and coverage."""
    S = 8
    col = np.zeros((32 * S, 32 * S, 3))
    cov = np.zeros((32 * S, 32 * S))
    yy, xx = np.mgrid[0:32 * S, 0:32 * S] + 0.5
    for path in WORMS:
        pts = np.array(path) * S
        # dense points along it
        dense = []
        for (x0, y0), (x1, y1) in zip(pts[:-1], pts[1:]):
            for t in np.linspace(0, 1, 12, endpoint=False):
                dense.append((x0 + (x1 - x0) * t, y0 + (y1 - y0) * t))
        dense.append(tuple(pts[-1]))
        n = len(dense)
        for k, (cx, cy) in enumerate(dense):
            t = k / (n - 1)
            rr = (0.95 - 0.25 * abs(t - 0.35)) * S  # (thickest a third along: its saddle)
            d2 = ((xx - cx) ** 2 + (yy - cy) ** 2) / (rr * rr)
            inside = d2 < 1.0
            nz = np.sqrt(np.clip(1.0 - d2, 0, 1))
            lam = np.clip(-0.5 * (xx - cx) / rr - 0.55 * (yy - cy) / rr + 0.65 * nz, 0, 1)
            ring = 0.88 + 0.12 * math.cos(k * 1.6)  # (its rings)
            c = WORM_DARK + (WORM - WORM_DARK) * (0.2 + 0.8 * lam)[..., None]
            c = c * ring + (np.clip(lam, 0, 1) ** 14 * 70)[..., None]
            col[inside] = c[inside]
            cov[inside] = 1.0

    def down(x):
        return x.reshape(32, S, 32, S, *x.shape[2:]).mean(axis=(1, 3))
    c32, a32 = down(col * cov[..., None]), down(cov)
    with np.errstate(invalid="ignore", divide="ignore"):
        c32 = np.where(a32[..., None] > 0, c32 / a32[..., None], 0)
    return c32, a32


def worm_icon():
    """A bottle of worms: soil to two thirds up (the potion's liquid made earth, grainy), pink worms against the glass
    and one over the top."""
    potion = np.asarray(Image.open(POTION).convert("RGBA"))
    empty = np.asarray(Image.open(BOTTLE).convert("RGBA")).astype(float)
    mask = liquid_mask(potion)
    out = potion.astype(float).copy()
    r = out[..., 0]
    sheen = np.minimum(out[..., 1], out[..., 2])
    lo, hi = np.percentile(r[mask], 3), np.percentile(r[mask], 97)
    t = np.clip((r - lo) / max(hi - lo, 1.0), 0.0, 1.0)[..., None]
    rnd = np.random.RandomState(7)
    grain = rnd.uniform(-1, 1, (32, 32, 1))
    col = np.where(t < 0.6, SOIL_DARK + (SOIL_MID - SOIL_DARK) * (t / 0.6),
                   SOIL_MID + (SOIL_LIGHT - SOIL_MID) * ((t - 0.6) / 0.4))
    col = col * (1 + 0.22 * grain) + sheen[..., None] * np.array([0.8, 0.8, 0.85]) * 0.8
    out[..., :3] = np.where(mask[..., None], np.clip(col, 0, 255), out[..., :3])
    rows = np.arange(32)[:, None] + 0.5
    above = mask & (rows < SOIL_TOP)
    out[above] = empty[above]
    # the soil's crumbly top
    top = mask & (np.abs(rows - (SOIL_TOP + 0.5)) < 0.75)
    out[..., :3] = np.where(top[..., None], out[..., :3] * 0.5 + SOIL_LIGHT * 0.5 * (1 + 0.3 * grain), out[..., :3])
    c32, a32 = worm_layer()
    inside = (mask | above)[..., None]
    k = np.clip(a32 * 1.3, 0, 1)[..., None] * inside * 0.95
    out[..., :3] = out[..., :3] * (1 - k) + c32 * k
    out[..., 3] = np.where(mask, np.maximum(out[..., 3], 240), out[..., 3])
    return Image.fromarray(np.clip(out + 0.5, 0, 255).astype(np.uint8), "RGBA")


def main():
    os.makedirs(OUT_ART, exist_ok=True)
    for count in (3, 2, 1):
        img = icon(count)
        img.save(os.path.join(OUT_ART, f"bottle_larva_{count}.png"))
        os.makedirs(os.path.join(ROOT, "fishmod", "out"), exist_ok=True)
        img.resize((128, 128), Image.NEAREST).save(os.path.join(ROOT, "fishmod", "out", f"bottle_larva_{count}_x4.png"))
    worm_icon().save(os.path.join(OUT_ART, "bottle_worm.png"))
    entries = [{"name": f"gFishmodBottleLarva{c}Tex", "src": f"src/fishmod_data/art/bottle_larva_{c}.png",
                "format": "rgba32", "out": OUT_C} for c in (3, 2, 1)]
    entries.append({"name": "gFishmodBottleWormTex", "src": "src/fishmod_data/art/bottle_worm.png", "format": "rgba32",
                    "out": OUT_C})
    with open(MANIFEST, "w") as f:
        json.dump({"textures": entries, "models": []}, f, indent=2)


if __name__ == "__main__":
    main()
