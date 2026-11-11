# frames4-gcc on the console: the full run (8 laps)

The whole game's C on GCC (`--gcc-all`), with everything up to frames4. 36,569 frames (0.58 h), all 8 laps (every
weather and time of day). Frames that sent a USB packet are left out.

| | vanilla | frames3 + texpack | frames4-gcc |
|---|---|---|---|
| fps | 17.2 | 14.9 | **17.5** |
| frames over 52 ms | 68% | 87% | **53%** |
| frame | 58.1 | 67.3 | **57.2** |
| CPU update | 35.4 | 36.7 | **27.3** |
| CPU draw | 19.7 | 27.9 | 23.7 |
| RSP graphics | 19.1 | 29.7 | 30.1 |
| RDP | 38.6 | 42.9 | 42.5 |
| fish update | | 6.8 | 4.7 |
| fish draw | | 9.0 | 7.4 |
| props draw | | 5.8 | 4.5 |

The full run confirms lap 1: faster than unmodified vanilla over the whole route, fish and props included. Slow frames
by what limits them: CPU 87%, RDP 13%.

Where it's still slow: the Pillar ruins legs (4, 5) are slow in every frame (mean 68 ms; vanilla 59-62). There the
CPU (update 31 + draw 28 ms) and the RDP (54 ms) are both over the 50 ms frame. The spikes on the west legs (2, 9, 11)
are the fish's update: En_Fishmod 15-21 ms in a spike against 9-10 ms on average there. En_Tite still costs 3.5-6 ms
on legs 4 and 5, En_Crow 2-5 ms on legs 2 and 5.

## Against vanilla

## Each part, ms a frame

| part | vanilla | frames4-gcc | difference |
|---|---|---|---|
| frame period | 58.1 | 57.2 | -0.9 |
| CPU update | 35.4 | 27.3 | -8.0 |
| CPU draw | 19.7 | 23.7 | +4.0 |
| RSP graphics | 19.1 | 30.1 | +11.0 |
| RDP | 38.6 | 42.5 | +3.9 |
| fish update | 0.0 | 4.7 | +4.7 |
| fish draw | 0.0 | 7.4 | +7.4 |
| props draw | 0.0 | 4.5 | +4.5 |
| update: fm_debug | 0.9 | 1.5 | +0.6 |
| update: col_oc | 1.1 | 0.7 | -0.4 |
| update: actors | 26.4 | 20.0 | -6.4 |
| update: dynapoly | 0.5 | 0.3 | -0.2 |
| update: effects | 1.5 | 1.1 | -0.3 |
| update: interface | 0.3 | 0.2 | -0.1 |
| update: camera | 3.9 | 2.8 | -1.1 |
| update: env | 0.4 | 0.3 | -0.1 |
| actors: bg | 0.7 | 0.3 | -0.5 |
| actors: player | 3.4 | 2.4 | -1.1 |
| actors: npc | 0.6 | 4.9 | +4.3 |
| actors: enemy | 15.0 | 5.8 | -9.1 |
| actors: prop | 0.5 | 0.4 | -0.1 |
| actors: itemaction | 1.0 | 0.6 | -0.4 |
| actors: misc | 0.0 | 0.6 | +0.6 |

## By area (each leg, all laps): ms a frame, and frames over 52 ms

| area | vanilla | frames4-gcc | difference | vanilla slow | frames4-gcc slow |
|---|---|---|---|---|---|
| 1: Across the west flat (under) -> Straight down (under) | 53.8 | 58.1 | +4.3 | 60% | 66% |
| 2: Straight down (under) -> North beach | 60.6 | 64.9 | +4.3 | 84% | 96% |
| 3: North beach -> Beach shallows (under) | 55.3 | 60.7 | +5.4 | 46% | 77% |
| 4: Beach shallows (under) -> Pillar ruins | 59.0 | 67.7 | +8.7 | 85% | 100% |
| 5: Pillar ruins -> Pillar ruins (under) | 61.9 | 67.4 | +5.6 | 92% | 100% |
| 6: Pillar ruins (under) -> Lab shore and dock | 69.9 | 63.1 | -6.8 | 98% | 79% |
| 7: Lab shore and dock -> Gerudo river inflow | 64.8 | 56.1 | -8.7 | 98% | 66% |
| 8: Gerudo river inflow -> Rock garden (under) | 68.7 | 61.7 | -7.0 | 94% | 88% |
| 9: Rock garden (under) -> West weed flat | 65.9 | 65.6 | -0.3 | 98% | 98% |
| 10: West weed flat -> West weed flat (under) | 59.3 | 65.5 | +6.2 | 95% | 94% |
| 11: West weed flat (under) -> The drop-off (under) | 60.4 | 65.9 | +5.5 | 98% | 91% |
| 12: The drop-off (under) -> Deep basin (under) | 61.4 | 53.1 | -8.3 | 98% | 40% |
| 13: Deep basin (under) -> Water Temple gate | 53.6 | 50.1 | -3.5 | 61% | 3% |
| 14: Water Temple gate -> South bridge | 58.9 | 51.0 | -7.9 | 87% | 19% |
| 15: South bridge -> Island shallows (under) | 56.8 | 50.7 | -6.1 | 62% | 15% |
| 16: Island shallows (under) -> Island shallows | 58.4 | 54.8 | -3.6 | 88% | 50% |
| 17: Island shallows -> East weed flats | 51.2 | 50.0 | -1.2 | 20% | 2% |
| 18: East weed flats -> East weed flats (under) | 50.1 | 50.4 | +0.4 | 4% | 15% |
| 19: East weed flats (under) -> South reed bank | 50.3 | 50.2 | -0.1 | 12% | 12% |
| 20: South reed bank -> Under the lilies | 50.5 | 50.1 | -0.3 | 13% | 5% |
| 21: Under the lilies -> East shore | 55.9 | 53.6 | -2.2 | 62% | 58% |
| 22: East shore -> Across the west flat (under) | 54.5 | 50.4 | -4.1 | 59% | 6% |

