#!/usr/bin/env python3
"""Makes a job pack for fishmod/pc/run_jobs.py (run on the user's PC): patches of built ROMs against the original,
and jobs.json saying how long each runs.

Usage: fishmod/pc/make_pack.py NAME OUT.zip JOB [JOB ...] [--parts N] [--uncapped]
  JOB = name=path/to/built.z64:seconds[:resume]
  --parts N: each JOB becomes N jobs on the one patch, each told its part in its starting save (SRAM 0x7C00: 'FMPT',
             part, parts; the LOD sweep and tuning read it), all resumable
  --uncapped: ares runs as fast as it can (no sound, nothing holding it to real time)
e.g. fishmod/pc/make_pack.py selftest out/selftest.zip tour=fishmod/out/fish-oot-debug-tour-opt5.z64:120
Needs flips (as the build does) and fishmod/out/original.z64 (made by any release build).
"""
import argparse
import json
import os
import struct
import subprocess
import tempfile
import zipfile

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))


def main():
    ap = argparse.ArgumentParser(usage=__doc__)
    ap.add_argument("name")
    ap.add_argument("out")
    ap.add_argument("jobs", nargs="+")
    ap.add_argument("--parts", type=int, default=0)
    ap.add_argument("--uncapped", action="store_true")
    args = ap.parse_args()
    original = os.path.join(ROOT, "fishmod", "out", "original.z64")
    spec = {"pack": args.name, "jobs": []}
    if args.uncapped:
        spec["uncapped"] = True
    with tempfile.TemporaryDirectory() as tmp, zipfile.ZipFile(args.out, "w", zipfile.ZIP_DEFLATED) as z:
        for j in args.jobs:
            jname, rest = j.split("=", 1)
            parts = rest.split(":")
            rom, seconds = parts[0], int(parts[1])
            patch = os.path.join(tmp, jname + ".bps")
            subprocess.run([os.environ.get("FLIPS", "flips"), "--create", "--bps", original, rom, patch], check=True,
                           capture_output=True)
            z.write(patch, jname + ".bps")
            resume = len(parts) > 2 and parts[2] == "resume"
            if args.parts > 0:
                for p in range(args.parts):
                    sram = struct.pack(">III", 0x464D5054, p, args.parts).hex()
                    spec["jobs"].append({"name": f"{jname}{p:02d}", "patch": jname + ".bps", "seconds": seconds,
                                         "resume": True, "sram": {"0x7C00": sram}})
            else:
                spec["jobs"].append({"name": jname, "patch": jname + ".bps", "seconds": seconds, "resume": resume})
        z.writestr("jobs.json", json.dumps(spec, indent=1))
    print(f"{args.out}: {len(spec['jobs'])} jobs")


if __name__ == "__main__":
    main()
