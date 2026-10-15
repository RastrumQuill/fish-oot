#!/usr/bin/env python3
"""Outlines for the plant cards: for each cut-out texture, a polygon of a few vertices that holds every texel that
can show (the opaque ones and a texel round them, which the N64's filtering can blend into the edge), and as little
else as it can. The cards are drawn as these polygons instead of whole rectangles, so the RDP no longer fills the
see-through parts of the rectangle: the same picture, fewer pixels. The more it searches, the tighter it gets.

The search is simulated annealing over polygons on the texel grid: vertices moved, added and removed, kept only
while the polygon stays simple and holds every texel that must show; the score is its area plus a cost per vertex
(more vertices are more work for the RSP and the RDP's triangle setup). Results are cached in
src/fishmod_data/card_outlines.json (made from the ROM's textures, so not committed), keyed by the texture's
contents, so the build only searches for textures that are new or changed.

Usage: fishmod/tools/card_outline.py [--iters N] [--restarts R] [--jobs J] [--force] [tex.png ...]
       (gen_habitat.py calls outline() during the build, with the default effort; run this with more to improve
       the cache: a result only replaces the cached one if it scores better)
"""
import argparse
import hashlib
import json
import math
import os
import random
import sys

import numpy as np
from PIL import Image

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
CACHE = os.path.join(ROOT, "src", "fishmod_data", "card_outlines.json")
VERT_COST = 0.006   # a vertex (a triangle more: its setup and its 176 bytes of RDP commands) is worth this much of the
                    # card's area: the hardware bench showed the RDP's filling is what costs, so pixels count for more
MAX_VERTS = 14
SUB = 4             # samples per texel side when checking that the polygon holds a texel
DEFAULT_ITERS = 6000
DEFAULT_RESTARTS = 2


def required_mask(img, repeat_v=1):
    """The texels that can show: opaque ones (alpha >= half) and their neighbours, the texture repeated up."""
    a = np.array(img.convert("RGBA"))[:, :, 3] >= 128
    a = np.tile(a, (repeat_v, 1))
    d = a.copy()
    d[1:, :] |= a[:-1, :]
    d[:-1, :] |= a[1:, :]
    d2 = d.copy()
    d2[:, 1:] |= d[:, :-1]
    d2[:, :-1] |= d[:, 1:]
    return d2


def samples(mask):
    ys, xs = np.nonzero(mask)
    offs = (np.arange(SUB) + 0.5) / SUB
    ox, oy = np.meshgrid(offs, offs)
    px = (xs[:, None] + ox.ravel()[None, :]).ravel()
    py = (ys[:, None] + oy.ravel()[None, :]).ravel()
    return px, py


def inside(poly, px, py):
    """Even-odd test of every sample against the polygon."""
    res = np.zeros(px.shape, bool)
    n = len(poly)
    for i in range(n):
        x0, y0 = poly[i]
        x1, y1 = poly[(i + 1) % n]
        if y0 == y1:
            continue
        cond = (py >= min(y0, y1)) & (py < max(y0, y1))
        xc = x0 + (py - y0) * (x1 - x0) / (y1 - y0)
        res ^= cond & (px < xc)
    return res


def area(poly):
    s = 0.0
    for i in range(len(poly)):
        x0, y0 = poly[i]
        x1, y1 = poly[(i + 1) % len(poly)]
        s += x0 * y1 - x1 * y0
    return abs(s) / 2


def _segs_cross(p, q, r, s):
    def orient(a, b, c):
        v = (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])
        return (v > 0) - (v < 0)

    o1, o2, o3, o4 = orient(p, q, r), orient(p, q, s), orient(r, s, p), orient(r, s, q)
    if o1 != o2 and o3 != o4:
        return True
    return False


def simple(poly):
    n = len(poly)
    if len(set(poly)) != n:
        return False
    for i in range(n):
        a, b = poly[i], poly[(i + 1) % n]
        if a == b:
            return False
        for j in range(i + 2, n):
            if i == 0 and j == n - 1:
                continue
            c, d = poly[j], poly[(j + 1) % n]
            if _segs_cross(a, b, c, d):
                return False
    # no zero-area spikes (collinear back-tracks)
    return area(poly) > 0


def score(poly, w, h):
    return area(poly) / (w * h) + VERT_COST * len(poly)


