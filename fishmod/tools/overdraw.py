#!/usr/bin/env python3
"""How much the habitat props fill the screen from a camera pose: the N64's RDP pays for every pixel of every
triangle drawn (see-through texels included), so the props' overdraw is the sum of their triangles' screen areas.
Picks the props the way fishmod_scatter.c does (draw distance by kind, the view cone, the budget nearest first, the
see-through cap), projects their triangles onto a 320x240 screen and counts how many times each pixel is drawn.

Usage: fishmod/tools/overdraw.py EYE_X EYE_Y EYE_Z AT_X AT_Y AT_Z [--hw] [--png out.png] [--fov 60] [--top N]
  --hw   the original-hardware settings (fishmod_scatter.c, FISHMOD_HW)
  --top  also list the N props that fill the most
Prints the overdraw in screens (1.0 = every pixel once) and the kinds that make it.
"""
import argparse
import math
import os
import re

import numpy as np

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
W, H = 320, 240

# fishmod_scatter.c (keep in step)
SETTINGS = {
    "emu": {"dist": {"small": 800, "mid": 1200, "far": 1600}, "budget": 900, "xlu_max": 200},
    # FISHMOD_HW: every other big plant card, none the camera is inside or right against
    "hw": {"dist": {"small": 800, "mid": 1200, "far": 1600}, "budget": 900, "xlu_max": 200, "thin": True,
           "near": 0.6},
}
DIST_UNDER_FOG = 900.0  # under water nothing further is drawn
SURFACE_Y = -1313.0     # Lake Hylia's full lake
# the big plant cards and their heights at scale 1 (gen_scatter.py HEIGHT): what FISHMOD_HW thins and near-skips
THIN = {"GRASS_TUFT": 16, "RIBBON_WEED": 80, "TALL_WEED": 190, "BROADLEAF": 40, "BUSH_WEED": 42, "KELP_FROND": 260,
        "VINE_STRAND": 150, "BIG_PONDWEED": 52, "FAN_WEED": 44}
VIEW_COS = 0.68
PROP_RADIUS = 90.0
BAND_NEAR, BAND_MID = 550.0, 1100.0
SMALL = {"GRASS_TUFT", "LEAF_LITTER"}
MID = {"POND_REED", "PEBBLE_PATCH", "RIBBON_WEED", "BROADLEAF", "BUSH_WEED", "BRANCH", "LILY_FLOWER", "LILY_CENTRE",
       "RUSH_TUFT", "MARIGOLD", "VINE_STRAND", "BIG_PONDWEED"}
XLU = {"POND_REED"}
VANILLA = {"POND_REED": "gFishingReedModelDL", "LILY_PAD": "gFishingLilyPadModelDL", "ROCK": "gFishingRockModelDL"}


def read(p):
    with open(os.path.join(ROOT, p)) as f:
        return f.read()


def kinds():
    t = read("src/fishmod_data/fishmod_props_kinds.h")
    block = t[t.index("#define FISHMOD_PROP_KINDS(X)"):]
    block = block[:block.index("\n\n")] if "\n\n" in block else block
    return re.findall(r"X\((\w+), (\w+)\)", block)


VTX = re.compile(r"VTX\(\s*(-?\w+),\s*(-?\w+),\s*(-?\w+),")


def parse_dl(dl_text, vtx_arrays):
    """Triangles (as vertex positions) of a display list, given its vertex arrays by name."""
    cache = [None] * 64
    tris = []
    for line in dl_text.splitlines():
        m = re.search(r"gsSPVertex\(&?(\w+)\[?(\d*)\]?,\s*(\d+),\s*(\d+)\)", line)
        if m:
            arr = vtx_arrays.get(m.group(1))
            if arr is None:
                continue
            start = int(m.group(2) or 0)
            for k in range(int(m.group(3))):
                cache[int(m.group(4)) + k] = arr[start + k]
            continue
        for m in re.finditer(r"gsSP2Triangles\((\d+), (\d+), (\d+), \d+, (\d+), (\d+), (\d+), \d+\)", line):
            idx = [int(v) for v in m.groups()]
            tris.append([cache[i] for i in idx[:3]])
            tris.append([cache[i] for i in idx[3:]])
        for m in re.finditer(r"gsSP1Triangle\((\d+), (\d+), (\d+), \d+\)", line):
            tris.append([cache[int(v)] for v in m.groups()])
    return [t for t in tris if None not in t]


