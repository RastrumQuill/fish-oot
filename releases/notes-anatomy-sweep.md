# The anatomy sweep

**`fish-oot-anatomy-sweep.bps`** (`fishmod/build.sh --script anatomy_sweep --hw --colpack --meshpack --gcc-all`):
frames19's code on the sweep bench: 50 laps of clear dawn (about 3.3 hours), every other lap as shipped, between them
one part of the frame left out. Read it with `fishmod/tools/anatomy_sweep_report.py RUN_FOLDER`.

Why: the RDP sweep's five parts (props, fish, water surface, sky, HUD) add up to 14 of its 44 ms; this takes apart the
other 30:
- **no terrain**: the room's opaque geometry (the lake bed, banks and cliffs);
- **no Link**;
- **no other actors** (all but Link and the fish);
- **no effects** (splashes, dust, sparkles: Effect_DrawAll, EffectSs_DrawAll);
- **nothing**: all of these and all of the RDP sweep's at once: what remains is the frame's own cost (clears, setup).
The report also gives the RDP's idle time, its frame time less its pipeline's busy time (waiting for the RSP's
commands, or for the frame to start), as shipped and with each part out.

Checked in ares: lap 0 as shipped; lap 1 with the terrain left out (the water surface, props and Link drawn, no
ground or cliffs).
