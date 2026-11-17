# CPU profile by system

- 1 exact profile(s) from ares
- 8,857,954,688 instructions, 590.7 s of CPU time on the emulator's clock (55,377,602,105 cycles); idle 51.9% of it
- data cache: 166,133,025 line fills, 102,526,387 write-backs; code cache: 137,763,108 line fills (a fill is about 40-48 cycles: 31.3% of all cycles)
- per frame (12172 frames): 2,004,835 cycles working (21.38 ms)
- the profiler and test harness's own code: 4.0% of the run
- shares below are of the time the game's own code was working (idle and the harness left out)

## Systems

| system | share | CPU s | instructions | cycles/instr | data fills | code fills | cache share of its time |
|---|---:|---:|---:|---:|---:|---:|---:|
| libultra (OS: threads, interrupts, PI, SI, VI, timers) | 35.86% | 93.34 | 2,339,445,395 | 3.74 | 54,786,118 | 26,642,122 | 59% |
| Audio | 22.99% | 59.83 | 1,424,565,246 | 3.94 | 39,768,636 | 27,692,916 | 66% |
| Maths (matrices, vectors, trig, random) | 9.75% | 25.38 | 488,939,175 | 4.87 | 9,600,114 | 15,429,979 | 57% |
| Animation | 4.85% | 12.62 | 249,604,869 | 4.74 | 5,784,421 | 6,287,096 | 56% |
| Collision | 4.49% | 11.68 | 175,615,092 | 6.23 | 6,870,098 | 8,016,767 | 73% |
| Scheduler, threads and system | 3.06% | 7.97 | 88,344,760 | 8.45 | 4,084,489 | 8,065,247 | 83% |
| Link | 2.51% | 6.53 | 64,788,308 | 9.44 | 2,535,350 | 7,407,757 | 84% |
| Actor framework (z_actor.c) | 2.41% | 6.27 | 54,970,514 | 10.70 | 3,702,093 | 5,636,256 | 86% |
| Actors | 2.01% | 5.22 | 75,012,553 | 6.53 | 3,324,581 | 2,559,315 | 68% |
| Sky, weather and lights | 1.63% | 4.23 | 55,301,616 | 7.18 | 2,058,098 | 3,592,449 | 75% |
| HUD and map | 1.47% | 3.82 | 31,661,760 | 11.32 | 1,973,834 | 3,861,319 | 87% |
| Fishing mod | 1.36% | 3.54 | 66,227,784 | 5.01 | 765,351 | 2,410,794 | 57% |
| Camera and view | 1.27% | 3.30 | 27,849,132 | 11.12 | 1,679,360 | 3,108,570 | 84% |
| C library (memory copies, printf, 64-bit maths) | 1.14% | 2.96 | 48,049,995 | 5.78 | 2,002,286 | 600,077 | 53% |
| Loading (DMA, decompression, overlays) | 1.00% | 2.61 | 93,623,884 | 2.61 | 1,608,086 | 457,037 | 52% |
| Game loop (game.c, z_play.c) | 0.99% | 2.58 | 17,760,995 | 13.62 | 1,421,350 | 2,782,170 | 90% |
| Effects | 0.71% | 1.85 | 22,707,888 | 7.65 | 1,683,381 | 757,724 | 81% |
| Display list setup | 0.59% | 1.55 | 13,050,502 | 11.11 | 779,191 | 1,533,140 | 86% |
| Title, file select, menus | 0.41% | 1.06 | 20,405,240 | 4.85 | 777,334 | 276,607 | 69% |
| Text | 0.37% | 0.95 | 14,181,188 | 6.31 | 723,195 | 382,786 | 72% |
| Cutscenes | 0.33% | 0.86 | 10,071,774 | 7.97 | 686,359 | 521,755 | 83% |
| Scenes, rooms and objects | 0.29% | 0.76 | 9,445,984 | 7.59 | 356,722 | 735,514 | 80% |
| Other (no source) | 0.28% | 0.74 | 27,693,289 | 2.49 | 122,725 | 528,706 | 46% |
| Pause menu and screen effects | 0.15% | 0.39 | 13,172,559 | 2.78 | 197,800 | 177,380 | 56% |
| Memory | 0.07% | 0.17 | 4,441,567 | 3.59 | 132,853 | 50,977 | 63% |
| Screen transitions | 0.03% | 0.07 | 347,593 | 18.36 | 17,399 | 104,457 | 92% |
| Save data and inventory | 0.00% | 0.01 | 161,632 | 3.57 | 3,257 | 3,751 | 66% |
| Other engine code | 0.00% | 0.00 | 47,580 | 5.15 | 2,406 | 622 | 76% |

## libultra (OS: threads, interrupts, PI, SI, VI, timers): 35.86%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| exceptasm.c | 21.67% | 60.4% | 44,630,645 | 10,955,741 |
| writebackdcacheall.c | 3.39% | 9.4% | 0 | 91,193 |
| timerintr.c | 2.93% | 8.2% | 1,610,545 | 4,450,133 |
| recvmesg.c | 2.55% | 7.1% | 3,411,409 | 1,280,581 |
| interrupt.c | 1.20% | 3.4% | 210,847 | 1,327,690 |
| sendmesg.c | 0.89% | 2.5% | 1,624,771 | 1,123,078 |
| vimgr.c | 0.46% | 1.3% | 650,305 | 723,022 |
| startthread.c | 0.30% | 0.8% | 524,997 | 564,740 |
| viswapcontext.c | 0.25% | 0.7% | 423,483 | 496,344 |
| contreaddata.c | 0.25% | 0.7% | 430,929 | 483,331 |
| sptask.c | 0.22% | 0.6% | 142,866 | 817,150 |
| gettime.c | 0.16% | 0.4% | 235,886 | 302,163 |
| writebackdcache.c | 0.15% | 0.4% | 0 | 158,815 |
| sirawdma.c | 0.14% | 0.4% | 84,081 | 476,388 |
| controller.c | 0.14% | 0.4% | 222,902 | 244,721 |
| getcount.c | 0.13% | 0.4% | 0 | 135,854 |
| invaldcache.c | 0.13% | 0.4% | 0 | 455,235 |
| setintmask.c | 0.10% | 0.3% | 46,374 | 192,480 |
| virtualtophysical.c | 0.08% | 0.2% | 0 | 296,289 |
| sprawdma.c | 0.08% | 0.2% | 59,393 | 255,648 |
| contquery.c | 0.08% | 0.2% | 45,446 | 303,937 |
| sp.c | 0.06% | 0.2% | 0 | 118,738 |
| siacs.c | 0.06% | 0.2% | 86,964 | 194,433 |
| setcompare.c | 0.05% | 0.1% | 0 | 128,801 |
| epirawread.c | 0.05% | 0.1% | 34,378 | 57,122 |
| (55 more) | 0.35% | 1.0% | | |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `__osDispatchThread` | 9.87% | 27.5% | 5,337,083 | 451 | 94 | 27,129,989 | 1,409,541 | src/libultra/os/exceptasm.c |
| `__osException` | 8.30% | 23.1% | 2,456,359 | 824 | 225 | 13,037,134 | 7,502,358 | src/libultra/os/exceptasm.c |
| `osWritebackDCacheAll` | 3.39% | 9.4% | 59,379 | 13,916 | 2,055 | 0 | 91,193 | src/libultra/os/writebackdcacheall.c |
| `osRecvMesg` | 2.55% | 7.1% | 3,302,596 | 188 | 52 | 3,411,409 | 1,280,581 | src/libultra/os/recvmesg.c |
| `__osEnqueueAndYield` | 2.05% | 5.7% | 2,880,723 | 174 | 63 | 2,985,684 | 1,229,814 | src/libultra/os/exceptasm.c |
| `__osTimerInterrupt` | 1.91% | 5.3% | 2,175,425 | 214 | 84 | 1,280,744 | 2,715,255 | src/libultra/os/timerintr.c |
| `__osEnqueueThread` | 1.12% | 3.1% | 8,003,748 | 34 | 14 | 1,378,415 | 418,829 | src/libultra/os/exceptasm.c |
| `__osInsertTimer` | 1.02% | 2.9% | 2,187,593 | 114 | 44 | 329,566 | 1,734,626 | src/libultra/os/timerintr.c |
| `osSendMesg` | 0.89% | 2.5% | 688,804 | 317 | 48 | 1,624,771 | 1,123,078 | src/libultra/os/sendmesg.c |
| `__osDisableInt` | 0.88% | 2.5% | 9,116,270 | 24 | 15 | 210,847 | 1,036,015 | src/libultra/os/interrupt.c |
| `viMgrMain` | 0.46% | 1.3% | 1 | 111,114,449 | 25,378,143 | 650,300 | 723,008 | src/libultra/io/vimgr.c |
| `__osPopThread` | 0.33% | 0.9% | 8,096,772 | 10 | 4 | 99,423 | 395,199 | src/libultra/os/exceptasm.c |
| `__osRestoreInt` | 0.32% | 0.9% | 9,116,026 | 9 | 7 | 0 | 291,675 | src/libultra/os/interrupt.c |
| `osStartThread` | 0.30% | 0.8% | 333,038 | 219 | 39 | 524,997 | 564,740 | src/libultra/os/startthread.c |
| `__osViSwapContext` | 0.25% | 0.7% | 35,376 | 1,702 | 129 | 423,483 | 496,344 | src/libultra/io/viswapcontext.c |
| `osGetTime` | 0.16% | 0.4% | 309,446 | 126 | 25 | 235,886 | 302,163 | src/libultra/os/gettime.c |
| `osWritebackDCache` | 0.15% | 0.4% | 132,057 | 279 | 44 | 0 | 158,815 | src/libultra/os/writebackdcache.c |
| `__osSiRawStartDma` | 0.14% | 0.4% | 141,404 | 244 | 28 | 84,081 | 476,388 | src/libultra/io/sirawdma.c |
| `osGetCount` | 0.13% | 0.4% | 8,513,784 | 4 | 3 | 0 | 135,854 | src/libultra/os/getcount.c |
| `osInvalDCache` | 0.13% | 0.4% | 141,630 | 225 | 67 | 0 | 455,235 | src/libultra/os/invaldcache.c |
| `osSpTaskLoad` | 0.11% | 0.3% | 59,369 | 457 | 55 | 83,494 | 413,082 | src/libultra/io/sptask.c |
| `osSetIntMask` | 0.10% | 0.3% | 208,330 | 118 | 38 | 46,374 | 192,480 | src/libultra/os/setintmask.c |
| `osContGetReadData` | 0.10% | 0.3% | 35,264 | 665 | 76 | 176,085 | 141,012 | src/libultra/io/contreaddata.c |
| `_VirtualToPhysicalTask` | 0.08% | 0.2% | 59,370 | 336 | 41 | 59,371 | 296,114 | src/libultra/io/sptask.c |
| `osVirtualToPhysical` | 0.08% | 0.2% | 595,584 | 33 | 9 | 0 | 296,289 | src/libultra/os/virtualtophysical.c |
| (76 more functions) | 1.04% | 2.9% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `osRecvMesg` (src/libultra/os/recvmesg.c): 20: 23%; 30: 23%, 1,374,985 data fills; 17: 16%, 911,492 data fills; 3: 13%, 402,673 data fills; 6: 9%, 548,853 data fills
- `__osTimerInterrupt` (src/libultra/os/timerintr.c): 34: 17%, 563,801 data fills; 49: 10%, 288,203 data fills; 29: 9%; 51: 8%; 55: 7%, 155,483 data fills
- `__osInsertTimer` (src/libultra/os/timerintr.c): 95: 26%; 100: 14%; 118: 14%; 101: 11%; 108: 10%

## Audio: 22.99%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| mixing (synthesis.c) | 8.97% | 39.0% | 13,393,565 | 10,046,416 |
| sequence player | 4.71% | 20.5% | 8,835,899 | 4,078,795 |
| note playback | 3.02% | 13.1% | 5,967,701 | 2,206,321 |
| note effects | 2.34% | 10.2% | 4,528,843 | 3,039,154 |
| loading (fonts, samples, sequences) | 1.73% | 7.5% | 3,206,665 | 3,347,610 |
| game side (sound effects, music commands) | 0.97% | 4.2% | 1,506,657 | 2,148,888 |
| thread and tasks | 0.82% | 3.6% | 1,436,336 | 2,011,271 |
| manager | 0.37% | 1.6% | 795,720 | 738,555 |
| heap and caches | 0.07% | 0.3% | 97,250 | 75,906 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `AudioSynth_ProcessNote` | 3.59% | 15.6% | 634,780 | 1,381 | 393 | 5,454,358 | 4,187,098 | src/audio/internal/synthesis.c |
| `AudioSeq_SequenceChannelProcessScript` | 2.94% | 12.8% | 1,198,085 | 599 | 225 | 5,402,195 | 1,562,679 | src/audio/internal/seqplayer.c |
| `AudioSynth_DoOneAudioUpdate` | 1.85% | 8.0% | 105,691 | 4,261 | 1,392 | 1,582,573 | 2,392,176 | src/audio/internal/synthesis.c |
| `Audio_ProcessNotes` | 1.61% | 7.0% | 105,687 | 3,708 | 807 | 4,095,168 | 800,569 | src/audio/internal/playback.c |
| `Audio_InitSampleState` | 1.33% | 5.8% | 635,039 | 510 | 140 | 1,724,171 | 1,209,720 | src/audio/internal/playback.c |
| `func_800DB03C` | 1.05% | 4.6% | 105,691 | 2,422 | 455 | 3,200,236 | 310,509 | src/audio/internal/synthesis.c |
| `AudioSeq_SequencePlayerProcessSequence` | 0.89% | 3.9% | 211,382 | 1,031 | 140 | 2,211,766 | 1,173,474 | src/audio/internal/seqplayer.c |
| `AudioSynth_ProcessEnvelope` | 0.76% | 3.3% | 634,801 | 291 | 111 | 837,415 | 631,216 | src/audio/internal/synthesis.c |
| `AudioThread_UpdateImpl` | 0.66% | 2.9% | 35,346 | 4,551 | 289 | 1,239,335 | 1,509,792 | src/audio/internal/thread.c |
| `Audio_SequenceChannelProcessSound` | 0.65% | 2.8% | 2,198,262 | 72 | 20 | 1,225,097 | 897,012 | src/audio/internal/effects.c |
| `AudioSynth_Update` | 0.65% | 2.8% | 35,229 | 4,474 | 701 | 605,121 | 1,175,047 | src/audio/internal/synthesis.c |
| `AudioSynth_InitNextRingBuf` | 0.60% | 2.6% | 211,641 | 696 | 158 | 937,232 | 698,155 | src/audio/internal/synthesis.c |
| `Audio_AdsrUpdate` | 0.49% | 2.1% | 635,009 | 189 | 44 | 1,259,239 | 453,985 | src/audio/internal/effects.c |
| `Audio_SequencePlayerProcessSound` | 0.47% | 2.0% | 211,384 | 541 | 203 | 367,518 | 975,231 | src/audio/internal/effects.c |
| `AudioLoad_DmaSampleData` | 0.41% | 1.8% | 184,815 | 545 | 66 | 770,324 | 884,080 | src/audio/internal/load.c |
| `Audio_NoteVibratoUpdate` | 0.38% | 1.7% | 635,001 | 147 | 15 | 1,370,074 | 130,812 | src/audio/internal/effects.c |
| `AudioSeq_HandleScriptFlowControl` | 0.37% | 1.6% | 3,150,233 | 29 | 17 | 289,753 | 345,797 | src/audio/internal/seqplayer.c |
| `AudioLoad_FindSampleDmaChecked` | 0.34% | 1.5% | 159,924 | 513 | 126 | 678,475 | 150,623 | src/audio/internal/load.c |
| `Audio_GetVibratoFreqScale` | 0.33% | 1.4% | 634,988 | 127 | 36 | 267,179 | 559,253 | src/audio/internal/effects.c |
| `AudioMgr_HandleRetrace.part.0` | 0.27% | 1.2% | 35,263 | 1,900 | 126 | 690,843 | 444,159 | src/code/audio_thread_manager.c |
| `AudioSeq_ProcessSequences` | 0.27% | 1.2% | 105,690 | 626 | 87 | 522,179 | 590,689 | src/audio/internal/seqplayer.c |
| `Fishmod_SampleStoreUse` | 0.25% | 1.1% | 181,232 | 332 | 53 | 273,018 | 688,847 | src/audio/internal/load.c |
| `Audio_ChooseActiveSfx` | 0.24% | 1.0% | 84,912 | 693 | 188 | 314,814 | 429,084 | src/audio/game/sfx.c |
| `AudioLoad_ProcessAsyncLoads` | 0.19% | 0.8% | 35,261 | 1,334 | 117 | 607,118 | 257,187 | src/audio/internal/load.c |
| `AudioSynth_LoadRingBuffer1` | 0.18% | 0.8% | 206,142 | 209 | 35 | 339,504 | 171,768 | src/audio/internal/synthesis.c |
| (190 more functions) | 2.22% | 9.6% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `AudioSynth_ProcessNote` (src/audio/internal/synthesis.c): 1073: 7%, 772,846 data fills; 974: 6%, 458,594 data fills; 1048: 5%, 423,693 data fills; 1205: 4%, 302,029 data fills; 1349: 4%, 228,644 data fills
- `AudioSeq_SequenceChannelProcessScript` (src/audio/internal/seqplayer.c): 1313: 17%, 1,434,245 data fills; 1297: 9%, 954,706 data fills; 1286: 8%, 289,377 data fills; 1811: 7%, 874,425 data fills; 1810: 6%, 741,022 data fills
- `AudioSynth_DoOneAudioUpdate` (src/audio/internal/synthesis.c): 929: 21%; 862: 17%, 546,136 data fills; 870: 4%; 940: 4%, 149,094 data fills; 930: 4%, 76,018 data fills
- `Audio_ProcessNotes` (src/audio/internal/playback.c): 211: 22%, 1,219,339 data fills; 208: 9%, 661,519 data fills; 250: 7%, 374,239 data fills; 251: 6%, 324,701 data fills; 328: 4%, 70,527 data fills
- `Audio_InitSampleState` (src/audio/internal/playback.c): 54: 11%; 51: 8%, 391,915 data fills; 48: 8%, 200,924 data fills; 120: 7%, 317,086 data fills; 127: 7%
- `func_800DB03C` (src/audio/internal/synthesis.c): 213: 43%, 1,914,194 data fills; 210: 29%, 1,122,724 data fills; 207: 10%; 214: 6%, 79,169 data fills; 208: 4%
- `AudioSeq_SequencePlayerProcessSequence` (src/audio/internal/seqplayer.c): 2169: 54%, 1,889,780 data fills; 2173: 9%; 1887: 6%; 1848: 6%, 135,467 data fills; 2170: 5%
- `AudioSynth_ProcessEnvelope` (src/audio/internal/synthesis.c): 1534: 26%, 284,414 data fills; 1531: 8%, 167,431 data fills; 1530: 7%, 72,627 data fills; 1490: 7%; 1485: 7%

