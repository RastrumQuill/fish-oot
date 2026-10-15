# Running test jobs on your PC

Your PC runs the long emulator jobs, so the cloud container isn't the bottleneck. These include the draw-distance
sweep, the rotation check and the calibrations. A Ryzen 7 5700X3D (16 threads) runs several ares windows at once, and
the RTX 3060 is there for GPU work later. I send a **job pack** (a .zip); you run one command; you send back the
**results zip** it makes.

## Setting up (once)

1. **Miniconda:** https://docs.conda.io/en/latest/miniconda.html (64-bit Windows installer).
2. **ares:** https://ares-emu.net/download. Unzip it somewhere, for example `C:\ares\`.
3. **Get this folder:** clone or download the repo, or just the `fishmod\pc\` folder from it.
4. **Make the environment.** In the *Anaconda Prompt*, from the repo folder:

       conda env create -f fishmod\pc\environment.yml
       conda activate fishoot

5. **Your ROM:** Ocarina of Time NTSC 1.2 (US). Any byte order is fine (.z64, .n64 or .v64); the runner converts it.

## Running a pack

    conda activate fishoot
    python fishmod\pc\run_jobs.py C:\Downloads\selftest.zip --rom C:\roms\oot.z64 --ares C:\ares\ares.exe

- **ares windows:** it opens several at once, each running a patched ROM, and closes each one when its time is up.
  Leave them alone, minimised is fine. `--parallel N` sets how many run at once (default 6).
- **The results file:** at the end it prints where `results_<pack>.zip` is. Send me that file.
- **Long jobs:** running the same pack again picks up where the last run stopped.
- **When a save goes missing:** if the runner says `save NOT FOUND`, ares is putting its saves somewhere else. Look
  under ares's *Settings → Paths → Saves* and pass that folder with `--saves-dir`.

## The first pack: `selftest`

It runs the Lake Hylia tour for two minutes. It shows that ares, the ROM and the patching work, and how fast your PC
runs the game. The runner prints `NNNN game frames ran (NN/s)`; about 20 per second is full speed.

**If no frames ran:** the game didn't start. That means the black screen from before: most likely ares's Expansion
Pak setting, or its video driver (try another under *Settings → Video*).
