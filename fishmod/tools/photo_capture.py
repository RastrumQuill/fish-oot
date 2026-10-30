#!/usr/bin/env python3
"""
Captures the wiki renders from the photo booth build (fishmod/build.sh --script photo).

Runs the ROM in ares on a virtual display, screenshots it every couple of seconds, reads which photo is
on screen from the marker square in the top-left corner (red = 16 * photo number), and keeps the last
shot of each photo once it has been on screen for a while. Saves fishmod/wiki/img/<species>.jpg (side view)
and <species>_34.jpg (three-quarter view).

Usage: fishmod/tools/photo_capture.py <rom> [--ares PATH] [--out DIR] [--timeout SECONDS]
"""

import argparse
import os
import sys
import tempfile
import time

from harness import Ares

# Must match sPhotoSpecies / PHOTO_POSES in src/code/fishmod_debug.c
PHOTOS = []
for sp in ["greengill", "hyrule_bass", "ordon_catfish", "hylian_loach", "sanke_carp", "hyrule_minnow"]:
    PHOTOS.append(sp)
    PHOTOS.append(sp + "_34")
PHOTOS.append("lineup")

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))


def read_marker(img):
    # marker square is at game pixels 4..11; sample its middle in the scaled window
    r, g, b = img.getpixel((10, 10))[:3]
    if b < 150 or g > 100:
        return None
    n = round(r / 16)
    return n if 1 <= n <= len(PHOTOS) else None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("rom")
    ap.add_argument("--ares", default=os.environ.get("ARES", "ares"))
    ap.add_argument("--out", default=os.path.join(ROOT, "fishmod", "wiki", "img"))
    ap.add_argument("--timeout", type=int, default=900)
    ap.add_argument("--display", default=os.environ.get("DISP", ":99"))
    args = ap.parse_args()

    os.makedirs(args.out, exist_ok=True)
    ares = Ares(args.rom, tempfile.mkdtemp(), args.ares, args.display).start()

    seen = {}  # photo number -> count of shots seen
    saved = set()
    last = None
    start = time.time()
    try:
        while time.time() - start < args.timeout and len(saved) < len(PHOTOS):
            time.sleep(2)
            img = ares.shot()
            n = read_marker(img)
            if n is None:
                continue
            seen[n] = seen.get(n, 0) + 1
            # wait for the fish to settle into the pose, then keep the newest shot of each photo
            if seen[n] >= 3 and n not in saved:
                name = PHOTOS[n - 1]
                frame = img.crop((30, 0, img.width, img.height))  # drop the marker column
                frame.save(os.path.join(args.out, name + ".jpg"), quality=92)
                saved.add(n)
                print(f"saved {name}.jpg ({len(saved)}/{len(PHOTOS)})", flush=True)
            last = n
    finally:
        ares.stop()

    missing = [PHOTOS[i - 1] for i in range(1, len(PHOTOS) + 1) if i not in saved]
    if missing:
        print("missing:", ", ".join(missing))
        sys.exit(1)


if __name__ == "__main__":
    main()
