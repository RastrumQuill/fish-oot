#!/usr/bin/env python3
"""Pop-in, seen and scored (fishmod/build.sh --script pop_ab): runs the ROM in ares, and at every tour stop keeps one
raw frame of each of the pop-in test's variants (fishmod_debug.c, FISHMOD_POP_AB): the frozen view as the game draws
it, the same view with every prop kind's distances taken 24 units nearer (one set's worth of travel), 150 units nearer,
and each kind alone 150 units nearer. The camera doesn't move between them, so whatever differs from the first is what
pops in, changes tier or finishes dissolving over that much travel: no parallax, no water, no sway in the difference.

Each difference gets two scores:
- LPIPS (needs torch and lpips): the distance between the two frames in a trained network's features, fitted to what
  people judge more different. The whole frame's, x1000, less the control's (the same view drawn twice). This is the
  number to calibrate distances to.
- FLIP (flip_metric.py), as a check on it and to say where on the screen: an error per pixel from a model of contrast
  sensitivity, colour and edges. Details below.

FLIP's part: scored with a model of what the eye sees when one picture replaces another, FLIP (flip_metric.py):
- each frame filtered by the eye's contrast sensitivity for its detail size (at --ppd pixels a degree of view: 20 is
  a TV across a room), colour difference taken where it's perceived, and a change that makes or breaks an edge or a
  dot weighted up: an error per pixel, 0 (nothing to see) to 1;
- the control (the same view drawn twice) gives the test's own noise: a pixel counts only beyond twice the control's
  error around it (the cut-outs' edges and the dither shimmer between two drawings; a pop doesn't);
- the pixels over 0.1 gathered into patches (touching pixels), each a pop: its size, its mean and peak error, where;
- a variant's strength is the error summed over its patches: a 30-pixel plant appearing at 0.3 is 9. As a first
  scale: under 3 not seen, 3 to 25 a flicker one can find, over 25 a pop one notices. (Starting points: look at the
  pairs the report saves and move them. Not modelled: where the viewer is looking, masking by the camera's motion.)

Writes <out>/report.md (per stop, per kind: the worst and the total), the worst pops as pictures (the view before, after,
and the patches lit), and <out>/pops.json for a calibration to read.

Usage: fishmod/tools/pop_ab_capture.py <rom> <outdir> [--stops N] [--ares PATH] [--display :90] [--timeout S]
       fishmod/tools/pop_ab_capture.py --score <outdir>      (again, from the frames a run kept)
Needs ares built with the ARES_AUTOSHOT patch (fishmod/docs/offline.md), numpy and Pillow; for LPIPS, torch and lpips
(pip install torch lpips: without them its columns are left empty).
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
VARIANTS = 36  # as fishmod_debug.c: 0 as it is, 1 all 24 nearer, 2 all 150 nearer, 3..34 each kind 150 nearer,
               # 35 as it is again (the control: what differs between two drawings of the same thing)
DE_SEEN = 0.10  # FLIP's error from which a pixel is part of a pop
PATCH_MIN = 3  # pixels: smaller patches are the dither's and the coverage's noise
DE_PEAK_MIN = 0.15  # and so are patches that nowhere reach this
SCALE = [(3, "not seen"), (25, "a flicker"), (1e9, "a pop")]


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


PPD = 20.0  # pixels per degree of view: 320 pixels across a 27" 4:3 set from 2 m (--ppd; flip_metric.py)


def de_map(a, b):
    """How visible the difference is at each pixel to someone watching one frame replace the other: FLIP's error
    (flip_metric.py: contrast sensitivity by detail size, perceived colour difference, edges and points), 0 to 1."""
    import flip_metric

    de = flip_metric.flip(to_screen(a), to_screen(b), PPD)
    de[:24, :24] = 0  # (the marker)
    return de


_LPIPS = None


def lpips_dist(a, b):
    """LPIPS (Zhang et al. 2018: the distance between two images in a trained network's features, fitted to people's
    judgements of which of two distortions looks more different), AlexNet, x1000; None without torch and lpips
    (pip install torch lpips). The whole frame's, as the measure is defined: a small pop is a small number."""
    global _LPIPS
    if _LPIPS is None:
        try:
            import lpips
            import torch
            import warnings

            warnings.filterwarnings("ignore")
            _LPIPS = (lpips.LPIPS(net="alex", verbose=False), torch)
        except Exception:
            _LPIPS = False
    if not _LPIPS:
        return None
    model, torch = _LPIPS

    def t(img):
        x = to_screen(img).astype(np.float32).copy()
        x[:24, :24] = 0  # (the marker)
        return torch.from_numpy(x / 127.5 - 1.0).permute(2, 0, 1)[None]

    with torch.no_grad():
        return 1000.0 * float(model(t(a), t(b)))


