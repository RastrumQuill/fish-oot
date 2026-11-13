#!/usr/bin/env python3
"""The lab: the console's runs ordered through git, with nobody there.

It runs until stopped (Ctrl+C, or close the window). Every few minutes it fetches the lab branch and reads its queue
(fishmod/pc/lab_queue.txt there): each line not run yet is run as the morning runner runs a line (the patch applied to
your ROM, uploaded, checked byte for byte, the frame log recorded), with the console switched off and on by a smart
plug to start it, and each run's results are pushed to the morning-results branch as soon as it ends (lab/<id> there).
Nobody needs to be at the console: a run that hangs is stopped after a few quiet minutes, and the next one starts from
a power-on anyway.

What it takes from the repository is data only: the queue and the patches it names, read out of the fetched branch
(`git show`), never checked out. Its own code is what you started; a new version of it runs only when you start it
again (after a pull of your own). A queue line may only name a .bps patch under releases/.

The queue's lines: ID PATCH [MINUTES] [LABEL], e.g.
  frames14-a releases/fish-oot-frames14.bps 12 frames14
An ID is run once (they're remembered in fishmod/pc/results/lab/done.txt); a new run of the same patch needs a new ID.

The plug: a Shelly (Gen1, or Gen2 and later: Plus, Plug S, Gen3), or any plug running Tasmota, on your network,
controlled over its local web page. The console's own power switch stays ON; the plug switches the power brick.
Settings in fishmod/pc/lab.json (made the first time, from the questions it asks):
  {"plug": "shelly2", "host": "192.168.1.50", "branch": "claude/harkinian-tp-fishing-mod-ehepm9", "poll_minutes": 2}
  plug: shelly1, shelly2 (Gen2 and later) or tasmota

Usage (in the fishoot conda env; lab.bat does this):
  python fishmod/pc/lab.py               the lab, until stopped
  python fishmod/pc/lab.py --test-plug   switch the console off and on once (checks the plug), then quit
  python fishmod/pc/lab.py --once        one look at the queue: run what's new, then quit
"""
import argparse
import datetime
import json
import os
import subprocess
import sys
import time
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import bps  # noqa: E402
import morning  # noqa: E402

CONFIG = os.path.join(HERE, "lab.json")
LAB_RESULTS = os.path.join(morning.RESULTS, "lab")
DONE = os.path.join(HERE, "results", "lab", "done.txt")
QUEUE_PATH = "fishmod/pc/lab_queue.txt"
OFF_SECONDS = 5  # the console left off this long (its power supply drained) before it's switched back on
log = morning.log


class Plug:
    """A smart plug on the local network, switched through its own web page."""

    def __init__(self, kind, host):
        if kind not in ("shelly1", "shelly2", "tasmota"):
            raise SystemExit("lab.json: plug must be shelly1, shelly2 or tasmota, not %r" % kind)
        self.kind = kind
        self.host = host

    def url(self, on):
        if self.kind == "shelly1":
            return "http://%s/relay/0?turn=%s" % (self.host, "on" if on else "off")
        if self.kind == "shelly2":
            return "http://%s/rpc/Switch.Set?id=0&on=%s" % (self.host, "true" if on else "false")
        return "http://%s/cm?cmnd=Power%%20%s" % (self.host, "On" if on else "Off")

    def set(self, on):
        last = None
        for attempt in range(5):
            try:
                with urllib.request.urlopen(self.url(on), timeout=10) as r:
                    r.read()
                return
            except Exception as e:  # (Wi-Fi blips: try again)
                last = e
                time.sleep(3)
        raise RuntimeError("the plug at %s didn't answer: %s" % (self.host, last))

    def cycle(self):
        self.set(False)
        time.sleep(OFF_SECONDS)
        self.set(True)