## The spikes

### Areas, by time lost over 50 ms a frame (all laps)

| area | frames | mean ms | std dev ms | over 52 ms | spikes (>mean+2sd) | time lost | biggest part of the spread |
|---|---|---|---|---|---|---|---|
| 4: Beach shallows (under) -> Pillar ruins | 1783 | 68.2 | 4.1 | 100% | 127 | 32.4 s (12%) | update: actors 91%, actors: npc 28% |
| 5: Pillar ruins -> Pillar ruins (under) | 1827 | 67.3 | 2.8 | 100% | 69 | 31.5 s (11%) | update: actors 81%, actors: npc 34% |
| 11: West weed flat (under) -> The drop-off (under) | 1510 | 67.1 | 11.2 | 95% | 43 | 25.8 s (9%) | update: actors 51%, actors: npc 24% |
| 2: Straight down (under) -> North beach | 1548 | 65.2 | 7.8 | 96% | 72 | 23.5 s (8%) | update: actors 51%, actors: npc 34% |
| 9: Rock garden (under) -> West weed flat | 1528 | 65.4 | 8.5 | 98% | 60 | 23.5 s (8%) | update: actors 62%, actors: npc 39% |
| 10: West weed flat -> West weed flat (under) | 1520 | 64.9 | 9.9 | 94% | 39 | 22.7 s (8%) | update: actors 58%, actors: npc 33% |
| 6: Pillar ruins (under) -> Lab shore and dock | 1598 | 63.9 | 9.0 | 83% | 30 | 22.4 s (8%) | update: actors 49%, actors: npc 20% |
| 1: Across the west flat (under) -> Straight down (under) | 1664 | 61.5 | 5.7 | 93% | 1 | 19.2 s (7%) | actors: npc 12%, update: actors 12% |
| 3: North beach -> Beach shallows (under) | 1758 | 60.6 | 7.2 | 77% | 3 | 18.8 s (7%) | update: actors 14%, actors: npc 10% |
| 8: Gerudo river inflow -> Rock garden (under) | 1522 | 62.2 | 8.3 | 88% | 65 | 18.6 s (7%) | update: actors 66%, actors: npc 37% |
| 7: Lab shore and dock -> Gerudo river inflow | 1576 | 56.8 | 6.7 | 71% | 37 | 11.0 s (4%) | update: actors 48%, actors: enemy 15% |
| 16: Island shallows (under) -> Island shallows | 1567 | 55.8 | 6.3 | 61% | 35 | 9.3 s (3%) | update: actors 30%, actors: enemy 14% |
| 21: Under the lilies -> East shore | 1541 | 53.8 | 4.8 | 59% | 31 | 6.5 s (2%) | update: actors 32%, actors: npc 16% |
| 12: The drop-off (under) -> Deep basin (under) | 1634 | 53.1 | 4.9 | 38% | 109 | 5.5 s (2%) | actors: prop 3%, actors: itemaction 2% |
| 14: Water Temple gate -> South bridge | 1696 | 51.2 | 3.3 | 22% | 100 | 2.7 s (1%) | update: actors 63%, actors: npc 25% |
| 15: South bridge -> Island shallows (under) | 1602 | 50.6 | 2.9 | 15% | 57 | 1.8 s (1%) | update: actors 94%, actors: npc 47% |
| 18: East weed flats -> East weed flats (under) | 1879 | 50.2 | 1.9 | 11% | 56 | 1.4 s (0%) | update: actors 29%, actors: npc 24% |
| 22: East shore -> Across the west flat (under) | 1793 | 50.5 | 2.0 | 7% | 66 | 1.3 s (0%) | update: animtask 25%, update: camera 13% |
| 19: East weed flats (under) -> South reed bank | 1728 | 50.0 | 1.9 | 9% | 55 | 1.1 s (0%) | update: actors 38%, actors: npc 23% |
| 20: South reed bank -> Under the lilies | 1836 | 50.1 | 1.3 | 4% | 27 | 0.9 s (0%) | update: actors 22%, actors: npc 22% |
| 13: Deep basin (under) -> Water Temple gate | 1779 | 50.0 | 1.3 | 2% | 27 | 0.6 s (0%) | update: actors 32%, actors: npc 28% |
| 17: Island shallows -> East weed flats | 1680 | 49.9 | 1.5 | 2% | 29 | 0.6 s (0%) | update: camera 17%, update: actors 9% |

