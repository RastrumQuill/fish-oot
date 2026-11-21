#!/usr/bin/env python3
"""The fishing feel tuner: the fight's and the bite's numbers changed from the desktop while the console plays.

A test build (play.bat, Lake Hylia or Kokiri Forest) reads them over the SummerCart64's USB once a frame
(src/code/fishmod_feel.inc.c). This window runs `sc64deployer debug` and talks through it: move a slider (or type a
number and press Enter) and the game has it on its next frame; the game says back what it holds, and the window shows
that. The numbers, their defaults and their ranges come from include/fishmod_feel.h.

  Get       ask the game for every number (what it really holds)
  Defaults  every number back to the build's default
  Save      the numbers to fishmod/pc/feel_saved.txt (send it over, or commit it: it becomes the new defaults)
  Load      a saved file's numbers sent to the game

Usage (feel.bat does this): python fishmod/pc/feel.py [--deployer sc64deployer.exe] [--fake DIR] [--check]
  --fake DIR   no cart: talk to the emulator's (ares with ARES_SC64_DIR=DIR: lines in as binaryin-*.txt files, the
               game's out as binaryout-*.bin)
  --check      no window: ask for every number, print them, and stop (a test of the link)

Upload the game with play.bat first: while this window is open it holds the cart's USB, so close it before the next
upload.
"""
import argparse
import glob
import os
import queue
import re
import subprocess
import sys
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
HEADER = os.path.join(ROOT, "include", "fishmod_feel.h")
SAVED = os.path.join(HERE, "feel_saved.txt")
sys.path.insert(0, HERE)


def read_list():
    """[(name, default, lo, hi, what)] from include/fishmod_feel.h's FEEL(...) lines"""
    out = []
    pat = re.compile(r'FEEL\(\s*\w+\s*,\s*"(\w+)"\s*,\s*([-\d.]+)f?\s*,\s*([-\d.]+)f?\s*,\s*([-\d.]+)f?\s*,\s*"([^"]*)"\)')
    for line in open(HEADER, encoding="utf-8"):
        m = pat.search(line)
        if m:
            out.append((m.group(1), float(m.group(2)), float(m.group(3)), float(m.group(4)), m.group(5)))
    return out


class Link:
    """the lines to and from the game: through `sc64deployer debug`, or the emulator's folder (--fake)"""

    def __init__(self, deployer=None, fake=None):
        self.fake = fake
        self.heard = queue.Queue()
        self.n = 0
        if fake:
            os.makedirs(fake, exist_ok=True)
            self.seen = set(glob.glob(os.path.join(fake, "binaryout-*.bin")))
            threading.Thread(target=self._watch, daemon=True).start()
        else:
            flags = subprocess.CREATE_NEW_PROCESS_GROUP if os.name == "nt" else 0
            self.proc = subprocess.Popen([deployer, "debug"], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                         stderr=subprocess.STDOUT, cwd=os.path.join(HERE, "work"),
                                         creationflags=flags)
            threading.Thread(target=self._read, daemon=True).start()

    def _read(self):
        for raw in iter(self.proc.stdout.readline, b""):
            self.heard.put(raw.decode("utf-8", errors="replace").strip())
        self.heard.put("<link closed>")

    def _watch(self):
        while True:
            for f in sorted(glob.glob(os.path.join(self.fake, "binaryout-*.bin"))):
                if f not in self.seen:
                    self.seen.add(f)
                    try:
                        self.heard.put(open(f, "rb").read().replace(b"\0", b"").decode("utf-8", "replace").strip())
                    except OSError:
                        pass
            time.sleep(0.1)

    def say(self, line):
        if self.fake:
            self.n += 1
            path = os.path.join(self.fake, "binaryin-%d-%06d.txt" % (int(time.time()), self.n))
            open(path + ".part", "w").write(line)
            os.replace(path + ".part", path)
        else:
            self.proc.stdin.write((line + "\n").encode())
            self.proc.stdin.flush()

    def close(self):
        if not self.fake and self.proc.poll() is None:
            try:
                self.proc.stdin.close()
                self.proc.wait(timeout=5)
            except Exception:
                self.proc.kill()


def parse_feel(line):
    """("name", value) from a "feel NAME VALUE" line (sc64deployer may put text before it), else None"""
    m = re.search(r"\bfeel (\w+) (-?[\d.]+)", line)
    return (m.group(1), float(m.group(2))) if m else None


def check(link, items):
    link.say("get")
    got = {}
    end = time.time() + 20
    while time.time() < end and len(got) < len(items):
        try:
            line = link.heard.get(timeout=0.5)
        except queue.Empty:
            continue
        p = parse_feel(line)
        if p:
            got[p[0]] = p[1]
    for name, default, lo, hi, what in items:
        print("%-14s %8s  (default %g)" % (name, got.get(name, "?"), default))
    return len(got) == len(items)


