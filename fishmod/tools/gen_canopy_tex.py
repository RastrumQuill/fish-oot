#!/usr/bin/env python3
"""Turn a photo of ground cover into a tiling I4 canopy texture (128x64, 16 greys, 4 KB).

Steps: crop a near-top-down region, flatten the lighting, tone down standout
leaves (they make the repeat obvious), make it wrap in both directions with
min-cut seams, shrink to 128x64 and quantise to 16 levels. The game tints it
with prim/env colour, so only brightness is kept.

  gen_canopy_tex.py photo.jpg out.i4.png --width 480 --overlap 56 --x 250 --y 420 --aspect 1.3
--width is the source pixels that become one tile (128 texels); height is half of it.
"""
import argparse
import numpy as np
from PIL import Image, ImageFilter

TEX_W, TEX_H = 128, 64


def blur(a, r):
    lo, rng = a.min(), max(a.max() - a.min(), 1e-6)
    b = Image.fromarray(((a - lo) / rng * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(r))
    return np.asarray(b).astype(np.float32) / 255 * rng + lo


def min_cut(cost):
    """Cheapest top-to-bottom path through cost (rows x cols); returns one column per row."""
    r, c = cost.shape
    acc = cost.copy()
    back = np.zeros((r, c), int)
    for i in range(1, r):
        for j in range(c):
            lo, hi = max(0, j - 1), min(c, j + 2)
            k = lo + int(np.argmin(acc[i - 1, lo:hi]))
            back[i, j] = k
            acc[i, j] += acc[i - 1, k]
    seam = np.zeros(r, int)
    seam[-1] = int(np.argmin(acc[-1]))
    for i in range(r - 1, 0, -1):
        seam[i - 1] = back[i, seam[i]]
    return seam


def wrap_x(a, w, ov):
    """a has w+ov columns; returns w columns that wrap left-right."""
    left = a[:, w:w + ov]   # continues column w-1 past the right edge
    right = a[:, 0:ov]      # continues into column ov
    seam = min_cut((left - right) ** 2)
    out = a[:, :w].copy()
    m = np.clip((np.arange(ov)[None, :] - seam[:, None] + 1.5) / 3.0, 0, 1)  # 3 px feather
    out[:, :ov] = left * (1 - m) + right * m
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("photo")
    ap.add_argument("out")
    ap.add_argument("--width", type=int, default=480)
    ap.add_argument("--overlap", type=int, default=56)
    ap.add_argument("--x", type=int, default=250)
    ap.add_argument("--y", type=int, default=420)
    ap.add_argument("--aspect", type=float, default=1.0,
                    help="vertical stretch to undo the photo's camera angle (leaves look wide when shot at a slant)")
    ap.add_argument("--tame", type=float, default=0.45, help="how much to flatten standout leaves")
    ap.add_argument("--floor", type=float, default=0.10, help="darkest grey, so gaps are not pure black")
    a = ap.parse_args()

    w, h, ov = a.width, a.width // 2, a.overlap
    src_h = round((h + ov) / a.aspect)
    src = Image.open(a.photo).convert("RGB").crop((a.x, a.y, a.x + w + ov, a.y + src_h))
    src = src.resize((w + ov, h + ov), Image.BICUBIC)
    rgb = np.asarray(src).astype(np.float32)
    lum = rgb[..., 0] * 0.30 + rgb[..., 1] * 0.60 + rgb[..., 2] * 0.10

    low = blur(lum, 70)                    # sun patches, distance falloff
    lum = lum - low + low.mean()
    mid = blur(lum, 18)                    # single bright or dark leaves
    lum = lum - a.tame * (mid - mid.mean())

    t = wrap_x(lum, w, ov)
    t = wrap_x(t.T, h, ov).T
    lo, hi = np.percentile(t, 1), np.percentile(t, 99.5)
    t = np.clip((t - lo) / (hi - lo), 0, 1)

    small = Image.fromarray((t * 255).astype(np.uint8)).resize((TEX_W, TEX_H), Image.LANCZOS)
    small = small.filter(ImageFilter.UnsharpMask(1, 60, 2))
    s = np.asarray(small).astype(np.float32) / 255
    s = a.floor + (1 - a.floor) * s
    q = np.clip(np.round(s * 15), 0, 15).astype(np.uint8)
    Image.fromarray(q * 17).save(a.out)
    print(f"{a.out}: {TEX_W}x{TEX_H} I4, {np.unique(q).size} levels")


if __name__ == "__main__":
    main()
