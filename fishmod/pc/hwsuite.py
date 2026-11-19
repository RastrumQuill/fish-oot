#!/usr/bin/env python3
"""The console test suite's report (fishmod/docs/hw-suite.md): reads a morning's folder of runs and says, for each
build, whether it ran right on the console and how fast.

  hwsuite.py MORNING_FOLDER [-o hwsuite.txt]

MORNING_FOLDER is fishmod/pc/results/morning/runs/<date-time> (a folder per run: debug.log, packets-*.zip). The
morning runner calls this itself after a queue of the suite's patches; it can be run again on any morning's folder.

For each run:
  - did it start and finish (FISHMOD-START, FISHMOD-DONE), and did the game report a crash;
  - its FISHMOD-CHECK line (frames played, the props' sets and their running hash, a replay's counts) against the
    values the same ROM gave in the emulator (releases/hwsuite/expected.json): the same means the build does on the
    console exactly what it does in ares; two runs of one ROM must agree with each other in any case;
  - from the frame log: frames a second, the frame's period, the CPU's update and draw, the RSP and the RDP, slow
    frames, over the frames the bench counts.
Then each build against the base build (the mean of its runs): the difference, and the spread between a build's own
runs, which is the noise the difference has to beat.

Needs nothing beyond Python's standard library.
"""
import argparse
import glob
import json
import os
import re
import struct
import sys
import tempfile
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
EXPECTED = os.path.join(ROOT, "releases", "hwsuite", "expected.json")
SLOW_US = 52000
# The bench's frames over which every build of the suite plays the same game: in ares the four builds' cameras and
# sets of props are identical to frame 871, then one camera decision goes another way in the bare-metal builds and
# Link (steered by the camera) ends up elsewhere, so later frames aren't the same work (fishmod/docs/hw-suite.md).
SAME_FROM, SAME_TO = 60, 860


def check_line(text):
    m = re.search(r"FISHMOD-CHECK frames (\d+) sets (\d+) sethash ([0-9A-Fa-f]+) replay (\d+)/(\d+) early (\d+) "
                  r"late (\d+) forced (\d+) snaps (\d+) rng (\d+)", text)
    if not m:
        return None
    k = ["frames", "sets", "sethash", "replay_frames", "replay_checks", "early", "late", "forced", "snaps", "rng"]
    d = dict(zip(k, m.groups()))
    return {a: (b.upper() if a == "sethash" else int(b)) for a, b in d.items()}


def frame_records(folder):
    """(counted, period us, cpu update us, cpu draw us, rsp gfx us, rsp audio us, rdp us) per frame, from the
    run's packed packets."""
    out = []
    for pack in sorted(glob.glob(os.path.join(folder, "packets-*.zip"))):
        with zipfile.ZipFile(pack) as z:
            for name in z.namelist():
                d = z.read(name)
                if d[:4] != b"FFRM" or len(d) < 16:
                    continue
                _, version, count, _ = struct.unpack(">4I", d[:16])
                words = 56 if (version & 0xFF) >= 2 else 52
                if version & 0x10000:
                    words = 58
                if version & 0x20000:
                    words = 60
                for i in range(count):
                    if 16 + 4 * words * (i + 1) > len(d):
                        break
                    r = struct.unpack_from(">8I", d, 16 + 4 * words * i)
                    out.append((r[1] & 1, r[2], r[3], r[4], r[5], r[6], r[7], r[0]))
    return out


def mean(xs):
    return sum(xs) / len(xs) if xs else 0.0


