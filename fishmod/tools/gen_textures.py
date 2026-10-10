#!/usr/bin/env python3
"""
Generates per-species fish textures from the Fishing Pond textures and the "textures" section of each
species' "look" in fishmod/data/species.json.

  src/overlays/actors/ovl_En_Fishmod/fishmod_textures.inc.c   texture data + per-species table (C)
  fishmod/out/textures_preview.png                           all generated textures, enlarged

Each texture role (back, belly, head, side, fin) maps to one texture of the base model. A species lists
pattern operations per role; roles it does not list keep the original texture.

Operations (applied in order, coordinates are 0..1 across the texture):
  {"op": "recolor", "dark": [r,g,b], "light": [r,g,b]}     map brightness onto a dark..light gradient
  {"op": "bars", "count": n, "color": [r,g,b], "strength": 0..1, "width": 0..1, "axis": "u"|"v",
   "fade": "none"|"top"|"bottom"}                           evenly spaced soft bars
  {"op": "spots", "count": n, "radius": r, "color": [r,g,b], "strength": 0..1, "seed": n,
   "region": [u0, v0, u1, v1]}                               random round spots
  {"op": "blotches", "count": n, "radius": r, "color": [r,g,b], "seed": n, "region": [...]}
                                                              large irregular patches (hard edged)
  {"op": "spot", "at": [u, v], "radius": r, "color": [r,g,b], "strength": 0..1}   one spot
  {"op": "brightness", "factor": f}
Every op also takes "exclude": [u0, v0, u1, v1] to leave a region (like the eye) untouched; the head
roles exclude the eye automatically.

Usage: fishmod/tools/gen_textures.py
"""

import json
import math
import os
import random

from PIL import Image

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
DATA = os.path.join(ROOT, "fishmod", "data", "species.json")
OBJ = os.path.join(ROOT, "extracted", "ntsc-1.2", "assets", "objects", "object_fish")
KEEP = os.path.join(ROOT, "extracted", "ntsc-1.2", "assets", "objects", "gameplay_keep")
OUT_C = os.path.join(ROOT, "src", "overlays", "actors", "ovl_En_Fishmod", "fishmod_textures.inc.c")
OUT_PREVIEW = os.path.join(ROOT, "fishmod", "out", "textures_preview.png")

# role -> (texture symbol) per base model; must match sTextureRoles in z_en_fishmod.c
ROLES = ["back", "belly", "head", "side", "fin"]
BASE_TEXTURES = {
    "bass": {
        "back": "gFishingFishTex_0016D0",
        "belly": "gFishingFishTex_0018D0",
        "head": "gFishingFishTex_001AD0",
        "side": "gFishingFishTex_0014D0",  # the bass has no side texture; this slot is its tail fin
        "fin": "gFishingFishTex_0022D0",
    },
    "loach": {
        "back": "gFishingLoachTex_00ED28",
        "belly": "gFishingLoachTex_00F528",
        "side": "gFishingLoachTex_00FD28",
        "head": "gFishingLoachTex_010528",
        "fin": "gFishingLoachTex_010DA8",
    },
    "small": {
        "back": "gFishBodyTex",
        "side": "gFishTailTex",  # tail fin, as for the bass
    },
}


def load(sym):
    path = os.path.join(OBJ, sym + ".rgba16.png")
    if not os.path.exists(path):
        path = os.path.join(KEEP, sym + ".rgba16.png")
    return Image.open(path).convert("RGBA")


def lum(c):
    return (0.299 * c[0] + 0.587 * c[1] + 0.114 * c[2]) / 255.0


def mix(a, b, t):
    return tuple(int(round(a[i] + (b[i] - a[i]) * t)) for i in range(3))


# Eye position in each head texture, kept as is by every op.
EYES = {"gFishingFishTex_001AD0": [0.62, 0.0, 0.95, 0.32], "gFishingLoachTex_010528": [0.25, 0.4, 0.47, 0.72]}


