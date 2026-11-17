# CPU profile by system

- 1 exact profile(s) from ares
- 6,158,719,360 instructions, 230.0 s of CPU time on the emulator's clock (21,565,692,744 cycles); idle 59.4% of it
- data cache: 55,318,303 line fills, 34,249,458 write-backs; code cache: 41,607,894 line fills (a fill is about 40-48 cycles: 25.9% of all cycles)
- per frame (5000 frames): 1,741,283 cycles working (18.57 ms)
- the profiler and test harness's own code: 0.2% of the run
- shares below are of the time the game's own code was working (idle and the harness left out)

## Systems

| system | share | CPU s | instructions | cycles/instr | data fills | code fills | cache share of its time |
|---|---:|---:|---:|---:|---:|---:|---:|
| Audio | 25.63% | 23.80 | 654,382,024 | 3.41 | 17,180,712 | 7,901,298 | 61% |
| libultra (OS: threads, interrupts, PI, SI, VI, timers) | 16.43% | 15.26 | 243,912,898 | 5.87 | 9,682,886 | 5,402,878 | 75% |
| Maths (matrices, vectors, trig, random) | 15.32% | 14.22 | 326,201,233 | 4.09 | 5,494,029 | 6,176,937 | 48% |
| Animation | 7.60% | 7.06 | 176,283,402 | 3.76 | 3,508,523 | 1,488,543 | 44% |
| Collision | 4.85% | 4.50 | 82,199,573 | 5.13 | 2,419,783 | 2,852,197 | 67% |
| Actors | 4.46% | 4.14 | 70,845,704 | 5.48 | 2,778,810 | 1,222,889 | 61% |
| Scheduler, threads and system | 3.32% | 3.09 | 33,614,508 | 8.61 | 1,574,602 | 3,174,491 | 84% |
| Actor framework (z_actor.c) | 2.92% | 2.71 | 28,104,687 | 9.05 | 1,849,979 | 1,966,195 | 83% |
| Sky, weather and lights | 2.58% | 2.39 | 38,561,285 | 5.82 | 1,192,029 | 1,670,786 | 68% |
| Loading (DMA, decompression, overlays) | 2.46% | 2.28 | 91,313,685 | 2.34 | 1,395,361 | 171,681 | 46% |
| Link | 2.14% | 1.98 | 19,469,760 | 9.55 | 741,999 | 2,294,290 | 84% |
| C library (memory copies, printf, 64-bit maths) | 1.80% | 1.67 | 16,364,580 | 9.55 | 1,744,207 | 212,331 | 79% |
| Fishing mod | 1.68% | 1.56 | 30,299,104 | 4.84 | 312,357 | 1,044,020 | 57% |
| HUD and map | 1.49% | 1.38 | 11,549,599 | 11.22 | 694,706 | 1,403,345 | 87% |
| Camera and view | 1.26% | 1.17 | 10,359,448 | 10.55 | 592,658 | 1,134,077 | 83% |
| Title, file select, menus | 1.12% | 1.04 | 20,400,692 | 4.79 | 783,697 | 243,955 | 68% |
| Game loop (game.c, z_play.c) | 0.91% | 0.85 | 5,374,591 | 14.81 | 508,189 | 873,666 | 91% |
| Cutscenes | 0.85% | 0.79 | 9,588,519 | 7.76 | 654,571 | 447,136 | 82% |
| Text | 0.81% | 0.75 | 12,226,873 | 5.77 | 592,979 | 225,589 | 69% |
| Display list setup | 0.73% | 0.68 | 5,804,848 | 10.95 | 346,546 | 682,544 | 86% |
| Other (no source) | 0.44% | 0.41 | 18,211,949 | 2.12 | 123,428 | 92,605 | 27% |
| Effects | 0.44% | 0.40 | 3,747,558 | 10.11 | 378,145 | 210,502 | 87% |
| Scenes, rooms and objects | 0.41% | 0.38 | 6,215,123 | 5.77 | 204,809 | 272,572 | 74% |
| Pause menu and screen effects | 0.16% | 0.15 | 6,359,379 | 2.19 | 70,152 | 38,120 | 44% |
| Memory | 0.15% | 0.14 | 3,975,526 | 3.20 | 104,150 | 26,804 | 59% |
| Screen transitions | 0.03% | 0.03 | 139,280 | 18.55 | 8,978 | 39,940 | 92% |
| Save data and inventory | 0.00% | 0.00 | 138,513 | 2.44 | 1,779 | 1,645 | 50% |
| Other engine code | 0.00% | 0.00 | 47,579 | 4.31 | 2,253 | 110 | 71% |

## Audio: 25.63%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| mixing (synthesis.c) | 10.30% | 40.2% | 5,755,453 | 2,778,920 |
| sequence player | 4.96% | 19.3% | 3,445,876 | 1,106,074 |
| note playback | 3.36% | 13.1% | 2,413,510 | 599,614 |
| note effects | 2.51% | 9.8% | 1,992,204 | 625,414 |
| loading (fonts, samples, sequences) | 1.89% | 7.4% | 1,950,558 | 713,905 |
| game side (sound effects, music commands) | 1.22% | 4.8% | 648,201 | 1,002,368 |
| thread and tasks | 0.89% | 3.5% | 596,260 | 779,352 |
| manager | 0.37% | 1.4% | 299,833 | 264,387 |
| heap and caches | 0.14% | 0.5% | 78,817 | 31,264 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `AudioSynth_ProcessNote` | 3.96% | 15.4% | 317,038 | 1,087 | 381 | 2,271,748 | 916,669 | src/audio/internal/synthesis.c |
| `AudioSeq_SequenceChannelProcessScript` | 3.29% | 12.8% | 475,708 | 602 | 249 | 2,210,949 | 494,026 | src/audio/internal/seqplayer.c |
| `AudioSynth_DoOneAudioUpdate` | 2.37% | 9.2% | 40,962 | 5,035 | 1,746 | 1,017,002 | 612,186 | src/audio/internal/synthesis.c |
| `Audio_ProcessNotes` | 1.88% | 7.3% | 40,962 | 3,985 | 954 | 1,744,994 | 278,190 | src/audio/internal/playback.c |
| `Audio_InitSampleState` | 1.42% | 5.5% | 317,113 | 389 | 141 | 626,399 | 265,853 | src/audio/internal/playback.c |
| `func_800DB03C` | 0.96% | 3.7% | 40,962 | 2,036 | 453 | 1,070,063 | 83,102 | src/audio/internal/synthesis.c |
| `AudioSynth_InitNextRingBuf` | 0.94% | 3.7% | 81,937 | 996 | 256 | 601,407 | 223,023 | src/audio/internal/synthesis.c |
| `AudioSynth_ProcessEnvelope` | 0.86% | 3.4% | 317,037 | 237 | 110 | 249,436 | 204,958 | src/audio/internal/synthesis.c |
| `AudioSeq_SequencePlayerProcessSequence` | 0.76% | 3.0% | 82,050 | 807 | 135 | 795,587 | 186,730 | src/audio/internal/seqplayer.c |
| `AudioLoad_DecreaseSampleDmaTtls` | 0.73% | 2.9% | 13,685 | 4,676 | 690 | 1,026,928 | 87,751 | src/audio/internal/load.c |
| `AudioThread_UpdateImpl` | 0.70% | 2.7% | 13,691 | 4,460 | 288 | 502,430 | 579,859 | src/audio/internal/thread.c |
| `AudioSynth_Update` | 0.68% | 2.6% | 13,654 | 4,328 | 689 | 242,830 | 453,213 | src/audio/internal/synthesis.c |
| `Audio_SequenceChannelProcessSound` | 0.61% | 2.4% | 905,638 | 59 | 20 | 483,158 | 135,083 | src/audio/internal/effects.c |
| `Audio_AdsrUpdate` | 0.57% | 2.2% | 317,111 | 158 | 43 | 550,760 | 144,381 | src/audio/internal/effects.c |
| `Audio_NoteVibratoUpdate` | 0.50% | 2.0% | 317,112 | 138 | 16 | 643,644 | 47,238 | src/audio/internal/effects.c |
| `AudioSeq_HandleScriptFlowControl` | 0.47% | 1.8% | 1,408,064 | 29 | 17 | 146,300 | 145,029 | src/audio/internal/seqplayer.c |
| `Audio_GetVibratoFreqScale` | 0.43% | 1.7% | 317,111 | 119 | 37 | 145,097 | 179,153 | src/audio/internal/effects.c |
| `Audio_SequencePlayerProcessSound` | 0.38% | 1.5% | 82,051 | 402 | 208 | 158,538 | 112,213 | src/audio/internal/effects.c |
| `AudioLoad_FindSampleDmaChecked` | 0.36% | 1.4% | 14,457 | 2,177 | 614 | 308,922 | 18,927 | src/audio/internal/load.c |
| `Audio_ChooseActiveSfx` | 0.31% | 1.2% | 34,685 | 780 | 198 | 135,978 | 217,487 | src/audio/game/sfx.c |
| `AudioMgr_HandleRetrace.part.0` | 0.29% | 1.1% | 13,684 | 1,855 | 125 | 267,575 | 175,203 | src/code/audio_thread_manager.c |
| `AudioSeq_ProcessSequences` | 0.22% | 0.9% | 40,962 | 467 | 86 | 167,614 | 151,316 | src/audio/internal/seqplayer.c |
| `AudioLoad_ProcessAsyncLoads` | 0.19% | 0.8% | 13,684 | 1,227 | 117 | 248,286 | 99,018 | src/audio/internal/load.c |
| `AudioSynth_SaveReverbSamples` | 0.18% | 0.7% | 81,925 | 189 | 43 | 100,721 | 72,633 | src/audio/internal/synthesis.c |
| `AudioSynth_LoadRingBuffer1` | 0.17% | 0.7% | 76,721 | 197 | 35 | 115,031 | 66,931 | src/audio/internal/synthesis.c |
| (190 more functions) | 2.39% | 9.3% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `AudioSynth_ProcessNote` (src/audio/internal/synthesis.c): 974: 7%, 209,638 data fills; 1073: 7%, 352,698 data fills; 1048: 5%, 206,094 data fills; 1349: 5%, 96,829 data fills; 1121: 5%, 231,505 data fills
- `AudioSeq_SequenceChannelProcessScript` (src/audio/internal/seqplayer.c): 1313: 17%, 608,504 data fills; 1297: 7%, 340,201 data fills; 1286: 6%, 100,412 data fills; 1810: 6%, 328,127 data fills; 1811: 6%, 357,771 data fills
- `AudioSynth_DoOneAudioUpdate` (src/audio/internal/synthesis.c): 929: 22%; 845: 20%, 507,229 data fills; 862: 7%; 928: 4%, 98,552 data fills; 930: 4%, 52,534 data fills
- `Audio_ProcessNotes` (src/audio/internal/playback.c): 211: 17%, 425,400 data fills; 208: 10%, 316,656 data fills; 250: 7%, 193,715 data fills; 216: 5%, 171,346 data fills; 328: 5%
- `Audio_InitSampleState` (src/audio/internal/playback.c): 54: 12%; 51: 9%, 173,769 data fills; 127: 7%; 38: 7%; 48: 6%, 57,684 data fills
- `func_800DB03C` (src/audio/internal/synthesis.c): 213: 44%, 677,678 data fills; 210: 26%, 333,595 data fills; 207: 11%; 208: 6%, 26,427 data fills; 214: 4%, 15,778 data fills
- `AudioSynth_InitNextRingBuf` (src/audio/internal/synthesis.c): 119: 23%, 188,731 data fills; 120: 23%, 181,814 data fills; 147: 9%, 47,373 data fills; 135: 8%, 72,253 data fills; 118: 7%
- `AudioSynth_ProcessEnvelope` (src/audio/internal/synthesis.c): 1534: 22%, 67,670 data fills; 1490: 8%; 1485: 7%; 1530: 7%, 22,382 data fills; 1531: 6%, 52,709 data fills

## libultra (OS: threads, interrupts, PI, SI, VI, timers): 16.43%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| exceptasm.c | 7.14% | 43.5% | 6,871,136 | 1,942,798 |
| writebackdcacheall.c | 3.63% | 22.1% | 0 | 46,703 |
| recvmesg.c | 1.35% | 8.2% | 966,652 | 425,404 |
| sendmesg.c | 0.88% | 5.4% | 611,982 | 297,990 |
| interrupt.c | 0.55% | 3.3% | 110,587 | 359,676 |
| startthread.c | 0.30% | 1.8% | 241,989 | 101,158 |
| viswapcontext.c | 0.28% | 1.7% | 164,443 | 218,918 |
| contreaddata.c | 0.24% | 1.4% | 163,572 | 156,558 |
| sptask.c | 0.23% | 1.4% | 56,360 | 301,713 |
| setintmask.c | 0.17% | 1.0% | 44,664 | 64,113 |
| gettime.c | 0.17% | 1.0% | 86,403 | 116,892 |
| invaldcache.c | 0.16% | 1.0% | 0 | 153,068 |
| writebackdcache.c | 0.15% | 0.9% | 0 | 48,070 |
| controller.c | 0.14% | 0.9% | 81,941 | 95,369 |
| virtualtophysical.c | 0.11% | 0.6% | 0 | 147,090 |
| vimgr.c | 0.09% | 0.6% | 44,027 | 87,718 |
| sirawdma.c | 0.09% | 0.6% | 23,744 | 82,493 |
| contquery.c | 0.08% | 0.5% | 3,957 | 126,557 |
| siacs.c | 0.08% | 0.5% | 38,812 | 84,473 |
| sprawdma.c | 0.07% | 0.4% | 23,339 | 62,531 |
| sp.c | 0.06% | 0.4% | 0 | 46,678 |
| epirawdma.c | 0.05% | 0.3% | 20,366 | 36,062 |
| devmgr.c | 0.05% | 0.3% | 26,251 | 40,789 |
| timerintr.c | 0.04% | 0.2% | 14,833 | 44,986 |
| getcount.c | 0.03% | 0.2% | 0 | 35,026 |
| (53 more) | 0.29% | 1.7% | | |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `__osDispatchThread` | 3.73% | 22.7% | 410,782 | 791 | 88 | 4,584,275 | 422,327 | src/libultra/os/exceptasm.c |
| `osWritebackDCacheAll` | 3.63% | 22.1% | 23,346 | 13,519 | 2,054 | 0 | 46,703 | src/libultra/os/writebackdcacheall.c |
| `__osException` | 2.14% | 13.0% | 118,916 | 1,565 | 236 | 1,459,233 | 1,114,624 | src/libultra/os/exceptasm.c |
| `osRecvMesg` | 1.35% | 8.2% | 446,235 | 262 | 46 | 966,652 | 425,404 | src/libultra/os/recvmesg.c |
| `osSendMesg` | 0.88% | 5.4% | 270,983 | 284 | 48 | 611,982 | 297,990 | src/libultra/os/sendmesg.c |
| `__osEnqueueAndYield` | 0.82% | 5.0% | 291,865 | 246 | 60 | 533,210 | 316,835 | src/libultra/os/exceptasm.c |
| `__osDisableInt` | 0.44% | 2.7% | 1,026,231 | 37 | 15 | 110,587 | 303,722 | src/libultra/os/interrupt.c |
| `__osEnqueueThread` | 0.36% | 2.2% | 628,135 | 49 | 14 | 240,427 | 65,012 | src/libultra/os/exceptasm.c |
| `osStartThread` | 0.30% | 1.8% | 137,689 | 188 | 39 | 241,989 | 101,158 | src/libultra/os/startthread.c |
| `__osViSwapContext` | 0.28% | 1.7% | 13,716 | 1,775 | 129 | 164,443 | 218,918 | src/libultra/io/viswapcontext.c |
| `osSetIntMask` | 0.17% | 1.0% | 150,846 | 99 | 38 | 44,664 | 64,113 | src/libultra/os/setintmask.c |
| `osGetTime` | 0.17% | 1.0% | 118,284 | 125 | 25 | 86,403 | 116,892 | src/libultra/os/gettime.c |
| `osInvalDCache` | 0.16% | 1.0% | 68,455 | 209 | 96 | 0 | 153,068 | src/libultra/os/invaldcache.c |
| `osWritebackDCache` | 0.15% | 0.9% | 51,764 | 256 | 36 | 0 | 48,070 | src/libultra/os/writebackdcache.c |
| `osSpTaskLoad` | 0.11% | 0.7% | 23,339 | 427 | 56 | 33,021 | 147,186 | src/libultra/io/sptask.c |
| `__osRestoreInt` | 0.11% | 0.7% | 1,026,211 | 10 | 7 | 0 | 55,954 | src/libultra/os/interrupt.c |
| `osVirtualToPhysical` | 0.11% | 0.6% | 238,441 | 39 | 9 | 0 | 147,090 | src/libultra/os/virtualtophysical.c |
| `osContGetReadData` | 0.10% | 0.6% | 13,685 | 618 | 76 | 68,215 | 50,010 | src/libultra/io/contreaddata.c |
| `viMgrMain` | 0.09% | 0.6% | 1 | 8,086,558 | 562,288 | 44,022 | 87,704 | src/libultra/io/vimgr.c |
| `__osPopThread` | 0.09% | 0.6% | 654,018 | 12 | 4 | 53,991 | 24,000 | src/libultra/os/exceptasm.c |
| `__osSiRawStartDma` | 0.09% | 0.6% | 55,025 | 145 | 28 | 23,744 | 82,493 | src/libultra/io/sirawdma.c |
| `_VirtualToPhysicalTask` | 0.09% | 0.5% | 23,339 | 318 | 41 | 23,339 | 107,849 | src/libultra/io/sptask.c |
| `__osPackReadData` | 0.08% | 0.5% | 13,685 | 490 | 99 | 54,740 | 45,707 | src/libultra/io/contreaddata.c |
| `__osContGetInitData` | 0.08% | 0.5% | 13,686 | 479 | 94 | 54,387 | 43,733 | src/libultra/io/controller.c |
| `osContStartQuery` | 0.07% | 0.4% | 13,685 | 447 | 33 | 3,955 | 109,557 | src/libultra/io/contquery.c |
| (73 more functions) | 0.84% | 5.1% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `osRecvMesg` (src/libultra/os/recvmesg.c): 30: 21%, 270,614 data fills; 17: 16%, 247,353 data fills; 20: 15%; 3: 15%, 125,723 data fills; 6: 13%, 218,459 data fills
- `osSendMesg` (src/libultra/os/sendmesg.c): 3: 23%, 151,975 data fills; 17: 17%; 7: 15%, 139,955 data fills; 18: 13%, 146,696 data fills; 28: 11%, 47,608 data fills

