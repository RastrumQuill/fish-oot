#!/usr/bin/env python3
"""Runs a ROM in ares on a virtual X display: the one launcher the capture tools share.

    with Ares(rom, workdir) as ares:
        img = ares.shot()   # the game view, as a PIL image

Each run gets its own copy of ares's settings (Ares.settings_copy): runs side by side don't race on the shared file,
and a run of another ares (e.g. the system's /usr/bin/ares, which rewrites it) can't break them.

CLI (what ares_capture.sh runs): harness.py capture <rom> <outdir> <seconds> [interval]
    a screenshot of the game view every [interval] seconds (default 5) for <seconds>.
Needs ARES=/path/to/ares (built with the N64 core), Xvfb and ImageMagick.
"""
import os
import signal
import subprocess
import sys
import time

# The game area of the ares window on the 1280x960 virtual display
GAME_BOX = (246, 178, 246 + 750, 178 + 570)
SCREEN = "1280x960x24"


def ensure_display(display, screen=SCREEN):
    """Starts Xvfb on the display unless something is already serving it."""
    env = dict(os.environ, DISPLAY=display)
    if subprocess.run(["xdpyinfo"], env=env, capture_output=True).returncode != 0:
        subprocess.Popen(["Xvfb", display, "-screen", "0", screen], stdout=subprocess.DEVNULL,
                         stderr=subprocess.DEVNULL)
        time.sleep(2)
    return env


def settings_copy(path):
    """Copies ares's settings to path, with the two settings another ares resets: an audio driver line (without it
    the N64 build quits at once: a black capture, no save) and Defocus: Allow (a headless window never has focus)."""
    src = os.path.expanduser("~/.local/share/ares/settings.bml")
    try:
        lines = open(src).read().splitlines()
    except OSError:
        lines = []
    lines = ["  Defocus: Allow" if l == "  Defocus: Pause" else l for l in lines]
    out, section, has_driver = [], None, False
    for l in lines:
        if l and not l[0].isspace():
            section = l
        elif section == "Audio" and l.startswith("  Driver:"):
            has_driver = True
        out.append(l)
    if not has_driver:
        if "Audio" in out:
            out.insert(out.index("Audio") + 1, "  Driver: None")
        else:
            out += ["Audio", "  Driver: None"]
    with open(path, "w") as f:
        f.write("\n".join(out) + "\n")
    return path


class Ares:
    """One ares run. workdir holds its settings copy, its log and the raw screenshots."""

    def __init__(self, rom, workdir, ares=None, display=None):
        self.ares = ares or os.environ.get("ARES", "ares")
        self.display = display or os.environ.get("DISP", ":99")
        self.rom = rom
        self.workdir = workdir
        self.proc = None

    def start(self):
        os.makedirs(self.workdir, exist_ok=True)
        self.env = ensure_display(self.display)
        settings = settings_copy(os.path.join(self.workdir, "settings.bml"))
        self.log = open(os.path.join(self.workdir, "ares.log"), "w")
        self.proc = subprocess.Popen(
            [self.ares, "--settings-file", settings, "--system", "Nintendo 64", "--setting", "Audio/Driver=None",
             "--setting", "Input/Defocus=Allow", "--no-file-prompt", self.rom],
            env=self.env, stdout=self.log, stderr=subprocess.STDOUT)
        self.started = time.time()
        return self

    def elapsed(self):
        return time.time() - self.started

    def running(self):
        return self.proc is not None and self.proc.poll() is None

    def shot(self, path=None):
        """Screenshots the display and returns the game view (a PIL image); saves it to path if given."""
        from PIL import Image

        raw = os.path.join(self.workdir, "screen.png")
        subprocess.run(["import", "-window", "root", raw], env=self.env, check=True)
        img = Image.open(raw).convert("RGB").crop(GAME_BOX)
        if path:
            img.save(path)
        return img

    def stop(self):
        if self.proc is not None:
            self.proc.kill()
            self.proc.wait()
            self.proc = None
            self.log.close()

    def __enter__(self):
        return self.start()

    def __exit__(self, *exc):
        self.stop()


def capture(rom, out, secs, interval=5):
    with Ares(rom, out) as ares:
        t = 0
        while t < secs:
            time.sleep(interval)
            t += interval
            ares.shot(os.path.join(out, f"shot_{t:04d}.png"))
    if os.path.exists(os.path.join(out, "screen.png")):
        os.remove(os.path.join(out, "screen.png"))


def main():
    if len(sys.argv) < 5 or sys.argv[1] != "capture":
        sys.exit(__doc__)
    signal.signal(signal.SIGTERM, lambda *_: sys.exit(1))  # a timeout's kill still stops ares (Ares.__exit__)
    capture(sys.argv[2], sys.argv[3], int(sys.argv[4]), int(sys.argv[5]) if len(sys.argv) > 5 else 5)


if __name__ == "__main__":
    main()
