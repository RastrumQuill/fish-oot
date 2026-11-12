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

## Second pass (comprehensive)

Written after the first pass above, to cover the areas it left out. Nothing listed in the first pass, in
[n64.md](n64.md), in [opt-plan-hw.md](../history/opt-plan-hw.md) or in the git log is repeated here unless something
new was found about it. Claims are tagged as above: **[primary]** read in source or Nintendo's docs, **[secondary]**
from an article or summary, **[ours]** our own estimate (each needs a console A/B before it stays). Items that would
change the picture are marked **visible**; items that would change audio output are marked **AUDIO** (the rule is
bit-exact unless the user decides otherwise).

**Where the frame is now** (frames6 console run, leg 1 under water, F3DEX3 counters): 5,169 vertices, 3,050
triangles in, 1,329 drawn; the RSP **stalled 19 ms a frame on a full RDP FIFO**. At the lake the RDP is the limit,
so RDP items come first. Local RDP prices used below come from rdpbench2
(`releases/history/console/rdpbench2-20261112.txt`):
- one full-screen fill: **0.91 ms** (test 1 minus the empty frame);
- the empty frame's setup (which includes the colour and Z clears): **2.46 ms**;
- large shaded triangles: 22 ns a pixel without Z, 64 with Z, 82.5 in 2-cycle fog AA with Z (tests 19, 23, 31);
- textured rectangles: 18.9 ns a pixel in 1-cycle, 38-41 in the 2-cycle and translucent modes (tests 4, 9, 10).

### Checklist

