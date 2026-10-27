# Test 5m: the play bench again, with a crash record

## The patch
- **`fish-oot-hwbench-play-5m.bps`**: the same play bench as 5l (fresh save; send the `.sav`; about 7 hours, then
  "BENCH DONE").

## Why again
The 5l run stopped after about 70 seconds, in the first leg (the fifth timing block, "no fish"), before it had saved
any results; the same build runs on past that point in the emulator. On the console a crash shows its screen only
after a button combination, so it looks like a frozen picture.

## What changed
- **Results are saved after every timing block**, not only after each leg: a run cut short still keeps what it timed.
- **A crash or a hang is written into the save**: which thread, where it stopped, and what the bench was doing. If it
  stops again, send the `.sav` as it is and say what the screen showed (frozen picture, black, or an error screen).

## From the 5l save
About 40 seconds of the first leg (Link in the iron boots on the west weed flat, under water): 20 fps on average, CPU
update 25.6 ms + display lists 15.1 ms, RSP 23.4 ms, RDP 33.4 ms; the fish's updates 5.4 ms on average, with one frame
at 52 ms.
