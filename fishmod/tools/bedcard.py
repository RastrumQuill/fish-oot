#!/usr/bin/env python3
"""Bed cards: each scatter chunk's distant plants as one card (at build time).

The far plants already become one card each (impostor.py); past the bed distance a whole chunk's card-drawn plants
become one card instead: the RDP pays a card's spans once on a line, not once per plant. Each chunk with enough of them
is rendered (impostor.render: the plants' own meshes, textures, vertex colours and tints, as the game shades them) from
four directions 45 degrees apart; the game lays the card in the plane of the view nearest its own direction to the
chunk (from the other side the double-sided card shows that view's mirror image, as it should), so four views cover
all eight. Each view is a 48 x 24 texture of 16 colours (CI4; colour 0 see-through).

Writes src/fishmod_data/fishmod_bc_<scene>.c (a ROM file: the game loads the card table and streams the textures of
the chunks it needs into a small cache; the scenes are listed in include/fishmod_bedcard_scenes.h). Not committed
(made from the game's textures).
Usage: fishmod/tools/bedcard.py [--preview DIR]
"""
import argparse
import math
import os
import re
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_habitat as gh  # noqa: E402
import impostor  # noqa: E402
import prop_kinds as pk  # noqa: E402

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
DATA = os.path.join(ROOT, "src", "fishmod_data")
SCENES = {"SCENE_LAKE_HYLIA": "spot06"}
MIN_PLANTS = 8   # a chunk with fewer card-drawn plants keeps them as they are
VIEWS = 4        # 0, 45, 90, 135 degrees
TEX_W, TEX_H = 48, 24
VIEW_BYTES = 32 + TEX_W * TEX_H // 2  # palette (16 RGBA5551) + texels (4 bits)


def read_scatter(base):
    src = open(os.path.join(DATA, f"fishmod_sc_{base}.c")).read()
    head = re.search(r"\{ FISHMOD_SCATTER_MAGIC, (-?\d+), (-?\d+), (\d+), (\d+), (\d+), (\d+)", src)
    x0, z0, chunk, width, depth, count = map(int, head.groups())
    starts = list(map(int, re.search(r"\{ FISHMOD_SCATTER_MAGIC[^}]*\},\s*\{([^}]*)\}", src).group(1).split(",")))
    props = [tuple(map(int, m)) for m in
             re.findall(r"\{ (-?\d+), (-?\d+), (-?\d+), (\d+), (\d+), (\d+), (\d+), -?\d+, -?\d+ \}", src)]
    return dict(x0=x0, z0=z0, chunk=chunk, width=width, depth=depth, starts=starts, props=props)


_mesh_cache = {}


def kind_mesh(kind):
    if kind not in _mesh_cache:
        tex = dict((k, t) for k, t, _ in pk.GENERATED).get(kind)
        mesh = os.path.join(gh.MESH, kind + ".obj")
        tpath = gh.texture_path(tex) if tex else None
        if not (tex and os.path.exists(mesh) and tpath):
            _mesh_cache[kind] = None
        else:
            pos, uv, col, tris = impostor.load_obj(mesh)
            _mesh_cache[kind] = (pos, uv, col, tris, np.asarray(Image.open(tpath).convert("RGBA")).astype(float))
    return _mesh_cache[kind]


def card_kinds():
    """The kinds drawn as single cards far away (the game's FISHMOD_PROP_KINDS_IMP: those with a card made)."""
    return {k for k in pk.flagged("impostor") if os.path.exists(os.path.join(gh.MESH_IMP, k + "_imp.png"))}


def render_chunk(members):
    """members: (kind, x, y, z, yaw, scale, tint) relative to the card's base centre. Returns (views RGBA, half
    width, height) or None."""
    by_tex = {}
    reach = 0.0
    y_top = 0.0
    for kind, x, y, z, yaw, scale, tint in members:
        m = kind_mesh(kind)
        if m is None:
            continue
        pos, uv, col, tris, tex = m
        a = yaw * 2 * math.pi / 256
        c, s = math.cos(a), math.sin(a)
        sc = scale / 64.0
        p = pos * sc
        # the game's prop matrix: a turn about y (OoT's: x' = x cos + z sin, z' = -x sin + z cos)
        px = p[:, 0] * c + p[:, 2] * s + x
        pz = -p[:, 0] * s + p[:, 2] * c + z
        world = np.stack([px, p[:, 1] + y, pz], axis=1)
        f = min(255, tint * 2) / 255.0  # the per-prop tint (the game's primitive colour without the light)
        by_tex.setdefault(kind, []).append((world, uv, col * f, tris, tex))
        reach = max(reach, float(np.sqrt(px ** 2 + pz ** 2).max()))
        y_top = max(y_top, float(world[:, 1].max()))
    if not by_tex or y_top <= 0:
        return None
    # texels as tall as the plants need (all 24 rows), rendered that fine across too, then squeezed to 48 columns:
    # the game stretches the card to the chunk's real width and height
    texel = y_top / (TEX_H - 2)
    fine_w = int(math.ceil(2 * reach / texel)) + 2
    origin = (-fine_w * texel / 2, y_top + texel)
    views = []
    for v in range(VIEWS):
        yaw = math.radians(v * 180.0 / VIEWS)
        img = np.zeros((TEX_H * impostor.SUPER, fine_w * impostor.SUPER, 4))
        depth = None
        # each kind's meshes drawn with its own texture; nearer pixels win across kinds (one depth test per render)
        layers = []
        for kind, items in by_tex.items():
            for world, uv, col, tris, tex in items:
                layers.append(impostor.render(world, uv, col, tris, tex, yaw, texel, origin, (fine_w, TEX_H)))
        for layer in layers:  # (no shared depth between renders: later ones fill only what's still empty)
            empty = img[:, :, 3] == 0
            img[empty] = layer[empty]
        views.append(img)
    return views, reach, y_top


