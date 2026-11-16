# rc1-headroom: the props' and fish's CPU work on rc1, and the pop-in test

Branch `claude/rc1-headroom`: rc1, the far single cards' fix (releases/notes-rc1-popin.md), and on top:

## The CPU work brought over (from claude/fish-prop-headroom, which was on the frames20 tree)

| | rc1 | this branch |
|---|---|---|
| the props' draw, its own stage timers, the pcprof_frames route in ares (ms a frame) | 7.3 | 3.6 |
| of that: making sets / the frame's commands / sway / plants making way | 3.3 / 3.0 / 0.6 / 0.3 | 1.3 / 1.6 / 0.3 / 0.3 |
| frames that make a new set | 50% | 19% |

- **Brought over:** a set held for a 4 degree turn or a 24 unit move; a frame's commands kept whole while nothing they
  depend on changes (the single cards' dissolve worked out with the set); materials as display lists of their own;
  the command buffer's cache lines claimed first; only the sway matrices in use remade; the far school followers
  every 2nd or 4th frame; the bubbles' matrix written directly.
- **Changed for rc1:** the water fog's key (new in rc1) took the camera's height in quarter units, so a camera that
  bobs remade every frame's commands; it's whole units from 40 up now (under 2.5% of the fog's slope).
- **Not brought over:** Scatter_Build's one-pass rewrite and the 16-byte per-prop entries. rc1's loop has the per-prop
  water fog and the bending plants in it, and its touch pass was already made cheap in rc1. They were about a sixth
  of the gain on frames20.
- **What still remakes a frame's commands:** a new set (as meant), and the water fog's key on about half the frames
  (the camera's height over the water, and the water's colour as the hour moves). Keeping the fog out of the props'
  commands (its numbers in a small list of the frame's own, called from them) would let them be kept through that.

Checked: the tour's 22 stops against the fixed rc1's, side by side (the same plants; the differences are the water
and the sway).

## The pop-in test (`--script pop_ab`, `fishmod/tools/pop_ab_capture.py`)

popin_check.py asks whether every prop that should be in the set is: it couldn't see the far cards' bug (every prop
was in the set, and drew nothing), and it can't say how much a pop shows. The LOD calibration compares the game's view
with a reference view, a threshold per pixel: it measures a standing difference, not the change at the moment of a
pop, and it was fooled the same way (a card that draws nothing differs from nothing).

This test looks at the picture, at the moment of the pop:

- At every tour stop the scene is frozen (as edge_ab's), and the same view is drawn again with the props' distances
  taken nearer by a step: how far each kind is drawn, its tier, its dissolve, the bed cards. The camera doesn't move.
  What differs between the two pictures is exactly what pops in, changes tier or finishes dissolving over that much
  travel: no parallax, no water, no sway.
- The turns: every kind 24 units nearer (one set's worth: what a single new set changes on screen), every kind 150
  nearer, then each kind alone 150 nearer (which kind it is).
- The score is how the eye takes it: both frames in Oklab, blurred by a pixel, the distance per pixel (dE x 100);
  pixels over dE 2 gathered into touching patches (a pop each: its size, mean and peak dE, where); a variant's strength
  is dE summed over its patches. As a first scale: under 40 not seen, 40 to 250 a flicker, over 250 a pop. Patches
  that nowhere reach dE 3 are left out (the dissolve's dither: kinds not in view scored 7 to 30 with them in).
- It writes `report.md` (per stop; per kind its worst stop and total), the pairs as pictures with the patches lit,
  and `pops.json`.

**To recalibrate with it:** a kind's distances are right when its strength at the stops is under the line you choose
(not when "leaving it out changes no view"). Lower a tier's distance or lengthen the dissolve, run it again, and the
number says whether the pop got quieter and what it cost in the other direction (a kind drawn nearer as its card
shows up as a bigger patch at its next transition). The step and the scale are at the top of the tool and the script
(`POP_AB_STEP`); other views are other tour stops.

About 95 seconds a stop in ares (35 pictures each).

### What it finds on this branch (releases/pop-ab-rc1-headroom.md, the tour's 22 stops)

- One set's worth of travel (24 units) is not seen at 13 of the 22 stops and a pop at 6 (stops 3, 4, 10, 11, 19, 21:
  the beds seen from above and the shallows).
- By kind, 150 units of travel: grass_tuft is by far the worst (its worst stop 10,152, a flicker or more at 12 stops),
  then bush_weed (4,559), cattails (3,966), reed_clump (3,199), tall_weed (2,558); vine_strand, broadleaf and
  ribbon_weed around 500 to 1,100; the rest under 450 or not seen. Those five are where a recalibration should start.
- The pictures show what kind of change each is: for bush_weed under water it is the single card against the model
  (the card's shape is not the model's: pop_s01_v07.png), not the plant appearing.
- The control (the same view drawn twice) is 0 at 14 stops and up to 571 at the others (the cut-outs' edges shimmer
  between two drawings); a pixel counts only beyond twice the control's difference around it.
