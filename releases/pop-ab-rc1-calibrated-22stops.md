# Pop-in at the tour's stops

Strength: FLIP's error (0 to 1 a pixel: how visible the change is to someone watching, at 20 pixels a degree) summed over the patches that change, at 320x240: a 30-pixel plant appearing at 0.3 is 9. Under 3 not seen, 3 to 25 a flicker, over 25 a pop (a first scale: look at the pairs and move it).

LPIPS: the whole frame's distance x1000, less the control's (the same view drawn twice): the number to calibrate to. FLIP: the sanity check, and where on the screen.

## Every kind together

| stop | 24 nearer (one set): LPIPS | FLIP | | 150 nearer: LPIPS | FLIP | | biggest patch at 150 | the control: LPIPS, FLIP |
|---|---|---|---|---|---|---|---|---|
| 1 | 0.2 | 0.8 | not seen | 2.2 | 9.7 | a flicker | 30 px at FLIP 0.164 (peak 0.206), at [71, 144] | 0.7, 0.0 |
| 2 | 0.0 | 0.0 | not seen | 0.0 | 0.0 | not seen | - | 0.2, 0.0 |
| 3 | 0.4 | 19.9 | a flicker | 0.6 | 40.9 | a pop | 30 px at FLIP 0.332 (peak 0.453), at [205, 135] | 0.2, 14.6 |
| 4 | 0.0 | 0.0 | not seen | 0.0 | 0.9 | not seen | 7 px at FLIP 0.13 (peak 0.163), at [67, 141] | 0.2, 0.0 |
| 5 | 0.0 | 0.0 | not seen | 0.2 | 3.7 | a flicker | 13 px at FLIP 0.151 (peak 0.239), at [285, 110] | 0.2, 4.0 |
| 6 | 0.0 | 0.0 | not seen | 0.0 | 0.0 | not seen | - | 0.3, 0.0 |
| 7 | 0.0 | 0.0 | not seen | 1.1 | 44.7 | a pop | 37 px at FLIP 0.339 (peak 0.715), at [22, 147] | 0.1, 0.0 |
| 8 | 0.0 | 0.0 | not seen | 0.1 | 11.7 | a flicker | 27 px at FLIP 0.143 (peak 0.246), at [258, 207] | 0.1, 0.0 |
| 9 | 0.4 | 1.2 | not seen | 1.6 | 9.5 | a flicker | 57 px at FLIP 0.143 (peak 0.187), at [148, 116] | 0.6, 0.7 |
| 10 | 1.5 | 58.2 | a pop | 3.8 | 165.8 | a pop | 131 px at FLIP 0.251 (peak 0.389), at [70, 57] | 0.7, 30.4 |
| 11 | 0.0 | 9.2 | a flicker | 0.9 | 45.2 | a pop | 145 px at FLIP 0.134 (peak 0.195), at [25, 136] | 0.4, 0.5 |
| 12 | 0.4 | 0.9 | not seen | 4.0 | 38.8 | a pop | 69 px at FLIP 0.141 (peak 0.238), at [258, 122] | 0.5, 0.0 |
| 13 | 0.1 | 0.0 | not seen | 0.1 | 0.0 | not seen | - | 0.5, 0.0 |
| 14 | 0.0 | 0.0 | not seen | 0.1 | 0.0 | not seen | - | 0.5, 0.0 |
| 15 | 0.0 | 0.0 | not seen | 0.2 | 9.4 | a flicker | 26 px at FLIP 0.215 (peak 0.57), at [27, 92] | 0.1, 0.7 |
| 16 | 0.0 | 0.0 | not seen | 0.0 | 0.0 | not seen | - | 0.5, 0.0 |
| 17 | 0.7 | 34.6 | a pop | 1.2 | 82.6 | a pop | 110 px at FLIP 0.153 (peak 0.268), at [232, 164] | 0.1, 0.0 |
| 18 | 0.1 | 5.9 | a flicker | 0.9 | 80.6 | a pop | 62 px at FLIP 0.237 (peak 0.652), at [6, 121] | 0.3, 11.2 |
| 19 | 0.2 | 4.3 | a flicker | 0.6 | 22.0 | a flicker | 52 px at FLIP 0.138 (peak 0.171), at [232, 146] | 0.3, 0.6 |
| 20 | 0.1 | 20.1 | a flicker | 2.0 | 129.6 | a pop | 271 px at FLIP 0.223 (peak 0.743), at [160, 91] | 0.4, 26.6 |
| 21 | 0.1 | 0.0 | not seen | 0.1 | 0.0 | not seen | - | 0.4, 0.0 |
| 22 | 0.1 | 17.8 | a flicker | 2.2 | 114.2 | a pop | 131 px at FLIP 0.437 (peak 0.829), at [242, 87] | 0.4, 38.7 |

