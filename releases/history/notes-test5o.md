# Test 5o: the area bench (the slow stretch, close up)

## The patch
- **`fish-oot-hwbench-area-5o.bps`**: the area bench (fresh save; send the `.sav`). About 10 minutes a lap; it
  stops by itself after 30 laps (about 5 hours) and shows "BENCH DONE". Any length of run is readable: it saves after
  every spot.

## What it does
The 5n bench found the stretch between the Gerudo river inflow, the west bridge and the rock garden at 12-13 fps,
with the CPU's update over the whole frame. This bench looks at it closely. Adult Link walks a slow circle at 10
spots (8 there: the inflow mouth, both sides of the bridge, under it, on it, the rock garden, the west weed flat, the
river upstream; and the north beach and the drop-off for comparison), four ways at each: in the iron boots on the bed
or in his own boots, each as shipped and with the mod's fish, props, water surface and sky off. Every step of the
game's update is timed (collision, actors, camera, environment and the rest), and every kind of actor's updates.

## Tested first
This exact ROM ran 2.5 hours in ares: 7 full laps, every spot and way recorded, no crash and no stall
(`soak-5o-area-ares.md`). The crash record and the watchdog from 5n are in it too.

## Already seen in ares
The mod's rock collision (3019 triangles on top of the lake's own ~1050, most of them in this stretch) makes every
ground and wall check dearer: without it the update there drops by 6-17 ms a frame (`area-collision-ab-ares.md`).
The console run confirms it on the hardware, and shows the rest (the Tektites, the Guays, the camera, the fish).
