# Pop-in at the tour's stops

Strength: FLIP's error (0 to 1 a pixel: how visible the change is to someone watching, at 20 pixels a degree) summed over the patches that change, at 320x240: a 30-pixel plant appearing at 0.3 is 9. Under 3 not seen, 3 to 25 a flicker, over 25 a pop (a first scale: look at the pairs and move it).

LPIPS: the whole frame's distance x1000, less the control's (the same view drawn twice): the number to calibrate to. FLIP: the sanity check, and where on the screen.

## Every kind together

| stop | 24 nearer (one set): LPIPS | FLIP | | 150 nearer: LPIPS | FLIP | | biggest patch at 150 | the control: LPIPS, FLIP |
|---|---|---|---|---|---|---|---|---|
| 1 | 0.3 | 1.1 | not seen | 2.1 | 8.5 | a flicker | 22 px at FLIP 0.179 (peak 0.209), at [71, 145] | 0.6, 0.0 |
| 4 | 0.0 | 0.0 | not seen | 0.2 | 3.0 | not seen | 7 px at FLIP 0.12 (peak 0.162), at [68, 128] | 0.3, 0.9 |
| 10 | 1.4 | 49.6 | a pop | 4.7 | 176.1 | a pop | 129 px at FLIP 0.241 (peak 0.344), at [70, 56] | 0.6, 31.8 |
| 11 | 0.2 | 3.1 | a flicker | 1.1 | 35.2 | a pop | 145 px at FLIP 0.129 (peak 0.19), at [25, 136] | 0.3, 0.0 |
| 12 | 0.2 | 0.0 | not seen | 1.5 | 7.3 | a flicker | 29 px at FLIP 0.138 (peak 0.188), at [187, 60] | 0.4, 0.0 |
| 14 | 0.1 | 0.5 | not seen | 0.1 | 0.5 | not seen | 4 px at FLIP 0.13 (peak 0.176), at [255, 5] | 0.5, 0.0 |
| 16 | 0.0 | 0.0 | not seen | 0.0 | 0.0 | not seen | - | 0.6, 0.0 |
| 19 | 0.3 | 8.6 | a flicker | 0.7 | 22.9 | a flicker | 49 px at FLIP 0.171 (peak 0.288), at [11, 137] | 0.3, 0.0 |
| 20 | 0.2 | 18.1 | a flicker | 2.0 | 122.7 | a pop | 271 px at FLIP 0.214 (peak 0.701), at [160, 92] | 0.4, 18.6 |
| 21 | 0.1 | 0.0 | not seen | 0.1 | 0.0 | not seen | - | 0.4, 0.0 |
| 22 | 0.3 | 16.3 | a flicker | 2.6 | 103.4 | a pop | 138 px at FLIP 0.377 (peak 0.73), at [243, 85] | 0.4, 28.0 |

## Each kind alone, 150 units nearer

| kind | worst stop (by LPIPS) | LPIPS there | FLIP there | | LPIPS summed over the stops | FLIP summed | stops where FLIP says a flicker or more |
|---|---|---|---|---|---|---|---|
| grass_tuft | 1 | 1.2 | 7.9 | a flicker | 2.4 | 47.2 | 4 |
| lily_flower | 22 | 1.4 | 13.7 | a flicker | 2.1 | 22.5 | 2 |
| big_pondweed | 10 | 0.8 | 50.6 | a pop | 2.0 | 101.0 | 4 |
| tall_weed | 12 | 1.5 | 6.9 | a flicker | 1.8 | 8.6 | 1 |
| bush_weed | 11 | 0.6 | 31.3 | a pop | 1.6 | 51.9 | 3 |
| spatterdock | 10 | 0.8 | 19.0 | a flicker | 1.6 | 29.9 | 2 |
| cattails | 22 | 1.2 | 57.1 | a pop | 1.5 | 57.1 | 1 |
| broadleaf | 10 | 0.4 | 14.9 | a flicker | 1.3 | 26.5 | 2 |
| rock_lit | 10 | 0.9 | 31.3 | a pop | 1.1 | 32.4 | 1 |
| ribbon_weed | 10 | 0.4 | 25.0 | a flicker | 1.1 | 27.2 | 1 |
| float_leaves | 20 | 0.6 | 16.8 | a flicker | 1.0 | 18.8 | 1 |
| vine_strand | 10 | 0.5 | 1.2 | not seen | 1.0 | 10.0 | 0 |
| lily_pad | 22 | 0.2 | 0.6 | not seen | 0.8 | 1.7 | 0 |
| iris | 20 | 0.4 | 47.9 | a pop | 0.8 | 49.6 | 1 |
| pond_reed | 10 | 0.4 | 13.8 | a flicker | 0.8 | 14.4 | 1 |
| leaf_litter | 10 | 0.2 | 0.5 | not seen | 0.4 | 2.7 | 0 |
| kelp_frond | 20 | 0.1 | 1.8 | not seen | 0.4 | 2.2 | 0 |
| dock_deck | 20 | 0.1 | 1.1 | not seen | 0.2 | 2.9 | 0 |

## Crossing the surface

The view with the haze half in (the moment the props' distances change sides), drawn with the over-water distances and with the under-water ones: what jumps in that one frame. `cross_sSS.png`.

| stop | LPIPS | FLIP | |
|---|---|---|---|
| 1 | 0.8 | 10.8 | a flicker |
| 4 | 0.1 | 2.7 | not seen |
| 10 | 0.2 | 0.7 | not seen |
| 11 | 0.4 | 8.8 | a flicker |
| 12 | 0.0 | 0.0 | not seen |
| 14 | 0.0 | 0.0 | not seen |
| 16 | 0.0 | 0.0 | not seen |
| 19 | 0.1 | 2.5 | not seen |
| 20 | 0.1 | 0.0 | not seen |
| 21 | 0.0 | 0.0 | not seen |
| 22 | 0.1 | 1.1 | not seen |

The pairs: `pop_sSS_vVV.png` (the view, the view with the distances nearer, the patches lit), for every variant FLIP calls a flicker or more. `pops.json` has every patch.
