#!/usr/bin/env python3
"""Pop-in, seen and scored (fishmod/build.sh --script pop_ab): runs the ROM in ares, and at every tour stop keeps one
raw frame of each of the pop-in test's variants (fishmod_debug.c, FISHMOD_POP_AB): the frozen view as the game draws
it, the same view with every prop kind's distances taken 24 units nearer (one set's worth of travel), 150 units nearer,
and each kind alone 150 units nearer. The camera doesn't move between them, so whatever differs from the first is what
pops in, changes tier or finishes dissolving over that much travel: no parallax, no water, no sway in the difference.

Each difference is scored as the eye would take it, not counted in pixels:
- both frames to Oklab (a perceptually even colour space), blurred a little (sigma 1 pixel at 320x240: the VI's filter
  and the screen), and the distance between them per pixel, x100 (dE: about 1 is the least difference one can see side
  by side; 2 is taken here as visible in a moving picture);
- the pixels over dE 2 gathered into patches (touching pixels), each a pop: its size, its mean dE, where it is;
- a variant's strength is the sum of dE over its patches (dE x pixels): a 30-pixel plant appearing at dE 10 is 300.
  As a first scale: under 40 nobody sees it, 40 to 250 is a flicker one can find when looking for it, over 250 is a pop
  one notices. (These are starting points: look at the pairs the report saves and move them.)

Writes <out>/report.md (per stop, per kind: the worst and the total), the worst pops as pictures (the view before, after,
and the patches lit), and <out>/pops.json for a calibration to read.

Usage: fishmod/tools/pop_ab_capture.py <rom> <outdir> [--stops N] [--ares PATH] [--display :90] [--timeout S]
       fishmod/tools/pop_ab_capture.py --score <outdir>      (again, from the frames a run kept)
Needs ares built with the ARES_AUTOSHOT patch (fishmod/docs/offline.md), numpy and Pillow.
"""
import argparse
import json
import os
import re
import subprocess
import sys
import time

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
SETTINGS = ["Audio/Driver=None", "Input/Defocus=Allow", "Video/Output=IntegerFixed", "Video/FixedScale=1",
            "Video/Shader=None", "Video/InterframeBlending=false", "Video/ColorEmulation=false",
            "Video/ColorBleed=false", "Video/Quality=SD", "Video/Supersampling=false"]
VARIANTS = 35  # as fishmod_debug.c: 0 as it is, 1 all 24 nearer, 2 all 150 nearer, 3.. each kind 150 nearer
DE_SEEN = 2.0
PATCH_MIN = 3  # pixels: smaller patches are the dither's and the coverage's noise
DE_PEAK_MIN = 3.0  # and so are patches that nowhere reach this (measured: kinds not in view scored 7 to 30 without it)
SCALE = [(40, "not seen"), (250, "a flicker"), (1e9, "a pop")]


def kind_names():
    s = open(os.path.join(HERE, "..", "..", "src", "fishmod_data", "fishmod_props_kinds.h")).read()
    body = s[s.index("#define FISHMOD_PROP_KIND_INFO(X)"):]
    return [m.lower() for m in re.findall(r"X\((\w+),", body.split("\n\n")[0])]


def read_marker(img):
    """(stop, variant or None while it settles), or None with no marker (as edge_ab_capture.py reads its own)."""
    if img.shape[0] < 64 or img.shape[1] < 64:
        return None
    lit = np.nonzero(img[:40, :60].astype(int).sum(2) > 30)
    if len(lit[0]) == 0:
        return None
    y0, x0 = lit[0].min(), lit[1].min()
    r, g, b = (int(v) for v in np.median(img[y0 + 2:y0 + 6, x0 + 4:x0 + 12].reshape(-1, 3), axis=0))
    stop = int(round(r / 8))
    if abs(r - 8 * stop) > 3 or not 1 <= stop <= 30:
        return None
    if b < 64:
        return stop, None
    v = int(round(g / 8))
    if abs(g - 8 * v) > 3:
        return None
    return stop, v + (32 if b < 190 else 0)


def oklab(rgb):
    c = rgb.astype(np.float64) / 255.0
    c = np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)
    l = 0.4122214708 * c[..., 0] + 0.5363325363 * c[..., 1] + 0.0514459929 * c[..., 2]
    m = 0.2119034982 * c[..., 0] + 0.6806995451 * c[..., 1] + 0.1073969566 * c[..., 2]
    s = 0.0883024619 * c[..., 0] + 0.2817188376 * c[..., 1] + 0.6299787005 * c[..., 2]
    l, m, s = np.cbrt(l), np.cbrt(m), np.cbrt(s)
    return np.stack([0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s,
                     1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s,
                     0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s], -1)


