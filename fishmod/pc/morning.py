#!/usr/bin/env python3
"""The morning runner: bench ROMs on the console, one after another, with nobody there.

For each line of the queue (fishmod/pc/morning_queue.txt, or patches given on the command line) it patches your ROM
(where it is: nothing is moved; the patched copy is made in fishmod/pc/work/ and deleted after its run), loads it
into the SummerCart64 with `sc64deployer upload --reboot` (the running game restarts into it: fish-oot's bench builds
answer the reboot request), records `sc64deployer debug` until the game says the route is done, and moves on. At the
end the results (the logs, each run's packets in packs of at most 29 MB, framelog.py's summary) are committed to the morning-results
branch and pushed, so they can be read from the repository; a results zip is made either way.

The first run: if the console isn't running a fish-oot bench build, nothing answers the reboot request, so it beeps
and asks you to press RESET (or switch the console on). After that it needs nobody.

Usage (in the fishoot conda env; morning.bat does this):
  python fishmod/pc/morning.py [PATCH.bps ...] [--rom ROM] [--deployer sc64deployer.exe] [--minutes N] [--no-pull]
         [--no-push]
The ROM's path is remembered (fishmod/pc/rom_path.txt, as run_all.py does), and the deployer's
(fishmod/pc/sc64deployer_path.txt).
"""
import argparse
import datetime
import glob
import hashlib
import json
import os
import shutil
import subprocess
import sys
import threading
import time
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)
import bps  # noqa: E402

QUEUE = os.path.join(HERE, "morning_queue.txt")
ROM_PATH_FILE = os.path.join(HERE, "rom_path.txt")
DEPLOYER_PATH_FILE = os.path.join(HERE, "sc64deployer_path.txt")
WORK = os.path.join(HERE, "work")
RESULTS = os.path.join(HERE, "results", "morning")
RESULTS_BRANCH = "morning-results"

START_SECONDS = 120   # from the reboot to the game saying FISHMOD-START
QUIET_SECONDS = 240   # no packet for this long: the game has stopped
DEFAULT_MINUTES = 25  # a run longer than this is stopped (a bench route takes about 11 minutes)


def log(msg):
    print(time.strftime("%H:%M:%S"), msg, flush=True)


def beep():
    if os.name == "nt":
        try:
            import winsound
            for _ in range(3):
                winsound.Beep(880, 300)
            return
        except Exception:
            pass
    print("\a", end="", flush=True)


