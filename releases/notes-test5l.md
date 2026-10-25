# Test 5l: the play log

## The patch
- **`fish-oot-playlog-5l.bps`**: the Lake Hylia test build as you play it (F3DEX3), with a recorder. Play normally, on
  a new file or your existing save; the saves aren't touched. Spend time at Lake Hylia wherever it slows down: walking
  the shore, swimming, fishing, in rain and at night if you can. The log is written every 30 seconds of play at the
  lake, so stay there half a minute after the last slow bit before switching off. Then send the `.sav`.

- **`fish-oot-hwbench-roam-5l.bps`**: the hardware bench, now roaming Lake Hylia (needs a fresh save; send the
  `.sav`). Instead of sitting at fixed views, the camera flies from each tour stop to the next, panning and tilting as
  it goes, above and under water. Each leg is flown once in each of eight modes: as shipped; **fish frozen** (no AI
  or swimming, still drawn); fish not drawn; no fish; no props; no water surface; no sky; all of those off. It stops
  by itself after 12 laps (about 8 hours) and says "BENCH DONE".

Either is useful alone; the play log is the quicker one.

## Why
The hardware bench flies a camera round fixed stops on a clear morning, and it ran at 20 fps there, but play at the
lake drops to 13-15 fps. Play adds Link, the rod, the game's own camera, swimming, the clock and the weather. This
build records at the lake, for each second: the frame time split into CPU update, CPU draw, RSP and RDP; the fish's
own share; and where Link and the camera were, the time, the weather, and whether you were swimming, fishing or under
water. It keeps the slowest 24 seconds (one per spot) and totals for each situation, so the fix can go where the time
actually goes.

## Also in this build
- **No moss mats** at the lake (they flickered); their budget went back to plants.
- **Short grass tilts** with the slope of the lake bed.