## Maths (matrices, vectors, trig, random): 9.75%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| sys_matrix.c | 6.09% | 62.5% | 6,834,981 | 9,051,963 |
| z_lib.c | 0.73% | 7.5% | 872,238 | 1,188,845 |
| cosf.c | 0.69% | 7.1% | 215,249 | 1,664,439 |
| sinf.c | 0.58% | 5.9% | 303,656 | 272,547 |
| mtxutil.c | 0.43% | 4.4% | 636,113 | 427,899 |
| sys_math_atan.c | 0.27% | 2.7% | 137,530 | 530,717 |
| lookat.c | 0.24% | 2.4% | 69,827 | 683,576 |
| perspective.c | 0.22% | 2.3% | 267,359 | 407,316 |
| lookathil.c | 0.16% | 1.6% | 130,571 | 412,839 |
| ortho.c | 0.13% | 1.3% | 86,094 | 298,143 |
| z_olib.c | 0.10% | 1.0% | 26,787 | 331,511 |
| qrand.c | 0.08% | 0.8% | 19,709 | 77,263 |
| sqrtf.c | 0.03% | 0.3% | 0 | 38,309 |
| us2dex.c | 0.01% | 0.1% | 0 | 44,612 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Matrix_MtxFToMtx` | 1.85% | 19.0% | 558,423 | 809 | 151 | 2,648,324 | 2,520,638 | src/code/sys_matrix.c |
| `Matrix_TranslateRotateZYX` | 1.21% | 12.4% | 491,588 | 601 | 173 | 447,499 | 1,685,546 | src/code/sys_matrix.c |
| `Matrix_MultVec3f` | 0.74% | 7.5% | 638,540 | 281 | 54 | 790,521 | 811,166 | src/code/sys_matrix.c |
| `cosf` | 0.69% | 7.1% | 425,561 | 397 | 79 | 215,249 | 1,664,439 | src/libultra/gu/cosf.c |
| `sinf` | 0.58% | 5.9% | 654,371 | 215 | 68 | 303,656 | 272,547 | src/libultra/gu/sinf.c |
| `Matrix_RotateZ` | 0.48% | 4.9% | 122,470 | 952 | 92 | 143,185 | 1,680,924 | src/code/sys_matrix.c |
| `Matrix_RotateY` | 0.44% | 4.5% | 271,653 | 397 | 56 | 1,096,809 | 177,843 | src/code/sys_matrix.c |
| `Matrix_Push` | 0.40% | 4.1% | 464,181 | 211 | 41 | 891,516 | 164,197 | src/code/sys_matrix.c |
| `guMtxF2L` | 0.30% | 3.1% | 101,637 | 728 | 174 | 434,091 | 291,169 | src/libultra/gu/mtxutil.c |
| `Matrix_Scale` | 0.25% | 2.6% | 252,492 | 246 | 68 | 116,696 | 335,244 | src/code/sys_matrix.c |
| `Math_Atan2S` | 0.21% | 2.2% | 393,583 | 131 | 34 | 130,588 | 379,790 | src/code/sys_math_atan.c |
| `guLookAtF` | 0.20% | 2.0% | 35,508 | 1,369 | 181 | 42,531 | 562,050 | src/libultra/gu/lookat.c |
| `guPerspectiveF` | 0.19% | 2.0% | 35,506 | 1,325 | 156 | 208,189 | 335,068 | src/libultra/gu/perspective.c |
| `Math_SinS` | 0.19% | 1.9% | 242,070 | 189 | 41 | 176,012 | 301,495 | src/code/z_lib.c |
| `guLookAtHiliteF` | 0.13% | 1.4% | 6,828 | 4,741 | 495 | 86,296 | 368,730 | src/libultra/gu/lookathil.c |
| `Math_CosS` | 0.13% | 1.3% | 219,723 | 143 | 30 | 87,753 | 267,436 | src/code/z_lib.c |
| `guMtxIdentF` | 0.13% | 1.3% | 101,754 | 303 | 106 | 202,022 | 136,730 | src/libultra/gu/mtxutil.c |
| `Matrix_SetTranslateRotateYXZ` | 0.12% | 1.2% | 48,989 | 575 | 38 | 259,576 | 155,777 | src/code/sys_matrix.c |
| `Matrix_MtxToMtxF` | 0.11% | 1.1% | 22,786 | 1,171 | 162 | 121,237 | 240,258 | src/code/sys_matrix.c |
| `guOrthoF` | 0.11% | 1.1% | 23,799 | 1,086 | 140 | 75,069 | 225,278 | src/libultra/gu/ortho.c |
| `Math_Vec3f_Copy` | 0.08% | 0.9% | 191,431 | 108 | 7 | 264,029 | 45,336 | src/code/z_lib.c |
| `Matrix_RotateX` | 0.08% | 0.8% | 194,327 | 101 | 31 | 13,661 | 172,163 | src/code/sys_matrix.c |
| `Matrix_Get` | 0.08% | 0.8% | 34,180 | 550 | 67 | 93,393 | 167,284 | src/code/sys_matrix.c |
| `Matrix_Translate` | 0.07% | 0.8% | 209,727 | 85 | 19 | 17,165 | 202,781 | src/code/sys_matrix.c |
| `Math_ScaledStepToS` | 0.07% | 0.7% | 123,040 | 134 | 37 | 54,311 | 103,736 | src/code/z_lib.c |
| (53 more functions) | 0.91% | 9.4% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Matrix_MtxFToMtx` (src/code/sys_matrix.c): 602: 22%, 1,143,458 data fills; 600: 21%, 1,039,388 data fills; 606: 11%, 338,504 data fills; 604: 10%; 605: 9%
- `Matrix_TranslateRotateZYX` (src/code/sys_matrix.c): 416: 8%; 417: 8%; 410: 8%, 151,689 data fills; 418: 7%; 400: 6%, 189,634 data fills
- `Matrix_MultVec3f` (src/code/sys_matrix.c): 641: 48%, 644,589 data fills; 642: 25%; 643: 17%; 639: 6%; 644: 5%, 64,378 data fills
- `cosf` (src/libultra/gu/cosf.c): 49: 23%; 39: 14%; 51: 10%; 44: 9%; 45: 8%
- `sinf` (src/libultra/gu/sinf.c): 55: 22%; 48: 10%; 56: 9%; 38: 9%, 80,682 data fills; 21: 8%, 91,657 data fills
- `Matrix_RotateZ` (src/code/sys_matrix.c): 243: 17%, 77,119 data fills; 251: 11%; 305: 10%; 275: 7%; 270: 7%
- `Matrix_RotateY` (src/code/sys_matrix.c): 241: 30%, 334,920 data fills; 179: 19%, 346,644 data fills; 226: 8%, 101,726 data fills; 231: 8%, 101,781 data fills; 229: 8%, 101,683 data fills
- `Matrix_Push` (src/code/sys_matrix.c): 40: 86%, 766,367 data fills; 39: 11%, 122,997 data fills; 42: 2%; 41: 1%

## Profiler and test harness (not in a release): 9.13%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| n64prof.c | 4.66% | 51.0% | 10,047,193 | 4,462,815 |
| fishmod_debug_pcprof.inc.c | 2.85% | 31.2% | 5,372,005 | 2,107,856 |
| fishmod_debug.c | 1.55% | 17.0% | 3,205,304 | 1,283,163 |
| fishmod_debug_inputrec.inc.c | 0.07% | 0.8% | 61,627 | 231,893 |
| fishmod_debug_rdpbench.inc.c | 0.00% | 0.0% | 0 | 11,410 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `n64prof_sample` | 2.84% | 31.1% | 2,176,192 | 318 | 76 | 6,210,804 | 2,828,665 | n64prof.c |
| `PcProf_Hook` | 2.39% | 26.1% | 2,175,422 | 268 | 43 | 3,991,604 | 1,857,928 | src/code/fishmod_debug_pcprof.inc.c |
| `n64prof_count_into` | 1.61% | 17.6% | 2,175,425 | 181 | 28 | 3,185,096 | 1,594,364 | n64prof.c |
| `Fishmod_DebugPlayUpdate` | 1.52% | 16.6% | 11,393 | 32,542 | 8,992 | 3,179,218 | 1,164,008 | src/code/fishmod_debug.c |
| `PcProf_Update` | 0.26% | 2.9% | 11,396 | 5,577 | 540 | 625,538 | 194,574 | src/code/fishmod_debug_pcprof.inc.c |
| `n64prof_pack` | 0.20% | 2.2% | 340 | 141,352 | 31,312 | 631,182 | 5,604 | n64prof.c |
| `PcProf_Ranges` | 0.19% | 2.1% | 1,426 | 32,837 | 2,685 | 750,426 | 21,416 | src/code/fishmod_debug_pcprof.inc.c |
| `Fishmod_InputRecInput` | 0.06% | 0.7% | 12,172 | 1,292 | 114 | 56,101 | 213,103 | src/code/fishmod_debug_inputrec.inc.c |
| `Fishmod_AutoplayInput` | 0.02% | 0.2% | 12,171 | 386 | 19 | 24,351 | 60,150 | src/code/fishmod_debug.c |
| `n64prof_frame_end` | 0.01% | 0.1% | 11,392 | 237 | 8 | 11,915 | 34,174 | n64prof.c |
| `Fishmod_DebugPrintf` | 0.01% | 0.1% | 34,638 | 67 | 9 | 1,234 | 37,053 | src/code/fishmod_debug.c |
| `Sc64_Command` | 0.01% | 0.1% | 4,053 | 459 | 55 | 2,056 | 30,541 | src/code/fishmod_debug_pcprof.inc.c |
| `InputPlay_Peek` | 0.01% | 0.1% | 7,543 | 193 | 30 | 5,186 | 16,613 | src/code/fishmod_debug_inputrec.inc.c |
| `n64prof_init` | 0.00% | 0.0% | 1 | 729,360 | 65,632 | 8,196 | 8 | n64prof.c |
| `Fishmod_DebugDrawText` | 0.00% | 0.0% | 12,207 | 49 | 2 | 0 | 11,942 | src/code/fishmod_debug.c |
| `Fishmod_RdpBenchDraw` | 0.00% | 0.0% | 11,439 | 50 | 2 | 0 | 11,410 | src/code/fishmod_debug_rdpbench.inc.c |
| `Fishmod_DebugWantsFish` | 0.00% | 0.0% | 9,551 | 54 | 5 | 1 | 9,552 | src/code/fishmod_debug.c |
| `Sc64_UsbSend` | 0.00% | 0.0% | 674 | 521 | 37 | 2,373 | 3,375 | src/code/fishmod_debug_pcprof.inc.c |
| `InputRec_Line` | 0.00% | 0.0% | 608 | 165 | 14 | 1 | 1,829 | src/code/fishmod_debug_inputrec.inc.c |
| `Fishmod_OverlayLoaded` | 0.00% | 0.0% | 49 | 1,257 | 224 | 491 | 439 | src/code/fishmod_debug.c |
| `Fishmod_InputRecState` | 0.00% | 0.0% | 9 | 3,294 | 463 | 241 | 216 | src/code/fishmod_debug_inputrec.inc.c |
| `InputPlay_Read` | 0.00% | 0.0% | 28 | 386 | 39 | 78 | 109 | src/code/fishmod_debug_inputrec.inc.c |
| `Sc64_UsbText` | 0.00% | 0.0% | 3 | 591 | 64 | 8 | 22 | src/code/fishmod_debug_pcprof.inc.c |
| `InputPlay_Report` | 0.00% | 0.0% | 2 | 537 | 29 | 10 | 9 | src/code/fishmod_debug_inputrec.inc.c |
| `Fishmod_DebugColNodes` | 0.00% | 0.0% | 5 | 212 | 17 | 5 | 15 | src/code/fishmod_debug.c |
| (4 more functions) | 0.00% | 0.0% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `n64prof_sample` (n64prof.c): 99: 57%, 5,030,690 data fills; 86: 7%, 312,176 data fills; 107: 7%, 290,669 data fills; 95: 7%, 293,182 data fills; 97: 5%
- `PcProf_Hook` (src/code/fishmod_debug_pcprof.inc.c): 636: 93%, 3,989,138 data fills; 637: 5%; 650: 2%
- `n64prof_count_into` (n64prof.c): 128: 51%, 2,175,411 data fills; 132: 18%, 1,007,635 data fills; 129: 11%; 131: 8%; 133: 4%
- `Fishmod_DebugPlayUpdate` (src/code/fishmod_debug.c): 1235: 94%, 3,019,112 data fills; 1453: 2%, 37,133 data fills; 1458: 1%, 43,561 data fills; 1201: 1%, 22,784 data fills; 1466: 1%, 17,238 data fills
- `PcProf_Update` (src/code/fishmod_debug_pcprof.inc.c): 987: 65%, 467,326 data fills; 769: 4%, 23,057 data fills; 889: 3%, 11,369 data fills; 853: 3%, 22,535 data fills; 909: 3%, 18,978 data fills