## Maths (matrices, vectors, trig, random): 15.32%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| sys_matrix.c | 9.35% | 61.1% | 4,180,779 | 3,367,326 |
| sinf.c | 1.36% | 8.9% | 141,227 | 127,650 |
| cosf.c | 1.36% | 8.9% | 98,411 | 867,275 |
| z_lib.c | 1.06% | 6.9% | 536,869 | 422,210 |
| sys_math_atan.c | 0.50% | 3.2% | 87,538 | 200,041 |
| mtxutil.c | 0.47% | 3.1% | 238,645 | 157,171 |
| lookathil.c | 0.35% | 2.3% | 100,862 | 322,832 |
| lookat.c | 0.24% | 1.5% | 5,280 | 257,402 |
| perspective.c | 0.22% | 1.4% | 77,694 | 161,487 |
| qrand.c | 0.17% | 1.1% | 9,534 | 65,076 |
| ortho.c | 0.11% | 0.7% | 7,115 | 100,618 |
| z_olib.c | 0.09% | 0.6% | 10,075 | 111,995 |
| sqrtf.c | 0.04% | 0.2% | 0 | 12,698 |
| us2dex.c | 0.00% | 0.0% | 0 | 3,156 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Matrix_MtxFToMtx` | 2.69% | 17.6% | 336,821 | 696 | 151 | 1,510,299 | 890,158 | src/code/sys_matrix.c |
| `Matrix_TranslateRotateZYX` | 1.51% | 9.9% | 260,117 | 506 | 165 | 173,771 | 555,287 | src/code/sys_matrix.c |
| `sinf` | 1.36% | 8.9% | 617,847 | 191 | 70 | 141,227 | 127,650 | src/libultra/gu/sinf.c |
| `cosf` | 1.36% | 8.9% | 388,919 | 304 | 79 | 98,411 | 867,275 | src/libultra/gu/cosf.c |
| `Matrix_MultVec3f` | 1.18% | 7.7% | 441,681 | 233 | 54 | 448,142 | 219,397 | src/code/sys_matrix.c |
| `Matrix_RotateY` | 1.09% | 7.1% | 256,846 | 369 | 54 | 1,031,136 | 54,405 | src/code/sys_matrix.c |
| `Matrix_RotateZ` | 0.71% | 4.6% | 122,196 | 507 | 92 | 10,783 | 751,497 | src/code/sys_matrix.c |
| `Matrix_Scale` | 0.59% | 3.9% | 205,945 | 251 | 68 | 151,531 | 199,161 | src/code/sys_matrix.c |
| `Matrix_Push` | 0.59% | 3.8% | 254,199 | 201 | 41 | 460,342 | 62,763 | src/code/sys_matrix.c |
| `Math_Atan2S` | 0.43% | 2.8% | 341,535 | 109 | 35 | 84,433 | 196,978 | src/code/sys_math_atan.c |
| `guMtxF2L` | 0.35% | 2.3% | 43,351 | 697 | 174 | 177,812 | 115,652 | src/libultra/gu/mtxutil.c |
| `guLookAtHiliteF` | 0.31% | 2.0% | 6,097 | 4,383 | 495 | 68,837 | 297,931 | src/libultra/gu/lookathil.c |
| `Matrix_SetTranslateRotateYXZ` | 0.24% | 1.6% | 34,638 | 606 | 38 | 226,291 | 54,736 | src/code/sys_matrix.c |
| `guLookAtF` | 0.21% | 1.3% | 13,991 | 1,278 | 181 | 276 | 219,431 | src/libultra/gu/lookat.c |
| `Math_SinS` | 0.20% | 1.3% | 122,445 | 144 | 37 | 64,532 | 110,902 | src/code/z_lib.c |
| `guPerspectiveF` | 0.19% | 1.2% | 13,991 | 1,189 | 156 | 64,186 | 133,016 | src/libultra/gu/perspective.c |
| `Matrix_RotateX` | 0.17% | 1.1% | 179,671 | 80 | 31 | 3,702 | 91,874 | src/code/sys_matrix.c |
| `Math_Vec3s_ToVec3f` | 0.15% | 1.0% | 259,951 | 52 | 16 | 80,271 | 5,363 | src/code/z_lib.c |
| `Math_CosS` | 0.15% | 1.0% | 110,555 | 118 | 30 | 31,848 | 108,437 | src/code/z_lib.c |
| `Math_Vec3f_Copy` | 0.15% | 1.0% | 104,914 | 121 | 7 | 174,437 | 18,979 | src/code/z_lib.c |
| `guMtxIdentF` | 0.13% | 0.8% | 43,351 | 254 | 106 | 60,833 | 41,519 | src/libultra/gu/mtxutil.c |
| `Math_SmoothStepToF` | 0.11% | 0.7% | 258,345 | 38 | 11 | 79,482 | 18,110 | src/code/z_lib.c |
| `Matrix_Translate` | 0.11% | 0.7% | 177,526 | 55 | 15 | 18,474 | 92,755 | src/code/sys_matrix.c |
| `Math_ApproachZeroF` | 0.11% | 0.7% | 222,553 | 42 | 15 | 63,539 | 2,857 | src/code/z_lib.c |
| `Matrix_MtxToMtxF` | 0.11% | 0.7% | 8,442 | 1,091 | 162 | 38,275 | 88,717 | src/code/sys_matrix.c |
| (53 more functions) | 1.13% | 7.4% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Matrix_MtxFToMtx` (src/code/sys_matrix.c): 602: 25%, 691,258 data fills; 600: 25%, 643,626 data fills; 606: 10%, 144,201 data fills; 603: 8%; 604: 8%
- `Matrix_TranslateRotateZYX` (src/code/sys_matrix.c): 416: 10%; 417: 9%; 418: 9%; 410: 8%, 50,865 data fills; 400: 6%, 83,373 data fills
- `sinf` (src/libultra/gu/sinf.c): 55: 26%; 48: 11%; 56: 10%; 52: 8%; 51: 8%
- `cosf` (src/libultra/gu/cosf.c): 49: 28%; 51: 12%; 44: 11%; 45: 10%; 39: 7%
- `Matrix_MultVec3f` (src/code/sys_matrix.c): 641: 51%, 408,664 data fills; 642: 22%; 643: 20%; 639: 4%; 644: 3%, 20,285 data fills
- `Matrix_RotateY` (src/code/sys_matrix.c): 241: 32%, 325,308 data fills; 179: 19%, 329,667 data fills; 226: 8%, 92,995 data fills; 231: 8%, 93,050 data fills; 229: 8%, 92,995 data fills
- `Matrix_RotateZ` (src/code/sys_matrix.c): 251: 20%; 269: 12%; 259: 12%; 268: 10%; 267: 10%
- `Matrix_Scale` (src/code/sys_matrix.c): 94: 13%, 47,351 data fills; 102: 13%, 26,264 data fills; 113: 12%, 49,658 data fills; 98: 8%, 27,218 data fills; 107: 7%

## Animation: 7.60%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_skin_matrix.c | 3.70% | 48.7% | 1,012,657 | 585,813 |
| z_skelanime.c | 2.10% | 27.6% | 1,330,540 | 805,363 |
| z_skin.c | 1.57% | 20.6% | 968,273 | 72,574 |
| z_skin_awb.c | 0.24% | 3.1% | 197,053 | 24,793 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `SkinMatrix_MtxFMtxFMult` | 1.73% | 22.8% | 172,453 | 875 | 273 | 216,099 | 263,259 | src/code/z_skin_matrix.c |
| `SkinMatrix_Vec3fMtxFMultXYZ` | 0.95% | 12.6% | 543,494 | 153 | 52 | 184,332 | 44,013 | src/code/z_skin_matrix.c |
| `Skin_UpdateVertices` | 0.94% | 12.4% | 98,941 | 828 | 239 | 585,039 | 13,012 | src/code/z_skin.c |
| `SkinMatrix_Vec3fMtxFMultXYZW` | 0.61% | 8.0% | 129,415 | 407 | 69 | 360,228 | 171,716 | src/code/z_skin_matrix.c |
| `Skin_ApplyLimbModifications` | 0.53% | 6.9% | 781 | 58,863 | 17,778 | 304,555 | 26,604 | src/code/z_skin.c |
| `SkelAnime_DrawLimb` | 0.53% | 6.9% | 115,076 | 399 | 120 | 324,969 | 109,947 | src/code/z_skelanime.c |
| `SkelAnime_DrawFlexLimbLod` | 0.47% | 6.2% | 84,420 | 489 | 139 | 276,831 | 122,350 | src/code/z_skelanime.c |
| `SkelAnime_GetFrameData` | 0.40% | 5.3% | 13,762 | 2,560 | 619 | 324,974 | 67,780 | src/code/z_skelanime.c |
| `SkinMatrix_SetRotateZYX` | 0.18% | 2.4% | 40,062 | 390 | 53 | 162,582 | 27,905 | src/code/z_skin_matrix.c |
| `SkinMatrix_SetTranslate` | 0.15% | 2.0% | 169,774 | 78 | 19 | 71,788 | 46,297 | src/code/z_skin_matrix.c |
| `func_800A698C` | 0.14% | 1.8% | 37,870 | 316 | 64 | 117,401 | 9,917 | src/code/z_skin_awb.c |
| `SkelAnime_Draw` | 0.13% | 1.7% | 8,852 | 1,264 | 101 | 65,871 | 122,691 | src/code/z_skelanime.c |
| `SkelAnime_DrawFlexLimbOpa` | 0.10% | 1.3% | 14,283 | 611 | 139 | 77,694 | 10,282 | src/code/z_skelanime.c |
| `Skin_ApplyAnimTransformations` | 0.09% | 1.2% | 781 | 10,475 | 1,685 | 74,725 | 14,766 | src/code/z_skin_awb.c |
| `SkelAnime_DrawFlexLod` | 0.08% | 1.1% | 4,222 | 1,649 | 129 | 30,879 | 88,936 | src/code/z_skelanime.c |
| `SkelAnime_DrawFlexLimb` | 0.07% | 0.9% | 10,591 | 556 | 146 | 36,314 | 21,189 | src/code/z_skelanime.c |
| `Skin_DrawImpl` | 0.06% | 0.8% | 781 | 6,369 | 1,253 | 50,132 | 15,957 | src/code/z_skin.c |
| `SkelAnime_LoopFull` | 0.04% | 0.6% | 12,246 | 311 | 33 | 30,198 | 24,947 | src/code/z_skelanime.c |
| `AnimTaskQueue_AddLoadPlayerFrame` | 0.04% | 0.6% | 4,243 | 859 | 71 | 27,716 | 38,169 | src/code/z_skelanime.c |
| `SkelAnime_AnimateFrame` | 0.04% | 0.5% | 13,179 | 274 | 22 | 27,108 | 29,117 | src/code/z_skelanime.c |
| `SkinMatrix_MtxFCopy` | 0.03% | 0.4% | 37,870 | 73 | 33 | 2,110 | 4,180 | src/code/z_skin_matrix.c |
| `Skin_DrawLimb` | 0.03% | 0.4% | 8,973 | 300 | 72 | 24,947 | 7,119 | src/code/z_skin.c |
| `SkelAnime_UpdateTranslation` | 0.02% | 0.3% | 1,802 | 1,207 | 99 | 10,241 | 26,968 | src/code/z_skelanime.c |
| `AnimTaskQueue_Update` | 0.02% | 0.3% | 4,226 | 514 | 47 | 14,325 | 23,826 | src/code/z_skelanime.c |
| `LinkAnimation_AnimateFrame` | 0.02% | 0.3% | 4,211 | 451 | 25 | 15,581 | 21,439 | src/code/z_skelanime.c |
| (51 more functions) | 0.18% | 2.3% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `SkinMatrix_MtxFMtxFMult` (src/code/z_skin_matrix.c): 88: 7%, 43,658 data fills; 100: 7%, 43,632 data fills; 106: 7%, 43,026 data fills; 94: 7%, 42,169 data fills; 148: 6%
- `SkinMatrix_Vec3fMtxFMultXYZ` (src/code/z_skin_matrix.c): 55: 40%, 152,080 data fills; 60: 30%; 65: 28%; 66: 2%
- `Skin_UpdateVertices` (src/code/z_skin.c): 32: 23%, 317,389 data fills; 42: 14%, 119,612 data fills; 44: 8%, 49,617 data fills; 23: 6%; 30: 6%, 54,996 data fills
- `SkinMatrix_Vec3fMtxFMultXYZW` (src/code/z_skin_matrix.c): 35: 47%, 277,344 data fills; 36: 22%, 75,774 data fills; 38: 15%; 37: 14%; 39: 2%
- `Skin_ApplyLimbModifications` (src/code/z_skin.c): 117: 18%, 80,286 data fills; 85: 15%, 103,868 data fills; 119: 13%, 59,999 data fills; 121: 5%; 125: 5%
- `SkelAnime_DrawLimb` (src/code/z_skelanime.c): 537: 15%, 81,498 data fills; 545: 11%, 36,338 data fills; 526: 9%; 539: 8%, 43,169 data fills; 568: 8%, 16,552 data fills
- `SkelAnime_DrawFlexLimbLod` (src/code/z_skelanime.c): 162: 13%, 75,476 data fills; 156: 10%, 39,089 data fills; 144: 8%; 170: 8%, 32,700 data fills; 171: 7%, 30,036 data fills
- `SkelAnime_GetFrameData` (src/code/z_skelanime.c): 503: 21%, 77,062 data fills; 501: 19%, 58,650 data fills; 499: 19%, 64,857 data fills; 491: 11%, 34,682 data fills; 500: 8%, 30,219 data fills