| Area | Searched | Usable found? |
|---|---|---|
| RDP: Z-buffer off for parts of the scene | James Lambert's N64 open-world engine, Banjo-Kazooie decomp (`modelRender.c`), Indiana Jones (Factor 5), HackerOoT's clears, our rdpbench2 | **yes**: S1, S2, S4 |
| RDP: perspective correction off (`G_TP_NONE`) | Nintendo manuals (`gDPSetTexturePersp`, pro-man ch. 13), F3DEX3 | **no**: the docs say perspective correction is *required* for all textured triangles with the standard RSP microcodes; no speed difference is documented. Only texture rectangles take `G_TP_NONE`, and OoT already does that |
| RDP: texture LOD / mipmapping / sizes | tiny3d `20_mipmaps`, F3DEX3, Nintendo tuning guide, rdpbench2 | **no new**: mipmapping needs 2-cycle and more TMEM, so it costs on the N64 rather than saving. The useful part (half-size far textures) is done (far tiers); texture loads are 2-6% of RDP time (hwbench-1) |
| RDP: 1-cycle / 2-cycle / copy / fill | z_vr_box_draw.c, SETUPDL_40, rdpbench2, HackerOoT | **yes**: S3 (sky in 1-cycle); S1/S2 (fill-mode clears). Copy mode only fits opaque, unblended rectangles: no HUD element of ours qualifies |
| RDP: anti-aliasing per object class | HackerSM64/HackerOoT `DISABLE_AA`, libultra modes | **no new**: props done, fish = first pass #6, opaque engine-wide = opt-plan. VI-side AA off is rejected (S-X3) |
| RDP: framebuffer format / resolution / VI | HackerSM64 `BORDER_HEIGHT_CONSOLE`, libdragon VI filters, Nintendo manuals | **yes, visible only**: S18 (overscan border). 16-bit colour is already in use; a smaller framebuffer with VI upscaling and VI AA off are rejected (S-X3, S-X4) |
| RDP: dithering / alpha compare cost | libdragon `rdpq_mode.h`, Nintendo manuals | **no**: dithering happens in the blender at no documented extra cycle. Alpha compare only saves (skipped writes), and we already use it (the dissolve, cut cards) |
| RDP: overdraw (front-to-back, occlusion) | Nintendo tuning guide ("draw the closest objects first"), F3DEX3 `SPAlphaCompareCull`, OoT `Room_DrawCullable` (already sorts nearest first) | **yes**: S4, S5, S6 |
| RDP/CPU: RDRAM banks and bandwidth | frames6 (frame buffers in separate banks), hwbench-5h `zsame`, first pass #5 | **no new**: done and measured here |
| RSP: vertex count / clipping | F3DEX3 porting guide (56-vertex buffer), F3DEX3 profiling counters | **yes**: S9 (repack vanilla lists for 56 vertices), S10 (measure clipping) |
| RSP: culling (`G_CULL_DL`, bounding volumes) | F3DEX3 gbi, tiny3d BVH, our draw paths | **no new**: the CPU already culls fish, props and actors before any list is emitted. A `G_CULL_DL` would duplicate that. Finer room culling is already planned (opt-plan engine-wide #4); frames6's 3,050-in / 1,329-drawn supports it |
| RSP: F3DEX3 features we don't use | F3DEX3 README, gbi.h, Porting guide, Performance.md, HackerOoT's use of them | **yes**: S2 (`SPMemset`, `SPFlush`), S6 (`SPAlphaCompareCull`), S8 (DL hints), S13 (no syncs in texture loads). Evaluated, not for us: snakes (first pass), occlusion plane (lost in fourway2), `G_ATTROFFSET_ST` (UV scroll; ours is CPU `Gfx_TwoTexScroll`, a few commands), `SPLightToRDP`, Fresnel, specular |
| RSP: lighting cost | F3DEX3 Performance.md, our code | **no new**: `gSPSetLights` is in use (`z_lights.c`), the props use ambient-only light, and the fish draw from the manager |
| RSP: matrix count | | **no new**: first pass #1 |
| RSP: DL size / branching | F3DEX3 hints, relative segments | **yes**: S8 |
| RSP: microcode choice | HackerSM64 `OBJECTS_REJ`, Turbo3D, F3DEX3 variants | **no**: `BrW_NOC` stays. Rej microcodes (64 vertices, no clipping) are an F3DEX2-era option that F3DEX3's 56 vertices mostly covers |
| CPU: animation | tiny3d (`t3danim`, `t3dskeleton`: streamed, sparse keyframes; dirty-bone updates), Perfect Dark decomp (`model_tick_anim*`), Kaze (bone removal, *secondary*), OoT `z_skelanime.c` | **yes, small**: S12 (Link's per-frame animation DMA), S22 (lazy pose). tiny3d's dirty-bone update doesn't fit OoT, because every limb matrix carries the full model-view and changes whenever the actor or camera moves |
| CPU: display list caching | libdragon `rspq` blocks, tiny3d (matrices/vertices DMA'd at run time so recorded lists stay valid) | **no new**: our props keep unchanged commands in place (4c74494), their matrices are kept for the scene (c4464a0) and the fish rig bases are built once. Link's per-frame list is a few words a limb next to the matrix maths |
| CPU: audio | Nintendo audio tuning guide, n_audio docs, HackerSM64 `config_audio.h`/`data.c`, OoT driver | **yes**: S7 (bit-exact batched audio frames), S17 (PCM samples, RSP side). n_audio is libultra's driver, not OoT's: not applicable |
| CPU: collision | Kaze's shadow raycasts (*secondary*), OoT `ActorShadow_DrawFeet`, `z_bgcheck.c` | **yes, small**: S11. A finer lookup grid is already done (`Fishmod_SizeCollision`) |
| CPU: camera | | **no new** (first pass: clearance bound applies to camera lines) |
| CPU: actor framework | OoT `Actor_Spawn`, ZeldaArena | **yes, small**: S14 (pool fish actors) |
| CPU: effects / particles | OoT `EffectSs_*`, our particles | **no new**: ripple matrices done (frames5); scanning the EffectSs table is a few µs |
| CPU: memory allocators | ZeldaArena (first-fit list), GRAPH_ALLOC (bump) | **yes, small**: S14 |
| CPU: ROM DMA / decompression | `AnimTaskQueue_AddLoadPlayerFrame`, Yaz0 | **yes, small**: S12. LZ4 in place of Yaz0 is already in opt-plan (load times only) |
| CPU: threads, priorities, interrupts | `padmgr.c`, `sched.c`, `thread.h` | **yes, small**: S15 (controller polling), S7 (audio wakes). The priorities are fine |
| CPU: caches / TLB / data layout | Kaze's cache-instruction video (title only), `sched.c` | **small**: S20. The TLB isn't used. Kaze's "more RAM than you think" (the RDRAM 9th bit) is about memory, not speed |
| CPU: compiler / linker | Kaze render work (*secondary*), Makefile flags | **yes**: S16 (jump-table threshold), S19 (`-G` small data), S21 (divide checks), S23 (LTO) |
| Frame pacing | Perfect Dark decomp (variable timestep), Nintendo tuning guide (triple buffering), commercial games | **no new**: run-ahead with a triple buffer is done, the think budget is done and the plant budget is planned. A variable timestep is not usable (S-X5) |
| Commercial games / postmortems | searches for WDC, F-Zero X, Rogue Squadron, Conker, Banjo-Tooie, Perfect Dark, Jet Force Gemini, Ep. 1 Racer | **little**: see "Commercial games" below. No technical postmortem with numbers was found for any of them |

**Sources that couldn't be reached from here:**
- **N64brew wiki:** returns 403 (bot check), and archive.org is blocked by the proxy.
- **Kaze's videos:** the titles were listed from his channel page, but the video content and transcripts were blocked
  (YouTube asks for a login). Titles worth watching by hand: "The Genius of the N64's CACHE Instruction", "ILLEGAL 3D
  Rendering Techniques (N64)", "Why Devs NEED TO know about Render Matrices!", "He spent weeks to save 100
  Microseconds", "How Optimizations made Mario 64 SLOWER", "Mario 64's Sound Engine has some WEIRD features",
  "The Most Wasteful Object in Mario 64 (24x Improvement)".
- **Game Developer / Retro Gamer postmortems** for the named games: none found with technical content.

### RDP: clears, sky, fog, Z

#### S1. No colour clear when the sky is drawn [primary: HackerOoT]
- **What it is:** `Gfx_SetupFrame` fills the whole colour buffer every frame. Above water it fills black, which the
  skybox then overwrites. HackerOoT (`z_play.c`, `Gfx_SetupFrame(gfxCtx, clearFB, ...)`) clears only when no skybox
  is drawn or the skybox is disabled.
- **Saves:** one full-screen fill, **0.91 ms of RDP** a frame above water [primary for us: rdpbench2 test 1]. Under
  water we draw no sky, so the fog-colour fill stays.
- **Already done here?** No (`z_play.c:1216`: `Gfx_SetupFrame` with black or the fog colour, always).
- **Side effects:**
  - OoT's normal sky has 5 faces, no bottom one (`z_vr_box_draw.c`). A pixel that no geometry covers while looking
    steeply down would show an old frame instead of black.
  - At Lake Hylia, terrain or water covers everything below the horizon. Check with capture shots looking down at the
    tour stops, and keep the fill for scenes or benches without the sky (`FISHMOD_BENCH_NOSKY`).
- **Effort:** trivial.

#### S2. The Z clear by the RSP (`SPMemset`) after the sky, with `SPFlush` [primary: F3DEX3, HackerOoT]
- **What it is:** F3DEX3's `SPMemset` fills RDRAM by RSP DMA, "faster than the RDP can in fill mode". HackerOoT's
  `Gfx_ClearZBuffer` (`gUseMemsetForZBuffer = 1`) issues it **after** the sky's commands, so the RSP clears Z while
  the RDP draws the sky. `SPFlush` (gbi.h) makes the RDP start on the first big clear at once: without it, the RDP
  can wait for up to 46 more commands.
- **Saves:** the Z fill's ~0.9 ms of RDP, moved to an RSP that is idle on FIFO stalls at that point [ours]. Under
  water (no sky), put `SPFlush` after the colour fill so the two clears overlap.
- **Already done here?** No (`z_rcp.c:1580`, fill-mode Z clear; no `gSPMemset` or `gSPFlush` anywhere in `src/`).
- **Side effects:** none. Same Z values; the letterbox rows are skipped as now. The graphics log shows the list
  change, and the picture is identical.
- **Effort:** low.

#### S3. The sky in 1-cycle with one texture while the blend is 0 [primary basis, ours for the gain]
- **What it is:** the sky draws in 2-cycle (`SETUPDL_40`):
  - the combiner is `(TEXEL1 − TEXEL0) × PRIM_A + TEXEL0`, with the second cycle a pass-through;
  - each tile loads two CI8 textures (`gDPLoadMultiTile` from `staticSegments[0]` and `[1]`).
- **Why it's exact:** with `skyboxBlend` = 0 (`gDPSetPrimColor(..., blend)`), the result is exactly `TEXEL0`. A
  1-cycle setup with combiner `TEXEL0`, the same bilinear filter, `G_RM_OPA_SURF` and only the first texture's loads
  draws the same pixels.
- **Saves:**
  - half of the sky's texture-load bytes;
  - the 2-cycle pixel rate: rdpbench2 has 1-cycle textured rectangles at 18.9 ns a pixel against 38-41 in 2-cycle.
  - Of the sky's measured **5.3 ms** (hwbench-4), maybe **1-2.5 ms** above water whenever the blend is 0 [ours].
- **Already done here?** No.
- **Side effects:**
  - none when the blend is 0;
  - while the blend moves (dawn and dusk transitions), keep 2-cycle;
  - at 255 the 2-cycle lerp isn't exactly `TEXEL1`, so keep 2-cycle there too unless a capture shows it equal.
- **Effort:** low-medium: a second sky path in `Skybox_Draw`; the face lists in `Skybox_Setup128` need a variant
  without the second texture's loads.

#### S6. Cull triangles that are wholly in fog, under water (`SPAlphaCompareCull`) [primary: F3DEX3 gbi]
- **What it is:** with `G_FOG`, the vertex shade alpha carries the fog factor. `gSPAlphaCompareCull(G_ALPHA_COMPARE_CULL_ABOVE,
  255)` culls a triangle whose three vertices are all at full fog. The gbi itself names this use: "cull tris which are
  fully in fog, replacing far clipping which is removed in F3DEX3".
- **Why it's (nearly) invisible under water:** there's no sky under water, and the frame is cleared to the fog colour
  (`FISHMOD_NO_SKY` path). A fully fogged triangle draws the fog colour over the fog colour. Anything behind it is
  further away, so it is fully fogged as well.
- **Saves:** RDP fill and setup for far terrain and bed under the haze, on the diving views (21% of them still over
  50 ms in `section_budget.py`) [ours; count culled triangles with F3DEX3 profiling build B before and after].
- **Already done here?** No.
- **Side effects and conditions:**
  - It must only be on in lists drawn with `G_FOG`. In an opaque mesh without fog, shade alpha is the vertex alpha,
    often 255, and the whole mesh would vanish. So: on around the room mesh and the props, off for everything else.
  - The blender's fog at alpha 255 may differ from the clear colour by an LSB or by dither noise, and a fogless
    translucent effect behind culled terrain could show through. Capture A/B under water.
- **Effort:** medium.

#### S4. Far backdrop geometry without Z [secondary: James Lambert's engine, Factor 5; primary: Banjo-Kazooie]
- **What it is:** draw the scene's far backdrop (Lake Hylia's surrounding cliffs and far banks) right after the sky
  with Z off: no Z read and no Z write. The backdrop's pixels then keep the cleared Z, so everything nearer still draws
  over it correctly.
- **Who else did it:**
  - James Lambert's open-world N64 engine draws distant objects first "with the Z-buffer disabled", back to front,
    then the near ones (Hackaday, code at `lambertjamesd/n64brew2025`) *[secondary]*.
  - Factor 5's Indiana Jones didn't use a Z-buffer because it "consumed the already constrained ... fill rate"
    *[secondary: OSNews]*.
  - Banjo-Kazooie's `modelRender.c` has per-model depth modes NONE / COMPARE / FULL [primary].
- **Saves:** our rdpbench2 has Z roughly tripling the cost of a large triangle's pixel (22 against 64 ns, and 82.5 in
  the fogged 2-cycle AA mode). For a backdrop covering a fifth of the screen above water, that is ~0.5-1 ms [ours].
- **Already done here?** No. The props' "no depth writes" test (hwbench-2: 4% slower) is a different thing: that
  kept the Z read and lost the rejection.
- **Side effects:**
  - The backdrop must not overlap itself, or must be sorted (back-face culling and a back-to-front order of its
    parts).
  - It must lie wholly behind everything else, so it has to be chosen by a build-time split of the room mesh with
    margins.
  - Fog is unchanged. Capture A/B.
- **Effort:** high (a scene display-list tool, like texpack).

#### S5. Near opaque props and fish before the room mesh [primary: Nintendo tuning guide; ours for the gain]
- **What it is:** Nintendo's tuning guide says to "draw the closest objects first so later objects only write the
  uncovered Z-buffer areas".
- **What we do now:**
  - OoT already sorts the room's cullable entries nearest first (`Room_DrawCullable`), and our props come in
    nearest-first bands.
  - But all props and fish draw *after* the room. So near plants under water are drawn over a bed that was already
    shaded, fogged and AA-blended behind them.
  - Drawing the near opaque props (and close fish) before the room turns those bed pixels into Z rejects, which cost
    a Z read instead of the 2-cycle fog AA pixel.
- **Saves:** depends on overlap; `overdraw.py --props` can count the bed pixels hidden by near props first [ours].
  hwbench-4's `nostrict` (order *among* props) showed nothing; this is order *against the terrain*, untested.
- **Already done here?** No (`EnFishmod_ScatterDraw` runs from the manager actor's draw, after the room).
- **Side effects:**
  - Edge pixels in modes that read the colour buffer blend with whatever is already there (the clear colour instead
    of the terrain).
  - Our near props use the edge mode without `IM_RD`, so they are the safe ones to move. Fish in `AA_ZB_OPA_SURF` are
    not, unless first pass #6 is done.
  - Capture A/B.
- **Effort:** low-medium (a hook before the room draw in `Play_Draw`).

### RSP and display lists

#### S8. F3DEX3 display-list hints [primary: F3DEX3 gbi]
- **What it is:** `gSPDisplayListHint(pkt, dl, count)` / `gSPBranchListHint` / `gSPEndDisplayListHint` tell the
  microcode how many commands the target list has, so it fetches only those instead of a full input buffer. The
  feature was suggested by Kaze. F3DEX3 estimates "maybe around 100 us per frame" of DRAM traffic. HLE ignores the
  bits.
- **Where:** the calls we emit most: per-prop and per-fish calls to fixed lists, the rig's tint segment calls, room
  entries. The counts are known at build time.
- **Already done here?** No (no `Hint` macro used).
- **Side effects:** none: "the actual on-screen result will not change regardless of the value of count". A wrong
  count only costs speed.
- **Effort:** low.

#### S9. Repack vanilla display lists for the 56-vertex buffer [primary: F3DEX3 Porting guide]
- **What it is:** vanilla lists were made for F3DZEX's 32-vertex buffer, so shared vertices get reloaded and
  re-transformed. The porting guide recommends re-exporting as many lists as possible for F3DEX3 "to take advantage
  of the substantially larger vertex buffer".
- **For us:** a build-time pass like texpack, which merges consecutive `gsSPVertex` loads where the triangles' working
  set fits 56 and remaps their indices: Lake Hylia's room mesh, Link and the lake's common objects.
- **Saves:** RSP vertex work and list bytes. frames6 leg 1 transformed 5,169 vertices for 3,050 triangles, about 70
  cycles a vertex pair. Only worth it where the RSP is the limit (the lab tank: RSP 28.7 ms at 36 fish), since at the
  lake the RSP is stalled on the RDP [ours].
- **Already done here?** No.
- **Side effects:** none if the triangles and vertices are the same (graphics log, picture identical).
- **Effort:** medium.

#### S10. Measure clipping, then the near plane [primary: F3DEX3 counters]
- F3DEX3 profiling build B counts clipped triangles. frames6 built it, but its clip counts aren't in the notes.
- Clipping a triangle costs hundreds of RSP cycles. Diving with the camera inside the weed beds and close to the bed
  is the likely case.
- If the count is high: a slightly larger `zNear` while under water cuts clipping and gains Z precision. That one
  needs a check for near-camera pop-through, so it is **visible-risk**.
- **Effort:** reading an existing counter first: trivial.

#### S13. No syncs inside texture loads, game-wide (`NO_SYNCS_IN_TEXTURE_LOADS`) [primary: F3DEX3 gbi, HackerOoT]
- **What it is:** F3DEX3 says disabling the syncs in the multi-command texture-load macros "saves a few percent of
  RDP cycles for each material setup". texpack only drops *empty* pipe syncs.
- **The catch:** HackerOoT ships with `ENABLE_F3DEX3_NOSYNCS false`, because some vanilla lists "rely on the syncs
  within the texture loading commands for correctness. Some have already had the missing syncs manually added, but
  some have not been fixed yet."
- **Saves:** rdpbench2 test 44 against 14 puts a load's syncs at about 0.9 µs. With a few hundred material loads a
  frame, 0.2-0.4 ms of RDP [ours].
- **Side effects:** possible texture garbage in vanilla lists that need their own syncs. It needs captures in every
  scene touched (the graphics log can't see RDP hazards).
- **Effort:** medium, with risk.

### CPU: audio

#### S7. Batched audio frames, kept bit-exact [primary basis: Nintendo audio tuning guide; ours for the method]
- **What it is:** the first pass's item 3 (audio every 3rd retrace) changed tick sizes (8 of 200 instead of 9 of
  176), so it isn't bit-exact. Instead, keep the ticks exactly as they are and only batch them:
  - on one wake, run the three audio frames' updates back to back (the same 3 × 3 ticks of 176 samples, the same
    order);
  - put their RSP commands in one task;
  - hand the three AI buffers to the AI one per retrace, as now (`osAiSetNextBuffer` from the retrace handler).
- **Exactness:**
  - The game's sound commands arrive once per game frame (20 Hz).
  - If the batch starts right after the game frame's commands, they are processed at the same tick as in the
    unbatched case where they land in the first of the three audio frames.
  - The audio check (lockstep, every buffer hashed) proves it or not.
- **Saves [ours]:**
  - two of every three audio-thread wakes;
  - their cache flushes of the game thread's working set;
  - two RSP task launches;
  - two graphics-task yield/resume pairs a frame.
- **Side effects:** +2 retraces (33 ms) of sound latency; 3 AI buffers held (Expansion Pak room is fine). **AUDIO:**
  exact by construction, to be proven by the audio check.
- **Effort:** medium (`AudioMgr_HandleRetrace`, `AudioThread_Update`, the AI buffer ring).

#### S17. Samples as raw 16-bit PCM instead of ADPCM (RSP audio time) [primary basis: Nintendo tuning guide; ours]
- **What it is:** Nintendo's guide: "minimize resampling and ADPCM processing ... use uncompressed audio if ROM space
  allows". OoT's driver has `CODEC_S16`. Samples decoded offline with an exact model of the RSP's ADPCM decode would
  give the same samples without the RSP decode.
- **Saves:** RSP audio time only (the CPU builds similar commands), so it helps the graphics task's share of the RSP.
- **Side effects:**
  - **AUDIO:** exact only if the offline decode matches the RSP bit for bit, loop predictor states included. The
    audio check proves it.
  - 3.5× the sample bytes (ROM, and the Expansion Pak sample store).
- **Effort:** medium-high. Low priority while the RSP waits on the RDP.

#### Searched, not usable
- **Nintendo's n_audio:** faster through linear envelopes and an unsplit command list, but it belongs to libultra's
  own audio library, not OoT's driver.
- **Sound mode (stereo, mono, headset, surround):** HackerSM64's `ENABLE_STEREO_HEADSET_EFFECTS` off is "slightly
  beneficial for CPU performance". In OoT that's the player's sound option, and changing it changes the sound
  (**AUDIO**).
- **HackerSM64's reverb `isMono` / `downsampleRate` knobs:** **AUDIO**; first pass #9 already covers reverb.

### CPU: collision, animation, actors, threads

#### S11. Shadow and footprint floor rays from Link's own floor [secondary: Kaze; primary: OoT code]
- **What Kaze did:** cut SM64's three raycasts per shadow to one, about 3 ms of SM64's render time (dtf.ru summary
  of his render video) *[secondary]*.
- **In OoT:**
  - `ActorShadow_DrawFeet` asks the floor under each foot (`func_800BFCB8`) every frame for Link (and the horse);
  - `z_player_lib.c:1228` raycasts for footprints.
  - The feet move every frame, so the floor answer cache rarely holds them.
- **For us:** first test whether the foot's XZ lies inside `actor.floorPoly` (Link's current floor, already found)
  and take its plane height; fall back to the search only if it doesn't. The collision log proves where it matches.