## Animation: 4.85%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_skin_matrix.c | 2.56% | 52.8% | 1,695,563 | 3,876,523 |
| z_skelanime.c | 1.62% | 33.4% | 2,824,264 | 2,307,380 |
| z_skin.c | 0.58% | 12.0% | 1,053,894 | 73,348 |
| z_skin_awb.c | 0.09% | 1.8% | 210,700 | 29,845 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `SkinMatrix_MtxFMtxFMult` | 1.64% | 33.8% | 300,467 | 1,332 | 273 | 527,219 | 3,170,612 | src/code/z_skin_matrix.c |
| `SkelAnime_DrawFlexLimbLod` | 0.55% | 11.3% | 227,853 | 586 | 141 | 912,218 | 596,387 | src/code/z_skelanime.c |
| `SkelAnime_DrawLimb` | 0.37% | 7.6% | 156,868 | 577 | 120 | 607,160 | 507,905 | src/code/z_skelanime.c |
| `Skin_UpdateVertices` | 0.35% | 7.2% | 98,943 | 861 | 239 | 622,421 | 12,950 | src/code/z_skin.c |
| `SkinMatrix_Vec3fMtxFMultXYZ` | 0.35% | 7.1% | 543,799 | 155 | 52 | 203,043 | 48,218 | src/code/z_skin_matrix.c |
| `SkinMatrix_Vec3fMtxFMultXYZW` | 0.30% | 6.1% | 151,121 | 479 | 69 | 452,184 | 396,248 | src/code/z_skin_matrix.c |
| `SkelAnime_GetFrameData` | 0.22% | 4.6% | 20,994 | 2,564 | 569 | 458,706 | 188,112 | src/code/z_skelanime.c |
| `Skin_ApplyLimbModifications` | 0.20% | 4.1% | 781 | 61,627 | 17,781 | 344,991 | 27,173 | src/code/z_skin.c |
| `SkinMatrix_SetRotateZYX` | 0.11% | 2.2% | 80,344 | 326 | 52 | 238,310 | 70,070 | src/code/z_skin_matrix.c |
| `SkinMatrix_SetTranslate` | 0.10% | 2.1% | 227,881 | 109 | 19 | 206,507 | 75,026 | src/code/z_skin_matrix.c |
| `SkelAnime_DrawFlexLod` | 0.09% | 1.9% | 11,424 | 1,946 | 129 | 158,092 | 254,014 | src/code/z_skelanime.c |
| `SkelAnime_Draw` | 0.08% | 1.7% | 12,066 | 1,646 | 101 | 165,824 | 168,278 | src/code/z_skelanime.c |
| `func_800A698C` | 0.05% | 1.0% | 37,870 | 326 | 64 | 126,941 | 12,739 | src/code/z_skin_awb.c |
| `SkelAnime_DrawFlexLimbOpa` | 0.04% | 0.8% | 14,283 | 703 | 139 | 97,052 | 11,197 | src/code/z_skelanime.c |
| `AnimTaskQueue_AddLoadPlayerFrame` | 0.04% | 0.8% | 11,565 | 847 | 71 | 64,140 | 110,236 | src/code/z_skelanime.c |
| `Skin_ApplyAnimTransformations` | 0.04% | 0.7% | 781 | 11,011 | 1,686 | 78,694 | 16,998 | src/code/z_skin_awb.c |
| `SkelAnime_LoopFull` | 0.03% | 0.7% | 19,420 | 397 | 33 | 50,936 | 62,117 | src/code/z_skelanime.c |
| `SkelAnime_DrawFlexLimb` | 0.03% | 0.6% | 10,592 | 649 | 146 | 53,717 | 18,285 | src/code/z_skelanime.c |
| `SkelAnime_AnimateFrame` | 0.03% | 0.6% | 20,351 | 334 | 22 | 42,562 | 64,386 | src/code/z_skelanime.c |
| `AnimTaskQueue_Update` | 0.02% | 0.5% | 11,398 | 495 | 43 | 27,097 | 66,507 | src/code/z_skelanime.c |
| `SkinMatrix_MtxFToNewMtx` | 0.02% | 0.5% | 54,719 | 99 | 15 | 14,384 | 69,983 | src/code/z_skin_matrix.c |
| `Skin_DrawImpl` | 0.02% | 0.5% | 781 | 6,835 | 1,253 | 54,542 | 16,281 | src/code/z_skin.c |
| `LinkAnimation_AnimateFrame` | 0.02% | 0.4% | 10,470 | 436 | 26 | 37,008 | 43,010 | src/code/z_skelanime.c |
| `SkinMatrix_SetScale` | 0.02% | 0.4% | 44,957 | 101 | 19 | 35,622 | 7,062 | src/code/z_skin_matrix.c |
| `LinkAnimation_Once` | 0.02% | 0.3% | 6,627 | 553 | 42 | 20,043 | 39,045 | src/code/z_skelanime.c |
| (54 more functions) | 0.12% | 2.6% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `SkinMatrix_MtxFMtxFMult` (src/code/z_skin_matrix.c): 88: 8%; 106: 8%; 100: 8%; 94: 8%; 148: 7%
- `SkelAnime_DrawFlexLimbLod` (src/code/z_skelanime.c): 171: 10%, 125,489 data fills; 162: 10%, 214,705 data fills; 195: 9%, 67,087 data fills; 156: 8%, 111,527 data fills; 170: 7%, 68,059 data fills
- `SkelAnime_DrawLimb` (src/code/z_skelanime.c): 568: 15%, 99,028 data fills; 537: 13%, 133,800 data fills; 547: 12%; 545: 9%, 63,875 data fills; 526: 7%
- `Skin_UpdateVertices` (src/code/z_skin.c): 32: 24%, 317,925 data fills; 42: 14%, 119,732 data fills; 44: 8%, 48,838 data fills; 23: 6%; 30: 6%, 56,132 data fills
- `SkinMatrix_Vec3fMtxFMultXYZ` (src/code/z_skin_matrix.c): 55: 41%, 173,507 data fills; 60: 29%; 65: 28%; 66: 2%
- `SkinMatrix_Vec3fMtxFMultXYZW` (src/code/z_skin_matrix.c): 35: 45%, 353,282 data fills; 36: 21%, 90,703 data fills; 38: 16%; 37: 15%; 39: 3%
- `SkelAnime_GetFrameData` (src/code/z_skelanime.c): 503: 20%, 101,478 data fills; 499: 17%, 86,814 data fills; 501: 17%, 73,307 data fills; 491: 12%, 49,133 data fills; 500: 8%, 51,746 data fills

## Collision: 4.49%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| floors, walls, ceilings (z_bgcheck.c) | 3.91% | 87.1% | 6,169,292 | 7,137,446 |
| colliders (z_collision_check.c) | 0.37% | 8.3% | 624,545 | 683,302 |
| geometry tests (sys_math3d.c) | 0.21% | 4.6% | 76,261 | 196,019 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `BgCheck_RaycastDownImpl` | 2.40% | 53.6% | 81,595 | 7,192 | 1,130 | 3,733,237 | 4,278,022 | src/code/z_bgcheck.c |
| `DynaPoly_UpdateContext` | 0.25% | 5.7% | 11,393 | 5,451 | 336 | 807,926 | 134,235 | src/code/z_bgcheck.c |
| `BgCheck_SphVsStaticWall` | 0.11% | 2.4% | 9,578 | 2,762 | 449 | 126,844 | 236,753 | src/code/z_bgcheck.c |
| `DynaPoly_AddBgActorToLookup` | 0.11% | 2.4% | 3,728 | 7,044 | 1,070 | 306,564 | 78,853 | src/code/z_bgcheck.c |
| `BgCheck_CheckWallImpl` | 0.10% | 2.2% | 9,578 | 2,463 | 159 | 153,448 | 230,684 | src/code/z_bgcheck.c |
| `BgCheck_CheckLineAgainstSSList` | 0.09% | 2.1% | 2,464 | 9,303 | 3,260 | 120,496 | 53,897 | src/code/z_bgcheck.c |
| `Math3D_CylVsCylOverlapCenterDist` | 0.08% | 1.8% | 93,727 | 205 | 77 | 42,791 | 7,264 | src/code/sys_math3d.c |
| `CollisionCheck_OC` | 0.05% | 1.1% | 11,393 | 1,024 | 310 | 52,836 | 83,616 | src/code/z_collision_check.c |
| `CollisionCheck_ResetDamage` | 0.05% | 1.0% | 86,777 | 132 | 5 | 130,560 | 50,543 | src/code/z_collision_check.c |
| `BgCheck_EntityRaycastDown5` | 0.05% | 1.0% | 33,803 | 326 | 18 | 46,965 | 145,981 | src/code/z_bgcheck.c |
| `BgCheck_GetWaterSurface` | 0.04% | 1.0% | 37,346 | 287 | 21 | 60,451 | 122,112 | src/code/z_bgcheck.c |
| `CollisionCheck_SetOC` | 0.04% | 0.9% | 22,307 | 448 | 43 | 71,127 | 90,053 | src/code/z_collision_check.c |
| `BgCheck_CheckCeilingImpl` | 0.04% | 0.8% | 9,338 | 983 | 90 | 25,288 | 121,152 | src/code/z_bgcheck.c |
| `BgCheck_PolyIntersectsSubdivision` | 0.03% | 0.8% | 17,205 | 493 | 126 | 10,440 | 89,436 | src/code/z_bgcheck.c |
| `BgCheck_GetStaticLookupIndicesFromPos` | 0.03% | 0.8% | 27,557 | 303 | 43 | 35,775 | 74,446 | src/code/z_bgcheck.c |
| `BgCheck_AnyRaycastDown1` | 0.03% | 0.7% | 19,922 | 409 | 41 | 45,471 | 93,137 | src/code/z_bgcheck.c |
| `StaticLookup_AddPolyToSSList` | 0.03% | 0.7% | 15,001 | 536 | 207 | 75,067 | 779 | src/code/z_bgcheck.c |
| `BgCheck_SphVsDynaWall` | 0.03% | 0.7% | 9,578 | 826 | 58 | 55,989 | 80,554 | src/code/z_bgcheck.c |
| `CollisionPoly_LineVsPoly` | 0.03% | 0.7% | 20,813 | 365 | 79 | 30,406 | 43,724 | src/code/z_bgcheck.c |
| `BgCheck_PosInStaticBoundingBox` | 0.03% | 0.7% | 21,698 | 347 | 45 | 38,518 | 82,124 | src/code/z_bgcheck.c |
| `DynaPoly_UnsetAllInteractFlags` | 0.03% | 0.7% | 60,879 | 121 | 19 | 19,887 | 99,963 | src/code/z_bgcheck.c |
| `CollisionCheck_SetAC` | 0.03% | 0.6% | 20,860 | 339 | 43 | 36,804 | 86,171 | src/code/z_collision_check.c |
| `Collider_ResetQuadAT` | 0.03% | 0.6% | 34,183 | 202 | 14 | 73,034 | 22,785 | src/code/z_collision_check.c |
| `CollisionCheck_Damage` | 0.03% | 0.6% | 11,394 | 568 | 59 | 38,348 | 79,720 | src/code/z_collision_check.c |
| `CollisionCheck_OC_CylVsCyl` | 0.02% | 0.6% | 93,701 | 65 | 31 | 28,212 | 3,636 | src/code/z_collision_check.c |
| (127 more functions) | 0.71% | 15.8% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `BgCheck_RaycastDownImpl` (src/code/z_bgcheck.c): 3199: 93%, 3,444,329 data fills; 3198: 3%, 167,121 data fills; 3220: 2%, 75,584 data fills; 3202: 1%, 34,488 data fills; 1243: 0%
- `DynaPoly_UpdateContext` (src/code/z_bgcheck.c): 4647: 60%, 558,206 data fills; 4648: 19%, 157,724 data fills; 4638: 5%, 29,777 data fills; 4651: 2%, 9,348 data fills; 4711: 2%

## Scheduler, threads and system: 3.06%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| sched.c | 1.12% | 36.5% | 1,528,695 | 3,288,852 |
| padmgr.c | 0.61% | 20.0% | 792,678 | 1,529,145 |
| graph.c | 0.33% | 10.6% | 569,697 | 848,997 |
| sys_rumble.c | 0.20% | 6.4% | 245,960 | 378,478 |
| irqmgr.c | 0.19% | 6.1% | 411,643 | 352,093 |
| math64.c | 0.14% | 4.4% | 65,643 | 315,178 |
| pad.c | 0.13% | 4.2% | 39,234 | 476,466 |
| fault_n64.c | 0.13% | 4.2% | 179,296 | 316,864 |
| padsetup.c | 0.09% | 3.0% | 70,499 | 127,642 |
| viconfig.c | 0.04% | 1.2% | 71,119 | 106,004 |
| z_rumble.c | 0.03% | 1.0% | 16,696 | 119,683 |
| z_debug.c | 0.03% | 0.9% | 31,073 | 73,123 |
| main.c | 0.02% | 0.8% | 34,059 | 70,897 |
| z_debug_display.c | 0.02% | 0.5% | 22,420 | 56,961 |
| sprintf.c | 0.00% | 0.1% | 4,567 | 4,756 |
| stackcheck.c | 0.00% | 0.0% | 1,170 | 37 |
| cic6105.c | 0.00% | 0.0% | 10 | 15 |
| driverominit.c | 0.00% | 0.0% | 4 | 15 |
| boot_main.c | 0.00% | 0.0% | 6 | 10 |
| z_nulltask.c | 0.00% | 0.0% | 8 | 10 |
| code_n64dd_800AD410.c | 0.00% | 0.0% | 3 | 6 |
| idle.c | 0.00% | 0.0% | 3 | 6 |
| z_prenmi_buff.c | 0.00% | 0.0% | 4 | 2 |
| runtime.c | 0.00% | 0.0% | 1 | 6 |
| z_n64dd.c | 0.00% | 0.0% | 1 | 1 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Sched_HandleRSPDone` | 0.23% | 7.7% | 59,373 | 963 | 66 | 319,717 | 678,429 | src/code/sched.c |
| `PadMgr_UpdateInputs` | 0.22% | 7.2% | 35,264 | 1,517 | 259 | 286,937 | 466,203 | src/code/padmgr.c |
| `Sched_Schedule` | 0.20% | 6.5% | 142,156 | 340 | 28 | 259,974 | 610,220 | src/code/sched.c |
| `RumbleMgr_Update` | 0.20% | 6.4% | 35,006 | 1,364 | 418 | 245,960 | 378,453 | src/code/sys_rumble.c |
| `Sched_RunTask` | 0.16% | 5.4% | 59,583 | 671 | 47 | 279,331 | 473,178 | src/code/sched.c |
| `PadMgr_HandleRetrace` | 0.16% | 5.3% | 35,262 | 1,112 | 133 | 250,599 | 397,422 | src/code/padmgr.c |
| `IrqMgr_HandleRetrace` | 0.15% | 4.8% | 35,265 | 1,020 | 61 | 376,371 | 211,016 | src/code/irqmgr.c |
| `Sched_HandleNotification` | 0.14% | 4.6% | 82,609 | 417 | 48 | 197,153 | 405,097 | src/code/sched.c |
| `Graph_Update` | 0.14% | 4.5% | 12,171 | 2,751 | 162 | 245,049 | 327,850 | src/code/graph.c |
| `Sched_HandleRetrace` | 0.12% | 4.0% | 35,267 | 853 | 55 | 202,780 | 306,606 | src/code/sched.c |
| `Graph_TaskSet00` | 0.12% | 4.0% | 12,171 | 2,443 | 182 | 230,004 | 326,926 | src/code/graph.c |
| `PadUtils_UpdateRelXY` | 0.10% | 3.4% | 153,223 | 164 | 26 | 39,224 | 378,846 | src/libu64/pad.c |
| `PadMgr_UpdateRumble` | 0.10% | 3.3% | 35,262 | 698 | 113 | 42,411 | 364,626 | src/code/padmgr.c |
| `PadSetup_Init` | 0.09% | 3.0% | 1 | 22,701,178 | 4,589,766 | 70,499 | 127,642 | src/libu64/padsetup.c |
| `PadMgr_RequestPadData` | 0.08% | 2.5% | 12,193 | 1,558 | 156 | 167,180 | 109,539 | src/code/padmgr.c |
| `Sched_ThreadEntry` | 0.07% | 2.4% | 1 | 17,844,693 | 2,122,549 | 2,826 | 319,987 | src/code/sched.c |
| `Fault_RemoveClient` | 0.07% | 2.3% | 48,985 | 354 | 24 | 90,210 | 186,769 | src/code/fault_n64.c |
| `Math_FAtan2F` | 0.07% | 2.2% | 136,159 | 122 | 25 | 30,021 | 141,602 | src/libc64/math64.c |
| `Math_FAtanContFracF` | 0.07% | 2.2% | 135,053 | 121 | 30 | 35,622 | 173,576 | src/libc64/math64.c |
| `Sched_TaskComplete.part.0` | 0.06% | 2.1% | 47,348 | 335 | 32 | 145,621 | 113,975 | src/code/sched.c |
| `Sched_QueueTask` | 0.06% | 2.1% | 47,324 | 330 | 23 | 47,171 | 224,209 | src/code/sched.c |
| `Graph_InitTHGA` | 0.05% | 1.8% | 12,172 | 1,096 | 87 | 82,466 | 157,283 | src/code/graph.c |
| `Fault_AddClient` | 0.05% | 1.6% | 48,992 | 251 | 28 | 76,873 | 117,839 | src/code/fault_n64.c |
| `IrqMgr_ThreadEntry` | 0.04% | 1.3% | 1 | 9,786,044 | 564,691 | 35,258 | 141,054 | src/code/irqmgr.c |
| `ViConfig_UpdateBlack` | 0.04% | 1.2% | 35,265 | 263 | 13 | 71,116 | 105,993 | src/boot/viconfig.c |
| (52 more functions) | 0.25% | 8.2% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Sched_HandleRSPDone` (src/code/sched.c): 683: 21%, 70,820 data fills; 715: 15%, 35,734 data fills; 682: 14%, 63,542 data fills; 673: 9%; 685: 8%, 30,351 data fills
- `PadMgr_UpdateInputs` (src/code/padmgr.c): 302: 25%, 175,130 data fills; 292: 14%, 38,288 data fills; 346: 9%; 307: 9%; 300: 9%, 29,519 data fills

