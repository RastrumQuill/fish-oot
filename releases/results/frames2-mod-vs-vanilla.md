# frames2: the mod as shipped against vanilla, complete runs

Both runs complete (8 laps each: mod 53,820 frames, vanilla 53,781; no missing frames). Frames that sent a USB packet are left out of both (the profiler's own ~9 ms). The mod's build: GCC, F3DEX3, the audio DMA search, the faster matrix conversion; vanilla: the original IDO build. Read with `fishmod/tools/framelog_vs.py VANILLA MOD --names vanilla,mod`.

| | vanilla | mod | difference |
|---|---|---|---|
| frames compared | 48559 | 41564 | |
| fps (frames compared) | 17.2 | 13.2 | -4.0 |
| frames over 52 ms | 68% | 98% | +30 |

## Each part, ms a frame

| part | vanilla | mod | difference |
|---|---|---|---|
| frame period | 58.1 | 75.6 | +17.5 |
| CPU update | 35.4 | 46.7 | +11.4 |
| CPU draw | 19.7 | 26.1 | +6.4 |
| RSP graphics | 19.1 | 30.1 | +11.0 |
| RDP | 38.6 | 43.1 | +4.4 |
| fish update | 0.0 | 7.2 | +7.2 |
| fish draw | 0.0 | 7.6 | +7.6 |
| props draw | 0.0 | 4.6 | +4.6 |
| update: fm_debug | 0.9 | 1.6 | +0.7 |
| update: col_oc | 1.1 | 1.0 | -0.0 |
| update: actors | 26.4 | 36.8 | +10.3 |
| update: dynapoly | 0.5 | 0.5 | -0.0 |
| update: effects | 1.5 | 1.4 | -0.0 |
| update: interface | 0.3 | 0.2 | -0.1 |
| update: camera | 3.9 | 4.3 | +0.4 |
| update: env | 0.4 | 0.3 | -0.1 |
| actors: bg | 0.7 | 1.4 | +0.7 |
| actors: player | 3.4 | 4.3 | +0.9 |
| actors: npc | 0.6 | 7.7 | +7.1 |
| actors: enemy | 15.0 | 14.3 | -0.7 |
| actors: prop | 0.5 | 0.5 | +0.0 |
| actors: itemaction | 1.0 | 1.1 | +0.0 |
| actors: misc | 0.0 | 0.7 | +0.7 |

## By area (each leg, all laps): ms a frame, and frames over 52 ms

| area | vanilla | mod | difference | vanilla slow | mod slow |
|---|---|---|---|---|---|
| 1: Across the west flat (under) -> Straight down (under) | 53.8 | 73.4 | +19.6 | 60% | 98% |
| 2: Straight down (under) -> North beach | 60.6 | 86.7 | +26.1 | 84% | 100% |
| 3: North beach -> Beach shallows (under) | 55.3 | 81.5 | +26.2 | 46% | 100% |
| 4: Beach shallows (under) -> Pillar ruins | 59.0 | 95.3 | +36.3 | 85% | 100% |
| 5: Pillar ruins -> Pillar ruins (under) | 61.9 | 90.8 | +29.0 | 92% | 100% |
| 6: Pillar ruins (under) -> Lab shore and dock | 69.9 | 85.3 | +15.5 | 98% | 100% |
| 7: Lab shore and dock -> Gerudo river inflow | 64.8 | 82.3 | +17.4 | 98% | 100% |
| 8: Gerudo river inflow -> Rock garden (under) | 68.7 | 96.2 | +27.5 | 94% | 100% |
| 9: Rock garden (under) -> West weed flat | 65.9 | 97.3 | +31.5 | 98% | 100% |
| 10: West weed flat -> West weed flat (under) | 59.3 | 81.0 | +21.7 | 95% | 100% |
| 11: West weed flat (under) -> The drop-off (under) | 60.4 | 85.1 | +24.8 | 98% | 100% |
| 12: The drop-off (under) -> Deep basin (under) | 61.4 | 71.3 | +9.8 | 98% | 100% |
| 13: Deep basin (under) -> Water Temple gate | 53.6 | 63.4 | +9.8 | 61% | 99% |
| 14: Water Temple gate -> South bridge | 58.9 | 66.8 | +7.9 | 87% | 100% |
| 15: South bridge -> Island shallows (under) | 56.8 | 67.1 | +10.3 | 62% | 99% |
| 16: Island shallows (under) -> Island shallows | 58.4 | 72.2 | +13.9 | 88% | 100% |
| 17: Island shallows -> East weed flats | 51.2 | 58.3 | +7.1 | 20% | 89% |
| 18: East weed flats -> East weed flats (under) | 50.1 | 61.2 | +11.1 | 4% | 99% |
| 19: East weed flats (under) -> South reed bank | 50.3 | 61.0 | +10.7 | 12% | 98% |
| 20: South reed bank -> Under the lilies | 50.5 | 63.9 | +13.4 | 13% | 100% |
| 21: Under the lilies -> East shore | 55.9 | 70.3 | +14.4 | 62% | 100% |
| 22: East shore -> Across the west flat (under) | 54.5 | 60.3 | +5.8 | 59% | 87% |

## CPU time by kind (share of all samples)

| kind | vanilla | mod |
|---|---|---|
| audio | 33.1% | 34.8% |
| everything else | 32.1% | 26.5% |
| collision | 18.8% | 13.8% |
| the mod's own code | 1.4% | 11.3% |
| interrupts and OS | 6.1% | 7.0% |
| matrices and animation | 8.5% | 6.5% |

## The top functions in the mod run

| function | vanilla | mod |
|---|---|---|
| `AudioSynth_ProcessNote` | 6.97% | 6.60% |
| `__osRestoreInt` | 3.68% | 4.36% |
| `Audio_ProcessNotes` | 2.80% | 3.30% |
| `EnFishmod_ScatterDraw` | 0.00% | 3.03% |
| `AudioSeq_SequenceChannelProcessScript` | 2.63% | 2.99% |
| `AudioSynth_ProcessEnvelope` | 1.60% | 2.58% |
| `AudioLoad_DmaSampleData` | 4.98% | 2.46% |
| `Audio_InitSampleState` | 1.65% | 2.38% |
| `(no` | 0.75% | 2.09% |
| `Actor_UpdateAll` | 1.55% | 1.94% |
| `Matrix_MtxFToMtx` | 2.22% | 1.92% |
| `BgCheck_RaycastDownStaticList` | 2.46% | 1.85% |
| `Fishmod_DebugPlayUpdate` | 0.00% | 1.71% |
| `AudioSynth_DoOneAudioUpdate` | 1.53% | 1.70% |
| `Audio_SequenceChannelProcessSound` | 1.30% | 1.42% |
| `CollisionPoly_GetVertices` | 0.51% | 1.32% |
| `Math3D_TriChkPointParaYImpl` | 1.03% | 1.20% |
| `AudioSynth_FinalResample` | 0.44% | 1.10% |
| `sins` | 0.86% | 0.90% |
| `memset` | 0.00% | 0.87% |
| `DynaPoly_UnsetAllInteractFlags` | 0.39% | 0.87% |
| `BgCheck_SphVsStaticWall` | 1.16% | 0.85% |
| `SkelAnime_GetFrameData` | 1.08% | 0.78% |
| `Actor_DrawAll` | 0.44% | 0.77% |
| `Matrix_TranslateRotateZYX` | 0.88% | 0.76% |
| `func_800DB03C` | 0.50% | 0.75% |
| `Math_FAtanContFracF` | 0.94% | 0.71% |
| `AudioSeq_ScriptReadU8` | 0.62% | 0.71% |
| `CollisionPoly_CheckYIntersect` | 0.58% | 0.69% |
| `Audio_SequencePlayerProcessSound` | 0.61% | 0.68% |

("The mod's own code" counts functions defined in the mod's files; in vanilla's column it's the few that share a name with the game's.)
