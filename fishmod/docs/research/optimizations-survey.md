# Survey: optimizations from other N64 projects that we don't use yet

Part of the fishmod research (see [README.md](README.md)). Written after frames10 / stress2, for the next round of
CPU work at Lake Hylia (and the RSP/RDP after it).

**What this covers:** techniques from fan mods (HackerSM64, HackerOoT, Kaze's SM64 work), homebrew (libdragon,
tiny3d), the F3DEX3 microcode, Majora's Mask (zeldaret/mm) and commercial N64 games. Each is checked against what
this project already does ([n64.md](n64.md), [opt-plan-hw.md](../history/opt-plan-hw.md), the roadmap, the git log),
so nothing here is a repeat of done work.

**How sure each claim is:**
- **[primary]**: read in the source itself (the F3DEX3 microcode and docs, the tiny3d docs, the HackerSM64 and
  HackerOoT code, the MM decomp, Nintendo's own developer docs).
- **[secondary]**: from an article, interview or summary, not checked against code.
- **[ours]**: our own estimate from the numbers above and the source. Not measured: each one needs a console A/B
  before it stays, as always.

**The numbers this starts from** (console, Lake Hylia, ms of a 50 ms frame): audio CPU ~12, matrix/math ~7,
collision ~6-7 (Guay 2.1, Tektite 1.6, Link 0.8; 0.35-0.7 per call), fish update ~5-6, camera ~2.8, prop drawing
~2.4; RSP 28-39, RDP 40-50. Tank stress: +0.5 ms RSP and +0.3 ms RDP per extra on-screen skinned fish (~53
vertices, ~25 triangles).

## The shortlist (ranked for us)

| # | Technique | Cost it targets | Expected gain for us | Effort | Basis |
|---|---|---|---|---|---|
| 1 | **Fish rig display list as "load all vertices per bone, then all triangles"**: one matrix + one vertex load per bone, triangles grouped by tint part | RSP per fish | ~30-40% of each fish's RSP command and matrix overhead [ours]; per-fish RSP maybe 0.5 to 0.3-0.35 ms | medium (build time only, in `Rig_BuildBase`) | OoT's own flex skeletons, tiny3d's skinned parts, F3DEX3's matrix path [primary] |
| 2 | **Audio output at 22,050 Hz**: 2 synthesis ticks per retrace instead of 3 | audio CPU + audio RSP | up to ~1/3 of the per-tick audio work, maybe 2-4 ms CPU [ours] | trivial to try (one table) | Nintendo's audio docs [primary]; our `heap.c` formula. **Audible:** needs a listening A/B |
| 3 | **Audio in bigger, rarer frames**: synthesize on every 3rd (or 2nd) retrace with 3x (2x) buffers | audio CPU per-frame overhead, cache thrash from the 60 Hz audio thread | unknown; 0.5-2 ms [ours] | low-medium | Nintendo audio tuning guide: "double all of the buffers" at ≤30 fps [primary] |
| 4 | **Clearance bound for wall and ceiling checks**: one full check records how far the nearest poly is; until the actor has moved that far, the answer is "no wall" without a search | collision CPU (Guay, Tektite, Link, camera) | most of the wall part of 3.7 ms of Guay + Tektite; 1.5-3 ms [ours] | medium (exact answers: the collision log checks it) | standard physics technique (conservative advancement); HackerSM64 trims `find_floor` calls the same way [primary, as an analogue] |
| 5 | **Claim cache lines for all display-list writes**, not just the matrices and the audio command list | CPU (RDRAM reads on write misses), RDRAM traffic shared with the RDP | 0.5-1 ms CPU [ours]; the audio version measured ~1.5% of the CPU | medium | VR4300 Create Dirty Exclusive; our own frames7 result; Kaze: CPU memory traffic slows rendering [secondary] |
| 6 | **Fish in the near props' edge mode** (`AA_EN` without `IM_RD`; the VI smooths edges) | RDP per fish | ~10% of fish RDP [ours, by analogy with props' +13% when reverted] | low (render mode + an edge A/B capture) | our hwbench-2/4 results; libultra's reduced-aliasing modes [primary] |
| 7 | **A mid-distance fish mesh LOD**: decimated mesh, fins merged, 2-3 bones, between the reduced rig and the billboard | RSP + RDP per mid fish | roughly halves a mid-distance fish's RSP and RDP [ours] | medium (offline tool; meshoptimizer) | tiny3d model pipeline, HackerSM64 `AUTO_LOD` [primary] |
| 8 | **Affine-only matrix maths**: skip the bottom row in rotations and multiplies; one fused rotate-translate-multiply per limb | matrix CPU (~7 ms) | 10-25% of it, ~0.7-1.5 ms [ours] | low-medium | HackerSM64 `mtxf_mul`, `mtxf_rotate_xyz_and_translate_and_mul` [primary] |
| 9 | **Fewer audio voices and cheaper reverb at the lake** (measure the peak first) | audio CPU + RSP | small CPU ("slightly", HackerSM64), more on the RSP | trivial | HackerSM64 `config_audio.h` [primary] |
| 10 | **Profile-guided code placement** for the 16 KB instruction cache: hot engine functions together, the audio driver's hot loop kept from colliding with them | CPU (I-cache misses) | unknown; Kaze calls cache misses the main cost [secondary] | medium (linker order from pcprof3) | Kaze interview [secondary]; HackerSM64 build flags [primary] |