### Spikes in the five areas losing the most time

### 4: Beach shallows (under) -> Pillar ruins: 127 spikes, 80.4 ms on average (the area's mean 68.2 ms)

| part | in spikes | area mean | more |
|---|---|---|---|
| CPU update | 45.9 | 31.2 | +14.7 |
| update: actors | 33.6 | 23.1 | +10.4 |
| RDP | 58.8 | 54.0 | +4.8 |
| CPU draw | 31.7 | 28.3 | +3.4 |
| actors: npc | 10.0 | 7.3 | +2.7 |
| RSP graphics | 49.4 | 46.7 | +2.6 |
| actors: player | 5.3 | 2.8 | +2.5 |
| update: camera | 6.1 | 4.0 | +2.1 |

Actors costing more in the spikes (ms a frame, spikes vs the area): Player 5.1 vs 2.8, En_Fishmod 9.5 vs 7.1, En_Tite 5.8 vs 4.0, En_Sw 0.7 vs 0.2

### 5: Pillar ruins -> Pillar ruins (under): 69 spikes, 78.9 ms on average (the area's mean 67.3 ms)

| part | in spikes | area mean | more |
|---|---|---|---|
| CPU update | 42.9 | 28.2 | +14.8 |
| update: actors | 31.7 | 20.9 | +10.9 |
| CPU draw | 32.2 | 28.2 | +4.0 |
| actors: enemy | 9.4 | 5.5 | +3.9 |
| actors: npc | 10.8 | 6.9 | +3.9 |
| RDP | 58.5 | 54.9 | +3.6 |
| fish update | 9.5 | 6.3 | +3.1 |
| props draw | 6.1 | 3.0 | +3.1 |

Actors costing more in the spikes (ms a frame, spikes vs the area): En_Fishmod 10.0 vs 6.8, En_Tite 5.6 vs 3.6, En_Crow 3.6 vs 1.8, Player 3.0 vs 1.8

### 11: West weed flat (under) -> The drop-off (under): 43 spikes, 96.0 ms on average (the area's mean 67.1 ms)

| part | in spikes | area mean | more |
|---|---|---|---|
| CPU update | 57.1 | 41.2 | +16.0 |
| update: actors | 44.0 | 31.1 | +12.9 |
| CPU draw | 34.2 | 23.7 | +10.5 |
| actors: npc | 17.9 | 9.6 | +8.4 |
| fish update | 16.7 | 8.7 | +8.0 |
| fish draw | 14.0 | 6.3 | +7.7 |
| props draw | 9.8 | 3.9 | +5.9 |
| RSP graphics | 36.0 | 31.3 | +4.7 |

Actors costing more in the spikes (ms a frame, spikes vs the area): En_Fishmod 18.2 vs 9.6, En_Tite 5.2 vs 3.8, Player 5.1 vs 4.1, En_River_Sound 0.5 vs 0.1

### 2: Straight down (under) -> North beach: 72 spikes, 84.1 ms on average (the area's mean 65.2 ms)

| part | in spikes | area mean | more |
|---|---|---|---|
| CPU update | 45.2 | 34.6 | +10.7 |
| update: actors | 37.2 | 27.3 | +9.9 |
| CPU draw | 36.2 | 28.1 | +8.0 |
| RSP graphics | 41.8 | 34.4 | +7.4 |
| RDP | 56.9 | 50.0 | +6.9 |
| fish update | 14.6 | 8.4 | +6.1 |
| actors: npc | 14.8 | 9.2 | +5.6 |
| fish draw | 14.8 | 11.1 | +3.7 |

Actors costing more in the spikes (ms a frame, spikes vs the area): En_Fishmod 15.3 vs 9.2, En_Crow 4.7 vs 3.6, Player 3.6 vs 2.6, En_Tite 4.0 vs 3.3

### 9: Rock garden (under) -> West weed flat: 60 spikes, 85.1 ms on average (the area's mean 65.4 ms)

| part | in spikes | area mean | more |
|---|---|---|---|
| CPU update | 52.9 | 38.8 | +14.2 |
| update: actors | 43.6 | 30.1 | +13.5 |
| fish update | 19.9 | 8.9 | +11.0 |
| actors: npc | 19.9 | 9.1 | +10.8 |
| CPU draw | 29.5 | 24.3 | +5.3 |
| fish draw | 13.1 | 8.3 | +4.8 |
| RSP graphics | 34.9 | 30.8 | +4.1 |
| props draw | 7.7 | 4.6 | +3.0 |

Actors costing more in the spikes (ms a frame, spikes vs the area): En_Fishmod 21.0 vs 9.7, En_Tite 4.5 vs 3.9, Player 3.6 vs 3.2

