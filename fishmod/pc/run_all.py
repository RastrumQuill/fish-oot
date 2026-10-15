#!/usr/bin/env python3
"""Runs every job pack in fishmod\\pc\\packs\\ (the .zip files I send; they're made from your ROM, so they never go in
the repo), the self-test first: if the game doesn't run in it, it stops there and says why. Then one zip of all the
results to send back.

Usage (in the repo, in the fishoot env):  python fishmod\\pc\\run_all.py --rom C:\\roms\\oot.z64 [--parallel 6]
Options are passed on to run_jobs.py (--ares, --parallel, --saves-dir).
"""
import argparse
import glob
import json
import os
import subprocess
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--rom", required=True)
    ap.add_argument("--packs", default=os.path.join(HERE, "packs"))
    ap.add_argument("--out", default=os.path.join(HERE, "results"))
    args, passthrough = ap.parse_known_args()

    packs = sorted(glob.glob(os.path.join(args.packs, "*.zip")))
    packs.sort(key=lambda p: 0 if "selftest" in os.path.basename(p) else 1)
    if not packs:
        raise SystemExit(f"no packs in {args.packs}: put the .zip packs I send there")
    print("packs:", ", ".join(os.path.basename(p) for p in packs))
    done = []
    for pack in packs:
        name = json.loads(zipfile.ZipFile(pack).read("jobs.json"))["pack"]
        print(f"\n=== {name} ===")
        r = subprocess.run([sys.executable, os.path.join(HERE, "run_jobs.py"), pack, "--rom", args.rom,
                            "--out", args.out] + passthrough)
        if r.returncode != 0:
            print(f"{name} failed (see above); carrying on with the rest")
            continue
        done.append(name)
        if name == "selftest":
            summary = json.load(open(os.path.join(args.out, name, "summary.json")))
            if not any(v.get("frames") for v in summary.values()):
                raise SystemExit("\nThe self-test's game didn't run (no frames counted), so the other packs wouldn't "
                                 "either. Check ares: the Expansion Pak setting, and the video driver (Settings → "
                                 "Video). See fishmod\\pc\\README.md.")
    allzip = os.path.join(args.out, "results_all.zip")
    with zipfile.ZipFile(allzip, "w", zipfile.ZIP_DEFLATED) as z:
        for name in done:
            r = os.path.join(args.out, f"results_{name}.zip")
            if os.path.exists(r):
                z.write(r, os.path.basename(r))
    print(f"\nall done. Send this one file back: {allzip}")


if __name__ == "__main__":
    main()
