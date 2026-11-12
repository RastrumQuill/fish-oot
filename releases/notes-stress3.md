# stress3: the stress benches judged by the screen, on frames10's code

**`fish-oot-stress3.bps`** (`fishmod/build.sh --script stress_prof --hw --colpack --gcc-all`): stress2 (see
[its notes](notes-stress2.md)) on frames10's code (see [its notes](notes-frames10.md)), with the bench fixed:

- **A step is judged by what reached the screen:** full speed when the pictures came at 19.7 fps or better and no
  more than 1 in 20 stayed 4 refreshes or longer. stress2 judged by frames whose period was over 52 ms, which run-ahead's
  third frame buffer mostly hides: it stopped the fish at 42 while every step to 42 was at 20 fps on screen. Each
  step's screen counts come as a `FISHMOD-STRESS-SCREEN` line too.
- **The fish bench's last step no longer takes in the warp to Lake Hylia** (its tag was left on: stress2's "42 fish,
  18.4 fps" was a 2-second loading frame).
- The fish bench's pass sums no longer carry over to the plants.

stress2 extrapolated about 75-80 fish (all close and fully drawn) before the RSP reaches the frame; this run should
find the real number. `fishmod/tools/stress_prof.py FOLDER --elf ELF` (it now shows the on-screen rate and the late
pictures per step).

## Results (console, 12 Nov, run 20261112-1202)

**72 fish, all close to the camera in the tank, hold 20 fps with no late pictures; 78 within the test (5 late of
320); 84-90 just under (19.6-19.7 fps); 96 clearly not (17.4 fps).** stress2 had extrapolated 75-80. At 96 the CPU
(51 ms), the RSP (44 ms) and the RDP (47 ms) all reach the frame together. Per extra fish from 48 to 90: CPU about
+0.4 ms, RSP +0.25, RDP +0.2. Plants: 40 hold 20 fps; past that the RDP alone (as stress2).

The bench's own screen counts (FISHMOD-STRESS-SCREEN) and the frame records agree step by step.