Measure before building any of them. Items 1, 6 and 7 can be priced first with F3DEX3's profiling builds (`F3DEX3_PROF=A|C`
give matrix loads, vertex and triangle cycles, DMA and FIFO stalls) in the tank stress bench, which we already have.

## The RSP: many skinned fish

### 1. One matrix and one vertex load per bone, then the triangles [primary basis, ours for the gain]

**What we do now** (`fishmod_rig.c`, `Rig_BuildBase`): each limb's display list is copied in order. Where a vertex
load spans bones it's split, with a `G_MTX_LOAD` wherever the bone changes, and `curBone` starts over at every limb.
So a fish's list holds a matrix load per bone *run per limb*, many short vertex loads, and a call to the tint segment
per part change.

**What a matrix load costs in F3DEX3** (read in `f3dex3.s`):
- `G_MTX_handler` clears `mvpValid` *and* `dirLightsXfrmValid`.
- The next vertex load then runs `mtx_multiply` (a 4x4 fixed-point multiply, a 16-pass inner loop, about 150 cycles
  [ours, counted roughly]).
- The light directions are then transformed again: 92-93 cycles for 0-2 directional lights (`Performance.md`).
- That comes on top of the command dispatch (10 cycles) and the 64-byte matrix DMA.
- So each bone change is about **300 RSP cycles (~5 µs)** before a single vertex.

**Each vertex load** costs 17 cycles before its DMA starts, plus the DMA wait, plus constant setup, and works in
pairs (an odd count wastes half a pair). Each `gSPDisplayList` to the tint segment is a fresh DL fetch going in and
another coming back.

**What OoT, SM64 and tiny3d do instead:**
- OoT's own flex skeletons (the ones the rig replaces) load each limb's vertices under that limb's matrix into fixed
  buffer slots. Later triangles then reference vertices loaded under different matrices: that's what the model's
  segment 0x0D "seams" are.
- tiny3d's model pipeline (`docs/modelOpt.md`) builds skinned meshes the same way. A "part" is one vertex load plus
  every triangle it can draw, and skinned parts are split once per bone. Duplicated vertices are accepted so that
  "every part will only ever need one vertex-load command". Its README: "fake-blending with 1 bone per vertex / up to
  3 bones per triangle", the same as our rig.

**For us:**
- Sort the species' baked vertices by bone at build time.
- Emit `G_MTX` + one `G_VTX` per bone, each into its own slot range. The fish's ~53 vertices fit F3DEX3's 56;
  anything bigger splits into two parts.
- Then emit every triangle, grouped by tint part so each part's tint is set once, with indices remapped (`gSP2Triangles`).
- Matrix loads drop to one per bone (6-9) and vertex loads to one per bone. With them go most of the MVP and light
  re-transforms and the tint DL round trips.