## Link: 2.51%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| ovl_player_actor | 1.40% | 55.9% | 1,430,489 | 4,395,211 |
| z_player_lib.c | 1.09% | 43.4% | 1,070,201 | 2,966,886 |
| z_player_call.c | 0.02% | 0.7% | 34,660 | 45,660 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Player_UpdateCommon` | 0.27% | 10.9% | 11,392 | 5,832 | 387 | 364,253 | 786,419 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_OverrideLimbDrawGameplayDefault` | 0.27% | 10.6% | 220,667 | 295 | 35 | 196,043 | 898,582 | src/code/z_player_lib.c |
| `Player_PostLimbDrawGameplay` | 0.24% | 9.5% | 239,238 | 243 | 58 | 179,705 | 583,881 | src/code/z_player_lib.c |
| `Player_OverrideLimbDrawGameplayCommon` | 0.21% | 8.2% | 239,241 | 211 | 35 | 189,347 | 572,886 | src/code/z_player_lib.c |
| `func_8008F87C.part.0` | 0.16% | 6.3% | 22,784 | 1,687 | 200 | 201,575 | 334,785 | src/code/z_player_lib.c |
| `Player_ProcessSceneCollision` | 0.14% | 5.7% | 9,326 | 3,748 | 275 | 157,083 | 416,834 | src/overlays/actors/ovl_player_actor/z_player.c |
| `func_80847298` | 0.13% | 5.2% | 11,393 | 2,820 | 242 | 29,978 | 486,075 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_DrawImpl` | 0.10% | 3.9% | 11,392 | 2,109 | 136 | 165,834 | 228,058 | src/code/z_player_lib.c |
| `Player_Draw` | 0.07% | 2.7% | 11,395 | 1,436 | 91 | 107,640 | 174,817 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_UpdateInterface.part.0` | 0.07% | 2.6% | 9,462 | 1,706 | 114 | 53,711 | 226,761 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_DrawGameplay` | 0.06% | 2.3% | 11,392 | 1,250 | 60 | 120,312 | 113,471 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_Update` | 0.06% | 2.2% | 11,393 | 1,207 | 60 | 97,255 | 133,851 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_InCsMode` | 0.05% | 2.1% | 34,938 | 374 | 23 | 87,771 | 122,978 | src/code/z_player_lib.c |
| `Player_Action_Idle` | 0.05% | 1.9% | 5,984 | 1,931 | 151 | 51,797 | 139,975 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_UpdateCamAndSeqModes.part.0` | 0.04% | 1.5% | 11,484 | 820 | 60 | 22,918 | 144,126 | src/overlays/actors/ovl_player_actor/z_player.c |
| `func_808382DC.part.0` | 0.03% | 1.2% | 6,903 | 1,101 | 72 | 40,891 | 90,594 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_ProcessControlStick` | 0.03% | 1.2% | 11,393 | 643 | 48 | 34,467 | 91,139 | src/overlays/actors/ovl_player_actor/z_player.c |
| `func_8083D6EC` | 0.03% | 1.2% | 11,392 | 628 | 45 | 28,904 | 91,137 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_ProcessItemButtons` | 0.03% | 1.1% | 6,893 | 1,004 | 161 | 23,468 | 89,793 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_HandleExitsAndVoids.part.0` | 0.03% | 1.1% | 9,597 | 720 | 64 | 21,721 | 105,437 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_TryActionHandlerList.part.0` | 0.02% | 0.9% | 6,893 | 843 | 140 | 37,909 | 55,136 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_UpdateZTargeting` | 0.02% | 0.9% | 11,392 | 469 | 42 | 2,159 | 95,974 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_UpdateItems` | 0.02% | 0.9% | 6,893 | 769 | 52 | 33,652 | 55,172 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_UpdateShapeYaw` | 0.02% | 0.7% | 11,393 | 396 | 23 | 23,910 | 46,767 | src/overlays/actors/ovl_player_actor/z_player.c |
| `func_80836AB8` | 0.02% | 0.7% | 3,281 | 1,294 | 126 | 3,643 | 72,182 | src/overlays/actors/ovl_player_actor/z_player.c |
| (104 more functions) | 0.35% | 14.1% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Player_UpdateCommon` (src/overlays/actors/ovl_player_actor/z_player.c): 12065: 3%, 22,784 data fills; 12032: 3%, 15,782 data fills; 11711: 3%; 11954: 3%, 9,294 data fills; 12121: 3%, 19,035 data fills
- `Player_OverrideLimbDrawGameplayDefault` (src/code/z_player_lib.c): 1360: 35%; 1357: 20%; 1389: 14%; 1412: 5%; 1370: 4%, 31,518 data fills
- `Player_PostLimbDrawGameplay` (src/code/z_player_lib.c): 1888: 16%; 1657: 15%; 1885: 11%, 38,961 data fills; 1881: 11%, 31,546 data fills; 1851: 8%, 17,503 data fills
- `Player_OverrideLimbDrawGameplayCommon` (src/code/z_player_lib.c): 1354: 27%; 1279: 21%, 68,087 data fills; 1320: 7%; 1346: 7%, 23,892 data fills; 1341: 5%, 14,716 data fills

## Actor framework (z_actor.c): 2.41%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_actor.c | 2.40% | 99.8% | 3,691,261 | 5,624,715 |
| z_cheap_proc.inc.c | 0.01% | 0.2% | 10,827 | 11,518 |
| z_actor_dlftbls.c | 0.00% | 0.0% | 5 | 23 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Actor_UpdateAll` | 0.53% | 22.1% | 11,393 | 11,412 | 1,202 | 940,187 | 1,006,530 | src/code/z_actor.c |
| `Actor_Draw.part.0` | 0.34% | 13.9% | 48,985 | 1,670 | 113 | 447,688 | 924,910 | src/code/z_actor.c |
| `Actor_DrawAll` | 0.29% | 12.1% | 11,392 | 6,229 | 439 | 564,283 | 626,711 | src/code/z_actor.c |
| `ActorShadow_DrawFeet` | 0.18% | 7.6% | 10,002 | 4,458 | 312 | 355,626 | 413,141 | src/code/z_actor.c |
| `func_8002E830` | 0.17% | 7.0% | 28,410 | 1,450 | 193 | 273,941 | 328,023 | src/code/z_actor.c |
| `ActorShadow_DrawFoot` | 0.11% | 4.7% | 19,715 | 1,401 | 105 | 156,737 | 285,693 | src/code/z_actor.c |
| `Actor_UpdateBgCheckInfo` | 0.10% | 4.0% | 11,291 | 2,075 | 141 | 154,859 | 244,091 | src/code/z_actor.c |
| `Actor_CullingVolumeTest` | 0.09% | 3.7% | 86,913 | 249 | 30 | 158,070 | 140,477 | src/code/z_actor.c |
| `Attention_Update` | 0.05% | 2.2% | 11,393 | 1,121 | 70 | 72,077 | 159,521 | src/code/z_actor.c |
| `func_8002E2AC` | 0.04% | 1.8% | 11,319 | 936 | 76 | 44,081 | 140,585 | src/code/z_actor.c |
| `Attention_FindActorInCategory` | 0.04% | 1.8% | 85,866 | 120 | 33 | 40,477 | 57,243 | src/code/z_actor.c |
| `Attention_FindActor` | 0.04% | 1.7% | 11,420 | 854 | 106 | 49,734 | 104,129 | src/code/z_actor.c |
| `Attention_SetNaviState` | 0.04% | 1.6% | 11,382 | 841 | 48 | 67,243 | 79,674 | src/code/z_actor.c |
| `Actor_UpdatePos` | 0.04% | 1.5% | 25,351 | 356 | 34 | 51,057 | 70,864 | src/code/z_actor.c |
| `Actor_Draw` | 0.04% | 1.5% | 48,983 | 178 | 21 | 49,652 | 94,696 | src/code/z_actor.c |
| `Actor_GetWorldPosShapeRot` | 0.03% | 1.4% | 36,602 | 223 | 27 | 15,094 | 116,941 | src/code/z_actor.c |
| `func_8002EBCC` | 0.03% | 1.2% | 11,392 | 643 | 40 | 36,978 | 79,748 | src/code/z_actor.c |
| `Attention_Draw` | 0.03% | 1.1% | 11,120 | 577 | 39 | 26,516 | 89,264 | src/code/z_actor.c |
| `Actor_DrawFaroresWindPointer` | 0.02% | 0.9% | 11,392 | 461 | 37 | 19,556 | 68,353 | src/code/z_actor.c |
| `Actor_ProjectPos` | 0.02% | 0.8% | 17,690 | 250 | 20 | 22,977 | 45,570 | src/code/z_actor.c |
| `Actor_UpdateVelocityXZGravity` | 0.02% | 0.7% | 14,220 | 297 | 30 | 19,130 | 50,498 | src/code/z_actor.c |
| `func_8002ED80` | 0.02% | 0.6% | 11,392 | 335 | 40 | 3,272 | 56,961 | src/code/z_actor.c |
| `Actor_SetFeetPos` | 0.01% | 0.6% | 193,674 | 18 | 7 | 61 | 36,970 | src/code/z_actor.c |
| `Player_GetHeight` | 0.01% | 0.6% | 18,303 | 188 | 12 | 15,757 | 43,508 | src/code/z_actor.c |
| `func_8002DCE4` | 0.01% | 0.6% | 11,393 | 298 | 17 | 16,283 | 33,903 | src/code/z_actor.c |
| (61 more functions) | 0.11% | 4.4% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Actor_UpdateAll` (src/code/z_actor.c): 2585: 8%, 84,924 data fills; 2544: 6%, 104,643 data fills; 2579: 6%, 84,920 data fills; 2545: 6%, 82,266 data fills; 2553: 5%, 75,671 data fills
- `Actor_Draw.part.0` (src/code/z_actor.c): 2737: 9%, 47,217 data fills; 2742: 9%, 63,952 data fills; 2789: 8%, 72,851 data fills; 2700: 8%; 2741: 8%, 50,563 data fills
- `Actor_DrawAll` (src/code/z_actor.c): 3087: 21%, 174,143 data fills; 3077: 11%, 79,962 data fills; 3066: 11%; 3090: 9%; 3045: 9%, 80,796 data fills

## Actors: 2.01%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| ovl_Object_Kankyo | 0.80% | 39.7% | 1,626,420 | 271,214 |
| ovl_En_Elf | 0.50% | 24.9% | 602,025 | 1,384,671 |
| ovl_En_Ganon_Mant | 0.46% | 22.7% | 664,614 | 412,497 |
| ovl_En_Kanban | 0.09% | 4.3% | 160,851 | 110,848 |
| ovl_En_Ko | 0.03% | 1.7% | 55,213 | 70,920 |
| ovl_En_Viewer | 0.02% | 1.2% | 31,213 | 67,153 |
| horses | 0.02% | 1.0% | 26,742 | 53,535 |
| ovl_En_Mag | 0.02% | 0.9% | 34,168 | 28,448 |
| ovl_Bg_Spot00_Hanebasi | 0.01% | 0.6% | 18,214 | 28,309 |
| ovl_En_Md | 0.01% | 0.6% | 16,357 | 30,870 |
| ovl_Obj_Mure2 | 0.01% | 0.5% | 24,953 | 9,665 |
| ovl_En_Holl | 0.01% | 0.5% | 14,856 | 22,189 |
| ovl_En_Kusa | 0.01% | 0.4% | 14,499 | 15,819 |
| ovl_Bg_Treemouth | 0.01% | 0.4% | 10,639 | 19,823 |
| ovl_En_Wood02 | 0.01% | 0.3% | 9,915 | 12,962 |
| ovl_En_River_Sound | 0.01% | 0.3% | 9,159 | 13,675 |
| ovl_Item_Shield | 0.00% | 0.1% | 4,407 | 6,141 |
| ovl_Obj_Hana | 0.00% | 0.0% | 315 | 530 |
| ovl_Obj_Comb | 0.00% | 0.0% | 21 | 46 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `ObjectKankyo_Fairies` | 0.42% | 20.9% | 1,845 | 55,543 | 12,155 | 635,667 | 186,411 | src/overlays/actors/ovl_Object_Kankyo/z_object_kankyo.c |
| `ObjectKankyo_DrawFairies.part.0` | 0.37% | 18.6% | 1,846 | 49,264 | 7,566 | 983,334 | 73,717 | src/overlays/actors/ovl_Object_Kankyo/z_object_kankyo.c |
| `EnGanonMant_UpdateStrand` | 0.36% | 17.9% | 6,744 | 12,988 | 2,278 | 407,893 | 379,069 | src/overlays/actors/ovl_En_Ganon_Mant/z_en_ganon_mant.c |
| `EnElf_Draw` | 0.11% | 5.6% | 16,024 | 1,710 | 120 | 167,684 | 293,666 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `EnGanonMant_UpdateVertices` | 0.08% | 3.8% | 562 | 33,449 | 5,749 | 216,425 | 9,018 | src/overlays/actors/ovl_En_Ganon_Mant/z_en_ganon_mant.c |
| `EnElf_OverrideLimbDraw` | 0.07% | 3.5% | 168,950 | 101 | 20 | 68,119 | 147,221 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `func_80A053F0` | 0.06% | 2.9% | 11,119 | 1,298 | 76 | 94,900 | 154,899 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `func_80A03CF8` | 0.06% | 2.7% | 11,120 | 1,209 | 92 | 54,356 | 176,834 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `EnKanban_Draw` | 0.04% | 2.2% | 2,144 | 5,078 | 1,431 | 92,542 | 25,210 | src/overlays/actors/ovl_En_Kanban/z_en_kanban.c |
| `func_80A02C98` | 0.04% | 1.8% | 11,083 | 792 | 86 | 27,067 | 123,312 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `EnKanban_Update` | 0.04% | 1.8% | 8,568 | 1,020 | 105 | 57,404 | 75,078 | src/overlays/actors/ovl_En_Kanban/z_en_kanban.c |
| `func_80A0461C` | 0.03% | 1.5% | 11,120 | 654 | 37 | 37,972 | 86,365 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `EnElf_UpdateLights` | 0.03% | 1.4% | 11,120 | 638 | 70 | 5,134 | 111,799 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `func_80A04414` | 0.03% | 1.3% | 11,119 | 581 | 31 | 28,087 | 89,154 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `func_80A02A20` | 0.03% | 1.3% | 16,033 | 398 | 34 | 42,382 | 59,038 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `EnGanonMant_Draw` | 0.02% | 0.8% | 562 | 6,969 | 731 | 33,972 | 16,832 | src/overlays/actors/ovl_En_Ganon_Mant/z_en_ganon_mant.c |
| `func_80A0353C` | 0.01% | 0.7% | 4,904 | 720 | 45 | 22,910 | 34,378 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `EnElf_GetCuePos` | 0.01% | 0.6% | 3,651 | 758 | 61 | 12,991 | 29,222 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `EnElf_SpawnSparkles` | 0.01% | 0.5% | 2,655 | 953 | 82 | 11,543 | 29,199 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `EnKo_OverrideLimbDraw` | 0.01% | 0.5% | 9,661 | 260 | 43 | 18,161 | 12,791 | src/overlays/actors/ovl_En_Ko/z_en_ko.c |
| `EnKo_Update` | 0.01% | 0.4% | 1,210 | 1,765 | 139 | 16,873 | 19,880 | src/overlays/actors/ovl_En_Ko/z_en_ko.c |
| `EnHoll_HorizontalInvisible` | 0.01% | 0.4% | 1,843 | 1,023 | 49 | 12,997 | 18,484 | src/overlays/actors/ovl_En_Holl/z_en_holl.c |
| `EnViewer_UpdateImpl` | 0.01% | 0.3% | 2,800 | 607 | 62 | 7,529 | 22,410 | src/overlays/actors/ovl_En_Viewer/z_en_viewer.c |
| `EnMag_DrawInner` | 0.01% | 0.3% | 272 | 6,047 | 637 | 9,340 | 16,563 | src/overlays/actors/ovl_En_Mag/z_en_mag.c |
| `EnKanban_Message` | 0.01% | 0.3% | 8,569 | 175 | 28 | 10,711 | 10,444 | src/overlays/actors/ovl_En_Kanban/z_en_kanban.c |
| (149 more functions) | 0.16% | 7.8% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `ObjectKankyo_Fairies` (src/overlays/actors/ovl_Object_Kankyo/z_object_kankyo.c): 272: 14%, 150,955 data fills; 307: 8%, 116,135 data fills; 262: 6%; 351: 5%, 59,052 data fills; 299: 4%, 59,241 data fills
- `ObjectKankyo_DrawFairies.part.0` (src/overlays/actors/ovl_Object_Kankyo/z_object_kankyo.c): 533: 29%, 353,433 data fills; 538: 13%, 117,783 data fills; 597: 9%, 92,957 data fills; 596: 7%, 84,909 data fills; 580: 5%, 63,261 data fills
- `EnGanonMant_UpdateStrand` (src/overlays/actors/ovl_En_Ganon_Mant/z_en_ganon_mant.c): 301: 8%; 284: 8%, 31,483 data fills; 323: 6%, 25,835 data fills; 242: 6%; 261: 6%

