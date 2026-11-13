# Patches

Apply a patch to an **Ocarina of Time NTSC 1.2 (US)** `.z64` ROM (MD5 `57a9719ad547c516342e1a15d5c28c3d`) with
[Flips](https://github.com/Alcaro/Flips) or the [online patcher](https://www.marcrobledo.com/RomPatcher.js/). See the
main [README](../README.md#controls) for controls.

Only the current patches are kept here. Published playtests (kokiri-test1 to kokiri-test4, v0.1.1) are on the
[releases page](https://github.com/RastrumQuill/fish-oot/releases); every earlier bench and test build's notes and
results are in [history/](history/), and its patch is in the repository's history.

| Patch | What it is | Notes |
|---|---|---|
| `fish-oot-qol7-testmode.bps`, `fish-oot-qol7-hw-testmode.bps` | The newest test build to play: the fish's new behaviour (schools, days, scent, learned baits) | [notes](notes-qol7.md) |
| `fish-oot-stress3.bps` | The fish and plant stress benches judged by the screen (how many really hold 20 fps), on frames10's code | [notes](notes-stress3.md) |
| `fish-oot-frames12.bps` | The bench route with less waiting on memory (actor lists and categories, collision lists, audio channels, the fish's fields, the fish manager) | [notes](notes-frames12.md) |
| `fish-oot-rsp-ab.bps` | The all-day RSP test: frames13's bench lap after lap, the audio's RSP work off for every other 8 laps | (rsp_ab script) |
| `fish-oot-frames13.bps`, `fish-oot-frames13-audio2.bps` | frames12 without the wall clearance bound (frames11's CPU slowdown, found by the bisection), and two audio memory changes | [notes](notes-frames13.md) |

The bench patches are run by `fishmod\pc\morning.bat` (see `fishmod/pc/MORNING.md`).

`-testmode` patches: a **new file** skips the intro and starts with testing items (existing files aren't changed).

To publish a release, add `fish-oot-<tag>.bps` (and any `fish-oot-<tag>-*.bps`) with `notes-<tag>.md` here and run the
Release workflow with that tag. When a patch is superseded, delete it and move its notes to `history/`.