- **Saves:** 2-3 floor queries a frame, ~0.1-0.3 ms [ours].
- **Exactness:** not guaranteed where floors stack, hence the fallback and the log.
- **Effort:** low-medium.

#### S12. Link's animation frames from RAM instead of a cart DMA every frame [primary: OoT `z_skelanime.c`]
- **What it is:** `AnimTaskQueue_AddLoadPlayerFrame` issues a `DMA_REQUEST_ASYNC` from `link_animetion` (2.5 MB in
  ROM) for every frame of every animation Link plays; `AnimTaskQueue_WaitForDma` blocks on it.
- **Each request costs:** the DmaMgr thread's wake (priority 16), the PI interrupt, two context switches and a
  blocking wait.
- **For us:** a small cache in the Expansion Pak, keyed by ROM offset, of the animations played at the lake (idle,
  run, swim, fishing).
- **Saves:** tens of µs per request [ours]; frees the PI bus.
- **Exactness:** exact (same bytes).
- **Effort:** low-medium.

#### S14. Pool fish actors instead of spawning and killing them [primary basis: OoT `Actor_Spawn`, ZeldaArena]
- **What it is:** the roster wakes fish "a slice at a time". Each wake is an `Actor_Spawn`: a first-fit ZeldaArena
  allocation over a linked list, an actor-sized clear, `Init`. Each sleep is a free, which fragments the arena.
