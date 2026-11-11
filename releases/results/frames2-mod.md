# frames2, the mod as shipped: the complete run (8 laps, 53,820 frames, 6,093 packets)

(left out: 12256 frames that sent a USB packet, the profiler's own cost: --keep-sends to keep them)
# Frame log: 41564 frames (0.87 h, 13.2 fps), 40929 over 52 ms (98%); 7001 gaps in the frame count

| ms per frame | slow frames | full-speed frames | difference |
|---|---|---|---|
| frame period | 75.98 | 50.59 | +25.39 |
| CPU update | 46.97 | 30.11 | +16.85 |
| CPU draw | 26.17 | 18.35 | +7.82 |
| RSP graphics | 30.32 | 17.05 | +13.27 |
| RDP | 43.19 | 36.05 | +7.14 |
| fish update | 7.33 | 1.71 | +5.62 |
| fish draw | 7.70 | 3.54 | +4.15 |
| props draw | 4.67 | 2.54 | +2.12 |
|   update: fm_play | 0.10 | 0.07 | +0.03 |
|   update: fm_debug | 1.58 | 1.43 | +0.16 |
|   update: objects | 0.05 | 0.03 | +0.02 |
|   update: col_at | 0.27 | 0.16 | +0.11 |
|   update: col_oc | 1.04 | 0.66 | +0.39 |
|   update: col_damage | 0.07 | 0.05 | +0.02 |
|   update: col_clear | 0.11 | 0.09 | +0.02 |
|   update: actors | 36.99 | 21.49 | +15.51 |
|   update: dynapoly | 0.45 | 0.33 | +0.12 |
|   update: effects | 1.42 | 1.46 | -0.04 |
|   update: interface | 0.24 | 0.24 | -0.00 |
|   update: animtask | 0.08 | 0.12 | -0.04 |
|   update: camera | 4.28 | 3.64 | +0.64 |
|   update: env | 0.30 | 0.27 | +0.03 |
|   actors: bg | 1.41 | 1.23 | +0.18 |
|   actors: player | 4.35 | 2.98 | +1.37 |
|   actors: npc | 7.83 | 1.85 | +5.98 |
|   actors: enemy | 14.35 | 9.40 | +4.95 |
|   actors: prop | 0.47 | 0.40 | +0.07 |
|   actors: itemaction | 1.07 | 0.73 | +0.34 |
|   actors: misc | 0.67 | 0.52 | +0.16 |

RDP in slow frames: command buffer busy 55%, pipeline 55%, TMEM 4%

RDP in full-speed frames: command buffer busy 67%, pipeline 67%, TMEM 4%

Costliest actor kinds in slow frames (ms per slow frame, where among a frame's top four):
  En_Tite                  9.66
  En_Fishmod               7.63
  En_Crow                  4.51
  Player                   4.18
  En_River_Sound           0.74
  En_Elf                   0.55
  En_Sw                    0.37
  En_Kakasi3               0.05
  En_Kakasi2               0.05
  En_A_Obj                 0.05
  En_Kanban                0.04
  En_Gs                    0.02

Slow frames by the longer of CPU (update + draw) and RDP: CPU 100%, RDP 0%

## Areas, by time lost over 50 ms a frame (all laps)

| area | frames | mean ms | std dev ms | over 52 ms | spikes (>mean+2sd) | time lost | biggest part of the spread |
|---|---|---|---|---|---|---|---|
| 9: Rock garden (under) -> West weed flat | 1730 | 97.3 | 17.0 | 100% | 66 | 81.9 s (8%) | update: actors 55%, actors: npc 25% |
| 4: Beach shallows (under) -> Pillar ruins | 1803 | 95.3 | 19.0 | 100% | 72 | 81.6 s (8%) | update: actors 61%, actors: npc 20% |
| 8: Gerudo river inflow -> Rock garden (under) | 1758 | 96.2 | 18.3 | 100% | 34 | 81.3 s (8%) | update: actors 57%, actors: npc 31% |
| 5: Pillar ruins -> Pillar ruins (under) | 1825 | 90.8 | 19.0 | 100% | 93 | 74.5 s (7%) | update: actors 66%, actors: npc 23% |
| 2: Straight down (under) -> North beach | 1823 | 86.7 | 14.2 | 100% | 112 | 66.9 s (6%) | update: actors 61%, actors: npc 25% |
| 6: Pillar ruins (under) -> Lab shore and dock | 1823 | 85.3 | 19.7 | 100% | 37 | 64.4 s (6%) | update: actors 56%, actors: npc 22% |
| 11: West weed flat (under) -> The drop-off (under) | 1818 | 85.1 | 12.7 | 100% | 98 | 63.9 s (6%) | update: actors 61%, actors: npc 34% |
| 1: Across the west flat (under) -> Straight down (under) | 2587 | 73.4 | 11.7 | 98% | 75 | 60.5 s (6%) | update: actors 51%, actors: npc 22% |
| 7: Lab shore and dock -> Gerudo river inflow | 1844 | 82.3 | 13.9 | 100% | 89 | 59.5 s (6%) | update: actors 52%, actors: npc 16% |
| 3: North beach -> Beach shallows (under) | 1860 | 81.5 | 15.7 | 100% | 143 | 58.7 s (6%) | update: actors 59%, actors: enemy 19% |
| 10: West weed flat -> West weed flat (under) | 1833 | 81.0 | 8.1 | 100% | 95 | 56.8 s (5%) | update: actors 63%, actors: npc 50% |
| 16: Island shallows (under) -> Island shallows | 1860 | 72.2 | 11.3 | 100% | 113 | 41.3 s (4%) | update: actors 43%, actors: enemy 22% |
| 12: The drop-off (under) -> Deep basin (under) | 1907 | 71.3 | 6.7 | 100% | 43 | 40.5 s (4%) | update: actors 23%, update: camera 14% |
| 21: Under the lilies -> East shore | 1902 | 70.3 | 6.2 | 100% | 43 | 38.6 s (4%) | update: actors 29%, actors: npc 16% |
| 15: South bridge -> Island shallows (under) | 1853 | 67.1 | 9.6 | 99% | 21 | 31.7 s (3%) | update: actors 53%, actors: npc 19% |
| 14: Water Temple gate -> South bridge | 1857 | 66.8 | 7.6 | 100% | 108 | 31.2 s (3%) | update: actors 52%, actors: enemy 21% |
| 20: South reed bank -> Under the lilies | 1923 | 63.9 | 5.4 | 100% | 64 | 26.7 s (3%) | update: actors 53%, actors: enemy 18% |
| 13: Deep basin (under) -> Water Temple gate | 1922 | 63.4 | 5.7 | 99% | 50 | 25.7 s (2%) | update: actors 45%, actors: enemy 21% |
| 18: East weed flats -> East weed flats (under) | 1904 | 61.2 | 5.9 | 99% | 60 | 21.3 s (2%) | update: actors 41%, actors: npc 11% |
| 19: East weed flats (under) -> South reed bank | 1909 | 61.0 | 5.9 | 98% | 38 | 21.0 s (2%) | update: actors 32%, actors: npc 16% |
| 22: East shore -> Across the west flat (under) | 1913 | 60.3 | 7.5 | 87% | 63 | 19.9 s (2%) | update: actors 62%, actors: enemy 30% |
| 17: Island shallows -> East weed flats | 1905 | 58.3 | 5.9 | 89% | 27 | 15.8 s (1%) | update: actors 41%, actors: enemy 14% |
| 23: Across the west flat (under) -> Straight down (under) | 5 | 59.1 | 4.0 | 100% | 0 | 0.0 s (0%) | update: camera 56%, update: col_at 53% |

## Spikes in the five areas losing the most time

### 9: Rock garden (under) -> West weed flat: 66 spikes, 134.8 ms on average (the area's mean 97.3 ms)

| part | in spikes | area mean | more |
|---|---|---|---|
| CPU update | 87.4 | 62.0 | +25.4 |
| update: actors | 74.2 | 50.4 | +23.8 |
| actors: npc | 31.0 | 13.4 | +17.5 |
| fish update | 30.2 | 13.0 | +17.2 |
| CPU draw | 41.8 | 31.9 | +9.9 |
| fish draw | 16.2 | 11.0 | +5.3 |
| props draw | 9.9 | 5.7 | +4.2 |
| actors: player | 8.5 | 6.5 | +2.1 |

Actors costing more in the spikes (ms a frame, spikes vs the area): En_Fishmod 31.8 vs 13.7, Player 7.9 vs 6.1, En_River_Sound 2.1 vs 1.2, En_Elf 1.3 vs 0.7

### 4: Beach shallows (under) -> Pillar ruins: 72 spikes, 137.7 ms on average (the area's mean 95.3 ms)

| part | in spikes | area mean | more |
|---|---|---|---|
| CPU update | 86.6 | 59.6 | +27.0 |
| update: actors | 72.4 | 47.0 | +25.4 |
| CPU draw | 45.9 | 32.8 | +13.1 |
| actors: npc | 22.5 | 13.9 | +8.5 |
| fish update | 20.4 | 12.5 | +7.8 |
| actors: enemy | 21.4 | 14.9 | +6.5 |
| fish draw | 16.2 | 11.4 | +4.7 |
| actors: player | 10.3 | 5.8 | +4.4 |

Actors costing more in the spikes (ms a frame, spikes vs the area): En_Fishmod 21.0 vs 13.0, En_Tite 14.9 vs 10.5, Player 9.9 vs 5.6, En_Crow 5.6 vs 3.8

### 8: Gerudo river inflow -> Rock garden (under): 34 spikes, 138.6 ms on average (the area's mean 96.2 ms)

| part | in spikes | area mean | more |
|---|---|---|---|
| CPU update | 94.8 | 60.7 | +34.2 |
| update: actors | 74.3 | 49.7 | +24.6 |
| actors: npc | 35.0 | 11.8 | +23.1 |
| fish update | 33.5 | 10.8 | +22.7 |
| CPU draw | 38.0 | 32.1 | +6.0 |
| update: animtask | 5.9 | 0.3 | +5.7 |
| RDP | 47.8 | 44.5 | +3.3 |
| RSP graphics | 33.8 | 30.9 | +3.0 |

Actors costing more in the spikes (ms a frame, spikes vs the area): En_Fishmod 34.9 vs 11.4, En_River_Sound 2.1 vs 1.2, Player 5.7 vs 5.2, En_Weather_Tag 0.3 vs 0.0

### 5: Pillar ruins -> Pillar ruins (under): 93 spikes, 133.9 ms on average (the area's mean 90.8 ms)

| part | in spikes | area mean | more |
|---|---|---|---|
| CPU update | 85.7 | 54.4 | +31.3 |
| update: actors | 69.7 | 43.5 | +26.3 |
| actors: npc | 23.9 | 13.6 | +10.3 |
| CPU draw | 42.8 | 33.5 | +9.3 |
| fish update | 21.1 | 11.9 | +9.2 |
| actors: enemy | 20.1 | 13.3 | +6.8 |
| actors: player | 8.1 | 4.4 | +3.7 |
| props draw | 6.7 | 3.6 | +3.1 |

Actors costing more in the spikes (ms a frame, spikes vs the area): En_Fishmod 22.4 vs 12.5, En_Tite 14.1 vs 10.2, Player 7.8 vs 4.3, En_Crow 5.5 vs 2.7

### 2: Straight down (under) -> North beach: 112 spikes, 120.6 ms on average (the area's mean 86.7 ms)

| part | in spikes | area mean | more |
|---|---|---|---|
| CPU update | 76.3 | 53.1 | +23.2 |
| update: actors | 65.1 | 43.5 | +21.5 |
| CPU draw | 40.1 | 30.9 | +9.2 |
| actors: npc | 19.7 | 11.7 | +8.0 |
| fish update | 18.6 | 10.9 | +7.7 |
| actors: player | 9.5 | 4.3 | +5.2 |
| actors: enemy | 17.6 | 15.6 | +2.0 |
| props draw | 9.6 | 8.1 | +1.6 |

Actors costing more in the spikes (ms a frame, spikes vs the area): En_Fishmod 19.2 vs 11.5, Player 9.1 vs 4.0, En_Tite 11.3 vs 9.7, En_Sw 0.9 vs 0.3