def apply(img, ops, sym=None):
    w, h = img.size
    px = img.load()
    for op in ops:
        kind = op["op"]
        excludes = []
        if op.get("exclude"):
            excludes.append(op["exclude"])
        if sym in EYES:
            excludes.append(EYES[sym])
        before = img.copy()
        if kind == "recolor":
            for y in range(h):
                for x in range(w):
                    r, g, b, a = px[x, y]
                    c = mix(op["dark"], op["light"], lum((r, g, b)))
                    px[x, y] = (*c, a)
        elif kind == "brightness":
            f = op["factor"]
            for y in range(h):
                for x in range(w):
                    r, g, b, a = px[x, y]
                    px[x, y] = (min(255, int(r * f)), min(255, int(g * f)), min(255, int(b * f)), a)
        elif kind == "bars":
            n = op["count"]
            width = op.get("width", 0.35)
            strength = op.get("strength", 0.6)
            axis = op.get("axis", "u")
            fade = op.get("fade", "none")
            for y in range(h):
                for x in range(w):
                    t = (x + 0.5) / w if axis == "u" else (y + 0.5) / h
                    other = (y + 0.5) / h if axis == "u" else (x + 0.5) / w
                    phase = (t * n) % 1.0
                    d = abs(phase - 0.5) * 2  # 0 at bar center
                    k = max(0.0, 1.0 - d / max(width, 0.01))
                    k = min(1.0, k * 1.5)
                    if fade == "top":
                        k *= 1.0 - other
                    elif fade == "bottom":
                        k *= other
                    r, g, b, a = px[x, y]
                    c = mix((r, g, b), op["color"], k * strength)
                    px[x, y] = (*c, a)
        elif kind in ("spots", "blotches", "spot"):
            rnd = random.Random(op.get("seed", 1))
            region = op.get("region", [0, 0, 1, 1])
            if kind == "spot":
                centers = [tuple(op["at"])]
            else:
                centers = [
                    (rnd.uniform(region[0], region[2]), rnd.uniform(region[1], region[3])) for _ in range(op["count"])
                ]
            radius = op.get("radius", 0.1)
            strength = op.get("strength", 1.0 if kind == "blotches" else 0.7)
            for cu, cv in centers:
                # blotches get a lumpy outline
                lumps = [rnd.uniform(0.7, 1.3) for _ in range(8)]
                for y in range(h):
                    for x in range(w):
                        u = (x + 0.5) / w
                        v = (y + 0.5) / h
                        du, dv = u - cu, v - cv
                        dist = math.hypot(du, dv)
                        rr = radius
                        if kind == "blotches":
                            ang = (math.atan2(dv, du) + math.pi) / (2 * math.pi) * 8
                            i0 = int(ang) % 8
                            fr = ang - int(ang)
                            rr = radius * (lumps[i0] * (1 - fr) + lumps[(i0 + 1) % 8] * fr)
                            k = 1.0 if dist < rr else 0.0
                        else:
                            k = max(0.0, 1.0 - dist / rr)
                            k = min(1.0, k * 2.0)
                        if k > 0:
                            r, g, b, a = px[x, y]
                            c = mix((r, g, b), op["color"], k * strength)
                            px[x, y] = (*c, a)
        else:
            raise SystemExit(f"unknown texture op {kind}")
        if excludes:
            bpx = before.load()
            for y in range(h):
                for x in range(w):
                    u = (x + 0.5) / w
                    v = (y + 0.5) / h
                    for e in excludes:
                        if e[0] <= u <= e[2] and e[1] <= v <= e[3]:
                            px[x, y] = bpx[x, y]
    return img


def to_rgba16_words(img):
    data = []
    w, h = img.size
    px = img.load()
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            v = ((r >> 3) << 11) | ((g >> 3) << 6) | ((b >> 3) << 1) | (1 if a >= 128 else 0)
            data.append(v)
    return data


def main():
    with open(DATA) as f:
        data = json.load(f)

    out = [
        "/* Generated by fishmod/tools/gen_textures.py from fishmod/data/species.json. Do not edit. */",
        "",
    ]
    table = []
    previews = []
    for sp in data["species"]:
        textures = sp.get("look", {}).get("textures", {})
        row = []
        for role in ROLES:
            ops = textures.get(role)
            sym = BASE_TEXTURES.get(sp["base"], {}).get(role)
            if not ops or not sym:
                row.append("NULL")
                continue
            img = apply(load(sym), ops, sym)
            name = f"sTex_{sp['id']}_{role}"
            words = to_rgba16_words(img)
            out.append(f"static u16 {name}[{len(words)}] ALIGNED(8) = {{")
            for i in range(0, len(words), 16):
                out.append("    " + ", ".join(f"0x{v:04X}" for v in words[i:i + 16]) + ",")
            out.append("};")
            out.append("")
            row.append(name)
            previews.append((sp["id"] + " " + role, load(sym), img))
        table.append((sp["id"], row))

    out.append("// Per species, per texture role (back, belly, head, side, fin); NULL keeps the pond texture.")
    out.append("static u16* sSpeciesTextures[FISHMOD_SPECIES_MAX][FISHMOD_TEX_ROLE_MAX] = {")
    for sid, row in table:
        out.append(f"    /* FISHMOD_SPECIES_{sid} */ {{ {', '.join(row)} }},")
    out.append("};")
    out.append("")
    with open(OUT_C, "w") as f:
        f.write("\n".join(out))

    if previews:
        from PIL import ImageDraw

        cell = 96
        sheet = Image.new("RGB", (cell * 2 + 220, (cell + 8) * len(previews)), (60, 60, 60))
        d = ImageDraw.Draw(sheet)
        for i, (label, before, after) in enumerate(previews):
            y = i * (cell + 8)
            for j, im in enumerate((before, after)):
                bg = Image.new("RGBA", im.size, (255, 0, 255, 255))
                bg.alpha_composite(im)
                sheet.paste(bg.convert("RGB").resize((cell, cell), Image.NEAREST), (j * (cell + 4), y))
            d.text((cell * 2 + 16, y + cell // 2), label, fill=(255, 255, 255))
        os.makedirs(os.path.dirname(OUT_PREVIEW), exist_ok=True)
        sheet.save(OUT_PREVIEW)
    print(f"generated {len(previews)} textures")


if __name__ == "__main__":
    main()
