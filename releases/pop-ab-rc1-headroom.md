# Pop-in at the tour's stops

Strength: the sum of dE over the patches that differ (dE x pixels at 320x240); under 40 not seen, 40 to 250 a flicker, over 250 a pop.

## Every kind together

| stop | 24 units nearer (one set) | | 150 units nearer | | biggest patch at 150 | the control's own (left out of the others) |
|---|---|---|---|---|---|---|
| 1 | 0 | not seen | 7241 | a pop | 650 px at dE 4.5 (peak 8.8), at [146, 133] | 0 |
| 2 | 0 | not seen | 0 | not seen | - | 0 |
| 3 | 831 | a pop | 1005 | a pop | 119 px at dE 7.1 (peak 10.7), at [204, 135] | 15 |
| 4 | 2705 | a pop | 4125 | a pop | 233 px at dE 5.4 (peak 10.2), at [78, 150] | 71 |
| 5 | 0 | not seen | 107 | a flicker | 17 px at dE 3.0 (peak 3.7), at [280, 116] | 0 |
| 6 | 0 | not seen | 0 | not seen | - | 0 |
| 7 | 0 | not seen | 160 | a flicker | 15 px at dE 4.3 (peak 6.9), at [24, 147] | 0 |
| 8 | 0 | not seen | 0 | not seen | - | 0 |
| 9 | 0 | not seen | 0 | not seen | - | 0 |
| 10 | 283 | a pop | 1458 | a pop | 104 px at dE 3.9 (peak 5.5), at [71, 57] | 98 |
| 11 | 1912 | a pop | 7705 | a pop | 353 px at dE 5.0 (peak 8.6), at [102, 128] | 71 |
| 12 | 0 | not seen | 319 | a pop | 53 px at dE 2.8 (peak 4.0), at [294, 47] | 0 |
| 13 | 0 | not seen | 28 | not seen | 11 px at dE 2.6 (peak 3.3), at [312, 187] | 0 |
| 14 | 29 | not seen | 335 | a pop | 42 px at dE 2.9 (peak 3.7), at [267, 13] | 0 |
| 15 | 0 | not seen | 87 | a flicker | 16 px at dE 3.9 (peak 6.7), at [28, 93] | 0 |
| 16 | 0 | not seen | 4205 | a pop | 280 px at dE 4.5 (peak 7.5), at [143, 91] | 0 |
| 17 | 0 | not seen | 29 | not seen | 9 px at dE 3.2 (peak 5.3), at [52, 33] | 0 |
| 18 | 0 | not seen | 145 | a flicker | 18 px at dE 3.0 (peak 4.1), at [24, 128] | 0 |
| 19 | 278 | a pop | 11883 | a pop | 652 px at dE 6.0 (peak 13.2), at [123, 173] | 285 |
| 20 | 73 | a flicker | 8250 | a pop | 215 px at dE 7.1 (peak 23.9), at [146, 78] | 207 |
| 21 | 738 | a pop | 1736 | a pop | 67 px at dE 6.4 (peak 10.6), at [72, 144] | 571 |
| 22 | 136 | a flicker | 1804 | a pop | 138 px at dE 10.6 (peak 25.8), at [243, 85] | 185 |

## Each kind alone, 150 units nearer

| kind | worst stop | its strength | | patches there | stops where it's a flicker or more | total over the stops |
|---|---|---|---|---|---|---|
| grass_tuft | 19 | 10152 | a pop | 8 | 12 | 19204 |
| bush_weed | 11 | 4559 | a pop | 8 | 8 | 11902 |
| cattails | 20 | 3966 | a pop | 8 | 3 | 5539 |
| reed_clump | 20 | 3199 | a pop | 8 | 3 | 3522 |
| tall_weed | 16 | 2558 | a pop | 8 | 1 | 2611 |
| vine_strand | 11 | 1128 | a pop | 8 | 3 | 1582 |
| broadleaf | 21 | 788 | a pop | 8 | 5 | 1593 |
| ribbon_weed | 11 | 533 | a pop | 2 | 5 | 1797 |
| iris | 20 | 410 | a pop | 6 | 2 | 531 |
| rock_lit | 10 | 400 | a pop | 1 | 1 | 505 |
| fan_weed | 14 | 297 | a pop | 4 | 1 | 418 |
| big_pondweed | 11 | 296 | a pop | 3 | 5 | 855 |
| spatterdock | 10 | 213 | a flicker | 4 | 2 | 398 |
| float_leaves | 20 | 181 | a flicker | 4 | 2 | 271 |
| lily_flower | 7 | 163 | a flicker | 5 | 5 | 602 |
| kelp_frond | 16 | 136 | a flicker | 1 | 1 | 170 |
| pond_reed | 10 | 116 | a flicker | 2 | 2 | 210 |
| lily_centre | 21 | 84 | a flicker | 2 | 1 | 171 |
| vine_curtain | 3 | 66 | a flicker | 3 | 1 | 140 |
| dock_deck | 21 | 65 | a flicker | 2 | 1 | 105 |
| lily_pad | 15 | 62 | a flicker | 1 | 1 | 133 |
| bank_roots | 19 | 52 | a flicker | 2 | 2 | 145 |
| pebble_patch | 17 | 50 | a flicker | 1 | 1 | 139 |
| rock | 17 | 50 | a flicker | 1 | 1 | 139 |
| dock_posts | 19 | 47 | a flicker | 2 | 1 | 106 |
| branch | 19 | 43 | a flicker | 2 | 1 | 75 |
| rush_tuft | 19 | 42 | a flicker | 3 | 1 | 68 |
| moss_patch | 19 | 42 | a flicker | 2 | 1 | 94 |
| sunken_log | 19 | 41 | a flicker | 2 | 1 | 124 |
| marigold | 22 | 40 | not seen | 1 | 0 | 112 |
| leaf_litter | 17 | 29 | not seen | 1 | 0 | 111 |
| old_posts | 19 | 10 | not seen | 1 | 0 | 10 |

The pairs: `pop_sSS_vVV.png` (the view, the view with the distances nearer, the patches lit), for every variant of strength 40 or more. `pops.json` has every patch.