def find_git():
    """git: on PATH, or the copy GitHub Desktop or Git for Windows installed (neither always puts it on PATH)."""
    found = shutil.which("git")
    if found:
        return found
    if os.name == "nt":
        guesses = []
        local = os.environ.get("LOCALAPPDATA", "")
        if local:
            # GitHub Desktop: %LOCALAPPDATA%\GitHubDesktop\app-<version>\resources\app\git\cmd\git.exe (newest first)
            guesses += sorted(glob.glob(os.path.join(local, "GitHubDesktop", "app-*", "resources", "app", "git", "cmd",
                                                     "git.exe")), reverse=True)
            guesses.append(os.path.join(local, "Programs", "Git", "cmd", "git.exe"))
        for base in (os.environ.get("ProgramFiles", r"C:\Program Files"),
                     os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")):
            guesses.append(os.path.join(base, "Git", "cmd", "git.exe"))
        for g in guesses:
            if os.path.isfile(g):
                return g
    return None


GIT = find_git()
# git never waits for a login it can't show (a pull or push run here, outside GitHub Desktop, may have none): it fails
# and the runner carries on
os.environ["GIT_TERMINAL_PROMPT"] = "0"
os.environ["GCM_INTERACTIVE"] = "never"
os.environ["GIT_ASKPASS"] = ""
os.environ["SSH_ASKPASS"] = ""
GIT_TIMEOUT = 120


def git(*args, env=None, check=True):
    r = subprocess.run([GIT, "-C", ROOT] + list(args), capture_output=True, text=True, env=env, timeout=GIT_TIMEOUT,
                       stdin=subprocess.DEVNULL)
    if check and r.returncode != 0:
        raise RuntimeError("git %s: %s" % (" ".join(args), (r.stderr or r.stdout).strip()))
    return r.stdout.strip()


# --- finding things ---

def remembered(path):
    try:
        p = open(path, encoding="utf-8").read().strip().strip('"')
        return p if os.path.isfile(p) else None
    except OSError:
        return None


def find_rom(given):
    for p in [given, remembered(ROM_PATH_FILE)]:
        if p and os.path.isfile(p):
            return os.path.abspath(p)
    beep()
    while True:
        p = input("Where is your Ocarina of Time NTSC 1.2 ROM? (its full path) ").strip().strip('"')
        if os.path.isfile(p):
            return os.path.abspath(p)
        print("  no file there")


def find_deployer(given):
    names = ["sc64deployer.exe", "sc64deployer"]
    guesses = [given, remembered(DEPLOYER_PATH_FILE)] + [shutil.which(n) for n in names]
    guesses += [os.path.join(d, n) for d in (HERE, ROOT) for n in names]
    for p in guesses:
        if p and os.path.isfile(p):
            return os.path.abspath(p)
    # (unzipped somewhere under Downloads or the Desktop?)
    for top in ("Downloads", "Desktop", "Documents"):
        base = os.path.join(os.path.expanduser("~"), top)
        for depth in ("*", os.path.join("*", "*"), os.path.join("*", "*", "*")):
            for n in names:
                for p in glob.glob(os.path.join(base, depth, n)) + glob.glob(os.path.join(base, n)):
                    if os.path.isfile(p):
                        return os.path.abspath(p)
    beep()
    while True:
        p = input("Where is sc64deployer.exe? (its full path) ").strip().strip('"')
        if os.path.isfile(p):
            return os.path.abspath(p)
        print("  no file there")


def read_queue(paths, minutes):
    """[(patch path, minutes, label)]: from the command line, or morning_queue.txt (patch [minutes] [label] a line)."""
    runs = []
    if paths:
        for p in paths:
            runs.append((os.path.abspath(p), minutes, os.path.splitext(os.path.basename(p))[0]))
        return runs
    if not os.path.isfile(QUEUE):
        raise SystemExit("No queue: %s is missing, and no patches were given" % QUEUE)
    for line in open(QUEUE, encoding="utf-8"):
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        parts = line.split()
        p = parts[0] if os.path.isabs(parts[0]) else os.path.join(ROOT, parts[0])
        m = float(parts[1]) if len(parts) > 1 else minutes
        label = parts[2] if len(parts) > 2 else os.path.splitext(os.path.basename(p))[0]
        runs.append((os.path.normpath(p), m, label))
    return runs


# --- the cart ---

def run_capture(cmd, out_path, timeout):
    """Runs a deployer command; its output to out_path (appended) and back as text."""
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout, errors="replace")
        text = (r.stdout or "") + (r.stderr or "")
        code = r.returncode
    except subprocess.TimeoutExpired as e:
        text = "(timed out)\n" + ((e.stdout or b"").decode(errors="replace") if isinstance(e.stdout, bytes)
                                  else (e.stdout or ""))
        code = -1
    with open(out_path, "a", encoding="utf-8") as f:
        f.write("$ %s\n%s\n(exit %d)\n" % (" ".join(cmd), text, code))
    return code, text


PACK_LIMIT = 29 * 1000 * 1000  # a pack of packets is closed before it reaches this many bytes
PACK_SETTLE = 2.0  # seconds a packet file must have been left alone before it's packed (sc64deployer done with it)