## Sky, weather and lights: 1.63%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_lights.c | 1.08% | 66.1% | 1,521,305 | 2,019,402 |
| z_kankyo.c | 0.51% | 31.2% | 480,298 | 1,451,590 |
| z_vr_box_draw.c | 0.04% | 2.4% | 45,847 | 119,835 |
| z_vr_box.c | 0.00% | 0.2% | 10,386 | 825 |
| z_env_flags.c | 0.00% | 0.0% | 262 | 797 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Lights_BindPoint` | 0.30% | 18.3% | 516,142 | 141 | 35 | 176,059 | 425,695 | src/code/z_lights.c |
| `Lights_BindAll` | 0.28% | 17.2% | 71,881 | 951 | 132 | 581,017 | 416,082 | src/code/z_lights.c |
| `Environment_Update` | 0.27% | 16.5% | 11,392 | 5,741 | 825 | 201,019 | 806,454 | src/code/z_kankyo.c |
| `Lights_Draw` | 0.17% | 10.6% | 71,793 | 586 | 50 | 218,232 | 484,383 | src/code/z_lights.c |
| `Lights_BindDirectional` | 0.11% | 6.5% | 143,535 | 180 | 27 | 165,197 | 206,092 | src/code/z_lights.c |
| `Lights_GlowCheck` | 0.10% | 6.4% | 11,429 | 2,233 | 161 | 221,864 | 184,197 | src/code/z_lights.c |
| `LightContext_NewLights` | 0.06% | 3.5% | 71,821 | 196 | 14 | 91,928 | 143,538 | src/code/z_lights.c |
| `Lights_DrawGlow` | 0.04% | 2.5% | 11,392 | 855 | 94 | 42,915 | 113,926 | src/code/z_lights.c |
| `Environment_DrawLightning` | 0.04% | 2.4% | 11,393 | 822 | 78 | 58,199 | 91,878 | src/code/z_kankyo.c |
| `Skybox_Draw` | 0.04% | 2.3% | 2,662 | 3,436 | 280 | 45,847 | 108,424 | src/code/z_vr_box_draw.c |
| `Environment_DrawRain` | 0.03% | 1.9% | 562 | 13,277 | 3,294 | 37,317 | 28,907 | src/code/z_kankyo.c |
| `Environment_DrawLensFlare` | 0.03% | 1.9% | 2,118 | 3,498 | 244 | 43,278 | 63,586 | src/code/z_kankyo.c |
| `Environment_DrawSunAndMoon` | 0.02% | 1.4% | 2,118 | 2,588 | 238 | 14,096 | 75,660 | src/code/z_kankyo.c |
| `Environment_FillScreen` | 0.02% | 1.3% | 14,750 | 338 | 27 | 2,882 | 86,314 | src/code/z_kankyo.c |
| `Environment_DrawSkyboxFilters` | 0.02% | 1.2% | 11,421 | 416 | 31 | 11,988 | 77,259 | src/code/z_kankyo.c |
| `Environment_GraphCallback` | 0.01% | 0.9% | 11,411 | 309 | 17 | 34,174 | 34,174 | src/code/z_kankyo.c |
| `Environment_UpdateLightningStrike` | 0.01% | 0.8% | 11,392 | 282 | 23 | 11,961 | 48,284 | src/code/z_kankyo.c |
| `Lights_PointNoGlowSetInfo` | 0.01% | 0.6% | 11,137 | 221 | 13 | 11,168 | 32,953 | src/code/z_lights.c |
| `Environment_ChangeLightSetting` | 0.01% | 0.6% | 9,326 | 255 | 9 | 14,140 | 27,990 | src/code/z_kankyo.c |
| `Environment_UpdateSkybox` | 0.01% | 0.6% | 1,384 | 1,589 | 160 | 8,670 | 30,047 | src/code/z_kankyo.c |
| `Environment_DrawCustomLensFlare` | 0.01% | 0.5% | 11,392 | 179 | 6 | 11,392 | 22,860 | src/code/z_kankyo.c |
| `Lights_PointGlowSetInfo` | 0.01% | 0.4% | 12,797 | 128 | 14 | 12,768 | 12,384 | src/code/z_lights.c |
| `Environment_LerpWeight` | 0.01% | 0.4% | 5,487 | 276 | 23 | 4,462 | 17,536 | src/code/z_kankyo.c |
| `Environment_PlayTimeBasedSequence` | 0.01% | 0.3% | 11,392 | 122 | 8 | 11,305 | 11,394 | src/code/z_kankyo.c |
| `Environment_DrawSunLensFlare` | 0.00% | 0.3% | 2,118 | 556 | 50 | 4,648 | 15,294 | src/code/z_kankyo.c |
| (20 more functions) | 0.01% | 0.7% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Lights_BindPoint` (src/code/z_lights.c): 127: 16%, 99,758 data fills; 128: 14%; 125: 12%; 126: 10%; 124: 8%
- `Lights_BindAll` (src/code/z_lights.c): 192: 43%, 246,764 data fills; 190: 23%, 232,033 data fills; 187: 17%; 195: 7%; 193: 5%, 33,631 data fills
- `Environment_Update` (src/code/z_kankyo.c): 1232: 4%; 1301: 3%, 11,393 data fills; 981: 3%, 9,669 data fills; 955: 3%, 11,465 data fills; 932: 3%

## HUD and map: 1.47%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_parameter.c | 1.10% | 75.0% | 1,550,456 | 2,808,017 |
| z_lifemeter.c | 0.30% | 20.3% | 363,168 | 822,602 |
| z_map_exp.c | 0.07% | 4.7% | 59,693 | 230,209 |
| z_construct.c | 0.00% | 0.0% | 496 | 455 |
| speed_meter.c | 0.00% | 0.0% | 9 | 24 |
| z_map_mark.c | 0.00% | 0.0% | 12 | 12 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Interface_Draw` | 0.40% | 27.1% | 11,120 | 8,723 | 631 | 601,652 | 999,396 | src/code/z_parameter.c |
| `Interface_DrawItemButtons` | 0.28% | 19.3% | 11,147 | 6,218 | 612 | 429,268 | 669,889 | src/code/z_parameter.c |
| `Health_DrawMeter` | 0.23% | 15.7% | 11,120 | 5,071 | 499 | 270,759 | 613,949 | src/code/z_lifemeter.c |
| `Interface_InitVertices` | 0.15% | 9.9% | 11,119 | 3,188 | 374 | 173,461 | 389,177 | src/code/z_parameter.c |
| `Interface_Update` | 0.10% | 6.7% | 11,392 | 2,105 | 126 | 134,768 | 294,777 | src/code/z_parameter.c |
| `Interface_DrawActionButton` | 0.06% | 4.2% | 11,120 | 1,355 | 114 | 68,821 | 166,816 | src/code/z_parameter.c |
| `Minimap_Draw` | 0.06% | 3.8% | 11,121 | 1,226 | 104 | 59,585 | 168,262 | src/code/z_map_exp.c |
| `Health_UpdateMeter` | 0.05% | 3.7% | 11,392 | 1,163 | 99 | 80,140 | 160,630 | src/code/z_lifemeter.c |
| `func_8008A8B8` | 0.04% | 3.0% | 22,293 | 490 | 58 | 68,606 | 78,975 | src/code/z_parameter.c |
| `Interface_DrawActionLabel` | 0.03% | 2.0% | 11,146 | 648 | 54 | 44,499 | 67,007 | src/code/z_parameter.c |
| `Magic_DrawMeter` | 0.01% | 0.9% | 11,140 | 295 | 27 | 1,029 | 55,689 | src/code/z_parameter.c |
| `Health_UpdateBeatingHeart` | 0.01% | 0.9% | 11,392 | 277 | 20 | 12,131 | 47,778 | src/code/z_lifemeter.c |
| `Map_Update` | 0.01% | 0.8% | 11,392 | 258 | 16 | 1 | 57,017 | src/code/z_map_exp.c |
| `Magic_Update` | 0.01% | 0.7% | 7,104 | 334 | 28 | 8,171 | 35,545 | src/code/z_parameter.c |
| `Interface_SetDoAction` | 0.01% | 0.6% | 9,471 | 222 | 12 | 18,714 | 19,186 | src/code/z_parameter.c |
| `Interface_SetNaviCall` | 0.01% | 0.4% | 9,463 | 167 | 17 | 220 | 28,387 | src/code/z_parameter.c |
| `Minimap_DrawCompassIcons` | 0.00% | 0.1% | 2,407 | 105 | 8 | 1 | 4,814 | src/code/z_map_exp.c |
| `Interface_LoadActionLabel` | 0.00% | 0.0% | 242 | 700 | 52 | 922 | 1,935 | src/code/z_parameter.c |
| `Interface_UpdateHudAlphas` | 0.00% | 0.0% | 77 | 897 | 59 | 239 | 1,017 | src/code/z_parameter.c |
| `Regs_InitDataImpl` | 0.00% | 0.0% | 5 | 6,454 | 477 | 298 | 301 | src/code/z_construct.c |
| `Health_IsCritical` | 0.00% | 0.0% | 52 | 369 | 12 | 103 | 215 | src/code/z_lifemeter.c |
| `Interface_Init` | 0.00% | 0.0% | 5 | 3,095 | 153 | 153 | 110 | src/code/z_construct.c |
| `Map_Init` | 0.00% | 0.0% | 5 | 1,479 | 72 | 69 | 51 | src/code/z_map_exp.c |
| `Interface_RaiseButtonAlphas` | 0.00% | 0.0% | 16 | 454 | 44 | 16 | 112 | src/code/z_parameter.c |
| `Interface_SetSceneRestrictions` | 0.00% | 0.0% | 5 | 1,038 | 244 | 37 | 45 | src/code/z_parameter.c |
| (13 more functions) | 0.00% | 0.0% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Interface_Draw` (src/code/z_parameter.c): 3347: 14%, 111,363 data fills; 3253: 8%, 55,604 data fills; 3314: 4%, 44,153 data fills; 3455: 3%, 11,645 data fills; 3445: 3%, 13,321 data fills
- `Interface_DrawItemButtons` (src/code/z_parameter.c): 2997: 28%, 171,687 data fills; 2854: 15%, 89,237 data fills; 2868: 7%, 18,383 data fills; 2875: 7%, 18,419 data fills; 2861: 6%, 18,698 data fills
- `Health_DrawMeter` (src/code/z_lifemeter.c): 465: 16%, 44,498 data fills; 436: 12%, 40,260 data fills; 347: 8%, 40,232 data fills; 489: 4%; 359: 4%

## Fishing mod: 1.36%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| fishmod.c | 0.50% | 37.0% | 385,223 | 1,134,234 |
| fishmod_trig.h | 0.48% | 35.4% | 154,129 | 476,340 |
| fishmod_tri.h | 0.24% | 17.4% | 20,470 | 438,380 |
| fishmod_items.c | 0.08% | 5.6% | 97,238 | 211,898 |
| ovl_En_Fishmod | 0.04% | 3.2% | 74,934 | 103,588 |
| fish manager | 0.01% | 0.9% | 20,560 | 29,717 |
| props (scatter) | 0.01% | 0.5% | 12,780 | 16,628 |
| fish rigs | 0.00% | 0.0% | 17 | 9 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Fishmod_SinCosBinang` | 0.48% | 35.4% | 810,290 | 145 | 43 | 154,129 | 476,340 | fishmod_trig.h |
| `Fishmod_ClaimDisplayLists` | 0.28% | 20.5% | 60,374 | 1,126 | 152 | 44,185 | 504,283 | src/code/fishmod.c |
| `Fishmod_PointDistSqToSeg2D` | 0.22% | 16.1% | 250,778 | 213 | 49 | 13,502 | 417,138 | fishmod_tri.h |
| `Fishmod_TerrainRA` | 0.05% | 4.0% | 11,393 | 1,156 | 74 | 84,394 | 151,356 | src/code/fishmod.c |
| `Fishmod_PlayUpdate` | 0.05% | 3.9% | 11,423 | 1,133 | 60 | 95,820 | 132,347 | src/code/fishmod.c |
| `Fishmod_TimeUpdate` | 0.05% | 3.8% | 11,393 | 1,100 | 57 | 100,775 | 148,969 | src/code/fishmod.c |
| `Fishmod_EquipButtons` | 0.04% | 2.9% | 11,421 | 836 | 78 | 75,590 | 84,593 | src/code/fishmod_items.c |
| `Fishmod_DpadDraw` | 0.03% | 2.1% | 11,119 | 612 | 57 | 21,648 | 88,955 | src/code/fishmod_items.c |
| `Fishmod_TriChkPointParaY.constprop.0` | 0.02% | 1.3% | 27,145 | 153 | 69 | 6,968 | 21,242 | fishmod_tri.h |
| `Fishmod_Relight` | 0.02% | 1.2% | 11,392 | 360 | 36 | 11,468 | 56,962 | src/code/fishmod.c |
| `EnFishmod_Draw` | 0.02% | 1.2% | 3,690 | 1,065 | 66 | 25,561 | 40,774 | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `Fishmod_AdvanceTime` | 0.02% | 1.2% | 9,239 | 415 | 30 | 21,098 | 47,806 | src/code/fishmod.c |
| `EnFishmod_ManagerDraw` | 0.01% | 0.9% | 1,845 | 1,661 | 107 | 20,547 | 29,711 | src/overlays/actors/ovl_En_Fishmod/fishmod_manager.c |
| `__udivdi3` | 0.01% | 0.9% | 919,007 | 3 | 2 | 0 | 23,638 | src/code/fishmod.c |
| `Fishmod_SceneHasFish` | 0.01% | 0.7% | 11,397 | 209 | 17 | 11,417 | 34,186 | src/code/fishmod.c |
| `Fishmod_HazeTint` | 0.01% | 0.7% | 11,392 | 196 | 10 | 11,392 | 34,176 | src/code/fishmod.c |
| `Fishmod_SlotItem` | 0.01% | 0.6% | 44,476 | 44 | 8 | 0 | 33,531 | src/code/fishmod_items.c |
| `EnFishmod_Update` | 0.01% | 0.6% | 1,845 | 1,053 | 65 | 13,776 | 22,142 | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_RodUpdate` | 0.01% | 0.6% | 1,845 | 1,049 | 65 | 11,323 | 20,304 | src/overlays/actors/ovl_En_Fishmod/fishmod_rod.c |
| `EnFishmod_RodDraw` | 0.01% | 0.5% | 1,845 | 980 | 42 | 15,034 | 12,949 | src/overlays/actors/ovl_En_Fishmod/fishmod_rod.c |
| `EnFishmod_ScatterDraw` | 0.01% | 0.5% | 1,845 | 933 | 50 | 12,758 | 16,608 | src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c |
| `Rod_PublishInfo` | 0.00% | 0.2% | 1,845 | 424 | 19 | 7,386 | 5,537 | src/overlays/actors/ovl_En_Fishmod/fishmod_rod.c |
| `Fishmod_TerrainModesWalk` | 0.00% | 0.1% | 0 | 0 | 0 | 4,493 | 176 | src/code/fishmod.c |
| `Fishmod_ClockDraw` | 0.00% | 0.1% | 2,416 | 100 | 4 | 0 | 4,814 | src/code/fishmod_items.c |
| `EnFishmod_ParticlesDraw` | 0.00% | 0.1% | 1,845 | 95 | 6 | 1,845 | 1,857 | src/overlays/actors/ovl_En_Fishmod/fishmod_particles.c |
| (27 more functions) | 0.00% | 0.0% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Fishmod_SinCosBinang` (fishmod_trig.h): 51: 29%; 48: 25%; 49: 22%; 50: 8%; 59: 7%, 45,074 data fills
- `Fishmod_ClaimDisplayLists` (src/code/fishmod.c): 1591: 42%; 1592: 32%; 1593: 26%; 1594: 0%
- `Fishmod_PointDistSqToSeg2D` (fishmod_tri.h): 32: 28%; 33: 15%; 38: 12%; 24: 12%; 22: 7%

## Camera and view: 1.27%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_view.c | 0.71% | 55.8% | 1,069,343 | 1,438,123 |
| z_camera.c | 0.51% | 40.0% | 562,210 | 1,486,544 |
| z_quake.c | 0.03% | 2.2% | 26,496 | 91,151 |
| shrink_window.c | 0.02% | 1.9% | 21,311 | 92,752 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Camera_Update` | 0.24% | 19.1% | 15,879 | 3,717 | 248 | 368,306 | 587,738 | src/code/z_camera.c |
| `View_ApplyPerspectiveToOverlay` | 0.21% | 16.3% | 22,239 | 2,275 | 285 | 327,091 | 347,982 | src/code/z_view.c |
| `View_ApplyOrthoToOverlay` | 0.19% | 15.2% | 23,512 | 2,006 | 199 | 285,330 | 416,072 | src/code/z_view.c |
| `View_ApplyPerspective` | 0.16% | 12.7% | 13,266 | 2,963 | 256 | 265,794 | 306,865 | src/code/z_view.c |
| `Camera_CalcUpFromPitchYawRoll` | 0.06% | 4.7% | 11,392 | 1,270 | 144 | 5,512 | 216,448 | src/code/z_camera.c |
| `Camera_Fixed3` | 0.05% | 4.2% | 7,435 | 1,750 | 117 | 78,166 | 133,825 | src/code/z_camera.c |
| `View_LookAt` | 0.05% | 3.7% | 35,506 | 326 | 31 | 70,839 | 109,493 | src/code/z_view.c |
| `View_ApplyLetterbox` | 0.05% | 3.7% | 13,269 | 854 | 81 | 42,220 | 139,835 | src/code/z_view.c |
| `Quake_Update` | 0.03% | 2.2% | 11,392 | 592 | 47 | 26,451 | 91,136 | src/code/z_quake.c |
| `Camera_RequestModeImpl` | 0.02% | 1.6% | 11,410 | 445 | 27 | 18,522 | 66,882 | src/code/z_camera.c |
| `Camera_Demo1` | 0.02% | 1.5% | 3,958 | 1,184 | 76 | 38,064 | 45,012 | src/code/z_camera.c |
| `Camera_UpdateWater` | 0.02% | 1.5% | 6,912 | 653 | 57 | 578 | 82,946 | src/code/z_camera.c |
| `Letterbox_Update` | 0.02% | 1.3% | 11,393 | 362 | 16 | 11,392 | 68,352 | src/code/shrink_window.c |
| `View_SetViewport` | 0.02% | 1.3% | 46,038 | 88 | 12 | 29,824 | 23,512 | src/code/z_view.c |
| `Camera_UpdateDistortion` | 0.02% | 1.3% | 11,392 | 347 | 33 | 55 | 68,355 | src/code/z_camera.c |
| `Camera_GetBgCamIndex` | 0.01% | 1.1% | 11,372 | 301 | 31 | 2,464 | 56,856 | src/code/z_camera.c |
| `Camera_GetWaterSurface` | 0.01% | 1.0% | 6,911 | 457 | 32 | 14,659 | 34,550 | src/code/z_camera.c |
| `Camera_UpdateInterface` | 0.01% | 0.9% | 6,904 | 382 | 39 | 13 | 48,346 | src/code/z_camera.c |
| `Camera_RequestBgCam` | 0.01% | 0.9% | 11,129 | 237 | 19 | 2,444 | 44,492 | src/code/z_camera.c |
| `View_StepDistortion.part.0` | 0.01% | 0.8% | 13,287 | 177 | 9 | 12,725 | 25,405 | src/code/z_view.c |
| `View_SetScale` | 0.01% | 0.7% | 11,392 | 197 | 5 | 22,903 | 11,394 | src/code/z_view.c |
| `View_SetPerspective` | 0.01% | 0.7% | 33,862 | 65 | 7 | 9,060 | 22,871 | src/code/z_view.c |
| `Camera_GetWaterBoxBgCamIndex` | 0.01% | 0.7% | 6,910 | 319 | 34 | 2,353 | 34,550 | src/code/z_camera.c |
| `Camera_Finish` | 0.01% | 0.6% | 11,392 | 157 | 4 | 10,867 | 22,784 | src/code/z_camera.c |
| `Camera_GetQuakeOffset` | 0.01% | 0.5% | 8,727 | 173 | 8 | 7,007 | 17,424 | src/code/z_camera.c |
| (24 more functions) | 0.02% | 1.9% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Camera_Update` (src/code/z_camera.c): 8074: 6%, 34,237 data fills; 8085: 6%, 29,491 data fills; 8155: 5%, 27,261 data fills; 8070: 5%, 25,067 data fills; 8087: 4%, 15,132 data fills
- `View_ApplyPerspectiveToOverlay` (src/code/z_view.c): 562: 11%, 53,303 data fills; 527: 11%; 541: 11%, 42,052 data fills; 539: 9%, 37,671 data fills; 523: 8%, 26,990 data fills

## C library (memory copies, printf, 64-bit maths): 1.14%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| memset.c | 0.37% | 32.2% | 848,547 | 141,104 |
| ll.c | 0.36% | 31.5% | 5,993 | 100,859 |
| bzero.c | 0.20% | 17.5% | 552,972 | 172 |
| bcopy.c | 0.16% | 14.3% | 517,808 | 266,358 |
| memmove.c | 0.02% | 1.3% | 51,421 | 131 |
| xlitob.c | 0.01% | 1.3% | 9,316 | 44,004 |
| xprintf.c | 0.01% | 1.2% | 9,642 | 35,964 |
| string.c | 0.01% | 0.5% | 4,555 | 5,225 |
| ldiv.c | 0.00% | 0.2% | 2,032 | 6,260 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `memset` | 0.37% | 32.2% | 110,365 | 810 | 227 | 848,547 | 141,104 | src/libc/memset.c |
| `__ull_div` | 0.35% | 30.7% | 924,502 | 92 | 14 | 2,515 | 80,515 | src/libultra/libc/ll.c |
| `bzero` | 0.20% | 17.5% | 39 | 1,246,843 | 70,917 | 552,972 | 172 | src/libultra/libc/bzero.c |
| `bcopy` | 0.16% | 14.3% | 65,000 | 612 | 71 | 517,808 | 266,358 | src/libultra/libc/bcopy.c |
| `memmove` | 0.02% | 1.3% | 18 | 208,241 | 18,713 | 51,421 | 131 | src/libc/memmove.c |
| `_Litob` | 0.01% | 1.3% | 5,695 | 624 | 107 | 9,316 | 44,004 | src/libultra/libc/xlitob.c |
| `_Printf` | 0.01% | 1.2% | 1,046 | 3,208 | 784 | 9,642 | 35,964 | src/libultra/libc/xprintf.c |
| `__ull_rem` | 0.01% | 0.5% | 5,698 | 222 | 14 | 2,496 | 12,436 | src/libultra/libc/ll.c |
| `memcpy` | 0.00% | 0.2% | 17,638 | 38 | 18 | 3,502 | 2,084 | src/libultra/libc/string.c |
| `strchr` | 0.00% | 0.2% | 12,004 | 51 | 27 | 1,049 | 3,139 | src/libultra/libc/string.c |
| `lldiv` | 0.00% | 0.2% | 2,461 | 237 | 45 | 2,032 | 6,260 | src/libultra/libc/ldiv.c |
| `__ll_div` | 0.00% | 0.2% | 2,461 | 212 | 18 | 956 | 5,272 | src/libultra/libc/ll.c |
| `__ll_mul` | 0.00% | 0.1% | 2,464 | 77 | 12 | 26 | 2,636 | src/libultra/libc/ll.c |
| `strlen` | 0.00% | 0.0% | 4 | 185 | 85 | 4 | 2 | src/libultra/libc/string.c |

## Loading (DMA, decompression, overlays): 1.00%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| yaz0.c | 0.77% | 76.7% | 1,209,049 | 11,702 |
| z_std_dma.c | 0.21% | 21.3% | 360,507 | 443,505 |
| loadfragment2_n64.c | 0.02% | 2.1% | 38,489 | 1,753 |
| z_DLF.c | 0.00% | 0.0% | 39 | 72 |
| z_locale.c | 0.00% | 0.0% | 2 | 5 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Yaz0_DecompressImpl` | 0.76% | 76.1% | 129 | 1,444,595 | 666,168 | 1,198,787 | 8,458 | src/boot/yaz0.c |
| `DmaMgr_ProcessRequest` | 0.10% | 9.5% | 14,674 | 1,588 | 208 | 203,611 | 116,778 | src/boot/z_std_dma.c |
| `DmaMgr_DmaRomToRam` | 0.07% | 6.9% | 20,240 | 840 | 72 | 104,598 | 172,410 | src/boot/z_std_dma.c |
| `DmaMgr_RequestAsync` | 0.03% | 3.2% | 14,674 | 535 | 32 | 42,541 | 93,572 | src/boot/z_std_dma.c |
| `Overlay_Relocate` | 0.02% | 2.0% | 49 | 101,489 | 41,242 | 38,009 | 947 | src/libu64/loadfragment2_n64.c |
| `DmaMgr_ThreadEntry` | 0.01% | 0.9% | 1 | 2,139,605 | 220,268 | 1,386 | 37,787 | src/boot/z_std_dma.c |
| `Yaz0_NextDMA` | 0.01% | 0.5% | 5,564 | 221 | 54 | 9,658 | 2,509 | src/boot/yaz0.c |
| `DmaMgr_AudioDmaHandler` | 0.00% | 0.4% | 3,410 | 289 | 20 | 6,780 | 10,852 | src/boot/z_std_dma.c |
| `DmaMgr_RequestSync` | 0.00% | 0.3% | 2,946 | 262 | 29 | 1,589 | 12,097 | src/boot/z_std_dma.c |
| `Overlay_Load` | 0.00% | 0.0% | 49 | 1,421 | 67 | 479 | 794 | src/libu64/loadfragment2_n64.c |
| `Yaz0_FirstDMA` | 0.00% | 0.0% | 129 | 348 | 38 | 241 | 480 | src/boot/yaz0.c |
| `Yaz0_Decompress` | 0.00% | 0.0% | 130 | 299 | 19 | 363 | 255 | src/boot/yaz0.c |
| `Overlay_LoadGameState` | 0.00% | 0.0% | 9 | 463 | 23 | 25 | 44 | src/code/z_DLF.c |
| `Overlay_FreeGameState` | 0.00% | 0.0% | 8 | 316 | 19 | 14 | 28 | src/code/z_DLF.c |
| `Overlay_AllocateAndLoad` | 0.00% | 0.0% | 3 | 241 | 23 | 1 | 12 | src/libu64/loadfragment2_n64.c |
| `DmaMgr_Init` | 0.00% | 0.0% | 1 | 612 | 55 | 2 | 9 | src/boot/z_std_dma.c |
| `Locale_Init` | 0.00% | 0.0% | 1 | 422 | 19 | 2 | 5 | src/boot/z_locale.c |