def window(link, items):
    import tkinter as tk
    from tkinter import filedialog, ttk

    root = tk.Tk()
    root.title("fish-oot: fishing feel")
    vars_, scales, status = {}, {}, tk.StringVar(value="Waiting for the game... (Get asks again)")
    frame = ttk.Frame(root, padding=8)
    frame.grid(sticky="nsew")
    root.columnconfigure(0, weight=1)
    frame.columnconfigure(2, weight=1)
    sending = {"quiet": False}

    def send(name):
        try:
            v = float(vars_[name].get())
        except ValueError:
            return
        link.say("set %s %g" % (name, v))

    for row, (name, default, lo, hi, what) in enumerate(items):
        ttk.Label(frame, text=name, width=14).grid(row=row, column=0, sticky="w")
        var = tk.StringVar(value="%g" % default)
        vars_[name] = var
        ent = ttk.Entry(frame, textvariable=var, width=8)
        ent.grid(row=row, column=1, padx=4)
        ent.bind("<Return>", lambda e, n=name: send(n))
        res = (hi - lo) / 200.0
        sc = tk.Scale(frame, from_=lo, to=hi, resolution=res, orient="horizontal", showvalue=False, length=260)
        sc.set(default)
        sc.grid(row=row, column=2, sticky="ew")

        def moved(val, n=name):
            if not sending["quiet"]:
                vars_[n].set("%.3g" % float(val))

        sc.configure(command=moved)
        sc.bind("<ButtonRelease-1>", lambda e, n=name: send(n))
        scales[name] = sc
        ttk.Button(frame, text="default", width=7,
                   command=lambda n=name, d=default: (vars_[n].set("%g" % d), send(n))).grid(row=row, column=3)
        ttk.Label(frame, text=what, foreground="#666").grid(row=row, column=4, sticky="w", padx=6)

    def save():
        with open(SAVED, "w", encoding="utf-8") as f:
            f.write("# fishing feel, saved %s (fishmod/pc/feel.py): name value\n" % time.strftime("%Y-%m-%d %H:%M"))
            for name, *_ in items:
                f.write("%s %s\n" % (name, vars_[name].get()))
        status.set("Saved to %s" % SAVED)

    def load():
        path = filedialog.askopenfilename(initialdir=HERE, filetypes=[("feel", "*.txt"), ("all", "*")])
        if not path:
            return
        for line in open(path, encoding="utf-8"):
            parts = line.split()
            if len(parts) == 2 and parts[0] in vars_:
                link.say("set %s %s" % (parts[0], parts[1]))
        status.set("Sent %s" % os.path.basename(path))

    bar = ttk.Frame(frame)
    bar.grid(row=len(items), column=0, columnspan=5, pady=(8, 0), sticky="w")
    ttk.Button(bar, text="Get", command=lambda: link.say("get")).pack(side="left")
    ttk.Button(bar, text="Defaults", command=lambda: link.say("reset")).pack(side="left", padx=4)
    ttk.Button(bar, text="Save", command=save).pack(side="left")
    ttk.Button(bar, text="Load", command=load).pack(side="left", padx=4)
    ttk.Label(frame, textvariable=status).grid(row=len(items) + 1, column=0, columnspan=5, sticky="w", pady=(6, 0))

    def poll():
        while True:
            try:
                line = link.heard.get_nowait()
            except queue.Empty:
                break
            p = parse_feel(line)
            if p and p[0] in vars_:
                sending["quiet"] = True
                vars_[p[0]].set("%g" % p[1])
                scales[p[0]].set(p[1])
                sending["quiet"] = False
                status.set("The game has %s = %g" % p)
            elif "feel-end" in line:
                status.set("In step with the game")
            elif "feel-unknown" in line:
                status.set("The game didn't know that name (an older build?)")
            elif "<link closed>" in line:
                status.set("sc64deployer stopped: is the cart connected? (close this and run feel.bat again)")
        root.after(100, poll)

    root.after(300, lambda: link.say("get"))
    poll()
    root.protocol("WM_DELETE_WINDOW", lambda: (link.close(), root.destroy()))
    root.mainloop()


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--deployer")
    ap.add_argument("--fake")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    items = read_list()
    if not items:
        sys.exit("No numbers found in %s" % HEADER)
    if args.fake:
        link = Link(fake=args.fake)
    else:
        import run_queue as rq
        deployer = rq.find_deployer(args.deployer)
        open(rq.DEPLOYER_PATH_FILE, "w", encoding="utf-8").write(deployer)
        os.makedirs(os.path.join(HERE, "work"), exist_ok=True)
        link = Link(deployer=deployer)
    try:
        if args.check:
            sys.exit(0 if check(link, items) else 1)
        window(link, items)
    finally:
        link.close()


if __name__ == "__main__":
    main()
