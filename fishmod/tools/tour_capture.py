#!/usr/bin/env python3
"""Screenshots every stop of the camera tour (fishmod/build.sh --script tour_auto): runs the ROM in ares on a
virtual display and saves one shot per stop once it has settled, read from the marker in the top-left corner
(red = 8 * stop number, exact in the N64's 5-bit colour, black while the camera glides).

Usage: fishmod/tools/tour_capture.py <rom> <outdir> [--stops N] [--ares PATH] [--timeout SECONDS]
"""
import argparse
import os
import tempfile
import time

from harness import Ares


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("rom")
    ap.add_argument("out")
    ap.add_argument("--stops", type=int, default=22)
    ap.add_argument("--ares", default=os.environ.get("ARES", "ares"))
    ap.add_argument("--timeout", type=int, default=1500)
    ap.add_argument("--display", default=os.environ.get("DISP", ":99"))
    args = ap.parse_args()

    os.makedirs(args.out, exist_ok=True)
    ares = Ares(args.rom, tempfile.mkdtemp(), args.ares, args.display).start()
    # stops already on disk are kept (a second run fills in the gaps)
    saved = {int(f[5:7]) for f in os.listdir(args.out) if f.startswith("stop_")}
    prev = None
    start = time.time()
    try:
        while time.time() - start < args.timeout and len(saved) < args.stops:
            time.sleep(0.8)
            img = ares.shot()
            r, g, b = img.getpixel((10, 10))
            n = round(r / 8)
            if g > 30 or b > 30 or r < 8 or abs(r - n * 8) > 3:
                prev = None
                continue
            # the same stop on two shots in a row (not a stray reddish pixel)
            if n == prev and 1 <= n <= args.stops and n not in saved:
                img.save(os.path.join(args.out, f"stop_{n:02d}.png"))
                saved.add(n)
                print(f"stop {n} ({time.time() - start:.0f} s)", flush=True)
            prev = n
    finally:
        ares.stop()
    print(f"saved {len(saved)} of {args.stops}")


if __name__ == "__main__":
    main()
