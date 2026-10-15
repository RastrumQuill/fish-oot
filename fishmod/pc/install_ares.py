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
    assets = [a for a in release["assets"] if a["name"].lower().endswith(".zip") and "windows" in a["name"].lower()]
    # the 64-bit x86 build, not ARM
    assets = [a for a in assets if "arm" not in a["name"].lower()] or assets
    if not assets:
        raise SystemExit("no Windows build in the latest ares release: get it from https://ares-emu.net/download")
    asset = assets[0]
    print(f"downloading ares {release['tag_name']}: {asset['name']} ({asset['size'] // (1 << 20)} MB)")
    data = urllib.request.urlopen(urllib.request.Request(asset["browser_download_url"],
                                                         headers={"User-Agent": "fishoot-installer"})).read()
    z = zipfile.ZipFile(io.BytesIO(data))
    tmp = args.dest + "_tmp"
    shutil.rmtree(tmp, ignore_errors=True)
    z.extractall(tmp)
    # the zip may keep everything in a folder of its own: use the folder that has ares.exe
    found = None
    for root, _, files in os.walk(tmp):
        if "ares.exe" in files:
            found = root
            break
    if found is None:
        raise SystemExit("ares.exe wasn't in the download: get it from https://ares-emu.net/download")
    shutil.rmtree(args.dest, ignore_errors=True)
    shutil.move(found, args.dest)
    shutil.rmtree(tmp, ignore_errors=True)
    print(f"ares is at {exe}")


if __name__ == "__main__":
    main()