## Game loop (game.c, z_play.c): 0.99%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_play.c | 0.82% | 82.9% | 1,161,538 | 2,316,502 |
| game.c | 0.16% | 15.6% | 237,012 | 420,085 |
| z_frame_advance.c | 0.01% | 1.5% | 22,790 | 45,574 |
| z_game_over.c | 0.00% | 0.0% | 5 | 5 |
| title_setup.c | 0.00% | 0.0% | 5 | 4 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Play_Draw` | 0.35% | 35.7% | 11,393 | 7,584 | 532 | 573,477 | 918,605 | src/code/z_play.c |
| `Play_Update` | 0.28% | 28.7% | 11,415 | 6,088 | 528 | 369,511 | 869,361 | src/code/z_play.c |
| `func_800BFCB8` | 0.09% | 9.2% | 19,924 | 1,119 | 77 | 93,349 | 247,015 | src/code/z_play.c |
| `GameState_SetFrameBuffer` | 0.05% | 4.9% | 12,171 | 965 | 54 | 96,368 | 97,468 | src/code/game.c |
| `GameState_Draw` | 0.04% | 3.9% | 12,171 | 778 | 53 | 48,642 | 109,561 | src/code/game.c |
| `Play_Main` | 0.04% | 3.6% | 11,393 | 764 | 56 | 56,215 | 105,051 | src/code/z_play.c |
| `GameState_Update` | 0.03% | 3.5% | 12,172 | 695 | 37 | 73,729 | 90,906 | src/code/game.c |
| `FishmodPlay_ClearZ` | 0.03% | 2.6% | 34,178 | 182 | 14 | 25,080 | 82,158 | src/code/z_play.c |
| `func_800C49F4` | 0.02% | 2.0% | 12,171 | 401 | 33 | 18,132 | 61,066 | src/code/game.c |
| `FrameAdvance_Update` | 0.01% | 1.5% | 11,393 | 317 | 8 | 22,785 | 45,569 | src/code/z_frame_advance.c |
| `Play_InCsMode` | 0.01% | 1.4% | 27,035 | 127 | 10 | 15,956 | 47,716 | src/code/z_play.c |
| `GameState_ReqPadData` | 0.01% | 1.1% | 12,173 | 218 | 19 | 38 | 48,743 | src/code/game.c |
| `FrameAdvance_IsEnabled` | 0.01% | 0.8% | 52,932 | 38 | 3 | 16,178 | 22,161 | src/code/z_play.c |
| `Play_GetCamera` | 0.01% | 0.7% | 11,759 | 151 | 7 | 10,479 | 22,829 | src/code/z_play.c |
| `GameState_IsRunning` | 0.00% | 0.3% | 12,211 | 51 | 2 | 8 | 12,177 | src/code/game.c |
| `Play_CamIsNotFixed` | 0.00% | 0.0% | 274 | 349 | 21 | 828 | 822 | src/code/z_play.c |
| `Play_Init` | 0.00% | 0.0% | 5 | 5,622 | 337 | 171 | 299 | src/code/z_play.c |
| `GameState_Init` | 0.00% | 0.0% | 9 | 906 | 68 | 61 | 79 | src/code/game.c |
| `Play_SpawnScene` | 0.00% | 0.0% | 5 | 1,510 | 63 | 55 | 64 | src/code/z_play.c |
| `Play_InitScene` | 0.00% | 0.0% | 5 | 1,362 | 56 | 51 | 59 | src/code/z_play.c |
| `Play_Destroy` | 0.00% | 0.0% | 4 | 1,462 | 79 | 45 | 60 | src/code/z_play.c |
| `Play_RequestCameraSetting` | 0.00% | 0.0% | 45 | 125 | 9 | 19 | 88 | src/code/z_play.c |
| `Play_ChangeCameraStatus` | 0.00% | 0.0% | 89 | 62 | 12 | 29 | 56 | src/code/z_play.c |
| `Play_SetupTransition` | 0.00% | 0.0% | 9 | 531 | 57 | 9 | 73 | src/code/z_play.c |
| `Play_SetupRespawnPoint` | 0.00% | 0.0% | 5 | 925 | 34 | 47 | 26 | src/code/z_play.c |
| (12 more functions) | 0.00% | 0.0% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Play_Draw` (src/code/z_play.c): 1528: 4%, 23,737 data fills; 1547: 3%, 35,574 data fills; 1226: 3%, 19,381 data fills; 1265: 3%, 11,519 data fills; 1408: 3%, 22,980 data fills
- `Play_Update` (src/code/z_play.c): 1165: 6%, 34,177 data fills; 1057: 4%, 13,057 data fills; 999: 4%, 31,440 data fills; 1036: 4%, 11,398 data fills; 638: 4%, 34,104 data fills

