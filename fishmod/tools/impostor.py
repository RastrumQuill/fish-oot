#!/usr/bin/env python3
"""The far plants' single card: a picture of the whole plant from the side, rendered from its own mesh, texture and
vertex colours (as the game shades it: texel times vertex colour), for one card the game turns to face the camera
(fishmod_scatter.c, the billboard tier). The card is the one clamped triangle that holds the picture
(card_outline.triangle).

impostor(kind, mesh, texture, out_dir) -> {"png", "obj", "height"} or None. Used by gen_habitat.py.
"""
import math
import os

import numpy as np
from PIL import Image

SIZE = 32          # the picture's longer side (texels): far plants are a few dozen pixels on screen at most
SUPER = 4          # supersampling per texel side
YAWS = range(0, 180, 15)  # views tried: the one showing the most of the plant is kept


def load_obj(path):
    pos, uv, col, tris = [], [], [], []
    for line in open(path):
        p = line.split()
        if not p:
            continue
        if p[0] == "v":
            pos.append([float(x) for x in p[1:4]])
            col.append([float(x) for x in p[4:7]] if len(p) >= 7 else [1.0, 1.0, 1.0])
        elif p[0] == "vt":
            uv.append([float(p[1]), float(p[2])])
        elif p[0] == "f":
            c = [[int(i) - 1 if i else -1 for i in (s.split("/") + ["", ""])[:2]] for s in p[1:]]
            for i in range(1, len(c) - 1):
                tris.append((c[0], c[i], c[i + 1]))
    return np.array(pos), np.array(uv) if uv else np.zeros((1, 2)), np.array(col), tris


def render(pos, uv, col, tris, tex, yaw, texel, origin, size):
    """Orthographic view along -z after turning the plant by yaw: RGBA image (supersampled), rows down."""
    W, H = size[0] * SUPER, size[1] * SUPER
    c, s = math.cos(yaw), math.sin(yaw)
    x = pos[:, 0] * c + pos[:, 2] * s
    z = -pos[:, 0] * s + pos[:, 2] * c
    px = (x - origin[0]) / texel * SUPER
    py = (origin[1] - pos[:, 1]) / texel * SUPER
    img = np.zeros((H, W, 4))
    depth = np.full((H, W), -1e9)
    th, tw = tex.shape[:2]
    for (a, ta), (b, tb), (cc, tc) in tris:
        P = np.array([[px[a], py[a]], [px[b], py[b]], [px[cc], py[cc]]])
        x0, y0 = np.floor(P.min(0)).astype(int)
        x1, y1 = np.ceil(P.max(0)).astype(int)
        x0, y0, x1, y1 = max(x0, 0), max(y0, 0), min(x1, W - 1), min(y1, H - 1)
        if x0 > x1 or y0 > y1:
            continue
        gx, gy = np.meshgrid(np.arange(x0, x1 + 1) + 0.5, np.arange(y0, y1 + 1) + 0.5)
        d = (P[1, 1] - P[2, 1]) * (P[0, 0] - P[2, 0]) + (P[2, 0] - P[1, 0]) * (P[0, 1] - P[2, 1])
        if abs(d) < 1e-9:
            continue
        w0 = ((P[1, 1] - P[2, 1]) * (gx - P[2, 0]) + (P[2, 0] - P[1, 0]) * (gy - P[2, 1])) / d
        w1 = ((P[2, 1] - P[0, 1]) * (gx - P[2, 0]) + (P[0, 0] - P[2, 0]) * (gy - P[2, 1])) / d
        w2 = 1 - w0 - w1
        inside = (w0 >= 0) & (w1 >= 0) & (w2 >= 0)
        if not inside.any():
            continue
        ws = (w0[inside], w1[inside], w2[inside])
        uvs = [uv[t] if t >= 0 else np.zeros(2) for t in (ta, tb, tc)]
        u = sum(w * q[0] for w, q in zip(ws, uvs))
        v = sum(w * q[1] for w, q in zip(ws, uvs))
        zz = sum(w * z[i] for w, i in zip(ws, (a, b, cc)))
        vc = sum(w[:, None] * col[i][None, :] for w, i in zip(ws, (a, b, cc)))
        # (wrapped, as the game samples the tall cards' repeating texture)
        tx = np.floor((u % 1.0) * tw).astype(int) % tw
        ty = np.floor(((1.0 - v) % 1.0) * th).astype(int) % th
        texel_rgba = tex[ty, tx]
        keep = texel_rgba[:, 3] >= 128
        yy, xx = gy[inside].astype(int)[keep], gx[inside].astype(int)[keep]
        zz = zz[keep]
        nearer = zz > depth[yy, xx]
        yy, xx, zz = yy[nearer], xx[nearer], zz[nearer]
        rgb = texel_rgba[keep][nearer][:, :3] * vc[keep][nearer]
        depth[yy, xx] = zz
        img[yy, xx, :3] = rgb
        img[yy, xx, 3] = 255
    return img