## Collision: 4.85%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| floors, walls, ceilings (z_bgcheck.c) | 3.77% | 77.9% | 2,003,764 | 2,456,002 |
| colliders (z_collision_check.c) | 0.59% | 12.1% | 361,220 | 281,094 |
| geometry tests (sys_math3d.c) | 0.49% | 10.1% | 54,799 | 115,101 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `BgCheck_RaycastDownImpl` | 2.01% | 41.4% | 27,532 | 6,344 | 991 | 1,068,871 | 1,332,937 | src/code/z_bgcheck.c |
| `DynaPoly_UpdateContext` | 0.30% | 6.1% | 4,222 | 6,118 | 408 | 305,972 | 76,796 | src/code/z_bgcheck.c |
| `DynaPoly_AddBgActorToLookup` | 0.23% | 4.8% | 3,728 | 5,392 | 1,070 | 163,444 | 68,037 | src/code/z_bgcheck.c |
| `Math3D_CylVsCylOverlapCenterDist` | 0.22% | 4.6% | 93,723 | 205 | 77 | 42,979 | 6,729 | src/code/sys_math3d.c |
| `StaticLookup_AddPolyToSSList` | 0.09% | 1.9% | 15,000 | 535 | 207 | 62,291 | 18,798 | src/code/z_bgcheck.c |
| `BgCheck_PolyIntersectsSubdivision` | 0.09% | 1.9% | 17,205 | 465 | 126 | 9,571 | 80,778 | src/code/z_bgcheck.c |
| `CollisionCheck_ResetDamage` | 0.09% | 1.8% | 72,431 | 106 | 5 | 105,048 | 11,625 | src/code/z_collision_check.c |
| `CollisionCheck_OC` | 0.08% | 1.7% | 4,221 | 1,693 | 765 | 26,744 | 32,927 | src/code/z_collision_check.c |
| `CollisionCheck_OC_CylVsCyl` | 0.07% | 1.4% | 93,694 | 62 | 31 | 24,175 | 3,190 | src/code/z_collision_check.c |
| `BgCheck_CheckWallImpl` | 0.07% | 1.4% | 2,672 | 2,155 | 158 | 35,970 | 63,843 | src/code/z_bgcheck.c |
| `BgCheck_RaycastDownDynaList` | 0.06% | 1.3% | 1,071 | 5,197 | 609 | 38,553 | 43,911 | src/code/z_bgcheck.c |
| `BgCheck_GetWaterSurface` | 0.06% | 1.3% | 9,460 | 576 | 61 | 31,732 | 59,512 | src/code/z_bgcheck.c |
| `BgCheck_SphVsStaticWall` | 0.06% | 1.2% | 2,672 | 1,967 | 174 | 25,381 | 56,260 | src/code/z_bgcheck.c |
| `BgCheck_InitializeStaticLookup` | 0.06% | 1.2% | 5 | 1,043,895 | 279,392 | 11,489 | 46,232 | src/code/z_bgcheck.c |
| `CollisionCheck_SetAC` | 0.05% | 1.1% | 13,684 | 338 | 43 | 21,652 | 56,819 | src/code/z_collision_check.c |
| `CollisionPoly_GetMinY` | 0.05% | 1.1% | 18,507 | 247 | 35 | 1,461 | 77,625 | src/code/z_bgcheck.c |
| `CollisionCheck_SetOC` | 0.05% | 1.0% | 15,132 | 272 | 43 | 27,912 | 33,693 | src/code/z_collision_check.c |
| `DynaPoly_UnsetAllInteractFlags` | 0.05% | 0.9% | 46,526 | 86 | 19 | 18,990 | 37,817 | src/code/z_bgcheck.c |
| `CollisionPoly_CheckYIntersectApprox1` | 0.05% | 0.9% | 21,420 | 185 | 83 | 5,355 | 11,781 | src/code/z_bgcheck.c |
| `BgCheck_Allocate` | 0.05% | 0.9% | 5 | 789,168 | 275,844 | 27,087 | 572 | src/code/z_bgcheck.c |
| `BgCheck_EntityRaycastDown5` | 0.04% | 0.9% | 12,555 | 305 | 18 | 19,767 | 48,138 | src/code/z_bgcheck.c |
| `CollisionCheck_Incompatible` | 0.04% | 0.9% | 94,464 | 38 | 21 | 16,020 | 3,665 | src/code/z_collision_check.c |
| `BgCheck_SubWalkList.isra.0` | 0.04% | 0.8% | 192 | 16,821 | 8,260 | 2,794 | 934 | src/code/z_bgcheck.c |
| `BgCheck_SphVsDynaWall` | 0.04% | 0.7% | 2,672 | 1,174 | 97 | 17,011 | 32,220 | src/code/z_bgcheck.c |
| `Math3D_TriLineIntersect` | 0.03% | 0.7% | 17,877 | 167 | 83 | 648 | 862 | src/code/sys_math3d.c |
| (121 more functions) | 0.88% | 18.1% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `BgCheck_RaycastDownImpl` (src/code/z_bgcheck.c): 3199: 93%, 991,810 data fills; 3198: 4%, 44,821 data fills; 3220: 2%, 12,308 data fills; 3202: 1%, 19,716 data fills; 1243: 0%
- `DynaPoly_UpdateContext` (src/code/z_bgcheck.c): 4647: 55%, 206,827 data fills; 4648: 18%, 57,038 data fills; 4638: 3%, 8,314 data fills; 4711: 2%, 3,520 data fills; 4651: 2%, 2,166 data fills
- `DynaPoly_AddBgActorToLookup` (src/code/z_bgcheck.c): 4484: 29%, 77,249 data fills; 4500: 10%, 11,557 data fills; 4489: 8%, 10,842 data fills; 4451: 7%, 16,726 data fills; 4449: 4%, 7,771 data fills
- `Math3D_CylVsCylOverlapCenterDist` (src/code/sys_math3d.c): 2007: 32%; 1993: 9%; 1998: 9%, 9,703 data fills; 2022: 7%; 2003: 7%, 7,120 data fills

## Actors: 4.46%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| ovl_Object_Kankyo | 2.15% | 48.2% | 1,527,253 | 233,829 |
| ovl_En_Ganon_Mant | 1.01% | 22.6% | 497,270 | 73,891 |
| ovl_En_Elf | 0.66% | 14.9% | 344,645 | 539,760 |
| ovl_En_Kanban | 0.21% | 4.6% | 157,316 | 37,395 |
| ovl_En_Ko | 0.09% | 1.9% | 49,014 | 64,168 |
| ovl_En_Mag | 0.05% | 1.2% | 32,940 | 28,067 |
| horses | 0.05% | 1.2% | 25,397 | 51,213 |
| ovl_Bg_Spot00_Hanebasi | 0.05% | 1.0% | 24,096 | 40,078 |
| ovl_En_Viewer | 0.04% | 1.0% | 21,029 | 37,420 |
| ovl_En_Md | 0.03% | 0.7% | 14,742 | 29,205 |
| ovl_Obj_Mure2 | 0.03% | 0.6% | 22,381 | 7,944 |
| ovl_En_Holl | 0.03% | 0.6% | 14,781 | 22,183 |
| ovl_Bg_Treemouth | 0.02% | 0.4% | 11,135 | 19,667 |
| ovl_En_Kusa | 0.02% | 0.4% | 13,720 | 8,820 |
| ovl_En_River_Sound | 0.02% | 0.3% | 8,678 | 13,870 |
| ovl_En_Wood02 | 0.01% | 0.3% | 10,008 | 8,673 |
| ovl_Item_Shield | 0.01% | 0.1% | 4,071 | 6,137 |
| ovl_Obj_Hana | 0.00% | 0.0% | 314 | 523 |
| ovl_Obj_Comb | 0.00% | 0.0% | 20 | 46 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `ObjectKankyo_Fairies` | 1.15% | 25.8% | 1,844 | 54,340 | 12,159 | 623,954 | 156,399 | src/overlays/actors/ovl_Object_Kankyo/z_object_kankyo.c |
| `ObjectKankyo_DrawFairies.part.0` | 0.99% | 22.1% | 1,845 | 46,592 | 7,567 | 895,902 | 66,355 | src/overlays/actors/ovl_Object_Kankyo/z_object_kankyo.c |
| `EnGanonMant_UpdateStrand` | 0.77% | 17.2% | 6,744 | 9,890 | 2,277 | 308,893 | 42,136 | src/overlays/actors/ovl_En_Ganon_Mant/z_en_ganon_mant.c |
| `EnGanonMant_UpdateVertices` | 0.19% | 4.3% | 562 | 29,978 | 5,748 | 157,567 | 7,416 | src/overlays/actors/ovl_En_Ganon_Mant/z_en_ganon_mant.c |
| `EnElf_Draw` | 0.17% | 3.8% | 8,852 | 1,661 | 158 | 120,578 | 102,666 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `EnKanban_Draw` | 0.12% | 2.7% | 2,144 | 4,833 | 1,430 | 87,295 | 18,794 | src/overlays/actors/ovl_En_Kanban/z_en_kanban.c |
| `EnElf_OverrideLimbDraw` | 0.10% | 2.2% | 123,929 | 69 | 20 | 44,778 | 43,458 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `EnKanban_Update` | 0.07% | 1.5% | 8,568 | 701 | 126 | 57,985 | 14,302 | src/overlays/actors/ovl_En_Kanban/z_en_kanban.c |
| `func_80A02C98` | 0.06% | 1.3% | 6,405 | 788 | 86 | 11,303 | 73,229 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `func_80A03CF8` | 0.06% | 1.3% | 3,947 | 1,256 | 92 | 21,323 | 63,534 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `func_80A053F0` | 0.05% | 1.1% | 3,947 | 1,106 | 74 | 28,768 | 47,877 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `func_80A02A20` | 0.04% | 0.9% | 8,852 | 381 | 34 | 27,562 | 23,181 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `EnGanonMant_Draw` | 0.04% | 0.8% | 562 | 5,817 | 731 | 24,659 | 16,828 | src/overlays/actors/ovl_En_Ganon_Mant/z_en_ganon_mant.c |
| `func_80A0353C` | 0.04% | 0.8% | 4,904 | 648 | 45 | 23,844 | 25,863 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `func_80A0461C` | 0.03% | 0.7% | 3,947 | 671 | 30 | 21,020 | 23,148 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `EnElf_GetCuePos` | 0.03% | 0.7% | 3,383 | 769 | 61 | 12,071 | 27,064 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `func_80A04414` | 0.03% | 0.6% | 3,947 | 600 | 31 | 11,891 | 31,624 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `EnElf_UpdateLights` | 0.03% | 0.6% | 3,947 | 599 | 70 | 167 | 39,184 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `EnKo_OverrideLimbDraw` | 0.02% | 0.6% | 9,660 | 223 | 43 | 14,562 | 11,316 | src/overlays/actors/ovl_En_Ko/z_en_ko.c |
| `EnKo_Update` | 0.02% | 0.5% | 1,210 | 1,622 | 139 | 14,826 | 18,553 | src/overlays/actors/ovl_En_Ko/z_en_ko.c |
| `EnMag_DrawInner` | 0.02% | 0.5% | 272 | 7,072 | 1,565 | 8,906 | 16,445 | src/overlays/actors/ovl_En_Mag/z_en_mag.c |
| `EnHoll_HorizontalInvisible` | 0.02% | 0.5% | 1,843 | 1,018 | 49 | 12,923 | 18,478 | src/overlays/actors/ovl_En_Holl/z_en_holl.c |
| `EnElf_Update` | 0.02% | 0.4% | 4,904 | 326 | 21 | 11,116 | 15,325 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `func_80B9A534` | 0.01% | 0.3% | 1,906 | 633 | 129 | 10,917 | 2,828 | src/overlays/actors/ovl_Obj_Mure2/z_obj_mure2.c |
| `EnKanban_Message` | 0.01% | 0.3% | 8,568 | 140 | 28 | 10,487 | 4,006 | src/overlays/actors/ovl_En_Kanban/z_en_kanban.c |
| (202 more functions) | 0.38% | 8.5% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `ObjectKankyo_Fairies` (src/overlays/actors/ovl_Object_Kankyo/z_object_kankyo.c): 272: 15%, 150,331 data fills; 307: 8%, 116,135 data fills; 262: 7%; 351: 5%, 58,653 data fills; 299: 4%, 58,975 data fills
- `ObjectKankyo_DrawFairies.part.0` (src/overlays/actors/ovl_Object_Kankyo/z_object_kankyo.c): 533: 31%, 341,785 data fills; 538: 14%, 115,489 data fills; 596: 7%, 80,876 data fills; 597: 6%, 64,726 data fills; 580: 5%, 61,726 data fills
- `EnGanonMant_UpdateStrand` (src/overlays/actors/ovl_En_Ganon_Mant/z_en_ganon_mant.c): 284: 8%; 242: 7%; 248: 7%; 261: 6%; 299: 5%

## Scheduler, threads and system: 3.32%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| sched.c | 1.17% | 35.1% | 568,698 | 1,220,796 |
| padmgr.c | 0.70% | 21.2% | 289,316 | 687,135 |
| graph.c | 0.38% | 11.3% | 235,987 | 347,228 |
| irqmgr.c | 0.20% | 6.0% | 160,967 | 136,987 |
| sys_rumble.c | 0.20% | 6.0% | 91,380 | 141,167 |
| fault_n64.c | 0.18% | 5.4% | 106,495 | 112,187 |
| pad.c | 0.14% | 4.2% | 13,697 | 181,562 |
| math64.c | 0.12% | 3.7% | 18,264 | 120,557 |
| padsetup.c | 0.10% | 3.0% | 25,244 | 75,907 |
| viconfig.c | 0.03% | 1.0% | 27,375 | 30,641 |
| z_rumble.c | 0.03% | 0.9% | 4,892 | 41,693 |
| z_debug.c | 0.03% | 0.8% | 9,182 | 30,011 |
| main.c | 0.03% | 0.8% | 13,460 | 27,409 |
| z_debug_display.c | 0.02% | 0.5% | 8,439 | 21,109 |
| stackcheck.c | 0.00% | 0.0% | 1,170 | 35 |
| cic6105.c | 0.00% | 0.0% | 10 | 15 |
| driverominit.c | 0.00% | 0.0% | 4 | 15 |
| boot_main.c | 0.00% | 0.0% | 6 | 10 |
| code_n64dd_800AD410.c | 0.00% | 0.0% | 3 | 7 |
| idle.c | 0.00% | 0.0% | 3 | 6 |
| z_nulltask.c | 0.00% | 0.0% | 4 | 5 |
| z_prenmi_buff.c | 0.00% | 0.0% | 4 | 2 |
| runtime.c | 0.00% | 0.0% | 1 | 6 |
| z_n64dd.c | 0.00% | 0.0% | 1 | 1 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `PadMgr_UpdateInputs` | 0.24% | 7.3% | 13,686 | 1,542 | 259 | 100,815 | 205,750 | src/code/padmgr.c |
| `Sched_HandleRSPDone` | 0.24% | 7.2% | 23,339 | 893 | 66 | 121,298 | 239,066 | src/code/sched.c |
| `PadMgr_HandleRetrace` | 0.22% | 6.6% | 13,685 | 1,401 | 133 | 92,578 | 240,463 | src/code/padmgr.c |
| `RumbleMgr_Update` | 0.20% | 6.0% | 13,461 | 1,290 | 403 | 91,380 | 141,141 | src/code/sys_rumble.c |
| `Sched_Schedule` | 0.19% | 5.7% | 55,688 | 298 | 27 | 89,589 | 193,271 | src/code/sched.c |
| `Sched_RunTask` | 0.17% | 5.3% | 23,445 | 650 | 47 | 108,816 | 188,800 | src/code/sched.c |
| `IrqMgr_HandleRetrace` | 0.16% | 4.7% | 13,687 | 997 | 61 | 147,272 | 82,175 | src/code/irqmgr.c |
| `Graph_Update` | 0.16% | 4.7% | 5,001 | 2,725 | 162 | 99,430 | 129,907 | src/code/graph.c |
| `Sched_HandleNotification` | 0.16% | 4.7% | 32,218 | 421 | 48 | 72,555 | 162,161 | src/code/sched.c |
| `Graph_TaskSet00` | 0.14% | 4.3% | 5,000 | 2,494 | 182 | 96,546 | 138,143 | src/code/graph.c |
| `Sched_HandleRetrace` | 0.13% | 4.1% | 13,687 | 858 | 56 | 77,149 | 119,471 | src/code/sched.c |
| `PadUtils_UpdateRelXY` | 0.11% | 3.3% | 59,741 | 160 | 26 | 13,695 | 144,113 | src/libu64/pad.c |
| `PadMgr_UpdateRumble` | 0.10% | 3.0% | 13,684 | 639 | 113 | 13,887 | 123,221 | src/code/padmgr.c |
| `PadSetup_Init` | 0.10% | 3.0% | 1 | 8,562,635 | 1,221,376 | 25,244 | 75,907 | src/libu64/padsetup.c |
| `Fault_RemoveClient` | 0.09% | 2.6% | 34,642 | 219 | 24 | 61,636 | 42,746 | src/code/fault_n64.c |
| `Fault_AddClient` | 0.09% | 2.6% | 34,644 | 217 | 28 | 39,857 | 64,425 | src/code/fault_n64.c |
| `PadMgr_RequestPadData` | 0.08% | 2.5% | 5,001 | 1,450 | 156 | 63,192 | 43,674 | src/code/padmgr.c |
| `Sched_TaskComplete.part.0` | 0.08% | 2.4% | 18,500 | 380 | 33 | 50,435 | 73,791 | src/code/sched.c |
| `Math_FAtan2F` | 0.07% | 2.1% | 36,645 | 162 | 25 | 8,377 | 67,581 | src/libc64/math64.c |
| `Sched_ThreadEntry` | 0.07% | 2.0% | 1 | 5,781,948 | 831,843 | 43 | 101,536 | src/code/sched.c |
| `Sched_QueueTask` | 0.07% | 2.0% | 18,507 | 310 | 22 | 18,278 | 80,463 | src/code/sched.c |
| `Graph_InitTHGA` | 0.06% | 1.9% | 5,001 | 1,083 | 87 | 35,002 | 64,085 | src/code/graph.c |
| `Math_FAtanContFracF` | 0.06% | 1.7% | 35,924 | 135 | 30 | 9,887 | 52,976 | src/libc64/math64.c |
| `IrqMgr_ThreadEntry` | 0.04% | 1.3% | 1 | 3,781,178 | 219,018 | 13,681 | 54,792 | src/code/irqmgr.c |
| `ViConfig_UpdateBlack` | 0.03% | 1.0% | 13,686 | 219 | 13 | 27,372 | 30,630 | src/boot/viconfig.c |
| (49 more functions) | 0.27% | 8.0% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `PadMgr_UpdateInputs` (src/code/padmgr.c): 302: 24%, 67,254 data fills; 292: 13%; 307: 9%; 342: 9%; 346: 9%
- `Sched_HandleRSPDone` (src/code/sched.c): 683: 23%, 26,800 data fills; 682: 16%, 28,643 data fills; 715: 13%, 13,516 data fills; 673: 10%; 685: 8%, 9,935 data fills
- `PadMgr_HandleRetrace` (src/code/padmgr.c): 382: 12%, 27,231 data fills; 416: 9%, 27,364 data fills; 352: 8%; 391: 7%; 359: 6%, 13,662 data fills

