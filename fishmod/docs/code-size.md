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
path that calls them).

**The mod's own code**

| what | size | how often | time | fills' share | reading |
|---|---:|---:|---:|---:|---|
| `EnFishmod_FishAI` (one fish thinking) | 10.9 KB routine, 3.7 KB runs per call across 15 routines | 9.6 a frame | 0.70 ms | 72% | The clearest case. About 100 line fills per call: nearly all of its 3.7 KB is fetched again every time, because between two fish the actor framework, collision and the other fish's update run. Most of its time is fetching, not thinking. `EnFishmod_SwimTowardImpl` inside it is the same (87%). |
| `EnFishmod_Update` (every fish, every frame) | 2.8 KB per call, 11 routines | 49.6 a frame | 1.92 ms | 18% | Over the per-item line but mostly staying cached: the far-follower path (`FollowFar`, 1 KB) is the bulk and runs back to back. Not urgent. |
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

**The engine**

| what | size | how often | fills' share | reading |
|---|---:|---:|---:|---|
| `BgCheck_RaycastDownImpl` (floor under an actor) | 3.3 KB per call, 2 routines | 14.7 a frame | 36% | One routine of 3.1 KB (the answer caches and the list walk inlined together). A per-item path worth splitting: cache hit as a small leaf, the walk behind it. |
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