## Each kind alone, 150 units nearer

| kind | worst stop (by LPIPS) | LPIPS there | FLIP there | | LPIPS summed over the stops | FLIP summed | stops where FLIP says a flicker or more |
|---|---|---|---|---|---|---|---|
| kelp_frond | 12 | 2.6 | 30.9 | a pop | 3.8 | 72.5 | 2 |
| lily_flower | 22 | 1.0 | 13.3 | a flicker | 3.2 | 59.2 | 4 |
| bush_weed | 9 | 0.8 | 9.1 | a flicker | 3.1 | 84.0 | 6 |
| grass_tuft | 1 | 1.4 | 7.6 | a flicker | 3.0 | 98.6 | 7 |
| ribbon_weed | 10 | 0.6 | 26.6 | a pop | 2.5 | 81.7 | 5 |
| tall_weed | 12 | 1.3 | 12.9 | a flicker | 2.4 | 27.8 | 2 |
| big_pondweed | 10 | 0.7 | 46.9 | a pop | 2.2 | 148.7 | 7 |
| cattails | 22 | 1.3 | 64.6 | a pop | 1.8 | 79.9 | 3 |
| rock_lit | 9 | 0.5 | 0.0 | not seen | 1.7 | 51.2 | 3 |
| broadleaf | 16 | 0.5 | 0.0 | not seen | 1.7 | 57.8 | 3 |
| spatterdock | 10 | 0.7 | 19.3 | a flicker | 1.6 | 50.1 | 5 |
| float_leaves | 20 | 0.9 | 15.7 | a flicker | 1.6 | 30.1 | 2 |
| leaf_litter | 16 | 0.5 | 0.0 | not seen | 1.2 | 15.6 | 2 |
| pebble_patch | 16 | 0.6 | 0.0 | not seen | 1.2 | 21.4 | 3 |
| sunken_log | 16 | 0.6 | 0.0 | not seen | 1.1 | 14.8 | 2 |
| vine_strand | 16 | 0.5 | 0.0 | not seen | 1.0 | 16.7 | 2 |
| lily_centre | 16 | 0.5 | 0.0 | not seen | 1.0 | 15.4 | 2 |
| pond_reed | 16 | 0.5 | 0.0 | not seen | 1.0 | 8.4 | 1 |
| iris | 20 | 0.2 | 50.6 | a pop | 0.9 | 65.9 | 3 |
| rock | 7 | 0.3 | 8.7 | a flicker | 0.8 | 19.2 | 2 |
| lily_pad | 7 | 0.2 | 5.4 | a flicker | 0.8 | 22.2 | 3 |
| marigold | 7 | 0.2 | 5.3 | a flicker | 0.7 | 14.2 | 2 |
| dock_posts | 7 | 0.2 | 5.5 | a flicker | 0.6 | 15.9 | 2 |
| reed_clump | 7 | 0.3 | 8.8 | a flicker | 0.5 | 20.1 | 2 |

The pairs: `pop_sSS_vVV.png` (the view, the view with the distances nearer, the patches lit), for every variant FLIP calls a flicker or more. `pops.json` has every patch.