## Actor framework (z_actor.c): 2.92%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_actor.c | 2.91% | 99.6% | 1,836,843 | 1,962,651 |
| z_cheap_proc.inc.c | 0.01% | 0.4% | 13,131 | 3,521 |
| z_actor_dlftbls.c | 0.00% | 0.0% | 5 | 23 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Actor_UpdateAll` | 0.81% | 27.7% | 4,221 | 16,672 | 1,957 | 569,601 | 415,406 | src/code/z_actor.c |
| `Actor_DrawAll` | 0.41% | 14.1% | 4,221 | 8,511 | 775 | 365,344 | 188,965 | src/code/z_actor.c |
| `Actor_Draw.part.0` | 0.39% | 13.4% | 34,638 | 982 | 112 | 275,939 | 234,421 | src/code/z_actor.c |
| `func_8002E830` | 0.22% | 7.6% | 14,032 | 1,383 | 192 | 133,265 | 157,259 | src/code/z_actor.c |
| `Actor_CullingVolumeTest` | 0.16% | 5.6% | 72,566 | 196 | 27 | 107,413 | 70,785 | src/code/z_actor.c |
| `ActorShadow_DrawFeet` | 0.10% | 3.5% | 2,813 | 3,163 | 303 | 54,251 | 88,249 | src/code/z_actor.c |
| `ActorShadow_DrawFoot` | 0.08% | 2.6% | 5,370 | 1,234 | 105 | 34,379 | 71,728 | src/code/z_actor.c |
| `Actor_UpdateBgCheckInfoImpl` | 0.07% | 2.3% | 4,386 | 1,324 | 96 | 31,947 | 69,615 | src/code/z_actor.c |
| `Attention_Update` | 0.06% | 2.0% | 4,221 | 1,214 | 70 | 30,605 | 63,366 | src/code/z_actor.c |
| `Actor_Draw` | 0.05% | 1.8% | 34,638 | 135 | 21 | 26,100 | 44,577 | src/code/z_actor.c |
| `Actor_UpdatePos` | 0.04% | 1.5% | 11,273 | 338 | 34 | 20,012 | 34,493 | src/code/z_actor.c |
| `func_8002E2AC` | 0.04% | 1.5% | 4,386 | 867 | 74 | 13,008 | 51,606 | src/code/z_actor.c |
| `Attention_SetNaviState` | 0.04% | 1.4% | 4,211 | 841 | 48 | 24,493 | 29,477 | src/code/z_actor.c |
| `Attention_FindActor` | 0.03% | 0.9% | 4,221 | 546 | 51 | 14,532 | 22,367 | src/code/z_actor.c |
| `ActorShadow_Draw` | 0.03% | 0.9% | 2,280 | 998 | 83 | 12,086 | 23,779 | src/code/z_actor.c |
| `Actor_UpdateVelocityXZGravity` | 0.03% | 0.9% | 7,314 | 310 | 30 | 12,471 | 23,606 | src/code/z_actor.c |
| `Attention_Draw` | 0.03% | 0.9% | 3,948 | 573 | 40 | 8,041 | 31,840 | src/code/z_actor.c |
| `func_8002EBCC` | 0.03% | 0.9% | 4,221 | 530 | 40 | 7,685 | 33,774 | src/code/z_actor.c |
| `Actor_GetWorldPosShapeRot` | 0.02% | 0.8% | 8,436 | 241 | 27 | 7,270 | 25,370 | src/code/z_actor.c |
| `Actor_DrawFaroresWindPointer` | 0.02% | 0.7% | 4,221 | 430 | 37 | 4,520 | 25,331 | src/code/z_actor.c |
| `Actor_MoveXZGravity` | 0.02% | 0.7% | 4,893 | 352 | 42 | 9,479 | 13,445 | src/code/z_actor.c |
| `Actor_ProjectPos` | 0.02% | 0.6% | 10,518 | 152 | 20 | 8,476 | 16,890 | src/code/z_actor.c |
| `Npc_TrackPointWithLimits` | 0.02% | 0.6% | 1,567 | 1,001 | 162 | 2,904 | 22,787 | src/code/z_actor.c |
| `Actor_UpdateBgCheckInfo` | 0.02% | 0.6% | 4,386 | 320 | 25 | 5,600 | 17,539 | src/code/z_actor.c |
| `func_8002DCE4` | 0.02% | 0.5% | 4,221 | 314 | 16 | 8,178 | 12,124 | src/code/z_actor.c |
| (62 more functions) | 0.18% | 6.1% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Actor_UpdateAll` (src/code/z_actor.c): 2585: 12%, 70,582 data fills; 2579: 8%, 70,578 data fills; 2553: 7%, 69,845 data fills; 2552: 7%, 71,990 data fills; 2545: 7%, 62,457 data fills
- `Actor_DrawAll` (src/code/z_actor.c): 3087: 29%, 145,157 data fills; 3077: 15%, 72,446 data fills; 3066: 8%, 17,226 data fills; 3045: 8%, 30,353 data fills; 3090: 7%, 12,206 data fills
- `Actor_Draw.part.0` (src/code/z_actor.c): 2746: 14%; 2737: 10%, 35,866 data fills; 2742: 10%, 39,838 data fills; 2789: 9%, 36,264 data fills; 2700: 8%
- `func_8002E830` (src/code/z_actor.c): 1613: 16%, 29,472 data fills; 1632: 11%; 1547: 9%, 13,341 data fills; 1619: 8%, 11,104 data fills; 1596: 8%, 10,359 data fills

## Sky, weather and lights: 2.58%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_lights.c | 1.69% | 65.7% | 895,613 | 783,319 |
| z_kankyo.c | 0.78% | 30.1% | 246,210 | 781,222 |
| z_vr_box_draw.c | 0.10% | 3.7% | 40,114 | 104,788 |
| z_vr_box.c | 0.01% | 0.4% | 9,837 | 660 |
| z_env_flags.c | 0.00% | 0.0% | 255 | 797 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Lights_BindPoint` | 0.63% | 24.4% | 429,817 | 127 | 35 | 107,522 | 260,438 | src/code/z_lights.c |
| `Lights_BindAll` | 0.44% | 17.2% | 43,081 | 893 | 166 | 388,051 | 111,731 | src/code/z_lights.c |
| `Environment_Update` | 0.34% | 13.0% | 4,221 | 6,927 | 1,077 | 80,474 | 367,450 | src/code/z_kankyo.c |
| `Lights_BindDirectional` | 0.18% | 7.1% | 86,161 | 186 | 27 | 102,532 | 129,301 | src/code/z_lights.c |
| `Lights_Draw` | 0.17% | 6.6% | 43,080 | 344 | 50 | 121,864 | 81,808 | src/code/z_lights.c |
| `Lights_GlowCheck` | 0.12% | 4.5% | 4,221 | 2,384 | 192 | 94,890 | 62,873 | src/code/z_lights.c |
| `LightContext_NewLights` | 0.09% | 3.7% | 43,080 | 191 | 14 | 58,543 | 77,750 | src/code/z_lights.c |
| `Skybox_Draw` | 0.09% | 3.6% | 2,398 | 3,412 | 272 | 40,114 | 100,567 | src/code/z_vr_box_draw.c |
| `Environment_DrawRain` | 0.09% | 3.5% | 562 | 13,789 | 3,293 | 36,959 | 35,861 | src/code/z_kankyo.c |
| `Environment_DrawLensFlare` | 0.08% | 3.0% | 2,118 | 3,139 | 244 | 32,348 | 63,568 | src/code/z_kankyo.c |
| `Environment_DrawSunAndMoon` | 0.06% | 2.4% | 2,118 | 2,559 | 238 | 13,555 | 75,497 | src/code/z_kankyo.c |
| `Lights_DrawGlow` | 0.04% | 1.6% | 4,221 | 826 | 106 | 12,474 | 42,225 | src/code/z_lights.c |
| `Environment_DrawLightning` | 0.04% | 1.5% | 4,221 | 797 | 79 | 21,705 | 34,603 | src/code/z_kankyo.c |
| `Environment_FillScreen` | 0.03% | 1.3% | 7,578 | 396 | 33 | 2,453 | 50,450 | src/code/z_kankyo.c |
| `Environment_DrawSkyboxFilters` | 0.02% | 1.0% | 4,221 | 515 | 45 | 4,717 | 34,233 | src/code/z_kankyo.c |
| `Environment_UpdateSkybox` | 0.02% | 1.0% | 1,384 | 1,541 | 160 | 7,791 | 29,982 | src/code/z_kankyo.c |
| `Environment_LerpWeight` | 0.02% | 0.6% | 5,216 | 267 | 23 | 4,216 | 15,692 | src/code/z_kankyo.c |
| `Environment_UpdateLightningStrike` | 0.01% | 0.6% | 4,221 | 309 | 26 | 4,760 | 19,599 | src/code/z_kankyo.c |
| `Environment_GraphCallback` | 0.01% | 0.6% | 4,221 | 298 | 17 | 12,664 | 12,664 | src/code/z_kankyo.c |
| `Environment_DrawSunLensFlare` | 0.01% | 0.6% | 2,118 | 586 | 50 | 5,488 | 14,901 | src/code/z_kankyo.c |
| `Lights_PointNoGlowSetInfo` | 0.01% | 0.4% | 3,965 | 223 | 13 | 3,975 | 11,627 | src/code/z_lights.c |
| `Environment_ChangeLightSetting` | 0.01% | 0.4% | 2,421 | 347 | 13 | 7,235 | 7,274 | src/code/z_kankyo.c |
| `Environment_DrawCustomLensFlare` | 0.01% | 0.3% | 4,221 | 178 | 6 | 4,221 | 8,442 | src/code/z_kankyo.c |
| `Lights_PointGlowSetInfo` | 0.01% | 0.3% | 5,626 | 132 | 14 | 5,604 | 5,390 | src/code/z_lights.c |
| `Environment_PlayTimeBasedSequence` | 0.01% | 0.2% | 4,221 | 110 | 8 | 4,133 | 4,221 | src/code/z_kankyo.c |
| (20 more functions) | 0.02% | 0.8% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Lights_BindPoint` (src/code/z_lights.c): 127: 18%, 72,693 data fills; 128: 17%; 125: 13%; 126: 12%; 130: 10%
- `Lights_BindAll` (src/code/z_lights.c): 192: 49%, 183,274 data fills; 190: 27%, 161,418 data fills; 187: 8%; 195: 6%; 193: 5%, 15,022 data fills
- `Environment_Update` (src/code/z_kankyo.c): 1134: 4%; 1108: 3%; 1073: 3%; 1078: 3%; 1118: 3%

## Loading (DMA, decompression, overlays): 2.46%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| yaz0.c | 2.11% | 85.8% | 1,150,943 | 12,072 |
| z_std_dma.c | 0.29% | 11.8% | 206,754 | 157,948 |
| loadfragment2_n64.c | 0.06% | 2.3% | 37,622 | 1,580 |
| z_DLF.c | 0.00% | 0.0% | 40 | 76 |
| z_locale.c | 0.00% | 0.0% | 2 | 5 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Yaz0_DecompressImpl` | 2.10% | 85.2% | 125 | 1,459,333 | 684,805 | 1,140,811 | 8,817 | src/boot/yaz0.c |
| `DmaMgr_ProcessRequest` | 0.15% | 6.2% | 7,127 | 1,867 | 286 | 133,474 | 44,450 | src/boot/z_std_dma.c |
| `DmaMgr_DmaRomToRam` | 0.08% | 3.2% | 12,671 | 535 | 72 | 51,077 | 45,339 | src/boot/z_std_dma.c |
| `Overlay_Relocate` | 0.06% | 2.3% | 49 | 100,661 | 41,269 | 37,206 | 931 | src/libu64/loadfragment2_n64.c |
| `DmaMgr_RequestAsync` | 0.04% | 1.6% | 7,126 | 487 | 32 | 18,219 | 41,542 | src/boot/z_std_dma.c |
| `Yaz0_NextDMA` | 0.01% | 0.6% | 5,544 | 218 | 54 | 9,542 | 2,395 | src/boot/yaz0.c |
| `DmaMgr_ThreadEntry` | 0.01% | 0.4% | 1 | 815,043 | 106,915 | 1,201 | 13,218 | src/boot/z_std_dma.c |
| `DmaMgr_RequestSync` | 0.01% | 0.3% | 2,819 | 259 | 29 | 1,394 | 11,539 | src/boot/z_std_dma.c |
| `DmaMgr_AudioDmaHandler` | 0.00% | 0.1% | 717 | 251 | 20 | 1,387 | 1,851 | src/boot/z_std_dma.c |
| `Overlay_Load` | 0.00% | 0.0% | 49 | 1,199 | 67 | 414 | 638 | src/libu64/loadfragment2_n64.c |
| `Yaz0_FirstDMA` | 0.00% | 0.0% | 125 | 377 | 38 | 239 | 541 | src/boot/yaz0.c |
| `Yaz0_Decompress` | 0.00% | 0.0% | 125 | 310 | 19 | 351 | 319 | src/boot/yaz0.c |
| `Overlay_LoadGameState` | 0.00% | 0.0% | 9 | 473 | 23 | 26 | 46 | src/code/z_DLF.c |
| `Overlay_FreeGameState` | 0.00% | 0.0% | 8 | 328 | 19 | 14 | 30 | src/code/z_DLF.c |
| `Overlay_AllocateAndLoad` | 0.00% | 0.0% | 3 | 238 | 23 | 2 | 11 | src/libu64/loadfragment2_n64.c |
| `DmaMgr_Init` | 0.00% | 0.0% | 1 | 612 | 55 | 2 | 9 | src/boot/z_std_dma.c |
| `Locale_Init` | 0.00% | 0.0% | 1 | 422 | 19 | 2 | 5 | src/boot/z_locale.c |

## Link: 2.14%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_player_lib.c | 1.14% | 53.5% | 337,916 | 1,228,906 |
| ovl_player_actor | 0.98% | 45.7% | 391,404 | 1,048,452 |
| z_player_call.c | 0.02% | 0.8% | 12,679 | 16,932 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Player_OverrideLimbDrawGameplayCommon` | 0.32% | 14.8% | 88,642 | 311 | 35 | 81,177 | 380,179 | src/code/z_player_lib.c |
| `Player_UpdateCommon` | 0.28% | 13.2% | 4,220 | 5,803 | 365 | 145,718 | 267,507 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_OverrideLimbDrawGameplayDefault` | 0.24% | 11.2% | 70,035 | 298 | 35 | 59,327 | 293,036 | src/code/z_player_lib.c |
| `Player_PostLimbDrawGameplay` | 0.22% | 10.5% | 88,641 | 221 | 58 | 55,420 | 188,748 | src/code/z_player_lib.c |
| `func_8008F87C.part.0` | 0.16% | 7.7% | 8,442 | 1,694 | 159 | 61,652 | 160,486 | src/code/z_player_lib.c |
| `func_80847298` | 0.16% | 7.4% | 4,220 | 3,248 | 281 | 16,363 | 206,785 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_ProcessSceneCollision` | 0.10% | 4.5% | 2,420 | 3,494 | 267 | 35,351 | 108,730 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_DrawImpl` | 0.10% | 4.5% | 4,221 | 1,973 | 137 | 55,492 | 80,185 | src/code/z_player_lib.c |
| `Player_Draw` | 0.07% | 3.1% | 4,221 | 1,383 | 91 | 34,613 | 67,004 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_Update` | 0.06% | 2.6% | 4,220 | 1,145 | 56 | 29,629 | 54,314 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_OverrideLimbDrawGameplayFirstPerson` | 0.05% | 2.1% | 18,606 | 214 | 26 | 2,213 | 63,825 | src/code/z_player_lib.c |
| `Player_DrawGameplay` | 0.05% | 2.1% | 4,221 | 935 | 60 | 27,671 | 41,324 | src/overlays/actors/ovl_player_actor/z_player.c |
| `func_8083D6EC` | 0.03% | 1.5% | 4,220 | 642 | 45 | 12,173 | 33,762 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_ProcessControlStick` | 0.03% | 1.4% | 4,220 | 632 | 47 | 12,822 | 33,761 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_UpdateCamAndSeqModes.part.0` | 0.03% | 1.2% | 4,278 | 529 | 34 | 7,252 | 32,043 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_UpdateShapeYaw` | 0.03% | 1.2% | 4,220 | 525 | 23 | 15,900 | 17,158 | src/overlays/actors/ovl_player_actor/z_player.c |
| `func_80852B4C` | 0.02% | 1.2% | 6,028 | 356 | 22 | 13,947 | 21,623 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_UpdateInterface.part.0` | 0.02% | 1.0% | 2,478 | 755 | 49 | 8,162 | 22,590 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_HandleExitsAndVoids.part.0` | 0.02% | 0.9% | 2,691 | 641 | 46 | 7,861 | 21,494 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_GetEnvironmentalHazard` | 0.02% | 0.8% | 8,442 | 184 | 16 | 11,167 | 16,884 | src/code/z_player_lib.c |
| `Player_UpdateZTargeting` | 0.02% | 0.8% | 4,220 | 348 | 31 | 1,780 | 25,328 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_InCsMode` | 0.01% | 0.7% | 6,612 | 188 | 10 | 5,407 | 15,972 | src/code/z_player_lib.c |
| `func_80852C50` | 0.01% | 0.6% | 2,085 | 576 | 41 | 7,118 | 12,584 | src/overlays/actors/ovl_player_actor/z_player.c |
| `func_80851688` | 0.01% | 0.6% | 2,160 | 540 | 33 | 2,258 | 19,190 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_Action_CsAction` | 0.01% | 0.5% | 3,928 | 221 | 21 | 18 | 15,724 | src/overlays/actors/ovl_player_actor/z_player.c |
| (66 more functions) | 0.08% | 3.7% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Player_OverrideLimbDrawGameplayCommon` (src/code/z_player_lib.c): 1279: 22%, 27,329 data fills; 1354: 18%; 1320: 14%; 1343: 12%; 1346: 7%, 12,946 data fills
- `Player_UpdateCommon` (src/overlays/actors/ovl_player_actor/z_player.c): 12032: 4%, 8,176 data fills; 12065: 4%, 8,440 data fills; 11711: 3%; 11727: 3%, 8,168 data fills; 12067: 2%, 8,435 data fills
- `Player_OverrideLimbDrawGameplayDefault` (src/code/z_player_lib.c): 1360: 33%; 1357: 19%; 1412: 18%; 1405: 5%, 10,005 data fills; 1370: 4%, 10,005 data fills
- `Player_PostLimbDrawGameplay` (src/code/z_player_lib.c): 1888: 16%; 1657: 16%; 1885: 11%, 12,999 data fills; 1881: 10%, 9,886 data fills; 1851: 7%

