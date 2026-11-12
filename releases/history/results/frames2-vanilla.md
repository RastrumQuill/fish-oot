# frames2, vanilla: the complete run (8 laps, 53,781 frames, 5,104 packets)

Read with `fishmod/tools/framelog.py FOLDER --all-frames --spikes`. Frames that sent a USB packet are left out (the profiler's own ~9 ms).

(left out: 5222 frames that sent a USB packet, the profiler's own cost: --keep-sends to keep them)
# Frame log: 48559 frames (0.78 h, 17.2 fps), 33068 over 52 ms (68%); 4005 gaps in the frame count

| ms per frame | slow frames | full-speed frames | difference |
|---|---|---|---|
| frame period | 61.84 | 50.01 | +11.83 |
| CPU update | 38.87 | 27.84 | +11.02 |
| CPU draw | 20.46 | 18.02 | +2.44 |
| RSP graphics | 21.80 | 13.48 | +8.31 |
| RDP | 40.46 | 34.78 | +5.69 |
|   update: fm_debug | 0.95 | 0.73 | +0.22 |
|   update: col_at | 0.24 | 0.16 | +0.08 |
|   update: col_oc | 1.23 | 0.77 | +0.46 |
|   update: col_damage | 0.07 | 0.05 | +0.02 |
|   update: col_clear | 0.08 | 0.06 | +0.02 |
|   update: actors | 29.25 | 20.35 | +8.90 |
|   update: dynapoly | 0.50 | 0.39 | +0.12 |
|   update: effects | 1.48 | 1.43 | +0.04 |
|   update: interface | 0.34 | 0.30 | +0.04 |
|   update: animtask | 0.10 | 0.05 | +0.05 |
|   update: camera | 4.20 | 3.16 | +1.04 |
|   update: env | 0.37 | 0.32 | +0.05 |
|   actors: bg | 0.79 | 0.61 | +0.18 |
|   actors: player | 3.87 | 2.52 | +1.36 |
|   actors: npc | 0.66 | 0.46 | +0.20 |
|   actors: enemy | 16.89 | 10.82 | +6.07 |
|   actors: prop | 0.48 | 0.45 | +0.03 |
|   actors: itemaction | 1.10 | 0.85 | +0.25 |

RDP in slow frames: command buffer busy 65%, pipeline 65%, TMEM 7%

RDP in full-speed frames: command buffer busy 69%, pipeline 69%, TMEM 7%

Costliest actor kinds in slow frames (ms per slow frame, where among a frame's top four):
  En_Tite                  10.28
  En_Crow                  6.61
  Player                   3.86
  En_Elf                   0.94
  En_River_Sound           0.41
  En_Sw                    0.32
  En_Kanban                0.05
  En_A_Obj                 0.05
  En_Kakasi2               0.04
  Obj_Mure2                0.03
  En_Kakasi3               0.02
  En_Weather_Tag           0.01

Slow frames by the longer of CPU (update + draw) and RDP: CPU 100%, RDP 0%

## Areas, by time lost over 50 ms a frame (all laps)

| area | frames | mean ms | std dev ms | over 52 ms | spikes (>mean+2sd) | time lost | biggest part of the spread |
|---|---|---|---|---|---|---|---|
| 6: Pillar ruins (under) -> Lab shore and dock | 2134 | 69.9 | 10.3 | 98% | 35 | 42.4 s (11%) | update: actors 52%, actors: enemy 30% |
| 8: Gerudo river inflow -> Rock garden (under) | 2079 | 68.7 | 10.6 | 94% | 9 | 39.0 s (10%) | update: actors 39%, actors: enemy 19% |
| 9: Rock garden (under) -> West weed flat | 2109 | 65.9 | 9.4 | 98% | 25 | 33.5 s (8%) | update: actors 45%, actors: enemy 25% |
| 7: Lab shore and dock -> Gerudo river inflow | 2135 | 64.8 | 8.2 | 98% | 79 | 31.7 s (8%) | update: actors 55%, actors: enemy 30% |
| 5: Pillar ruins -> Pillar ruins (under) | 2197 | 61.9 | 9.3 | 92% | 173 | 26.1 s (7%) | update: actors 59%, actors: enemy 35% |
| 12: The drop-off (under) -> Deep basin (under) | 2149 | 61.4 | 5.1 | 98% | 31 | 24.6 s (6%) | update: actors 35%, actors: enemy 18% |
| 2: Straight down (under) -> North beach | 2130 | 60.6 | 8.3 | 84% | 141 | 22.6 s (6%) | update: actors 50%, actors: enemy 27% |
| 11: West weed flat (under) -> The drop-off (under) | 2154 | 60.4 | 6.5 | 98% | 96 | 22.3 s (6%) | update: actors 52%, actors: enemy 36% |
| 10: West weed flat -> West weed flat (under) | 2156 | 59.3 | 6.5 | 95% | 103 | 20.1 s (5%) | update: actors 39%, actors: enemy 23% |
| 4: Beach shallows (under) -> Pillar ruins | 2197 | 59.0 | 7.3 | 85% | 98 | 19.8 s (5%) | update: actors 65%, actors: enemy 39% |
| 14: Water Temple gate -> South bridge | 2166 | 58.9 | 6.5 | 87% | 64 | 19.3 s (5%) | update: actors 42%, actors: enemy 26% |
| 16: Island shallows (under) -> Island shallows | 2147 | 58.4 | 5.3 | 88% | 27 | 18.0 s (5%) | update: actors 44%, actors: enemy 29% |
| 15: South bridge -> Island shallows (under) | 2141 | 56.8 | 7.1 | 62% | 11 | 15.1 s (4%) | update: actors 48%, actors: enemy 28% |
| 21: Under the lilies -> East shore | 2152 | 55.9 | 6.1 | 62% | 14 | 13.2 s (3%) | update: actors 35%, actors: player 19% |
| 3: North beach -> Beach shallows (under) | 2248 | 55.3 | 8.2 | 46% | 89 | 12.3 s (3%) | update: actors 68%, actors: enemy 31% |
| 1: Across the west flat (under) -> Straight down (under) | 2900 | 53.8 | 4.5 | 60% | 104 | 11.5 s (3%) | update: actors 40%, actors: enemy 29% |
| 22: East shore -> Across the west flat (under) | 2189 | 54.5 | 5.4 | 59% | 55 | 10.4 s (3%) | update: actors 56%, actors: enemy 43% |
| 13: Deep basin (under) -> Water Temple gate | 2141 | 53.6 | 3.9 | 61% | 105 | 8.1 s (2%) | update: actors 47%, actors: enemy 34% |
| 17: Island shallows -> East weed flats | 2276 | 51.2 | 3.3 | 20% | 143 | 3.5 s (1%) | update: actors 82%, actors: enemy 50% |
| 20: South reed bank -> Under the lilies | 2252 | 50.5 | 2.5 | 13% | 73 | 2.1 s (1%) | update: actors 55%, actors: enemy 33% |
| 19: East weed flats (under) -> South reed bank | 2202 | 50.3 | 2.6 | 12% | 73 | 1.9 s (0%) | update: actors 55%, actors: enemy 37% |
| 18: East weed flats -> East weed flats (under) | 2299 | 50.1 | 1.5 | 4% | 37 | 1.0 s (0%) | update: actors 22%, update: fm_debug 15% |
| 23: Across the west flat (under) -> Straight down (under) | 6 | 56.9 | 5.1 | 67% | 0 | 0.0 s (0%) | update: actors 48%, update: camera 29% |

## Spikes in the five areas losing the most time

### 6: Pillar ruins (under) -> Lab shore and dock: 35 spikes, 95.2 ms on average (the area's mean 69.9 ms)

| part | in spikes | area mean | more |
|---|---|---|---|
| CPU update | 59.6 | 43.6 | +16.0 |
| actors: enemy | 27.3 | 17.5 | +9.8 |
| update: actors | 42.1 | 32.8 | +9.3 |
| update: camera | 10.3 | 5.6 | +4.8 |
| CPU draw | 27.6 | 23.7 | +3.9 |
| update: col_oc | 2.8 | 1.2 | +1.6 |
| RSP graphics | 28.4 | 27.0 | +1.4 |
| RDP | 43.6 | 42.7 | +0.9 |

Actors costing more in the spikes (ms a frame, spikes vs the area): En_Tite 15.8 vs 10.9, En_Crow 11.5 vs 6.6, Obj_Mure2 0.7 vs 0.1, En_River_Sound 0.9 vs 0.4

### 8: Gerudo river inflow -> Rock garden (under): 9 spikes, 99.8 ms on average (the area's mean 68.7 ms)

| part | in spikes | area mean | more |
|---|---|---|---|
| CPU update | 71.0 | 42.3 | +28.6 |
| update: animtask | 25.7 | 0.2 | +25.5 |
| update: interface | 3.5 | 0.4 | +3.1 |
| RDP | 44.5 | 41.8 | +2.7 |
| CPU draw | 26.0 | 23.6 | +2.4 |
| RSP graphics | 25.3 | 23.0 | +2.4 |
| update: actors | 34.5 | 33.0 | +1.5 |
| actors: bg | 1.5 | 0.9 | +0.6 |

Actors costing more in the spikes (ms a frame, spikes vs the area): En_River_Sound 1.2 vs 0.5, En_Crow 9.1 vs 8.8

### 9: Rock garden (under) -> West weed flat: 25 spikes, 86.2 ms on average (the area's mean 65.9 ms)

| part | in spikes | area mean | more |
|---|---|---|---|
| CPU draw | 31.5 | 21.0 | +10.4 |
| CPU update | 52.3 | 42.1 | +10.3 |
| update: actors | 41.2 | 32.2 | +9.0 |
| RDP | 45.4 | 38.3 | +7.2 |
| actors: enemy | 24.0 | 19.7 | +4.3 |
| actors: player | 5.3 | 3.8 | +1.5 |
| update: fm_debug | 1.8 | 0.9 | +0.9 |
| actors: bg | 1.7 | 0.9 | +0.8 |

Actors costing more in the spikes (ms a frame, spikes vs the area): En_Crow 10.9 vs 8.3, En_Tite 13.1 vs 11.4, Player 5.3 vs 3.8, En_River_Sound 1.3 vs 0.5

### 7: Lab shore and dock -> Gerudo river inflow: 79 spikes, 82.5 ms on average (the area's mean 64.8 ms)

| part | in spikes | area mean | more |
|---|---|---|---|
| CPU update | 52.0 | 42.1 | +9.9 |
| update: actors | 41.8 | 33.0 | +8.9 |
| CPU draw | 25.3 | 20.1 | +5.2 |
| actors: enemy | 21.6 | 17.6 | +4.0 |
| actors: player | 8.2 | 5.4 | +2.7 |
| update: col_oc | 1.8 | 1.2 | +0.6 |
| actors: itemaction | 1.4 | 0.9 | +0.5 |
| update: fm_debug | 1.4 | 0.9 | +0.4 |

Actors costing more in the spikes (ms a frame, spikes vs the area): En_Tite 13.3 vs 10.4, Player 8.1 vs 5.4, En_Crow 8.3 vs 7.1, En_Elf 1.2 vs 0.8

### 5: Pillar ruins -> Pillar ruins (under): 173 spikes, 83.1 ms on average (the area's mean 61.9 ms)

| part | in spikes | area mean | more |
|---|---|---|---|
| CPU update | 52.2 | 38.1 | +14.1 |
| update: actors | 41.3 | 28.5 | +12.8 |
| actors: enemy | 23.5 | 16.3 | +7.2 |
| CPU draw | 27.4 | 21.7 | +5.7 |
| RSP graphics | 29.5 | 25.1 | +4.4 |
| RDP | 46.9 | 42.9 | +4.0 |
| actors: player | 6.0 | 3.9 | +2.1 |
| actors: itemaction | 2.6 | 1.6 | +1.0 |

Actors costing more in the spikes (ms a frame, spikes vs the area): En_Tite 13.5 vs 9.6, En_Crow 10.0 vs 6.6, Player 5.8 vs 3.8, En_Elf 2.4 vs 1.5


## CPU profile (top functions, all frames)

```
1 dump(s): 11261788 samples, 447 not in the dump (0.0%: table full or too cold), 13 outside the code and overlays
  share    samples  function
  6.97%     784358  AudioSynth_ProcessNote
  4.98%     560457  AudioLoad_DmaSampleData
  3.68%     414488  __osRestoreInt
  2.80%     315305  Audio_ProcessNotes
  2.63%     295639  AudioSeq_SequenceChannelProcessScript
  2.46%     277567  BgCheck_RaycastDownStaticList
  2.22%     250226  Matrix_MtxFToMtx
  1.65%     185867  Audio_InitSampleState
  1.60%     180010  AudioSynth_ProcessEnvelope
  1.55%     174398  Actor_UpdateAll
  1.53%     171813  AudioSynth_DoOneAudioUpdate
  1.30%     146074  Audio_SequenceChannelProcessSound
  1.19%     133946  BgCheck_Vec3sToVec3f
  1.16%     130846  BgCheck_SphVsStaticWall
  1.09%     122405  BgCheck_ResetPolyCheckTbl
  1.08%     121888  SkelAnime_GetFrameData
  1.03%     116174  Math3D_TriChkPointParaYImpl
  0.94%     105891  Math_FAtanContFracF
  0.91%     102615  Idle_ThreadEntry
  0.88%      99248  Matrix_TranslateRotateZYX
  0.86%      96622  sins
  0.83%      93219  BgCheck_RaycastDownDyna
  0.75%      84272  EffectSsGRipple_DrawRipple
  0.74%      83198  CollisionPoly_GetNormalF
  0.74%      83167  SkinMatrix_MtxFToMtx
  0.72%      80947  Math3D_PointDistSqToLine2D
  0.66%      74774  SkinMatrix_MtxFMtxFMult
  0.65%      73605  AudioSeq_SequencePlayerProcessSequence
  0.62%      70144  AudioSeq_ScriptReadU8
  0.61%      68401  Audio_SequencePlayerProcessSound
  0.60%      67273  Math3D_CirSquareVsTriSquare
  0.58%      65509  CollisionPoly_CheckYIntersect
  0.57%      64096  SkinMatrix_Vec3fMtxFMultXYZW
  0.56%      63179  BgCheck_RaycastDownImpl
  0.51%      57251  CollisionPoly_GetVertices
  0.50%      56736  Audio_AdsrUpdate
  0.50%      56113  func_800DB03C
  0.48%      54603  Audio_GetVibratoFreqScale
  0.48%      54330  Matrix_MtxFCopy
  0.46%      51279  AudioThread_UpdateImpl
```
