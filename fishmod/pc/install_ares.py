#!/usr/bin/env python3
"""Downloads the latest ares release for Windows (x64) from GitHub and unpacks it next to this script, in ares\\, where
run_jobs.py looks for it first.

Usage (in the fishoot conda env):  python pc\\install_ares.py [--dest DIR] [--force]
"""
import argparse
import io
import json
import os
import shutil
import urllib.request
import zipfile

API = "https://api.github.com/repos/ares-emulator/ares/releases/latest"
HERE = os.path.dirname(os.path.abspath(__file__))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dest", default=os.path.join(HERE, "ares"))
    ap.add_argument("--force", action="store_true", help="download again even if it's there")
    args = ap.parse_args()

    exe = os.path.join(args.dest, "ares.exe")
    if os.path.exists(exe) and not args.force:
        print(f"ares is already at {exe} (--force to get it again)")
        return
    req = urllib.request.Request(API, headers={"User-Agent": "fishoot-installer"})
    release = json.load(urllib.request.urlopen(req))
    zips = [a for a in release["assets"] if a["name"].lower().endswith(".zip")]
    # the Windows x64 build first; never debug symbols (a "pdb" or "symbols" zip has no ares.exe in it)
    def rank(a):
        n = a["name"].lower()
        if "windows" not in n and "win" not in n:
            return None
        if any(w in n for w in ("pdb", "symbol", "debug", "arm")):
            return None
        return 0 if ("x64" in n or "amd64" in n or "x86_64" in n) else 1
    cands = sorted((a for a in zips if rank(a) is not None), key=rank)
    if not cands:
        names = ", ".join(a["name"] for a in release["assets"])
        raise SystemExit(f"no Windows build among the latest ares release's files ({names}): get it from "
                         "https://ares-emu.net/download and unzip it into " + args.dest)
    tmp = args.dest + "_tmp"
    for asset in cands:
        print(f"downloading ares {release['tag_name']}: {asset['name']} ({asset['size'] // (1 << 20)} MB)")
        data = urllib.request.urlopen(urllib.request.Request(asset["browser_download_url"],
                                                             headers={"User-Agent": "fishoot-installer"})).read()
        shutil.rmtree(tmp, ignore_errors=True)
        zipfile.ZipFile(io.BytesIO(data)).extractall(tmp)
        # the zip may keep everything in a folder of its own: use the folder that has ares.exe
        found = None
        for root, _, files in os.walk(tmp):
            if any(f.lower() == "ares.exe" for f in files):
                found = root
                break
        if found is not None:
            break
        print(f"  no ares.exe in {asset['name']}, trying the next file")
    else:
        shutil.rmtree(tmp, ignore_errors=True)
        raise SystemExit("ares.exe wasn't in any of the downloads: get it from https://ares-emu.net/download and "
                         "unzip it into " + args.dest)
    shutil.rmtree(args.dest, ignore_errors=True)
    shutil.move(found, args.dest)
    shutil.rmtree(tmp, ignore_errors=True)
    print(f"ares is at {exe}")


if __name__ == "__main__":
    main()