## C library (memory copies, printf, 64-bit maths): 1.80%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| memset.c | 0.84% | 46.8% | 951,578 | 55,594 |
| bzero.c | 0.56% | 31.1% | 552,446 | 174 |
| bcopy.c | 0.20% | 11.1% | 207,907 | 128,768 |
| ll.c | 0.17% | 9.5% | 10 | 27,696 |
| memmove.c | 0.03% | 1.5% | 32,266 | 99 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `memset` | 0.84% | 46.8% | 36,099 | 2,028 | 256 | 951,578 | 55,594 | src/libc/memset.c |
| `bzero` | 0.56% | 31.1% | 37 | 1,313,305 | 74,679 | 552,446 | 174 | src/libultra/libc/bzero.c |
| `bcopy` | 0.20% | 11.1% | 28,928 | 600 | 69 | 207,907 | 128,768 | src/libultra/libc/bcopy.c |
| `__ull_div` | 0.17% | 9.5% | 154,056 | 97 | 14 | 10 | 27,696 | src/libultra/libc/ll.c |
| `memmove` | 0.03% | 1.5% | 15 | 154,132 | 14,112 | 32,266 | 99 | src/libc/memmove.c |

## Fishing mod: 1.68%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| fishmod.c | 0.67% | 39.7% | 148,518 | 570,145 |
| fishmod_trig.h | 0.60% | 35.5% | 63,366 | 133,003 |
| fishmod_tri.h | 0.24% | 14.2% | 7,605 | 169,884 |
| ovl_En_Fishmod | 0.07% | 4.2% | 42,682 | 60,915 |
| fishmod_items.c | 0.07% | 4.0% | 31,583 | 69,372 |
| fish manager | 0.02% | 1.3% | 10,161 | 24,051 |
| props (scatter) | 0.02% | 1.1% | 8,427 | 16,641 |
| fish rigs | 0.00% | 0.0% | 15 | 9 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Fishmod_SinCosBinang` | 0.60% | 35.5% | 396,258 | 131 | 43 | 63,366 | 133,003 | fishmod_trig.h |
| `Fishmod_ClaimDisplayLists` | 0.45% | 26.7% | 38,859 | 1,006 | 131 | 29,231 | 343,102 | src/code/fishmod.c |
| `Fishmod_PointDistSqToSeg2D` | 0.18% | 10.6% | 71,560 | 216 | 50 | 2,760 | 122,351 | fishmod_tri.h |
| `Fishmod_TriChkPointParaY.constprop.0` | 0.06% | 3.6% | 27,144 | 197 | 69 | 4,845 | 47,533 | fishmod_tri.h |
| `Fishmod_TerrainRA` | 0.06% | 3.6% | 4,221 | 1,234 | 78 | 34,170 | 57,586 | src/code/fishmod.c |
| `Fishmod_TimeUpdate` | 0.06% | 3.3% | 4,222 | 1,139 | 58 | 37,974 | 55,407 | src/code/fishmod.c |
| `Fishmod_PlayUpdate` | 0.04% | 2.5% | 4,222 | 866 | 56 | 25,365 | 45,164 | src/code/fishmod.c |
| `Fishmod_EquipButtons` | 0.03% | 1.7% | 4,221 | 602 | 30 | 24,324 | 21,125 | src/code/fishmod_items.c |
| `Fishmod_DpadDraw` | 0.03% | 1.6% | 3,948 | 599 | 57 | 7,259 | 31,584 | src/code/fishmod_items.c |
| `EnFishmod_RodUpdate` | 0.02% | 1.4% | 1,845 | 1,107 | 65 | 13,390 | 20,299 | src/overlays/actors/ovl_En_Fishmod/fishmod_rod.c |
| `EnFishmod_ManagerDraw` | 0.02% | 1.3% | 1,845 | 1,065 | 78 | 10,149 | 24,045 | src/overlays/actors/ovl_En_Fishmod/fishmod_manager.c |
| `EnFishmod_ScatterDraw` | 0.02% | 1.1% | 1,845 | 841 | 50 | 8,406 | 16,621 | src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c |
| `Fishmod_Relight` | 0.02% | 1.0% | 4,221 | 361 | 36 | 4,277 | 21,105 | src/code/fishmod.c |
| `EnFishmod_Update` | 0.01% | 0.8% | 1,845 | 619 | 36 | 9,038 | 12,917 | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_RodDraw` | 0.01% | 0.8% | 1,845 | 618 | 42 | 7,329 | 11,070 | src/overlays/actors/ovl_En_Fishmod/fishmod_rod.c |
| `Fishmod_AdvanceTime` | 0.01% | 0.8% | 2,255 | 501 | 30 | 6,793 | 12,885 | src/code/fishmod.c |
| `Rod_PublishInfo` | 0.01% | 0.6% | 1,845 | 485 | 19 | 7,383 | 7,381 | src/overlays/actors/ovl_En_Fishmod/fishmod_rod.c |
| `Fishmod_SceneHasFish` | 0.01% | 0.6% | 4,225 | 196 | 15 | 1,854 | 12,674 | src/code/fishmod.c |
| `Fishmod_HazeTint` | 0.01% | 0.6% | 4,221 | 195 | 10 | 4,221 | 12,663 | src/code/fishmod.c |
| `__udivdi3` | 0.01% | 0.5% | 154,058 | 5 | 2 | 0 | 9,224 | src/code/fishmod.c |
| `Fishmod_SlotItem` | 0.01% | 0.5% | 15,792 | 44 | 8 | 0 | 11,844 | src/code/fishmod_items.c |
| `EnFishmod_Draw` | 0.01% | 0.4% | 3,690 | 169 | 6 | 3,690 | 7,384 | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `Fishmod_TerrainModesWalk` | 0.00% | 0.3% | 0 | 0 | 0 | 4,462 | 44 | src/code/fishmod.c |
| `EnFishmod_ParticlesDraw` | 0.00% | 0.2% | 1,846 | 134 | 6 | 1,845 | 1,845 | src/overlays/actors/ovl_En_Fishmod/fishmod_particles.c |
| `Fishmod_ClockDraw` | 0.00% | 0.2% | 2,407 | 100 | 4 | 0 | 4,814 | src/code/fishmod_items.c |
| (27 more functions) | 0.00% | 0.0% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Fishmod_SinCosBinang` (fishmod_trig.h): 51: 28%; 49: 24%; 48: 23%; 50: 9%; 59: 6%, 14,813 data fills
- `Fishmod_ClaimDisplayLists` (src/code/fishmod.c): 1591: 40%; 1592: 35%; 1593: 24%; 1594: 0%

## HUD and map: 1.49%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_parameter.c | 1.05% | 70.6% | 515,957 | 964,695 |
| z_lifemeter.c | 0.30% | 20.1% | 124,612 | 294,054 |
| z_map_exp.c | 0.14% | 9.2% | 53,625 | 144,107 |
| z_construct.c | 0.00% | 0.0% | 491 | 454 |
| speed_meter.c | 0.00% | 0.0% | 9 | 23 |
| z_map_mark.c | 0.00% | 0.0% | 12 | 12 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Interface_Draw` | 0.37% | 25.0% | 3,948 | 8,192 | 608 | 190,819 | 337,692 | src/code/z_parameter.c |
| `Interface_DrawItemButtons` | 0.28% | 18.8% | 3,948 | 6,186 | 613 | 151,870 | 236,954 | src/code/z_parameter.c |
| `Health_DrawMeter` | 0.23% | 15.4% | 3,948 | 5,056 | 499 | 90,753 | 217,167 | src/code/z_lifemeter.c |
| `Interface_InitVertices` | 0.15% | 9.8% | 3,948 | 3,226 | 374 | 61,036 | 138,207 | src/code/z_parameter.c |
| `Minimap_Draw` | 0.12% | 8.1% | 3,948 | 2,674 | 219 | 53,518 | 118,072 | src/code/z_map_exp.c |
| `Interface_Update` | 0.09% | 6.4% | 4,221 | 1,959 | 121 | 46,661 | 106,363 | src/code/z_parameter.c |
| `Interface_DrawActionButton` | 0.06% | 4.1% | 3,948 | 1,334 | 114 | 22,870 | 55,330 | src/code/z_parameter.c |
| `Health_UpdateMeter` | 0.06% | 3.9% | 4,221 | 1,188 | 99 | 29,550 | 59,516 | src/code/z_lifemeter.c |
| `func_8008A8B8` | 0.04% | 2.8% | 7,896 | 457 | 58 | 21,656 | 27,755 | src/code/z_parameter.c |
| `Interface_DrawActionLabel` | 0.03% | 2.1% | 3,948 | 684 | 54 | 15,793 | 27,637 | src/code/z_parameter.c |
| `Magic_DrawMeter` | 0.01% | 0.9% | 3,948 | 288 | 27 | 10 | 19,740 | src/code/z_parameter.c |
| `Health_UpdateBeatingHeart` | 0.01% | 0.9% | 4,221 | 267 | 20 | 4,274 | 17,341 | src/code/z_lifemeter.c |
| `Map_Update` | 0.01% | 0.8% | 4,221 | 258 | 16 | 2 | 21,105 | src/code/z_map_exp.c |
| `Interface_SetDoAction` | 0.01% | 0.4% | 2,487 | 186 | 12 | 4,582 | 5,022 | src/code/z_parameter.c |
| `Interface_SetNaviCall` | 0.00% | 0.3% | 2,479 | 166 | 17 | 9 | 7,437 | src/code/z_parameter.c |
| `Minimap_DrawCompassIcons` | 0.00% | 0.2% | 2,407 | 105 | 8 | 0 | 4,814 | src/code/z_map_exp.c |
| `Interface_UpdateHudAlphas` | 0.00% | 0.1% | 77 | 898 | 59 | 239 | 1,017 | src/code/z_parameter.c |
| `Magic_Update` | 0.00% | 0.1% | 199 | 330 | 29 | 199 | 1,019 | src/code/z_parameter.c |
| `Regs_InitDataImpl` | 0.00% | 0.0% | 5 | 6,152 | 476 | 292 | 297 | src/code/z_construct.c |
| `Interface_LoadActionLabel` | 0.00% | 0.0% | 44 | 640 | 52 | 138 | 365 | src/code/z_parameter.c |
| `Interface_Init` | 0.00% | 0.0% | 5 | 3,068 | 153 | 154 | 113 | src/code/z_construct.c |
| `Map_Init` | 0.00% | 0.0% | 5 | 1,423 | 72 | 67 | 51 | src/code/z_map_exp.c |
| `Interface_SetSceneRestrictions` | 0.00% | 0.0% | 5 | 1,014 | 244 | 37 | 45 | src/code/z_parameter.c |
| `Message_Init` | 0.00% | 0.0% | 5 | 939 | 34 | 45 | 30 | src/code/z_construct.c |
| `Health_InitMeter` | 0.00% | 0.0% | 5 | 742 | 42 | 35 | 30 | src/code/z_lifemeter.c |
| (12 more functions) | 0.00% | 0.0% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Interface_Draw` (src/code/z_parameter.c): 3347: 15%, 39,592 data fills; 3253: 8%, 19,742 data fills; 3314: 5%, 15,037 data fills; 3455: 3%, 4,132 data fills; 4047: 3%, 4,181 data fills
- `Interface_DrawItemButtons` (src/code/z_parameter.c): 2997: 29%, 62,278 data fills; 2854: 15%, 31,587 data fills; 2861: 6%; 2868: 6%; 2875: 6%
- `Health_DrawMeter` (src/code/z_lifemeter.c): 465: 16%, 15,794 data fills; 436: 12%, 11,846 data fills; 347: 8%, 11,825 data fills; 359: 5%, 3,952 data fills; 349: 4%, 3,949 data fills

## Camera and view: 1.26%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_view.c | 0.74% | 58.8% | 357,328 | 602,279 |
| z_camera.c | 0.46% | 36.6% | 211,550 | 462,632 |
| shrink_window.c | 0.03% | 2.4% | 13,565 | 35,382 |
| z_quake.c | 0.03% | 2.2% | 10,215 | 33,784 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Camera_Update` | 0.26% | 20.5% | 8,442 | 2,655 | 187 | 149,121 | 226,869 | src/code/z_camera.c |
| `View_ApplyOrthoToOverlay` | 0.20% | 15.7% | 8,986 | 1,906 | 199 | 94,542 | 164,272 | src/code/z_view.c |
| `View_ApplyPerspectiveToOverlay` | 0.19% | 15.2% | 7,896 | 2,105 | 285 | 76,549 | 150,740 | src/code/z_view.c |
| `View_ApplyPerspective` | 0.18% | 14.4% | 6,095 | 2,579 | 257 | 108,271 | 136,534 | src/code/z_view.c |
| `Camera_CalcUpFromPitchYawRoll` | 0.06% | 5.0% | 4,221 | 1,304 | 144 | 3,775 | 80,202 | src/code/z_camera.c |
| `View_ApplyLetterbox` | 0.06% | 4.7% | 6,096 | 842 | 81 | 20,759 | 60,487 | src/code/z_view.c |
| `View_LookAt` | 0.05% | 4.1% | 13,991 | 318 | 31 | 27,491 | 39,691 | src/code/z_view.c |
| `Camera_Demo1` | 0.05% | 4.1% | 3,695 | 1,200 | 78 | 36,689 | 43,372 | src/code/z_camera.c |
| `Quake_Update` | 0.03% | 2.2% | 4,221 | 580 | 47 | 10,170 | 33,769 | src/code/z_quake.c |
| `View_SetViewport` | 0.02% | 1.4% | 17,168 | 91 | 12 | 11,908 | 8,468 | src/code/z_view.c |
| `Letterbox_Update` | 0.02% | 1.4% | 4,221 | 363 | 16 | 4,222 | 25,327 | src/code/shrink_window.c |
| `Camera_UpdateDistortion` | 0.02% | 1.3% | 4,221 | 348 | 33 | 44 | 25,326 | src/code/z_camera.c |
| `Camera_RequestModeImpl` | 0.01% | 1.2% | 4,238 | 297 | 17 | 4,306 | 17,055 | src/code/z_camera.c |
| `Camera_GetBgCamIndex` | 0.01% | 1.1% | 4,200 | 284 | 30 | 48 | 21,001 | src/code/z_camera.c |
| `View_StepDistortion.part.0` | 0.01% | 0.9% | 6,095 | 168 | 9 | 5,620 | 10,832 | src/code/z_view.c |
| `Camera_RequestBgCam` | 0.01% | 0.9% | 3,947 | 251 | 19 | 1,540 | 15,802 | src/code/z_camera.c |
| `Camera_Fixed3` | 0.01% | 0.9% | 526 | 1,789 | 117 | 6,309 | 9,473 | src/code/z_camera.c |
| `View_SetScale` | 0.01% | 0.7% | 4,221 | 183 | 5 | 8,443 | 4,221 | src/code/z_view.c |
| `Letterbox_GetSize` | 0.01% | 0.7% | 11,095 | 68 | 3 | 5,622 | 6,324 | src/code/shrink_window.c |
| `Camera_Finish` | 0.01% | 0.6% | 4,221 | 166 | 4 | 3,711 | 8,442 | src/code/z_camera.c |
| `View_Apply` | 0.01% | 0.5% | 6,095 | 96 | 9 | 0 | 10,918 | src/code/z_view.c |
| `View_ApplyTo` | 0.01% | 0.5% | 286 | 1,942 | 212 | 2,020 | 6,864 | src/code/z_view.c |
| `View_SetPerspective` | 0.01% | 0.5% | 12,347 | 42 | 7 | 583 | 6,860 | src/code/z_view.c |
| `Camera_GetInputDirYaw` | 0.00% | 0.4% | 4,221 | 91 | 2 | 4,220 | 4,221 | src/code/z_camera.c |
| `Letterbox_SetSizeTarget` | 0.00% | 0.3% | 3,712 | 91 | 3 | 3,712 | 3,712 | src/code/shrink_window.c |
| (22 more functions) | 0.01% | 0.8% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Camera_Update` (src/code/z_camera.c): 8155: 6%, 12,644 data fills; 8074: 6%, 12,668 data fills; 8085: 5%, 11,100 data fills; 8061: 4%, 6,948 data fills; 8070: 4%, 8,529 data fills

## Title, file select, menus: 1.12%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| ovl_file_choose | 1.12% | 100.0% | 783,691 | 243,950 |
| ovl_title | 0.00% | 0.0% | 6 | 5 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `FileSelect_SetWindowContentVtx` | 0.67% | 60.0% | 548 | 107,107 | 25,923 | 485,868 | 77,992 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_DrawWindowContents` | 0.13% | 11.6% | 463 | 24,600 | 2,946 | 81,384 | 61,426 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_SetWindowVtx` | 0.06% | 5.6% | 548 | 10,008 | 2,767 | 44,306 | 13,150 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_DrawCharacterTransition` | 0.06% | 5.5% | 7,150 | 749 | 108 | 53,976 | 1,430 | src/overlays/gamestates/ovl_file_choose/z_file_nameset.c |
| `FileSelect_SetKeyboardVtx` | 0.04% | 4.0% | 110 | 35,497 | 8,936 | 28,795 | 2,420 | src/overlays/gamestates/ovl_file_choose/z_file_nameset.c |
| `FileSelect_DrawFileInfo` | 0.03% | 3.0% | 1,389 | 2,132 | 253 | 18,857 | 20,941 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_DrawCharacter` | 0.03% | 2.6% | 5,408 | 467 | 74 | 22,328 | 5,140 | src/overlays/gamestates/ovl_file_choose/z_file_nameset.c |
| `FileSelect_Main` | 0.02% | 2.0% | 548 | 3,613 | 235 | 12,736 | 19,729 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_ConfigModeDraw` | 0.02% | 1.9% | 459 | 4,079 | 308 | 14,031 | 14,754 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_SetNameEntryVtx` | 0.02% | 1.6% | 110 | 14,024 | 2,321 | 9,220 | 8,856 | src/overlays/gamestates/ovl_file_choose/z_file_nameset.c |
| `FileSelect_DrawKeyboard` | 0.01% | 0.6% | 110 | 5,195 | 1,527 | 3,453 | 2,090 | src/overlays/gamestates/ovl_file_choose/z_file_nameset.c |
| `FileSelect_DrawNameEntry` | 0.01% | 0.5% | 110 | 4,224 | 327 | 2,149 | 5,136 | src/overlays/gamestates/ovl_file_choose/z_file_nameset.c |
| `FileSelect_SelectModeDraw` | 0.00% | 0.4% | 89 | 4,144 | 277 | 2,547 | 3,293 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_PulsateCursor` | 0.00% | 0.3% | 659 | 422 | 38 | 1,481 | 3,143 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_ConfigModeUpdate` | 0.00% | 0.1% | 459 | 245 | 10 | 918 | 918 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_UpdateMainMenu` | 0.00% | 0.1% | 298 | 371 | 36 | 310 | 1,504 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_FadeInMenuElements` | 0.00% | 0.1% | 51 | 1,184 | 87 | 565 | 450 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_UpdateKeyboardCursor` | 0.00% | 0.0% | 80 | 587 | 47 | 213 | 692 | src/overlays/gamestates/ovl_file_choose/z_file_nameset.c |
| `FileSelect_SelectModeUpdate` | 0.00% | 0.0% | 88 | 222 | 10 | 176 | 176 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_FinishFadeIn` | 0.00% | 0.0% | 46 | 258 | 17 | 46 | 184 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_ConfirmFile` | 0.00% | 0.0% | 44 | 250 | 28 | 1 | 193 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_FadeMainToSelect` | 0.00% | 0.0% | 8 | 1,139 | 100 | 80 | 81 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_InitContext` | 0.00% | 0.0% | 1 | 4,813 | 215 | 40 | 30 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_RotateToNameEntry` | 0.00% | 0.0% | 13 | 337 | 16 | 39 | 39 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| `FileSelect_FadeOut` | 0.00% | 0.0% | 26 | 167 | 13 | 52 | 39 | src/overlays/gamestates/ovl_file_choose/z_file_choose.c |
| (11 more functions) | 0.00% | 0.0% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `FileSelect_SetWindowContentVtx` (src/overlays/gamestates/ovl_file_choose/z_file_choose.c): 879: 21%, 178,649 data fills; 878: 12%, 89,874 data fills; 896: 12%, 89,876 data fills; 920: 2%, 16,714 data fills; 895: 2%

## Game loop (game.c, z_play.c): 0.91%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_play.c | 0.72% | 78.6% | 405,029 | 686,573 |
| game.c | 0.18% | 19.9% | 98,880 | 170,195 |
| z_frame_advance.c | 0.01% | 1.5% | 4,270 | 16,889 |
| title_setup.c | 0.00% | 0.0% | 5 | 4 |
| z_game_over.c | 0.00% | 0.0% | 5 | 5 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Play_Draw` | 0.38% | 42.0% | 4,221 | 7,924 | 545 | 223,605 | 358,097 | src/code/z_play.c |
| `Play_Update` | 0.19% | 20.3% | 4,222 | 3,828 | 245 | 112,732 | 172,118 | src/code/z_play.c |
| `GameState_SetFrameBuffer` | 0.05% | 5.8% | 5,001 | 930 | 54 | 35,023 | 39,995 | src/code/game.c |
| `GameState_Draw` | 0.05% | 5.1% | 5,000 | 817 | 53 | 22,593 | 45,000 | src/code/game.c |
| `func_800BFCB8` | 0.04% | 4.9% | 5,580 | 692 | 77 | 18,453 | 40,988 | src/code/z_play.c |
| `GameState_Update` | 0.04% | 4.5% | 5,001 | 718 | 37 | 30,909 | 35,009 | src/code/game.c |
| `Play_Main` | 0.04% | 4.4% | 4,222 | 828 | 56 | 24,464 | 40,238 | src/code/z_play.c |
| `FishmodPlay_ClearZ` | 0.03% | 2.8% | 12,663 | 175 | 14 | 9,984 | 28,358 | src/code/z_play.c |
| `func_800C49F4` | 0.03% | 2.7% | 5,000 | 436 | 33 | 10,248 | 25,006 | src/code/game.c |
| `FrameAdvance_IsEnabled` | 0.02% | 2.5% | 31,570 | 62 | 3 | 8,276 | 30,312 | src/code/z_play.c |
| `FrameAdvance_Update` | 0.01% | 1.5% | 4,221 | 283 | 8 | 4,265 | 16,884 | src/code/z_frame_advance.c |
| `GameState_ReqPadData` | 0.01% | 1.4% | 5,001 | 218 | 19 | 4 | 20,004 | src/code/game.c |
| `Play_GetCamera` | 0.01% | 1.0% | 4,588 | 174 | 7 | 4,223 | 9,002 | src/code/z_play.c |
| `Play_InCsMode` | 0.00% | 0.5% | 5,928 | 73 | 5 | 2,002 | 5,931 | src/code/z_play.c |
| `GameState_IsRunning` | 0.00% | 0.3% | 5,009 | 51 | 2 | 8 | 5,008 | src/code/game.c |
| `Play_CamIsNotFixed` | 0.00% | 0.1% | 274 | 350 | 21 | 826 | 822 | src/code/z_play.c |
| `Play_Init` | 0.00% | 0.0% | 5 | 5,223 | 337 | 170 | 260 | src/code/z_play.c |
| `GameState_Init` | 0.00% | 0.0% | 9 | 935 | 68 | 61 | 86 | src/code/game.c |
| `Play_SpawnScene` | 0.00% | 0.0% | 5 | 1,448 | 63 | 55 | 55 | src/code/z_play.c |
| `Play_ChangeCameraStatus` | 0.00% | 0.0% | 88 | 76 | 12 | 29 | 81 | src/code/z_play.c |
| `Play_InitScene` | 0.00% | 0.0% | 5 | 1,291 | 56 | 51 | 50 | src/code/z_play.c |
| `Play_Destroy` | 0.00% | 0.0% | 4 | 1,354 | 79 | 45 | 56 | src/code/z_play.c |
| `Play_SetupRespawnPoint` | 0.00% | 0.0% | 5 | 964 | 34 | 47 | 30 | src/code/z_play.c |
| `Play_SetupTransition` | 0.00% | 0.0% | 9 | 523 | 57 | 9 | 70 | src/code/z_play.c |
| `Play_RequestCameraSetting` | 0.00% | 0.0% | 44 | 93 | 9 | 21 | 54 | src/code/z_play.c |
| (11 more functions) | 0.00% | 0.0% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Play_Draw` (src/code/z_play.c): 1528: 5%, 8,281 data fills; 1547: 4%, 16,043 data fills; 1226: 3%, 8,090 data fills; 1265: 3%, 4,277 data fills; 1240: 2%, 4,225 data fills

## Cutscenes: 0.85%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_demo.c | 0.74% | 86.7% | 631,662 | 352,948 |
| z_cutscene_spline.c | 0.11% | 13.3% | 22,909 | 94,188 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Cutscene_ProcessScript` | 0.23% | 26.3% | 4,226 | 4,636 | 1,001 | 102,463 | 179,572 | src/code/z_demo.c |
| `CutsceneCmd_UpdateCamEyeSpline` | 0.21% | 24.5% | 17,680 | 1,032 | 82 | 247,289 | 29,744 | src/code/z_demo.c |
| `CutsceneCmd_UpdateCamAtSpline` | 0.20% | 23.9% | 17,680 | 1,005 | 83 | 247,297 | 25,454 | src/code/z_demo.c |
| `func_800BB0A0` | 0.06% | 6.8% | 6,056 | 841 | 127 | 9,129 | 48,546 | src/code/z_cutscene_spline.c |
| `func_800BB2B4` | 0.05% | 6.4% | 6,057 | 786 | 150 | 13,780 | 45,642 | src/code/z_cutscene_spline.c |
| `CutsceneCmd_Destination` | 0.02% | 2.5% | 3,526 | 522 | 27 | 12,642 | 17,902 | src/code/z_demo.c |
| `CutsceneCmd_Text` | 0.02% | 2.3% | 9,021 | 189 | 22 | 4,830 | 22,702 | src/code/z_demo.c |
| `Cutscene_UpdateScripted` | 0.02% | 2.1% | 4,221 | 370 | 25 | 8,448 | 21,111 | src/code/z_demo.c |
| `Cutscene_SetupScripted` | 0.01% | 1.6% | 4,221 | 278 | 24 | 85 | 21,152 | src/code/z_demo.c |
| `CutsceneCmd_Misc` | 0.01% | 1.5% | 8,138 | 140 | 22 | 4,685 | 11,939 | src/code/z_demo.c |
| `Cutscene_UpdateManual` | 0.01% | 1.1% | 4,221 | 193 | 8 | 2,384 | 12,663 | src/code/z_demo.c |
| `CutsceneHandler_RunScript` | 0.01% | 0.7% | 4,221 | 121 | 10 | 858 | 8,442 | src/code/z_demo.c |
| `CutsceneCmd_Transition.part.0` | 0.00% | 0.2% | 307 | 538 | 48 | 567 | 2,207 | src/code/z_demo.c |
| `Cutscene_HandleEntranceTriggers` | 0.00% | 0.0% | 5 | 1,639 | 333 | 95 | 35 | src/code/z_demo.c |
| `Cutscene_HandleConditionalTriggers` | 0.00% | 0.0% | 5 | 369 | 22 | 9 | 20 | src/code/z_demo.c |
| `Cutscene_InitContext` | 0.00% | 0.0% | 5 | 211 | 3 | 10 | 5 | src/code/z_demo.c |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Cutscene_ProcessScript` (src/code/z_demo.c): 1866: 14%, 24,280 data fills; 1858: 12%; 2174: 8%, 19,709 data fills; 2246: 6%, 12,937 data fills; 1944: 5%, 11,861 data fills
- `CutsceneCmd_UpdateCamEyeSpline` (src/code/z_demo.c): 1610: 87%, 236,706 data fills; 1589: 6%, 7,388 data fills; 1590: 3%; 1581: 3%; 1619: 2%
- `CutsceneCmd_UpdateCamAtSpline` (src/code/z_demo.c): 1649: 86%, 235,603 data fills; 1629: 6%, 7,201 data fills; 1630: 4%, 4,422 data fills; 1621: 2%; 1658: 1%

## Text: 0.81%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_message.c | 0.81% | 99.9% | 592,592 | 225,431 |
| z_kanfont.c | 0.00% | 0.1% | 387 | 158 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Message_DrawTextChar` | 0.53% | 65.4% | 55,061 | 839 | 148 | 476,712 | 30,973 | src/code/z_message.c |
| `Message_DrawText` | 0.14% | 16.9% | 1,668 | 7,173 | 1,848 | 53,246 | 48,675 | src/code/z_message.c |
| `Message_DrawMain` | 0.05% | 6.1% | 4,221 | 1,026 | 76 | 22,491 | 48,233 | src/code/z_message.c |
| `Message_Draw` | 0.02% | 2.6% | 4,221 | 442 | 37 | 5,595 | 25,327 | src/code/z_message.c |
| `Message_DrawTextBox` | 0.02% | 2.3% | 1,090 | 1,468 | 99 | 11,159 | 15,260 | src/code/z_message.c |
| `Message_Update` | 0.02% | 2.2% | 4,221 | 372 | 21 | 9,262 | 18,824 | src/code/z_message.c |
| `Message_GetState` | 0.02% | 2.1% | 5,809 | 250 | 17 | 6,487 | 20,154 | src/code/z_message.c |
| `Message_Decode` | 0.01% | 0.9% | 21 | 31,902 | 4,497 | 1,663 | 9,778 | src/code/z_message.c |
| `Message_DrawTextboxIcon` | 0.00% | 0.4% | 84 | 3,487 | 298 | 1,004 | 3,399 | src/code/z_message.c |
| `Message_SetTextColor` | 0.00% | 0.4% | 1,161 | 238 | 21 | 1,005 | 3,788 | src/code/z_message.c |
| `Message_FindMessageNES` | 0.00% | 0.3% | 8 | 30,333 | 3,403 | 3,414 | 32 | src/code/z_message.c |
| `Font_LoadOrderedFont` | 0.00% | 0.1% | 7 | 7,999 | 3,904 | 387 | 82 | src/code/z_kanfont.c |
| `Message_GrowTextbox` | 0.00% | 0.1% | 36 | 1,325 | 98 | 312 | 468 | src/code/z_message.c |
| `Message_OpenText` | 0.00% | 0.0% | 8 | 2,463 | 144 | 131 | 224 | src/code/z_message.c |
| `Message_ShouldAdvance` | 0.00% | 0.0% | 74 | 244 | 16 | 66 | 233 | src/code/z_message.c |
| `Font_LoadChar` | 0.00% | 0.0% | 1,020 | 11 | 8 | 0 | 58 | src/code/z_kanfont.c |
| `Message_StartTextbox` | 0.00% | 0.0% | 8 | 406 | 21 | 35 | 32 | src/code/z_message.c |
| `Message_SetTables` | 0.00% | 0.0% | 5 | 323 | 17 | 10 | 15 | src/code/z_message.c |
| `Font_LoadMessageBoxIcon` | 0.00% | 0.0% | 18 | 55 | 7 | 0 | 18 | src/code/z_kanfont.c |
| `Message_CloseTextbox.part.0` | 0.00% | 0.0% | 4 | 222 | 23 | 0 | 16 | src/code/z_message.c |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Message_DrawTextChar` (src/code/z_message.c): 424: 31%, 192,818 data fills; 431: 16%, 80,793 data fills; 439: 16%, 83,927 data fills; 421: 8%; 438: 7%, 28,212 data fills

## Display list setup: 0.73%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_rcp.c | 0.44% | 60.4% | 202,845 | 431,518 |
| gfxprint.c | 0.24% | 33.5% | 124,875 | 204,995 |
| sys_cfb.c | 0.02% | 2.2% | 10,007 | 15,015 |
| sys_ucode.c | 0.01% | 1.6% | 5,527 | 10,548 |
| gfxalloc.c | 0.01% | 1.5% | 3,236 | 10,400 |
| TwoHeadGfxArena.c | 0.01% | 0.9% | 0 | 10,003 |
| z_vimode.c | 0.00% | 0.0% | 56 | 65 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Gfx_SetupFrame` | 0.23% | 30.8% | 5,000 | 3,923 | 336 | 112,229 | 217,095 | src/code/z_rcp.c |
| `GfxPrint_Setup` | 0.21% | 29.0% | 5,000 | 3,693 | 340 | 112,401 | 169,997 | src/libu64/gfxprint.c |
| `Gfx_SetFog2` | 0.04% | 5.3% | 8,442 | 402 | 44 | 8,442 | 38,001 | src/code/z_rcp.c |
| `Gfx_TwoTexScroll` | 0.03% | 4.8% | 8,103 | 373 | 60 | 24,319 | 27,111 | src/code/z_rcp.c |
| `Gfx_SetupDL_39Overlay` | 0.03% | 3.6% | 18,199 | 127 | 9 | 18,181 | 15,868 | src/code/z_rcp.c |
| `Gfx_SetupDL_25Opa` | 0.02% | 2.7% | 14,429 | 120 | 9 | 3,288 | 28,198 | src/code/z_rcp.c |
| `Gfx_SetupDL_25Xlu` | 0.02% | 2.6% | 11,184 | 151 | 9 | 7,886 | 19,826 | src/code/z_rcp.c |
| `GfxPrint_Init` | 0.02% | 2.5% | 5,001 | 321 | 21 | 7,544 | 19,992 | src/libu64/gfxprint.c |
| `func_80093C80` | 0.02% | 2.1% | 6,901 | 196 | 16 | 4,970 | 20,703 | src/code/z_rcp.c |
| `Gfx_SetupDL_27Xlu` | 0.01% | 1.6% | 8,852 | 114 | 9 | 3,829 | 13,055 | src/code/z_rcp.c |
| `Gfx_SetupDL` | 0.01% | 1.4% | 9,108 | 101 | 11 | 5,283 | 9,853 | src/code/z_rcp.c |
| `func_800947AC` | 0.01% | 1.4% | 4,221 | 217 | 12 | 3,645 | 12,663 | src/code/z_rcp.c |
| `GfxPrint_Open` | 0.01% | 1.4% | 5,000 | 181 | 14 | 4,695 | 10,001 | src/libu64/gfxprint.c |
| `Gfx_SetupDL_42Overlay` | 0.01% | 1.2% | 7,897 | 98 | 9 | 3,937 | 7,925 | src/code/z_rcp.c |
| `SysCfb_GetFbPtr` | 0.01% | 1.2% | 5,002 | 149 | 12 | 4,998 | 10,005 | src/code/sys_cfb.c |
| `SysCfb_GetFbCount` | 0.01% | 1.0% | 5,001 | 124 | 3 | 5,001 | 5,001 | src/code/sys_cfb.c |
| `Gfx_Open` | 0.01% | 0.8% | 23,715 | 21 | 2 | 0 | 9,245 | src/code/gfxalloc.c |
| `Gfx_Close` | 0.01% | 0.7% | 23,715 | 20 | 5 | 3,236 | 1,155 | src/code/gfxalloc.c |
| `SysUcode_GetUCode` | 0.00% | 0.6% | 5,526 | 74 | 3 | 5,000 | 0 | src/code/sys_ucode.c |
| `GfxPrint_Close` | 0.00% | 0.5% | 5,000 | 61 | 6 | 219 | 4,998 | src/libu64/gfxprint.c |
| `Gfx_SetupDL_57Opa` | 0.00% | 0.5% | 2,950 | 104 | 9 | 564 | 4,814 | src/code/z_rcp.c |
| `Gfx_SetupDL_39Ptr` | 0.00% | 0.5% | 3,649 | 81 | 9 | 2,020 | 2,287 | src/code/z_rcp.c |
| `Gfx_SetupDL_40Opa` | 0.00% | 0.4% | 2,398 | 118 | 9 | 565 | 4,796 | src/code/z_rcp.c |
| `THGA_Init` | 0.00% | 0.4% | 20,004 | 14 | 2 | 0 | 5,003 | src/code/TwoHeadGfxArena.c |
| `SysUcode_GetUCodeBootSize` | 0.00% | 0.4% | 5,075 | 53 | 6 | 0 | 5,010 | src/code/sys_ucode.c |
| (16 more functions) | 0.02% | 2.2% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Gfx_SetupFrame` (src/code/z_rcp.c): 1522: 7%; 1536: 6%, 8,844 data fills; 1521: 6%, 10,550 data fills; 1523: 6%, 5,005 data fills; 1526: 5%, 8,674 data fills

## Profiler and test harness (not in a release): 0.48%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| fishmod_debug.c | 0.40% | 83.8% | 345,899 | 235,276 |
| fishmod_debug_inputrec.inc.c | 0.07% | 15.7% | 30,078 | 79,812 |
| fishmod_debug_rdpbench.inc.c | 0.00% | 0.5% | 0 | 4,221 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Fishmod_DebugPlayUpdate` | 0.37% | 78.4% | 4,222 | 7,697 | 1,246 | 335,402 | 205,124 | src/code/fishmod_debug.c |
| `Fishmod_InputRecInput` | 0.07% | 14.2% | 5,001 | 1,180 | 96 | 27,645 | 73,187 | src/code/fishmod_debug_inputrec.inc.c |
| `Fishmod_AutoplayInput` | 0.02% | 4.1% | 5,001 | 338 | 19 | 10,004 | 20,011 | src/code/fishmod_debug.c |
| `InputPlay_Peek` | 0.01% | 1.4% | 3,352 | 171 | 30 | 2,147 | 6,354 | src/code/fishmod_debug_inputrec.inc.c |
| `Fishmod_DebugDrawText` | 0.00% | 0.6% | 5,000 | 50 | 2 | 0 | 5,000 | src/code/fishmod_debug.c |
| `Fishmod_DebugWantsFish` | 0.00% | 0.6% | 2,381 | 102 | 5 | 0 | 4,762 | src/code/fishmod_debug.c |
| `Fishmod_RdpBenchDraw` | 0.00% | 0.5% | 4,221 | 50 | 2 | 0 | 4,221 | src/code/fishmod_debug_rdpbench.inc.c |
| `Fishmod_OverlayLoaded` | 0.00% | 0.1% | 49 | 1,120 | 224 | 483 | 358 | src/code/fishmod_debug.c |
| `Fishmod_InputRecState` | 0.00% | 0.1% | 9 | 3,065 | 459 | 215 | 190 | src/code/fishmod_debug_inputrec.inc.c |
| `InputPlay_Read` | 0.00% | 0.0% | 24 | 332 | 39 | 61 | 66 | src/code/fishmod_debug_inputrec.inc.c |
| `Fishmod_DebugColNodes` | 0.00% | 0.0% | 5 | 228 | 17 | 5 | 15 | src/code/fishmod_debug.c |
| `InputPlay_Setup.part.0` | 0.00% | 0.0% | 1 | 1,024 | 52 | 6 | 10 | src/code/fishmod_debug_inputrec.inc.c |
| `Fishmod_InputRecSram` | 0.00% | 0.0% | 1 | 437 | 30 | 4 | 5 | src/code/fishmod_debug_inputrec.inc.c |
| `Fishmod_DebugNormalBoot` | 0.00% | 0.0% | 1 | 360 | 14 | 3 | 3 | src/code/fishmod_debug.c |
| `Fishmod_DebugPrintf` | 0.00% | 0.0% | 1 | 140 | 3 | 1 | 2 | src/code/fishmod_debug.c |
| `Fishmod_DebugRosterCount` | 0.00% | 0.0% | 1 | 131 | 3 | 1 | 1 | src/code/fishmod_debug.c |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Fishmod_DebugPlayUpdate` (src/code/fishmod_debug.c): 1235: 90%, 324,916 data fills; 1201: 2%, 4,222 data fills; 1466: 2%; 1146: 2%; 1365: 1%

## Other (no source): 0.44%

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `_icon_item_jpn_staticSegmentDataEnd` | 0.36% | 81.9% | 0 | 0 | 0 | 65,536 | 51,062 |  |
| `_n64ddSegmentBssEnd` | 0.08% | 18.0% | 0 | 0 | 0 | 57,734 | 41,259 |  |
| `gDekuTreeMeetingCs` | 0.00% | 0.0% | 0 | 0 | 0 | 61 | 194 |  |
| `_ovl_En_MagSegmentRoDataEnd` | 0.00% | 0.0% | 59 | 204 | 88 | 32 | 29 |  |
| `_ovl_En_KanbanSegmentRoDataEnd` | 0.00% | 0.0% | 30 | 197 | 66 | 48 | 23 |  |
| `gPauseMapMarkDataTable` | 0.00% | 0.0% | 0 | 0 | 0 | 14 | 33 |  |
| `_ovl_titleSegmentRoDataEnd` | 0.00% | 0.0% | 0 | 0 | 0 | 3 | 4 |  |
| `_ovl_titleSegmentRoDataStart` | 0.00% | 0.0% | 1 | 49 | 1 | 0 | 1 |  |

## Effects: 0.44%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_effect.c | 0.25% | 58.2% | 267,892 | 80,359 |
| z_effect_soft_sprite.c | 0.12% | 27.3% | 75,000 | 89,506 |
| z_eff_blure.c | 0.03% | 6.1% | 10,346 | 29,610 |
| ovl_Effect_Ss_KiraKira | 0.03% | 5.9% | 17,729 | 6,321 |
| ovl_Effect_Ss_Dust | 0.01% | 2.1% | 6,732 | 2,625 |
| z_effect_soft_sprite_old_init.c | 0.00% | 0.4% | 426 | 2,065 |
| z_eff_shield_particle.c | 0.00% | 0.0% | 20 | 12 |
| z_eff_spark.c | 0.00% | 0.0% | 0 | 4 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Effect_UpdateAll` | 0.13% | 29.1% | 4,221 | 2,616 | 230 | 135,085 | 46,445 | src/code/z_effect.c |
| `Effect_DrawAll` | 0.13% | 29.0% | 4,221 | 2,607 | 218 | 132,517 | 33,809 | src/code/z_effect.c |
| `EffectSs_DrawAll` | 0.06% | 14.5% | 4,221 | 1,301 | 127 | 34,882 | 50,196 | src/code/z_effect_soft_sprite.c |
| `EffectSs_UpdateAll` | 0.04% | 8.3% | 4,221 | 743 | 101 | 20,637 | 31,949 | src/code/z_effect_soft_sprite.c |
| `EffectSsKiraKira_Draw` | 0.02% | 4.7% | 2,160 | 816 | 131 | 14,917 | 3,252 | src/overlays/effects/ovl_Effect_Ss_KiraKira/z_eff_ss_kirakira.c |
| `EffectBlure_Update` | 0.01% | 3.1% | 4,222 | 275 | 17 | 4,222 | 16,885 | src/code/z_eff_blure.c |
| `EffectBlure_Draw` | 0.01% | 3.0% | 4,221 | 266 | 14 | 5,977 | 12,665 | src/code/z_eff_blure.c |
| `EffectSs_Update` | 0.01% | 2.8% | 4,683 | 224 | 31 | 13,944 | 1,895 | src/code/z_effect_soft_sprite.c |
| `EffectSsDust_Draw` | 0.01% | 1.9% | 737 | 962 | 184 | 6,597 | 1,852 | src/overlays/effects/ovl_Effect_Ss_Dust/z_eff_ss_dust.c |
| `func_809AABF0` | 0.00% | 1.0% | 2,159 | 174 | 40 | 2,774 | 1,729 | src/overlays/effects/ovl_Effect_Ss_KiraKira/z_eff_ss_kirakira.c |
| `EffectSs_Delete` | 0.00% | 0.6% | 401 | 544 | 36 | 1,758 | 2,086 | src/code/z_effect_soft_sprite.c |
| `EffectSs_InitInfo` | 0.00% | 0.5% | 5 | 37,080 | 2,958 | 2,335 | 50 | src/code/z_effect_soft_sprite.c |
| `EffectSs_Spawn` | 0.00% | 0.5% | 206 | 895 | 82 | 743 | 2,490 | src/code/z_effect_soft_sprite.c |
| `EffectSsKiraKira_SpawnDispersed` | 0.00% | 0.3% | 137 | 930 | 86 | 286 | 1,651 | src/code/z_effect_soft_sprite_old_init.c |
| `EffectSsKiraKira_Init` | 0.00% | 0.2% | 0 | 0 | 0 | 38 | 1,340 | src/overlays/effects/ovl_Effect_Ss_KiraKira/z_eff_ss_kirakira.c |
| `EffectSsDust_Update` | 0.00% | 0.2% | 737 | 107 | 40 | 77 | 505 | src/overlays/effects/ovl_Effect_Ss_Dust/z_eff_ss_dust.c |
| `EffectSs_FindSlot` | 0.00% | 0.2% | 206 | 366 | 19 | 546 | 824 | src/code/z_effect_soft_sprite.c |
| `func_800287AC` | 0.00% | 0.1% | 69 | 510 | 42 | 140 | 414 | src/code/z_effect_soft_sprite_old_init.c |
| `EffectSsDust_Init` | 0.00% | 0.0% | 0 | 0 | 0 | 58 | 266 | src/overlays/effects/ovl_Effect_Ss_Dust/z_eff_ss_dust.c |
| `Effect_InitContext` | 0.00% | 0.0% | 5 | 2,132 | 144 | 140 | 20 | src/code/z_effect.c |
| `EffectSs_ClearAll` | 0.00% | 0.0% | 4 | 2,334 | 207 | 155 | 16 | src/code/z_effect_soft_sprite.c |
| `Effect_DeleteAll` | 0.00% | 0.0% | 4 | 2,243 | 208 | 140 | 24 | src/code/z_effect.c |
| `EffectBlure_InitElements` | 0.00% | 0.0% | 5 | 1,767 | 183 | 125 | 20 | src/code/z_eff_blure.c |
| `EffectBlure_Init2` | 0.00% | 0.0% | 5 | 688 | 81 | 22 | 35 | src/code/z_eff_blure.c |
| `Effect_Add` | 0.00% | 0.0% | 5 | 626 | 58 | 10 | 45 | src/code/z_effect.c |
| (5 more functions) | 0.00% | 0.0% | | | | | | |

