#!/usr/bin/env python3
"""A small offline renderer for item icons in the game's manner: the game's icons are 3D models rendered ahead of time,
lit softly from the upper left with a fill from the right, shaded smoothly with a little gloss, and shrunk to 32 x 32
over black (which leaves their edges a little dark). This draws triangle meshes the same way, at 8x, and shrinks them.

  m = Mesh(); m.add_tri(...) / m.lathe(...) / m.tube(...)
  img = render([mesh, ...], yaw, pitch, roll)            -> 32x32 RGBA (PIL)
  big = render(..., size=256)                             -> the 8x picture, for looking at
Textures are numpy RGBA arrays (H, W, 4); a mesh's colour is its texture times its vertex colours (as the RSP's
TEXEL0 x SHADE), lit per pixel.
"""
import math

import numpy as np
from PIL import Image

LIGHT_KEY = np.array([-0.55, 0.65, 0.52])  # upper left, toward the viewer
LIGHT_FILL = np.array([0.7, -0.1, 0.3])
LIGHT_KEY = LIGHT_KEY / np.linalg.norm(LIGHT_KEY)
LIGHT_FILL = LIGHT_FILL / np.linalg.norm(LIGHT_FILL)


class Mesh:
    def __init__(self, texture=None, gloss=0.35, shine=24.0, clamp=True, alpha_cut=0.0, two_sided=True):
        self.p, self.n, self.uv, self.c, self.tris = [], [], [], [], []
        self.texture = texture  # RGBA float 0-1, or None
        self.gloss, self.shine, self.clamp, self.alpha_cut, self.two_sided = gloss, shine, clamp, alpha_cut, two_sided

    def vert(self, p, n=(0, 1, 0), uv=(0, 0), c=(1, 1, 1, 1)):
        self.p.append(np.asarray(p, float))
        n = np.asarray(n, float)
        ln = np.linalg.norm(n)
        self.n.append(n / ln if ln > 0 else np.array([0.0, 1.0, 0.0]))
        self.uv.append(np.asarray(uv, float))
        c = tuple(c) + ((1.0,) if len(c) == 3 else ())
        self.c.append(np.asarray(c, float))
        return len(self.p) - 1

    def tri(self, a, b, c):
        self.tris.append((a, b, c))

    def lathe(self, profile, segs=24, color=lambda t, y, a: (1, 1, 1), center=(0, 0, 0)):
        """A surface of revolution about the y axis: profile [(radius, y)] from top to bottom; color(t, y, angle)."""
        rings = []
        for i, (r, y) in enumerate(profile):
            # the profile's slope gives the normal's tilt
            r0, y0 = profile[max(i - 1, 0)]
            r1, y1 = profile[min(i + 1, len(profile) - 1)]
            dr, dy = r1 - r0, y1 - y0
            ring = []
            for k in range(segs + 1):
                a = 2 * math.pi * k / segs
                ca, sa = math.cos(a), math.sin(a)
                nrm = (ca * -dy, dr, sa * -dy) if (dr or dy) else (ca, 0, sa)
                if dy > 0:
                    nrm = tuple(-x for x in nrm)
                p = (center[0] + r * ca, center[1] + y, center[2] + r * sa)
                ring.append(self.vert(p, nrm, (k / segs, i / max(len(profile) - 1, 1)),
                                      color(i / max(len(profile) - 1, 1), y, a)))
            rings.append(ring)
        for i in range(len(rings) - 1):
            for k in range(segs):
                a, b, c, d = rings[i][k], rings[i][k + 1], rings[i + 1][k + 1], rings[i + 1][k]
                self.tri(a, b, c)
                self.tri(a, c, d)

    def tube(self, path, radius, segs=10, color=lambda t: (1, 1, 1)):
        """A tube along path [(x, y, z)], radius (a number or a function of t along it)."""
        rings = []
        pts = [np.asarray(q, float) for q in path]
        for i, q in enumerate(pts):
            t = i / max(len(pts) - 1, 1)
            d = pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)]
            d = d / (np.linalg.norm(d) or 1)
            up = np.array([0, 0, 1.0]) if abs(d[2]) < 0.9 else np.array([0, 1.0, 0])
            u = np.cross(d, up)
            u /= np.linalg.norm(u)
            v = np.cross(d, u)
            r = radius(t) if callable(radius) else radius
            ring = []
            for k in range(segs + 1):
                a = 2 * math.pi * k / segs
                nrm = u * math.cos(a) + v * math.sin(a)
                ring.append(self.vert(q + nrm * r, nrm, (k / segs, t), color(t)))
            rings.append(ring)
        for i in range(len(rings) - 1):
            for k in range(segs):
                a, b, c, d = rings[i][k], rings[i][k + 1], rings[i + 1][k + 1], rings[i + 1][k]
                self.tri(a, b, c)
                self.tri(a, c, d)

    def torus(self, center, axis_u, axis_v, r_big, r_small, segs=24, color=(1, 1, 1)):
        path = [np.asarray(center) + (np.asarray(axis_u) * math.cos(a) + np.asarray(axis_v) * math.sin(a)) * r_big
                for a in np.linspace(0, 2 * math.pi, segs + 1)]
        self.tube(path, r_small, 8, lambda t: color)


