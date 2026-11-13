# Verifying skeletons and animations: proving a rig change draws exactly the same

Before any change to how a skeleton is drawn (its limb display lists cleaned, limbs merged under one matrix, its
vertices merged, its animation code made faster), this proves the change draws **exactly** what it drew before:
every pose of every animation, from several angles, with every variant the actor can show. Repeatable: the same
inputs every run, logs compared byte for byte, no human judgement. Link first, then any actor.

## What "the same" means, at three levels

A change passes only if all three agree. Each catches what the one before can't.

1. **The pose** (CPU). For every animation frame, the matrix the game computes for every limb (the `Mtx` it writes
   into the display list, bytes) and the limb's joint values. A change to the animation code or the matrix maths must
   leave these byte-identical. A change that merges limbs under one matrix is checked here too: the merged limb's
   vertices, put through its new matrix, must land where the old matrix put them (exactly, in the RSP's fixed point).
2. **The draw stream** (what the RSP is asked to draw). Every display list command run for the actor, walked as the
   RSP walks it (calls followed, segments resolved), reduced to a stream of draws: each triangle's three vertices
   (position, texture coordinates, colour or normal, as bytes), the matrix in effect, and the full RSP and RDP state
   at that moment (combiner, render mode, geometry mode, textures and tiles, texture memory contents, colours,
   lights). Commands that change nothing aren't in the stream, so a cleaned list gives the same stream as the
   original: this is the fish rig's proof (`fishmod/tools/rigcheck.py`) applied to whole actors.
3. **The picture**. Each pose drawn on a plain backdrop (no sky, water or effects: nothing that animates), from fixed
   camera angles, captured in ares as the VI outputs it, compared pixel for pixel. Catches anything the models above
   miss (an RSP or RDP behaviour the state model doesn't know).

## The harness: a test ROM that poses the actor (repeatable)

A script build (`fishmod/scripts/animcheck.inc.c`, in the style of `audlog` and `collog`), so the A and B ROMs differ
only in the change under test:

- It loads a fixed scene with nothing else drawn (an empty room, or the test map; the sky off, the lights fixed:
  a fixed ambient and one fixed directional light, the time of day frozen) and spawns the actor under test.
- It **does not play** the animations in real time. It steps through a table of cases, one per frame of the game:
  each case sets the skeleton to an exact pose (an animation and a frame: `Animation_*` with the frame set
  directly, or Link's own `LinkAnimation_*` path with its frame data loaded from `link_animetion`) and draws it once
  through the actor's own draw function, so its limb callbacks (attached items, the face, equipment) run as in play.
- The cases cover, for each animation: every whole frame; the half frames between them (interpolation); and blends
  between pairs of animations at fixed weights (the morphs the game does on transitions), sampled.
- Per case it logs, into RAM (dumped through ares' GDB server, as `collog.sh` does): the case number, the hash of
  the limb matrices (level 1), and the hash of the draw stream (level 2). Optionally, per case, the screenshot marker
  (level 3; see below).
- Comparison: `fishmod/tools/animcheck_compare.py A.bin B.bin` prints the first case that differs (animation, frame,
  which limb or which draw), or that all N cases are identical.

The walker for level 2 runs **in the ROM** (so the log is small: one hash per case), with its state model written
once and shared: the same rules as `rigcheck.py` (written separately from the optimization it checks, so a mistake in
one isn't repeated in the other). Its Python twin (in `dlstat.py`) checks it against RAM dumps of a few cases, so the
two models agree.

## Link specifically

Link is drawn by `Player_Draw` with many variants. The case table covers them as axes, every combination that the
game can show sampled at least once:

- **Age**: child and adult (different skeletons, `gLinkChildSkel` / `gLinkAdultSkel`).
- **Animations**: every Link animation in `gameplay_keep` and the `link_animetion` table (several hundred), every
  frame. The table is generated from the extracted assets (`assets/objects/gameplay_keep`, `assets/misc/link_animetion`),
  so none is missed.
- **Equipment** (drawn through the limb callbacks): each sword (none, Kokiri, Master, Biggoron, broken), each shield
  (none, Deku, Hylian, Mirror), sheathed and in hand; the item in hand (each one that changes the hand's display list:
  bottle, bow, slingshot, hookshot, hammer, boomerang, ocarina, the fishing rod...); boots and tunics (colours);
  masks (child).
- **Face**: every eye and mouth texture index (the face is texture-swapped).
- **Lighting**: the case table runs twice, with two fixed light directions (Link is lit: his normals matter).
- **Angles** (level 3 only): front, back, side, above, from a fixed distance.

Several thousand cases at one a frame: a few minutes in ares per ROM. Run A (before) and B (after) in parallel.

## What exists already and what's to build

| Piece | Status |
|---|---|
| Logs in RAM dumped through ares' GDB server (`collog.sh`, `--audio`) | exists: add an `--anim` mode |
| A scripted test-scene boot (the scripts framework, `Fishmod_DebugBoot`) | exists |
| The display-list walker and state model (Python: `dlstat.py`, `rigcheck.py`) | exists: to port its state model to C for the in-ROM walker |
| Raw-frame capture in ares with markers (`edge_ab_capture.py`, the ARES_AUTOSHOT patch) | exists: generalise to "capture marker N" |
| The case table generator (animations from the extracted assets, the variant axes) | to build |
| The harness script (`animcheck.inc.c`): posing, drawing, hashing, logging | to build |
| `animcheck_compare.py` | to build |
| A self-test: a deliberate one-pixel change (a texture coordinate off by one in one limb) must be caught by level 2 and 3, and a deliberate no-op change (a redundant state command added) must pass | to build, run once per harness change |

## How a skeleton change is accepted

1. Build A (before) and B (after) with the `animcheck` script.
2. Run both in ares; compare level 1 and level 2 logs: every case identical.
3. Capture level 3 for both; every pixel identical.
4. Then it goes into a bench build for the console (the speed), never before.

A change that is meant to look different (a lower-detail model in the distance) doesn't use this: it's compared by
eye, and said so.