## Scenes, rooms and objects: 0.41%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_room.c | 0.27% | 65.2% | 123,657 | 154,413 |
| z_scene_table.c | 0.07% | 18.2% | 37,714 | 78,946 |
| z_scene.c | 0.07% | 16.6% | 43,437 | 39,205 |
| z_path.c | 0.00% | 0.0% | 1 | 8 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Room_DrawCullable` | 0.21% | 50.1% | 2,680 | 6,713 | 1,630 | 99,755 | 83,129 | src/code/z_room.c |
| `Scene_DrawConfigKokiriForest` | 0.04% | 10.5% | 1,845 | 2,050 | 182 | 21,224 | 46,126 | src/code/z_scene_table.c |
| `Object_UpdateEntries` | 0.04% | 9.4% | 4,221 | 802 | 78 | 24,704 | 25,358 | src/code/z_scene.c |
| `Object_IsLoaded` | 0.03% | 6.8% | 72,758 | 34 | 7 | 17,688 | 12,842 | src/code/z_scene.c |
| `Room_DrawImageSingle` | 0.02% | 5.9% | 1,541 | 1,365 | 126 | 6,212 | 30,443 | src/code/z_room.c |
| `Room_Draw` | 0.02% | 5.1% | 8,442 | 218 | 19 | 12,767 | 21,244 | src/code/z_room.c |
| `Scene_DrawConfigHyruleField` | 0.02% | 3.7% | 835 | 1,597 | 141 | 5,892 | 17,075 | src/code/z_scene_table.c |
| `Scene_Draw` | 0.01% | 2.9% | 4,221 | 251 | 12 | 7,516 | 12,663 | src/code/z_scene_table.c |
| `Room_ProcessRoomRequest` | 0.01% | 1.6% | 4,226 | 134 | 5 | 4,248 | 4,258 | src/code/z_room.c |
| `Room_DrawBackground2D` | 0.00% | 1.2% | 526 | 795 | 103 | 16 | 6,839 | src/code/z_room.c |
| `Scene_DrawConfigDefault` | 0.00% | 1.0% | 1,541 | 222 | 15 | 3,082 | 3,082 | src/code/z_scene_table.c |
| `func_80095AA0` | 0.00% | 0.6% | 8,442 | 26 | 2 | 0 | 4,220 | src/code/z_room.c |
| `Room_DrawImage` | 0.00% | 0.5% | 1,541 | 105 | 7 | 0 | 3,082 | src/code/z_room.c |
| `Room_DecodeJpeg` | 0.00% | 0.2% | 526 | 144 | 7 | 527 | 1,057 | src/code/z_room.c |
| `Object_GetSlot` | 0.00% | 0.1% | 225 | 161 | 41 | 273 | 189 | src/code/z_scene.c |
| `Scene_CommandAlternateHeaderList` | 0.00% | 0.1% | 11 | 1,786 | 160 | 179 | 127 | src/code/z_scene.c |
| `Object_LoadPersistent` | 0.00% | 0.0% | 15 | 988 | 48 | 112 | 116 | src/code/z_scene.c |
| `Object_InitContext` | 0.00% | 0.0% | 5 | 2,299 | 101 | 120 | 43 | src/code/z_scene.c |
| `Scene_CommandObjectList` | 0.00% | 0.0% | 6 | 1,575 | 182 | 79 | 81 | src/code/z_scene.c |
| `Room_RequestNewRoom.part.0` | 0.00% | 0.0% | 6 | 1,522 | 79 | 73 | 72 | src/code/z_room.c |
| `Scene_CommandTimeSettings` | 0.00% | 0.0% | 6 | 1,420 | 96 | 51 | 85 | src/code/z_scene.c |
| `Scene_ExecuteCommands` | 0.00% | 0.0% | 11 | 605 | 35 | 44 | 71 | src/code/z_scene.c |
| `Room_SetupFirstRoom` | 0.00% | 0.0% | 5 | 1,183 | 69 | 44 | 57 | src/code/z_room.c |
| `Scene_CommandPlayerEntryList` | 0.00% | 0.0% | 5 | 880 | 41 | 43 | 27 | src/code/z_scene.c |
| `Scene_CommandCollisionHeader` | 0.00% | 0.0% | 5 | 836 | 73 | 18 | 50 | src/code/z_scene.c |
| (22 more functions) | 0.00% | 0.1% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Room_DrawCullable` (src/code/z_room.c): 192: 21%, 44,760 data fills; 205: 12%, 30,896 data fills; 215: 8%; 196: 5%; 252: 4%, 4,897 data fills

