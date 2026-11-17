# Sharing a sweep between machines

A sweep is hundreds of emulator runs. Any Linux machine can take some of them: your PC (under WSL), a Colab
session, a server. Each machine runs the same worker. The workers share one queue and never take the same run.

## How it works

- **The queue** is `fishmod/sweep/queue.json` on the default branch. It names a sweep's builds (as patches for your
  ROM, under `releases/sweep/`) and its runs.
- **A worker** fetches the queue, makes each build from your ROM, and takes units of work. A unit is one part of one
  leg, played on every build. When a unit ends the worker checks it there and then: the same sound as the reference
  build buffer for buffer, the end reached, nothing stopped.
- **The results** go to the `sweep-results` branch, a small folder per unit. A unit being worked on has a claim file
  there, so another worker skips it. A claim is free again after 8 hours (the worker stopped).
- **Your ROM never leaves your machine.** Only the patches and the results travel.

The worker reads data only from the repository (the queue, patches, symbol files). Its code is what you started; a
new version runs after your own `git pull`.

## On your PC (Windows, with WSL)

Once:

1. In PowerShell as administrator: `wsl --install`, then restart. Open "Ubuntu" from the Start menu and pick a
   user name.
2. In Ubuntu, let git use your Windows GitHub login:

   ```bash
   git config --global credential.helper "/mnt/c/Program\ Files/Git/mingw64/bin/git-credential-manager.exe"
   ```

3. Clone the repository inside Ubuntu (a clone under `/mnt/c` works but is slow):

   ```bash
   git clone https://github.com/RastrumQuill/fish-oot ~/fish-oot
   ```

Each time (the ROM path is remembered after the first):

```bash
cd ~/fish-oot && git pull && fishmod/sweep/worker.sh --rom /mnt/c/roms/oot.z64
```

The first start installs packages (it asks for your password) and builds ares, about 15 minutes. After that it
prints each run as it ends and each unit's verdict, and stops when the queue is empty. Ctrl+C stops it; its
unfinished units become free again after 8 hours, or at once when you start it again.

It runs one emulator per core, less one. `--parallel N` sets another number.

## On Colab

Open `fishmod/sweep/colab.ipynb` in Colab and run its cells. It asks for a GitHub token (to clone and to push the
results) and for your ROM (uploaded to the session, or read from your Drive). A free session has 2 cores and ends
after some hours; what it finished is already pushed, and the rest is taken by whoever comes next.

## Without a git login

`--no-push` keeps the results on the machine and makes a zip at the end. On a machine with a login,
`fishmod/sweep/worker.py --collect THE.zip` adds it to the results branch.

## Seeing where a sweep is

```bash
fishmod/sweep/worker.py --status
```

prints, per sweep: units done, failed (with why), claimed (by whom) and left. `--verbose` adds every unit's verdict.

## Making a sweep (the lead's side)

1. Build each ROM, then per build: `fishmod/tools/bps.py --create --bps BASE ROM releases/sweep/ID-BUILD.bps` (a later
   build can be a patch on an earlier one: list both) and
   `fishmod/tools/audio_sweep_report.py --syms ELF releases/sweep/ID-BUILD.sym.gz`.
2. Add the sweep to `queue.json` (each build's patches, symbols and the ROM's sha1; the jobs in the order to run
   them) and push.

Runs are the same on every machine: the emulator's cycle counts and the sound log depend only on the ROM and the
emulator's source, both fixed by the repository. `"nodraw": true` in a sweep runs the emulator without drawing the
picture (`ARES_NORDP=1`), which is faster and changes nothing the audio checks read.
