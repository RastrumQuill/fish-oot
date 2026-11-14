#!/usr/bin/env python3
"""Which low-poly shapes give the most roundness for the RDP's time: a post (or a ball) built each way, drawn from many
angles with fishmod/tools/mesh_preview.py's rasterizer, back faces culled, and costed with the console-calibrated
model (a fixed cost a triangle, a cost per scanline, a cost per pixel). For each: the triangles drawn, the cost, and
how round its outline looks (the narrowest silhouette over the widest, over every angle: a true circle is 1.0) and how
much the outline wobbles up its height (the "dented" look of offset rings).

Usage: shape_study.py [--png OUT.png]
"""
import math
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mesh_preview as mp  # noqa: E402

H_POST, R_POST = 80.0, 5.0


def post(sides, twist=0.0, rings=1, alt=False):
    """a post along y: `rings` segments; each ring turned by twist*(its height share) of a side, or with `alt` every
    other ring turned by half a side (the offset-rows layout)"""
    v, f = [], []
    for k in range(rings + 1):
        t = k / rings
        off = (0.5 if (alt and k % 2) else 0.0) + twist * t
        for s in range(sides):
            a = 2 * math.pi * (s + off) / sides
            v.append((R_POST * math.cos(a), H_POST * t, R_POST * math.sin(a)))
    for k in range(rings):
        b0, b1 = k * sides, (k + 1) * sides
        for s in range(sides):
            a, b = b0 + s, b0 + (s + 1) % sides
            c, d = b1 + s, b1 + (s + 1) % sides
            twisted = (twist != 0.0) or alt
            if twisted:
                # (the faces between offset rings: triangles that follow the offset, not a quad split at random)
                if (alt and k % 2) or (not alt and twist < 0):
                    f += [(a, b, c), (b, d, c)]
                else:
                    f += [(a, b, d), (a, d, c)]
            else:
                f += [(a, b, d), (a, d, c)]
    return np.array(v), orient(np.array(v), f, (0, None))


def billboard():
    """one camera-facing card as wide as the post (the impostor): turned to the camera when drawn"""
    v = np.array([(-R_POST, 0, 0), (R_POST, 0, 0), (R_POST, H_POST, 0), (-R_POST, H_POST, 0)])
    return v, [(0, 1, 2), (0, 2, 3)]


def ball(kind):
    """low spheres for rocks: an octahedron (8), an icosahedron (20), a UV sphere 6x4 (40)"""
    if kind == "octahedron":
        v = [(1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1)]
        f = [(0, 2, 4), (4, 2, 1), (1, 2, 5), (5, 2, 0), (0, 4, 3), (4, 1, 3), (1, 5, 3), (5, 0, 3)]
    elif kind == "icosahedron":
        p = (1 + 5 ** 0.5) / 2
        v = [(-1, p, 0), (1, p, 0), (-1, -p, 0), (1, -p, 0), (0, -1, p), (0, 1, p), (0, -1, -p), (0, 1, -p),
             (p, 0, -1), (p, 0, 1), (-p, 0, -1), (-p, 0, 1)]
        f = [(0, 11, 5), (0, 5, 1), (0, 1, 7), (0, 7, 10), (0, 10, 11), (1, 5, 9), (5, 11, 4), (11, 10, 2), (10, 7, 6),
             (7, 1, 8), (3, 9, 4), (3, 4, 2), (3, 2, 6), (3, 6, 8), (3, 8, 9), (4, 9, 5), (2, 4, 11), (6, 2, 10),
             (8, 6, 7), (9, 8, 1)]
    else:  # uv 6 x 4
        v, f = [], []
        n, m = 6, 4
        v.append((0, 1, 0))
        for i in range(1, m):
            th = math.pi * i / m
            for j in range(n):
                ph = 2 * math.pi * j / n
                v.append((math.sin(th) * math.cos(ph), math.cos(th), math.sin(th) * math.sin(ph)))
        v.append((0, -1, 0))
        for j in range(n):
            f.append((0, 1 + j, 1 + (j + 1) % n))
        for i in range(m - 2):
            for j in range(n):
                a, b = 1 + i * n + j, 1 + i * n + (j + 1) % n
                c, d = a + n, b + n
                f += [(a, c, d), (a, d, b)]
        last = len(v) - 1
        for j in range(n):
            a, b = 1 + (m - 2) * n + j, 1 + (m - 2) * n + (j + 1) % n
            f.append((last, b, a))
    v = np.array(v, float)
    v = v / np.linalg.norm(v, axis=1)[:, None] * 30.0 + (0, 30.0, 0)
    return v, orient(v, f, (0, 30.0))


