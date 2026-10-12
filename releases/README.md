# Patches

Every fish-oot patch lives here. Apply one to an **Ocarina of Time NTSC 1.2 (US)** `.z64` ROM
(MD5 `57a9719ad547c516342e1a15d5c28c3d`) with [Flips](https://github.com/Alcaro/Flips) or the
[online patcher](https://www.marcrobledo.com/RomPatcher.js/). The newest is also on the
[releases page](https://github.com/RastrumQuill/fish-oot/releases). See the main [README](../README.md#controls) for
controls.

| Release | Patches | Notes |
|---|---|---|
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
