# Pop-in at the tour's stops

Strength: FLIP's error (0 to 1 a pixel: how visible the change is to someone watching, at 20 pixels a degree) summed over the patches that change, at 320x240: a 30-pixel plant appearing at 0.3 is 9. Under 3 not seen, 3 to 25 a flicker, over 25 a pop (a first scale: look at the pairs and move it).

LPIPS: the whole frame's distance x1000, less the control's (the same view drawn twice): the number to calibrate to. FLIP: the sanity check, and where on the screen.

## Every kind together

| stop | 24 nearer (one set): LPIPS | FLIP | | 150 nearer: LPIPS | FLIP | | biggest patch at 150 | the control: LPIPS, FLIP |
|---|---|---|---|---|---|---|---|---|
| 1 | 0.0 | 1.2 | not seen | 26.0 | 384.8 | a pop | 318 px at FLIP 0.246 (peak 0.402), at [148, 135] | 4.4, 33.6 |
| 2 | 0.0 | 0.0 | not seen | 0.0 | 0.0 | not seen | - | 0.2, 0.0 |
| 3 | 0.4 | 25.8 | a pop | 0.5 | 45.6 | a pop | 40 px at FLIP 0.351 (peak 0.457), at [205, 138] | 0.2, 11.7 |
| 4 | 3.3 | 69.6 | a pop | 6.4 | 239.2 | a pop | 162 px at FLIP 0.266 (peak 0.375), at [163, 136] | 1.2, 34.6 |
| 5 | 0.0 | 0.6 | not seen | 0.2 | 8.6 | a flicker | 24 px at FLIP 0.209 (peak 0.341), at [302, 127] | 0.2, 2.0 |
| 6 | 0.0 | 0.0 | not seen | 0.0 | 0.0 | not seen | - | 0.3, 0.0 |
| 7 | 0.0 | 0.0 | not seen | 1.1 | 44.7 | a pop | 37 px at FLIP 0.339 (peak 0.715), at [22, 147] | 0.1, 0.0 |
| 8 | 0.0 | 0.0 | not seen | 0.1 | 11.8 | a flicker | 27 px at FLIP 0.143 (peak 0.246), at [258, 207] | 0.1, 0.0 |
| 9 | 0.0 | 0.0 | not seen | 0.8 | 0.0 | not seen | - | 0.7, 0.0 |
| 10 | 1.6 | 60.7 | a pop | 5.0 | 188.0 | a pop | 131 px at FLIP 0.251 (peak 0.389), at [70, 57] | 1.0, 46.7 |
| 11 | 4.7 | 54.0 | a pop | 17.8 | 432.7 | a pop | 266 px at FLIP 0.27 (peak 0.435), at [102, 128] | 2.2, 51.4 |
| 12 | 0.0 | 0.0 | not seen | 1.4 | 64.6 | a pop | 134 px at FLIP 0.178 (peak 0.329), at [293, 36] | 1.7, 23.3 |
| 13 | 0.0 | 0.0 | not seen | 0.1 | 4.5 | a flicker | 21 px at FLIP 0.163 (peak 0.235), at [311, 186] | 0.5, 0.0 |
| 14 | 3.2 | 31.6 | a pop | 6.1 | 83.3 | a pop | 134 px at FLIP 0.143 (peak 0.279), at [222, 84] | 1.0, 0.9 |
| 15 | 0.0 | 0.0 | not seen | 0.2 | 8.4 | a flicker | 26 px at FLIP 0.215 (peak 0.57), at [27, 92] | 0.1, 1.2 |
| 16 | 0.0 | 0.0 | not seen | 29.5 | 368.6 | a pop | 1200 px at FLIP 0.17 (peak 0.371), at [50, 93] | 1.4, 6.7 |
| 17 | 0.7 | 34.6 | a pop | 1.2 | 82.5 | a pop | 110 px at FLIP 0.153 (peak 0.268), at [232, 164] | 0.1, 0.0 |
| 18 | 0.1 | 5.9 | a flicker | 0.9 | 81.8 | a pop | 52 px at FLIP 0.245 (peak 0.652), at [6, 121] | 0.3, 10.5 |
| 19 | 0.9 | 11.5 | a flicker | 27.2 | 502.5 | a pop | 545 px at FLIP 0.277 (peak 0.49), at [123, 177] | 4.0, 113.4 |
| 20 | 0.1 | 22.1 | a flicker | 15.3 | 818.4 | a pop | 1513 px at FLIP 0.257 (peak 0.869), at [138, 78] | 0.5, 27.1 |
| 21 | 3.2 | 11.8 | a flicker | 7.5 | 50.8 | a pop | 78 px at FLIP 0.155 (peak 0.294), at [16, 154] | 5.0, 114.5 |
| 22 | 0.3 | 23.6 | a flicker | 2.2 | 121.9 | a pop | 145 px at FLIP 0.421 (peak 0.821), at [242, 84] | 0.5, 43.9 |