def meshes(kind_list):
    out = {}
    t = read("src/fishmod_data/fishmod_props.inc.c")
    arrays = {}
    for m in re.finditer(r"Vtx (\w+)\[\d+\] = \{(.*?)\};", t, re.S):
        arrays[m.group(1)] = [tuple(int(v, 0) for v in g) for g in VTX.findall(m.group(2))]
    for kid, name in kind_list:
        if kid in VANILLA:
            continue
        m = re.search(r"Gfx gFishmodProp_%sDL\[\] = \{(.*?)\};" % name, t, re.S)
        if m:
            out[kid] = parse_dl(m.group(1), arrays)
    obj = os.path.join(ROOT, "extracted/ntsc-1.2/assets/objects/object_fish")
    varr = {}
    for f in os.listdir(obj):
        if f.startswith("object_fish_Vtx_") or f.endswith("Vtx.inc.c"):
            name = f[:-len(".inc.c")]
            with open(os.path.join(obj, f)) as fh:
                varr[name] = [tuple(int(v, 0) for v in g) for g in VTX.findall(fh.read())]
    for kid, dl in VANILLA.items():
        p = os.path.join(obj, dl + ".inc.c")
        if os.path.exists(p):
            with open(p) as fh:
                out[kid] = parse_dl(fh.read(), varr)
    return out


def props(kind_list):
    t = read("src/fishmod_data/fishmod_sc_spot06.c")
    rows = re.findall(r"\{ (-?\d+), (-?\d+), (-?\d+), (\d+), (\d+), (\d+), (\d+), (-?\d+), (-?\d+) \}", t)
    names = [k for k, _ in kind_list]
    return [(int(x), int(y), int(z), names[int(k)], int(yaw), int(sc), int(tx), int(tz))
            for x, y, z, k, yaw, sc, tint, tx, tz in rows]


def pick(plist, eye, look, cfg):
    """The props drawn, as fishmod_scatter.c's Scatter_Build picks them."""
    bands = [[], [], []]
    for p in plist:
        kid = p[3]
        dist = cfg["dist"]["small" if kid in SMALL else "mid" if kid in MID else "far"]
        if kid in cfg.get("dist_kind", {}):
            dist = cfg["dist_kind"][kid]
        dx, dy, dz = p[0] - eye[0], p[1] - eye[1], p[2] - eye[2]
        dsq = dx * dx + dz * dz
        if dsq > dist * dist:
            continue
        if cfg.get("thin") and kid in THIN:
            if ((p[0] + p[2]) >> 3) & 1:
                continue
            h = THIN[kid] * p[5] / 64.0
            if dsq + (p[1] + h / 2 - eye[1]) ** 2 < (h * cfg["near"] + 20.0) ** 2:
                continue
        front = dx * look[0] + dy * look[1] + dz * look[2] + PROP_RADIUS
        if ((front < 0) or (front * front < VIEW_COS ** 2 * (dsq + dy * dy))) and dsq > (PROP_RADIUS * 2) ** 2:
            continue
        b = 0 if dsq < BAND_NEAR ** 2 else 1 if dsq < BAND_MID ** 2 else 2
        bands[b].append(p)
    out, budget, xlu = [], cfg["budget"], cfg["xlu_max"]
    for band in bands:
        for p in band:
            if budget <= 0:
                break
            if p[3] in XLU:
                xlu -= 1
                if xlu < 0:
                    continue
            budget -= 1
            out.append(p)
    return out


def view_proj(eye, at, fov):
    f = np.array(at, float) - np.array(eye, float)
    f /= np.linalg.norm(f)
    up = np.array([0.0, 1.0, 0.0])
    if abs(f @ up) > 0.99:
        up = np.array([0.0, 0.0, 1.0])
    s = np.cross(f, up)
    s /= np.linalg.norm(s)
    u = np.cross(s, f)
    return s, u, f, 1.0 / math.tan(math.radians(fov) / 2)


