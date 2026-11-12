# Patches

Apply a patch to an **Ocarina of Time NTSC 1.2 (US)** `.z64` ROM (MD5 `57a9719ad547c516342e1a15d5c28c3d`) with
[Flips](https://github.com/Alcaro/Flips) or the [online patcher](https://www.marcrobledo.com/RomPatcher.js/). See the
main [README](../README.md#controls) for controls.

Only the current patches are kept here. Published playtests (kokiri-test1 to kokiri-test4, v0.1.1) are on the
[releases page](https://github.com/RastrumQuill/fish-oot/releases); every earlier bench and test build's notes and
results are in [history/](history/), and its patch is in the repository's history.

| Patch | What it is | Notes |
|---|---|---|
| `fish-oot-qol5-testmode.bps`, `fish-oot-qol5-hw-testmode.bps` | The newest test build to play: clear a C button by pressing it again; the D-pad hides when empty; the cheaper fish update | [notes](notes-qol5.md) |
| `fish-oot-rdpbench1.bps` | The RDP calibration bench (console timing of controlled drawing, for the RDP cost model) | [notes](notes-rdpbench1.md) |
| `fish-oot-frames7.bps`, `fish-oot-frames7-noclaim.bps` | The bench route with the audio work, and the same without the command-list cache claiming | [notes](notes-frames7.md) |
| `fish-oot-frames8.bps` | frames7 plus run-ahead, the collision record and the props' screen edges | [notes](notes-frames8.md) |

The bench patches are run by `fishmod\pc\morning.bat` (see `fishmod/pc/MORNING.md`).

`-testmode` patches: a **new file** skips the intro and starts with testing items (existing files aren't changed).

To publish a release, add `fish-oot-<tag>.bps` (and any `fish-oot-<tag>-*.bps`) with `notes-<tag>.md` here and run the
Release workflow with that tag. When a patch is superseded, delete it and move its notes to `history/`.