def read_run(folder):
    log_path = os.path.join(folder, "debug.log")
    text = open(log_path, encoding="utf-8", errors="replace").read() if os.path.isfile(log_path) else ""
    run = {"folder": os.path.basename(folder), "started": "FISHMOD-START" in text, "done": "FISHMOD-DONE" in text,
           "crash": [l for l in text.splitlines() if "FISHMOD-CODEGUARD" in l][:3],
           "usb_stalls": text.count("FISHMOD-USB-STALL"),
           "check": check_line(text)}
    recs = frame_records(folder)
    counted = [r for r in recs if r[0]] or recs
    run["frames_logged"] = len(recs)
    # the stretch every build plays alike (SAME_FROM..SAME_TO: see the report's note)
    same = [r for r in recs if SAME_FROM <= r[7] < SAME_TO]
    if same:
        run["same_cpu"] = mean([r[2] + r[3] for r in same]) / 1000.0
        run["same_n"] = len(same)
    if counted:
        run["period"] = mean([r[1] for r in counted]) / 1000.0
        run["fps"] = 1000.0 / run["period"] if run["period"] else 0.0
        run["update"] = mean([r[2] for r in counted]) / 1000.0
        run["draw"] = mean([r[3] for r in counted]) / 1000.0
        run["rsp"] = mean([r[4] + r[5] for r in counted]) / 1000.0
        run["rdp"] = mean([r[6] for r in counted]) / 1000.0
        run["slow"] = 100.0 * sum(1 for r in counted if r[1] > SLOW_US) / len(counted)
    return run


