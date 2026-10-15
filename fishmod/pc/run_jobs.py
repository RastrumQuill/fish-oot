#!/usr/bin/env python3
"""Runs a job pack on this PC and zips the results to send back.

A job pack (a .zip made by fishmod/pc/make_pack.py) holds .bps patches and jobs.json: which patched ROMs to run in
ares, for how long, and how many at once. Each job gets its own folder, its own copy of the patched ROM and its own
ares; when its time is up its window is closed politely (so ares writes the save file), and the save (the game's SRAM,
where the test ROMs put their results) goes into the results.

Usage (in the fishoot conda env, see README.md):
  python fishmod/pc/run_jobs.py PACK.zip --rom "C:\\roms\\oot-ntsc-1.2.z64" [--ares "C:\\ares\\ares.exe"]
         [--parallel 6] [--out results] [--saves-dir DIR]
Results: results\\<pack>\\ (a folder per job) and results_<pack>.zip next to it: send the zip.
Running the same pack again carries on from the saves already there (the long jobs resume).
"""
import argparse
import glob
import json
import os
import shutil
import struct
import subprocess
import sys
import time
import zipfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bps  # noqa: E402

ARES_GUESSES = [os.path.join(os.path.dirname(os.path.abspath(__file__)), "ares", "ares.exe"),  # (install_ares.py)
                r"C:\ares\ares.exe", r"C:\Program Files\ares\ares.exe", os.path.expandvars(r"%LOCALAPPDATA%\ares\ares.exe")]


def find_ares(given):
    for p in ([given] if given else []) + [os.environ.get("ARES", "")] + ARES_GUESSES:
        if p and os.path.isfile(p):
            return p
    found = shutil.which("ares")
    if found:
        return found
    raise SystemExit("ares not found: pass --ares C:\\path\\to\\ares.exe")


def close_gracefully(proc, wait=30):
    """Closes ares's window (it writes the save on the way out); kills it only if it won't go."""
    if os.name != "nt":
        proc.terminate()  # (Linux: ares writes its save on SIGTERM)
        try:
            proc.wait(wait)
            return "closed"
        except subprocess.TimeoutExpired:
            proc.kill()
            return "killed (the save may be missing)"
    try:
        import win32con
        import win32gui
        import win32process

        def close(hwnd, _):
            if win32process.GetWindowThreadProcessId(hwnd)[1] == proc.pid:
                win32gui.PostMessage(hwnd, win32con.WM_CLOSE, 0, 0)
        win32gui.EnumWindows(close, None)
    except ImportError:
        subprocess.run(["taskkill", "/PID", str(proc.pid)], capture_output=True)
    try:
        proc.wait(wait)
        return "closed"
    except subprocess.TimeoutExpired:
        proc.kill()
        return "killed (the save may be missing)"


def health(ram):
    """The debug builds keep running totals at SRAM 0x7F00 ('FMST'): how many frames ran (the game was running)."""
    if len(ram) < 0x7F10 or ram[0x7F00:0x7F04] != b"FMST":
        return None
    return struct.unpack(">I", ram[0x7F04:0x7F08])[0] + 600  # (it starts counting after the first 600 frames)


def find_save(job_dir, name, saves_dir, since):
    cands = glob.glob(os.path.join(job_dir, name + ".*"))
    if saves_dir:
        cands += glob.glob(os.path.join(saves_dir, "**", name + "*"), recursive=True)
    cands = [c for c in cands if not c.endswith(".z64") and os.path.getmtime(c) >= since - 1
             and os.path.getsize(c) >= 0x8000]
    return max(cands, key=os.path.getmtime) if cands else None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("pack")
    ap.add_argument("--rom", required=True, help="your Ocarina of Time NTSC 1.2 (US) ROM (any byte order)")
    ap.add_argument("--ares")
    ap.add_argument("--parallel", type=int, default=6)
    ap.add_argument("--out", default="results")
    ap.add_argument("--saves-dir", help="ares's saves folder, if it doesn't put saves next to the ROM")
    args = ap.parse_args()

    ares = find_ares(args.ares)
    base = bps.to_z64(open(args.rom, "rb").read())
    pack = zipfile.ZipFile(args.pack)
    spec = json.loads(pack.read("jobs.json"))
    name = spec["pack"]
    out = os.path.join(args.out, name)
    os.makedirs(out, exist_ok=True)
    print(f"pack {name}: {len(spec['jobs'])} jobs, {args.parallel} at a time, ares {ares}")

    queue = list(spec["jobs"])
    running = []
    summary = {}
    while queue or running:
        while queue and len(running) < args.parallel:
            job = queue.pop(0)
            jd = os.path.abspath(os.path.join(out, job["name"]))  # (ares runs in it: paths must be absolute)
            os.makedirs(jd, exist_ok=True)
            rom = os.path.join(jd, job["name"] + ".z64")
            open(rom, "wb").write(bps.apply(base, pack.read(job["patch"])))
            # carry on from an earlier run's save, for the long resumable jobs
            prev = os.path.join(jd, "result.sav")
            if job.get("resume") and os.path.exists(prev):
                shutil.copy(prev, os.path.join(jd, job["name"] + ".ram"))
            cmd = [ares, "--system", "Nintendo 64", "--no-file-prompt"]
            if os.name != "nt":
                cmd += ["--setting", "Audio/Driver=None"]  # (a headless Linux box has no sound device)
            cmd += ["--setting", "Input/Defocus=Allow", rom]  # (keeps running when its window isn't in front)
            proc = subprocess.Popen(cmd, cwd=jd)
            running.append((job, jd, proc, time.time()))
            print(f"  started {job['name']} ({job['seconds']} s)")
        time.sleep(2)
        for item in list(running):
            job, jd, proc, t0 = item
            if proc.poll() is None and time.time() - t0 < job["seconds"]:
                continue
            how = "exited by itself" if proc.poll() is not None else close_gracefully(proc)
            running.remove(item)
            save = find_save(jd, job["name"], args.saves_dir, t0)
            res = {"seconds": round(time.time() - t0), "ares": how, "save": None, "frames": None}
            if save:
                ram = open(save, "rb").read()
                shutil.copy(save, os.path.join(jd, "result.sav"))
                res["save"] = os.path.basename(save)
                res["frames"] = health(ram)
            summary[job["name"]] = res
            fr = res["frames"]
            print(f"  done {job['name']}: {how}; save {'found' if save else 'NOT FOUND'}"
                  + (f"; {fr} game frames ran ({fr / max(res['seconds'], 1):.1f}/s)" if fr else
                     ("; no frame count: either the game didn't run, or it ran under 600 frames (too slow or too short a job)"
                      if save else "")))
    json.dump(summary, open(os.path.join(out, "summary.json"), "w"), indent=1)
    zpath = os.path.join(args.out, f"results_{name}.zip")
    with zipfile.ZipFile(zpath, "w", zipfile.ZIP_DEFLATED) as z:
        z.write(os.path.join(out, "summary.json"), "summary.json")
        for job in spec["jobs"]:
            r = os.path.join(out, job["name"], "result.sav")
            if os.path.exists(r):
                z.write(r, f"{job['name']}.sav")
    print(f"\nsend this file back: {os.path.abspath(zpath)}")


if __name__ == "__main__":
    main()