class Packer:
    """Moves `sc64deployer debug`'s packet files (one file a packet, a thousand or more a run) from its spool folder
    into compressed packs in the run's folder, packets-001.zip and on, none over PACK_LIMIT bytes."""

    def __init__(self, spool, folder):
        self.spool = spool
        self.folder = folder
        self.index = 1
        self.count = 0
        self.bytes = 0

    def pack_path(self):
        return os.path.join(self.folder, "packets-%03d.zip" % self.index)

    def step(self, final=False):
        now = time.time()
        files = sorted(glob.glob(os.path.join(self.spool, "binaryout-*.bin")))
        for f in files:
            try:
                st = os.stat(f)
            except OSError:
                continue
            if not final and (now - st.st_mtime) < PACK_SETTLE:
                continue
            path = self.pack_path()
            if os.path.exists(path) and os.path.getsize(path) + st.st_size > PACK_LIMIT:
                self.index += 1
                path = self.pack_path()
            with zipfile.ZipFile(path, "a", zipfile.ZIP_DEFLATED) as z:
                z.write(f, os.path.basename(f))
            self.count += 1
            self.bytes += st.st_size
            os.remove(f)

    def unpack_to(self, dest):
        """Every pack's packets in one folder (for the readers, which want a folder of them)."""
        os.makedirs(dest, exist_ok=True)
        for p in sorted(glob.glob(os.path.join(self.folder, "packets-*.zip"))):
            with zipfile.ZipFile(p) as z:
                z.extractall(dest)


class Debug:
    """`sc64deployer debug` in a spool folder (it saves each packet there as a file), watched for the game's lines."""

    def __init__(self, deployer, folder, spool):
        self.folder = folder
        self.started = threading.Event()
        self.done = threading.Event()
        self.lines = 0
        self.last_packet = None
        self.log = open(os.path.join(folder, "debug.log"), "a", encoding="utf-8", errors="replace")
        flags = subprocess.CREATE_NEW_PROCESS_GROUP if os.name == "nt" else 0
        self.proc = subprocess.Popen([deployer, "debug"], cwd=spool, stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE, stderr=subprocess.STDOUT, creationflags=flags)
        self.reader = threading.Thread(target=self._read, daemon=True)
        self.reader.start()

    def _read(self):
        for raw in iter(self.proc.stdout.readline, b""):
            line = raw.decode("utf-8", errors="replace")
            self.lines += 1
            if "FISHMOD-START" in line:
                self.started.set()
            if "FISHMOD-DONE" in line:
                self.done.set()
            if "Wrote [" in line:  # (one of those a packet: noted, not logged)
                self.last_packet = time.time()
            else:
                self.log.write(time.strftime("%H:%M:%S ") + line.rstrip() + "\n")
                self.log.flush()

    def alive(self):
        return self.proc.poll() is None

    def stop(self):
        """Ends it the polite way (stdin closed: it stops listening and switches save writeback off), else kills it."""
        try:
            self.proc.stdin.close()
        except OSError:
            pass
        try:
            self.proc.wait(10)
        except subprocess.TimeoutExpired:
            self.proc.kill()
            self.proc.wait(10)
        self.reader.join(5)
        self.log.close()


# --- one run ---