def orient(v, f, centre):
    cx = np.array([0.0, 0.0, 0.0])
    out = []
    for a, b, c in f:
        n = np.cross(v[b] - v[a], v[c] - v[a])
        mid = (v[a] + v[b] + v[c]) / 3
        ref = mid.copy()
        if centre[1] is None:
            ref[1] = 0.0  # (a post: outward from its axis)
            d = ref - np.array([0, 0, 0])
            d[1] = 0
        else:
            d = mid - np.array([0, centre[1], 0])
        out.append((a, b, c) if np.dot(n, d) >= 0 else (a, c, b))
    return out


def outline(v, f, n_angles=180):
    """roundness and wobble, geometrically: the surface sampled densely, its width across the line of sight measured
    in height bands, from n_angles directions round it (orthographic: the shape's own outline, not the pixels')"""
    pts = []
    for a, b, c in f:
        for i in range(13):
            for j in range(13 - i):
                u, w = i / 12.0, j / 12.0
                pts.append(v[a] * (1 - u - w) + v[b] * u + v[c] * w)
    pts = np.array(pts)
    y0, y1 = pts[:, 1].min(), pts[:, 1].max()
    bands = np.clip(((pts[:, 1] - y0) / max(y1 - y0, 1e-6) * 10).astype(int), 0, 9)
    mean_w, wob = [], []
    for k in range(n_angles):
        yaw = math.pi * k / n_angles
        x = pts[:, 0] * math.cos(yaw) + pts[:, 2] * math.sin(yaw)
        ws = [x[bands == b].max() - x[bands == b].min() for b in range(1, 9) if (bands == b).any()]
        mean_w.append(np.mean(ws))
        wob.append(np.std(ws) / np.mean(ws))
    mean_w = np.array(mean_w)
    return mean_w.min() / mean_w.max(), float(np.mean(wob))


def measure(v, f, dist, face_camera=False, n_angles=24):
    """the RDP's cost (us) and the triangles drawn, from n_angles directions, a little above"""
    costs, tris = [], []
    for k in range(n_angles):
        yaw = 2 * math.pi * k / n_angles + 0.013
        top = v[:, 1].max()
        eye = (dist * math.sin(yaw), top * 0.5 + dist * 0.15, -dist * math.cos(yaw))
        at = (0, top * 0.5, 0)
        vv = v
        if face_camera:
            # (its front, -z, turned to face the eye)
            c, s = math.cos(-yaw), math.sin(-yaw)
            vv = np.c_[v[:, 0] * c + v[:, 2] * s, v[:, 1], -v[:, 0] * s + v[:, 2] * c]
        cv = mp.Canvas(sky=(0, 0, 0))
        cv.draw(mp.look_at(eye, at), vv, np.zeros((len(vv), 2)), np.ones((len(vv), 3)), f, None, cull=not face_camera)
        costs.append(cv.cost_ns)
        tris.append(cv.tris)
    return np.mean(costs) / 1000.0, np.mean(tris)


