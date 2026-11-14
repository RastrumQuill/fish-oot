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
| `fish-oot-frames13r.bps`, `fish-oot-frames14c.bps` | the bench's random numbers reseeded every frame (frames14's legs 3 and 4 had drawn another scene: a Tektite meeting Link elsewhere); 13r: frames13's drawing, the control; 14c: the lake terrain cut by texture (each texture loaded once, as the original), the rig cleaned | [notes](notes-frames14.md) |
| `fish-oot-frames15.bps`, `fish-oot-frames15L.bps` | the fish drawn flat past 200 units (one cycle, unlit, the fog per fish in the combiner; billboards too), memmove a word at a time; 15L: the same code with every fish lit, the control | [notes](notes-frames15.md) |
| `fish-oot-frames16.bps` | frames15 with the fish update's code compact (its time no longer depends on where the code lands in the instruction cache: frames15's regression) | [notes](notes-frames16.md) |
| `fish-oot-frames14.bps` | frames13 with the fish rig's lists cleaned, the lake terrain cut into areas the RSP culls and lit by the CPU | [notes](notes-frames14.md) |
| `fish-oot-rsp-ab.bps` | The RSP test: frames13's bench for 4 laps (about 16 minutes), the audio's RSP work on, off, on, off | (rsp_ab script) |
| `fish-oot-frames13.bps`, `-prof`, `-audio2`, `-reverb2`, `-27k` | frames12 without the wall clearance bound (frames11's CPU slowdown, found by the bisection), and two audio memory changes; -prof: F3DEX3's profiling counters (where the RSP's time goes); -audio2: the audio mixed every two refreshes (half the RSP audio tasks); -reverb2: every reverb at half rate or less; -27k: the sound mixed at 26.8 kHz (both change the sound: RSP tests) | [notes](notes-frames13.md) |

The bench patches are run by `fishmod\pc\morning.bat` (see `fishmod/pc/MORNING.md`).

`-testmode` patches: a **new file** skips the intro and starts with testing items (existing files aren't changed).

To publish a release, add `fish-oot-<tag>.bps` (and any `fish-oot-<tag>-*.bps`) with `notes-<tag>.md` here and run the
Release workflow with that tag. When a patch is superseded, delete it and move its notes to `history/`.