def spread(n, r=2):
    """Each pixel's largest value within r pixels (the noise's reach: an edge's shimmer moves a pixel or two)."""
    p = np.pad(n, r, mode="edge")
    return np.max([p[y:y + n.shape[0], x:x + n.shape[1]] for y in range(2 * r + 1) for x in range(2 * r + 1)], 0)


def score(a, b, noise=None):
    """The pops between two frames of one view: (strength, [patch: size, mean dE, peak dE, box], the dE map). With
    the control's map (the same view drawn twice), a pixel counts only by what it differs beyond twice the noise
    around it: the cut-outs' edges shimmer between two drawings (the coverage, the dither), a pop doesn't."""
    de = de_map(a, b)
    if noise is not None:
        de = np.where(de > 2.0 * noise, de, 0.0)
    found = []
    for p in patches(de > DE_SEEN):
        if len(p) < PATCH_MIN:
            continue
        ys, xs = np.array([q[0] for q in p]), np.array([q[1] for q in p])
        v = de[ys, xs]
        if v.max() < DE_PEAK_MIN:
            continue  # (the dissolve's dither and the coverage's last bit: a few pixels just over the line)
        found.append({"px": len(p), "mean": round(float(v.mean()), 3), "peak": round(float(v.max()), 3),
                      "sum": round(float(v.sum()), 2),
                      "box": [int(xs.min()), int(ys.min()), int(xs.max()), int(ys.max())]})
    found.sort(key=lambda f: -f["sum"])
    return round(sum(f["sum"] for f in found), 2), found, de


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
        while time.time() - t0 < args.timeout and len(have) < VARIANTS * (len(args.only) if args.only else args.stops):
            names = sorted(os.listdir(frames_dir))
            for name in names[:-1]:  # (the newest may still be being written)
                path = os.path.join(frames_dir, name)
                try:
                    img = np.asarray(Image.open(path).convert("RGB"))
                except OSError:
                    continue
                os.remove(path)
                m = read_marker(img)
                if not m or m[1] is None or m[0] > args.stops or m[1] >= VARIANTS or (args.only and m[0] not in args.only):
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
        ctl = os.path.join(out, f"s{stop:02d}_v{VARIANTS - 1:02d}.png")
        noise = spread(de_map(a, np.asarray(Image.open(ctl).convert("RGB")))) if os.path.exists(ctl) else None
        lp0 = lpips_dist(a, np.asarray(Image.open(ctl).convert("RGB"))) if os.path.exists(ctl) else 0.0
        if noise is not None:
            rows.append({"stop": stop, "variant": VARIANTS - 1, "what": "control",
                         "strength": score(a, np.asarray(Image.open(ctl).convert("RGB")))[0], "patches": [],
                         "lpips": lp0, "lpips_net": 0.0})
        for v in range(1, VARIANTS - 1):
            p = os.path.join(out, f"s{stop:02d}_v{v:02d}.png")
            if not os.path.exists(p):
                continue
            b = np.asarray(Image.open(p).convert("RGB"))
            s, found, de = score(a, b, noise)
            lp = lpips_dist(a, b)
            rows.append({"stop": stop, "variant": v, "what": label(v), "strength": s, "patches": found[:8],
                         "lpips": lp, "lpips_net": None if lp is None else round(max(lp - (lp0 or 0.0), 0.0), 2)})
            if s >= SCALE[0][0]:
                # the view before and after and the patches lit, at 3x
                A, B = to_screen(a).astype(np.uint8), to_screen(b).astype(np.uint8)
                heat = (A * 0.3).astype(np.uint8)
                lit = np.clip(de / 0.5, 0, 1)[..., None]
                heat = (heat * (1 - lit) + np.array([255, 60, 40]) * lit).astype(np.uint8)
                trio = np.concatenate([A, B, heat], 1)
                Image.fromarray(trio).resize((trio.shape[1] * 3, trio.shape[0] * 3), Image.NEAREST).save(
                    os.path.join(out, f"pop_s{stop:02d}_v{v:02d}.png"))
    json.dump(rows, open(os.path.join(out, "pops.json"), "w"), indent=1)
    with open(os.path.join(out, "report.md"), "w") as f:
        f.write("# Pop-in at the tour's stops\n\nStrength: FLIP's error (0 to 1 a pixel: how visible the change is to "
                f"someone watching, at {PPD:g} pixels a degree) summed over the patches that change, at 320x240: a "
                "30-pixel plant appearing at 0.3 is 9. Under 3 not seen, 3 to 25 a flicker, over 25 a pop (a first "
                "scale: look at the pairs and move it).\n\n")
        L = lambda r: "-" if r.get("lpips_net") is None else f"{r['lpips_net']:.1f}"
        f.write("LPIPS: the whole frame's distance x1000, less the control's (the same view drawn twice): the number to "
                "calibrate to. FLIP: the sanity check, and where on the screen.\n\n")
        f.write("## Every kind together\n\n| stop | 24 nearer (one set): LPIPS | FLIP | | 150 nearer: LPIPS | FLIP | | "
                "biggest patch at 150 | the control: LPIPS, FLIP |\n|---|---|---|---|---|---|---|---|---|\n")
        for stop in range(1, stops + 1):
            r1 = next((r for r in rows if r["stop"] == stop and r["variant"] == 1), None)
            r2 = next((r for r in rows if r["stop"] == stop and r["variant"] == 2), None)
            rc = next((r for r in rows if r["stop"] == stop and r["what"] == "control"), None)
            if not r1 or not r2:
                continue
            big = r2["patches"][0] if r2["patches"] else None
            f.write(f"| {stop} | {L(r1)} | {r1['strength']:.1f} | {verdict(r1['strength'])} | {L(r2)} | "
                    f"{r2['strength']:.1f} | {verdict(r2['strength'])} | "
                    + (f"{big['px']} px at FLIP {big['mean']} (peak {big['peak']}), at {big['box'][:2]}" if big else "-")
                    + " | " + (f"{rc['lpips']:.1f}, {rc['strength']:.1f}" if rc and rc.get("lpips") is not None
                               else f"-, {rc['strength']:.1f}" if rc else "-") + " |\n")
        f.write("\n## Each kind alone, 150 units nearer\n\n| kind | worst stop (by LPIPS) | LPIPS there | FLIP there | | "
                "LPIPS summed over the stops | FLIP summed | stops where FLIP says a flicker or more |\n"
                "|---|---|---|---|---|---|---|---|\n")
        kinds = {}
        for r in rows:
            if 3 <= r["variant"] < VARIANTS - 1:
                kinds.setdefault(r["what"], []).append(r)
        key = lambda r: (r["lpips_net"] if r.get("lpips_net") is not None else r["strength"])
        for what, rs in sorted(kinds.items(), key=lambda kv: -sum(key(r) for r in kv[1])):
            worst = max(rs, key=key)
            if worst["strength"] <= 0 and key(worst) < 0.5:
                continue
            f.write(f"| {what} | {worst['stop']} | {L(worst)} | {worst['strength']:.1f} | {verdict(worst['strength'])} | "
                    + ("-" if worst.get("lpips_net") is None else f"{sum(r['lpips_net'] for r in rs):.1f}")
                    + f" | {sum(r['strength'] for r in rs):.1f} | "
                    f"{sum(1 for r in rs if r['strength'] >= SCALE[0][0])} |\n")
        f.write("\nThe pairs: `pop_sSS_vVV.png` (the view, the view with the distances nearer, the patches lit), for "
                "every variant FLIP calls a flicker or more. `pops.json` has every patch.\n")
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
    ap.add_argument("--only", type=lambda t: [int(x) for x in t.split(",")], help="only these stops (pop_ab_cal)")
    ap.add_argument("--ppd", type=float, default=PPD, help="pixels per degree of view (default 20: a TV across a room)")
    args = ap.parse_args()
    globals()["PPD"] = args.ppd
    if args.score:
        return report(args.score, args.stops)
    if not args.rom or not args.out:
        sys.exit(__doc__)
    os.makedirs(args.out, exist_ok=True)
    capture(args)
    report(args.out, args.stops)


if __name__ == "__main__":
    main()
