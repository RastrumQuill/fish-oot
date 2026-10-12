#!/usr/bin/env python3
"""Renders the habitat props (fishmod/tools/gen_habitat.py) next to vanilla art, for review before they go in the
game: a style sheet.

A small software renderer: perspective, z-buffer, textures (greyscale cards tinted by vertex colour, colour
textures multiplied by vertex colour), 1-bit alpha like the N64's alpha compare, and vanilla props lit by their
normals. No filtering tricks: point sampling at about the size the game shows them.

Panels:
  1. vanilla props from object_fish (already in memory wherever fish are) and the new props, same camera;
  2. the new textures beside the vanilla textures they were sampled from (4x);
  3. a small arranged cluster (a weed bed by a drop-off) the way the sub-biome recipes place things.

Usage: fishmod/tools/style_sheet.py [out.png]     (default fishmod/docs/style-sheet-habitat.png; needs the
                                                   extracted assets, Pillow and numpy)
"""
import math
import os
import re
import sys

import numpy as np
from PIL import Image, ImageDraw

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
OBJ_FISH = os.path.join(ROOT, "extracted", "ntsc-1.2", "assets", "objects", "object_fish")
SCENES = os.path.join(ROOT, "extracted", "ntsc-1.2", "assets", "scenes")
ART = os.path.join(ROOT, "fishmod", "art", "habitat")
ART_ROM = os.path.join(ROOT, "fishmod", "art", "habitat_rom")  # edited from vanilla textures (not committed)
MESH = os.path.join(ROOT, "fishmod", "assets", "habitat")

BG_TOP = np.array([0x6e, 0x8f, 0x8a], float)  # the lake bed's teal (Lake Hylia's bed texture), lighter at the top
BG_BOT = np.array([0x29, 0x58, 0x58], float)
LIGHT = np.array([0.4, 0.8, 0.45])
LIGHT /= np.linalg.norm(LIGHT)


# ---------------------------------------------------------------------------------------------------------------
# Models: lists of triangles, each vertex (pos[3], uv[2], colour[3] 0-1), plus a texture and a mode


class Model:
    def __init__(self, tris, tex, mode, name):
        self.tris = tris    # list of [(pos, uv, col)] * 3
        self.tex = tex      # numpy HxWx4 float 0-1
        self.mode = mode    # "ia": tex intensity * vertex colour; "rgba": tex * vertex colour; "lit": tex * light
        self.name = name

    def moved(self, offset, scale=1.0, yaw=0.0):
        c, s = math.cos(yaw), math.sin(yaw)
        out = []
        for t in self.tris:
            nt = []
            for p, uv, col in t:
                x, y, z = p[0] * scale, p[1] * scale, p[2] * scale
                nt.append(((x * c + z * s + offset[0], y + offset[1], -x * s + z * c + offset[2]), uv, col))
            out.append(nt)
        return Model(out, self.tex, self.mode, self.name)


def load_png(path, mode=None):
    im = Image.open(path).convert("RGBA")
    a = np.asarray(im).astype(float) / 255.0
    if path.endswith(".png") and Image.open(path).mode in ("LA", "L"):
        a[..., 0] = a[..., 1] = a[..., 2] = np.asarray(Image.open(path).convert("LA"))[..., 0] / 255.0
    return a


def load_obj(name):
    path = os.path.join(MESH, name + ".obj")
    text = open(path).read()
    tex_name = re.search(r"texture: (\w+)", text).group(1)
    vs, vts, tris = [], [], []
    for line in text.splitlines():
        parts = line.split()
        if not parts:
            continue
        if parts[0] == "v":
            vs.append(tuple(float(x) for x in parts[1:]))
        elif parts[0] == "vt":
            vts.append((float(parts[1]), float(parts[2])))
        elif parts[0] == "f":
            idx = [tuple(int(i) for i in p.split("/")) for p in parts[1:]]
            tris.append([(vs[i - 1][:3], vts[t - 1], vs[i - 1][3:6]) for i, t in idx])
    path = os.path.join(ART_ROM, tex_name + ".png")
    if not os.path.exists(path):
        path = os.path.join(ART, tex_name + ".png")
    img = Image.open(path)
    mode = "ia" if img.mode in ("LA", "L") else "rgba"
    return Model(tris, load_png(path), mode, name)