- **For us:** keep N fish actors allocated and rebind them to roster entries.
- **Saves:** spikes rather than averages. Check `framelog.py --spikes` for frames with roster wakes first.
- **Exactness:** exact if the fish's state is fully re-initialised (the fish's own RNG is per fish since refactor
  stage 7).
- **Effort:** medium.

#### S15. Read the controllers once per game frame [primary: OoT `padmgr.c`]
- **What it is:** `PadMgr_HandleRetrace` runs `osContStartReadData` and the rumble update on **every** retrace,
  3 times per 20 fps frame. Each is an SI round trip, an interrupt and a thread wake.
- **For us:** poll only on the retrace before the game frame starts. That gives the same or fresher input.
- **Saves:** ~0.1-0.2 ms CPU a frame [ours].
- **Side effects:** input timing moves by up to a retrace (gameplay timing, not results). Scripted benches feed
  their own input. The rumble pak's motor timing needs a check.
- **Effort:** low.

#### S22. Lazy pose decode for actors not drawn [ours]
- `SkelAnime_Update` decodes the frame into `jointTable` every frame, drawn or not. Deferring the decode to the draw
  saves a few µs per off-screen actor.
- Only for actors whose code doesn't read joints in update. Small; listed for completeness.

#### Searched, not usable
- **tiny3d's dirty-bone update** (`t3d_skeleton_update` recomputes only changed bones and their children) doesn't
  fit OoT: every limb matrix carries the full model-view, so all limbs change whenever the actor or camera moves. Our
  rig's "only moving limbs get matrices" is the part that fits.
