# texbench1: the stress benches with and without the lossless texture pass

What `--texpack` (tools/texpack.py) buys on the console. The same two stress benches as stress1 (Lake Hylia's fish let
into the lab tank 6 more a step; the kelp forest 40 more plants a step, three passes each), each built twice from the
commit that adds this file, the same in every way but the textures:

| patch | build |
|---|---|
| **`fish-oot-texbench1-fish-hw.bps`** | `fishmod/build.sh --script stress_fish --hw` |
| **`fish-oot-texbench1-fish-texpack-hw.bps`** | `... --script stress_fish --hw --texpack` |
| **`fish-oot-texbench1-plants-hw.bps`** | `... --script stress_plants --hw` |
| **`fish-oot-texbench1-plants-texpack-hw.bps`** | `... --script stress_plants --hw --texpack` |

`--texpack`: the game's 16-bit textures with no more than 256 colours (1,793 of 2,975) as 8-bit colour-indexed ones
with a palette of exactly their colours (every pixel the same; each load moves about a third less), and the pipe
syncs with nothing to wait for (3,413) removed. Pictures, play and timing of the game itself don't change.

Console, **blank save** for each (each writes over the save). About 10 minutes each, **STRESS DONE** at the end. Send
the four `.sav` files; `fishmod/tools/stress_report.py SAVE` reads them. The columns to compare are **RDP** (and RSP):
the CPU's should come out about the same. The plant bench starts at 40 plants, which stress1 found already over the
frame: its RDP times still compare, step by step.

## Checked in ares
- Every converted texture decodes through its palette to exactly the original texels (1,793 of 1,793).
- The regression scripts' state fingerprints match the normal build's (23 of 23); Lake Hylia's frames match but for
  the debug text and the RDP's dither noise, which follows timing.
- Both texpack benches run and record their steps.

ares's RDP timings aren't the console's (stress1: 7.5 against 28 ms), so the measuring is the console's.