def load_vanilla(model, tex, tex_w, tex_h):
    """A model from object_fish: its Vtx (s/t, normals) and the triangles of its display list."""
    vtx = []
    for m in re.finditer(r"VTX\(\s*(-?\w+),\s*(-?\w+),\s*(-?\w+),\s*(-?\w+),\s*(-?\w+),\s*(\w+),\s*(\w+),\s*(\w+)",
                         open(os.path.join(OBJ_FISH, model + "Vtx.inc.c")).read()):
        g = [int(x, 0) for x in m.groups()]
        n = [((v + 128) % 256) - 128 for v in g[5:8]]
        nl = math.sqrt(sum(v * v for v in n)) or 1
        vtx.append(((g[0], g[1], g[2]), (g[3] / 32.0 / tex_w, g[4] / 32.0 / tex_h), tuple(v / nl for v in n)))
    tris = []
    dl = open(os.path.join(OBJ_FISH, model + "DL.inc.c")).read()
    for m in re.finditer(r"gsSP2Triangles\((\d+), (\d+), (\d+), 0, (\d+), (\d+), (\d+), 0\)", dl):
        g = [int(x) for x in m.groups()]
        tris.append([vtx[g[0]], vtx[g[1]], vtx[g[2]]])
        tris.append([vtx[g[3]], vtx[g[4]], vtx[g[5]]])
    for m in re.finditer(r"gsSP1Triangle\((\d+), (\d+), (\d+), 0\)", dl):
        g = [int(x) for x in m.groups()]
        tris.append([vtx[g[0]], vtx[g[1]], vtx[g[2]]])
    return Model(tris, load_png(os.path.join(OBJ_FISH, tex)), "lit", model.replace("gFishing", "").replace("Model", ""))


# ---------------------------------------------------------------------------------------------------------------
# Rendering


def look_at(eye, target):
    f = np.array(target, float) - np.array(eye, float)
    f /= np.linalg.norm(f)
    r = np.cross(f, [0, 1, 0])
    r /= np.linalg.norm(r)
    u = np.cross(r, f)
    return np.array(eye, float), r, u, f


def render(models, w, h, eye, target, fov=40.0):
    color = np.zeros((h, w, 3))
    for y in range(h):
        color[y, :] = BG_TOP + (BG_BOT - BG_TOP) * (y / h)
    depth = np.full((h, w), np.inf)
    cam, r, u, f = look_at(eye, target)
    focal = (h / 2) / math.tan(math.radians(fov) / 2)

    def proj(p):
        d = np.array(p, float) - cam
        z = d.dot(f)
        return (w / 2 + d.dot(r) / z * focal, h / 2 - d.dot(u) / z * focal, z)

    for m in models:
        th, tw = m.tex.shape[:2]
        for tri in m.tris:
            ps = [proj(v[0]) for v in tri]
            if min(p[2] for p in ps) < 1:
                continue
            xs = [p[0] for p in ps]
            ys = [p[1] for p in ps]
            x0, x1 = max(0, int(min(xs))), min(w - 1, int(max(xs)) + 1)
            y0, y1 = max(0, int(min(ys))), min(h - 1, int(max(ys)) + 1)
            (ax, ay, az), (bx, by, bz), (cx, cy, cz) = ps
            area = (bx - ax) * (cy - ay) - (cx - ax) * (by - ay)
            if abs(area) < 1e-6 or x1 < x0 or y1 < y0:
                continue
            gx, gy = np.meshgrid(np.arange(x0, x1 + 1) + 0.5, np.arange(y0, y1 + 1) + 0.5)
            w0 = ((bx - gx) * (cy - gy) - (cx - gx) * (by - gy)) / area
            w1 = ((cx - gx) * (ay - gy) - (ax - gx) * (cy - gy)) / area
            w2 = 1 - w0 - w1
            inside = (w0 >= 0) & (w1 >= 0) & (w2 >= 0)
            if not inside.any():
                continue
            # perspective-correct interpolation
            iz = w0 / az + w1 / bz + w2 / cz
            zz = 1 / iz

            def interp(k):
                a, b, c = (np.array(v[k], float) for v in tri)
                return ((w0 / az)[..., None] * a + (w1 / bz)[..., None] * b + (w2 / cz)[..., None] * c) * zz[..., None]

            uv = interp(1)
            col = interp(2)
            tu = (np.floor(uv[..., 0] * tw).astype(int)) % tw
            tv = (np.floor(uv[..., 1] * th).astype(int)) % th
            texel = m.tex[tv, tu]
            alpha = texel[..., 3] > 0.5
            if m.mode == "ia":
                rgb = texel[..., :3] * col
            elif m.mode == "rgba":
                rgb = texel[..., :3] * col
            else:
                n = col / (np.linalg.norm(col, axis=-1, keepdims=True) + 1e-6)
                lam = np.clip((n * LIGHT).sum(-1), 0, 1)
                rgb = texel[..., :3] * (0.55 + 0.45 * lam)[..., None]
            region = depth[y0:y1 + 1, x0:x1 + 1]
            mask = inside & alpha & (zz < region)
            region[mask] = zz[mask]
            color[y0:y1 + 1, x0:x1 + 1][mask] = np.clip(rgb[mask] * 255, 0, 255)
    return Image.fromarray(color.astype(np.uint8))


