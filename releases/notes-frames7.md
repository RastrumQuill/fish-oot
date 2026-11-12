# frames7: the morning runner's first build, audio layout, the sample store

**`fish-oot-frames7.bps`** (`fishmod/build.sh --script pcprof_frames --hw --colpack --gcc-all`): frames6 plus the
changes below. It runs the same route and sends the same USB frame log and profile. It also works with
`fishmod\pc\morning.bat` (see `fishmod/pc/MORNING.md`).

## What changed

| change | checked |
|---|---|
| Answers `sc64deployer upload --reboot`: Halt (it stops touching the cart) and Reboot (it starts the newly uploaded ROM's boot code, as a reset would) | ares: a forced reboot restarts the game cleanly, 4 times in a row |
| Says `FISHMOD-START` and `FISHMOD-DONE` over USB, so the runner knows when a run begins and ends | ares |
| Audio: a byte for each note's "enabled" bit, so the per-buffer scans read one byte, not a 224-byte note | audio check: every sample alike (8,253 buffers) |
| Audio: each sequence channel's per-tick fields (player, layers, delay) packed at the front, in one or two cache lines instead of four | audio check: every sample alike (8,253 buffers) |
| Audio: the sample store. The 870 KB between the frame buffers (unused since frames6) keeps a copy of each sample played from the cart, loaded in the background, then played from RAM | audio check: every sample alike (7,593 buffers). ares bench: cart sample reads 1,812,521 -> 1,476; 61 samples, 632 KB of the store; frame 52.0 -> 50.6 ms, slow frames 28% -> 13% |

## Memory over the bench route (ares, the lowest free at any frame)

| heap | size | most used | least free |
|---|---|---|---|
| system heap | 5.07 MB | 4.31 MB | 653 KB (one block) |
| play state's Zelda arena (inside the system heap) | 2.12 MB | 1.26 MB | 880 KB |
| sample store | 870 KB | 632 KB | 238 KB |
