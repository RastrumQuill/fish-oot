# Code size against the instruction cache

The VR4300's instruction cache is 16 KB, direct mapped, 32-byte lines. A line fetched from RAM costs about 48 cycles.
Code that's bigger than it needs to be on a path that runs often doesn't just cost its own instructions: it pushes
other hot code out, and is pushed out itself, so each pass pays to fetch it again.

Three searches, cheapest first:

| tool | reads | says |
|---|---|---|
| `fishmod/tools/codescan.py --elf ELF` | the built code only | innermost loops and routines by the size of what they call, and code in one loop's path that shares cache lines. Candidates: size alone isn't a fault |
| `fishmod/tools/hotcode.py PROFILE --elf ELF --frames N` | ares's exact profile | the same question measured: what runs per call or per item, how often, and how many code fills it took |
| reading | the candidates | which are real, and what to do |

The lines (filters, not targets): 4 KB of code per call for a routine with a real share of the frame; 2 KB for
anything that runs once per item (fish, actor, prop, collision polygon, note).

## First reading: Lake Hylia (17 Nov, `releases/profiles/hot-lake.md`)

From the play bench's first 3,850 frames under ares's exact profiler. "Fills' share" is the path's instruction-cache
fills at 48 cycles each against its time; it's a ceiling where helpers are shared (the trig routines count in every
path that calls them). The readings below are from these numbers and the routines' shapes in the reports; the
source of each hasn't been gone through line by line yet, so the fixes are proposals to check, not findings.

**The mod's own code**

| what | size | how often | time | fills' share | reading |
|---|---:|---:|---:|---:|---|
| `EnFishmod_FishAI` (one fish thinking) | 10.9 KB routine, 3.7 KB runs per call across 15 routines | 9.6 a frame | 0.70 ms | 72% | The clearest case. About 100 line fills per call: nearly all of its 3.7 KB is fetched again every time, because between two fish the actor framework, collision and the other fish's update run. Most of its time is fetching, not thinking. `EnFishmod_SwimTowardImpl` inside it is the same (87%). |
| `EnFishmod_Update` (every fish, every frame) | 2.8 KB per call, 11 routines | 49.6 a frame | 1.92 ms | 18% | Over the per-item line but mostly staying cached: `FollowFar` (1 KB) is the biggest piece of it. Not urgent. |
| `EnFishmod_ScatterDraw` (props, drawing) | 14.4 KB routine; 8.3 KB runs per call; the per-prop loop 1.7 KB + trig | once a frame, 25.6 loop passes | 1.67 ms | 35% (the loop: 41%) | Nearly the whole cache for one routine. The loop itself is small enough; the cost is the 6.5 KB around it that runs once and evicts what the frame needs next, and is evicted itself by next frame. |
| `Scatter_Build` (props, choosing the set) | 10.0 KB routine; 7.7 KB runs per call; the loop's body 4.4 KB | 0.2 calls a frame, 14.5 passes | 0.67 ms | 56% | The loop body alone is over twice the per-item line. Runs one frame in five since the reuse work, so its fetch cost is paid in full each time. |
| the mod's whole draw (`EnFishmod_ManagerDraw` and below) | 23.6 KB across 40 routines | once a frame | 3.03 ms | 31% | Bigger than the cache by itself: props, rigs, billboards, particles and fog each push the others out within one frame. |

What these point at, in the order I'd try:

1. **Fish AI**: run the fish that think this frame back to back (a pass in the manager, as the far followers
   already do) instead of one per actor update, so the 3.7 KB is fetched once a frame, not ten times. No rewrite
   needed; the win is the 72%.
2. **Props draw**: split `EnFishmod_ScatterDraw` into the once-a-frame set-up and the per-prop loop as separate
   routines, with the per-kind and rare cases (touch, bend rebuild, debug counters) out of line. The aim is the
   8.3 KB that runs per call, not the 14.4 KB total.
3. **Props build**: the 4.4 KB loop body is the candidate for the tighter rewrite (or assembly): one prop's test
   and record, with tier selection by table rather than inline branches.
4. **Draw order**: the 23.6 KB of draw code runs as one interleaved pass; grouped by kind (all rigs, then all
   billboards, then props) each piece is fetched once.

