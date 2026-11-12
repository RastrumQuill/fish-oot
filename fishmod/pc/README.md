# Running test jobs on your PC

Your PC runs the long emulator jobs, so the cloud container isn't the bottleneck. These include the draw-distance
sweep, the rotation check and the calibrations. A Ryzen 7 5700X3D (16 threads) runs several ares windows at once, and
the RTX 3060 is there for GPU work later. I send a **job pack** (a .zip); you run one command; you send back the
**results zip** it makes.

## Setting up (once)

1. **Miniconda:** https://docs.conda.io/en/latest/miniconda.html (64-bit Windows installer).
2. **The repo:** clone it, for example `git clone https://github.com/RastrumQuill/fish-oot C:\fish-oot`, then switch
   to the working branch: `git checkout claude/harkinian-tp-fishing-mod-ehepm9`. `git pull` gets updates.
3. **Setup:** in the *Anaconda Prompt*, run `C:\fish-oot\fishmod\pc\setup.bat`. It makes the `fishoot` environment
   and downloads the latest ares into `fishmod\pc\ares\`. If that download fails, get ares from
   https://ares-emu.net/download and unzip it there.
4. **Your ROM:** Ocarina of Time NTSC 1.2 (US), in any byte order (.z64, .n64 or .v64).

## Running everything

Put the packs I send (.zip) in `fishmod\pc\packs\`. Then, in the Anaconda Prompt:

    C:\fish-oot\fishmod\pc\run_all.bat C:\roms\oot.z64

It runs the self-test first. If the game doesn't run there, it stops and says why. Otherwise it runs every other pack,
then prints one file to send back: `fishmod\pc\results\results_all.zip`.

## Running one pack

    conda activate fishoot
    cd C:\fish-oot
    python fishmod\pc\run_jobs.py fishmod\pc\packs\selftest.zip --rom C:\roms\oot.z64

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

## The LOD tuning pack: `lodtune3`

It runs 6 ares at once, each pinned to its own core, with the first two cores left to Windows. Each instance runs
as fast as it can: no sound, no frame limit. It looks at Lake Hylia from 3,396 views (above and under water, looking
down, level and up), each at dawn, noon, dusk and midnight. For every plant kind it finds the nearest distance each
of three things can start at without the picture changing: where the kind stops being drawn, where it becomes a
single card, and where it switches to the half-size texture.

- **Run it:** put `lodtune3.zip` in `fishmod\pc\packs\` and run `run_all.bat` as usual. Each instance runs up to
  8 hours. Running it again carries on where it stopped, so if it isn't finished, run it again.
- **How many at once:** the pack asks for 6; `--parallel N` overrides it. `--keep-cores` sets how many
  of the first cores are left alone.
- **Send back** `results_all.zip`. It holds one save per part. Partial results are still useful.

## The capture card viewer (`capture.bat`)

The console's picture and sound from a capture card, with as little delay as possible: the video is read on its own
thread keeping only the newest frame, and the sound goes from the card's input to the speakers in small blocks
(WASAPI). `capture.bat --list` shows the devices, `capture.bat --levels` how loud each sound input is right now (the
card's picture open meanwhile: some cards only send sound while their video is captured); `capture.bat --video 1 --audio "part of its name"` picks them (the
choice is remembered in `capture.json`). The first run installs OpenCV and sounddevice into the environment.

Keys in the window: **S** or **F12** a screenshot to the clipboard (also saved in `fishmod/pc/captures/`), **A** the
last 10 seconds of sound as a .wav with a screenshot, **F** or **F11** full screen, **M** sound off/on, **L** the
frame rate and delay, **Q** quit. **Ctrl+Alt+S** takes a screenshot to the clipboard from anywhere.

Other programs on the same PC can read it at `http://127.0.0.1:8765/frame.jpg` (the newest picture), `/frame.png`,
`/audio.wav?s=5` (the last few seconds of sound) and `/` (a page showing the picture).
