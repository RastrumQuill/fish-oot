# Hardware test 1: the bench and the 4-bit texture switch

Two patches, both for the original-hardware (console) build. Apply to your NTSC 1.2 (US) ROM as before. Both need the
Expansion Pak.

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

## 2. `fish-oot-lake-live-test.bps`: playable, with the switch

Test mode as before (D-pad Down warps to Lake Hylia, D-pad Up to Kokiri Forest), plus:

- **Bottom of the screen:** the frame rate. Under it, the RDP's counters as shares of its cycles:
  - **b**: command buffer busy (fetching from RAM, or held up);
  - **p**: pipeline drawing;
  - **t**: loading textures;
  - **tex8** / **tex4**: which prop textures are in use.
- **D-pad Right:** switches the props' textures between 8 bits and 4 bits. You'll hear a click. 4 bits means half
  the RAM read for each texture load, decoded by the RDP at no CPU cost; the textures show 16 shades instead of 256.
- **D-pad Left:** hides or shows the RDP line.

Worth trying it at the heavy places: the west weed flat under the water, the island shallows, and the south reed bank.
Note the frame rate and the b/p/t numbers with each setting.

The game runs at 20 fps at most, so the frame rate only shows a difference where a frame takes too long. The bench's
CPU, RSP and RDP times show the headroom below that.
