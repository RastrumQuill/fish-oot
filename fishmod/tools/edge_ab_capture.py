#!/usr/bin/env python3
"""Exact A/B screenshots of the props' two edge modes (fishmod/build.sh --script edge_ab): runs the ROM in ares on a
virtual display, with ares saving its raw output frames itself (the console's VI output, before any scaling or
colour correction: needs ares built with the ARES_AUTOSHOT patch, see fishmod/docs/offline.md), and at every tour stop
keeps one frame of each mode once the scene is frozen. It reads the marker in
the top-left corner (red = 8 * stop number, green 0 / 248 for the default / other mode, blue 248 when ready). Then
it writes, per stop, the two shots side by side at 4x, a map of the pixels that differ, and a table.

Usage: fishmod/tools/edge_ab_capture.py <rom> <outdir> [--stops N] [--ares PATH] [--display :90] [--timeout S]
"""
import argparse
import os
import subprocess
import time

import numpy as np
from PIL import Image

SETTINGS = ["Audio/Driver=None", "Input/Defocus=Allow", "Video/Output=IntegerFixed", "Video/FixedScale=1",
            "Video/Shader=None", "Video/InterframeBlending=false", "Video/ColorEmulation=false",
            "Video/ColorBleed=false", "Nintendo64/Quality=SD", "Nintendo64/Supersampling=false"]


def read_marker(img):
    """(stop, mode or None) from the marker, or None while there's no marker. The raw frame is the VI's output
    (640x240 here, the picture offset by the borders), and the VI's filters move colours by a step or two, so the
    marker is read from the median of a block at the picture's top-left, to the nearest step of 8."""
    if img.shape[0] < 64 or img.shape[1] < 64:
        return None  # (before the VI is set up)
    lit = np.nonzero(img[:40, :60].astype(int).sum(2) > 30)
    if len(lit[0]) == 0:
        return None
    y0, x0 = lit[0].min(), lit[1].min()
    r, g, b = (int(v) for v in np.median(img[y0 + 2:y0 + 6, x0 + 4:x0 + 12].reshape(-1, 3), axis=0))
    stop = int(round(r / 8))
    if abs(r - 8 * stop) > 3 or not 1 <= stop <= 30 or min(g, 248 - g) > 6 or min(b, 248 - b) > 6:
        return None
    return stop, (None if b < 124 else (0 if g < 124 else 1))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("rom")
    ap.add_argument("out")
    ap.add_argument("--stops", type=int, default=22)
    ap.add_argument("--ares", default=os.environ.get("ARES", "ares"))
    ap.add_argument("--display", default=":90")
    ap.add_argument("--timeout", type=int, default=7200)
    args = ap.parse_args()

    os.makedirs(args.out, exist_ok=True)
    frames_dir = os.path.join(args.out, "frames")
    os.makedirs(frames_dir, exist_ok=True)
    env = dict(os.environ, DISPLAY=args.display, ARES_AUTOSHOT=frames_dir, ARES_AUTOSHOT_EVERY="10")
    if subprocess.run(["xdpyinfo"], env=env, capture_output=True).returncode != 0:
        subprocess.Popen(["Xvfb", args.display, "-screen", "0", "1600x1200x24"], stdout=subprocess.DEVNULL,
                         stderr=subprocess.DEVNULL)
        time.sleep(2)
    cmd = [args.ares, "--system", "Nintendo 64", "--no-file-prompt"]
    for s_ in SETTINGS:
        cmd += ["--setting", s_]
    ares = subprocess.Popen(cmd + [args.rom], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    shots = {}
    first = {}  # (stop, mode) -> the first ready frame, kept until a second one confirms the scene was steady
    t0 = time.time()
    try:
        while time.time() - t0 < args.timeout and len(shots) < 2 * args.stops:
            names = sorted(os.listdir(frames_dir))
            for name in names[:-1]:  # (the newest may still be being written)
                path = os.path.join(frames_dir, name)
                try:
                    img = np.asarray(Image.open(path).convert("RGB"))
                except OSError:
                    continue
                os.remove(path)
                m = read_marker(img)
                if not m or m[1] is None or m[0] > args.stops or m in shots:
                    continue
                if m not in first:
                    first[m] = img
                    continue
                steady = int((np.abs(first[m].astype(int) - img.astype(int)).max(2) > 0).sum())
                shots[m] = img
                stop, mode = m
                Image.fromarray(img).save(os.path.join(args.out, f"stop_{stop:02d}_{'new' if mode == 0 else 'old'}.png"))
                print(f"stop {stop} {'new' if mode == 0 else 'old'} edge mode ({time.time() - t0:.0f} s; {img.shape[1]}x"
                      f"{img.shape[0]}; {steady} px changed between two frames of the same mode)", flush=True)
            time.sleep(0.5)
    finally:
        ares.terminate()
    h, w = next(iter(shots.values())).shape[:2] if shots else (480, 640)

    rows = []
    for stop in range(1, args.stops + 1):
        if (stop, 0) not in shots or (stop, 1) not in shots:
            continue
        new, old = shots[(stop, 0)].astype(int), shots[(stop, 1)].astype(int)
        d = np.abs(new - old).max(2)
        d[:40, :40] = 0  # (the marker)
        changed = int((d > 0).sum())
        big = int((d > 24).sum())
        rows.append((stop, changed, big, float(d.mean())))
        # side by side at 4x (old left, new right) and the differing pixels in red on a dimmed picture
        pair = np.concatenate([old, new], axis=1).astype(np.uint8)
        Image.fromarray(pair).resize((pair.shape[1] * 4 // 2, pair.shape[0] * 4 // 2), Image.NEAREST).save(
            os.path.join(args.out, f"pair_{stop:02d}.png"))
        dim = (new * 0.35).astype(np.uint8)
        dim[d > 0] = [255, 60, 60]
        Image.fromarray(dim).resize((w * 2, h * 2), Image.NEAREST).save(os.path.join(args.out, f"diff_{stop:02d}.png"))
    with open(os.path.join(args.out, "summary.md"), "w") as f:
        f.write("| stop | pixels differing | of which by more than 24/255 | mean difference |\n|---|---|---|---|\n")
        for stop, changed, big, mean in rows:
            f.write(f"| {stop} | {changed} ({100 * changed / (w * h):.2f}%) | {big} | {mean:.3f} |\n")
    print(open(os.path.join(args.out, "summary.md")).read())


if __name__ == "__main__":
    main()
