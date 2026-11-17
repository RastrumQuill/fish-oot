#!/usr/bin/env python3
"""The sweep worker: any Linux machine (your PC under WSL, a Colab session, a server) takes runs of a sweep from the
queue in the repository, plays them in ares, and pushes the results. Several machines share one sweep; each takes
different runs. fishmod/sweep/README.md has the setup; worker.sh installs what this needs and starts it.

What it reads from the repository is data only: fishmod/sweep/queue.json on the queue branch, and the patches and
symbol files it names (read with `git show`, never checked out). Your ROM never leaves your machine: each build is
made from it by its patches, here.

A sweep in the queue:
  {"id": "audio-1", "nodraw": true, "maxsecs": 9000, "ref": "R",
   "builds": {"R": {"patches": ["releases/sweep/audio-1-R.bps"], "syms": "releases/sweep/audio-1-R.sym.gz",
                    "sha1": "..."}, ...},
   "jobs": [["t2", 2], ["o2", 1], ["s2", 4], ...]}          (variant, parts: fishmod/tools/audio_sweep.sh)
A unit of work is one part of one variant, played on every build (so the unit can be checked where it ran: the same
sound as the reference build, the end reached, nothing stopped). Results go to the results branch:
  <id>/<variant>.<part>/<build>/{aud.bin, report.json, report.txt, run.log, bus.txt, rsp.bin}   and   <id>/<variant>.<part>/check.txt
and a unit being worked on has <id>/claims/<variant>.<part>.json (who and when; stale after --claim-hours).

Usage:
  fishmod/sweep/worker.py --rom ROM [--parallel N] [--name NAME] [--from-end] [--no-push] [--once N]
  fishmod/sweep/worker.py --status          what is done, claimed, failed and left, from the results branch
  fishmod/sweep/worker.py --collect ZIP...  (no git login on the worker: it made zips with --no-push; this adds them)
"""
import argparse
import atexit
import concurrent.futures
import hashlib
import json
import os
import queue as queuelib
import shutil
import socket
import struct
import subprocess
import sys
import time
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
TOOLS = os.path.join(ROOT, "fishmod", "tools")
sys.path.insert(0, os.path.join(ROOT, "fishmod", "pc"))
import bps  # noqa: E402

QUEUE_PATH = "fishmod/sweep/queue.json"
QUEUE_BRANCH = "claude/harkinian-tp-fishing-mod-ehepm9"
RESULTS_BRANCH = "sweep-results"
WORK = os.environ.get("FISHOOT_SWEEP_WORK") or os.path.expanduser("~/.fishoot-sweep")
KEEP = ("aud.bin", "report.json", "report.txt", "run.log", "bus.txt", "rsp.bin")
KINDS = {"s": (0, 0), "c": (0, 1), "q": (1, 0), "e": (2, 0), "o": (3, 0), "t": (4, 0)}  # letter: (leg, child at night)


def log(msg):
    print("[%s] %s" % (time.strftime("%H:%M:%S"), msg), flush=True)


def git(*args, cwd=ROOT, check=True, binary=False, env=None):
    r = subprocess.run(["git"] + list(args), cwd=cwd, capture_output=True, stdin=subprocess.DEVNULL, env=env)
    if check and r.returncode != 0:
        raise RuntimeError("git %s: %s" % (" ".join(args[:3]), r.stderr.decode(errors="replace").strip()))
    return r.stdout if binary else r.stdout.decode(errors="replace").strip()


# --- the queue (data out of the fetched branch) ---

class Source:
    """Where the queue and its files are read from: a fetched branch, or (--local) the files of this checkout."""

    def __init__(self, branch, local):
        self.ref = None
        if not local:
            git("fetch", "-q", "origin", branch)
            self.ref = git("rev-parse", "FETCH_HEAD")

    def read(self, path):
        if os.path.isabs(path) or ".." in path.split("/"):
            raise RuntimeError("the queue names a path outside the repository: %s" % path)
        if self.ref is None:
            return open(os.path.join(ROOT, path), "rb").read()
        return git("show", "%s:%s" % (self.ref, path), binary=True)

    def queue(self):
        return json.loads(self.read(QUEUE_PATH))["sweeps"]


def units_of(sweep):
    return ["%s.%d" % (v, k) for v, parts in sweep["jobs"] for k in range(parts)]


def parts_of(sweep, unit):
    return dict((v, p) for v, p in sweep["jobs"])[unit.split(".")[0]]


# --- the results branch (a clone of its own under WORK: your checkout isn't touched) ---