## Each kind alone, 150 units nearer

| kind | worst stop (by LPIPS) | LPIPS there | FLIP there | | LPIPS summed over the stops | FLIP summed | stops where FLIP says a flicker or more |
|---|---|---|---|---|---|---|---|
| grass_tuft | 19 | 22.4 | 366.8 | a pop | 59.9 | 785.3 | 11 |
| bush_weed | 11 | 8.8 | 152.1 | a pop | 23.6 | 396.3 | 10 |
| tall_weed | 16 | 11.4 | 245.2 | a pop | 13.4 | 271.0 | 3 |
| cattails | 20 | 8.5 | 338.5 | a pop | 10.1 | 419.8 | 4 |
| vine_strand | 11 | 6.1 | 216.0 | a pop | 9.6 | 351.6 | 2 |
| reed_clump | 20 | 6.8 | 344.4 | a pop | 8.8 | 382.5 | 5 |
| fan_weed | 14 | 6.3 | 81.7 | a pop | 7.2 | 91.0 | 2 |
| ribbon_weed | 19 | 2.5 | 85.8 | a pop | 7.0 | 303.4 | 9 |
| big_pondweed | 21 | 3.0 | 9.9 | a flicker | 5.5 | 177.8 | 9 |
| broadleaf | 1 | 2.9 | 26.2 | a pop | 5.1 | 137.0 | 7 |
| kelp_frond | 16 | 2.4 | 67.6 | a pop | 4.3 | 112.0 | 2 |
| lily_flower | 7 | 1.0 | 27.5 | a pop | 2.9 | 64.2 | 4 |
| sunken_log | 12 | 0.9 | 0.0 | not seen | 2.8 | 21.8 | 3 |
| rush_tuft | 1 | 1.3 | 2.0 | not seen | 2.0 | 6.0 | 0 |
| rock_lit | 9 | 0.7 | 0.0 | not seen | 1.9 | 54.4 | 3 |
| dock_posts | 11 | 1.0 | 2.2 | not seen | 1.9 | 21.6 | 2 |
| float_leaves | 20 | 0.7 | 15.7 | a flicker | 1.8 | 32.9 | 3 |
| old_posts | 11 | 0.7 | 0.0 | not seen | 1.6 | 4.4 | 0 |
| lily_centre | 11 | 0.3 | 0.9 | not seen | 1.4 | 17.1 | 2 |
| rock | 12 | 0.6 | 0.0 | not seen | 1.3 | 23.7 | 2 |
| spatterdock | 10 | 0.4 | 19.3 | a flicker | 1.2 | 53.2 | 5 |
| lily_pad | 11 | 0.3 | 0.9 | not seen | 1.1 | 25.7 | 3 |
| pebble_patch | 7 | 0.3 | 8.6 | a flicker | 0.8 | 22.6 | 2 |
| marigold | 7 | 0.2 | 5.3 | a flicker | 0.7 | 16.7 | 2 |

The pairs: `pop_sSS_vVV.png` (the view, the view with the distances nearer, the patches lit), for every variant FLIP calls a flicker or more. `pops.json` has every patch.
