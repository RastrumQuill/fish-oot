# texbench1: the stress benches with and without the lossless texture pass

What `--texpack` (tools/texpack.py) buys on the console. One ROM runs both stress benches, one after the other: the
fish bench (Lake Hylia's fish let into the lab tank 6 more a step), then it moves to Lake Hylia by itself for the
plant bench (the kelp forest in the deep basin, 40 more plants a step), three passes each. Built twice from the commit
that adds this file, the same in every way but the textures:

| patch | build |
|---|---|
| **`fish-oot-texbench1-base-hw.bps`** | `fishmod/build.sh --script stress_both --hw` |
| **`fish-oot-texbench1-texpack-hw.bps`** | `fishmod/build.sh --script stress_both --hw --texpack` |

`--texpack`: the game's 16-bit textures with no more than 256 colours (1,793 of 2,975) as 8-bit colour-indexed ones
with a palette of exactly their colours (every pixel the same; each load moves about a third less), and the pipe
syncs with nothing to wait for (3,413) removed. Pictures, play and timing of the game itself don't change.

Console, **blank save** for each (it writes over the save). About 20 minutes each: the fish bench, a fade to Lake
Hylia, the plant bench, then **STRESS DONE**. Send the two `.sav` files; `fishmod/tools/stress_report.py SAVE` prints
both benches from each. The columns to compare are **RDP** (and RSP): the CPU's should come out about the same. The
plant bench starts at 40 plants, which stress1 found already over the frame: its RDP times still compare, step by step.

## Checked in ares
- Every converted texture decodes through its palette to exactly the original texels (1,793 of 1,793).
- The regression scripts' state fingerprints match the normal build's (23 of 23); Lake Hylia's frames match but for
  the debug text and the RDP's dither noise, which follows timing.
- The two-bench ROM runs the fish bench, moves to Lake Hylia, runs the plant bench and saves both (a shortened build:
  one pass, short steps).

ares's RDP timings aren't the console's (stress1: 7.5 against 28 ms), so the measuring is the console's.
