# Test 5d: cheaper far plants, the tour as a tour

## What changed
- **Far plants draw their whole cards, uncut.** The second bench showed plants past 500 units cost twice what the
  near ones do. They're a few pixels each, so their cost is per triangle, and cutting the cards to their outlines had
  tripled the triangles (grass tufts 16 to 40, bush weed 12 to 54). Past 500 units the uncut cards are used now: the
  same picture, a third of the triangles. Near plants keep the cut cards, where pixels are what count.
- **Lake Hylia's intro cutscene is marked as seen** in the test files and the tour/bench, so it no longer takes the
  camera on the first warp there.
- **The tour:** the captions describe each place instead of asking questions, a frame-rate counter sits at the top
  right, and the controls hint fits inside the box.
- **Bench crash trail:** moved to SRAM 0x7000. It was at 0x7E00, which the game's profile also uses.

## 1. `fish-oot-hwbench3.bps` (and `-gcc`)
The overnight bench, with a fresh `.sav`. The modes:
- base (as shipped);
- altedge (old edge mode);
- trimfar (far plants cut to outlines, as before: what the uncut far cards save);
- nonear, nofar, noprops, nosky, noxlu.

The 1-cycle and no-depth-write modes are gone; the last bench ruled both out. Run the GCC one another night to
compare CPU times.

## 2. `fish-oot-lake-tour-5d.bps`
The tour, with the frame-rate counter. On the console, this shows directly what the far-plant change does at each
stop.

## 3. `fish-oot-kokiri-test5d.bps`
The Kokiri / Lake Hylia test with these changes (IDO build).