def main():
    posts = [
        ("triangle prism", post(3)), ("square prism", post(4)), ("pentagon prism", post(5)),
        ("hexagon prism", post(6)), ("octagon prism", post(8)),
        ("triangle, twisted 1/2", post(3, 0.5)), ("square, twisted 1/2", post(4, 0.5)),
        ("pentagon, twisted 1/2", post(5, 0.5)), ("hexagon, twisted 1/2", post(6, 0.5)),
        ("square, 4 rows offset", post(4, 0, 4, alt=True)), ("hexagon, 4 rows offset", post(6, 0, 4, alt=True)),
    ]
    print("Posts (80 tall, radius 5): triangles a model / drawn, RDP us at 150 and 600 units, roundness (narrowest "
          "outline / widest), wobble up the height")
    print("| shape | tris | drawn (near) | us near | us far | roundness | wobble |")
    print("|---|---|---|---|---|---|---|")
    for name, (v, f) in posts:
        n = measure(v, f, 150.0)
        fr = measure(v, f, 600.0)
        rd, wb = outline(v, f)
        print("| %s | %d | %.1f | %.1f | %.1f | %.3f | %.3f |" % (name, len(f), n[1], n[0], fr[0], rd, wb))
    v, f = billboard()
    n = measure(v, f, 150.0, face_camera=True)
    fr = measure(v, f, 600.0, face_camera=True)
    print("| camera-facing card | 2 | %.1f | %.1f | %.1f | 1.000 (always faces you) | 0.000 |" % (n[1], n[0], fr[0]))
    print("\nBalls (radius 30, for rocks):")
    print("| shape | tris | drawn | us near | us far | roundness |")
    print("|---|---|---|---|---|---|")
    for kind in ("octahedron", "uv 6x4", "icosahedron"):
        v, f = ball(kind)
        n = measure(v, f, 200.0)
        fr = measure(v, f, 800.0)
        rd, _ = outline(v, f)
        print("| %s | %d | %.1f | %.1f | %.1f | %.3f |" % (kind, len(f), n[1], n[0], fr[0], rd))


if __name__ == "__main__" and "--png" not in sys.argv:
    main()


def sheet(out):
    """the post shapes close up from three angles, shaded smoothly by radial normals (as baked vertex colours)"""
    from PIL import Image, ImageDraw
    shapes = [("square prism (8)", post(4)), ("hexagon prism (12)", post(6)), ("octagon prism (16)", post(8)),
              ("triangle twisted (6)", post(3, 0.5)), ("square twisted (8)", post(4, 0.5)),
              ("pentagon prism (10)", post(5)), ("pentagon twisted (10)", post(5, 0.5))]
    cw, ch = 160, 240
    img = Image.new("RGB", (cw * len(shapes), ch * 3 + 20), (30, 30, 30))
    d = ImageDraw.Draw(img)
    light = np.array([0.6, 0.3, -0.75])
    for i, (name, (v, f)) in enumerate(shapes):
        d.text((i * cw + 4, 4), name, fill=(255, 255, 0))
        rn = v.copy()
        rn[:, 1] = 0
        rn /= np.linalg.norm(rn, axis=1)[:, None]
        shade = np.clip(rn @ light, 0, 1) * 0.65 + 0.3
        col = np.c_[shade * 0.75, shade * 0.6, shade * 0.42]
        for j, yaw in enumerate((0.2, 0.75, 1.3)):
            eye = (60 * math.sin(yaw), 55, -60 * math.cos(yaw))
            cv = mp.Canvas(sky=(110, 140, 160))
            cv.draw(mp.look_at(eye, (0, 40, 0), fov=70), v, np.zeros((len(v), 2)), col, f, None, cull=True)
            im = Image.fromarray(np.clip(cv.rgb, 0, 255).astype(np.uint8)).crop((80, 0, 240, 240))
            img.paste(im, (i * cw, 20 + j * ch))
    img.save(out)


if __name__ == "__main__" and "--png" in sys.argv:
    sheet(sys.argv[sys.argv.index("--png") + 1])