def one_run(n, patch, minutes, label, rom, deployer, stamp_dir, first):
    folder = os.path.join(stamp_dir, "%02d-%s" % (n, label))
    os.makedirs(folder, exist_ok=True)
    info = {"run": n, "label": label, "patch": os.path.relpath(patch, ROOT) if patch.startswith(ROOT) else patch,
            "minutes_max": minutes, "status": "failed", "started": time.strftime("%Y-%m-%d %H:%M:%S")}
    t0 = time.time()
    try:
        data = open(patch, "rb").read()
        info["patch_sha1"] = hashlib.sha1(data).hexdigest()
        target = bps.apply(rom, data)
    except Exception as e:
        info["error"] = "patch: %s" % e
        log("  patching failed: %s" % e)
        return info, True
    os.makedirs(WORK, exist_ok=True)
    rom_out = os.path.join(WORK, "morning.z64")  # (a ROM: kept out of the results, deleted after)
    open(rom_out, "wb").write(target)
    del target

    upload_log = os.path.join(folder, "upload.log")
    answered = False
    for attempt in range(3):
        code, text = run_capture([deployer, "upload", "--reboot", rom_out], upload_log, 300)
        if code == 0:
            answered = "no response for [Halt]" not in text and "no response for [Reboot]" not in text
            break
        log("  upload failed (exit %d): %s" % (code, text.strip().splitlines()[-1:] or ""))
        if attempt == 0:
            beep()
            log("  is the console switched on? Trying again in 30 s")
        time.sleep(30)
    else:
        info["error"] = "upload failed"
        os.remove(rom_out)
        return info, False
    log("  uploaded%s" % ("" if answered else " (the running game didn't answer the reboot request)"))

    spool = os.path.join(WORK, "spool")
    shutil.rmtree(spool, ignore_errors=True)
    os.makedirs(spool)
    packer = Packer(spool, folder)
    dbg = Debug(deployer, folder, spool)
    try:
        wait = START_SECONDS
        if not answered:
            beep()
            if first:
                log("  >>> Press RESET on the console (or switch it on) to start the new ROM <<<")
                wait = 3 * 3600
            else:
                log("  the last game stopped answering: waiting %d s in case it restarts anyway" % wait)
        if not dbg.started.wait(wait):
            info["error"] = "the new ROM never said FISHMOD-START"
            log("  " + info["error"])
            return info, False
        log("  running")
        info["status"] = "running"
        deadline = t0 + minutes * 60
        while not dbg.done.is_set():
            time.sleep(2)
            packer.step()
            if time.time() > deadline:
                info["error"] = "still running after %g minutes: stopped" % minutes
                break
            age = (time.time() - dbg.last_packet) if dbg.last_packet else None
            if age is not None and age > QUIET_SECONDS:
                info["error"] = "no packets for %d s: the game stopped" % QUIET_SECONDS
                break
            if not dbg.alive():
                info["error"] = "sc64deployer debug exited"
                break
        if dbg.done.is_set():
            time.sleep(3)  # (the last packets)
            info["status"] = "done"
            info.pop("error", None)
        else:
            log("  " + info["error"])
            info["status"] = "incomplete"
    finally:
        dbg.stop()
        packer.step(final=True)
        shutil.rmtree(spool, ignore_errors=True)
        if os.path.exists(rom_out):
            os.remove(rom_out)
    info["packets"] = packer.count
    info["bytes"] = packer.bytes
    info["packs"] = packer.index if packer.count else 0
    info["seconds"] = round(time.time() - t0)
    # framelog.py's summary (and the RDP bench's table), from the packs unpacked into a scratch folder
    if packer.count:
        unpack = os.path.join(WORK, "unpack")
        shutil.rmtree(unpack, ignore_errors=True)
        packer.unpack_to(unpack)
        shutil.copy(os.path.join(folder, "debug.log"), unpack)
        fl = os.path.join(ROOT, "fishmod", "tools", "framelog.py")
        try:
            r = subprocess.run([sys.executable, fl, unpack, "--by-leg"], capture_output=True, text=True, timeout=600,
                               errors="replace")
            open(os.path.join(folder, "framelog.txt"), "w", encoding="utf-8").write(r.stdout + r.stderr)
        except Exception as e:
            open(os.path.join(folder, "framelog.txt"), "w", encoding="utf-8").write("framelog.py failed: %s\n" % e)
        # the RDP calibration bench's table, when it's that run (its tests describe themselves in the debug log)
        try:
            if "RDPTEST" in open(os.path.join(folder, "debug.log"), encoding="utf-8", errors="replace").read():
                rb = os.path.join(ROOT, "fishmod", "tools", "rdpbench.py")
                r = subprocess.run([sys.executable, rb, unpack, "--csv", os.path.join(folder, "rdpbench.csv")],
                                   capture_output=True, text=True, timeout=600, errors="replace")
                open(os.path.join(folder, "rdpbench.txt"), "w", encoding="utf-8").write(r.stdout + r.stderr)
        except Exception as e:
            open(os.path.join(folder, "rdpbench.txt"), "w", encoding="utf-8").write("rdpbench.py failed: %s\n" % e)
        shutil.rmtree(unpack, ignore_errors=True)
    # carry on only if this game is one that answers the next reboot request (it finished, or at least started)
    return info, info["status"] in ("done", "incomplete")


