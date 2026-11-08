# Patches

Every fish-oot patch lives here. Apply one to an **Ocarina of Time NTSC 1.2 (US)** `.z64` ROM
(MD5 `57a9719ad547c516342e1a15d5c28c3d`) with [Flips](https://github.com/Alcaro/Flips) or the
[online patcher](https://www.marcrobledo.com/RomPatcher.js/). The newest is also on the
[releases page](https://github.com/RastrumQuill/fish-oot/releases). See the main [README](../README.md#controls) for
controls.

| Release | Patches | Notes |
|---|---|---|
| fourway1 (bench) | `fish-oot-fourway1-hw.bps` | [notes](notes-fourway1.md): occlusion plane x bed cards, console |
| fourway2 (bench) | `fish-oot-fourway2-hw.bps` | [notes](notes-fourway2.md): fourway1 plus a code guard for its console crash |
| qol1 (test build) | `fish-oot-qol1-testmode.bps`, `-hw-testmode` | [notes](notes-qol1.md): D-pad item buttons, equipment on buttons, hold B for text, strength speeds up pushing and casting |
| qol2 (test build) | `fish-oot-qol2-testmode.bps`, `-hw-testmode` | [notes](notes-qol2.md): qol1 with the shaded D-pad and ammo counts |
| qol3 (test build) | `fish-oot-qol3-testmode.bps`, `-hw-testmode` | [notes](notes-qol3.md): qol2 plus a day clock under the minimap |
| pcprof2 (profile) | `fish-oot-pcprof2-hw.bps` | [notes](notes-pcprof2.md): the compiler profile run again, on the memset and matrix fixes |
| qol4 (test build) | `fish-oot-qol4-testmode.bps`, `-hw-testmode` | [notes](notes-qol4.md): the D-pad always shown; memset and matrix speedups; occlusion plane and bed cards off |
| fishopt1 (bench) | `fish-oot-fishopt1-hw.bps` | [notes](notes-fishopt1.md): the play bench on the cheaper fish update (and the memset and matrix fixes) |
| qol5 (test build) | `fish-oot-qol5-testmode.bps`, `-hw-testmode` | [notes](notes-qol5.md): clear a C button by pressing it again; the D-pad hides when empty; the cheaper fish update |
| vrtime1 (timing tests) | `fish-oot-vrtime1-hw.bps` | [notes](notes-vrtime1.md): the VR4300 timing tests, console |
| pcprof1 (profile build) | `fish-oot-pcprof1-hw.bps` | [notes](notes-pcprof1.md): the PC-sampling compiler profile, console |
| refactor1 (test build) | `fish-oot-refactor1.bps`, `-hw`, `-testmode`, `-hw-testmode` | [notes](notes-refactor1.md): the refactored game; the rod in any scene, saves with a version mark |
| lake-tour1 (feedback build) | `fish-oot-lake-tour1.bps` | [notes](notes-lake-tour1.md): a camera tour of Lake Hylia's habitat props, above and below the water |
| kokiri-test4 (newest) | `fish-oot-kokiri-test4.bps`, `-testmode`, `-animstudio` | [notes](notes-kokiri-test4.md): faster fish (fish manager, rigs), fish that stay for the day, animation studio |
| kokiri-test3.1 | `fish-oot-kokiri-test3.1.bps`, `-testmode` | [notes](notes-kokiri-test3.1.md): bait bottles, fixes |
| kokiri-test3 | `fish-oot-kokiri-test3.bps`, `-testmode` | [notes](notes-kokiri-test3.md) |
| kokiri-test2.1 | `fish-oot-kokiri-test2.1.bps`, `-testmode` | [notes](notes-kokiri-test2.1.md) |
| kokiri-test2 | `fish-oot-kokiri-test2.bps`, `-testmode` | [notes](notes-kokiri-test2.md) |
| kokiri-test1 | `fish-oot-kokiri-test1.bps` | First playtest: fishing in Kokiri Forest (6 species), charge casting, nibbles and bites, tension fights, keep-in-bottle or release, catch records |
| v0.1.1 | `fish-oot-v0.1.1.bps` | [notes](notes-v0.1.1.md) |
| perf A / B | `fish-oot-perf-A.bps`, `fish-oot-perf-B.bps` | [notes](notes-perf-builds.md): the first optimization comparison builds (not releases) |

`-testmode` patches: a **new file** skips the intro and starts with testing items (existing files aren't changed).

To publish a release, add `fish-oot-<tag>.bps` (and any `fish-oot-<tag>-*.bps`) with `notes-<tag>.md` here and run the
Release workflow with that tag.