def raster(buf, kbuf, kidx, pts):
    (x0, y0), (x1, y1), (x2, y2) = pts
    area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0)
    if abs(area) < 1e-6:
        return
    minx, maxx = max(int(math.floor(min(x0, x1, x2))), 0), min(int(math.ceil(max(x0, x1, x2))), W - 1)
    miny, maxy = max(int(math.floor(min(y0, y1, y2))), 0), min(int(math.ceil(max(y0, y1, y2))), H - 1)
    if minx > maxx or miny > maxy:
        return
    xs, ys = np.meshgrid(np.arange(minx, maxx + 1) + 0.5, np.arange(miny, maxy + 1) + 0.5)
    w0 = (x1 - xs) * (y2 - ys) - (x2 - xs) * (y1 - ys)
    w1 = (x2 - xs) * (y0 - ys) - (x0 - xs) * (y2 - ys)
    w2 = (x0 - xs) * (y1 - ys) - (x1 - xs) * (y0 - ys)
    inside = ((w0 >= 0) & (w1 >= 0) & (w2 >= 0)) | ((w0 <= 0) & (w1 <= 0) & (w2 <= 0))
    buf[miny:maxy + 1, minx:maxx + 1] += inside
    kbuf[kidx] += int(inside.sum())


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("pose", type=float, nargs=6)
    ap.add_argument("--hw", action="store_true")
    ap.add_argument("--top", type=int, default=0)
    ap.add_argument("--png")
    ap.add_argument("--fov", type=float, default=60.0)
    args = ap.parse_args()
    eye, at = args.pose[:3], args.pose[3:]
    cfg = dict(SETTINGS["hw" if args.hw else "emu"])
    if eye[1] < SURFACE_Y:
        cfg["dist"] = {k: min(v, DIST_UNDER_FOG) for k, v in cfg["dist"].items()}

    kind_list = kinds()
    mesh = meshes(kind_list)
    look = np.array(at) - np.array(eye)
    look /= np.linalg.norm(look)
    drawn = pick(props(kind_list), eye, look, cfg)
    s, u, f, focal = view_proj(eye, at, args.fov)
    aspect = W / H
    buf = np.zeros((H, W), np.int32)
    names = sorted({p[3] for p in drawn})
    kbuf = [0] * len(names)
    near = 10.0
    per_prop = []
    for x, y, z, kid, yaw, sc, tx, tz in drawn:
        before = sum(kbuf)
        tris = mesh.get(kid)
        if not tris:
            continue
        k = sc / 64.0
        a = yaw * 2 * math.pi / 256
        ca, sa = math.cos(a), math.sin(a)
        flat = kid == "LILY_PAD"
        for tri in tris:
            pts = []
            ok = True
            for vx, vy, vz in tri:
                # as Scatter_Matrix (no tilt: logs are few): x' = (x c + z s), z' = (-x s + z c), times the scale
                wx = x + (vx * ca + vz * sa) * k
                wy = y + vy * (1.0 if flat else k)
                wz = z + (-vx * sa + vz * ca) * k
                d = np.array([wx - eye[0], wy - eye[1], wz - eye[2]])
                depth = d @ f
                if depth < near:
                    depth = near
                    ok = ok and False if depth < 0 else ok
                sx = W / 2 + (d @ s) / depth * focal / aspect * W / 2
                sy = H / 2 - (d @ u) / depth * focal * H / 2
                pts.append((sx, sy))
            raster(buf, kbuf, names.index(kid), pts)
        per_prop.append((sum(kbuf) - before, kid, math.dist((x, y, z), eye), sc))
    total = buf.sum() / (W * H)
    print(f"{len(drawn)} props drawn, overdraw {total:.2f} screens (worst pixel {buf.max()}x)")
    for n, c in sorted(zip(names, kbuf), key=lambda t: -t[1])[:10]:
        print(f"  {n:14s} {c / (W * H):6.2f}")
    if args.top:
        for px, kid, d, sc in sorted(per_prop, reverse=True)[:args.top]:
            print(f"    {kid:12s} {px / (W * H):5.2f} screens at {d:5.0f} (scale {sc / 64:.2f})")
    if args.png:
        from PIL import Image
        img = np.clip(buf * 16, 0, 255).astype(np.uint8)
        Image.fromarray(img).resize((W * 2, H * 2)).save(args.png)


if __name__ == "__main__":
    main()