class Results:
    def __init__(self, push):
        self.dir = os.path.join(WORK, "results")
        self.push_on = push
        if not os.path.isdir(os.path.join(self.dir, ".git")):
            os.makedirs(self.dir, exist_ok=True)
            git("init", "-q", cwd=self.dir)
            git("remote", "add", "origin", os.environ.get("FISHOOT_SWEEP_REMOTE") or git("remote", "get-url", "origin"),
                cwd=self.dir)
            for k in ("user.name", "user.email", "credential.helper", "http.proxy", "http.extraheader"):
                v = git("config", "--get", k, check=False)
                if v:
                    git("config", k, v, cwd=self.dir)
        if not git("config", "--get", "user.email", cwd=self.dir, check=False):
            git("config", "user.name", "fish-oot sweep", cwd=self.dir)
            git("config", "user.email", "sweep@fish-oot.invalid", cwd=self.dir)
        git("checkout", "-q", "-B", RESULTS_BRANCH, cwd=self.dir, check=False)
        self.sync()

    def sync(self):
        """The branch as it is on the remote (local commits not pushed yet are kept on top of it)."""
        r = subprocess.run(["git", "fetch", "-q", "origin", RESULTS_BRANCH], cwd=self.dir, capture_output=True,
                           stdin=subprocess.DEVNULL)
        if r.returncode != 0:
            return False  # (no such branch yet, or no network: carry on with what's here)
        if not git("rev-parse", "-q", "--verify", "HEAD", cwd=self.dir, check=False):
            git("reset", "-q", "--hard", "FETCH_HEAD", cwd=self.dir)
            return True
        if subprocess.run(["git", "rebase", "-q", "FETCH_HEAD"], cwd=self.dir, capture_output=True).returncode != 0:
            git("rebase", "--abort", cwd=self.dir, check=False)
            return None  # (somebody else wrote the same file first)
        return True

    def commit_push(self, msg):
        """True: on the remote (or kept locally under --no-push). None: lost to somebody else's commit (undone)."""
        git("add", "-A", cwd=self.dir)
        if not git("status", "--porcelain", cwd=self.dir):
            return True
        git("commit", "-q", "-m", msg, cwd=self.dir)
        if not self.push_on:
            return True
        for attempt in range(6):
            r = subprocess.run(["git", "push", "-q", "origin", "HEAD:refs/heads/" + RESULTS_BRANCH], cwd=self.dir,
                               capture_output=True, stdin=subprocess.DEVNULL)
            if r.returncode == 0:
                return True
            s = self.sync()
            if s is None:
                git("reset", "-q", "--hard", "FETCH_HEAD", cwd=self.dir)
                return None
            if s is False:
                log("push failed (%s): retrying in a minute" % r.stderr.decode(errors="replace").strip()[:200])
                time.sleep(60)
        log("push still failing: the results stay in %s and go with the next push" % self.dir)
        return True

    def path(self, *p):
        return os.path.join(self.dir, *p)

    def state(self, sweep, unit, claim_hours):
        """'done', 'failed', ('claimed', by) or 'free'."""
        chk = self.path(sweep["id"], unit, "check.txt")
        if os.path.exists(chk):
            return "done" if open(chk).read().startswith("PASS") else "failed"
        c = self.path(sweep["id"], "claims", unit + ".json")
        if os.path.exists(c):
            try:
                d = json.load(open(c))
                if time.time() - d["time"] < claim_hours * 3600:
                    return ("claimed", d["worker"])
            except (ValueError, KeyError):
                pass
        return "free"


# --- the builds' ROMs ---

def make_roms(src, sweep, base):
    out = {}
    os.makedirs(os.path.join(WORK, "roms"), exist_ok=True)
    cache = {}
    for name, b in sweep["builds"].items():
        rom_path = os.path.join(WORK, "roms", "%s-%s.z64" % (sweep["id"], name))
        sym_path = os.path.join(WORK, "roms", "%s-%s.sym.gz" % (sweep["id"], name))
        if not (os.path.exists(rom_path) and hashlib.sha1(open(rom_path, "rb").read()).hexdigest() == b["sha1"]):
            rom = base
            for i, p in enumerate(b["patches"]):
                key = tuple(b["patches"][:i + 1])
                if key not in cache:
                    cache[key] = bps.apply(rom, src.read(p))
                rom = cache[key]
            if hashlib.sha1(rom).hexdigest() != b["sha1"]:
                raise RuntimeError("build %s of %s didn't come out as the queue says (wrong ROM?)" % (name, sweep["id"]))
            open(rom_path, "wb").write(rom)
        open(sym_path, "wb").write(src.read(b["syms"]))
        out[name] = (rom_path, sym_path)
    return out