## Pause menu and screen effects: 0.16%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| jpegdecoder.c | 0.09% | 56.1% | 13,449 | 319 |
| PreRender.c | 0.02% | 15.5% | 21,109 | 8,475 |
| z_jpeg.c | 0.02% | 11.3% | 22,329 | 892 |
| z_kaleido_scope_call.c | 0.02% | 10.4% | 8,475 | 16,933 |
| z_kaleido_setup.c | 0.01% | 6.3% | 4,450 | 11,088 |
| jpegutils.c | 0.00% | 0.1% | 94 | 26 |
| z_kaleido_manager.c | 0.00% | 0.1% | 146 | 159 |
| ovl_kaleido_scope | 0.00% | 0.1% | 75 | 149 |
| z_vismono.c | 0.00% | 0.0% | 7 | 40 |
| z_viszbuffer.c | 0.00% | 0.0% | 9 | 22 |
| z_viscvg.c | 0.00% | 0.0% | 9 | 17 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `JpegDecoder_ParseNextSymbol` | 0.04% | 26.2% | 35,357 | 103 | 84 | 3,241 | 26 | src/code/jpegdecoder.c |
| `PreRender_SetValues` | 0.02% | 15.5% | 4,227 | 510 | 12 | 21,109 | 8,451 | src/code/PreRender.c |
| `JpegDecoder_ProcessMcu` | 0.02% | 15.2% | 1,800 | 1,175 | 778 | 7,691 | 25 | src/code/jpegdecoder.c |
| `JpegDecoder_ReadBits` | 0.02% | 14.2% | 68,702 | 29 | 22 | 1,973 | 34 | src/code/jpegdecoder.c |
| `KaleidoScopeCall_LoadPlayer` | 0.02% | 10.4% | 8,452 | 172 | 12 | 8,460 | 16,909 | src/code/z_kaleido_scope_call.c |
| `Jpeg_CopyToZbuffer` | 0.01% | 9.3% | 300 | 4,317 | 569 | 19,959 | 186 | src/code/z_jpeg.c |
| `KaleidoSetup_Update` | 0.01% | 6.3% | 2,206 | 396 | 27 | 4,375 | 11,030 | src/code/z_kaleido_setup.c |
| `Jpeg_ParseMarkers` | 0.00% | 1.3% | 1 | 175,611 | 91,230 | 1,437 | 11 | src/code/z_jpeg.c |
| `Jpeg_ScheduleDecoderTask` | 0.00% | 0.6% | 75 | 1,095 | 107 | 847 | 490 | src/code/z_jpeg.c |
| `JpegDecoder_Decode` | 0.00% | 0.5% | 75 | 951 | 315 | 544 | 234 | src/code/jpegdecoder.c |
| `Jpeg_Decode` | 0.00% | 0.1% | 1 | 18,578 | 4,420 | 86 | 205 | src/code/z_jpeg.c |
| `JpegUtils_ProcessHuffmanTable` | 0.00% | 0.1% | 4 | 3,119 | 2,192 | 41 | 13 | src/code/jpegutils.c |
| `KaleidoManager_GetRamAddr` | 0.00% | 0.0% | 30 | 224 | 29 | 40 | 80 | src/code/z_kaleido_manager.c |
| `KaleidoSetup_Init` | 0.00% | 0.0% | 5 | 1,188 | 70 | 75 | 50 | src/code/z_kaleido_setup.c |
| `KaleidoScope_DrawWorldMap` | 0.00% | 0.0% | 0 | 0 | 0 | 32 | 57 | src/overlays/misc/ovl_kaleido_scope/z_kaleido_map.c |
| `KaleidoManager_ClearOvl` | 0.00% | 0.0% | 5 | 689 | 20 | 30 | 20 | src/code/z_kaleido_manager.c |
| `KaleidoManager_Destroy` | 0.00% | 0.0% | 4 | 710 | 24 | 26 | 19 | src/code/z_kaleido_manager.c |
| `JpegUtils_ProcessQuantizationTable` | 0.00% | 0.0% | 3 | 940 | 335 | 33 | 3 | src/code/jpegutils.c |
| `KaleidoManager_LoadOvl` | 0.00% | 0.0% | 5 | 544 | 23 | 30 | 20 | src/code/z_kaleido_manager.c |
| `JpegUtils_SetHuffmanTable` | 0.00% | 0.0% | 4 | 678 | 275 | 20 | 10 | src/code/jpegutils.c |
| `KaleidoScope_DrawDungeonMap` | 0.00% | 0.0% | 0 | 0 | 0 | 17 | 21 | src/overlays/misc/ovl_kaleido_scope/z_kaleido_map.c |
| `KaleidoScope_DrawInventoryEditor` | 0.00% | 0.0% | 0 | 0 | 0 | 11 | 25 | src/overlays/misc/ovl_kaleido_scope/z_kaleido_debug.c |
| `KaleidoManager_Init` | 0.00% | 0.0% | 5 | 381 | 25 | 20 | 20 | src/code/z_kaleido_manager.c |
| `KaleidoScope_Update` | 0.00% | 0.0% | 0 | 0 | 0 | 9 | 25 | src/overlays/misc/ovl_kaleido_scope/z_kaleido_scope.c |
| `KaleidoScopeCall_Init` | 0.00% | 0.0% | 5 | 334 | 19 | 15 | 20 | src/code/z_kaleido_scope_call.c |
| (14 more functions) | 0.00% | 0.1% | | | | | | |

