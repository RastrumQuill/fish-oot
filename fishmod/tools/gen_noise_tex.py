#!/usr/bin/env python3
"""Soft grey noise that tiles perfectly (made in frequency space, so it wraps by construction).

Layered over a canopy texture at an unrelated scale, so the two repeats never line up.
  gen_noise_tex.py out.i4.png --size 64 --seed 3
"""
import argparse
import numpy as np
from PIL import Image


def soft_noise(n, seed, blobs=3.0, detail=0.25):
    """blobs: about how many soft blobs across the tile. detail: weight of a finer layer."""
    rng = np.random.default_rng(seed)
    f = np.fft.fftfreq(n)[:, None] ** 2 + np.fft.fftfreq(n)[None, :] ** 2
    f = np.sqrt(f) * n                       # cycles per tile
    white = np.fft.fft2(rng.standard_normal((n, n)))
    coarse = np.real(np.fft.ifft2(white * np.exp(-(f / blobs) ** 2)))
    fine = np.real(np.fft.ifft2(white * np.exp(-(f / (blobs * 3)) ** 2) * (f > blobs)))
    a = coarse / coarse.std() + detail * fine / max(fine.std(), 1e-9)
    lo, hi = np.percentile(a, 1), np.percentile(a, 99)
    return np.clip((a - lo) / (hi - lo), 0, 1)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--size", type=int, default=64)
    ap.add_argument("--seed", type=int, default=3)
    ap.add_argument("--blobs", type=float, default=3.0)
    a = ap.parse_args()
    q = np.clip(np.round(soft_noise(a.size, a.seed, a.blobs) * 15), 0, 15).astype(np.uint8)
    Image.fromarray(q * 17).save(a.out)
    print(f"{a.out}: {a.size}x{a.size} I4, {a.size * a.size // 2} bytes")


if __name__ == "__main__":
    main()