def variant_word(variant, parts, part):
    """The options word in the ROM's header (0x38): fishmod/tools/audio_sweep.sh."""
    kind, child = KINDS[variant[0]]
    return int(variant[1]) | child << 4 | parts << 8 | part << 16 | kind << 24


# --- one run, one unit ---

def one_run(sweep, unit, build, roms, slots, ares, env):
    rom_path, sym_path = roms[build]
    d = os.path.join(WORK, "runs", sweep["id"], unit, build)
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(d)
    variant, part = unit.split(".")
    rom = bytearray(open(rom_path, "rb").read())
    struct.pack_into(">I", rom, 0x38, variant_word(variant, parts_of(sweep, unit), int(part)))
    open(os.path.join(d, "rom.z64"), "wb").write(rom)
    slot = slots.get()
    t0 = time.time()
    try:
        e = dict(env, ARES=ares, FISHMOD_ROOT=ROOT)
        if sweep.get("nodraw"):
            e["ARES_NORDP"] = "1"
        with open(os.path.join(d, "run.log"), "w") as f:
            subprocess.run(["bash", os.path.join(TOOLS, "audio_sweep_run.sh"), str(slot), os.path.join(d, "rom.z64"),
                            sym_path, d, str(sweep.get("maxsecs", 9000))], stdout=f, stderr=subprocess.STDOUT, env=e)
        with open(os.path.join(d, "report.txt"), "w") as f:
            subprocess.run([sys.executable, os.path.join(TOOLS, "audio_sweep_report.py"), d, "--elf", sym_path],
                           stdout=f, stderr=subprocess.STDOUT, env=e)
    finally:
        slots.put(slot)
    for n in os.listdir(d):
        if n not in KEEP:
            os.remove(os.path.join(d, n))
    head = open(os.path.join(d, "report.txt")).readline().strip()
    log("  %s %s: %s (%d min)" % (unit, build, head.split(": ", 1)[-1], (time.time() - t0) / 60))
    return d


def check_unit(sweep, unit):
    """The unit's verdict: (passed, text). Every build against the reference: the same sound, buffer for buffer; both
    run to the end; no thread stopped; the sound log not full (then its end wasn't compared)."""
    base = os.path.join(WORK, "runs", sweep["id"], unit)
    ref = sweep.get("ref", "R")
    lines, ok = [], True

    def rep(b):
        try:
            return json.load(open(os.path.join(base, b, "report.json")))
        except (OSError, ValueError):
            return None

    for b in sweep["builds"]:
        r = rep(b)
        if r is None:
            lines.append("%s: NO REPORT (the run didn't start or was cut short)" % b)
            ok = False
            continue
        bad = []
        if not r.get("ended"):
            bad.append("END NOT REACHED")
        if r.get("stopped"):
            bad.append("STOPPED %s" % ["thread %d at %s" % (t["id"], t["at"]) for t in r["stopped"]])
        if r.get("buffers", 0) >= r.get("buffers_max", 1):
            bad.append("SOUND LOG FULL")
        if b != ref:
            s = subprocess.run([sys.executable, os.path.join(TOOLS, "audlog_compare.py"),
                                os.path.join(base, ref, "aud.bin"), os.path.join(base, b, "aud.bin")],
                               capture_output=True, text=True).stdout.strip().splitlines()
            snd = s[-1] if s else "NO SOUND LOG"
            if not snd.startswith("SAME"):
                bad.append(snd)
            c = subprocess.run([sys.executable, os.path.join(TOOLS, "audio_sweep_report.py"), "--compare",
                                os.path.join(base, ref), os.path.join(base, b)], capture_output=True, text=True).stdout
            if "FRAMES-DIFFER" in c:
                bad.append("FRAMES DIFFER")
            cost = [l for l in c.splitlines() if l.startswith("all ")]
            lines.append("%s vs %s: %s; %s" % (b, ref, snd, " ".join(cost[0].split()) if cost else "no cost line"))
            lines += ["  " + " ".join(l.split()) for l in c.splitlines() if l.startswith("memory ")]
        else:
            lines.append("%s: %d legs, %s buffers" % (b, len(r["legs"]), r.get("buffers", "?")))
        if bad:
            ok = False
            lines.append("  %s FAILED: %s" % (b, "; ".join(bad)))
    return ok, "%s %s (%s, %s)\n%s\n" % ("PASS" if ok else "FAIL", unit, socket.gethostname(),
                                         time.strftime("%Y-%m-%d %H:%M"), "\n".join(lines))