## Memory: 0.15%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_memory_utils.c | 0.12% | 80.5% | 79,468 | 8,581 |
| TwoHeadArena.c | 0.02% | 13.1% | 15,305 | 15,106 |
| __osMalloc_n64.c | 0.01% | 6.0% | 9,313 | 2,112 |
| malloc.c | 0.00% | 0.2% | 30 | 502 |
| z_malloc.c | 0.00% | 0.2% | 0 | 412 |
| gamealloc.c | 0.00% | 0.0% | 30 | 74 |
| listalloc.c | 0.00% | 0.0% | 4 | 17 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `MemCopy` | 0.12% | 80.5% | 133,081 | 77 | 28 | 79,468 | 8,581 | src/code/z_memory_utils.c |
| `THA_Init` | 0.02% | 10.3% | 20,018 | 65 | 6 | 15,224 | 10,018 | src/code/TwoHeadArena.c |
| `ArenaImpl_GetSizes` | 0.00% | 2.9% | 269 | 1,374 | 155 | 4,960 | 548 | src/libc64/__osMalloc_n64.c |
| `THA_IsCrash` | 0.00% | 2.7% | 15,000 | 23 | 5 | 3 | 4,998 | src/code/TwoHeadArena.c |
| `__osMalloc` | 0.00% | 2.4% | 227 | 1,318 | 165 | 3,411 | 1,158 | src/libc64/__osMalloc_n64.c |
| `__osFree` | 0.00% | 0.6% | 227 | 343 | 47 | 804 | 295 | src/libc64/__osMalloc_n64.c |
| `SystemArena_GetSizes` | 0.00% | 0.2% | 137 | 164 | 14 | 1 | 411 | src/libc64/malloc.c |
| `ZeldaArena_GetSizes` | 0.00% | 0.1% | 132 | 102 | 6 | 0 | 264 | src/code/z_malloc.c |
| `__osMallocR` | 0.00% | 0.1% | 9 | 1,417 | 95 | 126 | 85 | src/libc64/__osMalloc_n64.c |
| `THA_AllocTailAlign16` | 0.00% | 0.1% | 111 | 72 | 7 | 67 | 53 | src/code/TwoHeadArena.c |
| `ZeldaArena_Malloc` | 0.00% | 0.0% | 196 | 30 | 4 | 0 | 107 | src/code/z_malloc.c |
| `SystemArena_Free` | 0.00% | 0.0% | 41 | 98 | 16 | 26 | 34 | src/libc64/malloc.c |
| `SystemArena_Malloc` | 0.00% | 0.0% | 31 | 105 | 18 | 3 | 48 | src/libc64/malloc.c |
| `GameAlloc_Malloc` | 0.00% | 0.0% | 14 | 170 | 24 | 3 | 35 | src/code/gamealloc.c |
| `GameAlloc_Cleanup` | 0.00% | 0.0% | 8 | 282 | 20 | 14 | 23 | src/code/gamealloc.c |
| `__osMallocInit` | 0.00% | 0.0% | 6 | 328 | 18 | 12 | 18 | src/libc64/__osMalloc_n64.c |
| `ZeldaArena_Free` | 0.00% | 0.0% | 186 | 10 | 4 | 0 | 22 | src/code/z_malloc.c |
| `THA_AllocTailAlign` | 0.00% | 0.0% | 25 | 56 | 6 | 6 | 15 | src/code/TwoHeadArena.c |
| `GameAlloc_Free` | 0.00% | 0.0% | 5 | 184 | 11 | 10 | 5 | src/code/gamealloc.c |
| `THA_GetRemaining` | 0.00% | 0.0% | 5 | 173 | 4 | 5 | 10 | src/code/TwoHeadArena.c |
| `GameAlloc_Init` | 0.00% | 0.0% | 9 | 88 | 4 | 3 | 11 | src/code/gamealloc.c |
| `ListAlloc_FreeAll` | 0.00% | 0.0% | 4 | 189 | 5 | 4 | 12 | src/code/listalloc.c |
| `THA_Destroy` | 0.00% | 0.0% | 13 | 47 | 3 | 0 | 12 | src/code/TwoHeadArena.c |
| `SystemArena_MallocR` | 0.00% | 0.0% | 3 | 170 | 18 | 0 | 9 | src/libc64/malloc.c |
| `ZeldaArena_Init` | 0.00% | 0.0% | 5 | 101 | 5 | 0 | 10 | src/code/z_malloc.c |
| (4 more functions) | 0.00% | 0.0% | | | | | | |

## Screen transitions: 0.03%

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `TransitionFade_Update` | 0.02% | 61.5% | 4,372 | 363 | 25 | 6,774 | 22,233 | src/code/z_fbdemo_fade.c |
| `TransitionFade_Draw` | 0.01% | 37.7% | 4,507 | 216 | 7 | 2,040 | 17,440 | src/code/z_fbdemo_fade.c |
| `TransitionFade_IsDone` | 0.00% | 0.6% | 159 | 91 | 2 | 159 | 159 | src/code/z_fbdemo_fade.c |
| `TransitionFade_Init` | 0.00% | 0.1% | 13 | 200 | 14 | 5 | 39 | src/code/z_fbdemo_fade.c |
| `TransitionFade_Start` | 0.00% | 0.1% | 13 | 110 | 10 | 0 | 26 | src/code/z_fbdemo_fade.c |
| `TransitionFade_SetType` | 0.00% | 0.1% | 13 | 109 | 7 | 0 | 27 | src/code/z_fbdemo_fade.c |
| `TransitionFade_SetColor` | 0.00% | 0.0% | 13 | 33 | 2 | 0 | 8 | src/code/z_fbdemo_fade.c |
| `TransitionFade_Destroy` | 0.00% | 0.0% | 12 | 34 | 2 | 0 | 8 | src/code/z_fbdemo_fade.c |

## Save data and inventory: 0.00%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_sram.c | 0.00% | 63.4% | 1,255 | 135 |
| z_ss_sram.c | 0.00% | 36.5% | 521 | 1,505 |
| z_common_data.c | 0.00% | 0.2% | 3 | 5 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Sram_VerifyAndLoadAllSaves` | 0.00% | 54.5% | 1 | 183,992 | 111,643 | 1,123 | 38 | src/code/z_sram.c |
| `SsSram_ReadWrite` | 0.00% | 23.1% | 137 | 570 | 54 | 508 | 739 | src/code/z_ss_sram.c |
| `SsSram_Dma` | 0.00% | 13.3% | 137 | 328 | 42 | 13 | 766 | src/code/z_ss_sram.c |
| `Sram_InitSave` | 0.00% | 5.2% | 1 | 17,526 | 12,547 | 30 | 20 | src/code/z_sram.c |
| `Sram_InitNewSave` | 0.00% | 2.1% | 4 | 1,735 | 161 | 59 | 28 | src/code/z_sram.c |
| `Sram_InitDebugSave` | 0.00% | 0.6% | 1 | 2,010 | 176 | 15 | 12 | src/code/z_sram.c |
| `Sram_OpenSave` | 0.00% | 0.4% | 1 | 1,432 | 98 | 15 | 14 | src/code/z_sram.c |
| `Sram_InitSram` | 0.00% | 0.4% | 1 | 1,390 | 130 | 12 | 14 | src/code/z_sram.c |
| `SaveContext_Init` | 0.00% | 0.2% | 1 | 518 | 30 | 3 | 5 | src/code/z_common_data.c |
| `Sram_Alloc` | 0.00% | 0.1% | 2 | 132 | 12 | 1 | 4 | src/code/z_sram.c |
| `Sram_Init` | 0.00% | 0.1% | 5 | 50 | 2 | 0 | 5 | src/code/z_sram.c |

## Other engine code: 0.00%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| kanji_tex.c | 0.00% | 55.2% | 1,055 | 100 |
| entry.c | 0.00% | 44.3% | 1,196 | 2 |
| driveexist.c | 0.00% | 0.5% | 2 | 8 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `KanjiTex_CodepointToOffset` | 0.00% | 55.2% | 1,652 | 68 | 22 | 1,055 | 100 | src/code/kanji_tex.c |
| `entrypoint` | 0.00% | 44.3% | 1 | 90,853 | 11,969 | 1,196 | 2 | src/makerom/entry.c |
| `LeoDriveExist` | 0.00% | 0.5% | 1 | 959 | 43 | 2 | 8 | src/libleo/api/driveexist.c |

