# Pop-in at the tour's stops

Strength: FLIP's error (0 to 1 a pixel: how visible the change is to someone watching, at 20 pixels a degree) summed over the patches that change, at 320x240: a 30-pixel plant appearing at 0.3 is 9. Under 3 not seen, 3 to 25 a flicker, over 25 a pop (a first scale: look at the pairs and move it).

LPIPS: the whole frame's distance x1000, less the control's (the same view drawn twice): the number to calibrate to. FLIP: the sanity check, and where on the screen.

## Every kind together

| stop | 24 nearer (one set): LPIPS | FLIP | | 150 nearer: LPIPS | FLIP | | biggest patch at 150 | the control: LPIPS, FLIP |
|---|---|---|---|---|---|---|---|---|
| 1 | 0.3 | 0.8 | not seen | 2.1 | 14.9 | a flicker | 31 px at FLIP 0.17 (peak 0.205), at [71, 144] | 0.7, 0.0 |
| 4 | 0.0 | 0.9 | not seen | 0.2 | 3.9 | a flicker | 3 px at FLIP 0.299 (peak 0.362), at [314, 155] | 0.3, 0.6 |
| 10 | 1.0 | 49.2 | a pop | 4.7 | 157.4 | a pop | 134 px at FLIP 0.232 (peak 0.356), at [70, 56] | 0.8, 25.1 |
| 11 | 0.1 | 3.4 | a flicker | 1.0 | 30.8 | a pop | 116 px at FLIP 0.131 (peak 0.191), at [25, 136] | 0.3, 0.0 |
| 12 | 0.3 | 0.0 | not seen | 1.8 | 6.8 | a flicker | 29 px at FLIP 0.138 (peak 0.188), at [187, 60] | 0.5, 0.0 |
| 14 | 0.1 | 0.5 | not seen | 0.1 | 0.5 | not seen | 4 px at FLIP 0.13 (peak 0.176), at [255, 5] | 0.5, 0.0 |
| 16 | 0.0 | 0.0 | not seen | 0.0 | 0.0 | not seen | - | 0.6, 0.0 |
| 19 | 0.2 | 4.7 | a flicker | 0.5 | 12.7 | a flicker | 32 px at FLIP 0.126 (peak 0.159), at [232, 149] | 0.3, 0.0 |
| 20 | 0.2 | 18.3 | a flicker | 1.4 | 120.4 | a pop | 270 px at FLIP 0.216 (peak 0.701), at [160, 92] | 0.4, 16.8 |
| 21 | 0.1 | 0.0 | not seen | 0.1 | 0.0 | not seen | - | 0.4, 0.0 |
| 22 | 0.4 | 20.4 | a flicker | 2.8 | 107.7 | a pop | 143 px at FLIP 0.375 (peak 0.745), at [243, 83] | 0.5, 33.9 |

## Each kind alone, 150 units nearer

| kind | worst stop (by LPIPS) | LPIPS there | FLIP there | | LPIPS summed over the stops | FLIP summed | stops where FLIP says a flicker or more |
|---|---|---|---|---|---|---|---|
| grass_tuft | 1 | 1.2 | 12.0 | a flicker | 2.1 | 45.1 | 4 |
| tall_weed | 12 | 1.7 | 6.9 | a flicker | 1.8 | 9.4 | 1 |
| cattails | 22 | 1.2 | 58.6 | a pop | 1.6 | 60.1 | 1 |
| lily_flower | 22 | 1.2 | 13.9 | a flicker | 1.5 | 21.2 | 2 |
| big_pondweed | 10 | 0.6 | 38.8 | a pop | 1.5 | 88.5 | 4 |
| rock_lit | 10 | 0.8 | 31.5 | a pop | 1.5 | 33.0 | 1 |
| vine_strand | 10 | 0.8 | 0.0 | not seen | 1.3 | 8.6 | 1 |
| bush_weed | 11 | 0.5 | 26.3 | a pop | 1.2 | 49.5 | 3 |
| broadleaf | 10 | 0.4 | 11.0 | a flicker | 1.0 | 23.0 | 2 |
| spatterdock | 10 | 0.5 | 16.4 | a flicker | 0.7 | 30.0 | 2 |
| float_leaves | 20 | 0.4 | 12.8 | a flicker | 0.5 | 15.4 | 1 |
| fan_weed | 20 | 0.2 | 0.7 | not seen | 0.5 | 1.9 | 0 |
| lily_centre | 20 | 0.3 | 0.5 | not seen | 0.5 | 2.5 | 0 |
| lily_pad | 22 | 0.3 | 0.6 | not seen | 0.5 | 3.5 | 0 |
| iris | 20 | 0.4 | 47.9 | a pop | 0.4 | 49.2 | 1 |
| branch | 20 | 0.2 | 0.5 | not seen | 0.4 | 2.6 | 0 |
| rush_tuft | 22 | 0.2 | 0.5 | not seen | 0.3 | 0.5 | 0 |
| pond_reed | 10 | 0.1 | 12.1 | a flicker | 0.3 | 12.1 | 1 |
| kelp_frond | 22 | 0.2 | 0.6 | not seen | 0.3 | 2.6 | 0 |
| vine_curtain | 20 | 0.1 | 0.6 | not seen | 0.2 | 1.8 | 0 |
| moss_patch | 22 | 0.1 | 1.7 | not seen | 0.2 | 4.3 | 0 |
| reed_clump | 22 | 0.1 | 2.3 | not seen | 0.2 | 2.9 | 0 |
| bank_roots | 22 | 0.1 | 2.1 | not seen | 0.2 | 2.6 | 0 |
| dock_posts | 22 | 0.1 | 0.8 | not seen | 0.2 | 0.8 | 0 |
| leaf_litter | 22 | 0.1 | 0.5 | not seen | 0.2 | 0.5 | 0 |

## Crossing the surface

The view with the haze half in (the moment the props' distances change sides), drawn with the over-water distances and with the under-water ones: what jumps in that one frame. `cross_sSS.png`.

| stop | LPIPS | FLIP | |
|---|---|---|---|
| 1 | 0.9 | 16.3 | a flicker |
| 4 | 0.1 | 1.8 | not seen |
| 10 | 0.0 | 1.3 | not seen |
| 11 | 0.5 | 10.4 | a flicker |
| 12 | 0.0 | 0.0 | not seen |
| 14 | 0.0 | 0.0 | not seen |
| 16 | 0.0 | 0.0 | not seen |
| 19 | 0.3 | 3.9 | a flicker |
| 20 | 0.0 | 0.0 | not seen |
| 21 | 0.0 | 0.0 | not seen |
| 22 | 0.0 | 0.8 | not seen |

The pairs: `pop_sSS_vVV.png` (the view, the view with the distances nearer, the patches lit), for every variant FLIP calls a flicker or more. `pops.json` has every patch.