def load_config():
    if os.path.isfile(CONFIG):
        return json.load(open(CONFIG, encoding="utf-8"))
    print("The lab's settings (saved in %s):" % CONFIG)
    cfg = {
        "plug": input("  The plug: shelly1 (Shelly Gen1), shelly2 (Shelly Gen2 or later) or tasmota? ").strip(),
        "host": input("  Its address on your network (e.g. 192.168.1.50)? ").strip(),
        "branch": input("  The branch to take the queue from [claude/harkinian-tp-fishing-mod-ehepm9]? ").strip()
        or "claude/harkinian-tp-fishing-mod-ehepm9",
        "poll_minutes": 2,
    }
    json.dump(cfg, open(CONFIG, "w", encoding="utf-8"), indent=1)
    return cfg


def fetch(branch):
    """The branch's newest commit (fetched, nothing checked out), or None."""
    try:
        r = subprocess.run([morning.GIT, "-C", morning.ROOT, "fetch", "-q", "origin", branch], capture_output=True,
                           timeout=morning.GIT_TIMEOUT, stdin=subprocess.DEVNULL)
    except subprocess.TimeoutExpired:
        return None
    if r.returncode != 0:
        return None
    return morning.git("rev-parse", "FETCH_HEAD")


def show(commit, path):
    """A file's bytes in that commit, or None."""
    r = subprocess.run([morning.GIT, "-C", morning.ROOT, "show", "%s:%s" % (commit, path)], capture_output=True,
                       timeout=morning.GIT_TIMEOUT, stdin=subprocess.DEVNULL)
    return r.stdout if r.returncode == 0 else None


def read_queue(commit):
    """[(id, patch path in the repository, minutes, label)] from the fetched queue (bad lines skipped, with a word)."""
    text = show(commit, QUEUE_PATH)
    if text is None:
        return []
    runs = []
    for line in text.decode("utf-8", "replace").splitlines():
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        parts = line.split()
        if len(parts) < 2:
            continue
        rid, patch = parts[0], parts[1].replace("\\", "/")
        if not (patch.startswith("releases/") and patch.endswith(".bps") and ".." not in patch):
            log("queue: %s skipped (only .bps patches under releases/)" % rid)
            continue
        try:
            minutes = float(parts[2]) if len(parts) > 2 else morning.DEFAULT_MINUTES
        except ValueError:
            continue
        label = parts[3] if len(parts) > 3 else os.path.splitext(os.path.basename(patch))[0]
        if not all(c.isalnum() or c in "-_." for c in rid + label):
            log("queue: %s skipped (ids and labels: letters, digits, - _ .)" % rid)
            continue
        runs.append((rid, patch, min(minutes, 60.0), label))
    return runs


def done_ids():
    if not os.path.isfile(DONE):
        return set()
    return set(l.strip() for l in open(DONE, encoding="utf-8") if l.strip())


def mark_done(rid):
    os.makedirs(os.path.dirname(DONE), exist_ok=True)
    with open(DONE, "a", encoding="utf-8") as f:
        f.write(rid + "\n")