def squeeze(img):
    """The supersampled render (any width) averaged down to TEX_H x TEX_W texels: (alpha share, colour of the covered
    samples)."""
    h, w = img.shape[:2]
    a = np.zeros((TEX_H, TEX_W))
    rgb = np.zeros((TEX_H, TEX_W, 3))
    rows = np.linspace(0, h, TEX_H + 1).astype(int)
    cols = np.linspace(0, w, TEX_W + 1).astype(int)
    for j in range(TEX_H):
        for i in range(TEX_W):
            blk = img[rows[j]:rows[j + 1], cols[i]:cols[i + 1]]
            cov = blk[:, :, 3] > 0
            a[j, i] = cov.mean() * 255 if blk.size else 0
            if cov.any():
                rgb[j, i] = blk[cov][:, :3].mean(0)
    return a, rgb


def to_ci4(img):
    """Supersampled RGBA -> (palette RGBA5551 x16, 4-bit indices), colour 0 see-through."""
    a, rgb = squeeze(img)
    opaque = a >= 127.5
    opaque[0, :] = opaque[-1, :] = opaque[:, 0] = opaque[:, -1] = False  # the see-through margin (clamped)
    pal = [(0, 0, 0)] * 16
    idx = np.zeros((TEX_H, TEX_W), np.uint8)
    if opaque.any():
        pix = np.clip(rgb[opaque], 0, 255).astype(np.uint8)
        q = Image.fromarray(pix.reshape(-1, 1, 3)).quantize(colors=15, method=Image.Quantize.MEDIANCUT)
        qp = q.getpalette()[:45]
        for i in range(15):
            pal[i + 1] = tuple(qp[i * 3:i * 3 + 3]) if i * 3 + 2 < len(qp) else (0, 0, 0)
        idx[opaque] = np.asarray(q).reshape(-1) + 1
    tlut = []
    for i, (r, g, b) in enumerate(pal):
        tlut.append(0 if i == 0 else ((r >> 3) << 11) | ((g >> 3) << 6) | ((b >> 3) << 1) | 1)
    packed = (idx[:, 0::2] << 4) | idx[:, 1::2]
    return tlut, packed.reshape(-1), pal, idx


def build(scene, base, preview):
    sc = read_scatter(base)
    kinds = card_kinds()
    cards = []
    blobs = []
    for c in range(sc["width"] * sc["depth"]):
        ps = sc["props"][sc["starts"][c]:sc["starts"][c + 1]]
        members = [p for p in ps if pk.GAME[p[3]] in kinds]
        if len(members) < MIN_PLANTS:
            continue
        cx = sum(p[0] for p in members) / len(members)
        cz = sum(p[2] for p in members) / len(members)
        y0 = min(p[1] for p in members)
        rel = [(pk.GAME[p[3]], p[0] - cx, p[1] - y0, p[2] - cz, p[4], p[5], p[6]) for p in members]
        r = render_chunk(rel)
        if r is None:
            continue
        views, reach, height = r
        blob = b""
        for v, img in enumerate(views):
            tlut, packed, pal, idx = to_ci4(img)
            blob += b"".join(t.to_bytes(2, "big") for t in tlut) + bytes(packed.tolist())
            if preview:
                rgba = np.zeros((TEX_H, TEX_W, 4), np.uint8)
                for i in range(1, 16):
                    rgba[idx == i] = list(pal[i]) + [255]
                os.makedirs(preview, exist_ok=True)
                Image.fromarray(rgba).resize((TEX_W * 4, TEX_H * 4), Image.NEAREST).save(
                    os.path.join(preview, f"{base}_c{c:03d}_v{v}.png"))
        assert len(blob) == VIEWS * VIEW_BYTES
        cards.append((c, round(cx), round(cz), y0, round(reach), round(height), len(members)))
        blobs.append(blob)
    # the ROM file: a header, the card table, then each card's four views
    lines = [f"// Generated by fishmod/tools/bedcard.py from the scatter and the plants' textures. Do not edit.",
             '#include "fishmod_bedcard.h"', "",
             "typedef struct {", "    FishmodBedCardHeader header;",
             f"    FishmodBedCard cards[{max(len(cards), 1)}];",
             f"    u8 views[{max(len(cards), 1)}][{VIEWS * VIEW_BYTES}];", f"}} FishmodBedCards_{base};", "",
             f"FishmodBedCards_{base} gFishmodBedCards_{base} = {{",
             f"    {{ FISHMOD_BEDCARD_MAGIC, {len(cards)}, {VIEWS * VIEW_BYTES}, 0 }},", "    {"]
    for c, cx, cz, y0, reach, height, n in cards:
        lines.append(f"        {{ {c}, {cx}, {cz}, {y0}, {reach}, {height} }}, // {n} plants")
    if not cards:
        lines.append("        { -1, 0, 0, 0, 0, 0 },")
    lines += ["    },", "    {"]
    for blob in blobs or [bytes(VIEWS * VIEW_BYTES)]:
        hexes = ", ".join(f"0x{b:02X}" for b in blob)
        lines.append(f"        {{ {hexes} }},")
    lines += ["    },", "};", ""]
    with open(os.path.join(DATA, f"fishmod_bc_{base}.c"), "w") as f:
        f.write("\n".join(lines))
    return len(cards), sum(c[6] for c in cards)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--preview")
    args = ap.parse_args()
    scenes = []
    for scene, base in SCENES.items():
        n, plants = build(scene, base, args.preview)
        scenes.append((scene, base))
        print(f"{scene}: {n} bed cards ({plants} plants)")
    # (the scenes with cards are listed in include/fishmod_bedcard_scenes.h)

if __name__ == "__main__":
    main()
