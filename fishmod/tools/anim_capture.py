#!/usr/bin/env python3
"""
Records the animation studio (fishmod/build.sh --script anim --opt 4) to video clips: one per species and view
(from above, three-quarter), as MP4 and GIF, at the game's 20 frames per second.

Runs the ROM in ares on a virtual display and records the window with ffmpeg; duplicate frames (the display runs
faster than the game) are dropped, so the clips play at real speed. The corner marker (red = 16 * segment) says
which species and view is on screen, and splits the recording.

--quality HD or UHD renders at 2x or 4x the N64's resolution (ares' accurate renderer through Vulkan; slow in
software). The game itself is unchanged: only the rasterizing is finer.

Usage: fishmod/tools/anim_capture.py <rom> [--ares PATH] [--out DIR] [--quality SD|HD|UHD] [--seconds N]
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
import time

# Must match sStudioSpecies and STUDIO_VIEWS in src/code/fishmod_debug.c
SPECIES = ["greengill", "hyrule_bass", "ordon_catfish", "hylian_loach", "sanke_carp", "hyrule_minnow"]
VIEWS = ["top", "34"]
SCALES = {"SD": 2, "HD": 2, "UHD": 4}


def window_geometry(display):
    env = dict(os.environ, DISPLAY=display)
    out = subprocess.run(["xdotool", "search", "--class", "ares"], env=env, capture_output=True, text=True).stdout.split()
    best = None
    for wid in out:
        geo = subprocess.run(["xdotool", "getwindowgeometry", wid], env=env, capture_output=True, text=True).stdout
        try:
            pos = geo.split("Position: ")[1].split(" ")[0].split(",")
            size = geo.split("Geometry: ")[1].strip().split("x")
            x, y, w, h = int(pos[0]), int(pos[1]), int(size[0]), int(size[1])
        except (IndexError, ValueError):
            continue
        if best is None or w * h > best[2] * best[3]:
            best = (x, y, w, h)
    return best


def viewport(frame_png, scale):
    """The game picture inside the window, found from the studio's marker square (at 4,4 of the 320x240 picture)."""
    from PIL import Image

    img = Image.open(frame_png).convert("RGB")
    w, h = img.size
    px = img.load()
    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y]
            if b > 200 and g < 60:
                # the marker's corner, as long as the square really is there
                ok = all(px[min(w - 1, x + dx), min(h - 1, y + dy)][2] > 200 for dx in (2, 5) for dy in (2, 5))
                if ok:
                    return x - 4 * scale, y - 4 * scale, 320 * scale, 240 * scale
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("rom")
    ap.add_argument("--ares", default=os.environ.get("ARES", "ares"))
    ap.add_argument("--out", default="fishmod/out/anim")
    ap.add_argument("--quality", default="SD", choices=list(SCALES))
    ap.add_argument("--seconds", type=int, default=900, help="how long to record (wall clock)")
    ap.add_argument("--display", default=":105")
    ap.add_argument("--keep-raw", action="store_true")
    args = ap.parse_args()

    os.makedirs(args.out, exist_ok=True)
    work = tempfile.mkdtemp()
    rom = os.path.join(work, "anim.z64")
    shutil.copy(args.rom, rom)
    env = dict(os.environ, DISPLAY=args.display)

    # ares saves its settings when it quits: keep the user's (the other capture tools expect the default window)
    settings = os.path.expanduser("~/.local/share/ares/settings.bml")
    settings_backup = os.path.join(work, "settings.bml")
    if os.path.exists(settings):
        shutil.copy(settings, settings_backup)

    xvfb = subprocess.Popen(["Xvfb", args.display, "-screen", "0", "2700x2100x24"], stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL)
    time.sleep(2)
    ares = subprocess.Popen([args.ares, "--system", "Nintendo 64", "--setting", "Audio/Driver=None",
                             "--setting", "Input/Defocus=Allow", "--setting", "Nintendo64/Quality=" + args.quality,
                             "--setting", "Video/WindowWidth=%d" % (320 * SCALES[args.quality]),
                             "--setting", "Video/WindowHeight=%d" % (240 * SCALES[args.quality]),
                             "--no-file-prompt", rom], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        # wait for the window and the studio (the backdrop) to come up
        geo = None
        vp = None
        for _ in range(120):
            time.sleep(2)
            geo = window_geometry(args.display)
            if geo is None:
                continue
            shot = os.path.join(work, "probe.png")
            subprocess.run(["import", "-window", "root", "-crop", "%dx%d+%d+%d" % (geo[2], geo[3], geo[0], geo[1]),
                            "+repage", shot], env=env, check=False)
            vp = viewport(shot, SCALES[args.quality])
            if vp is not None:
                break
        if vp is None:
            sys.exit("the studio never came up")
        # the backdrop may not reach the very edges: snap to the window's picture area (4:3) around it
        x, y, w, h = vp
        print("window", geo, "viewport", vp)

        raw = os.path.join(work, "raw.mkv")
        subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-f", "x11grab", "-draw_mouse", "0", "-framerate", "60",
                        "-video_size", "%dx%d" % (w, h), "-i", "%s+%d,%d" % (args.display, geo[0] + x, geo[1] + y),
                        "-t", str(args.seconds), "-c:v", "libx264", "-preset", "ultrafast", "-crf", "10", raw],
                       env=env, check=True)
    finally:
        ares.terminate()
        time.sleep(1)
        ares.kill()
        xvfb.terminate()
        if os.path.exists(settings_backup):
            shutil.copy(settings_backup, settings)

    # one frame per game frame, at the game's rate
    dedup = os.path.join(work, "dedup.mkv")
    subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-i", raw, "-vf",
                    "mpdecimate=hi=64*4:lo=64*2:frac=0.1,setpts=N/20/TB", "-r", "20", "-c:v", "libx264",
                    "-preset", "fast", "-crf", "12", dedup], check=True)

    # read the marker of every frame (a small copy is enough)
    probe = subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries", "stream=width,height",
                            "-of", "csv=p=0", dedup], capture_output=True, text=True, check=True).stdout.strip()
    vw, vh = (int(v) for v in probe.split(",")[:2])
    sw, sh = 160, 120
    pipe = subprocess.Popen(["ffmpeg", "-loglevel", "error", "-i", dedup, "-vf", "scale=%d:%d" % (sw, sh), "-f",
                             "rawvideo", "-pix_fmt", "rgb24", "-"], stdout=subprocess.PIPE)
    markers = []
    mx, my = 3, 3  # the marker square (4..11 of 320) at half scale
    while True:
        buf = pipe.stdout.read(sw * sh * 3)
        if len(buf) < sw * sh * 3:
            break
        i = (my * sw + mx) * 3
        r, g, b = buf[i], buf[i + 1], buf[i + 2]
        markers.append(int(round(r / 16.0)) if b > 150 and g < 90 else 0)
    pipe.wait()

    # runs of the same marker -> clips (the first and last frames of each run are dropped: camera cuts)
    runs = []
    start = 0
    for n in range(1, len(markers) + 1):
        if n == len(markers) or markers[n] != markers[start]:
            runs.append((markers[start], start, n))
            start = n
    done = set()
    for m, a, b in runs:
        seg = m - 1
        if m == 0 or seg >= len(SPECIES) * len(VIEWS) or (b - a) < 40 or seg in done:
            continue
        done.add(seg)
        name = "%s_%s" % (SPECIES[seg // len(VIEWS)], VIEWS[seg % len(VIEWS)])
        a += 2
        b -= 2
        mp4 = os.path.join(args.out, name + ".mp4")
        gif = os.path.join(args.out, name + ".gif")
        sel = "select=between(n\\,%d\\,%d),setpts=N/20/TB" % (a, b - 1)
        subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-i", dedup, "-vf", sel, "-r", "20", "-c:v", "libx264",
                        "-pix_fmt", "yuv420p", "-crf", "16", mp4], check=True)
        gw = min(vw, 640)
        subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-i", dedup, "-vf",
                        sel + ",scale=%d:-1:flags=lanczos,split[s0][s1];[s0]palettegen=max_colors=128[p];"
                        "[s1][p]paletteuse=dither=bayer" % gw, "-r", "20", gif], check=True)
        print("saved %s (%d frames)" % (name, b - a))
    if args.keep_raw:
        shutil.copy(dedup, os.path.join(args.out, "all.mkv"))
    shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    main()
