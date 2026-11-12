# stress1: how much the console holds at 20 fps

Two stress benches, to gather numbers before going back to Lake Hylia. Each raises one thing a step at a time until
the game can't hold full speed (20 fps), three passes, and saves after every step. Both are on today's game
(1-cycle plants, the cheaper fish update, the memset and matrix fixes). Built from the commit that adds this file
with `fishmod/build.sh --script stress_fish --hw` and `--script stress_plants --hw`.

## The patches (console; **blank save**, each writes over the save files)
- **`fish-oot-stress-fish1-hw.bps`**: adult Link in the Iron Boots at the bottom of the Lakeside Laboratory's tank;
  Lake Hylia's fish let into the tank 6 more every 13 seconds (they swim and think as in the lake, all close to the
  camera: the worst case for drawing them).
- **`fish-oot-stress-plants1-hw.bps`**: Link in the Iron Boots on the bare floor of Lake Hylia's deep basin, the scene's
  own plants off, and a kelp forest ahead of him (the tall underwater kinds, full models, drawn kind by kind and nearest
  first, as the game draws its plants), 40 more plants every 10 seconds.

Each takes about 10 minutes and says **STRESS DONE** at the end (the step and count show on screen as it goes). Send
both `.sav` files.

## What counts as full speed
A step holds full speed if its average frame is within 20 fps **and** no more than 1 frame in 20 takes over 52 ms
(an average alone hides a stutter: a slow frame and a quick one after it). A pass ends one step after the second step
that misses.

## What it records, per step
The frame rate, the CPU's update and draw, the RSP and the RDP, the fish's own update and draw, the worst frame and the
share of slow frames. `fishmod/tools/stress_report.py SAVE` prints the table and the largest count that held 20 fps;
the CPU, RSP and RDP columns say which one runs out first.

Checked in ares (both run their steps and save; ares's timings aren't the console's).