def bounds(models):
    pts = np.array([v[0] for m in models for t in m.tris for v in t], float)
    return pts.min(0), pts.max(0)


def tile(models, label, w=200, h=170):
    lo, hi = bounds(models)
    c = (lo + hi) / 2
    size = max(hi - lo) * 1.25 + 10
    eye = (c[0] + size * 0.75, c[1] + size * 0.45, c[2] + size * 1.0)
    im = render(models, w, h, eye, c)
    d = ImageDraw.Draw(im)
    d.rectangle([0, h - 14, w, h], fill=(0, 0, 0))
    d.text((3, h - 13), label, fill=(255, 255, 255))
    return im


def tex_tile(path, label, size=96):
    im = Image.open(path).convert("RGBA")
    s = size / max(im.size)
    im = im.resize((max(1, int(im.width * s)), max(1, int(im.height * s))), Image.NEAREST)
    bg = Image.new("RGBA", (size + 8, size + 20), (255, 0, 255, 255))
    bg.alpha_composite(im, (4, 4))
    d = ImageDraw.Draw(bg)
    d.text((3, size + 6), label[:17], fill=(255, 255, 255, 255))
    return bg.convert("RGB")


def grid(images, cols, pad=4, bg=(25, 25, 25)):
    w = max(i.width for i in images)
    h = max(i.height for i in images)
    rows = (len(images) + cols - 1) // cols
    out = Image.new("RGB", (cols * (w + pad) + pad, rows * (h + pad) + pad), bg)
    for k, im in enumerate(images):
        out.paste(im, (pad + (k % cols) * (w + pad), pad + (k // cols) * (h + pad)))
    return out


def heading(text, width):
    im = Image.new("RGB", (width, 22), (10, 10, 10))
    ImageDraw.Draw(im).text((6, 5), text, fill=(255, 230, 120))
    return im


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "fishmod", "docs", "style-sheet-habitat.png")
    vanilla = [
        load_vanilla("gFishingReedModel", "gFishingLilyPadTex.rgba16.png", 32, 32),
        load_vanilla("gFishingLilyPadModel", "gFishingLilyPadTex.rgba16.png", 32, 32),
        load_vanilla("gFishingRockModel", "gFishingRockTex.rgba16.png", 32, 32),
        load_vanilla("gFishingWoodPostModel", "gFishingWoodPostTex.rgba16.png", 32, 32),
    ]
    new_names = ["reed_clump", "ribbon_weed", "bush_weed", "sunken_log", "drowned_tree", "pebble_patch",
                 "silt_patch"]
    new = [load_obj(n) for n in new_names]
    stump = [load_obj("stump"), load_obj("stump_top")]
    flower = [load_obj("lily_flower"), load_obj("lily_centre")]
    pad_with_flower = [vanilla[1]] + [f.moved((4, 1, 4), 1.2) for f in flower]

    t1 = ([tile([m], "vanilla " + m.name) for m in vanilla] + [tile([m], "new " + m.name) for m in new] +
          [tile(stump, "new stump"), tile(flower, "new lily flower"), tile(pad_with_flower, "vanilla pad + flower")])
    panel1 = grid(t1, 5)

    v = os.path.join(SCENES, "misc", "turibori")
    s = os.path.join(SCENES, "overworld", "spot06")
    tex_pairs = [
        (os.path.join(v, "turibori_scene_00003CE0_Tex.rgba16.png"), "vanilla pond reeds"),
        (os.path.join(ART, "reed_card.png"), "new reed card"),
        (os.path.join(ART, "ribbon_card.png"), "new ribbon card"),
        (os.path.join(ART, "bush_card.png"), "new bush card"),
        (os.path.join(v, "turibori_scene_0000FCE0_Tex.rgba16.png"), "vanilla pebbles"),
        (os.path.join(ART_ROM, "pebbles.png"), "new pebbles (vanilla)"),
        (os.path.join(v, "turibori_scene_0000ECE0_Tex.rgba16.png"), "vanilla pond moss"),
        (os.path.join(ART, "silt.png"), "new silt decal"),
        (os.path.join(OBJ_FISH, "gFishingWoodPostTex.rgba16.png"), "vanilla wood post"),
        (os.path.join(ART_ROM, "bark.png"), "new bark (from post)"),
        (os.path.join(ART, "log_rings.png"), "new log rings"),
        (os.path.join(OBJ_FISH, "gFishingLilyPadTex.rgba16.png"), "vanilla lily pad"),
        (os.path.join(ROOT, "extracted", "ntsc-1.2", "assets", "objects", "object_dy_obj",
                      "gGreatFairyFlowerTex.ia16.png"), "vanilla fairy flower"),
        (os.path.join(ART_ROM, "lily_flower.png"), "new lily (from it)"),
        (os.path.join(s, "spot06_scene_000170B8_Tex.rgba16.png"), "vanilla lake bed"),
        (os.path.join(s, "spot06_scene_0000C2B8_Tex.rgba16.png"), "vanilla lake sand"),
    ]
    panel2 = grid([tex_tile(p, l) for p, l in tex_pairs], 8)

    # a weed bed by a drop-off: one hero (the log), secondaries (weed clumps), small pieces (pebbles, a rock)
    rock = vanilla[2]
    cluster = [
        load_obj("silt_patch").moved((0, 0, 0), 1.6),
        load_obj("sunken_log").moved((10, 0, 20), 0.8, 0.5),
        load_obj("ribbon_weed").moved((-60, 0, -30)), load_obj("ribbon_weed").moved((-40, 0, -60), 1.2, 1.0),
        load_obj("ribbon_weed").moved((-80, 0, -70), 0.9, 2.0),
        load_obj("bush_weed").moved((70, 0, -50), 1.0, 0.3), load_obj("bush_weed").moved((95, 0, -20), 0.8, 1.3),
        load_obj("pebble_patch").moved((-20, 1, 80), 0.8),
        rock.moved((-90, 2, 40), 0.35, 0.7), rock.moved((60, 2, 70), 0.25, 2.0),
        load_obj("stump").moved((120, 0, 60), 1.2), load_obj("stump_top").moved((120, 0, 60), 1.2),
    ]
    lo, hi = bounds(cluster)
    c = (lo + hi) / 2
    big = render(cluster, panel1.width - 8, 300, (c[0] + 160, c[1] + 150, c[2] + 300), (c[0], c[1] + 10, c[2]), 45)

    width = max(panel1.width, panel2.width, big.width + 8)
    parts = [heading("1. Vanilla props (object_fish) and the new props, same camera", width), panel1,
             heading("2. New textures next to the vanilla textures they were sampled from (4x)", width), panel2,
             heading("3. A weed bed cluster: hero log, ribbon weed and bushes, pebbles, vanilla rocks, a stump", width),
             big]
    sheet = Image.new("RGB", (width, sum(p.height for p in parts)), (25, 25, 25))
    y = 0
    for p in parts:
        sheet.paste(p, (0 if p is not big else 4, y))
        y += p.height
    sheet.save(out)
    print(f"wrote {out} ({sheet.width}x{sheet.height})")


if __name__ == "__main__":
    main()
