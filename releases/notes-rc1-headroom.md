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
- Two scores for each difference. **LPIPS** (the distance between the two frames in a trained network's features,
  fitted to what people judge more different; AlexNet, the whole frame, x1000, less the control's): the number to
  calibrate to. **FLIP** (fishmod/tools/flip_metric.py: contrast sensitivity by detail size at a viewing distance,
  perceived colour difference, edges and points; an error per pixel, summed over the patches that change): the check
  on it, and what says where on the screen the pop is. FLIP here is written from the paper, not NVIDIA's code: its
  ranking is what to trust, its absolute numbers should be checked against `flip-evaluator` once.
- The control (the same view drawn twice) is taken off both: LPIPS by subtraction, FLIP pixel by pixel (a pixel
  counts beyond twice the control's error around it). LPIPS under about 1 for a kind is within that noise.
- It writes `report.md` (per stop; per kind its worst stop and total), the pairs as pictures with the patches lit,
  and `pops.json`.

**To recalibrate with it:** a kind's distances are right when its strength at the stops is under the line you choose
(not when "leaving it out changes no view"). Lower a tier's distance or lengthen the dissolve, run it again, and the
number says whether the pop got quieter and what it cost in the other direction (a kind drawn nearer as its card
shows up as a bigger patch at its next transition). The step and the scale are at the top of the tool and the script
(`POP_AB_STEP`); other views are other tour stops.

About 95 seconds a stop in ares (35 pictures each).

### What it finds on this branch (releases/pop-ab-rc1-headroom.md, the tour's 22 stops)

- One set's worth of travel (24 units): LPIPS 0.0 to 0.9 at 17 stops, 1.6 to 4.7 at five (4, 10, 11, 14, 21: beds
  seen from above and the shallows), where FLIP also finds patches.
- By kind, 150 units of travel, LPIPS summed over the stops (FLIP's sum beside it): grass_tuft 59.9 (785), bush_weed
  23.6 (396), tall_weed 13.4 (271), cattails 10.1 (420), vine_strand 9.6 (352), reed_clump 8.8 (383), fan_weed 7.2
  (91), ribbon_weed 7.0 (303); the rest 5.5 and under. The two scores put the same kinds on top: a recalibration
  should start with grass_tuft and bush_weed.
- The pictures show what kind of change each is: for bush_weed under water it is the single card against the model
  (the card's shape is not the model's), not the plant appearing.