def label_of(folder_name):
    """A run's folder is <number>-<label> or <label>; its build is the label without a trailing -1, -2 (the A B A B
    repeats)."""
    name = re.sub(r"^\d+[-_]", "", folder_name)
    return name, re.sub(r"-\d+$", "", name)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("folder")
    ap.add_argument("-o", "--output")
    ap.add_argument("--base", default="hw-base", help="the build the others are compared with")
    a = ap.parse_args()
    expected = json.load(open(EXPECTED)) if os.path.isfile(EXPECTED) else {}
    runs = []
    never = []
    for d in sorted(glob.glob(os.path.join(a.folder, "*"))):
        if os.path.isdir(d) and os.path.isfile(os.path.join(d, "debug.log")):
            r = read_run(d)
            r["name"], r["build"] = label_of(r["folder"])
            runs.append(r)
        elif os.path.isdir(d) and os.path.isfile(os.path.join(d, "upload.log")):
            never.append(label_of(os.path.basename(d))[0])  # (uploaded, or tried to, and never ran)
    if not runs:
        sys.exit(f"{a.folder}: no runs (folders with a debug.log)")
    out = []
    w = out.append
    fails = 0
    w("Console test suite: " + os.path.basename(os.path.normpath(a.folder)))
    w("")
    w("1. Did each run work")
    w("")
    builds = {}
    for r in runs:
        builds.setdefault(r["build"], []).append(r)
        notes = []
        ok = True
        if not r["started"]:
            notes.append("never started (no FISHMOD-START)")
            ok = False
        elif not r["done"]:
            notes.append("did not finish (no FISHMOD-DONE: hung, crashed, or ran out of time)")
            ok = False
        if r["crash"]:
            notes.append("the game reported: " + " | ".join(r["crash"]))
            ok = False
        if r.get("usb_stalls"):
            notes.append("%d USB stalls (the link, not the game)" % r["usb_stalls"])
        c = r["check"]
        e = expected.get(r["build"])
        if r["done"] and c is None:
            notes.append("no FISHMOD-CHECK line")
            ok = False
        if c is not None:
            if c["replay_frames"]:
                bad = c["early"] + c["late"] + c["forced"] + c["snaps"] + c["rng"]
                notes.append("replay %d frames, %d checks: %s" % (
                    c["replay_frames"], c["replay_checks"],
                    "in step, no repairs" if bad == 0 else
                    "REPAIRS: early %d late %d forced %d snaps %d rng %d" % (c["early"], c["late"], c["forced"],
                                                                              c["snaps"], c["rng"])))
                if e and c["replay_frames"] != e.get("replay_frames", c["replay_frames"]):
                    notes.append("played %d of the recording's %d frames" % (c["replay_frames"], e["replay_frames"]))
                    ok = False
                # (repairs on the console aren't a failure by themselves: its timing isn't the emulator's. They say
                # how far a build drifts; forced scenes mean it lost the recording's path)
                if c["forced"]:
                    ok = False
            else:
                notes.append("frames %d, sets %d, hash %s" % (c["frames"], c["sets"], c["sethash"]))
                if e:
                    # (the sets and their hash are what the game did; the frame count is where the end was noticed,
                    # which can fall a few frames apart on the console)
                    same = all(c[k] == e[k] for k in ("sets", "sethash") if k in e)
                    notes.append("the same sets as in the emulator" if same else
                                 "NOT the emulator's sets (%s, hash %s)" % (e.get("sets"), e.get("sethash")))
                    if "frames" in e and c["frames"] != e["frames"]:
                        notes.append("%+d frames at the end" % (c["frames"] - e["frames"]))
        fails += 0 if ok else 1
        w("  %-4s %-22s %s" % ("ok" if ok else "FAIL", r["name"], "; ".join(notes)))
    for name in never:
        fails += 1
        w("  %-4s %-22s never ran (no debug.log: see its upload.log)" % ("FAIL", name))
    w("")
    w("2. Do a build's own runs agree (the same ROM twice must do the same thing)")
    w("")
    for b, rs in builds.items():
        cs = [r["check"] for r in rs if r["check"]]
        if len(cs) >= 2:
            same = all((c["frames"], c["sets"], c["sethash"]) == (cs[0]["frames"], cs[0]["sets"], cs[0]["sethash"])
                       for c in cs)
            w("  %-4s %-22s %d runs %s" % ("ok" if same else "DIFF", b, len(cs),
                                           "agree" if same else "DO NOT agree: the run isn't repeatable on the console"))
    w("")
    w("3. How fast (the frames the bench counts; ms a frame)")
    w("")
    w("  %-22s %6s %8s %8s %8s %8s %8s %7s" % ("run", "fps", "period", "update", "draw", "RSP", "RDP", "slow %"))
    for r in runs:
        if "period" in r:
            w("  %-22s %6.2f %8.2f %8.2f %8.2f %8.2f %8.2f %7.1f" % (r["name"], r["fps"], r["period"], r["update"],
                                                                     r["draw"], r["rsp"], r["rdp"], r["slow"]))
        else:
            w("  %-22s (no frame log)" % r["name"])
    w("")
    w("4. Each build against %s (CPU = update + draw; the spread is between a build's own runs: a difference" % a.base)
    w("   smaller than the spreads is noise)")
    w("")
    stats = {}
    for b, rs in builds.items():
        ps = [r for r in rs if "period" in r]
        if ps:
            cpu = [r["update"] + r["draw"] for r in ps]
            per = [r["period"] for r in ps]
            stats[b] = (mean(cpu), max(cpu) - min(cpu), mean(per), max(per) - min(per), len(ps))
    if a.base in stats:
        b0 = stats[a.base]
        w("  %-22s %5s %9s %8s %9s %10s %8s" % ("build", "runs", "CPU ms", "spread", "vs base", "period ms", "vs base"))
        for b, s in stats.items():
            w("  %-22s %5d %9.2f %8.2f %+8.1f%% %10.2f %+7.1f%%" % (b, s[4], s[0], s[1], 100 * (s[0] / b0[0] - 1) if b0[0] else 0,
                                                                   s[2], 100 * (s[2] / b0[2] - 1) if b0[2] else 0))
    else:
        w("  (no run of %s with a frame log: nothing to compare with)" % a.base)
    w("")
    w("5. The same over frames %d to %d only, where every build plays the same game (the fair comparison: after" %
      (SAME_FROM, SAME_TO))
    w("   that the builds' cameras part and they aren't doing the same work)")
    w("")
    same = {}
    for b, rs in builds.items():
        xs = [r["same_cpu"] for r in rs if "same_cpu" in r]
        if xs:
            same[b] = (mean(xs), max(xs) - min(xs), len(xs))
    if a.base in same:
        b0 = same[a.base][0]
        w("  %-22s %5s %9s %8s %9s" % ("build", "runs", "CPU ms", "spread", "vs base"))
        for b, x in same.items():
            w("  %-22s %5d %9.2f %8.2f %+8.1f%%" % (b, x[2], x[0], x[1], 100 * (x[0] / b0 - 1) if b0 else 0))
    w("")
    w("RESULT: %s" % ("all runs worked" if fails == 0 else "%d run(s) FAILED" % fails))
    text = "\n".join(out) + "\n"
    if a.output:
        open(a.output, "w", encoding="utf-8").write(text)
    sys.stdout.write(text)
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