## Effects: 0.71%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_effect_soft_sprite.c | 0.27% | 38.5% | 655,319 | 300,235 |
| z_effect.c | 0.25% | 34.9% | 724,230 | 216,693 |
| ovl_Effect_Ss_KiraKira | 0.15% | 20.9% | 255,246 | 122,984 |
| z_eff_blure.c | 0.03% | 3.8% | 35,712 | 79,807 |
| z_effect_soft_sprite_old_init.c | 0.01% | 1.4% | 5,666 | 32,267 |
| ovl_Effect_Ss_Dust | 0.00% | 0.6% | 7,188 | 5,722 |
| z_eff_shield_particle.c | 0.00% | 0.0% | 20 | 12 |
| z_eff_spark.c | 0.00% | 0.0% | 0 | 4 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Effect_UpdateAll` | 0.13% | 18.1% | 11,392 | 2,753 | 230 | 364,547 | 125,432 | src/code/z_effect.c |
| `EffectSs_DrawAll` | 0.12% | 17.0% | 11,392 | 2,587 | 355 | 295,289 | 115,368 | src/code/z_effect_soft_sprite.c |
| `Effect_DrawAll` | 0.12% | 16.8% | 11,393 | 2,561 | 218 | 359,393 | 91,156 | src/code/z_effect.c |
| `EffectSsKiraKira_Draw` | 0.11% | 15.6% | 42,466 | 639 | 131 | 203,512 | 68,217 | src/overlays/effects/ovl_Effect_Ss_KiraKira/z_eff_ss_kirakira.c |
| `EffectSs_UpdateAll` | 0.08% | 11.4% | 11,393 | 1,741 | 304 | 187,511 | 103,978 | src/code/z_effect_soft_sprite.c |
| `EffectSs_Update` | 0.05% | 6.6% | 44,961 | 254 | 31 | 130,764 | 17,903 | src/code/z_effect_soft_sprite.c |
| `func_809AABF0` | 0.03% | 4.3% | 42,467 | 175 | 40 | 50,937 | 28,257 | src/overlays/effects/ovl_Effect_Ss_KiraKira/z_eff_ss_kirakira.c |
| `EffectBlure_Update` | 0.01% | 2.1% | 11,412 | 320 | 17 | 18,298 | 45,568 | src/code/z_eff_blure.c |
| `EffectBlure_Draw` | 0.01% | 1.7% | 11,394 | 262 | 14 | 17,264 | 34,176 | src/code/z_eff_blure.c |
| `EffectSs_Delete` | 0.01% | 1.5% | 5,441 | 491 | 36 | 23,529 | 19,283 | src/code/z_effect_soft_sprite.c |
| `EffectSsKiraKira_SpawnDispersed` | 0.01% | 1.4% | 2,654 | 891 | 86 | 5,526 | 31,853 | src/code/z_effect_soft_sprite_old_init.c |
| `EffectSs_Spawn` | 0.01% | 1.3% | 2,723 | 848 | 82 | 10,255 | 32,694 | src/code/z_effect_soft_sprite.c |
| `EffectSsKiraKira_Init` | 0.01% | 1.0% | 0 | 0 | 0 | 797 | 26,510 | src/overlays/effects/ovl_Effect_Ss_KiraKira/z_eff_ss_kirakira.c |
| `EffectSs_FindSlot` | 0.00% | 0.6% | 2,723 | 353 | 19 | 5,479 | 10,893 | src/code/z_effect_soft_sprite.c |
| `EffectSsDust_Draw` | 0.00% | 0.5% | 737 | 1,187 | 184 | 7,048 | 4,927 | src/overlays/effects/ovl_Effect_Ss_Dust/z_eff_ss_dust.c |
| `EffectSs_InitInfo` | 0.00% | 0.1% | 5 | 37,697 | 2,960 | 2,337 | 100 | src/code/z_effect_soft_sprite.c |
| `EffectSsDust_Update` | 0.00% | 0.0% | 737 | 109 | 40 | 81 | 523 | src/overlays/effects/ovl_Effect_Ss_Dust/z_eff_ss_dust.c |
| `func_800287AC` | 0.00% | 0.0% | 69 | 510 | 42 | 140 | 414 | src/code/z_effect_soft_sprite_old_init.c |
| `EffectSsDust_Init` | 0.00% | 0.0% | 0 | 0 | 0 | 59 | 270 | src/overlays/effects/ovl_Effect_Ss_Dust/z_eff_ss_dust.c |
| `Effect_InitContext` | 0.00% | 0.0% | 5 | 2,108 | 144 | 140 | 20 | src/code/z_effect.c |
| `Effect_DeleteAll` | 0.00% | 0.0% | 4 | 2,503 | 208 | 140 | 24 | src/code/z_effect.c |
| `EffectBlure_InitElements` | 0.00% | 0.0% | 5 | 1,959 | 183 | 126 | 20 | src/code/z_eff_blure.c |
| `EffectSs_ClearAll` | 0.00% | 0.0% | 4 | 2,434 | 207 | 155 | 16 | src/code/z_effect_soft_sprite.c |
| `EffectBlure_Init2` | 0.00% | 0.0% | 5 | 783 | 81 | 24 | 35 | src/code/z_eff_blure.c |
| `Effect_Add` | 0.00% | 0.0% | 5 | 626 | 58 | 10 | 45 | src/code/z_effect.c |
| (5 more functions) | 0.00% | 0.0% | | | | | | |

## Display list setup: 0.59%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_rcp.c | 0.34% | 56.9% | 427,215 | 908,600 |
| gfxprint.c | 0.21% | 35.5% | 295,617 | 500,057 |
| sys_cfb.c | 0.01% | 2.3% | 24,348 | 36,525 |
| sys_ucode.c | 0.01% | 2.3% | 19,604 | 31,885 |
| gfxalloc.c | 0.01% | 2.1% | 12,351 | 31,658 |
| TwoHeadGfxArena.c | 0.01% | 0.9% | 0 | 24,348 |
| z_vimode.c | 0.00% | 0.0% | 56 | 67 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `GfxPrint_Setup` | 0.19% | 31.2% | 12,172 | 3,719 | 340 | 271,370 | 414,647 | src/libu64/gfxprint.c |
| `Gfx_SetupFrame` | 0.18% | 30.6% | 12,172 | 3,642 | 308 | 243,923 | 484,535 | src/code/z_rcp.c |
| `Gfx_SetFog2` | 0.04% | 5.9% | 22,788 | 378 | 44 | 22,833 | 91,158 | src/code/z_rcp.c |
| `Gfx_SetupDL_39Overlay` | 0.02% | 2.9% | 46,888 | 89 | 9 | 19,303 | 45,617 | src/code/z_rcp.c |
| `GfxPrint_Init` | 0.01% | 2.5% | 12,197 | 299 | 21 | 17,710 | 48,686 | src/libu64/gfxprint.c |
| `Gfx_SetupDL_25Xlu2` | 0.01% | 2.3% | 42,481 | 78 | 9 | 38,555 | 6,412 | src/code/z_rcp.c |
| `Gfx_SetupDL_25Xlu` | 0.01% | 2.1% | 18,361 | 165 | 9 | 15,253 | 33,720 | src/code/z_rcp.c |
| `func_80093C80` | 0.01% | 2.1% | 14,078 | 211 | 16 | 15,894 | 42,219 | src/code/z_rcp.c |
| `Gfx_TwoTexScroll` | 0.01% | 2.0% | 8,105 | 359 | 60 | 24,352 | 24,998 | src/code/z_rcp.c |
| `Gfx_SetupDL_42Overlay` | 0.01% | 1.9% | 22,262 | 125 | 9 | 9,383 | 38,227 | src/code/z_rcp.c |
| `Gfx_SetupDL_25Opa` | 0.01% | 1.8% | 21,604 | 120 | 9 | 4,531 | 42,541 | src/code/z_rcp.c |
| `func_800947AC` | 0.01% | 1.8% | 11,393 | 227 | 12 | 10,573 | 34,269 | src/code/z_rcp.c |
| `Gfx_SetupDL` | 0.01% | 1.4% | 16,280 | 122 | 11 | 9,686 | 23,653 | src/code/z_rcp.c |
| `GfxPrint_Open` | 0.01% | 1.3% | 12,171 | 150 | 14 | 6,225 | 24,439 | src/libu64/gfxprint.c |
| `SysCfb_GetFbPtr` | 0.01% | 1.3% | 12,172 | 150 | 12 | 12,170 | 24,345 | src/code/sys_cfb.c |
| `Gfx_Close` | 0.01% | 1.1% | 59,611 | 28 | 5 | 12,351 | 6,136 | src/code/gfxalloc.c |
| `Gfx_SetupDL_27Xlu` | 0.01% | 1.1% | 12,066 | 135 | 9 | 7,254 | 19,737 | src/code/z_rcp.c |
| `SysCfb_GetFbCount` | 0.01% | 1.1% | 12,172 | 128 | 3 | 12,170 | 12,171 | src/code/sys_cfb.c |
| `Gfx_Open` | 0.01% | 0.9% | 59,674 | 23 | 2 | 0 | 25,522 | src/code/gfxalloc.c |
| `SysUcode_GetUCode` | 0.00% | 0.7% | 19,605 | 52 | 3 | 12,170 | 1 | src/code/sys_ucode.c |
| `SysUcode_GetUCodeData` | 0.00% | 0.7% | 19,605 | 51 | 3 | 7,434 | 7,436 | src/code/sys_ucode.c |
| `GfxPrint_Close` | 0.00% | 0.5% | 12,233 | 59 | 6 | 279 | 12,171 | src/libu64/gfxprint.c |
| `THGA_Init` | 0.00% | 0.5% | 48,733 | 14 | 2 | 0 | 12,171 | src/code/TwoHeadGfxArena.c |
| `SysUcode_GetUCodeBootSize` | 0.00% | 0.5% | 12,320 | 54 | 6 | 0 | 12,224 | src/code/sys_ucode.c |
| `THGA_IsCrash` | 0.00% | 0.5% | 36,564 | 18 | 2 | 0 | 12,177 | src/code/TwoHeadGfxArena.c |
| (16 more functions) | 0.01% | 1.6% | | | | | | |

## Title, file select, menus: 0.41%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| ovl_file_choose | 0.41% | 100.0% | 777,325 | 276,597 |
| ovl_title | 0.00% | 0.0% | 9 | 10 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `FileSelect_SetWindowContentVtx` | 0.24% | 60.3% | 548 | 109,072 | 25,928 | 478,752 | 104,095 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_DrawWindowContents` | 0.05% | 11.3% | 463 | 24,249 | 2,947 | 80,234 | 61,671 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_SetWindowVtx` | 0.02% | 5.6% | 548 | 10,136 | 2,768 | 44,595 | 13,152 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_DrawCharacterTransition` | 0.02% | 5.3% | 7,150 | 732 | 108 | 54,006 | 1,430 | src/overlays/gamestates/ovl_file_choose/z_file_nameset.c |
| `FileSelect_SetKeyboardVtx` | 0.02% | 3.9% | 110 | 35,327 | 8,938 | 29,303 | 2,420 | src/overlays/gamestates/ovl_file_choose/z_file_nameset.c |
| `FileSelect_DrawFileInfo` | 0.01% | 3.1% | 1,389 | 2,223 | 253 | 18,801 | 23,741 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_DrawCharacter` | 0.01% | 2.6% | 5,408 | 474 | 74 | 22,364 | 5,480 | src/overlays/gamestates/ovl_file_choose/z_file_nameset.c |
| `FileSelect_ConfigModeDraw` | 0.01% | 2.1% | 459 | 4,477 | 308 | 13,985 | 19,014 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_Main` | 0.01% | 1.9% | 548 | 3,517 | 235 | 12,174 | 18,028 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_SetNameEntryVtx` | 0.01% | 1.6% | 110 | 14,397 | 2,322 | 10,554 | 8,871 | src/overlays/gamestates/ovl_file_choose/z_file_nameset.c |
| `FileSelect_DrawKeyboard` | 0.00% | 0.6% | 110 | 5,730 | 1,527 | 3,692 | 3,167 | src/overlays/gamestates/ovl_file_choose/z_file_nameset.c |
| `FileSelect_DrawNameEntry` | 0.00% | 0.5% | 110 | 4,267 | 328 | 2,282 | 5,056 | src/overlays/gamestates/ovl_file_choose/z_file_nameset.c |
| `FileSelect_SelectModeDraw` | 0.00% | 0.4% | 89 | 4,116 | 277 | 2,546 | 3,219 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_PulsateCursor` | 0.00% | 0.3% | 658 | 390 | 38 | 1,481 | 2,740 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_ConfigModeUpdate` | 0.00% | 0.1% | 459 | 252 | 10 | 916 | 918 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_UpdateMainMenu` | 0.00% | 0.1% | 298 | 371 | 36 | 310 | 1,504 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_FadeInMenuElements` | 0.00% | 0.1% | 51 | 1,229 | 87 | 564 | 453 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_UpdateKeyboardCursor` | 0.00% | 0.1% | 80 | 661 | 47 | 213 | 692 | src/overlays/gamestates/ovl_file_choose/z_file_nameset.c |
| `FileSelect_SelectModeUpdate` | 0.00% | 0.0% | 88 | 241 | 10 | 176 | 176 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_FinishFadeIn` | 0.00% | 0.0% | 46 | 282 | 17 | 46 | 171 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_ConfirmFile` | 0.00% | 0.0% | 44 | 284 | 28 | 1 | 224 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_FadeMainToSelect` | 0.00% | 0.0% | 8 | 1,209 | 100 | 80 | 81 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_FadeOut` | 0.00% | 0.0% | 26 | 287 | 13 | 52 | 79 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_FadeInFileInfo` | 0.00% | 0.0% | 8 | 646 | 32 | 32 | 48 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_RotateToNameEntry` | 0.00% | 0.0% | 13 | 377 | 16 | 39 | 39 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| (12 more functions) | 0.00% | 0.0% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `FileSelect_SetWindowContentVtx` (src/overlays/gamestates/ovl_file_choose/z_file_choose.c): 879: 21%, 178,176 data fills; 878: 12%, 90,024 data fills; 896: 11%, 89,176 data fills; 920: 2%, 16,473 data fills; 902: 2%

## Text: 0.37%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_message.c | 0.36% | 99.4% | 720,423 | 377,801 |
| z_elf_message.c | 0.00% | 0.5% | 2,401 | 4,802 |
| z_kanfont.c | 0.00% | 0.1% | 371 | 183 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Message_DrawTextChar` | 0.21% | 56.9% | 60,399 | 843 | 148 | 528,854 | 34,287 | src/code/z_message.c |
| `Message_DrawText` | 0.06% | 15.8% | 1,840 | 7,677 | 1,845 | 65,329 | 72,511 | src/code/z_message.c |
| `Message_DrawMain` | 0.04% | 10.3% | 11,393 | 807 | 59 | 45,703 | 101,808 | src/code/z_message.c |
| `Message_Draw` | 0.02% | 6.6% | 11,393 | 516 | 37 | 27,150 | 68,384 | src/code/z_message.c |
| `Message_Update` | 0.01% | 3.2% | 11,393 | 251 | 13 | 16,999 | 34,330 | src/code/z_message.c |
| `Message_GetState` | 0.01% | 3.0% | 20,065 | 135 | 10 | 14,178 | 37,068 | src/code/z_message.c |
| `Message_DrawTextBox` | 0.01% | 2.1% | 1,273 | 1,506 | 99 | 13,313 | 17,917 | src/code/z_message.c |
| `QuestHint_GetNaviTextId` | 0.00% | 0.5% | 2,405 | 177 | 7 | 2,401 | 4,802 | src/code/z_elf_message.c |
| `Message_SetTextColor` | 0.00% | 0.4% | 1,566 | 235 | 21 | 1,354 | 5,071 | src/code/z_message.c |
| `Message_DrawTextboxIcon` | 0.00% | 0.4% | 92 | 3,493 | 298 | 1,169 | 3,739 | src/code/z_message.c |
| `Message_Decode` | 0.00% | 0.3% | 23 | 13,076 | 4,596 | 1,860 | 1,475 | src/code/z_message.c |
| `Message_FindMessageNES` | 0.00% | 0.3% | 9 | 30,115 | 3,412 | 3,851 | 48 | src/code/z_message.c |
| `Message_GrowTextbox` | 0.00% | 0.1% | 45 | 1,310 | 98 | 390 | 585 | src/code/z_message.c |
| `Font_LoadOrderedFont` | 0.00% | 0.1% | 7 | 8,056 | 3,905 | 371 | 94 | src/code/z_kanfont.c |
| `Message_OpenText` | 0.00% | 0.0% | 10 | 2,233 | 130 | 156 | 254 | src/code/z_message.c |
| `Message_ShouldAdvance` | 0.00% | 0.0% | 76 | 243 | 16 | 66 | 240 | src/code/z_message.c |
| `Font_LoadChar` | 0.00% | 0.0% | 1,138 | 11 | 8 | 0 | 68 | src/code/z_kanfont.c |
| `Message_StartTextbox` | 0.00% | 0.0% | 9 | 469 | 21 | 40 | 45 | src/code/z_message.c |
| `Message_SetTables` | 0.00% | 0.0% | 5 | 323 | 17 | 10 | 15 | src/code/z_message.c |
| `Message_CloseTextbox.part.0` | 0.00% | 0.0% | 6 | 235 | 23 | 1 | 24 | src/code/z_message.c |
| `Font_LoadMessageBoxIcon` | 0.00% | 0.0% | 21 | 55 | 7 | 0 | 21 | src/code/z_kanfont.c |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Message_DrawTextChar` (src/code/z_message.c): 424: 31%, 211,727 data fills; 431: 16%, 88,867 data fills; 439: 16%, 92,879 data fills; 421: 8%; 438: 7%, 31,012 data fills

## Cutscenes: 0.33%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_demo.c | 0.29% | 87.5% | 662,154 | 425,177 |
| z_cutscene_spline.c | 0.04% | 12.5% | 24,205 | 96,578 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Cutscene_ProcessScript` | 0.09% | 26.0% | 4,492 | 4,650 | 984 | 110,585 | 192,033 | src/code/z_demo.c |
| `CutsceneCmd_UpdateCamEyeSpline` | 0.08% | 23.4% | 18,226 | 1,033 | 81 | 250,545 | 31,547 | src/code/z_demo.c |
| `CutsceneCmd_UpdateCamAtSpline` | 0.07% | 22.6% | 18,213 | 997 | 82 | 250,558 | 27,209 | src/code/z_demo.c |
| `func_800BB0A0` | 0.02% | 6.4% | 6,058 | 853 | 127 | 9,123 | 50,046 | src/code/z_cutscene_spline.c |
| `func_800BB2B4` | 0.02% | 6.1% | 6,056 | 810 | 150 | 15,082 | 46,532 | src/code/z_cutscene_spline.c |
| `Cutscene_UpdateScripted` | 0.01% | 3.7% | 11,392 | 258 | 17 | 15,883 | 43,152 | src/code/z_demo.c |
| `Cutscene_UpdateManual` | 0.01% | 2.8% | 11,420 | 199 | 11 | 9,992 | 34,176 | src/code/z_demo.c |
| `CutsceneCmd_Text` | 0.01% | 2.4% | 10,085 | 188 | 22 | 5,366 | 25,216 | src/code/z_demo.c |
| `CutsceneCmd_Destination` | 0.01% | 2.1% | 3,526 | 487 | 28 | 12,639 | 17,933 | src/code/z_demo.c |
| `Cutscene_SetupScripted` | 0.01% | 1.6% | 4,488 | 277 | 24 | 56 | 22,487 | src/code/z_demo.c |
| `CutsceneCmd_Misc` | 0.01% | 1.5% | 8,671 | 141 | 22 | 4,949 | 13,272 | src/code/z_demo.c |
| `CutsceneHandler_RunScript` | 0.00% | 0.7% | 4,487 | 121 | 10 | 887 | 8,974 | src/code/z_demo.c |
| `CutsceneHandler_DoNothing` | 0.00% | 0.4% | 6,919 | 50 | 2 | 0 | 6,904 | src/code/z_demo.c |
| `CutsceneCmd_Transition.part.0` | 0.00% | 0.2% | 307 | 549 | 48 | 577 | 2,206 | src/code/z_demo.c |
| `Cutscene_HandleEntranceTriggers` | 0.00% | 0.0% | 5 | 1,799 | 333 | 95 | 35 | src/code/z_demo.c |
| `Cutscene_HandleConditionalTriggers` | 0.00% | 0.0% | 5 | 361 | 22 | 9 | 20 | src/code/z_demo.c |
| `Cutscene_InitContext` | 0.00% | 0.0% | 5 | 211 | 3 | 10 | 5 | src/code/z_demo.c |
| `CutsceneHandler_StopScript` | 0.00% | 0.0% | 1 | 618 | 57 | 3 | 8 | src/code/z_demo.c |

## Scenes, rooms and objects: 0.29%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_room.c | 0.20% | 69.5% | 203,869 | 528,915 |
| z_scene.c | 0.05% | 16.4% | 89,544 | 91,631 |
| z_scene_table.c | 0.04% | 14.0% | 63,308 | 114,954 |
| z_path.c | 0.00% | 0.0% | 1 | 14 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Room_DrawCullable` | 0.08% | 25.8% | 2,680 | 6,905 | 1,631 | 108,662 | 87,347 | src/code/z_room.c |
| `Room_DrawImageSingle` | 0.06% | 21.5% | 8,712 | 1,770 | 167 | 41,653 | 221,373 | src/code/z_room.c |
| `Object_UpdateEntries` | 0.03% | 11.0% | 11,392 | 693 | 75 | 60,624 | 68,439 | src/code/z_scene.c |
| `Room_DrawBackground2D` | 0.02% | 8.3% | 7,435 | 801 | 103 | 92 | 97,670 | src/code/z_room.c |
| `Room_Draw` | 0.02% | 7.9% | 22,810 | 247 | 19 | 34,474 | 67,182 | src/code/z_room.c |
| `Scene_DrawConfigKokiriForest` | 0.02% | 5.2% | 1,845 | 2,037 | 182 | 21,185 | 46,259 | src/code/z_scene_table.c |
| `Object_IsLoaded` | 0.02% | 5.2% | 87,100 | 43 | 7 | 27,825 | 22,211 | src/code/z_scene.c |
| `Scene_Draw` | 0.01% | 4.1% | 11,392 | 260 | 12 | 18,639 | 34,178 | src/code/z_scene_table.c |
| `Scene_DrawConfigDefault` | 0.01% | 2.8% | 8,712 | 230 | 15 | 17,446 | 17,424 | src/code/z_scene_table.c |
| `Room_ProcessRoomRequest` | 0.01% | 2.0% | 11,397 | 128 | 5 | 11,419 | 11,503 | src/code/z_room.c |
| `Room_DecodeJpeg` | 0.01% | 1.9% | 7,435 | 183 | 7 | 7,436 | 14,876 | src/code/z_room.c |
| `Scene_DrawConfigHyruleField` | 0.01% | 1.9% | 835 | 1,590 | 141 | 6,038 | 17,093 | src/code/z_scene_table.c |
| `Room_DrawImage` | 0.00% | 1.3% | 8,712 | 105 | 7 | 0 | 17,424 | src/code/z_room.c |
| `func_80095AA0` | 0.00% | 0.8% | 22,814 | 26 | 2 | 0 | 11,391 | src/code/z_room.c |
| `Object_GetSlot` | 0.00% | 0.1% | 225 | 175 | 41 | 324 | 164 | src/code/z_scene.c |
| `Scene_CommandAlternateHeaderList` | 0.00% | 0.0% | 11 | 1,789 | 160 | 179 | 127 | src/code/z_scene.c |
| `Object_LoadPersistent` | 0.00% | 0.0% | 15 | 993 | 48 | 112 | 116 | src/code/z_scene.c |
| `Object_InitContext` | 0.00% | 0.0% | 5 | 2,270 | 101 | 120 | 45 | src/code/z_scene.c |
| `Scene_CommandObjectList` | 0.00% | 0.0% | 6 | 1,662 | 182 | 79 | 81 | src/code/z_scene.c |
| `Room_RequestNewRoom.part.0` | 0.00% | 0.0% | 6 | 1,543 | 79 | 74 | 73 | src/code/z_room.c |
| `Scene_CommandTimeSettings` | 0.00% | 0.0% | 6 | 1,380 | 96 | 50 | 85 | src/code/z_scene.c |
| `Scene_ExecuteCommands` | 0.00% | 0.0% | 11 | 605 | 35 | 44 | 71 | src/code/z_scene.c |
| `Room_SetupFirstRoom` | 0.00% | 0.0% | 5 | 1,231 | 69 | 44 | 62 | src/code/z_room.c |
| `Scene_CommandPlayerEntryList` | 0.00% | 0.0% | 5 | 864 | 41 | 43 | 27 | src/code/z_scene.c |
| `Scene_CommandCollisionHeader` | 0.00% | 0.0% | 5 | 852 | 73 | 18 | 50 | src/code/z_scene.c |
| (22 more functions) | 0.00% | 0.0% | | | | | | |

