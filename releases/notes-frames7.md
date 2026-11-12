# frames7: the morning runner's first build, audio layout

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
