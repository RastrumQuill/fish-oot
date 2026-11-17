# Code size scan (static)

- 31517 routines; 7756 a frame can reach by direct calls; 208 rows over the lines (loops over 2 KB, routines over 4 KB), the 208 biggest shown
- path: the loop's (or routine's) own code plus the routines it calls, to the depth given, each once, whole
- conflict: for a path that fits the 16 KB cache, its bytes on lines another part of it also uses

## Loops

| routine | line | own bytes | path KB | routines | calls in it | by pointer | loops inside | conflict | system | file |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---|---|
| `EnFishmod_FishAI` | 1951 | 3,836 | 14.0 | 35 | 69 | 0 | 0 | 3,552 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_FishAI` | 1777 | 3,736 | 14.1 | 34 | 64 | 0 | 0 | 3,488 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_FishAI` | 1860 | 3,404 | 14.2 | 35 | 59 | 0 | 0 | 3,392 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_FishAI` | 1719 | 4,240 | 14.9 | 37 | 77 | 0 | 0 | 3,232 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `BgCheck_CheckWallImpl` | 3579 | 1,160 | 12.7 | 15 | 4 | 0 | 0 | 2,944 | Collision | src/code/z_bgcheck.c |
| `EnFishmod_RodUpdate` | 696 | 2,264 | 11.9 | 36 | 31 | 0 | 0 | 2,848 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_rod.c |
| `EnFishmod_RodUpdate` | 825 | 1,824 | 11.5 | 36 | 28 | 0 | 0 | 2,848 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_rod.c |
| `AudioSeq_ProcessSequences` | 2184 | 60 | 10.0 | 18 | 2 | 0 | 0 | 2,496 | Audio | src/audio/internal/seqplayer.c |
| `BgCheck_CheckWallImpl` | 3477 | 688 | 12.2 | 15 | 3 | 0 | 0 | 2,464 | Collision | src/code/z_bgcheck.c |
| `BgCheck_CheckWallImpl` | 3496 | 696 | 12.2 | 15 | 3 | 0 | 0 | 2,432 | Collision | src/code/z_bgcheck.c |
| `EnFishmod_RodUpdate` | 675 | 1,840 | 11.4 | 36 | 27 | 0 | 0 | 2,304 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_rod.c |
| `EnFishmod_Draw` | 3208 | 832 | 14.1 | 32 | 12 | 0 | 0 | 2,016 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `Actor_UpdateBgCheckInfo` | 1450 | 696 | 6.0 | 17 | 9 | 0 | 0 | 1,888 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `AudioHeap_ResetStep` | 942 | 464 | 11.8 | 29 | 3 | 0 | 5 | 1,824 | Audio | src/audio/internal/heap.c |
| `Actor_DrawAll` | 3141 | 480 | 9.6 | 33 | 10 | 0 | 2 | 1,664 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `Player_ProcessSceneCollision` | 11131 | 1,096 | 14.2 | 37 | 10 | 0 | 0 | 1,568 | Link | src/overlays/actors/ovl_player_actor/z_player.c |
| `func_800F8F88` | 695 | 40 | 4.8 | 9 | 3 | 0 | 0 | 1,504 | Audio | src/audio/game/sfx.c |
| `EnFishmod_FishAI` | 2000 | 2,788 | 11.3 | 28 | 51 | 0 | 0 | 1,440 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `AudioHeap_ResetStep` | 914 | 376 | 9.4 | 24 | 1 | 0 | 3 | 1,408 | Audio | src/audio/internal/heap.c |
| `EnFishmod_RodUpdate` | 1031 | 880 | 11.6 | 24 | 9 | 0 | 0 | 1,312 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_rod.c |
| `BgCheck_CheckLineImpl` | 2092 | 212 | 5.3 | 8 | 2 | 0 | 0 | 1,280 | Collision | src/code/z_bgcheck.c |
| `BgCheck_CheckLineImpl` | 3812 | 204 | 5.2 | 8 | 2 | 0 | 1 | 1,280 | Collision | src/code/z_bgcheck.c |
| `EnFishmod_FishAI` | 1849 | 2,552 | 11.0 | 28 | 47 | 0 | 0 | 1,216 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `BgCheck_RaycastDownImpl` | 3185 | 580 | 3.9 | 13 | 4 | 0 | 0 | 1,216 | Collision | src/code/z_bgcheck.c |
| `BgCheck_RaycastDownImpl` | 3185 | 580 | 3.9 | 13 | 4 | 0 | 0 | 1,216 | Collision | src/code/z_bgcheck.c |
| `EnFishmod_RodUpdate` | 816 | 1,280 | 11.6 | 28 | 18 | 0 | 0 | 1,184 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_rod.c |
| `AudioSeq_SequencePlayerProcessSequence` | 2168 | 40 | 7.7 | 15 | 1 | 0 | 0 | 1,184 | Audio | src/audio/internal/seqplayer.c |
| `EnFishmod_FishAI` | 1975 | 2,172 | 11.8 | 29 | 37 | 0 | 0 | 1,152 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_FishAI` | 1985 | 2,088 | 11.7 | 29 | 37 | 0 | 0 | 1,152 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_Draw` | 3132 | 1,528 | 8.9 | 20 | 20 | 0 | 1 | 1,152 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_Draw` | 3109 | 1,524 | 8.5 | 19 | 19 | 0 | 1 | 1,152 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `Actor_DrawAll` | 3141 | 456 | 8.3 | 27 | 9 | 0 | 2 | 1,120 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `BgCheck_CheckCeilingImpl` | 3692 | 536 | 4.1 | 8 | 4 | 0 | 0 | 1,088 | Collision | src/code/z_bgcheck.c |
| `BgCheck_CheckCeilingImpl` | 3003 | 480 | 4.0 | 8 | 4 | 0 | 0 | 1,088 | Collision | src/code/z_bgcheck.c |
| `AudioSeq_SeqLayerProcessScript` | 665 | 144 | 7.4 | 17 | 5 | 0 | 0 | 960 | Audio | src/audio/internal/seqplayer.c |
| `Actor_DrawAll` | 844 | 444 | 6.5 | 23 | 10 | 0 | 2 | 864 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `Fishmod_ReleaseBottleFish` | 522 | 292 | 2.7 | 11 | 3 | 0 | 0 | 832 | Fishing mod | src/code/fishmod.c |
| `EnFishmod_ManagerDraw` | 995 | 408 | 8.3 | 16 | 7 | 0 | 0 | 800 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_manager.c |
| `EnFishmod_FishAI` | 1902 | 2,280 | 10.7 | 27 | 40 | 0 | 0 | 768 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `BgCheck_CheckWallImpl` | 3508 | 1,212 | 10.0 | 15 | 6 | 0 | 0 | 768 | Collision | src/code/z_bgcheck.c |
| `Audio_PlayActiveSfx` | 511 | 276 | 4.3 | 13 | 5 | 0 | 0 | 768 | Audio | src/audio/game/sfx.c |
| `Audio_PlayActiveSfx` | 472 | 360 | 4.4 | 13 | 6 | 0 | 0 | 736 | Audio | src/audio/game/sfx.c |
| `EnFishmod_RodDraw` | 1373 | 1,156 | 5.1 | 15 | 19 | 0 | 1 | 704 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_rod.c |
| `EnFishmod_FishAI` | 1903 | 2,204 | 10.6 | 27 | 38 | 0 | 0 | 672 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `BossSst_HandCollisionCheck` | 711 | 132 | 4.2 | 16 | 4 | 0 | 0 | 672 | Actors | src/overlays/actors/ovl_Boss_Sst/z_boss_sst.c |
| `Actor_UpdateBgCheckInfo` | 1473 | 264 | 4.1 | 9 | 4 | 0 | 0 | 672 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `AudioLoad_LoadPermanentSamples` | 2543 | 212 | 3.2 | 8 | 1 | 0 | 0 | 640 | Audio | src/audio/internal/load.c |
| `EnFishmod_RodUpdate` | 800 | 860 | 8.1 | 16 | 8 | 0 | 0 | 608 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_rod.c |
| `Actor_DrawAll` | 2948 | 388 | 6.4 | 22 | 5 | 0 | 0 | 608 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `Actor_DrawAll` | 3110 | 372 | 6.1 | 21 | 4 | 0 | 2 | 608 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `Actor_DrawAll` | 3094 | 360 | 6.1 | 21 | 4 | 0 | 1 | 608 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `Actor_UpdateAll` | 2625 | 464 | 5.2 | 15 | 9 | 2 | 0 | 608 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `EnGeldB_CollisionCheck.part.0` | 1418 | 320 | 3.4 | 15 | 8 | 0 | 0 | 608 | Actors | src/overlays/actors/ovl_En_GeldB/z_en_geldb.c |
| `EnFishmod_FishAI` | 1879 | 1,180 | 9.0 | 24 | 23 | 0 | 0 | 576 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `AudioSynth_Update` | 275 | 128 | 3.1 | 6 | 6 | 0 | 0 | 576 | Audio | src/audio/internal/synthesis.c |
| `Actor_DrawLensActors` | 2711 | 104 | 2.8 | 12 | 1 | 0 | 0 | 576 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `Actor_UpdateBgCheckInfo` | 1510 | 620 | 3.7 | 15 | 8 | 0 | 0 | 544 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `Scatter_Build` | 2518 | 3,192 | 5.4 | 6 | 6 | 0 | 8 | 512 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c |
| `Scatter_Build` | 2511 | 2,188 | 4.4 | 6 | 6 | 0 | 8 | 512 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c |
| `BodyBreak_SpawnParts` | 3884 | 296 | 3.6 | 14 | 5 | 0 | 0 | 512 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `Math3D_TriVsSphIntersect` | 1572 | 244 | 3.2 | 8 | 3 | 0 | 0 | 512 | Collision | src/code/sys_math3d.c |
| `Attention_Draw` | 473 | 108 | 2.1 | 8 | 5 | 0 | 0 | 512 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `AudioMgr_ThreadEntry` | 338 | 180 | 2.5 | 17 | 4 | 0 | 0 | 448 | Audio | src/code/audio_thread_manager.c |
| `Actor_Spawn` | 3396 | 260 | 2.2 | 11 | 6 | 0 | 0 | 448 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `Actor_Spawn` | 3391 | 216 | 2.2 | 9 | 3 | 0 | 0 | 384 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `Actor_Spawn` | 3489 | 184 | 2.1 | 8 | 1 | 0 | 0 | 384 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `Actor_Spawn` | 3489 | 172 | 2.1 | 8 | 1 | 0 | 0 | 384 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `Audio_Update` | 2365 | 80 | 9.8 | 14 | 4 | 0 | 0 | 352 | Audio | src/audio/game/general.c |
| `BgCheck_CheckWallImpl` | 3559 | 1,096 | 7.2 | 11 | 3 | 0 | 0 | 352 | Collision | src/code/z_bgcheck.c |
| `Attention_FindActorInCategory` | 3686 | 332 | 2.9 | 5 | 2 | 0 | 0 | 320 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `Actor_GetProjectileActor` | 4037 | 260 | 2.4 | 5 | 4 | 0 | 0 | 320 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `EnFishmod_ScatterDraw` | 3342 | 1,240 | 5.3 | 9 | 8 | 0 | 3 | 288 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c |
| `Player_ProcessSceneCollision` | 11357 | 492 | 5.0 | 17 | 11 | 0 | 0 | 288 | Link | src/overlays/actors/ovl_player_actor/z_player.c |
| `Actor_SpawnTransitionActors` | 3518 | 168 | 2.0 | 9 | 1 | 0 | 0 | 288 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `EnFishmod_RodUpdate` | 466 | 960 | 5.9 | 23 | 13 | 0 | 0 | 256 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_rod.c |
| `DynaPoly_UpdateContext` | 4681 | 60 | 3.7 | 9 | 1 | 0 | 0 | 256 | Collision | src/code/z_bgcheck.c |
| `BgCheck_SphVsStaticWall` | 1860 | 1,976 | 3.2 | 7 | 5 | 0 | 0 | 256 | Collision | src/code/z_bgcheck.c |
| `Actor_DrawAll` | 2416 | 152 | 3.2 | 18 | 8 | 0 | 0 | 256 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `BgCheck_SphVsStaticWall` | 1476 | 1,532 | 2.8 | 7 | 5 | 0 | 0 | 256 | Collision | src/code/z_bgcheck.c |
| `BgCheck_SphVsStaticWall` | 1498 | 1,384 | 2.6 | 7 | 5 | 0 | 0 | 256 | Collision | src/code/z_bgcheck.c |
| `AudioLoad_AsyncLoadInner` | 1419 | 636 | 5.3 | 10 | 4 | 0 | 0 | 224 | Audio | src/audio/internal/load.c |
| `BgCheck_CheckLineImpl` | 3003 | 788 | 4.6 | 12 | 7 | 0 | 0 | 224 | Collision | src/code/z_bgcheck.c |
| `AudioSynth_Update` | 299 | 376 | 8.1 | 12 | 4 | 0 | 2 | 192 | Audio | src/audio/internal/synthesis.c |
| `AudioSynth_DoOneAudioUpdate` | 927 | 200 | 6.1 | 7 | 1 | 0 | 1 | 192 | Audio | src/audio/internal/synthesis.c |
| `AudioSynth_DoOneAudioUpdate` | 239 | 172 | 6.0 | 7 | 1 | 0 | 1 | 192 | Audio | src/audio/internal/synthesis.c |
| `AudioLoad_AsyncLoadInner` | 1419 | 624 | 5.3 | 9 | 3 | 0 | 0 | 192 | Audio | src/audio/internal/load.c |
| `AudioLoad_AsyncLoadInner` | 1401 | 324 | 5.0 | 9 | 3 | 0 | 0 | 192 | Audio | src/audio/internal/load.c |
| `EnFishmod_ScatterDraw` | 3345 | 1,172 | 4.1 | 7 | 7 | 0 | 3 | 192 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c |
| `Audio_PlayActiveSfx` | 463 | 292 | 3.6 | 10 | 3 | 0 | 0 | 192 | Audio | src/audio/game/sfx.c |
| `Fishmod_PlayUpdate` | 264 | 216 | 2.9 | 6 | 4 | 0 | 0 | 192 | Fishing mod | src/code/fishmod.c |
| `AudioThread_UpdateImpl` | 381 | 272 | 6.2 | 11 | 5 | 0 | 0 | 160 | Audio | src/audio/internal/thread.c |
| `EnFishmod_SwimTowardImpl` | 1034 | 492 | 5.9 | 14 | 10 | 0 | 0 | 160 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `AudioSeq_SequencePlayerProcessSequence` | 2149 | 840 | 4.2 | 12 | 6 | 0 | 0 | 160 | Audio | src/audio/internal/seqplayer.c |
| `BgCheck_CheckLineAgainstSSList` | 2053 | 560 | 2.1 | 7 | 1 | 0 | 1 | 160 | Collision | src/code/z_bgcheck.c |
| `BgCheck_CheckLineAgainstSSList` | 2067 | 312 | 2.1 | 8 | 2 | 0 | 1 | 160 | Collision | src/code/z_bgcheck.c |
| `BgCheck_CheckLineAgainstSSList` | 2067 | 284 | 2.1 | 8 | 2 | 0 | 1 | 160 | Collision | src/code/z_bgcheck.c |
| `BgCheck_CheckLineAgainstSSList` | 2037 | 480 | 2.0 | 7 | 1 | 0 | 0 | 160 | Collision | src/code/z_bgcheck.c |
| `BgCheck_CheckLineAgainstSSList` | 2041 | 476 | 2.0 | 7 | 1 | 0 | 0 | 160 | Collision | src/code/z_bgcheck.c |
| `EnFishmod_ManagerDraw` | 1032 | 128 | 4.8 | 10 | 4 | 0 | 0 | 128 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_manager.c |
| `EnFishmod_Draw` | 2602 | 388 | 3.5 | 11 | 8 | 0 | 0 | 128 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_Draw` | 2597 | 324 | 3.4 | 10 | 7 | 0 | 0 | 128 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_SwimTowardImpl` | 746 | 464 | 2.4 | 7 | 10 | 0 | 1 | 128 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_FishAI` | 1440 | 132 | 2.4 | 7 | 2 | 0 | 0 | 128 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_SwimTowardImpl` | 1073 | 412 | 2.2 | 6 | 9 | 0 | 1 | 128 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `Actor_Draw.part.0` | 2741 | 428 | 2.0 | 8 | 5 | 2 | 0 | 128 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `EnFishmod_SwimTowardImpl` | 1068 | 484 | 5.6 | 13 | 7 | 0 | 0 | 96 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `AudioSeq_SequenceChannelProcessScript` | 1839 | 24 | 4.5 | 8 | 1 | 0 | 0 | 96 | Audio | src/audio/internal/seqplayer.c |
| `BgCheck_CheckLineImpl` | 3804 | 124 | 3.7 | 6 | 1 | 0 | 1 | 96 | Collision | src/code/z_bgcheck.c |
| `BgCheck_CheckLineImpl` | 3805 | 104 | 3.6 | 6 | 1 | 0 | 0 | 96 | Collision | src/code/z_bgcheck.c |
| `BgCheck_CheckLineImpl` | 2091 | 96 | 3.6 | 6 | 1 | 0 | 0 | 96 | Collision | src/code/z_bgcheck.c |
| `EnFishmod_ManagerDraw` | 1006 | 148 | 3.2 | 10 | 4 | 0 | 0 | 96 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_manager.c |
| `EnFishmod_Update` | 1521 | 428 | 3.0 | 12 | 8 | 0 | 1 | 96 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_RodDraw` | 1226 | 444 | 2.7 | 9 | 6 | 0 | 0 | 96 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_rod.c |
| `EnFishmod_RodDraw` | 1196 | 428 | 2.7 | 9 | 6 | 0 | 1 | 96 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_rod.c |
| `EnFishmod_RodDraw` | 1235 | 368 | 2.6 | 9 | 6 | 0 | 0 | 96 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_rod.c |
| `EnFishmod_RodDraw` | 1239 | 364 | 2.6 | 9 | 6 | 0 | 0 | 96 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_rod.c |
| `EnFishmod_ManagerUpdate` | 678 | 440 | 2.6 | 10 | 2 | 0 | 0 | 96 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_manager.c |
| `Audio_SetSfxProperties` | 2661 | 300 | 2.2 | 6 | 5 | 0 | 0 | 96 | Audio | src/audio/game/general.c |
| `AudioSeq_SeqLayerProcessScriptStep5` | 697 | 136 | 2.2 | 8 | 2 | 0 | 0 | 96 | Audio | src/audio/internal/seqplayer.c |
| `AudioSynth_Update` | 289 | 256 | 6.8 | 9 | 1 | 0 | 2 | 64 | Audio | src/audio/internal/synthesis.c |
| `EnFishmod_SwimTowardImpl` | 999 | 288 | 5.6 | 13 | 5 | 0 | 0 | 64 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `AudioSeq_SequencePlayerProcessSequence` | 2128 | 1,444 | 4.3 | 13 | 7 | 0 | 2 | 64 | Audio | src/audio/internal/seqplayer.c |
| `Audio_UpdateActiveSequences` | 741 | 1,056 | 4.1 | 8 | 2 | 0 | 2 | 64 | Audio | src/audio/game/sequence.c |
| `Audio_UpdateActiveSequences` | 673 | 1,028 | 4.1 | 8 | 2 | 0 | 2 | 64 | Audio | src/audio/game/sequence.c |
| `AudioHeap_AllocCached` | 131 | 384 | 3.6 | 14 | 3 | 0 | 2 | 64 | Audio | src/audio/internal/heap.c |
| `AudioHeap_AllocCached` | 131 | 300 | 3.5 | 14 | 4 | 0 | 1 | 64 | Audio | src/audio/internal/heap.c |
| `Audio_UpdateActiveSequences` | 470 | 668 | 3.5 | 7 | 1 | 0 | 1 | 64 | Audio | src/audio/game/sequence.c |
| `AudioHeap_AllocCached` | 528 | 268 | 3.5 | 14 | 4 | 0 | 1 | 64 | Audio | src/audio/internal/heap.c |
| `AudioHeap_AllocCached` | 549 | 228 | 3.5 | 14 | 4 | 0 | 1 | 64 | Audio | src/audio/internal/heap.c |
| `EnFishmod_ManagerDraw` | 1011 | 184 | 3.1 | 9 | 3 | 0 | 0 | 64 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_manager.c |
| `AudioSeq_SequenceChannelProcessScript` | 1514 | 788 | 3.0 | 14 | 9 | 0 | 0 | 64 | Audio | src/audio/internal/seqplayer.c |
| `BgCheck_SphVsDynaWall` | 5125 | 588 | 3.0 | 7 | 3 | 0 | 0 | 64 | Collision | src/code/z_bgcheck.c |
| `Audio_ProcessSeqCmds` | 411 | 36 | 2.9 | 7 | 1 | 0 | 0 | 64 | Audio | src/audio/game/sequence.c |
| `Audio_AllocNote` | 905 | 708 | 2.8 | 9 | 17 | 0 | 0 | 64 | Audio | src/audio/internal/playback.c |
| `EnFishmod_FindWaterPoint` | 680 | 272 | 2.6 | 8 | 4 | 0 | 0 | 64 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `AudioSeq_SequenceChannelProcessScript` | 1385 | 568 | 2.6 | 13 | 7 | 0 | 0 | 64 | Audio | src/audio/internal/seqplayer.c |
| `Fishmod_ExtendMessage` | 564 | 2,332 | 2.3 | 2 | 1 | 0 | 27 | 64 | Fishing mod | src/code/fishmod.c |
| `Math3D_TriVsSphIntersect` | 1511 | 496 | 2.3 | 6 | 4 | 0 | 0 | 64 | Collision | src/code/sys_math3d.c |
| `Fishmod_ExtendMessage` | 564 | 2,160 | 2.2 | 2 | 1 | 0 | 24 | 64 | Fishing mod | src/code/fishmod.c |
| `AudioSeq_SequencePlayerProcessSequence` | 2173 | 1,340 | 2.1 | 6 | 4 | 0 | 2 | 64 | Audio | src/audio/internal/seqplayer.c |
| `EnFishmod_StressPlantsDraw` | 3437 | 620 | 6.3 | 5 | 5 | 0 | 0 | 32 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c |
| `EnFishmod_StressPlantsDraw` | 3501 | 228 | 6.0 | 5 | 3 | 0 | 0 | 32 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c |
| `func_80037D98` | 6375 | 204 | 4.9 | 7 | 1 | 0 | 0 | 32 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `func_80037D98` | 6375 | 160 | 4.8 | 7 | 1 | 0 | 0 | 32 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `func_80037D98` | 6374 | 100 | 4.8 | 7 | 1 | 0 | 0 | 32 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `EnFishmod_ScatterDraw` | 1685 | 1,164 | 4.6 | 7 | 4 | 0 | 2 | 32 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c |
| `EnFishmod_ScatterDraw` | 1727 | 1,044 | 4.5 | 7 | 4 | 0 | 2 | 32 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c |
| `func_80037CB8` | 6327 | 152 | 4.4 | 6 | 3 | 0 | 0 | 32 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `EnFishmod_ScatterDraw` | 1563 | 704 | 4.2 | 7 | 4 | 0 | 0 | 32 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c |
| `EnFishmod_ScatterDraw` | 1746 | 644 | 4.1 | 7 | 6 | 0 | 0 | 32 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c |
| `Scatter_Build` | 2829 | 2,100 | 4.0 | 5 | 1 | 0 | 5 | 32 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c |
| `EnFishmod_ParticlesUpdate` | 114 | 300 | 3.2 | 10 | 9 | 0 | 0 | 32 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_particles.c |
| `EnFishmod_ProbeWater` | 727 | 320 | 3.1 | 8 | 3 | 0 | 1 | 32 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_ProbeWater` | 727 | 224 | 3.0 | 8 | 3 | 0 | 0 | 32 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `func_8002E830` | 1613 | 604 | 3.0 | 4 | 1 | 0 | 5 | 32 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `Math3D_TriVsSphIntersect` | 1554 | 292 | 3.0 | 7 | 3 | 0 | 0 | 32 | Collision | src/code/sys_math3d.c |
| `func_8002E830` | 1594 | 536 | 2.9 | 4 | 1 | 0 | 1 | 32 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `func_8002E830` | 1594 | 472 | 2.8 | 4 | 1 | 0 | 1 | 32 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `Scatter_Build` | 2736 | 256 | 2.8 | 7 | 2 | 0 | 0 | 32 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c |
| `AudioHeap_AllocCached` | 475 | 424 | 2.7 | 11 | 2 | 0 | 1 | 32 | Audio | src/audio/internal/heap.c |
| `AudioHeap_AllocCached` | 475 | 348 | 2.7 | 11 | 2 | 0 | 1 | 32 | Audio | src/audio/internal/heap.c |
| `Math3D_TriLineIntersect` | 1352 | 252 | 2.7 | 5 | 2 | 0 | 0 | 32 | Collision | src/code/sys_math3d.c |
| `Scatter_Build` | 2802 | 716 | 2.6 | 5 | 1 | 0 | 0 | 32 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c |
| `Attention_Draw` | 510 | 504 | 2.6 | 8 | 5 | 0 | 0 | 32 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `Scatter_Build` | 2722 | 552 | 2.5 | 5 | 1 | 0 | 0 | 32 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c |
| `BgCheck_RaycastDownStaticList` | 855 | 516 | 2.4 | 4 | 1 | 0 | 3 | 32 | Collision | src/code/z_bgcheck.c |
| `EnFishmod_OpenWaterYaw` | 942 | 116 | 2.4 | 5 | 3 | 0 | 0 | 32 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `BgCheck_CheckStaticCeiling` | 1951 | 400 | 2.3 | 4 | 1 | 0 | 0 | 32 | Collision | src/code/z_bgcheck.c |
| `BgCheck_CheckStaticCeiling` | 1941 | 336 | 2.3 | 4 | 1 | 0 | 0 | 32 | Collision | src/code/z_bgcheck.c |
| `SkelAnime_DrawFlex` | 728 | 108 | 2.2 | 6 | 2 | 0 | 0 | 32 | Animation | src/code/z_skelanime.c |
| `Fishmod_DpadDraw` | 512 | 1,072 | 2.2 | 7 | 3 | 0 | 3 | 32 | Fishing mod | src/code/fishmod_items.c |
| `BgCheck_CheckStaticCeiling` | 1951 | 236 | 2.2 | 4 | 1 | 0 | 0 | 32 | Collision | src/code/z_bgcheck.c |
| `SkelAnime_DrawFlexLod` | 254 | 104 | 2.2 | 6 | 2 | 0 | 0 | 32 | Animation | src/code/z_skelanime.c |
| `SkelAnime_Draw` | 616 | 100 | 2.2 | 6 | 2 | 0 | 0 | 32 | Animation | src/code/z_skelanime.c |
| `SkelAnime_DrawFlexOpa` | 473 | 96 | 2.1 | 6 | 2 | 0 | 0 | 32 | Animation | src/code/z_skelanime.c |
| `SkelAnime_DrawLod` | 133 | 100 | 2.1 | 6 | 2 | 0 | 0 | 32 | Animation | src/code/z_skelanime.c |
| `SkelAnime_DrawOpa` | 350 | 92 | 2.1 | 6 | 2 | 0 | 0 | 32 | Animation | src/code/z_skelanime.c |
| `EnFishmod_Update` | 2377 | 676 | 46.9 | 85 | 15 | 0 | 0 |  | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_Update` | 2347 | 520 | 21.4 | 37 | 10 | 0 | 0 |  | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_Update` | 2323 | 356 | 21.1 | 36 | 7 | 0 | 0 |  | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_RodUpdate` | 440 | 2,456 | 17.8 | 42 | 36 | 0 | 0 |  | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_rod.c |
| `EnFishmod_RodUpdate` | 1310 | 120 | 7.6 | 9 | 3 | 0 | 0 | 0 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_rod.c |
| `Math3D_LineVsCube` | 853 | 1,716 | 5.7 | 6 | 11 | 0 | 0 | 0 | Collision | src/code/sys_math3d.c |
| `Fishmod_ReleaseBottleFish` | 488 | 124 | 5.3 | 3 | 1 | 0 | 0 | 0 | Fishing mod | src/code/fishmod.c |
| `CollisionPoly_LineVsPoly` | 686 | 216 | 3.4 | 7 | 3 | 0 | 0 | 0 | Collision | src/code/z_bgcheck.c |
| `AudioSeq_SequenceChannelProcessScript` | 1432 | 1,792 | 2.8 | 7 | 4 | 0 | 0 | 0 | Audio | src/audio/internal/seqplayer.c |
| `Scatter_Build` | 2649 | 2,580 | 2.5 | 1 | 0 | 0 | 6 | 0 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c |
| `Scatter_Build` | 2649 | 2,572 | 2.5 | 1 | 0 | 0 | 6 | 0 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c |
| `Scatter_Build` | 2217 | 1,380 | 2.5 | 4 | 8 | 0 | 3 | 0 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c |
| `EnFishmod_RodUpdate` | 1314 | 116 | 2.5 | 7 | 2 | 0 | 0 | 0 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_rod.c |
| `EnFishmod_OverrideLimbDraw` | 2396 | 536 | 2.4 | 5 | 4 | 0 | 0 | 0 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_OpenWaterTo` | 1367 | 144 | 2.3 | 6 | 1 | 0 | 0 | 0 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `BgCheck_RaycastDownStatic` | 1318 | 212 | 2.3 | 3 | 2 | 0 | 0 | 0 | Collision | src/code/z_bgcheck.c |
| `CollisionPoly_LineVsPoly` | 683 | 176 | 2.3 | 5 | 2 | 0 | 0 | 0 | Collision | src/code/z_bgcheck.c |
| `AudioMgr_ThreadEntry` | 278 | 348 | 2.3 | 13 | 11 | 0 | 1 | 0 | Audio | src/code/audio_thread_manager.c |
| `BgCheck_CheckLineInSubdivision` | 2129 | 112 | 2.2 | 5 | 1 | 0 | 0 | 0 | Collision | src/code/z_bgcheck.c |
| `AudioMgr_ThreadEntry` | 276 | 336 | 2.2 | 12 | 10 | 0 | 1 | 0 | Audio | src/code/audio_thread_manager.c |
| `AudioSeq_SeqLayerProcessScript` | 665 | 44 | 2.2 | 7 | 2 | 0 | 0 | 0 | Audio | src/audio/internal/seqplayer.c |
| `func_800374E0` | 5056 | 484 | 2.1 | 4 | 17 | 0 | 0 | 0 | Actor framework (z_actor.c) | src/code/z_actor.c |
| `EnFishmod_ManagerDraw` | 834 | 660 | 2.1 | 5 | 3 | 0 | 0 | 0 | Fishing mod | src/overlays/actors/ovl_En_Fishmod/fishmod_manager.c |
| `func_800374E0` | 6144 | 396 | 2.0 | 4 | 7 | 0 | 0 | 0 | Actor framework (z_actor.c) | src/code/z_actor.c |