## Other (no source): 0.28%

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `_icon_item_jpn_staticSegmentDataEnd` | 0.25% | 89.1% | 0 | 0 | 0 | 65,536 | 481,272 |  |
| `_n64ddSegmentBssEnd` | 0.03% | 10.9% | 0 | 0 | 0 | 57,189 | 47,434 |  |

## Pause menu and screen effects: 0.15%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| jpegdecoder.c | 0.07% | 46.6% | 36,119 | 18,514 |
| z_kaleido_setup.c | 0.03% | 17.3% | 24,635 | 87,737 |
| PreRender.c | 0.02% | 14.9% | 57,095 | 23,039 |
| z_kaleido_scope_call.c | 0.02% | 11.9% | 34,082 | 45,817 |
| z_jpeg.c | 0.01% | 9.1% | 45,396 | 1,912 |
| jpegutils.c | 0.00% | 0.1% | 193 | 46 |
| z_kaleido_manager.c | 0.00% | 0.0% | 146 | 164 |
| ovl_kaleido_scope | 0.00% | 0.0% | 109 | 74 |
| z_vismono.c | 0.00% | 0.0% | 7 | 38 |
| z_viszbuffer.c | 0.00% | 0.0% | 9 | 24 |
| z_viscvg.c | 0.00% | 0.0% | 9 | 15 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `JpegDecoder_ParseNextSymbol` | 0.03% | 22.0% | 70,719 | 114 | 84 | 13,058 | 7,383 | src/code/jpegdecoder.c |
| `KaleidoSetup_Update` | 0.03% | 17.3% | 9,189 | 691 | 51 | 24,560 | 87,678 | src/code/z_kaleido_setup.c |
| `PreRender_SetValues` | 0.02% | 14.9% | 11,414 | 477 | 12 | 57,095 | 23,015 | src/code/PreRender.c |
| `JpegDecoder_ProcessMcu` | 0.02% | 12.8% | 3,600 | 1,303 | 778 | 16,993 | 7,445 | src/code/jpegdecoder.c |
| `KaleidoScopeCall_LoadPlayer` | 0.02% | 11.9% | 22,821 | 192 | 12 | 34,067 | 45,793 | src/code/z_kaleido_scope_call.c |
| `JpegDecoder_ReadBits` | 0.02% | 11.3% | 137,408 | 30 | 22 | 4,604 | 2,931 | src/code/jpegdecoder.c |
| `Jpeg_CopyToZbuffer` | 0.01% | 7.4% | 600 | 4,517 | 569 | 39,937 | 761 | src/code/z_jpeg.c |
| `Jpeg_ParseMarkers` | 0.00% | 1.0% | 2 | 182,898 | 91,238 | 2,877 | 22 | src/code/z_jpeg.c |
| `Jpeg_ScheduleDecoderTask` | 0.00% | 0.6% | 150 | 1,388 | 107 | 2,406 | 579 | src/code/z_jpeg.c |
| `JpegDecoder_Decode` | 0.00% | 0.5% | 150 | 1,146 | 315 | 1,464 | 755 | src/code/jpegdecoder.c |
| `Jpeg_Decode` | 0.00% | 0.1% | 2 | 22,276 | 4,420 | 176 | 550 | src/code/z_jpeg.c |
| `JpegUtils_ProcessHuffmanTable` | 0.00% | 0.1% | 8 | 3,199 | 2,192 | 87 | 26 | src/code/jpegutils.c |
| `KaleidoManager_GetRamAddr` | 0.00% | 0.0% | 30 | 229 | 29 | 40 | 81 | src/code/z_kaleido_manager.c |
| `JpegUtils_ProcessQuantizationTable` | 0.00% | 0.0% | 6 | 1,000 | 335 | 66 | 6 | src/code/jpegutils.c |
| `KaleidoSetup_Init` | 0.00% | 0.0% | 5 | 1,198 | 70 | 75 | 51 | src/code/z_kaleido_setup.c |
| `JpegUtils_SetHuffmanTable` | 0.00% | 0.0% | 8 | 702 | 275 | 40 | 14 | src/code/jpegutils.c |
| `KaleidoScope_DrawEquipment` | 0.00% | 0.0% | 0 | 0 | 0 | 98 | 16 | src/overlays/misc/ovl_kaleido_scope/z_kaleido_equipment.c |
| `KaleidoManager_ClearOvl` | 0.00% | 0.0% | 5 | 718 | 20 | 30 | 23 | src/code/z_kaleido_manager.c |
| `KaleidoManager_Destroy` | 0.00% | 0.0% | 4 | 732 | 24 | 26 | 20 | src/code/z_kaleido_manager.c |
| `KaleidoManager_LoadOvl` | 0.00% | 0.0% | 5 | 552 | 23 | 30 | 20 | src/code/z_kaleido_manager.c |
| `KaleidoScope_DrawDungeonMap` | 0.00% | 0.0% | 0 | 0 | 0 | 2 | 40 | src/overlays/misc/ovl_kaleido_scope/z_kaleido_map.c |
| `KaleidoManager_Init` | 0.00% | 0.0% | 5 | 389 | 25 | 20 | 20 | src/code/z_kaleido_manager.c |
| `KaleidoScopeCall_Init` | 0.00% | 0.0% | 5 | 334 | 19 | 15 | 20 | src/code/z_kaleido_scope_call.c |
| `VisMono_Init` | 0.00% | 0.0% | 14 | 111 | 13 | 0 | 27 | src/code/z_vismono.c |
| `VisZBuffer_Init` | 0.00% | 0.0% | 9 | 158 | 7 | 9 | 17 | src/code/z_viszbuffer.c |
| (14 more functions) | 0.00% | 0.0% | | | | | | |

## Memory: 0.07%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_memory_utils.c | 0.04% | 66.8% | 82,491 | 9,044 |
| TwoHeadArena.c | 0.02% | 24.9% | 35,552 | 36,985 |
| __osMalloc_n64.c | 0.00% | 7.6% | 14,744 | 2,822 |
| malloc.c | 0.00% | 0.4% | 32 | 1,184 |
| z_malloc.c | 0.00% | 0.3% | 0 | 844 |
| gamealloc.c | 0.00% | 0.0% | 30 | 81 |
| listalloc.c | 0.00% | 0.0% | 4 | 17 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `MemCopy` | 0.04% | 66.8% | 138,440 | 77 | 28 | 82,491 | 9,044 | src/code/z_memory_utils.c |
| `THA_Init` | 0.01% | 19.5% | 48,701 | 64 | 6 | 35,374 | 24,633 | src/code/TwoHeadArena.c |
| `THA_IsCrash` | 0.00% | 5.3% | 36,519 | 23 | 5 | 99 | 12,263 | src/code/TwoHeadArena.c |
| `ArenaImpl_GetSizes` | 0.00% | 4.9% | 717 | 1,102 | 117 | 9,999 | 1,444 | src/libc64/__osMalloc_n64.c |
| `__osMalloc` | 0.00% | 2.0% | 232 | 1,343 | 164 | 3,801 | 725 | src/libc64/__osMalloc_n64.c |
| `__osFree` | 0.00% | 0.6% | 227 | 400 | 47 | 794 | 541 | src/libc64/__osMalloc_n64.c |
| `SystemArena_GetSizes` | 0.00% | 0.4% | 361 | 164 | 14 | 4 | 1,083 | src/libc64/malloc.c |
| `ZeldaArena_GetSizes` | 0.00% | 0.2% | 356 | 102 | 6 | 0 | 712 | src/code/z_malloc.c |
| `__osMallocR` | 0.00% | 0.1% | 9 | 1,513 | 97 | 138 | 86 | src/libc64/__osMalloc_n64.c |
| `THA_AllocTailAlign16` | 0.00% | 0.1% | 111 | 72 | 7 | 68 | 54 | src/code/TwoHeadArena.c |
| `ZeldaArena_Malloc` | 0.00% | 0.0% | 196 | 24 | 4 | 0 | 83 | src/code/z_malloc.c |
| `SystemArena_Free` | 0.00% | 0.0% | 41 | 98 | 16 | 24 | 35 | src/libc64/malloc.c |
| `SystemArena_Malloc` | 0.00% | 0.0% | 36 | 110 | 18 | 4 | 57 | src/libc64/malloc.c |
| `GameAlloc_Malloc` | 0.00% | 0.0% | 14 | 197 | 24 | 3 | 42 | src/code/gamealloc.c |
| `GameAlloc_Cleanup` | 0.00% | 0.0% | 8 | 282 | 20 | 14 | 23 | src/code/gamealloc.c |
| `ZeldaArena_Free` | 0.00% | 0.0% | 186 | 12 | 4 | 0 | 30 | src/code/z_malloc.c |
| `__osMallocInit` | 0.00% | 0.0% | 6 | 328 | 18 | 12 | 18 | src/libc64/__osMalloc_n64.c |
| `THA_AllocTailAlign` | 0.00% | 0.0% | 25 | 54 | 6 | 6 | 15 | src/code/TwoHeadArena.c |
| `GameAlloc_Free` | 0.00% | 0.0% | 5 | 194 | 11 | 10 | 6 | src/code/gamealloc.c |
| `THA_GetRemaining` | 0.00% | 0.0% | 5 | 181 | 4 | 5 | 10 | src/code/TwoHeadArena.c |
| `ListAlloc_FreeAll` | 0.00% | 0.0% | 4 | 199 | 5 | 4 | 12 | src/code/listalloc.c |
| `GameAlloc_Init` | 0.00% | 0.0% | 9 | 78 | 4 | 3 | 10 | src/code/gamealloc.c |
| `THA_Destroy` | 0.00% | 0.0% | 13 | 40 | 3 | 0 | 10 | src/code/TwoHeadArena.c |
| `SystemArena_MallocR` | 0.00% | 0.0% | 3 | 170 | 18 | 0 | 9 | src/libc64/malloc.c |
| `ZeldaArena_Init` | 0.00% | 0.0% | 5 | 101 | 5 | 0 | 10 | src/code/z_malloc.c |
| (4 more functions) | 0.00% | 0.0% | | | | | | |

## Screen transitions: 0.03%

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `TransitionFade_Update` | 0.02% | 61.4% | 11,543 | 339 | 24 | 14,205 | 58,064 | src/code/z_fbdemo_fade.c |
| `TransitionFade_Draw` | 0.01% | 38.2% | 11,679 | 209 | 6 | 3,026 | 46,126 | src/code/z_fbdemo_fade.c |
| `TransitionFade_IsDone` | 0.00% | 0.3% | 159 | 128 | 2 | 159 | 159 | src/code/z_fbdemo_fade.c |
| `TransitionFade_Init` | 0.00% | 0.0% | 13 | 225 | 14 | 9 | 39 | src/code/z_fbdemo_fade.c |
| `TransitionFade_Start` | 0.00% | 0.0% | 13 | 110 | 10 | 0 | 26 | src/code/z_fbdemo_fade.c |
| `TransitionFade_SetType` | 0.00% | 0.0% | 13 | 109 | 7 | 0 | 27 | src/code/z_fbdemo_fade.c |
| `TransitionFade_SetColor` | 0.00% | 0.0% | 13 | 33 | 2 | 0 | 8 | src/code/z_fbdemo_fade.c |
| `TransitionFade_Destroy` | 0.00% | 0.0% | 12 | 34 | 2 | 0 | 8 | src/code/z_fbdemo_fade.c |

## Save data and inventory: 0.00%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_ss_sram.c | 0.00% | 56.0% | 1,554 | 3,583 |
| z_sram.c | 0.00% | 43.9% | 1,700 | 163 |
| z_common_data.c | 0.00% | 0.1% | 3 | 5 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `SsSram_ReadWrite` | 0.00% | 41.2% | 375 | 634 | 55 | 1,541 | 2,258 | src/code/z_ss_sram.c |
| `Sram_VerifyAndLoadAllSaves` | 0.00% | 37.9% | 1 | 218,537 | 111,659 | 1,498 | 66 | src/code/z_sram.c |
| `SsSram_Dma` | 0.00% | 14.8% | 375 | 227 | 42 | 13 | 1,325 | src/code/z_ss_sram.c |
| `Sram_InitSave` | 0.00% | 3.8% | 1 | 21,638 | 12,548 | 87 | 21 | src/code/z_sram.c |
| `Sram_InitNewSave` | 0.00% | 1.4% | 4 | 1,948 | 161 | 72 | 31 | src/code/z_sram.c |
| `Sram_InitDebugSave` | 0.00% | 0.3% | 1 | 2,010 | 176 | 15 | 12 | src/code/z_sram.c |
| `Sram_OpenSave` | 0.00% | 0.3% | 1 | 1,512 | 98 | 15 | 14 | src/code/z_sram.c |
| `Sram_InitSram` | 0.00% | 0.2% | 1 | 1,198 | 130 | 12 | 10 | src/code/z_sram.c |
| `SaveContext_Init` | 0.00% | 0.1% | 1 | 518 | 30 | 3 | 5 | src/code/z_common_data.c |
| `Sram_Alloc` | 0.00% | 0.1% | 2 | 152 | 12 | 1 | 4 | src/code/z_sram.c |
| `Sram_Init` | 0.00% | 0.0% | 5 | 50 | 2 | 0 | 5 | src/code/z_sram.c |

## Other engine code: 0.00%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| kanji_tex.c | 0.00% | 62.5% | 1,208 | 613 |
| entry.c | 0.00% | 37.1% | 1,196 | 2 |
| driveexist.c | 0.00% | 0.4% | 2 | 7 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `KanjiTex_CodepointToOffset` | 0.00% | 62.5% | 1,652 | 93 | 22 | 1,208 | 613 | src/code/kanji_tex.c |
| `entrypoint` | 0.00% | 37.1% | 1 | 90,853 | 11,969 | 1,196 | 2 | src/makerom/entry.c |
| `LeoDriveExist` | 0.00% | 0.4% | 1 | 911 | 43 | 2 | 7 | src/libleo/api/driveexist.c |

