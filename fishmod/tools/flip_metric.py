#!/usr/bin/env python3
"""FLIP (Andersson et al. 2020, "FLIP: A Difference Evaluator for Alternating Images"), the LDR model, in numpy: how
visible the difference between two images is to someone watching one replace the other, per pixel, 0 (none) to 1.

Written from the paper, not from NVIDIA's code (which this machine couldn't install): check it against
`flip-evaluator` before trusting the absolute numbers. What it models, which a colour distance doesn't:
- the eye's contrast sensitivity by detail size, per channel (each image filtered in an opponent colour space by how
  many pixels a degree of view holds: fine colour detail is lost, fine brightness detail much less);
- colour difference where it's perceived (CIELAB with the Hunt effect: colours differ more when bright), compressed
  so large differences don't swamp small ones;
- edges and points: a difference that makes or breaks an edge or a dot counts for more than the same difference in
  a flat patch.
What it doesn't: where the viewer is looking, the masking by a busy or moving picture, how long a change takes.

ppd: pixels per degree of view. An N64 picture is 320 pixels wide: on a 27" 4:3 set (0.55 m wide) from 2 m that is
about 20; a 20" set from 1.5 m about 21; an emulator window filling a monitor up close is nearer 8.
"""
import numpy as np

QC, PC, PT, QF, W = 0.7, 0.4, 0.95, 0.5, 0.082
REF_WHITE = np.array([0.950428545, 1.0, 1.088900371])  # D65
M_RGB2XYZ = np.array([[10135552 / 24577794, 8788810 / 24577794, 4435075 / 24577794],
                      [2613072 / 12288897, 8788810 / 12288897, 887015 / 12288897],
                      [1425312 / 73733382, 8788810 / 73733382, 70074185 / 73733382]])
M_XYZ2RGB = np.linalg.inv(M_RGB2XYZ)


def srgb2lin(c):
    return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)


def lin2ycxcz(c):
    xyz = (c @ M_RGB2XYZ.T) / REF_WHITE
    return np.stack([116 * xyz[..., 1] - 16, 500 * (xyz[..., 0] - xyz[..., 1]), 200 * (xyz[..., 1] - xyz[..., 2])], -1)


def ycxcz2lin(c):
    y = (c[..., 0] + 16) / 116
    xyz = np.stack([y + c[..., 1] / 500, y, y - c[..., 2] / 200], -1) * REF_WHITE
    return xyz @ M_XYZ2RGB.T


def lin2lab(c):
    xyz = (c @ M_RGB2XYZ.T) / REF_WHITE
    d = 6 / 29
    f = np.where(xyz > d ** 3, np.cbrt(np.maximum(xyz, 0)), xyz / (3 * d * d) + 4 / 29)
    return np.stack([116 * f[..., 1] - 16, 500 * (f[..., 0] - f[..., 1]), 200 * (f[..., 1] - f[..., 2])], -1)


def hunt(lab):
    return np.stack([lab[..., 0], 0.01 * lab[..., 0] * lab[..., 1], 0.01 * lab[..., 0] * lab[..., 2]], -1)


def hyab(a, b):
    d = a - b
    return np.abs(d[..., 0]) + np.sqrt(d[..., 1] ** 2 + d[..., 2] ** 2)


def conv2(img, k):
    """img (H, W) with kernel k (2r+1, 2r+1), the edges repeated."""
    r = k.shape[0] // 2
    p = np.pad(img, r, mode="edge")
    out = np.zeros_like(img, dtype=np.float64)
    for y in range(k.shape[0]):
        for x in range(k.shape[1]):
            if k[y, x] != 0.0:
                out += k[y, x] * p[y:y + img.shape[0], x:x + img.shape[1]]
    return out


def csf_kernels(ppd):
    a1 = {"A": 1.0, "RG": 1.0, "BY": 34.1}
    b1 = {"A": 0.0047, "RG": 0.0053, "BY": 0.04}
    a2 = {"A": 0.0, "RG": 0.0, "BY": 13.5}
    b2 = {"A": 1e-5, "RG": 1e-5, "BY": 0.025}
    r = int(np.ceil(3 * np.sqrt(0.04 / (2 * np.pi ** 2)) * ppd))
    x, y = np.meshgrid(np.arange(-r, r + 1), np.arange(-r, r + 1))
    d2 = (x ** 2 + y ** 2) / ppd ** 2
    ks = []
    for c in ("A", "RG", "BY"):
        g = a1[c] * np.sqrt(np.pi / b1[c]) * np.exp(-np.pi ** 2 * d2 / b1[c]) + \
            a2[c] * np.sqrt(np.pi / b2[c]) * np.exp(-np.pi ** 2 * d2 / b2[c])
        ks.append(g / g.sum())
    return ks


def prefilter(lin, ks):
    ycc = lin2ycxcz(lin)
    f = np.stack([conv2(ycc[..., i], ks[i]) for i in range(3)], -1)
    return np.clip(ycc2lin_safe(f), 0.0, 1.0)


def ycc2lin_safe(f):
    return ycxcz2lin(f)


def feature_kernels(ppd):
    sd = 0.5 * W * ppd
    r = int(np.ceil(3 * sd))
    x, y = np.meshgrid(np.arange(-r, r + 1), np.arange(-r, r + 1))
    g = np.exp(-(x ** 2 + y ** 2) / (2 * sd * sd))
    edge = -x * g
    point = (x ** 2 / (sd * sd) - 1) * g

    def norm(k):
        pos, neg = k[k > 0].sum(), -k[k < 0].sum()
        return np.where(k > 0, k / pos, k / neg)

    return norm(edge), norm(point)


def flip(a, b, ppd=20.0):
    """The FLIP error map of two sRGB images (H, W, 3, 0..255): 0 none, 1 the most."""
    A, B = srgb2lin(a.astype(np.float64) / 255), srgb2lin(b.astype(np.float64) / 255)
    ks = csf_kernels(ppd)
    la, lb = hunt(lin2lab(prefilter(A, ks))), hunt(lin2lab(prefilter(B, ks)))
    de = hyab(la, lb) ** QC
    green, blue = hunt(lin2lab(np.array([0.0, 1.0, 0.0]))), hunt(lin2lab(np.array([0.0, 0.0, 1.0])))
    cmax = hyab(green, blue) ** QC
    lim = PC * cmax
    dec = np.where(de < lim, PT / lim * de, PT + (de - lim) / (cmax - lim) * (1 - PT))
    # features, on the achromatic channel in 0..1
    ya, yb = (lin2ycxcz(A)[..., 0] + 16) / 116, (lin2ycxcz(B)[..., 0] + 16) / 116
    ek, pk = feature_kernels(ppd)
    mag = lambda img, k: np.sqrt(conv2(img, k) ** 2 + conv2(img, k.T) ** 2)
    def_ = (np.maximum(np.abs(mag(ya, ek) - mag(yb, ek)), np.abs(mag(ya, pk) - mag(yb, pk))) / np.sqrt(2)) ** QF
    return np.clip(dec, 0, 1) ** (1 - np.clip(def_, 0, 1))


if __name__ == "__main__":
    z = np.zeros((48, 64, 3), np.uint8)
    w = np.full((48, 64, 3), 255, np.uint8)
    g = np.full((48, 64, 3), 128, np.uint8)
    g2 = g.copy(); g2[20:28, 28:36] = 140
    print("same: %.4f  black/white: %.4f  grey with a faint 8x8 square (mean, peak): %.4f %.4f" %
          (flip(g, g).max(), flip(z, w).mean(), flip(g, g2).mean(), flip(g, g2).max()))