def rotation(yaw, pitch, roll):
    cy, sy = math.cos(yaw), math.sin(yaw)
    cp, sp = math.cos(pitch), math.sin(pitch)
    cr, sr = math.cos(roll), math.sin(roll)
    ry = np.array([[cy, 0, sy], [0, 1, 0], [-sy, 0, cy]])
    rx = np.array([[1, 0, 0], [0, cp, -sp], [0, sp, cp]])
    rz = np.array([[cr, -sr, 0], [sr, cr, 0], [0, 0, 1]])
    return rz @ rx @ ry


def _sample(tex, uv, clamp):
    h, w = tex.shape[:2]
    u, v = uv[..., 0] * w - 0.5, uv[..., 1] * h - 0.5
    if clamp:
        u, v = np.clip(u, 0, w - 1), np.clip(v, 0, h - 1)
    x0, y0 = np.floor(u).astype(int), np.floor(v).astype(int)
    fx, fy = (u - x0)[..., None], (v - y0)[..., None]

    def at(x, y):
        if clamp:
            return tex[np.clip(y, 0, h - 1), np.clip(x, 0, w - 1)]
        return tex[y % h, x % w]

    return (at(x0, y0) * (1 - fx) * (1 - fy) + at(x0 + 1, y0) * fx * (1 - fy) + at(x0, y0 + 1) * (1 - fx) * fy +
            at(x0 + 1, y0 + 1) * fx * fy)


def raster(meshes, yaw=0.0, pitch=0.0, roll=0.0, size=256, margin=0.06, ambient=0.46, sprites=None, fit=None):
    """The meshes (and any sprites: [(x, y, z, radius, rgba array)] composited in 3D order) at size x size: returns
    (rgb, coverage). Orthographic, fitted to the meshes' extent."""
    R = rotation(yaw, pitch, roll)
    allp = [m_p for m in meshes for m_p in m.p]
    if sprites:
        allp += [np.asarray(s[:3]) + d * s[3] for s in sprites for d in (np.eye(3), -np.eye(3)) for d in d]
    P = np.array(allp) @ R.T
    lo, hi = P[:, :2].min(0), P[:, :2].max(0)
    if fit is not None:
        lo, hi = fit
    span = (hi - lo).max() * (1 + 2 * margin)
    mid = (lo + hi) / 2
    scale = size / span

    def to_screen(q):
        q = q @ R.T
        x = (q[..., 0] - mid[0]) * scale + size / 2
        y = (mid[1] - q[..., 1]) * scale + size / 2
        return x, y, q[..., 2]

    rgb = np.zeros((size, size, 3))
    cov = np.zeros((size, size))
    zb = np.full((size, size), -1e9)
    view = np.array([0, 0, 1.0])
    for m in meshes:
        Pm = np.array(m.p)
        Nm = np.array(m.n) @ R.T
        X, Y, Z = to_screen(Pm)
        UV = np.array(m.uv)
        C = np.array(m.c)
        for a, b, c in m.tris:
            xs, ys = np.array([X[a], X[b], X[c]]), np.array([Y[a], Y[b], Y[c]])
            x0, x1 = int(max(math.floor(xs.min()), 0)), int(min(math.ceil(xs.max()), size - 1))
            y0, y1 = int(max(math.floor(ys.min()), 0)), int(min(math.ceil(ys.max()), size - 1))
            if x1 < x0 or y1 < y0:
                continue
            area = (xs[1] - xs[0]) * (ys[2] - ys[0]) - (xs[2] - xs[0]) * (ys[1] - ys[0])
            if abs(area) < 1e-9:
                continue
            gy, gx = np.mgrid[y0:y1 + 1, x0:x1 + 1] + 0.5
            w0 = ((xs[1] - gx) * (ys[2] - gy) - (xs[2] - gx) * (ys[1] - gy)) / area
            w1 = ((xs[2] - gx) * (ys[0] - gy) - (xs[0] - gx) * (ys[2] - gy)) / area
            w2 = 1 - w0 - w1
            inside = (w0 >= -1e-6) & (w1 >= -1e-6) & (w2 >= -1e-6)
            if not inside.any():
                continue
            z = w0 * Z[a] + w1 * Z[b] + w2 * Z[c]
            sub = zb[y0:y1 + 1, x0:x1 + 1]
            front = inside & (z > sub)
            if not front.any():
                continue
            W = np.stack([w0, w1, w2], -1)[front]
            n = W @ Nm[[a, b, c]]
            n /= np.linalg.norm(n, axis=1, keepdims=True) + 1e-9
            if m.two_sided:
                n = np.where((n[:, 2:3] < 0), -n, n)
            col = W @ C[[a, b, c]]
            alpha = col[:, 3]
            if m.texture is not None:
                t = _sample(m.texture, W @ UV[[a, b, c]], m.clamp)
                col = col[:, :3] * t[:, :3]
                alpha = alpha * t[:, 3]
            else:
                col = col[:, :3]
            keep = alpha > max(m.alpha_cut, 0.02)
            key = np.clip(n @ LIGHT_KEY, 0, 1)
            fill = np.clip(n @ LIGHT_FILL, 0, 1)
            lit = ambient + 0.62 * key + 0.18 * fill
            half = LIGHT_KEY + view
            half /= np.linalg.norm(half)
            spec = m.gloss * np.clip(n @ half, 0, 1) ** m.shine
            shade = np.clip(col * lit[:, None] * (1 + getattr(m, "lift", 0.0)) + spec[:, None], 0, 1)
            yy, xx = np.nonzero(front)
            yy, xx = yy[keep] + y0, xx[keep] + x0
            rgb[yy, xx] = shade[keep]
            cov[yy, xx] = np.clip(alpha[keep] * 1.5, 0, 1)
            zb[yy, xx] = z[front][keep]
    if sprites:
        # back to front over what's drawn (depth tested against it)
        for sx, sy, sz, sr, img in sorted(sprites, key=lambda s: (np.asarray(s[:3]) @ R.T)[2]):
            X, Y, Z = to_screen(np.array([sx, sy, sz]))
            r = sr * scale
            x0, x1 = int(max(X - r, 0)), int(min(X + r, size - 1))
            y0, y1 = int(max(Y - r, 0)), int(min(Y + r, size - 1))
            if x1 < x0 or y1 < y0:
                continue
            gy, gx = np.mgrid[y0:y1 + 1, x0:x1 + 1] + 0.5
            uv = np.stack([(gx - (X - r)) / (2 * r), (gy - (Y - r)) / (2 * r)], -1)
            t = _sample(img, uv, True)
            a = t[..., 3] * (Z >= zb[y0:y1 + 1, x0:x1 + 1] - 2 * scale)
            region = rgb[y0:y1 + 1, x0:x1 + 1]
            rgb[y0:y1 + 1, x0:x1 + 1] = region * (1 - a[..., None]) + t[..., :3] * a[..., None]
            cov[y0:y1 + 1, x0:x1 + 1] = np.maximum(cov[y0:y1 + 1, x0:x1 + 1], a)
    return rgb, cov


