# Study: N64 performance, fish AI and fishing feel

Three parts, researched in parallel:

- [study-n64.md](study-n64.md): how the N64 works where it matters, and what Kaze, HackerSM64/HackerOoT and the
  F3DEX3 author do about it.
- [study-ai.md](study-ai.md): efficient fish AI. What vanilla OoT, Twilight Princess and Majora's Mask fish actually
  do (read from the decomps), steering and schooling on N64 budgets, 3D navigation without pathfinding, and a
  proposed AI v2.
- [study-mechanics.md](study-mechanics.md): what makes acclaimed fishing feel good (OoT, TP, Sega Bass Fishing,
  FFXV, RDR2, Sea of Thieves and others), what players criticise, and how it maps to the N64 pad.

The research sandbox couldn't open many websites, so claims from articles and videos come from search summaries
(the parts say which). The decomp code and the F3DEX3 and ares sources were read directly.

## What the three agree on

**The fish actors are CPU-bound, and the cost isn't the maths.**

- A fish costs ~0.63 ms (update + draw), about 59,000 CPU cycles. The likely causes:
  - cache misses: the 8 KB data cache is far smaller than our 2.7 KB-per-fish actor working set;
  - per-actor engine overhead: light binding and matrix push/pop per limb;
  - in the AI, the floor probes and bank raycasts.
- The lab tank uses ~46 of the 50 ms frame on the CPU, so render-side work (F3DEX3, AA, textures) can't help until
  the CPU side shrinks.

**Vanilla already shows the way.**

- OoT's own pond fish steer with real yaw *and* pitch and "bump and slide" off walls.
- Its 60 small fish aren't actors at all: compact structs in one array, swimming in 3 schools.
- TP re-aims only every 4 frames and has fish learn to ignore lures.

**Feel comes from reading the fish, not a meter.**

- The fish is the readout, and the decisions are where to cast, what bait, and how to work it.
- The bite has a rhythm and three cues (bobber, sound, rumble).
- The fight is driven by the fish: steer against its run, reel while it's tired.
- Failure says why it happened, and rarity comes from knowledge, not dice.
- Avoid abstract bars, rings, prompts, mashing and stick spinning.

## Combined plan, in order

### Performance (kokiri-test4/5)

1. **Profile first.**
   - Use ares's cache-miss profiler and per-call-site CPU counters on fish update and draw.
   - Find out whether the frame is CPU- or RCP-bound.
   - Everything below gets measured against this.
2. **AI hot path:**
   - Replace the per-move floor probes, open-water search and bank raycasts with one look-ahead and a cached whisker.
   - Swap `cosf` for `Math_CosS`, and use squared-distance compares instead of `sqrtf`.
3. **Draw overhead:**
   - `ACTOR_FLAG_IGNORE_POINT_LIGHTS` on fish.
   - Fewer limbs on small and far fish.
   - One-axis limb bends without a full rotation.
4. **A fish manager:**
   - Run all fish phase by phase (AI, motion, pose, draw sorted by species).
   - Keep hot data in a compact array, and bind lights once.
   - This is the likely big win, and the base for items 5–6 below.

### AI v2 (on top of the manager)

5. **Real 3D swimming:**
   - A yaw + pitch heading as in vanilla's pond fish.
   - A central flock array with separation for dense packing.
   - Small species as school particles (vanilla's trick), promoted to full fish when one goes for the bait.
6. **Habitats and obstacles:**
   - Weed beds, rocks and logs as simple shapes (spheres, capsules).
   - Later, a baked floor + ceiling + shore-distance field from the collision, made with the asset pipeline.
7. **Senses and mood:**
   - A sight cone and a short "lateral line" for lure motion, plus scent for bait.
   - Hunger and fear, with time and weather multipliers.
   - Shy bits per bait, and fishing pressure per spot.

### Fishing feel (alongside, one at a time)

8. **Make fish visibly react:** notice, follow, inspect, strike, with a clear "interested" tell.
9. **Retrieve style as the lure skill:**
   - steady (hold A), twitch (tap), jerk (stick sweep), stop-and-go (release A);
   - each species has a preference, and many strike on the pause.
10. **Bite rhythm:** nibble, nibble, BITE on the bobber, sound and rumble. The hook-set window opens instantly.
11. **The fight:**
    - Steer against the run and reel in the tired windows; the line breaks only from what the player did.
    - Rumble and rod bend are the gauge, with a creak near breaking.
    - Telegraphed jumps (ease off) and head shakes.
12. **Failure messages that name the cause; knowledge-gated rare fish** (time, rain, spot, bait), hinted in the world
    and the journal.

### Not now

Inline assembly, fixed-point matrices, resolution tricks, engine-wide collision rewrites, occlusion planes, triangle
snakes, voxel or distance-field navigation, full boids for big fish, fast inverse square root, line wear as a
consumable.