**A list of the mod's five biggest routines, against these measurements**

The list (sizes from a build where `Scatter_Touch` is its own routine; in the GCC bench build it's inlined into
`EnFishmod_ScatterDraw`, which is 14.4 KB there against the list's 17.9 KB):

| routine (list's size) | measured here | agrees? |
|---|---|---|
| `EnFishmod_ScatterDraw` (17.9 KB, "can't stay in cache by itself") | 8.3 KB of it runs on a call; 35% of its time is code fills | yes; the part to shrink is what runs, 8.3 KB |
| `EnFishmod_FishAI` (10.9 KB, "run dozens of times a frame") | 3.7 KB runs per call, 9.6 calls a frame, 72% of its time is code fills | yes, and it's the worst by share; per call it's 3.7 KB, not 10.9, so running the fish back to back is enough to fix most of it |
| `EnFishmod_RigDraw` (4.5 KB, "hot path is small") | 5.5 KB with its helpers, 2.9 calls a frame, 0.50 ms, 26% code fills; its inner loop is 0.1 KB | yes: lower priority |
| `Scatter_Touch` (3.3 KB, per touched plant) | not measured: inlined here, and the lake route's first laps touch few plants | open: needs a route that walks through the reeds |
| `EnFishmod_ManagerUpdate` (3.3 KB, "loop body could shrink a lot") | 2.0 KB runs per call, its loop 0.3 KB at 15.9 passes, 0.22 ms, 18% code fills | the loop is already small in this build; little to gain |

These were measured on the bench build with the sampler running, which inflates code fills (see input-replay.md,
"Measure with the clean build"): the ranking should hold, the shares will come down. To be redone on a clean build.

**What compiling for size saves (17 Nov)**

The same ROM (`bench_clean --no-timers`) built three ways with GCC: everything at `-O2`; the mod's own files at `-Os`
(`--mod-opt Os`); the whole game at `-Os` (`--gcc Os`).

Whole code, as linked:

| | `-O2` | `-Os` | saved |
|---|---:|---:|---:|
| the mod's overlay (`ovl_En_Fishmod`) | 139,456 | 124,000 | 11% |
| the engine (`code`) | 962,336 | 848,256 | 12% |
| Link (`ovl_player_actor`) | 175,136 | 128,656 | 27% |
| the pause menu | 84,480 | 78,352 | 7% |

Routine by routine in the mod it's mostly 10-20% (`EnFishmod_RigDraw` 4,184 to 3,308, `EnFishmod_ManagerUpdate` 3,304
to 2,704, `EnFishmod_FishAI` with its swim routine 11,872 to 10,764). The props' family changes shape (at `-Os` the
build routine is inlined into the draw and the touch code is split out) and comes to 24,468 against 21,976: 10%.

What runs per call, from ares's exact profile of both over the bench's first three legs:

| path | `-O2` | mod at `-Os` | saved |
|---|---:|---:|---:|
| the mod's whole draw (`EnFishmod_ManagerDraw` and below) | 33.4 KB | 31.9 KB | 4% |
| `EnFishmod_ScatterDraw`, a call | 21.4 KB | 20.6 KB | 4% |
| `EnFishmod_RigDraw`, a call | 5.2 KB | 4.9 KB | 6% |
| `EnFishmod_FishAI`, a call | 4.2 KB | 4.1 KB | 2% |
| `EnFishmod_Update`, a call | 2.5 KB | 2.4 KB | 4% |
| `EnFishmod_SwimToward`, a call | 2.1 KB | 2.1 KB | 0% |
| `EnFishmod_ManagerUpdate`, a call | 2.0 KB | 2.1 KB | none |

So the flag takes a tenth off the mod on disk and 2-6% off the paths that matter. What `-Os` removes is mostly
what wasn't running: copies made by inlining, padding, the cold ends of routines. The instructions on the hot paths
are nearly the same ones. Nothing here moves a path across the 2 KB or 4 KB lines. (These paths are bigger than the
lake table's above: this route walks through plants, so the touch code runs, and the timers are compiled out.)
The whole game at `-Os` wasn't profiled; clang at `-Os` doesn't build this tree (an inline `cache` instruction in
fishmod.c it rejects).

**Where the instruction cache's cost actually is (17 Nov, the clean `-O2` build, the bench's first three legs)**