def run_one(rid, patch_rel, minutes, label, commit, rom, deployer, plug, push):
    """One queued run, its results pushed. False if the console seems to need someone."""
    data = show(commit, patch_rel)
    if data is None:
        log("%s: %s isn't in the branch: skipped" % (rid, patch_rel))
        mark_done(rid)
        return True
    os.makedirs(morning.WORK, exist_ok=True)
    patch = os.path.join(morning.WORK, "lab.bps")
    open(patch, "wb").write(data)
    stamp = "%s-%s" % (datetime.datetime.now().strftime("%Y%m%d-%H%M"), rid)
    stamp_dir = os.path.join(LAB_RESULTS, stamp)
    os.makedirs(stamp_dir, exist_ok=True)
    log("Run %s: %s (%s, at most %g min)" % (rid, label, patch_rel, minutes))
    info, ok = morning.one_run(1, patch, minutes, label, rom, deployer, stamp_dir, False, power=plug)
    info["id"] = rid
    info["patch"] = patch_rel
    info["commit"] = commit
    json.dump([info], open(os.path.join(stamp_dir, "runs.json"), "w"), indent=1)
    open(os.path.join(stamp_dir, "summary.txt"), "w", encoding="utf-8").write(
        "Lab run %s: %s %s %s packets %.1f MB %s s %s\n" % (rid, label, info["status"], info.get("packets", 0),
                                                             info.get("bytes", 0) / 1e6, info.get("seconds", ""),
                                                             info.get("error", "")))
    log("  %s: %s%s" % (label, info["status"], (" (%s)" % info["error"]) if info.get("error") else ""))
    os.remove(patch)
    mark_done(rid)
    try:
        result = morning.publish(stamp, stamp_dir, push)
    except Exception as e:
        result = "not pushed: %s" % e
    log("  results: %s" % result)
    if info["status"] not in ("done", "incomplete") and plug is not None:
        # (it never started, or stopped: off and on, so the next upload finds a console that isn't stuck)
        try:
            plug.cycle()
        except Exception as e:
            log("  %s" % e)
    return ok or plug is not None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rom")
    ap.add_argument("--deployer")
    ap.add_argument("--test-plug", action="store_true", help="switch the console off and on once, then quit")
    ap.add_argument("--once", action="store_true", help="run what's queued now, then quit")
    ap.add_argument("--no-push", action="store_true")
    ap.add_argument("--soft-reboot", action="store_true",
                    help="no plug: start runs with the software reboot (unreliable on 12 Nov 2026: for testing it)")
    args = ap.parse_args()

    if args.soft_reboot and not os.path.isfile(CONFIG):
        cfg = {"branch": "claude/harkinian-tp-fishing-mod-ehepm9", "poll_minutes": 2}
    else:
        cfg = load_config()
    plug = None if args.soft_reboot else Plug(cfg["plug"], cfg["host"])
    if args.test_plug:
        log("the plug: off for %d s, then on" % OFF_SECONDS)
        plug.cycle()
        log("done: the console should be starting")
        return
    morning.SOFT_REBOOT = args.soft_reboot
    if morning.GIT is None:
        raise SystemExit("git not found: the lab needs it (the queue comes from the repository)")
    if os.name == "nt":  # (no sleeping while it runs: the USB link would drop)
        try:
            import ctypes
            ctypes.windll.kernel32.SetThreadExecutionState(0x80000000 | 0x00000001)
        except Exception:
            pass
    rom_path = morning.find_rom(args.rom)
    deployer = morning.find_deployer(args.deployer)
    open(morning.ROM_PATH_FILE, "w", encoding="utf-8").write(rom_path)
    open(morning.DEPLOYER_PATH_FILE, "w", encoding="utf-8").write(deployer)
    rom = bps.to_z64(open(rom_path, "rb").read())
    branch = cfg.get("branch", "claude/harkinian-tp-fishing-mod-ehepm9")
    poll = max(0.5, float(cfg.get("poll_minutes", 2)))
    log("The lab: the queue from %s:%s every %g min; Ctrl+C stops it" % (branch, QUEUE_PATH, poll))
    while True:
        commit = fetch(branch)
        if commit is None:
            log("fetch failed (offline?): trying again later")
        else:
            done = done_ids()
            todo = [r for r in read_queue(commit) if r[0] not in done]
            for rid, patch_rel, minutes, label in todo:
                if not run_one(rid, patch_rel, minutes, label, commit, rom, deployer, plug, not args.no_push):
                    # (no plug: with the software reboot it carries on regardless: a hung game just fails the runs
                    # after it, which costs nothing while nobody's there)
                    log("  the console may need someone (no plug): carrying on")
                commit = fetch(branch) or commit  # (new lines may have come in meanwhile: the next pass picks them up)
        if args.once:
            return
        time.sleep(poll * 60)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        log("stopped")