## Routines without such a loop

| routine | line | own bytes | path KB | routines | calls in it | by pointer | loops inside | conflict | system | file |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---|---|
| `BgCheck_AnyRaycastDown1` | 3332 | 164 | 5.2 | 2 | 1 | 0 | 0 | 0 | Collision | src/code/z_bgcheck.c |
| `BgCheck_EntityRaycastDown1` | 3242 | 76 | 5.2 | 2 | 1 | 0 | 0 | 0 | Collision | src/code/z_bgcheck.c |
| `BgCheck_EntityRaycastDown4` | 3279 | 76 | 5.2 | 2 | 1 | 0 | 0 | 0 | Collision | src/code/z_bgcheck.c |
| `BgCheck_EntityRaycastDown3` | 3266 | 72 | 5.2 | 2 | 1 | 0 | 0 | 0 | Collision | src/code/z_bgcheck.c |
| `BgCheck_EntityRaycastDown5` | 3292 | 72 | 5.2 | 2 | 1 | 0 | 0 | 0 | Collision | src/code/z_bgcheck.c |
| `BgCheck_EntityRaycastDown6` | 3305 | 72 | 5.2 | 2 | 1 | 0 | 0 | 0 | Collision | src/code/z_bgcheck.c |
| `BgCheck_EntityRaycastDown2` | 3255 | 68 | 5.2 | 2 | 1 | 0 | 0 | 0 | Collision | src/code/z_bgcheck.c |

