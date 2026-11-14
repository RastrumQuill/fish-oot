#!/usr/bin/env python3
"""A software preview of prop meshes at the N64's resolution (320x240): textured, vertex-coloured, depth-tested, back
faces culled where asked, the scene's fog, a water surface and a lake bed for context, from any camera. And what each
mesh would cost the RDP drawn that way (fishmod/tools/rdp_cost.py's model, fitted to the console's calibration: a fixed
cost a triangle, a cost per scanline and per pixel), so designs can be compared by look and by cost together.

Used as a module (fishmod/tools/dock_designs.py) or:
  mesh_preview.py MESH.obj TEXTURE.png OUT.png [--eye X Y Z] [--at X Y Z]
"""
import math
import os
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

W, H = 320, 240
# rdp_cost.py: 1-cycle, textured, Z-buffered (the props' mode): ns = a + b * scanlines + c * pixels
COST = (960.0, 424.5, 25.8)


def look_at(eye, at, fov=60.0):
    eye, at = np.array(eye, float), np.array(at, float)
    f = at - eye
    f /= np.linalg.norm(f)
    r = np.cross(f, (0, 1, 0))
    r /= np.linalg.norm(r)
    u = np.cross(r, f)
    view = np.eye(4)
    view[0, :3], view[1, :3], view[2, :3] = r, u, -f
    view[:3, 3] = -view[:3, :3] @ eye
    t = 1.0 / math.tan(math.radians(fov) / 2)
    near, far = 10.0, 12800.0
    proj = np.array([[t * H / W, 0, 0, 0], [0, t, 0, 0],
                     [0, 0, -(far + near) / (far - near), -2 * far * near / (far - near)], [0, 0, -1, 0]])
    return proj @ view


class Canvas:
    def __init__(self, sky=(120, 150, 170)):
        self.rgb = np.zeros((H, W, 3), float)
        self.rgb[:] = sky
        self.z = np.full((H, W), np.inf)
        self.cost_ns = 0.0
        self.tris = 0
        self.px = 0.0

    def draw(self, mvp, verts, uvs, cols, faces, tex=None, cull=True, fog=None, count=True, tint=None,
             water_y=None, under_tint=None):
        """verts (N,3), uvs (N,2) in texels/size units (0-1 a repeat), cols (N,3) 0-1, faces [(a,b,c)] 0-based"""
        V = np.c_[np.asarray(verts, float), np.ones(len(verts))] @ mvp.T
        T = None if tex is None else np.asarray(tex, float)
        for a, b, c in faces:
            P = V[[a, b, c]]
            if np.any(P[:, 3] < 1.0):
                continue  # (behind the near plane: a preview doesn't clip)
            sx = W / 2 + W / 2 * P[:, 0] / P[:, 3]
            sy = H / 2 - H / 2 * P[:, 1] / P[:, 3]
            area2 = (sx[1] - sx[0]) * (sy[2] - sy[0]) - (sx[2] - sx[0]) * (sy[1] - sy[0])
            if cull and area2 >= 0:
                continue  # counter-clockwise front faces (screen y down: negative signed area)
            if abs(area2) < 1e-6:
                continue
            x0, x1 = max(int(math.floor(sx.min())), 0), min(int(math.ceil(sx.max())), W - 1)
            y0, y1 = max(int(math.floor(sy.min())), 0), min(int(math.ceil(sy.max())), H - 1)
            if x0 > x1 or y0 > y1:
                continue
            ys, xs = np.mgrid[y0:y1 + 1, x0:x1 + 1] + 0.5
            w0 = ((sx[1] - xs) * (sy[2] - ys) - (sx[2] - xs) * (sy[1] - ys)) / area2
            w1 = ((sx[2] - xs) * (sy[0] - ys) - (sx[0] - xs) * (sy[2] - ys)) / area2
            w2 = 1 - w0 - w1
            inside = (w0 >= 0) & (w1 >= 0) & (w2 >= 0)
            if not inside.any():
                continue
            if count:
                npx = inside.sum()
                lines = np.any(inside, axis=1).sum()
                self.cost_ns += COST[0] + COST[1] * lines + COST[2] * npx
                self.tris += 1
                self.px += npx
            iw = 1.0 / P[:, 3]
            pw = w0 * iw[0] + w1 * iw[1] + w2 * iw[2]
            z = (w0 * P[0, 2] * iw[0] + w1 * P[1, 2] * iw[1] + w2 * P[2, 2] * iw[2]) / pw
            zb = self.z[y0:y1 + 1, x0:x1 + 1]
            ok = inside & (z < zb)
            if not ok.any():
                continue

            def interp(q):
                q = np.asarray(q, float)
                return ((w0 * iw[0])[..., None] * q[0] + (w1 * iw[1])[..., None] * q[1] +
                        (w2 * iw[2])[..., None] * q[2]) / pw[..., None]
            col = interp(np.asarray(cols)[[a, b, c]])
            if T is not None:
                uv = interp(np.asarray(uvs)[[a, b, c]])
                th, tw = T.shape[:2]
                tu = np.floor(uv[..., 0] * tw).astype(int) % tw
                tv = np.floor(uv[..., 1] * th).astype(int) % th
                texel = T[tv, tu]
                if texel.shape[-1] == 4:
                    ok = ok & (texel[..., 3] > 127)
                    texel = texel[..., :3]
                col = col * texel / 255.0 * 255.0
            else:
                col = col * 255.0
            if tint is not None:
                col = col * np.asarray(tint)
            if water_y is not None and under_tint is not None:
                wpos = interp(np.asarray(verts, float)[[a, b, c]])
                below = wpos[..., 1] < water_y
                col = np.where(below[..., None], col * np.asarray(under_tint), col)
            if fog is not None:
                fc, fnear, ffar = fog
                dist = 1.0 / pw
                k = np.clip((dist - fnear) / (ffar - fnear), 0, 1)[..., None]
                col = col * (1 - k) + np.asarray(fc) * k
            rgb = self.rgb[y0:y1 + 1, x0:x1 + 1]
            rgb[ok] = col[ok]
            zb[ok] = z[ok]

    def image(self, scale=3):
        img = Image.fromarray(np.clip(self.rgb, 0, 255).astype(np.uint8))
        return img.resize((W * scale, H * scale), Image.NEAREST)


def load_obj(path):
    v, vt, f, cols = [], [], [], []
    for line in open(path):
        p = line.split()
        if not p:
            continue
        if p[0] == "v":
            v.append([float(x) for x in p[1:4]])
            cols.append([float(x) for x in p[4:7]] if len(p) >= 7 else [1, 1, 1])
        elif p[0] == "vt":
            vt.append([float(p[1]), 1.0 - float(p[2])])
        elif p[0] == "f":
            f.append(tuple(int(x.split("/")[0]) - 1 for x in p[1:4]))
    return np.array(v), np.array(vt), np.array(cols), f


def main():
    import argparse
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("obj")
    ap.add_argument("texture")
    ap.add_argument("out")
    ap.add_argument("--eye", type=float, nargs=3, default=(0, 60, -200))
    ap.add_argument("--at", type=float, nargs=3, default=(0, 0, 0))
    a = ap.parse_args()
    v, vt, cols, f = load_obj(a.obj)
    c = Canvas()
    c.draw(look_at(a.eye, a.at), v, vt, cols, f, np.asarray(Image.open(a.texture).convert("RGBA")))
    c.image().save(a.out)
    print("%d triangles drawn, %.0f pixels, about %.2f ms of RDP" % (c.tris, c.px, c.cost_ns / 1e6))


if __name__ == "__main__":
    main()
