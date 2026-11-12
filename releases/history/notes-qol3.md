# qol3: a day clock under the minimap

qol2 (see [its notes](notes-qol2.md)) plus a clock under the overworld minimap: an arc from midnight (left) through
noon (top) to midnight (right) in the sky's colours (night blue, dawn rose, gold noon, dusk orange), with the sun by
day and the moon by night riding it at the time. No numbers. It fades with the minimap (L hides both), and hides where time is frozen (the scene's time speed is 0).

The minimap is raised just as far as each map needs for the clock to end above a CRT's safe area (most maps: a few
pixels or not at all); Link's arrow and the entrance marks move with it. Our own art: `fishmod/tools/gen_clock_tex.py`.

Not yet: the clock in dungeons and in places without a minimap.

Patches: `fish-oot-qol3-testmode.bps` (emulators), `fish-oot-qol3-hw-testmode.bps` (console).
