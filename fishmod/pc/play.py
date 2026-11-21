#!/usr/bin/env python3
"""Loads a copy of fish-oot onto the console to play: pick one, it's patched from your ROM and sent to the
SummerCart64, then you switch the console off and on.

  1. The game, to play (releases/fish-oot-play.bps)
  2. Test mode at Lake Hylia: a new file starts there as adult Link, the lake full (releases/fish-oot-test-lake.bps)
  3. Test mode in Kokiri Forest: a new file starts there as child Link (releases/fish-oot-test-kokiri.bps)

Test mode: new files skip the intro and start with the rod (on C-Down), bugs (C-Right), lures, bottles, the slingshot and
the Goron's Bracelet; loading a test file goes back to its place. Hold L and press the D-pad:
  Up / Down      warp to Kokiri Forest (child) / Lake Hylia (adult)
  Right          with the rod out of the water: pick which fish to call (each species in turn, or the nearest);
                 with the bobber in the water: that fish comes straight to the bait (its own nibbles, bite and fight)
  Left           the frame-time readout's detail
With a test build running, feel.bat opens the fishing feel tuner (sliders that change the fight's numbers live).

Usage (play.bat does this): python fishmod/pc/play.py [1|2|3|PATCH.bps] [--rom ROM] [--deployer sc64deployer.exe]
  [--no-pull]
The ROM's and the deployer's paths are the ones the queue runner remembers (rom_path.txt, sc64deployer_path.txt).
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import bps  # noqa: E402
import run_queue as rq  # noqa: E402

CHOICES = [
    ("The game, to play", "releases/fish-oot-play.bps"),
    ("Test mode at Lake Hylia (adult, the lake full)", "releases/fish-oot-test-lake.bps"),
    ("Test mode in Kokiri Forest (child)", "releases/fish-oot-test-kokiri.bps"),
]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("which", nargs="?", help="1, 2, 3 or a patch")
    ap.add_argument("--rom")
    ap.add_argument("--deployer")
    ap.add_argument("--no-pull", action="store_true")
    args = ap.parse_args()

    if not args.no_pull and rq.GIT is not None:
        try:
            rq.log("git pull...")
            rq.git("pull", "-q", "--ff-only")
        except Exception as e:  # (an old copy still works)
            rq.log("git pull failed, carrying on with what's here: %s" % e)

    which = args.which
    if not which:
        print()
        for i, (name, path) in enumerate(CHOICES, 1):
            print("  %d. %s" % (i, name))
        print()
        which = input("Which one? (1-%d) " % len(CHOICES)).strip()
    if which.isdigit() and 1 <= int(which) <= len(CHOICES):
        name, patch = CHOICES[int(which) - 1]
        patch = os.path.join(rq.ROOT, patch)
    else:
        name, patch = os.path.basename(which), which
    if not os.path.isfile(patch):
        sys.exit("No patch at %s" % patch)

    rom = rq.find_rom(args.rom)
    deployer = rq.find_deployer(args.deployer)
    open(rq.ROM_PATH_FILE, "w", encoding="utf-8").write(rom)
    open(rq.DEPLOYER_PATH_FILE, "w", encoding="utf-8").write(deployer)

    rq.log("%s: patching %s" % (name, os.path.basename(rom)))
    target = bps.apply(bps.to_z64(open(rom, "rb").read()), open(patch, "rb").read())
    os.makedirs(rq.WORK, exist_ok=True)
    out = os.path.join(rq.WORK, "play.z64")
    open(out, "wb").write(target)
    del target

    log_path = os.path.join(rq.WORK, "play_upload.log")
    for attempt in range(3):
        rq.log("sending it to the SummerCart64...")
        code, text = rq.run_capture([deployer, "upload", out], log_path, 300)
        if code == 0:
            # (the deployer doesn't check what it wrote: read it back and compare)
            bad = rq.verify_upload(deployer, out, log_path)
            if bad == "ok":
                break
            rq.log("the copy in the cart differs from what was sent (%s): sending it again" % bad)
            continue
        rq.log("the upload failed (exit %d): is the console switched on and the cart connected?" % code)
        if attempt < 2:
            input("Press Enter to try again. ")
    else:
        os.remove(out)
        sys.exit("Couldn't load it (see %s)." % log_path)
    os.remove(out)
    rq.beep()
    print()
    rq.log("Loaded: %s. Switch the console OFF, then ON, to start it." % name)


if __name__ == "__main__":
    main()