# --- the worker ---

def find_ares(given):
    for p in (given, os.environ.get("ARES"), os.path.join(WORK, "ares", "build", "rundir", "bin", "ares"),
              "/opt/src/ares/build/rundir/bin/ares"):
        if p and os.path.isfile(p):
            return p
    sys.exit("no ares: run fishmod/sweep/worker.sh (it builds it), or pass --ares")


def start_display():
    """A display of our own (nothing appears on your screen)."""
    for n in range(197, 207):
        if not os.path.exists("/tmp/.X11-unix/X%d" % n):
            x = subprocess.Popen(["Xvfb", ":%d" % n, "-screen", "0", "1024x768x24"], stdout=subprocess.DEVNULL,
                                 stderr=subprocess.DEVNULL)
            atexit.register(x.terminate)
            time.sleep(2)
            return ":%d" % n
    sys.exit("no free display number")


def claim(res, sweeps, want, a, mine):
    """Up to `want` free units, claimed on the results branch: [(sweep, unit)]."""
    for attempt in range(8):
        res.sync()
        free = []
        for sw in sweeps:
            us = units_of(sw)
            for u in (reversed(us) if a.from_end else us):
                st = res.state(sw, u, a.claim_hours)
                if (st == "free" or st == ("claimed", a.name)) and (sw["id"], u) not in mine:
                    free.append((sw, u))
        take = free[:want]
        if not take:
            return []
        for sw, u in take:
            p = res.path(sw["id"], "claims", u + ".json")
            os.makedirs(os.path.dirname(p), exist_ok=True)
            json.dump({"worker": a.name, "time": int(time.time())}, open(p, "w"))
        if res.commit_push("%s takes %s" % (a.name, " ".join(u for _, u in take))):
            return take
        # (somebody claimed one of them a moment before: look again)
    return []


def publish(res, sweep, unit, text, a):
    src = os.path.join(WORK, "runs", sweep["id"], unit)
    for attempt in range(4):
        dst = res.path(sweep["id"], unit)
        if os.path.exists(os.path.join(dst, "check.txt")) and open(os.path.join(dst, "check.txt")).read().startswith("PASS"):
            log("  %s: already there from another worker (kept theirs)" % unit)
            return
        shutil.rmtree(dst, ignore_errors=True)
        shutil.copytree(src, dst)
        open(os.path.join(dst, "check.txt"), "w").write(text)
        c = res.path(sweep["id"], "claims", unit + ".json")
        if os.path.exists(c):
            os.remove(c)
        if res.commit_push("%s: %s %s" % (a.name, unit, text.split()[0])):
            return
    log("  %s: couldn't publish (kept in %s)" % (unit, src))


def status(a):
    src = Source(a.branch, a.local)
    res = Results(False)
    for sw in src.queue():
        us = units_of(sw)
        st = [(u, res.state(sw, u, a.claim_hours)) for u in us]
        done = [u for u, s in st if s == "done"]
        failed = [u for u, s in st if s == "failed"]
        claimed = [(u, s[1]) for u, s in st if isinstance(s, tuple)]
        print("%s: %d of %d units done, %d failed, %d claimed, %d left" % (
            sw["id"], len(done), len(us), len(failed), len(claimed), len(us) - len(done) - len(failed) - len(claimed)))
        for u in failed:
            print("  FAILED %s:\n    %s" % (u, open(res.path(sw["id"], u, "check.txt")).read().strip().replace("\n", "\n    ")))
        if claimed:
            print("  claimed: " + ", ".join("%s (%s)" % c for c in claimed))
        if a.verbose:
            for u in done:
                print("  " + open(res.path(sw["id"], u, "check.txt")).read().strip().replace("\n", "\n    "))


