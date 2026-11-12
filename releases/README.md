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
| `fish-oot-stress2.bps` | The fish and plant stress benches on the current code, with the profile (how many fish and plants hold 20 fps, and what costs the most at each step) | [notes](notes-stress2.md) |
| `fish-oot-frames10.bps` | The bench route: Guays and Tektites check collision less while idle and far, the fish manager's travel without raycasts | [notes](notes-frames10.md) |
| `fish-oot-frames9.bps` | The bench route with the first fish AI work (cheaper far schools, no repeated shore raycasts) | [notes](notes-frames9.md) |
| `fish-oot-frames8b.bps` | The bench route, the baseline before the fish AI work | [notes](notes-frames8b.md) |

The bench patches are run by `fishmod\pc\morning.bat` (see `fishmod/pc/MORNING.md`).

`-testmode` patches: a **new file** skips the intro and starts with testing items (existing files aren't changed).

To publish a release, add `fish-oot-<tag>.bps` (and any `fish-oot-<tag>-*.bps`) with `notes-<tag>.md` here and run the
Release workflow with that tag. When a patch is superseded, delete it and move its notes to `history/`.