def blur(a, sigma=1.0):
    r = int(3 * sigma + 0.5)
    k = np.exp(-0.5 * (np.arange(-r, r + 1) / sigma) ** 2)
    k /= k.sum()
    for axis in (0, 1):
        pad = [(0, 0)] * a.ndim
        pad[axis] = (r, r)
        p = np.pad(a, pad, mode="edge")
        a = sum(k[i] * np.take(p, np.arange(i, i + a.shape[axis]), axis=axis) for i in range(2 * r + 1))
    return a


def to_screen(img):
    """The raw frame at the game's 320x240 (ares gives 640 wide: each pixel twice)."""
    if img.shape[1] >= 2 * 300:
        img = img[:, : img.shape[1] // 2 * 2].reshape(img.shape[0], -1, 2, 3).mean(2)
    return img


def patches(mask):
    """Touching pixels over the threshold, as lists of (y, x)."""
    seen = np.zeros_like(mask, bool)
    out = []
    h, w = mask.shape
    for y, x in zip(*np.nonzero(mask)):
        if seen[y, x]:
            continue
        stack = [(y, x)]
        seen[y, x] = True
        cur = []
        while stack:
            cy, cx = stack.pop()
            cur.append((cy, cx))
            for ny in (cy - 1, cy, cy + 1):
                for nx in (cx - 1, cx, cx + 1):
                    if 0 <= ny < h and 0 <= nx < w and mask[ny, nx] and not seen[ny, nx]:
                        seen[ny, nx] = True
                        stack.append((ny, nx))
        out.append(cur)
    return out


def score(a, b):
    """The pops between two frames of one view: (strength, [patch: size, mean dE, peak dE, box], the dE map)."""
    A, B = to_screen(a), to_screen(b)
    de = 100.0 * np.sqrt(((blur(oklab(A)) - blur(oklab(B))) ** 2).sum(-1))
    de[:24, :24] = 0  # (the marker)
    found = []
    for p in patches(de > DE_SEEN):
        if len(p) < PATCH_MIN:
            continue
        ys, xs = np.array([q[0] for q in p]), np.array([q[1] for q in p])
        v = de[ys, xs]
        if v.max() < DE_PEAK_MIN:
            continue  # (the dissolve's dither and the coverage's last bit: a few pixels just over the line)
        found.append({"px": len(p), "mean": round(float(v.mean()), 1), "peak": round(float(v.max()), 1),
                      "sum": round(float(v.sum()), 1),
                      "box": [int(xs.min()), int(ys.min()), int(xs.max()), int(ys.max())]})
    found.sort(key=lambda f: -f["sum"])
    return round(sum(f["sum"] for f in found), 1), found, de


def verdict(s):
    return next(name for lim, name in SCALE if s < lim)


def capture(args):
    frames_dir = os.path.join(args.out, "raw")
    os.makedirs(frames_dir, exist_ok=True)
    env = dict(os.environ, DISPLAY=args.display, ARES_AUTOSHOT=frames_dir, ARES_AUTOSHOT_EVERY="4")
    if subprocess.run(["xdpyinfo"], env=env, capture_output=True).returncode != 0:
        subprocess.Popen(["Xvfb", args.display, "-screen", "0", "1600x1200x24"], stdout=subprocess.DEVNULL,
                         stderr=subprocess.DEVNULL)
        time.sleep(2)
    cmd = [args.ares, "--system", "Nintendo 64", "--no-file-prompt"]
    for s_ in SETTINGS:
        cmd += ["--setting", s_]
    ares = subprocess.Popen(cmd + [args.rom], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    have = set()
    t0 = time.time()
    try:
        while time.time() - t0 < args.timeout and len(have) < VARIANTS * args.stops:
            names = sorted(os.listdir(frames_dir))
            for name in names[:-1]:  # (the newest may still be being written)
                path = os.path.join(frames_dir, name)
                try:
                    img = np.asarray(Image.open(path).convert("RGB"))
                except OSError:
                    continue
                os.remove(path)
                m = read_marker(img)
                if not m or m[1] is None or m[0] > args.stops or m[1] >= VARIANTS:
                    continue
                # (the last settled frame of a variant is the one kept: written over until the variant changes)
                Image.fromarray(img).save(os.path.join(args.out, f"s{m[0]:02d}_v{m[1]:02d}.png"))
                if m not in have:
                    have.add(m)
                    if m[1] == 0:
                        print(f"stop {m[0]} ({time.time() - t0:.0f} s)", flush=True)
            time.sleep(0.5)
    finally:
        ares.terminate()
    print(f"kept {len(have)} of {VARIANTS * args.stops} frames")


def report(out, stops):
    names = kind_names()
    label = lambda v: "all kinds, 24 nearer" if v == 1 else "all kinds, 150 nearer" if v == 2 else \
        (names[v - 3] if v - 3 < len(names) else f"kind {v - 3}")
    rows = []
    for stop in range(1, stops + 1):
        base = os.path.join(out, f"s{stop:02d}_v00.png")
        if not os.path.exists(base):
            continue
        a = np.asarray(Image.open(base).convert("RGB"))
        for v in range(1, VARIANTS):
            p = os.path.join(out, f"s{stop:02d}_v{v:02d}.png")
            if not os.path.exists(p):
                continue
            b = np.asarray(Image.open(p).convert("RGB"))
            s, found, de = score(a, b)
            rows.append({"stop": stop, "variant": v, "what": label(v), "strength": s, "patches": found[:8]})
            if s >= SCALE[0][0]:
                # the view before and after and the patches lit, at 3x
                A, B = to_screen(a).astype(np.uint8), to_screen(b).astype(np.uint8)
                heat = (A * 0.3).astype(np.uint8)
                lit = np.clip(de / 12.0, 0, 1)[..., None]
                heat = (heat * (1 - lit) + np.array([255, 60, 40]) * lit).astype(np.uint8)
                trio = np.concatenate([A, B, heat], 1)
                Image.fromarray(trio).resize((trio.shape[1] * 3, trio.shape[0] * 3), Image.NEAREST).save(
                    os.path.join(out, f"pop_s{stop:02d}_v{v:02d}.png"))
    json.dump(rows, open(os.path.join(out, "pops.json"), "w"), indent=1)
    with open(os.path.join(out, "report.md"), "w") as f:
        f.write("# Pop-in at the tour's stops\n\nStrength: the sum of dE over the patches that differ (dE x pixels at "
                "320x240); under 40 not seen, 40 to 250 a flicker, over 250 a pop.\n\n")
        f.write("## Every kind together\n\n| stop | 24 units nearer (one set) | | 150 units nearer | | biggest patch at 150 |\n"
                "|---|---|---|---|---|---|\n")
        for stop in range(1, stops + 1):
            r1 = next((r for r in rows if r["stop"] == stop and r["variant"] == 1), None)
            r2 = next((r for r in rows if r["stop"] == stop and r["variant"] == 2), None)
            if not r1 or not r2:
                continue
            big = r2["patches"][0] if r2["patches"] else None
            f.write(f"| {stop} | {r1['strength']:.0f} | {verdict(r1['strength'])} | {r2['strength']:.0f} | "
                    f"{verdict(r2['strength'])} | "
                    + (f"{big['px']} px at dE {big['mean']} (peak {big['peak']}), at {big['box'][:2]}" if big else "-")
                    + " |\n")
        f.write("\n## Each kind alone, 150 units nearer\n\n| kind | worst stop | its strength | | patches there | "
                "stops where it's a flicker or more | total over the stops |\n|---|---|---|---|---|---|---|\n")
        kinds = {}
        for r in rows:
            if r["variant"] >= 3:
                kinds.setdefault(r["what"], []).append(r)
        for what, rs in sorted(kinds.items(), key=lambda kv: -max(r["strength"] for r in kv[1])):
            worst = max(rs, key=lambda r: r["strength"])
            if worst["strength"] <= 0:
                continue
            f.write(f"| {what} | {worst['stop']} | {worst['strength']:.0f} | {verdict(worst['strength'])} | "
                    f"{len(worst['patches'])} | {sum(1 for r in rs if r['strength'] >= SCALE[0][0])} | "
                    f"{sum(r['strength'] for r in rs):.0f} |\n")
        f.write("\nThe pairs: `pop_sSS_vVV.png` (the view, the view with the distances nearer, the patches lit), for "
                "every variant of strength 40 or more. `pops.json` has every patch.\n")
    print(open(os.path.join(out, "report.md")).read())


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("rom", nargs="?")
    ap.add_argument("out", nargs="?")
    ap.add_argument("--score", metavar="OUTDIR")
    ap.add_argument("--stops", type=int, default=22)
    ap.add_argument("--ares", default=os.environ.get("ARES", "ares"))
    ap.add_argument("--display", default=":90")
    ap.add_argument("--timeout", type=int, default=7200)
    args = ap.parse_args()
    if args.score:
        return report(args.score, args.stops)
    if not args.rom or not args.out:
        sys.exit(__doc__)
    os.makedirs(args.out, exist_ok=True)
    capture(args)
    report(args.out, args.stops)


if __name__ == "__main__":
    main()