def impostor(kind, mesh, texture, out_dir):
    pos, uv, col, tris = load_obj(mesh)
    tex = np.asarray(Image.open(texture).convert("RGBA")).astype(float)
    if len(tris) == 0:
        return None
    # the plant's reach from its stem in any direction, and its height: the picture's frame (the same for every view)
    r = np.sqrt(pos[:, 0] ** 2 + pos[:, 2] ** 2).max()
    y0, y1 = pos[:, 1].min(), pos[:, 1].max()
    width, height = 2 * r, y1 - y0
    # power-of-two sides, the longer one SIZE, a texel's margin all round so the clamped edge is see-through
    texel = max(width, height) / (SIZE - 2)
    W = 1 << max(3, math.ceil(math.log2(width / texel + 2)))
    H = 1 << max(3, math.ceil(math.log2(height / texel + 2)))
    origin = (-W * texel / 2, y1 + (H * texel - height) / 2)
    best = None
    for yaw in YAWS:
        img = render(pos, uv, col, tris, tex, math.radians(yaw), texel, origin, (W, H))
        cover = (img[:, :, 3] > 0).sum()
        if best is None or cover > best[0]:
            best = (cover, img)
    img = best[1]
    # down to the texels: colour averaged over the covered samples, opaque where at least half the samples are
    big = img.reshape(H, SUPER, W, SUPER, 4)
    a = big[:, :, :, :, 3].mean((1, 3))
    covered = (big[:, :, :, :, 3] > 0).sum((1, 3))
    rgb = big[:, :, :, :, :3].sum((1, 3)) / np.maximum(covered, 1)[:, :, None]
    out = np.zeros((H, W, 4), np.uint8)
    out[:, :, :3] = np.clip(rgb, 0, 255)
    out[:, :, 3] = np.where(a >= 127.5, 255, 0)
    out[0, :, 3] = out[-1, :, 3] = out[:, 0, 3] = out[:, -1, 3] = 0  # the see-through margin (clamped)
    if out[:, :, 3].max() == 0:
        return None
    os.makedirs(out_dir, exist_ok=True)
    png = os.path.join(out_dir, kind + "_imp.png")
    Image.fromarray(out).save(png)

    import card_outline
    t = card_outline.triangle(png)
    if t is not None:
        poly = [tuple(p) for p in t[0]]
    else:
        poly = [(0, 0), (W, 0), (W, H), (0, H)]
    # texel (u right, v down) -> model (x right, y up, z 0): the card faces +z
    obj = os.path.join(out_dir, kind + "_imp.obj")
    with open(obj, "w") as f:
        f.write(f"# {kind}: the far single card (fishmod/tools/impostor.py)\n")
        for u, v in poly:
            f.write(f"v {origin[0] + u * texel:.3f} {origin[1] - v * texel:.3f} 0 1 1 1\n")
        for u, v in poly:
            f.write(f"vt {u / W:.6f} {1 - v / H:.6f}\n")
        f.write("f " + " ".join(f"{i + 1}/{i + 1}" for i in range(len(poly))) + "\n")
    return {"png": png, "obj": obj, "height": height, "size": (W, H), "triangles": len(poly) - 2}


if __name__ == "__main__":
    import sys
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import gen_habitat as gh
    out = sys.argv[1] if len(sys.argv) > 1 else "/tmp/imp"
    for kind, tex, fmt in gh.PROP_KINDS:
        if kind in gh.IMPOSTOR_KINDS:
            r = impostor(kind, os.path.join(gh.MESH, kind + ".obj"), gh.texture_path(tex), out)
            print(kind, r and (r["size"], round(r["height"]), r["triangles"]))