- Changing fog or lighting between the vertex loads and the triangles would break this (F3DEX3's "obscure semantic
  differences"), but the tints only set colours.

**Gain [ours]:**
- With assumed counts of ~15 matrix loads, ~20 vertex loads and ~8 tint calls today against 7 / 7 / 3 after, a fish's
  RSP command overhead goes from ~13k to ~9k cycles.
- Vertex lighting (77 cycles a pair) and triangles (149 each) are unchanged.
- Count the real numbers first: log `G_MTX` and `G_VTX` per `base->dl` at build, and read prof C's matrix-load
  counter in the tank.
- If the measured 0.5 ms per fish is mostly FIFO stalls (prof A), the RDP is the real limit and this saves less.

### 7. A mid-distance mesh LOD [primary basis]
- We have the full rig, the reduced rig (fewer bones, same mesh) and the far billboard (`FISHMOD_LOD_DIST` 750).
- A small fish's single-triangle fins cost the RSP ~150 cycles each to draw and the RDP a full triangle setup each,
  for a few pixels.
- A decimated mid mesh (fins merged into the body silhouette, ~25-30 vertices, ~12-15 triangles, 2-3 bones) cuts both.
- tiny3d runs meshoptimizer in its build. HackerSM64's `AUTO_LOD` switches models by distance.
- Pick the switch distance with the pixel test we used for the props.

### Checked and not for us
- **Triangle snakes / strips (F3DEX3, tiny3d):** F3DEX3's own `Performance.md` now says snakes are "no longer
  competitive in RSP time" (~70 µs RDRAM saved against ~400 µs more RSP in a typical scene). tiny3d's strips belong to
  its own microcode.
- **CPU skinning** (OoT/MM `z_skin.c`, Epona's way): rewrites vertices on the CPU every frame. Our CPU is the tighter
  side.
- **Shared-pose instancing:** every bone matrix holds the fish's world transform, so the RSP still needs per-fish
  matrices. `G_MTX_MUL` on the RSP is discouraged in F3DEX3 (moved to an overlay; accuracy). It would only save the
  CPU's pose chain, which isn't where fish time goes.
- **Turbo3D microcode** (SGI's "many objects" ucode): it has no lighting and no clipping, and switching microcode
  mid-frame costs. The figures for it (500-600k polygons a second) come from a low-quality document *(secondary)*.
- **MM's RSP point lights** (`ACTOR_FLAG_UCODE_POINT_LIGHT_ENABLED`): F3DEX3 point lighting is 194 cycles a vertex
  pair against 70 for one directional light. Wrong way for fish.

## The RDP per fish

### 6. Fish in the near props' edge mode [primary basis, gain by analogy]
- The fish draw with `SETUPDL_25`: `AA_ZB_OPA_SURF`, 2-cycle with fog, which reads the colour buffer for every
  pixel.
- The props' edge mode without `IM_RD` (the VI smooths edges from coverage) was worth 10-13% of the props' RDP time
  (hwbench-2, hwbench-4).
- For fish the opaque equivalent is libultra's `RA_ZB_OPA_SURF`.
- The risk is the same as for the engine-wide idea in opt-plan-hw.md: edges of small far objects. Use it within the
  same 500 units and capture A/B.
- Under water the haze hides most edge differences anyway.

### Others for the fish
- **1-cycle mode:** only 1% for props (memory-bound). Not worth it for fish.
- **CI4 skins:** already listed in n64.md. Texture loads were 2-6% of RDP time for props.
- **The rdpbench2 cost model** (46 workloads) can price a fish in each mode before any console run.

## The CPU: audio (~12 ms, the biggest bucket)

Everything in our audio work so far keeps the output bit-exact (the audio check). The next two items change *when* or
*at what rate* audio is made. That isn't bit-exact, so it has to be judged by ear and by latency.

**How OoT splits audio work** (our `src/audio/internal/heap.c`):
- `samplesPerFrameTarget = ALIGN16(32000 / 60) = 544`.
- `ticksPerUpdate = (544 + 16) / 0xD0 + 1 = 3`: three synthesis ticks of 176 samples per retrace, 180 a second.
- The 0xD0 (208 samples) cap is the RSP's DMEM buffer (`DMEM_1CH_SIZE = 13 * 16 * 2` bytes).
- Every tick processes every active note (envelopes, ADPCM and resample commands, sample DMA) and lets each sequence
  player advance.

### 2. 22,050 Hz output: two ticks per retrace [primary basis, ours for the gain]
- `ticksPerUpdate` becomes 2 when `ALIGN16(rate / 60) + 16 < 416`, i.e. at ≤ 23,040 Hz: 120 ticks a second instead
  of 180. At 22,050 Hz that's 2 ticks of 184 samples. (At 24 kHz it's still 3: 400 + 16 = 416.)
- 22,050 Hz is also the lowest rate Nintendo's manuals call fine, so there's no room between "cheaper" and "too low".
- Nintendo's tuning guide says to pick a rate just under a multiple of the chunk size so the last pass isn't mostly
  empty, and that a lower rate shortens RSP time.
- Its manuals say 22.05 kHz and 32 kHz "do not produce a substantial loss of audio quality", and below 22.05 kHz
  noise becomes noticeable.
- MM and OoT both ship 32 kHz (OoT 23-24 voices, MM 16-28; checked in both `session_config.c`). Many other N64 games used 22,050 Hz
  *(secondary)*.

**Risks:**
- Content above ~11-12 kHz goes. That needs an A/B on real speakers.
- The sequence player's tempo ceiling (`maxTempo`) scales with ticks per update: at most one sequence tick per audio
  tick, so 120 a second. Check every Lake Hylia and fishing song's tempo against it.

**Gain [ours]:** if most of the 12 ms is per-tick note work (the profile's hottest audio lines are per-note, per-tick
loops), roughly a third of it, 2-4 ms, plus RSP audio time. Measure on the console.

### 3. Audio in bigger, rarer frames [primary basis]
- Nintendo's guide: "If the game runs at 30 frames per second or less ... audio processing time is reduced by
  doubling all of the buffers"; also "create audio over 2 frames".
- We render at 20 fps, but the audio thread runs on *every* retrace (`AudioMgr_HandleRetrace`, 60 Hz).
- Running it every 3rd retrace with 3x the samples keeps the same tick count (8 ticks of 200 instead of 9 of 176:
  slightly fewer). But the per-audio-frame work runs 20 times a second instead of 60:
  - command queues;
  - load processing;
  - the sample DMA countdown scans;
  - the AI buffer bookkeeping;
  - the RSP task launch and the graphics task's yield/resume.
- The game thread's caches are also flushed by the audio thread a third as often.

**Costs:**
- Up to +33 ms of sound-effect latency.
- 3x AI buffers (fine with the Expansion Pak).
- The audio frame must still be started on time when a game frame runs long (it's driven by the retrace, not the game
  frame, so that holds).

### 9. Voices and reverb [primary]
- HackerSM64 exposes the note count (24 on console) with "lower values ... can potentially improve performance
  slightly". Its better reverb has `downsampleRate` / `isMono` knobs that cut RSP work.
- For us: log the peak active notes at the lake (the per-note scans scale with `numNotes`, the byte mirror
  notwithstanding). If the lake never needs 24, a lake-specific spec with fewer notes is free.
- Reverb at the lake is mostly RSP time.

### Checked and not for us
- **Moving audio mixing to the RSP** (libdragon's mixer, Factor 5's MusyX *(secondary)*): OoT's synthesis already runs
  on the RSP. What the CPU spends is the sequencer and building the command lists, which a microcode doesn't remove
  without rewriting the driver.

## The CPU: collision (~6-7 ms)

### 4. A clearance bound for walls and ceilings [technique: standard; analogue in HackerSM64]
- A full wall/ceiling search can also report the distance to the nearest poly it saw in the cells it searched.
  Record that, and the actor's position.
- On later frames, while the distance moved since then (summed, or straight-line from the recorded point) plus the
  check radius stays under that clearance, the answer is provably "no wall, no ceiling": return it without a search.
- Moving platforms invalidate the bound: their BgActor transform-changed flags are already there (OoT and MM both
  have `BgActor_IsTransformUnchanged`).
- Guays circling over open water and Tektites hopping in the open are the best case: their nearest geometry is
  hundreds of units away. Link on the open beach too.
- Floors still change with every step, but the floor answer cache already covers repeat questions.
- **Exactness:** the collision log (FISHMOD_COLLOG) can prove it bit-identical over the route. That keeps the
  project's "same answers" rule, unlike the one-in-four Guay throttle.
- **Analogue:** HackerSM64's `FAST_FLOOR_ALIGN` and `AUTO_COLLISION_DISTANCE` (`config_collision.h`) cut collision
  queries by the same kind of reasoning (skip when the answer can't matter / doesn't reach). The clearance bound
  itself is the conservative-advancement idea from physics engines; no N64 source uses exactly this *(unverified for
  N64)*.

### Smaller collision items
- **Trim each actor's `Actor_UpdateBgCheckInfo` flags** to what its code reads. The Guay asks walls, floor and
  ceiling (`FLAG_0 | 1 | 2`); the Tektite's flags vary by state. Cheap, invisible if nothing reads the dropped
  answer.
- **MM's single `Sphere` collider type** (`Collider_InitSphere`, `CollisionCheck_*_SphereVs*` in `z_collision_check.c`)
  replaces 1-element JntSph colliders. A micro-saving in AT/AC/OC, only if a lake actor uses a 1-element JntSph.
- **The camera (2.8 ms):** OoT's camera does several line tests a frame. The clearance bound above applies to the
  camera's "is the view blocked" lines in open water as well.

## The CPU: matrices (~7 ms)

### 8. Affine-only, fused limb matrices [primary]
OoT's `Matrix_TranslateRotateZYX` (ours, already with combined sin/cos and skipped zero angles) still updates the
w row (`wx, wy, wz, ww`) in every rotation. For affine matrices that row is always (0, 0, 0, 1), so that's 4 of
every 16 multiplies wasted. HackerSM64 removes it:

- `mtxf_mul` "assum[es] they are both transformation matrices with a w-component of 1 ... saves some multiplications
  and addition", and writes 1.0 by bit pattern.
- `mtxf_rotate_xyz_and_translate_and_mul` builds the 3x3 rotation from six sin/cos values and multiplies it into
  the parent in one pass (27 + 9 multiplies) instead of three separate rotation applies.
- `mtxf_to_mtx_fast` skips the zero column (we already have word stores and claimed lines).

**For us:** an affine-only path in `Matrix_TranslateRotateZYX`, `Matrix_RotateZYX` and the push-multiply pattern
used by `SkelAnime_Draw*` (Link's limbs are the bulk). The results are bit-identical except possibly the sign of a
zero in the w row, which the fixed-point conversion erases. The graphics log will show it.

**Gain [ours]:** 10-25% of the matrix time. First split the 7 ms by caller (Link's skeleton, other skeletons,
camera, our props/rig) from pcprof3 to see what's left to take.

MM's `sys_matrix.c` adds `Matrix_MultZero`, `Matrix_MultVecX/Y/Z` (transform an axis vector without the full
multiply) [primary]. That's a micro-win wherever OoT code multiplies `(0, 0, z)`-style vectors.

## The CPU: memory and caches

### 5. Claim every display-list cache line before writing it [primary basis, ours for the gain]
- The data cache allocates on write, so the first store into a new 16-byte line reads it from RDRAM (~40 cycles)
  even though every byte will be overwritten.
- We already claim lines (Create Dirty Exclusive) for `Matrix_MtxFToMtx`'s output and the audio command list. The
  audio claim measured ~1.5% of the CPU.
- The display lists themselves (POLY_OPA/XLU/OVERLAY and the work buffer) are written the same way, 2 commands a
  line, tens of KB a frame.

**How to do it:**
- A helper that, at natural points (each actor draw, each prop batch), claims the lines *wholly ahead of* the
  current write pointer, up to ~1 KB.
- Never claim the line the pointer is in: it holds commands already written.
- The pool's unwritten tail is never read, so a claimed-then-evicted line's garbage is harmless.

**Side benefit:** every RDRAM read the CPU skips is bus time the RDP gets back. Kaze's point that "whenever you use
the CPU, you also slow down the rendering" (SED interview) *(secondary)* is the reason this matters even when the
frame is RDP-bound.

### 10. Profile-guided code placement [secondary basis]
- The 16 KB instruction cache is direct-mapped, so hot functions 16 KB apart evict each other.
- pcprof3 has per-instruction samples. From them:
  - link the hottest `src/code` functions (actor loop, BgCheck inner tests, matrix, audio synthesis/seqplayer)
    contiguously;
  - put the audio driver's hot loop in a part of the 16 KB "colour" that the game thread's hot set doesn't use (the
    audio thread interrupts the game thread 60 times a second today).
- Kaze attributes much of SM64's slowness to cache misses *(secondary)*. HackerSM64 builds with `-Os`, little
  inlining and aligned hot functions [primary].
- Our own tries: 32-byte function alignment hung at boot (frames6). Ordering is a different change: no alignment
  padding, just link order.
- Overlays (fish, Guay) load at runtime addresses we don't choose, so this is for the code segment and audio.

### Already done here, not repeated
Data packing of hot audio fields, the sample store in Expansion Pak RAM, frame buffers in separate banks, the
run-ahead triple buffer, word memset, polynomial trig, GCC/clang/PGO builds.

## The CPU: actor and fish update

- **MM's actor loop is OoT's** (see below). There's no hidden actor-update speedup to borrow.
- **Indiana Jones (Factor 5):** "Objects and scripts that were idle were not updated", and smaller data structures
  (16-bit instead of float, bit fields) "improved CPU cache hit rates" (Game Developer article) *(secondary)*. We do
  the first (think budget, dormant fish, unspawned fish days). The second is n64.md's "hot fields in one small struct"
  item, still worth doing for the fish's per-frame fields if the fish update profile shows misses.
- **TP's 4-frame re-aim** and the global think budget are in fish-ai.md and done.

## What Majora's Mask changed from OoT (read in zeldaret/mm)

The short answer: **the engine is nearly the same; MM's speed came from content choices, not a faster engine.**

| Area | MM vs OoT | Useful to us? |
|---|---|---|
| Actor update (`Actor_UpdateActor`, `Actor_UpdateAll`) | Same flow: culling-volume flag decides update/draw unless the culling-disabled flags are set; adds freeze categories (talking, ocarina, time-stop cutscenes) and `halfDaysBits` to kill actors not in the current half-day | No speedup to borrow |
| Culling (`Actor_CullingCheck`) | Same projected-position test, fields renamed `cullingVolumeDistance/Scale/Downward` | No |
| Lights (`Actor_Draw`) | `ACTOR_FLAG_UCODE_POINT_LIGHT_ENABLED`: point lights on the RSP (MM's PL microcode), CPU binding skipped | No (RSP cost; see above) |
| Matrices (`sys_matrix.c`) | Adds `Matrix_MultZero`, `Matrix_MultVecX/Y/Z`, `Matrix_RotateXFApply` style helpers | Micro, see item 8 |
| Collision (`z_bgcheck.c`) | Same lookup grid and BgActor transform-unchanged skip (OoT has it too); more raycast variants | No |
| Colliders (`z_collision_check.c`) | New single-`Sphere` collider type | Micro |
| Audio | Same driver; `gAudioSpecs` at 32 kHz with 16-28 notes | No; MM didn't make audio cheaper |
| Grass (`ObjGrass`) | Opaque meshes in groups, one cull test and one light bind a group, fade 1080-1180 | Already in opt-plan-hw.md |
| Lab fish (`En_Fish2`) | | Already in fish-ai.md |

2Ship2Harkinian and Ship of Harkinian are PC ports (interpolation, PC renderers). Their changes don't carry back to
N64 cost, and they weren't researched further.

## Commercial games: what could and couldn't be checked

- **Factor 5 (Rogue Squadron, Battle for Naboo, Indiana Jones):** custom graphics microcode and the MusyX audio
  microcode *(secondary: OSNews, olivieryuyu's microcode RE blog)*. The microcode details aren't public as source.
- **Indiana Jones (Game Developer, "Bringing Dr. Jones to the Infernal Machine")** *(secondary)*:
  - TLB paging of code (4 KB pages, ~1,000 lines of assembly; 400 KB physical for ~1 MB of code);
  - LZ texture cache;
  - 16-bit data;
  - animation key frames 28 to 6 bytes;
  - display lists built when geometry becomes visible and purged after a while;
  - idle objects not updated.

  Memory-driven. The cache-hit side effect is the only part for us (see above).
- **Boss Game Studios (World Driver Championship):** custom microcode is widely reported. No technical description
  or number was found *(unverified)*.
- **Rare (Conker, Banjo-Tooie, DK64, Perfect Dark):** custom microcode is often claimed. No primary or technical
  source was found for Conker's skinning or any measured gain *(unverified)*.
- **Kaze (SM64, Return to Yoshi's Island):**
  - interview: cache-line alignment, keeping a thread's working set in the cache, CPU traffic slowing the RCP
    *(secondary)*;
  - occlusion culling 10-20% in some areas *(secondary)*. Our occlusion-plane test lost (fourway2).
  - The specific collision/animation videos couldn't be opened from here.

## Sources

Primary (read directly):
- F3DEX3: `README.md`, `docs/Documentation/Performance.md`, `Design Tradeoffs.md`, `Backwards Compatibility.md`,
  `docs/Code/Counters.md`, `gbi.h` (`gSPMatrix` notes), `f3dex3.s` (`G_MTX_handler`, `mtx_multiply`, `mvpValid`):
  https://github.com/HackerN64/F3DEX3
- tiny3d: `README.md`, `docs/modelOpt.md`: https://github.com/HailToDodongo/tiny3d
- HackerSM64: `include/config/config_collision.h`, `config_audio.h`, `config_graphics.h`, `src/engine/math_util.c`:
  https://github.com/HackerN64/HackerSM64
- HackerOoT: `include/config/config_graphics.h` (F3DEX3 default, NOSYNCS caveat): https://github.com/HackerN64/HackerOoT
- Majora's Mask decomp: `src/code/z_actor.c`, `z_bgcheck.c`, `z_collision_check.c`, `sys_matrix.c`,
  `src/audio/session_config.c`: https://github.com/zeldaret/mm
- Nintendo N64 Audio Tuning Guide (developer news): https://ultra64.ca/files/documentation/online-manuals/man/developerNews/news-03-5.html
- Nintendo programming manual, audio constraints: https://ultra64.ca/files/documentation/online-manuals/man-v5-1/pro-man/pro20/20-02.htm
- Our own: `src/audio/internal/heap.c` (tick formula), `include/audio.h` (`DMEM_1CH_SIZE`),
  `src/overlays/actors/ovl_En_Fishmod/fishmod_rig.c`, `fishmod_manager.c`, `src/code/sys_matrix.c`,
  `src/code/z_bgcheck.c`, releases/history notes (frames6, frames7, pcprof2).

Secondary:
- Kaze Emanuar interview (Software Engineering Daily): https://softwareengineeringdaily.com/wp-content/uploads/2024/03/SED1666-Kaze_Emanuar.txt
- Kaze's occlusion culling: https://80.lv/articles/occlusion-culling-system-for-optimized-rendering-on-nintendo-64
- Indiana Jones memory article: https://gamedeveloper.com/programming/bringing-dr-jones-to-the-infernal-machine-dealing-with-memory-constraints
- Factor 5 microcode RE: https://www.osnews.com/story/129733/ and https://olivieryuyu.blogspot.com/2020/01/
- SM64 optimization summaries: https://setsideb.com/?p=8300 , https://hackaday.com/2022/04/21/optimized-super-mario-64-offers-exciting-possibilities