- **Perfect Dark's `model_tick_anim*`** step animations in 1/240 s units for its variable timestep. That's not a
  level-of-detail scheme.
- **Kaze's SM64 bone removal** (1.5 ms) is done for the fish (the rigs), and Link's skeleton is shared vanilla data.
- **Animation compression** (tiny3d's quantized, streamed keyframes; ACL) saves memory, not time. It's already in
  opt-plan.

### CPU: caches and the compiler

#### S16. GCC jump-table threshold [secondary: Kaze, via Wiseguy]
- **What it is:** limiting jump tables to switches with more than ~15 cases (`--param case-values-threshold=15`)
  saved 0.4 ms of SM64's render time, and turning the geometry processor's large switches into one table saved
  another 0.5 ms (dtf.ru summary) *[secondary]*.
- **Why it can help:** a jump table costs a D-cache line from `.rodata` plus an indirect jump; a short compare chain
  costs I-cache.
- **For us:** one flag in `GCC_CODE_CFLAGS`, measured with pcprof's cache counters.
- **Side effects:** none (same semantics).
- **Effort:** trivial.

#### S19. `$gp`-relative small data for the code segment [ours]
- **What it is:** everything builds with `-G 0`, so each global access is `lui` plus a load. `-G 8` for `src/code`
  only would make small globals one instruction: smaller code and fewer I-cache lines.
- **Constraint:** overlays must stay at `-G 0` and must not see those globals as small. OoT's relocator has no
  `GPREL16`, so every extern they use from `src/code` needs care (or a section attribute).
- **Effort:** medium, with risk. Measure the code-size change first.

#### S20. Ranged cache writebacks before RSP tasks [primary: `sched.c`]
- The scheduler calls `osWritebackDCacheAll()` before every task start, about 7 a frame with the audio tasks and
  graphics resumes: 512 cache ops plus the dirty lines, each time.
- Writing back only the lists, matrices and command buffers just built would cost less. Small (<0.1 ms) [ours].
- **Effort:** medium, because a missed range is a corrupted frame.

#### S21. No divide-by-zero checks [primary: Makefile]
- `-mdivide-breaks` puts a check and `break` after every integer divide. `-mno-check-zero-division` drops them: a
  little code size, with a debug aid lost.
- **Effort:** trivial.

#### S23. Link-time optimization for `src/code` [ours]
- GCC `-flto` allows cross-file inlining and dead-code removal. Uncertain on the N64: inlining can cost I-cache, and
  HackerSM64 keeps inlining low on purpose.
- Last in line, after S16 and first pass #10.

### Changes the picture or the sound (for the user's decision)

#### S18. A permanent overscan border **visible** [primary: HackerSM64 `config_rom.h`; ours for the gain]
- **What it is:** HackerSM64's `BORDER_HEIGHT_CONSOLE`: black borders "provide a (small) performance boost" on
  console.
- **For us:** OoT's letterbox machinery already skips the bordered rows in its clears. Eight lines top and bottom are
  6.7% of the screen's pixels, so the same share of per-pixel RDP work: ~2 ms at the heaviest stops [ours]. On a CRT
  most of those lines are in the overscan anyway (the HUD was already moved for a CRT's safe area, d4bc66d).
- **Side effects:** visible on displays that show the whole picture (capture card, HDMI mods).
- **Effort:** trivial.

#### Rejected
- **S-X3, VI anti-aliasing off or point-sampled VI modes:** less VI read bandwidth, but our near props' edge mode
  depends on the VI smoothing from coverage. Visible.
- **S-X4, a smaller framebuffer upscaled by the VI:** visible blur. Already on the "not now" list (resolution
  tricks).
- **S-X5, a variable timestep (GoldenEye / Perfect Dark's `g_Vars.lvupdate240`, primary in the PD decomp):** the game
  runs whatever frame rate it gets at a consistent speed. OoT's logic is fixed-step; this would change gameplay.
- **S-X6, 22,050 Hz, fewer voices, cheaper reverb, mono:** **AUDIO**; first pass #2 and #9 already cover them.

### Commercial games: what was found
- **F-Zero X (60 fps):**
  - "Lack of detail is the price Nintendo EAD willingly paid" (Hardcore Gaming 101) *[secondary]*;
  - no trackside scenery, and fog *[secondary, low-quality sources]*.
  - Nothing technical beyond content cuts.
- **World Driver Championship:** "custom microcode optimization", a 640×480 mode without the Expansion Pak (Wikipedia
  summary) *[secondary]*. No description of the microcode.
- **Factor 5 (Rogue Squadron, Battle for Naboo, Indiana Jones):**
  - streamed level data, textures, animation, audio and code from the cartridge "almost like RAM" (Wikipedia
    summary) *[secondary]*;
  - Indiana Jones had no Z-buffer (OSNews) *[secondary]*: S4's idea.
- **GoldenEye / Perfect Dark:**
  - variable timestep (PD decomp, primary). Not usable (S-X5).
  - Martin Hollis on Perfect Dark's engine limits (Nintendo Life) *[secondary]*: no technique.
- **Banjo-Kazooie:** per-model depth modes (NONE / COMPARE / FULL; shadows and decals compare-only) [primary:
  `src/core2/modelRender.c`]. That is S4's mechanism.
- **Conker, Banjo-Tooie, Jet Force Gemini, Ep. 1 Racer:** nothing technical found (searched for microcode, skinning,
  frame rate and postmortems).

### The full ranked list (second pass only)

Ranked for Lake Hylia as it is now: RDP-bound at the heavy stops, CPU ~36 ms, the invisible-first rule, and audio
bit-exact.

| Rank | Item | Cost it targets | Expected gain [ours unless noted] | Effort | Picture / sound | Basis |
|---|---|---|---|---|---|---|
| 1 | S1 no colour clear under the sky | RDP | 0.91 ms (measured fill) above water | trivial | same where geometry covers (check looking down) | HackerOoT [primary] |
| 2 | S2 Z clear by `SPMemset` after the sky, `SPFlush` | RDP | up to ~0.9 ms | low | identical | F3DEX3, HackerOoT [primary] |
| 3 | S3 sky in 1-cycle when the blend is 0 | RDP | 1-2.5 ms of the sky's 5.3 | low-medium | identical at blend 0 | SETUPDL_40, rdpbench2 [primary basis] |
| 4 | S6 fog cull under water | RDP (diving views) | unknown; count with prof B | medium | near-invisible, capture A/B | F3DEX3 gbi [primary] |
| 5 | S7 batched audio frames, bit-exact | CPU, RSP | part of the audio thread's 60 Hz overhead; first pass #3's gain without the change of sound | medium | exact (audio check), +33 ms latency | Nintendo guide [primary basis] |
| 6 | S5 near props before the room | RDP | depends on overlap (overdraw.py) | low-medium | capture A/B (edges) | Nintendo guide [primary] |
| 7 | S8 DL hints | RDRAM / RSP | ~0.1 ms (F3DEX3's own estimate) | low | identical | F3DEX3 [primary] |
| 8 | S16 jump-table threshold | CPU | ~1% (SM64: 0.4 ms) | trivial | identical | Kaze [secondary] |
| 9 | S10 read the clip counter, maybe a larger `zNear` under water | RSP | unknown until read | trivial to read | `zNear` is visible-risk | F3DEX3 [primary] |
| 10 | S4 far backdrop without Z | RDP above water | ~0.5-1 ms | high | capture A/B | Lambert, Factor 5 [secondary]; BK [primary] |
| 11 | S12 Link's animation frames from RAM | CPU, PI bus | tens of µs a frame | low-medium | identical | OoT [primary] |
| 12 | S11 feet and footprint floors from Link's floor poly | CPU collision | 0.1-0.3 ms | low-medium | exact where it hits (collog) | Kaze [secondary], OoT [primary] |
| 13 | S15 controllers read once per game frame | CPU | 0.1-0.2 ms | low | input timing ±1 retrace | OoT [primary] |
| 14 | S9 vanilla lists repacked for 56 vertices | RSP | only where the RSP limits (tank) | medium | identical | F3DEX3 [primary] |
| 15 | S13 no syncs in texture loads | RDP | 0.2-0.4 ms | medium, risky | risk of texture garbage | F3DEX3, HackerOoT [primary] |
| 16 | S14 pooled fish actors | CPU spikes | spikes, not the mean | medium | exact | OoT [primary basis] |
| 17 | S19 `-G` small data for `src/code` | CPU (I-cache) | ~1-3% of code size | medium, risky | identical | ours |
| 18 | S20 ranged writebacks before RSP tasks | CPU | <0.1 ms | medium | identical | `sched.c` [primary] |
| 19 | S17 PCM samples | RSP audio | RSP only | medium-high | exact if proven | Nintendo guide [primary basis] |
| 20 | S21 no divide checks | CPU | tiny | trivial | identical | Makefile |
| 21 | S22 lazy pose decode | CPU | a few µs per off-screen actor | low | identical | ours |
| 22 | S23 LTO | CPU | uncertain, may lose | medium | identical | ours |
| — | S18 overscan border | RDP | ~6.7% of pixel work (~2 ms at the worst stops) | trivial | **visible** on full-picture displays | HackerSM64 [primary] |

**Against the first pass's shortlist:**
- **S1 + S2** (about 1-1.8 ms of RDP, trivial to low effort, identical picture, shipped by HackerOoT) rank above all
  of its ten items: the lake is RDP-bound, and those are a certain win for an hour's work.
- **S3** ranks above its items 1-3: RDP rather than RSP, and exact.
- **S7** ranks above its items 2 and 3, because it targets the same audio overhead without changing the sound.
- **S6** sits with its item 4 (worth more than its cost, with a check needed).
- The rest come after it.

**Before building anything:**
- the frame log's RDP time above water (S1-S3) and under water (S6);
- F3DEX3 prof B's clip and cull counts (S6, S10);
- `overdraw.py --props` for S5.

### Sources (second pass)

Primary, read here:
- HackerOoT `src/code/z_rcp.c` (`Gfx_ClearZBuffer`, `gUseMemsetForZBuffer`), `src/code/z_play.c` (the clear only
  without a skybox, Z clear after the sky), `src/code/sched.c`, `include/config/config_graphics.h`
  (`ENABLE_F3DEX3_NOSYNCS` caveat): https://github.com/HackerN64/HackerOoT
- F3DEX3 `README.md`, `gbi.h` (`gSPMemset`, `gSPFlush`, `gSPDisplayListHint`, `gSPAlphaCompareCull`,
  `NO_SYNCS_IN_TEXTURE_LOADS`), `docs/Documentation/Porting your Romhack.md`, `Performance.md`:
  https://github.com/HackerN64/F3DEX3
- HackerSM64 `include/config/config_graphics.h`, `config_rom.h` (`BORDER_HEIGHT_CONSOLE`), `config_audio.h`,
  `config_world.h`, `src/audio/data.c` (reverb knobs): https://github.com/HackerN64/HackerSM64
- tiny3d `README.md`, `docs/modelFormat.md` (animation stream), `src/t3d/t3dskeleton.c` (dirty-bone update):
  https://github.com/HailToDodongo/tiny3d
- libdragon `include/rdpq.h` (blocks), `include/rdpq_mode.h` (dithering, Z modes): https://github.com/DragonMinded/libdragon
- Banjo-Kazooie decomp `src/core2/modelRender.c` (per-model depth modes): https://github.com/n64decomp/banjo-kazooie
- Perfect Dark decomp `src/lib/model.c`, `g_Vars.lvupdate240`: https://github.com/n64decomp/perfect_dark
- Nintendo: the performance tuning guide (kantan 4-2): https://ultra64.ca/files/documentation/online-manuals/man-v5-1/kantan/step2/4/4_2.htm ;
  `gDPSetTexturePersp`: https://ultra64.ca/files/documentation/online-manuals/man-v5-1/n64man/gdp/gDPSetTexturePersp.htm ;
  n_audio: https://ultra64.ca/files/documentation/online-manuals/man-v5-2/allman52/ucode/naudio/02.htm
- Our own: `src/code/z_rcp.c` (`Gfx_SetupFrame`), `z_play.c`, `z_vr_box_draw.c`, `z_skelanime.c`, `z_actor.c`
  (`ActorShadow_DrawFeet`), `padmgr.c`, `z_bgcheck.c`, `Makefile`, `tools/texpack.py`,
  `releases/history/console/rdpbench2-20261112.txt`, `releases/history/results/frames6-prelim.md`, hwbench-4/5h.

Secondary:
- Kaze's SM64 render work, step by step (GCC, maths, shadow raycasts, I-cache, bones, jump tables):
  https://dtf.ru/id118330/886840-kak-uskorit-render-mario-64-do-124
- Kaze's channel (titles only; content not reachable): https://www.youtube.com/@KazeN64/videos
- James Lambert's open-world engine (far objects without Z, back to front): https://hackaday.com/2026/03/29/writing-an-open-world-engine-for-the-nintendo-64/ ,
  code https://github.com/lambertjamesd/n64brew2025
- Factor 5 microcode / no Z-buffer: https://www.osnews.com/story/129733/
- F-Zero X: https://www.hardcoregaming101.net/f-zero-x/ ; World Driver Championship: https://en.wikipedia.org/wiki/World_Driver_Championship
- Kaze, extra RAM (the 9th bit): https://timeextension.com/news/2024/12/mario-64-modder-explains-why-n64-has-more-ram-than-you-think
