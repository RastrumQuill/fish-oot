#!/usr/bin/env python3
"""Makes a job pack for fishmod/pc/run_jobs.py (run on the user's PC): patches of built ROMs against the original,
and jobs.json saying how long each runs.

Usage: fishmod/pc/make_pack.py NAME OUT.zip JOB [JOB ...]
  JOB = name=path/to/built.z64:seconds[:resume]
e.g. fishmod/pc/make_pack.py selftest out/selftest.zip tour=fishmod/out/fish-oot-debug-tour-opt5.z64:120
Needs flips (as the build does) and fishmod/out/original.z64 (made by any release build).
"""
import json
import os
import subprocess
import sys
import tempfile
import zipfile

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))


def main():
    if len(sys.argv) < 4:
        raise SystemExit(__doc__)
    name, out, jobs = sys.argv[1], sys.argv[2], sys.argv[3:]
    original = os.path.join(ROOT, "fishmod", "out", "original.z64")
    spec = {"pack": name, "jobs": []}
    with tempfile.TemporaryDirectory() as tmp, zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
        for j in jobs:
            jname, rest = j.split("=", 1)
            parts = rest.split(":")
            rom, seconds = parts[0], int(parts[1])
            patch = os.path.join(tmp, jname + ".bps")
            subprocess.run([os.environ.get("FLIPS", "flips"), "--create", "--bps", original, rom, patch], check=True,
                           capture_output=True)
            z.write(patch, jname + ".bps")
            spec["jobs"].append({"name": jname, "patch": jname + ".bps", "seconds": seconds,
                                 "resume": len(parts) > 2 and parts[2] == "resume"})
        z.writestr("jobs.json", json.dumps(spec, indent=1))
    print(f"{out}: {len(jobs)} jobs")


if __name__ == "__main__":
    main()
