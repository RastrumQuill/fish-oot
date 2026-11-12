# fourway2: the four-way bench with a code guard

fourway1 crashed on the console about 10 seconds in ("Coprocessor unusable", PC 80019EE0). Mapped with that build's
ELF, 80019EE0 is ordinary code (the return point in `EffectSs_Spawn` after it calls `EffectSs_FindSlot`): an
instruction that can't raise that exception. So the code there had been **overwritten in RAM**. The same ROM runs
cleanly in ares, so whatever writes there only does it on the console.

fourway2 is the same bench with two catches for the writer:
- **A CPU watchpoint** on that address (physical 0x19EE0). If a CPU store hits it, the game stops at once with
  **"Watchpoint"** on the crash screen, and its PC is the instruction that did it. A photo of that screen is enough.
- **A code guard**: checksums of all the game's code, re-checked a few kilobytes a frame (a full pass every few
  seconds). If something else changed the code (the RSP's or the RDP's writes, a DMA), the game stops with
  **"code changed at ..."**, and the save holds the changed block, the frame and where the bench was.

Also: the occlusion plane's maths now refuse a NaN or infinity before converting to integers (the console traps on
those; ares may not).

Built from the commit that adds this file, with `fishmod/build.sh --script hwbench_fourway --hw`. Checked in ares
for 200 s: no false alarm.

## Running it
**Blank save** (it writes over the save files). If it stops, photograph the crash screen (hold the button combo if
it's a frozen picture) and send the `.sav`. If it doesn't crash, it's the normal four-way bench: send the `.sav`
whenever you stop it. Read with `fishmod/tools/hwbench_report.py SAVE`.