# --- the results back ---

def publish(stamp, stamp_dir, push):
    """Commits results/morning/runs/<stamp> to the morning-results branch (without touching your checkout) and pushes."""
    if not push:
        return "not pushed (--no-push)"
    try:
        git("rev-parse", "--git-dir")
    except Exception as e:
        return "not pushed: not a git checkout (%s)" % e
    parent = None
    try:
        fetched = subprocess.run([GIT, "-C", ROOT, "fetch", "-q", "origin", RESULTS_BRANCH], capture_output=True,
                                 timeout=GIT_TIMEOUT, stdin=subprocess.DEVNULL).returncode == 0
    except subprocess.TimeoutExpired:
        return "not pushed: git fetch timed out (no GitHub login outside GitHub Desktop?)"
    if fetched:
        parent = git("rev-parse", "FETCH_HEAD")
    os.makedirs(WORK, exist_ok=True)
    index = os.path.join(WORK, "morning.index")
    if os.path.exists(index):
        os.remove(index)
    env = dict(os.environ, GIT_INDEX_FILE=index)
    if not git("config", "user.email", check=False):
        env.update(GIT_AUTHOR_NAME="fish-oot morning", GIT_AUTHOR_EMAIL="morning@fish-oot.invalid",
                   GIT_COMMITTER_NAME="fish-oot morning", GIT_COMMITTER_EMAIL="morning@fish-oot.invalid")
    if parent:
        git("read-tree", parent, env=env)
    else:
        git("read-tree", "--empty", env=env)
    rel = os.path.relpath(stamp_dir, RESULTS).replace(os.sep, "/")
    git_dir = git("rev-parse", "--absolute-git-dir")
    r = subprocess.run([GIT, "--git-dir", git_dir, "--work-tree", ".", "add", "-f", "--", rel], cwd=RESULTS,
                       capture_output=True, text=True, env=env)
    if r.returncode != 0:
        return "not pushed: git add: %s" % r.stderr.strip()
    tree = git("write-tree", env=env)
    msg = "Morning runs %s" % stamp
    args = ["commit-tree", tree, "-m", msg] + (["-p", parent] if parent else [])
    commit = git(*args, env=env)
    os.remove(index)
    try:
        r = subprocess.run([GIT, "-C", ROOT, "push", "-q", "origin", "%s:refs/heads/%s" % (commit, RESULTS_BRANCH)],
                           capture_output=True, text=True, timeout=GIT_TIMEOUT, stdin=subprocess.DEVNULL)
    except subprocess.TimeoutExpired:
        return "not pushed: git push timed out (no GitHub login outside GitHub Desktop?)"
    if r.returncode != 0:
        return "push failed: %s" % (r.stderr or r.stdout).strip()
    return "pushed to %s (%s)" % (RESULTS_BRANCH, commit[:10])


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("patches", nargs="*", help="patches to run (default: morning_queue.txt)")
    ap.add_argument("--rom")
    ap.add_argument("--deployer")
    ap.add_argument("--minutes", type=float, default=DEFAULT_MINUTES, help="the longest a run may take")
    ap.add_argument("--no-pull", action="store_true", help="don't git pull first")
    ap.add_argument("--no-push", action="store_true", help="don't push the results")
    args = ap.parse_args()

    if GIT is None:
        log("git not found (not on PATH, and no GitHub Desktop or Git for Windows copy): no pull, and the results are "
            "only zipped")
        args.no_pull = True
        args.no_push = True
    if not args.no_pull:
        log("git pull...")
        try:
            r = subprocess.run([GIT, "-C", ROOT, "pull", "-q", "--ff-only"], capture_output=True, text=True,
                               timeout=GIT_TIMEOUT, stdin=subprocess.DEVNULL)
        except subprocess.TimeoutExpired:
            r = subprocess.CompletedProcess([], 1, "", "timed out (no GitHub login outside GitHub Desktop?)")
        log("git pull: %s" % ("up to date" if r.returncode == 0 else "skipped (%s)" % r.stderr.strip()[:200]))
        if r.returncode == 0 and not args.patches:
            # (this script may have changed with the pull: run the new one)
            sys.exit(subprocess.run([sys.executable, os.path.abspath(__file__), "--no-pull"] + sys.argv[1:]).returncode)

    if os.name == "nt":  # (no sleeping while it runs: the USB link would drop)
        try:
            import ctypes
            ctypes.windll.kernel32.SetThreadExecutionState(0x80000000 | 0x00000001)  # ES_CONTINUOUS | SYSTEM_REQUIRED
        except Exception:
            pass
    runs = read_queue(args.patches, args.minutes)
    missing = [p for p, _, _ in runs if not os.path.isfile(p)]
    if missing:
        raise SystemExit("Missing patches: " + ", ".join(missing))
    rom_path = find_rom(args.rom)
    deployer = find_deployer(args.deployer)
    open(ROM_PATH_FILE, "w", encoding="utf-8").write(rom_path)
    open(DEPLOYER_PATH_FILE, "w", encoding="utf-8").write(deployer)
    rom = bps.to_z64(open(rom_path, "rb").read())
    log("ROM: %s; deployer: %s" % (rom_path, deployer))
    log("%d runs, about %d minutes in all:" % (len(runs), len(runs) * 13))
    for n, (p, m, label) in enumerate(runs, 1):
        log("  %2d. %s (at most %g min)" % (n, label, m))

    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M")
    stamp_dir = os.path.join(RESULTS, "runs", stamp)
    os.makedirs(stamp_dir, exist_ok=True)
    infos = []
    for n, (patch, minutes, label) in enumerate(runs, 1):
        log("Run %d/%d: %s" % (n, len(runs), label))
        first = not any(i.get("status") in ("done", "incomplete") for i in infos)  # (nothing of ours running yet)
        info, carry_on = one_run(n, patch, minutes, label, rom, deployer, stamp_dir, first)
        infos.append(info)
        log("  %s: %s%s" % (label, info["status"], (" (%s)" % info["error"]) if info.get("error") else ""))
        json.dump(infos, open(os.path.join(stamp_dir, "runs.json"), "w"), indent=1)
        if not carry_on:
            log("Stopping here: the console needs someone (the rest of the queue is left)")
            break

    lines = ["Morning runs %s" % stamp, ""]
    for i in infos:
        lines.append("%2d. %-28s %-10s %4s packets %6.1f MB %5s s  %s" % (
            i["run"], i["label"], i["status"], i.get("packets", 0), i.get("bytes", 0) / 1e6, i.get("seconds", ""),
            i.get("error", "")))
    if len(infos) < len(runs):
        lines.append("(%d not run)" % (len(runs) - len(infos)))
    open(os.path.join(stamp_dir, "summary.txt"), "w", encoding="utf-8").write("\n".join(lines) + "\n")
    print("\n".join(lines))

    zip_path = os.path.join(HERE, "results", "morning_%s.zip" % stamp)
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_STORED) as z:
        for base, _, files in os.walk(stamp_dir):
            for f in files:
                full = os.path.join(base, f)
                z.write(full, os.path.relpath(full, os.path.dirname(stamp_dir)))
    try:
        result = publish(stamp, stamp_dir, not args.no_push)
    except Exception as e:
        result = "not pushed: %s" % e
    log("Results: %s" % result)
    log("Also zipped: %s" % zip_path)


if __name__ == "__main__":
    main()