def anneal(mask, iters, seed):
    h, w = mask.shape
    px, py = samples(mask)
    rnd = random.Random(seed)
    poly = [(0, 0), (w, 0), (w, h), (0, h)]
    best = cur = poly
    best_s = cur_s = score(poly, w, h)

    def valid(p):
        if len(p) < 3 or not simple(p):
            return False
        return bool(inside(p, px, py).all())

    for it in range(iters):
        temp = 0.03 * (0.002 / 0.03) ** (it / max(iters - 1, 1))
        p = list(cur)
        n = len(p)
        r = rnd.random()
        if r < 0.15 and n < MAX_VERTS:
            i = rnd.randrange(n)
            a, b = p[i], p[(i + 1) % n]
            p.insert(i + 1, (round((a[0] + b[0]) / 2 * 2) / 2, round((a[1] + b[1]) / 2 * 2) / 2))
        elif r < 0.25 and n > 3:
            del p[rnd.randrange(n)]
        else:
            i = rnd.randrange(n)
            step = rnd.choice((0.5, 1, 1, 2, 4))
            x = min(max(p[i][0] + rnd.choice((-1, 0, 1)) * step, 0), w)
            y = min(max(p[i][1] + rnd.choice((-1, 0, 1)) * step, 0), h)
            p[i] = (x, y)
        if p == cur or not valid(p):
            continue
        s = score(p, w, h)
        if s <= cur_s or rnd.random() < math.exp((cur_s - s) / temp):
            cur, cur_s = p, s
            if s < best_s:
                best, best_s = p, s
    return best, best_s


def _job(args):
    key, path, repeat_v, iters, seed = args
    mask = required_mask(Image.open(path), repeat_v)
    poly, s = anneal(mask, iters, seed)
    return key, poly, s


def _key(path, repeat_v):
    with open(path, "rb") as f:
        digest = hashlib.sha1(f.read()).hexdigest()[:16]
    return f"{os.path.basename(path)}:{repeat_v}:{digest}"


def load_cache():
    try:
        with open(CACHE) as f:
            return json.load(f)
    except (OSError, ValueError):
        return {}


def save_cache(cache):
    os.makedirs(os.path.dirname(CACHE), exist_ok=True)
    with open(CACHE, "w") as f:
        json.dump(cache, f, indent=0, sort_keys=True)


def outline(path, repeat_v=1, iters=DEFAULT_ITERS, restarts=DEFAULT_RESTARTS):
    """The polygon for a texture, in UV (0..1 across, 0..repeat_v up, image rows down): from the cache, or searched
    for (and cached). None if the texture has no see-through texels worth cutting."""
    img = Image.open(path)
    if np.array(img.convert("RGBA"))[:, :, 3].min() >= 128:
        return None
    cache = load_cache()
    key = _key(path, repeat_v)
    if key not in cache:
        results = [_job((key, path, repeat_v, iters, 1000 + r)) for r in range(restarts)]
        _, poly, s = min(results, key=lambda t: t[2])
        cache[key] = {"poly": poly, "score": s}
        save_cache(cache)
    w, h = img.size
    return [(x / w, y / h) for x, y in cache[key]["poly"]]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("textures", nargs="*")
    ap.add_argument("--iters", type=int, default=40000)
    ap.add_argument("--restarts", type=int, default=8)
    ap.add_argument("--jobs", type=int, default=os.cpu_count())
    ap.add_argument("--repeat", type=int, default=1)
    ap.add_argument("--force", action="store_true", help="replace the cached outline even if it scored better")
    args = ap.parse_args()
    paths = args.textures or [os.path.join(ROOT, "fishmod", "art", "habitat_rom", f)
                              for f in sorted(os.listdir(os.path.join(ROOT, "fishmod", "art", "habitat_rom")))]
    jobs = []
    for path in paths:
        if np.array(Image.open(path).convert("RGBA"))[:, :, 3].min() >= 128:
            continue
        key = _key(path, args.repeat)
        jobs += [(key, path, args.repeat, args.iters, 7000 + r) for r in range(args.restarts)]
    from multiprocessing import Pool
    with Pool(args.jobs) as pool:
        results = pool.map(_job, jobs)
    cache = load_cache()
    for key in sorted({k for k, _, _ in results}):
        _, poly, s = min((r for r in results if r[0] == key), key=lambda t: t[2])
        old = cache.get(key)
        if args.force or old is None or s < old["score"]:
            cache[key] = {"poly": poly, "score": s}
        kept = cache[key]
        w, h = Image.open([p for k2, p, *_ in jobs if k2 == key][0]).size
        h *= args.repeat
        note = "" if kept["score"] == s else f" (this run scored {s:.3f}, the cached {kept['score']:.3f} is kept)"
        print(f"{key.split(':')[0]:22s} {len(kept['poly']):2d} vertices, {area(kept['poly']) / (w * h):.2f} of the card"
              + note)
    save_cache(cache)


if __name__ == "__main__":
    sys.exit(main())
