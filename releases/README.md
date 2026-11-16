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
| `fish-oot-frames17.bps` | frames16 with the north-beach dock rebuilt (twisted-pentagon posts cut to the lake bed, back faces culled: the dock's RDP time down by about two thirds) | [notes](notes-frames17.md) |
| `fish-oot-frames18.bps` | frames17 with the code and data pinned in the CPU's caches (mod code last in the segment, each section and the heap on a fixed boundary, the fish overlay and instances fixed sizes): edits no longer move the engine's cache placement | [notes](notes-frames18.md) |
| `fish-oot-prop-sweep.bps` | about 5.5 hours: every other lap shipped, between them one plant kind left out, drawn whole, or with its cut split low (21 tests) | [notes](notes-prop-sweep.md) |
| `fish-oot-frames19.bps` | frames18 with bush, tall and ribbon weed drawn with whole cards up close (the prop sweep: the cut cost more for these) | [notes](notes-frames19.md) |
| `fish-oot-frames20.bps` | frames19 with the caustics faded with depth and colourless, the rocks made for where they lie, plants making way for Link | [notes](notes-frames20.md) |
| `fish-oot-frames21.bps` | frames20 with the shore grass in colour again and a cheaper touch check | [notes](notes-frames21.md) |
| `fish-oot-terrain-sweep.bps` | 18 laps: the terrain anti-aliased without reading back, the sky drawn last, both, against shipped | [notes](notes-terrain-sweep.md) |
| `fish-oot-frames22.bps` | frames21 with texpack game-wide (CI4, narrower intensity formats, halved repeats and mirrors), ripples loaded once, plants that bend | [notes](notes-frames22.md) |
| `fish-oot-frames23.bps` | frames22 with the terrain's RA, depth darkening, water fog from above, floating plants that move | [notes](notes-frames23.md) |
| `fish-oot-frames24.bps` | frames23 with the water fog's dark band and pillars fixed (the fog in the vertex colours, the haze left on), cheaper; lily pads and reeds fixed | [notes](notes-frames24.md) |
| `fish-oot-frames25.bps` | frames24 with the bend-plant crash fixed, plants bending as if held up by the water, no depth darkening | [notes](notes-frames25.md) |
| `fish-oot-frames26.bps` | frames25 with the water fog from above being the underwater fog, by the ray's length in the water (bed and props, vertex by vertex), nothing poking out of the water, reeds bending | [notes](notes-frames26.md) |
| `fish-oot-frames27.bps` | frames26 with the water fog from above in the RSP's own fog (the bed in depth bands, each prop by its depth; no CPU fog), plants bending as ropes, the lens working on the D-pad | [notes](notes-frames27.md) |
| `fish-oot-frames28.bps` | frames27 with plants no longer left out between the camera and Link | [notes](notes-frames28.md) |
| `fish-oot-frames28-nobands.bps` | frames28 without the lake bed's water-fog bands (the props' fog still on): what the bands cost | [notes](notes-frames28-nobands.md) |
| `fish-oot-frames28-audiocol.bps`, `-nocut.bps`, `-bands4.bps`, `-all.bps` | frames28 with the audio and collision leads' work; the bed's fog bands uncut; cut at 4 depths; all of them | [notes](notes-frames28-set.md) |
| `fish-oot-audio-laps.bps`, `-every2.bps` | the audio lead's laps bench (claude/sleepy-faraday-ve9g2u, their patches): the audio driver woken every refresh against every second one, three laps each | [notes](notes-audio-laps.md) |
| `fish-oot-rc1.bps`, `-test.bps`, `-bench.bps` | **rc1, the release candidate**: frames28 with the bed's water fog uncut, the collision and audio leads' work merged; to play, test mode, and the bench later builds are measured against | [notes](notes-rc1.md) |
| `fish-oot-rc1-popin-test.bps` | rc1's test build with the far single cards the right size (they were a few units across: plants popped in near the camera, under water nearer than Link) | [notes](notes-rc1-popin.md) |
| (branch `claude/rc1-headroom`, no patch yet) | rc1 with the far cards' fix, the props' and fish's CPU work brought onto it, and the pop-in test (`--script pop_ab`): its first report is `pop-ab-rc1-headroom.md` | [notes](notes-rc1-headroom.md) |
| `fish-oot-frames20-headroom.bps` | the props' and fish's CPU work (claude/fish-prop-headroom, their patch) on frames20-colnew's tree, to compare with colnew | [notes](notes-frames20-headroom.md) |
| `fish-oot-audio-ab.bps` | the audio lead's A/B (claude/sleepy-faraday-ve9g2u, their patch): 6 laps, the audio whole / without its RSP tasks / without the driver, twice | [notes](notes-audio-ab.md) |
| `fish-oot-frames20-colbase.bps`, `-colnew.bps` | frames20's collision and the collision work's (claude/collision-opt), the same tree and toolchain, to compare on the console with the Expansion Pak in | [notes](notes-frames20-col.md) |
| `fish-oot-anatomy-sweep.bps` | about 3.3 hours: every other lap shipped, between them the terrain, Link, the other actors, the effects, or everything left out; and the RDP's idle time | [notes](notes-anatomy-sweep.md) |
| `fish-oot-frames14.bps` | frames13 with the fish rig's lists cleaned, the lake terrain cut into areas the RSP culls and lit by the CPU | [notes](notes-frames14.md) |
| `fish-oot-rsp-ab.bps` | The RSP test: frames13's bench for 4 laps (about 16 minutes), the audio's RSP work on, off, on, off | (rsp_ab script) |
| `fish-oot-frames13.bps`, `-prof`, `-audio2`, `-reverb2`, `-27k` | frames12 without the wall clearance bound (frames11's CPU slowdown, found by the bisection), and two audio memory changes; -prof: F3DEX3's profiling counters (where the RSP's time goes); -audio2: the audio mixed every two refreshes (half the RSP audio tasks); -reverb2: every reverb at half rate or less; -27k: the sound mixed at 26.8 kHz (both change the sound: RSP tests) | [notes](notes-frames13.md) |

The bench patches are run by `fishmod\pc\morning.bat` (see `fishmod/pc/MORNING.md`).

`-testmode` patches: a **new file** skips the intro and starts with testing items (existing files aren't changed).

To publish a release, add `fish-oot-<tag>.bps` (and any `fish-oot-<tag>-*.bps`) with `notes-<tag>.md` here and run the
Release workflow with that tag. When a patch is superseded, delete it and move its notes to `history/`.
