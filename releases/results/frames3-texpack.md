# frames3-texpack: the complete run against frames2's mod and vanilla

frames3 with texture packing (8 laps, 53,820 frames, no missing frames). Frames that sent a USB packet are left out (the profiler's own cost).

| | frames2 | frames3tp | difference |
|---|---|---|---|
| frames compared | 41564 | 42778 | |
| fps (frames compared) | 13.2 | 14.9 | +1.6 |
| frames over 52 ms | 98% | 87% | -12 |

## Each part, ms a frame

| part | frames2 | frames3tp | difference |
|---|---|---|---|
| frame period | 75.6 | 67.3 | -8.3 |
| CPU update | 46.7 | 36.7 | -10.0 |
| CPU draw | 26.1 | 27.9 | +1.8 |
| RSP graphics | 30.1 | 29.7 | -0.4 |
| RDP | 43.1 | 42.9 | -0.2 |
| fish update | 7.2 | 6.8 | -0.5 |
| fish draw | 7.6 | 9.0 | +1.3 |
| props draw | 4.6 | 5.8 | +1.2 |
| update: fm_debug | 1.6 | 1.5 | -0.1 |
| update: col_oc | 1.0 | 0.9 | -0.2 |
| update: actors | 36.8 | 27.6 | -9.1 |
| update: dynapoly | 0.5 | 0.4 | -0.1 |
| update: effects | 1.4 | 1.6 | +0.2 |
| update: camera | 4.3 | 3.5 | -0.8 |
| update: env | 0.3 | 0.3 | +0.0 |
| actors: bg | 1.4 | 0.4 | -1.0 |
| actors: player | 4.3 | 3.6 | -0.8 |
| actors: npc | 7.7 | 7.0 | -0.7 |
| actors: enemy | 14.3 | 7.6 | -6.6 |
| actors: prop | 0.5 | 0.5 | +0.0 |
| actors: itemaction | 1.1 | 0.9 | -0.2 |
| actors: misc | 0.7 | 0.7 | +0.0 |

## By area (each leg, all laps): ms a frame, and frames over 52 ms

| area | frames2 | frames3tp | difference | frames2 slow | frames3tp slow |
|---|---|---|---|---|---|
| 1: Across the west flat (under) -> Straight down (under) | 73.4 | 65.7 | -7.7 | 98% | 84% |
| 2: Straight down (under) -> North beach | 86.7 | 78.8 | -7.9 | 100% | 100% |
| 3: North beach -> Beach shallows (under) | 81.5 | 69.4 | -12.2 | 100% | 98% |
| 4: Beach shallows (under) -> Pillar ruins | 95.3 | 85.5 | -9.8 | 100% | 100% |
| 5: Pillar ruins -> Pillar ruins (under) | 90.8 | 83.7 | -7.2 | 100% | 100% |
| 6: Pillar ruins (under) -> Lab shore and dock | 85.3 | 75.4 | -10.0 | 100% | 90% |
| 7: Lab shore and dock -> Gerudo river inflow | 82.3 | 68.0 | -14.2 | 100% | 99% |
| 8: Gerudo river inflow -> Rock garden (under) | 96.2 | 83.7 | -12.6 | 100% | 100% |
| 9: Rock garden (under) -> West weed flat | 97.3 | 86.6 | -10.8 | 100% | 100% |
| 10: West weed flat -> West weed flat (under) | 81.0 | 74.5 | -6.5 | 100% | 100% |
| 11: West weed flat (under) -> The drop-off (under) | 85.1 | 72.7 | -12.5 | 100% | 100% |
| 12: The drop-off (under) -> Deep basin (under) | 71.3 | 63.1 | -8.2 | 100% | 99% |
| 13: Deep basin (under) -> Water Temple gate | 63.4 | 57.2 | -6.2 | 99% | 85% |
| 14: Water Temple gate -> South bridge | 66.8 | 58.3 | -8.5 | 100% | 86% |
| 15: South bridge -> Island shallows (under) | 67.1 | 60.7 | -6.4 | 99% | 82% |
| 16: Island shallows (under) -> Island shallows | 72.2 | 65.1 | -7.1 | 100% | 93% |
| 17: Island shallows -> East weed flats | 58.3 | 52.1 | -6.2 | 89% | 32% |
| 18: East weed flats -> East weed flats (under) | 61.2 | 55.5 | -5.7 | 99% | 65% |
| 19: East weed flats (under) -> South reed bank | 61.0 | 55.3 | -5.7 | 98% | 67% |
| 20: South reed bank -> Under the lilies | 63.9 | 56.9 | -7.0 | 100% | 85% |
| 21: Under the lilies -> East shore | 70.3 | 65.3 | -5.0 | 100% | 100% |
| 22: East shore -> Across the west flat (under) | 60.3 | 54.8 | -5.5 | 87% | 61% |

## Against vanilla

| | vanilla | frames3tp | difference |
|---|---|---|---|
| frames compared | 48559 | 42778 | |
| fps (frames compared) | 17.2 | 14.9 | -2.4 |
| frames over 52 ms | 68% | 87% | +19 |

## Each part, ms a frame

| part | vanilla | frames3tp | difference |
|---|---|---|---|
| frame period | 58.1 | 67.3 | +9.3 |
| CPU update | 35.4 | 36.7 | +1.4 |
| CPU draw | 19.7 | 27.9 | +8.2 |
| RSP graphics | 19.1 | 29.7 | +10.6 |
| RDP | 38.6 | 42.9 | +4.3 |
| fish update | 0.0 | 6.8 | +6.8 |
| fish draw | 0.0 | 9.0 | +9.0 |
| props draw | 0.0 | 5.8 | +5.8 |
| update: fm_debug | 0.9 | 1.5 | +0.6 |
| update: col_oc | 1.1 | 0.9 | -0.2 |
| update: actors | 26.4 | 27.6 | +1.2 |
| update: dynapoly | 0.5 | 0.4 | -0.1 |
| update: effects | 1.5 | 1.6 | +0.1 |
| update: interface | 0.3 | 0.2 | -0.1 |
| update: camera | 3.9 | 3.5 | -0.4 |
| update: env | 0.4 | 0.3 | -0.0 |
| actors: bg | 0.7 | 0.4 | -0.3 |
| actors: player | 3.4 | 3.6 | +0.1 |
| actors: npc | 0.6 | 7.0 | +6.4 |
| actors: enemy | 15.0 | 7.6 | -7.3 |
| actors: prop | 0.5 | 0.5 | +0.0 |
| actors: itemaction | 1.0 | 0.9 | -0.1 |
| actors: misc | 0.0 | 0.7 | +0.7 |

## By area (each leg, all laps): ms a frame, and frames over 52 ms

| area | vanilla | frames3tp | difference | vanilla slow | frames3tp slow |
|---|---|---|---|---|---|
| 1: Across the west flat (under) -> Straight down (under) | 53.8 | 65.7 | +11.9 | 60% | 84% |
| 2: Straight down (under) -> North beach | 60.6 | 78.8 | +18.3 | 84% | 100% |
| 3: North beach -> Beach shallows (under) | 55.3 | 69.4 | +14.1 | 46% | 98% |
| 4: Beach shallows (under) -> Pillar ruins | 59.0 | 85.5 | +26.5 | 85% | 100% |
| 5: Pillar ruins -> Pillar ruins (under) | 61.9 | 83.7 | +21.8 | 92% | 100% |
| 6: Pillar ruins (under) -> Lab shore and dock | 69.9 | 75.4 | +5.5 | 98% | 90% |
| 7: Lab shore and dock -> Gerudo river inflow | 64.8 | 68.0 | +3.2 | 98% | 99% |
| 8: Gerudo river inflow -> Rock garden (under) | 68.7 | 83.7 | +14.9 | 94% | 100% |
| 9: Rock garden (under) -> West weed flat | 65.9 | 86.6 | +20.7 | 98% | 100% |
| 10: West weed flat -> West weed flat (under) | 59.3 | 74.5 | +15.2 | 95% | 100% |
| 11: West weed flat (under) -> The drop-off (under) | 60.4 | 72.7 | +12.3 | 98% | 100% |
| 12: The drop-off (under) -> Deep basin (under) | 61.4 | 63.1 | +1.6 | 98% | 99% |
| 13: Deep basin (under) -> Water Temple gate | 53.6 | 57.2 | +3.6 | 61% | 85% |
| 14: Water Temple gate -> South bridge | 58.9 | 58.3 | -0.6 | 87% | 86% |
| 15: South bridge -> Island shallows (under) | 56.8 | 60.7 | +3.9 | 62% | 82% |
| 16: Island shallows (under) -> Island shallows | 58.4 | 65.1 | +6.7 | 88% | 93% |
| 17: Island shallows -> East weed flats | 51.2 | 52.1 | +0.9 | 20% | 32% |
| 18: East weed flats -> East weed flats (under) | 50.1 | 55.5 | +5.4 | 4% | 65% |
| 19: East weed flats (under) -> South reed bank | 50.3 | 55.3 | +5.0 | 12% | 67% |
| 20: South reed bank -> Under the lilies | 50.5 | 56.9 | +6.4 | 13% | 85% |
| 21: Under the lilies -> East shore | 55.9 | 65.3 | +9.4 | 62% | 100% |
| 22: East shore -> Across the west flat (under) | 54.5 | 54.8 | +0.3 | 59% | 61% |