def shrink(rgb, cov, out=32, shadow=0.55):
    """Down to out x out over black, as the game's icons were made: the edges keep their darkening, the alpha is the
    coverage; under it the soft dark shadow the game's icons have, down and to the right."""
    s = rgb.shape[0] // out
    if shadow:
        off = int(s * 1.2)
        sh = np.zeros_like(cov)
        sh[off:, off:] = cov[:-off, :-off]
        # (blurred a little: a box of half a pixel)
        k = max(s // 2, 1)
        c = np.cumsum(np.cumsum(np.pad(sh, ((k, k), (k, k))), 0), 1)
        sh = (c[2 * k:, 2 * k:] - c[:-2 * k, 2 * k:] - c[2 * k:, :-2 * k] + c[:-2 * k, :-2 * k]) / (2 * k) ** 2
        sh = sh[:cov.shape[0], :cov.shape[1]] * shadow
        rgb = rgb * cov[..., None]  # (the object over its shadow, both over black)
        cov = cov + sh * (1 - cov)
        with np.errstate(invalid="ignore", divide="ignore"):
            rgb = np.where(cov[..., None] > 0, rgb / cov[..., None], 0)
    pre = (rgb * cov[..., None]).reshape(out, s, out, s, 3).mean(axis=(1, 3))
    a = cov.reshape(out, s, out, s).mean(axis=(1, 3))
    # (a touch of the unpremultiplied colour back in, so thin parts don't vanish into black)
    with np.errstate(invalid="ignore", divide="ignore"):
        un = np.where(a[..., None] > 0, pre / a[..., None], 0)
    col = pre * 0.55 + un * a[..., None] ** 0.5 * 0.45
    col = np.nan_to_num(col)
    img = np.dstack([np.clip(col * 255 + 0.5, 0, 255), np.clip(a * 255 + 0.5, 0, 255)]).astype(np.uint8)
    return Image.fromarray(img, "RGBA")


def render(meshes, yaw=0.0, pitch=0.0, roll=0.0, size=32, sprites=None, margin=0.06, **kw):
    rgb, cov = raster(meshes, yaw, pitch, roll, size=256, sprites=sprites, margin=margin, **kw)
    if size == 256:
        return Image.fromarray(np.dstack([rgb * 255, cov * 255]).clip(0, 255).astype(np.uint8), "RGBA")
    return shrink(rgb, cov, size)


def load_texture(path):
    return np.asarray(Image.open(path).convert("RGBA"), float) / 255.0
