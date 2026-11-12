# Test 5: Lake Hylia on the console

Three patches. Apply each to your NTSC 1.2 (US) ROM as before. All of them need the Expansion Pak, and each is the
same ROM for emulators and the console.

## 1. `fish-oot-hwbench.bps`: the overnight bench

Leave it running; it needs no input.

- It boots into Lake Hylia and goes round the 22 tour stops. At each stop it times about half a minute of frames in five
  modes:
  - **as shipped**;
  - **4-bit textures** for the props;
  - **cold caches**: both caches filled with other data before every update;
  - **no props**;
  - **no fish**.
- After each stop it runs a few seconds of memory tests: load latency, and read, write and copy speed, each sorted by
  whether the RDP was busy or idle.
- The line at the bottom says where it is: `bench lap 3 stop 12 tex4`. A lap takes about 15 minutes.
- Everything it measures is added up in SRAM after every stop, so the SummerCart writes it to the `.sav`. If it's
  stopped and started again, it carries on adding to what's there. Delete the `.sav` to start from zero.
- It also records the RDP's own hardware counters: cycles busy fetching commands or waiting on memory, cycles
  drawing, and cycles loading textures. No emulator has these; they're the main reason to run it on the console.
- This ROM's `.sav` is only bench data: nothing in it is a game save.

Send back the `.sav` after a night (or any length: more laps only means steadier numbers).
`fishmod/tools/hwbench_report.py` turns it into the report.

## 2. `fish-oot-kokiri-test5.bps`: the test, with Lake Hylia as adult Link

The Kokiri test as before: a new file skips the intro and starts with the rod, bottles and so on. Changes:

- **D-pad Down** warps to Lake Hylia as **adult Link**, with the lake full. He's wearing the **Iron Boots** and the
  **Zora Tunic**, so he won't drown while you test under water. Take the boots off in the equipment screen to swim.
  He has the Master Sword and Hylian Shield.
- **D-pad Up** warps back to Kokiri Forest as **child Link**, with the Kokiri Sword, Deku Shield and his own tunic and
  boots.
- **Bottom of the screen:** the frame rate. Under it, the RDP's own counters, as shares of its cycles:
  - **b**: command buffer busy (fetching from RAM, or held up);
  - **p**: pipeline drawing;
  - **t**: loading textures;
  - **tex8** / **tex4**: which prop textures are in use.
- **D-pad Right:** switches the props' textures between 8 bits and 4 bits. You'll hear a click. 4 bits means half the
  RAM read per texture load, decoded by the RDP at no CPU cost. On the PC I couldn't tell them apart.
- **D-pad Left:** hides or shows the RDP line.

Worth trying it at the heavy places under the water: the west weed flat, the island shallows, and the south reed bank.
Note the frame rate and the b/p/t numbers with each texture setting. Collisions and interactions as adult (walking the
bed in the boots, the dock, rocks, fishing from the shore and the dock) are worth a check too.

The game runs at 20 fps at most, so the frame rate only shows a difference where a frame takes too long. The bench's
CPU, RSP and RDP times show the headroom below that.

## 3. `fish-oot-lake-tour.bps`: the camera tour

The tour of the 22 stops, as before (A next, B back, stick to look), on the final layout:

- the lusher placement (the same layout every build now);
- the east-shore lily field, moved onto water shallow enough for it;
- the props' cards cut to their textures' outlines (the same picture, about a fifth less to fill);
- the draw distances measured on the game's own frames, per zone of the lake.