| | share of the CPU's working time |
|---|---:|
| all code fills, the whole game | 25.1% |
| all data fills | 25.6% |
| the mod's code, all of it | 16.1% |
| the mod's code fills | 3.6% (23% of the mod's own time) |
| the mod's data fills | 3.8% |

The mod's code fills are 3.6% of the frame. That is the most that any amount of shrinking, splitting or assembly
in the mod's five routines can give back, and nothing gives back all of it. The other 21.5% of code fills is the
engine's: Link, collision, the actor framework, the camera, the OS.

Two things tried against the mod's share:

- `hotcode.py --map ROUTINE` (new) lists a routine's running bytes by source line. `EnFishmod_ScatterDraw`'s 9.0 KB
  and `Scatter_Build`'s 7.7 KB are spread thin: the biggest single pieces are the touch code (1.7 KB + 1.0 KB) and the
  sway (0.5 KB), and the rest is hundreds of lines of under 100 bytes each. There's no block to lift out; shrinking
  them means rewriting them.
- The fish's thinking run back to back after all actors (instead of one per actor update): its code fills fell 23%
  and its time 7%, about 0.03 ms a frame. The "72% of its time is fetching" above was a ceiling (a path's routines'
  fills from every caller), and the real figure is far lower. Not kept: too little for changing the order fish
  update in.

The whole game at `-Os` (`--gcc Os --gcc-all`) doesn't boot (it never starts the graphics thread; libultra at `-Os`
is the known cause on clang), so the engine's side is still unmeasured. That's the next thing to measure: the game's
C at `-Os` with libultra left at `-O2`, then code layout (hot routines together, cold apart).

**The engine**

| what | size | how often | fills' share | reading |
|---|---:|---:|---:|---|
| `BgCheck_RaycastDownImpl` (floor under an actor) | 3.3 KB per call, 2 routines | 14.7 a frame | 36% | One routine of 3.1 KB running per call. A per-item path worth looking at for a split: the common answer as a small leaf, the rest behind it. |
| `Actor_Draw` | 2.1 KB, 11 routines | 11.8 a frame | 55% | Small, but between two calls a whole actor's draw runs, so it's refetched each time. Inherent to the framework's shape; shrinking helps a little. |
| `Actor_UpdateBgCheckInfo` | 14.6 KB across 36 routines | 5.9 a frame | 64% | Wall, floor, ceiling and water checks in one call: almost the cache. The vanilla structure; the collision work already took its data misses, this is what's left. |
| `AudioSynth_ProcessNote` | 2.1 KB | 146 a frame | 12% | Over the line but runs back to back and stays cached: fine as it is. |
| Link (`Player_UpdateCommon` 25.6 KB, `Player_ProcessSceneCollision` 17.6 KB), the camera (10-16 KB) | | once a frame | ~100% | Each is fetched from RAM every frame in full. Once-a-frame code this size can't stay cached; only smaller hot paths (cold parts out of line, `-Os` for these files) would change it. |

## The static scan

`codescan.py` over the same build lists 1,536 innermost loops and routines over the lines among code a frame can
reach, most of them in actors (917) and Link (249): vanilla per-actor code whose loops usually run a handful of
times. In the engine's core and the mod it's 208 rows over 88 routines (`releases/profiles/scan-core-static.md`).
Read against the profile above, the static list is right about the mod (`EnFishmod_FishAI`'s 4.2 KB loop,
`Scatter_Build`'s 3.2 KB loop, `EnFishmod_RodUpdate`'s 2.5 KB loop come out near the top) and wrong by omission
about frequency: it can't tell `Scatter_Build` (14 passes) from `Message_DrawMain` (rarely anything). Its own
contribution is the conflict column: in `EnFishmod_FishAI`'s loop, 3.2 KB of the path lands on cache lines another
part of the same path uses, a layout fault that costs every pass whatever else is running, and one a link-order
change fixes. That's worth a pass of its own over the 88.

Not yet read: the rod (`EnFishmod_RodUpdate`, only while fishing: the lake route's first laps don't cast), and every
scene but the lake (the every-scene run is the input for that).
