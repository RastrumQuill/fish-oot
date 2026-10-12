# Research: N64 performance, fish AI and fishing feel

Three parts, researched in parallel:

- [n64.md](n64.md): how the N64 works where it matters, and what Kaze, HackerSM64/HackerOoT and the
  F3DEX3 author do about it.
- [fish-ai.md](fish-ai.md): efficient fish AI. What vanilla OoT, Twilight Princess and Majora's Mask fish actually
  do (read from the decomps), steering and schooling on N64 budgets, 3D navigation without pathfinding, and a
  proposed AI v2.
- [fishing-feel.md](fishing-feel.md): what makes acclaimed fishing feel good (OoT, TP, Sega Bass Fishing,
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

## What came of it

The plan that came out of this is in the [roadmap](../roadmap.md) (Next). The fish manager, rigs and LOD ladder
were built in kokiri-test4.

Older notes on Twilight Princess and Sega Bass Fishing: [fishing-games.md](fishing-games.md).