def collect(a):
    res = Results(True)
    for z in a.collect:
        with zipfile.ZipFile(z) as f:
            for n in f.namelist():
                if os.path.isabs(n) or ".." in n.split("/"):
                    sys.exit("%s: a path outside the results (%s)" % (z, n))
            f.extractall(res.dir)
        print("%s: %s" % (z, "added" if res.commit_push("collected %s" % os.path.basename(z)) else "conflicts with what's there"))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rom", help="Ocarina of Time NTSC 1.2 (US), any byte order (remembered)")
    ap.add_argument("--parallel", type=int, help="runs at once (default: the cores, less one)")
    ap.add_argument("--name", default=socket.gethostname(), help="this worker's name in the claims")
    ap.add_argument("--ares")
    ap.add_argument("--branch", default=QUEUE_BRANCH, help="the branch the queue is read from")
    ap.add_argument("--local", action="store_true", help="the queue and its files from this checkout, not the remote")
    ap.add_argument("--from-end", action="store_true", help="take units from the end of the queue (two workers meet in the middle)")
    ap.add_argument("--no-push", action="store_true", help="keep the results here and zip them (no git login)")
    ap.add_argument("--once", type=int, metavar="N", help="at most N units, then stop")
    ap.add_argument("--only", help="only this sweep id")
    ap.add_argument("--claim-hours", type=float, default=8, help="a claim older than this is free again")
    ap.add_argument("--status", action="store_true")
    ap.add_argument("--verbose", action="store_true")
    ap.add_argument("--collect", nargs="+", metavar="ZIP")
    a = ap.parse_args()
    os.makedirs(WORK, exist_ok=True)
    if a.status:
        return status(a)
    if a.collect:
        return collect(a)

    remembered = os.path.join(WORK, "rom_path.txt")
    rom_path = a.rom or (open(remembered).read().strip() if os.path.exists(remembered) else None)
    if not rom_path or not os.path.isfile(rom_path):
        sys.exit("the ROM: --rom PATH (Ocarina of Time NTSC 1.2 (US))")
    open(remembered, "w").write(os.path.abspath(rom_path))
    base = bps.to_z64(open(rom_path, "rb").read())
    ares = find_ares(a.ares)
    par = a.parallel or max(1, (os.cpu_count() or 2) - 1)
    env = dict(os.environ, DISPLAY=start_display(), SWEEP_PORT_BASE=os.environ.get("SWEEP_PORT_BASE", "9323"))
    slots = queuelib.Queue()
    for s in range(par):
        slots.put(s)
    res = Results(not a.no_push)
    log("worker %s: %d runs at once, ares %s" % (a.name, par, ares))

    done_units = 0
    mine = set()
    roms = {}
    pending = {}  # future -> (sweep, unit, build)
    left = {}     # (sweep id, unit) -> builds still running
    with concurrent.futures.ThreadPoolExecutor(par) as pool:
        while True:
            # (keep the pool full: a unit is as many runs as there are builds)
            room = par - len(pending)
            more = a.once is None or len(mine) < a.once
            if room > 0 and more:
                src = Source(a.branch, a.local)
                sweeps = [s for s in src.queue() if not a.only or s["id"] == a.only]
                nb = max(len(s["builds"]) for s in sweeps) if sweeps else 1
                want = max(1, -(-room // nb))
                if a.once is not None:
                    want = min(want, a.once - len(mine))
                for sw, u in claim(res, sweeps, want, a, mine):
                    if sw["id"] not in roms:
                        log("making the ROMs of %s" % sw["id"])
                        roms[sw["id"]] = make_roms(src, sw, base)
                    mine.add((sw["id"], u))
                    left[(sw["id"], u)] = set(sw["builds"])
                    log("taking %s %s" % (sw["id"], u))
                    for b in sw["builds"]:
                        pending[pool.submit(one_run, sw, u, b, roms[sw["id"]], slots, ares, env)] = (sw, u, b)
            if not pending:
                break
            finished, _ = concurrent.futures.wait(pending, timeout=600, return_when=concurrent.futures.FIRST_COMPLETED)
            for f in finished:
                sw, u, b = pending.pop(f)
                if f.exception():
                    log("  %s %s: %s" % (u, b, f.exception()))
                left[(sw["id"], u)].discard(b)
                if not left[(sw["id"], u)]:
                    ok, text = check_unit(sw, u)
                    log("%s %s: %s" % (sw["id"], u, text.strip().replace("\n", "\n           ")))
                    publish(res, sw, u, text, a)
                    done_units += 1
    log("nothing left to take: %d units done here" % done_units)
    if a.no_push:
        z = os.path.join(WORK, "sweep-results-%s-%s.zip" % (a.name, time.strftime("%Y%m%d-%H%M")))
        with zipfile.ZipFile(z, "w", zipfile.ZIP_DEFLATED) as f:
            for sid, u in sorted(mine):
                d = res.path(sid, u)
                for dp, _, fs in os.walk(d):
                    for n in fs:
                        p = os.path.join(dp, n)
                        f.write(p, os.path.relpath(p, res.dir))
        log("not pushed (--no-push): send %s" % z)


if __name__ == "__main__":
    main()
