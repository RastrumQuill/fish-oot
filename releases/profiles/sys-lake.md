# CPU profile by system

- 1 exact profile(s) from ares
- 6,240,708,992 instructions, 391.1 s of CPU time on the emulator's clock (36,669,276,737 cycles); idle 5.1% of it
- data cache: 114,613,124 line fills, 64,655,878 write-backs; code cache: 85,491,093 line fills (a fill is about 40-48 cycles: 30.7% of all cycles)
- per frame (3850 frames): 3,985,140 cycles working (42.51 ms)
- the profiler and test harness's own code: 53.1% of the run
- shares below are of the time the game's own code was working (idle and the harness left out)

## Systems

| system | share | CPU s | instructions | cycles/instr | data fills | code fills | cache share of its time |
|---|---:|---:|---:|---:|---:|---:|---:|
| libultra (OS: threads, interrupts, PI, SI, VI, timers) | 25.06% | 41.02 | 778,156,140 | 4.94 | 28,950,550 | 14,861,102 | 69% |
| Audio | 20.72% | 33.91 | 853,484,043 | 3.72 | 22,978,001 | 12,598,635 | 63% |
| Fishing mod | 12.20% | 19.97 | 407,531,138 | 4.59 | 11,335,912 | 8,626,103 | 61% |
| Collision | 9.34% | 15.28 | 293,867,640 | 4.88 | 8,374,659 | 9,774,888 | 66% |
| Maths (matrices, vectors, trig, random) | 8.61% | 14.09 | 270,786,532 | 4.88 | 6,061,254 | 8,125,246 | 57% |
| Actor framework (z_actor.c) | 5.20% | 8.50 | 102,715,929 | 7.76 | 6,408,411 | 4,961,640 | 81% |
| Animation | 3.95% | 6.47 | 122,568,031 | 4.95 | 4,704,707 | 2,081,708 | 64% |
| Effects | 2.20% | 3.61 | 55,860,676 | 6.06 | 3,257,348 | 1,114,717 | 74% |
| Scheduler, threads and system | 2.13% | 3.48 | 36,705,797 | 8.89 | 1,715,064 | 3,515,458 | 82% |
| Sky, weather and lights | 1.70% | 2.78 | 32,449,195 | 8.05 | 1,322,627 | 2,550,521 | 78% |
| Link | 1.54% | 2.51 | 24,330,258 | 9.68 | 1,022,323 | 2,903,743 | 85% |
| Actors | 1.48% | 2.43 | 23,782,337 | 9.57 | 1,475,076 | 2,184,034 | 85% |
| HUD and map | 1.31% | 2.15 | 20,764,870 | 9.70 | 1,179,920 | 2,008,375 | 84% |
| Camera and view | 1.09% | 1.79 | 14,344,220 | 11.69 | 885,526 | 1,781,377 | 85% |
| C library (memory copies, printf, 64-bit maths) | 0.91% | 1.48 | 19,293,735 | 7.21 | 1,159,504 | 284,633 | 64% |
| Game loop (game.c, z_play.c) | 0.61% | 1.00 | 7,019,748 | 13.42 | 589,998 | 1,019,370 | 89% |
| Loading (DMA, decompression, overlays) | 0.57% | 0.93 | 34,263,706 | 2.54 | 576,040 | 139,756 | 50% |
| Display list setup | 0.46% | 0.75 | 8,179,629 | 8.64 | 406,338 | 693,272 | 81% |
| Other (no source) | 0.37% | 0.60 | 24,667,217 | 2.29 | 104,601 | 336,260 | 37% |
| Scenes, rooms and objects | 0.37% | 0.60 | 10,924,567 | 5.15 | 413,338 | 289,520 | 73% |
| Text | 0.05% | 0.09 | 876,364 | 9.59 | 36,342 | 109,446 | 86% |
| Memory | 0.05% | 0.08 | 738,128 | 10.38 | 104,941 | 19,135 | 87% |
| Pause menu and screen effects | 0.04% | 0.07 | 365,902 | 17.93 | 46,687 | 65,707 | 93% |
| Screen transitions | 0.01% | 0.02 | 113,408 | 19.31 | 5,357 | 34,944 | 92% |
| Cutscenes | 0.01% | 0.02 | 100,601 | 20.33 | 11,915 | 26,963 | 95% |
| Save data and inventory | 0.00% | 0.00 | 12,340 | 9.56 | 619 | 1,285 | 86% |
| Other engine code | 0.00% | 0.00 | 17,093 | 6.48 | 1,375 | 40 | 80% |

## Profiler and test harness (not in a release): 126.81%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| fishmod_debug_pcprof.inc.c | 119.99% | 94.6% | 1,844,001 | 1,954,084 |
| n64prof.c | 5.75% | 4.5% | 8,272,671 | 2,747,528 |
| fishmod_debug.c | 1.06% | 0.8% | 1,363,554 | 651,875 |
| fishmod_debug_bench.inc.c | 0.01% | 0.0% | 4,456 | 12,026 |
| fishmod_debug_rdpbench.inc.c | 0.00% | 0.0% | 0 | 7,698 |
| fishmod_debug_inputrec.inc.c | 0.00% | 0.0% | 0 | 3,851 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `PcProf_Update` | 118.06% | 93.1% | 3,850 | 4,704,657 | 234,631 | 340,381 | 79,151 | src/code/fishmod_debug_pcprof.inc.c |
| `n64prof_sample` | 4.54% | 3.6% | 687,472 | 1,014 | 276 | 6,884,552 | 1,430,999 | n64prof.c |
| `PcProf_Hook` | 1.15% | 0.9% | 687,488 | 256 | 43 | 714,892 | 1,450,834 | src/code/fishmod_debug_pcprof.inc.c |
| `Fishmod_DebugPlayUpdate` | 1.03% | 0.8% | 3,850 | 41,136 | 9,795 | 1,347,186 | 593,363 | src/code/fishmod_debug.c |
| `n64prof_count_into` | 0.98% | 0.8% | 687,399 | 218 | 32 | 956,040 | 1,297,760 | n64prof.c |
| `n64prof_pack` | 0.22% | 0.2% | 302 | 110,528 | 22,184 | 419,967 | 3,336 | n64prof.c |
| `PcCalib_LoadMiss` | 0.15% | 0.1% | 1 | 23,267,477 | 1,967,290 | 491,354 | 51 | src/code/fishmod_debug_pcprof.inc.c |
| `PcCalib_Div` | 0.14% | 0.1% | 1 | 21,937,922 | 823,422 | 1,015 | 212 | src/code/fishmod_debug_pcprof.inc.c |
| `PcCalib_LoadHit` | 0.14% | 0.1% | 1 | 21,404,252 | 16,524,102 | 19,237 | 65 | src/code/fishmod_debug_pcprof.inc.c |
| `PcCalib_FetchMiss` | 0.12% | 0.1% | 1 | 18,693,558 | 2,665,313 | 326 | 333,645 | src/code/fishmod_debug_pcprof.inc.c |
| `PcProf_Ranges` | 0.11% | 0.1% | 482 | 36,293 | 3,013 | 272,499 | 7,268 | src/code/fishmod_debug_pcprof.inc.c |
| `PcCalib_Alu` | 0.11% | 0.1% | 1 | 16,915,233 | 13,973,541 | 917 | 59,419 | src/code/fishmod_debug_pcprof.inc.c |
| `Fishmod_AutoplayInput` | 0.02% | 0.0% | 3,850 | 607 | 42 | 15,291 | 26,715 | src/code/fishmod_debug.c |
| `Fishmod_DebugPrintf` | 0.01% | 0.0% | 17,204 | 91 | 9 | 570 | 27,362 | src/code/fishmod_debug.c |
| `Sc64_Command` | 0.01% | 0.0% | 2,637 | 486 | 55 | 1,470 | 21,190 | src/code/fishmod_debug_pcprof.inc.c |
| `n64prof_frame_end` | 0.01% | 0.0% | 3,856 | 262 | 8 | 3,916 | 15,427 | n64prof.c |
| `PlayBench_Steer` | 0.01% | 0.0% | 1,943 | 496 | 41 | 4,184 | 11,782 | src/code/fishmod_debug_bench.inc.c |
| `n64prof_init` | 0.00% | 0.0% | 1 | 736,099 | 65,627 | 8,196 | 6 | n64prof.c |
| `Fishmod_RdpBenchDraw` | 0.00% | 0.0% | 3,870 | 97 | 2 | 0 | 7,698 | src/code/fishmod_debug_rdpbench.inc.c |
| `Sc64_UsbSend` | 0.00% | 0.0% | 414 | 492 | 37 | 1,895 | 2,104 | src/code/fishmod_debug_pcprof.inc.c |
| `Fishmod_DebugDrawText` | 0.00% | 0.0% | 3,859 | 50 | 2 | 0 | 3,850 | src/code/fishmod_debug.c |
| `Fishmod_InputRecInput` | 0.00% | 0.0% | 3,855 | 50 | 2 | 0 | 3,850 | src/code/fishmod_debug_inputrec.inc.c |
| `Fishmod_OverlayLoaded` | 0.00% | 0.0% | 31 | 1,609 | 317 | 402 | 288 | src/code/fishmod_debug.c |
| `PlayBench_Place` | 0.00% | 0.0% | 19 | 1,596 | 86 | 272 | 244 | src/code/fishmod_debug_bench.inc.c |
| `Watchdog_Entry` | 0.00% | 0.0% | 1 | 19,532 | 919 | 91 | 269 | src/code/fishmod_debug.c |
| (6 more functions) | 0.00% | 0.0% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `PcProf_Update` (src/code/fishmod_debug_pcprof.inc.c): 932: 100%; 987: 0%, 278,755 data fills; 769: 0%, 7,616 data fills; 808: 0%, 8,194 data fills; 909: 0%, 6,262 data fills
- `n64prof_sample` (n64prof.c): 99: 73%, 5,911,887 data fills; 98: 7%; 94: 4%, 284,249 data fills; 86: 4%, 262,810 data fills; 104: 3%
- `PcProf_Hook` (src/code/fishmod_debug_pcprof.inc.c): 636: 83%, 713,769 data fills; 637: 15%; 650: 2%
- `Fishmod_DebugPlayUpdate` (src/code/fishmod_debug.c): 1235: 81%, 1,142,735 data fills; 1420: 13%, 144,961 data fills; 1453: 1%, 16,329 data fills; 1458: 1%, 12,145 data fills; 1466: 1%, 7,076 data fills
- `n64prof_count_into` (n64prof.c): 132: 33%, 771,524 data fills; 128: 19%, 182,926 data fills; 129: 14%; 131: 12%; 133: 5%
- `n64prof_pack` (n64prof.c): 250: 62%, 279,145 data fills; 251: 26%, 138,472 data fills; 249: 10%; 252: 1%; 267: 0%, 840 data fills

## libultra (OS: threads, interrupts, PI, SI, VI, timers): 25.06%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| exceptasm.c | 15.69% | 62.6% | 23,824,376 | 7,078,880 |
| timerintr.c | 2.22% | 8.8% | 1,462,232 | 2,562,391 |
| writebackdcacheall.c | 2.09% | 8.3% | 0 | 33,639 |
| recvmesg.c | 1.59% | 6.3% | 1,580,979 | 645,081 |
| interrupt.c | 0.79% | 3.2% | 70,776 | 974,076 |
| sendmesg.c | 0.53% | 2.1% | 575,090 | 492,675 |
| vimgr.c | 0.36% | 1.4% | 432,668 | 384,502 |
| startthread.c | 0.20% | 0.8% | 205,071 | 298,509 |
| getcount.c | 0.14% | 0.6% | 0 | 122,284 |
| sptask.c | 0.13% | 0.5% | 59,498 | 294,868 |
| contreaddata.c | 0.13% | 0.5% | 146,196 | 168,547 |
| viswapcontext.c | 0.13% | 0.5% | 141,916 | 187,774 |
| gettime.c | 0.12% | 0.5% | 100,869 | 170,118 |
| setintmask.c | 0.11% | 0.4% | 35,048 | 84,975 |
| writebackdcache.c | 0.10% | 0.4% | 0 | 81,267 |
| controller.c | 0.08% | 0.3% | 82,261 | 94,413 |
| sirawdma.c | 0.08% | 0.3% | 29,127 | 179,698 |
| invaldcache.c | 0.05% | 0.2% | 0 | 103,962 |
| sprawdma.c | 0.05% | 0.2% | 22,682 | 102,885 |
| sp.c | 0.04% | 0.2% | 0 | 45,360 |
| virtualtophysical.c | 0.04% | 0.2% | 0 | 92,753 |
| contquery.c | 0.04% | 0.2% | 13,174 | 98,971 |
| setcompare.c | 0.04% | 0.2% | 0 | 79,565 |
| siacs.c | 0.04% | 0.1% | 32,456 | 72,349 |
| epirawread.c | 0.03% | 0.1% | 11,962 | 19,502 |
| (55 more) | 0.22% | 0.9% | | |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `__osDispatchThread` | 7.00% | 27.9% | 1,709,619 | 629 | 93 | 13,595,313 | 955,879 | src/libultra/os/exceptasm.c |
| `__osException` | 6.74% | 26.9% | 784,503 | 1,318 | 225 | 9,153,712 | 4,717,146 | src/libultra/os/exceptasm.c |
| `osWritebackDCacheAll` | 2.09% | 8.3% | 22,686 | 14,114 | 2,054 | 0 | 33,639 | src/libultra/os/writebackdcacheall.c |
| `osRecvMesg` | 1.59% | 6.3% | 1,074,181 | 227 | 52 | 1,580,979 | 645,081 | src/libultra/os/recvmesg.c |
| `__osTimerInterrupt` | 1.42% | 5.7% | 687,476 | 316 | 84 | 1,109,902 | 1,511,114 | src/libultra/os/timerintr.c |
| `__osEnqueueAndYield` | 1.02% | 4.1% | 925,115 | 170 | 63 | 210,090 | 922,597 | src/libultra/os/exceptasm.c |
| `__osInsertTimer` | 0.80% | 3.2% | 691,342 | 177 | 48 | 352,325 | 1,051,268 | src/libultra/os/timerintr.c |
| `__osEnqueueThread` | 0.70% | 2.8% | 2,557,342 | 42 | 14 | 743,193 | 244,828 | src/libultra/os/exceptasm.c |
| `__osDisableInt` | 0.59% | 2.4% | 2,962,654 | 31 | 15 | 70,776 | 764,699 | src/libultra/os/interrupt.c |
| `osSendMesg` | 0.53% | 2.1% | 236,187 | 343 | 48 | 575,090 | 492,675 | src/libultra/os/sendmesg.c |
| `viMgrMain` | 0.36% | 1.4% | 1 | 55,664,543 | 8,047,110 | 432,663 | 384,488 | src/libultra/io/vimgr.c |
| `__osPopThread` | 0.22% | 0.9% | 2,594,309 | 13 | 4 | 122,068 | 238,430 | src/libultra/os/exceptasm.c |
| `osStartThread` | 0.20% | 0.8% | 110,378 | 284 | 39 | 205,071 | 298,509 | src/libultra/os/startthread.c |
| `__osRestoreInt` | 0.20% | 0.8% | 2,962,546 | 10 | 7 | 0 | 209,377 | src/libultra/os/interrupt.c |
| `osGetCount` | 0.14% | 0.6% | 5,396,824 | 4 | 3 | 0 | 122,284 | src/libultra/os/getcount.c |
| `__osViSwapContext` | 0.13% | 0.5% | 11,838 | 1,672 | 129 | 141,916 | 187,774 | src/libultra/io/viswapcontext.c |
| `osGetTime` | 0.12% | 0.5% | 123,833 | 150 | 25 | 100,869 | 170,118 | src/libultra/os/gettime.c |
| `osSetIntMask` | 0.11% | 0.4% | 182,564 | 92 | 38 | 35,048 | 84,975 | src/libultra/os/setintmask.c |
| `osWritebackDCache` | 0.10% | 0.4% | 48,152 | 326 | 50 | 0 | 81,267 | src/libultra/os/writebackdcache.c |
| `__osSiRawStartDma` | 0.08% | 0.3% | 49,181 | 259 | 28 | 29,127 | 179,698 | src/libultra/io/sirawdma.c |
| `osSpTaskLoad` | 0.07% | 0.3% | 22,681 | 452 | 57 | 36,818 | 150,065 | src/libultra/io/sptask.c |
| `osInvalDCache` | 0.05% | 0.2% | 47,009 | 174 | 63 | 0 | 103,962 | src/libultra/os/invaldcache.c |
| `__osSpRawStartDma` | 0.05% | 0.2% | 45,361 | 170 | 30 | 22,682 | 102,885 | src/libultra/io/sprawdma.c |
| `_VirtualToPhysicalTask` | 0.05% | 0.2% | 22,680 | 340 | 42 | 22,680 | 114,357 | src/libultra/io/sptask.c |
| `osContGetReadData` | 0.05% | 0.2% | 11,785 | 624 | 76 | 58,925 | 47,625 | src/libultra/io/contreaddata.c |
| (76 more functions) | 0.63% | 2.5% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `osRecvMesg` (src/libultra/os/recvmesg.c): 30: 27%, 708,390 data fills; 20: 19%; 17: 18%, 472,214 data fills; 3: 12%, 138,600 data fills; 6: 8%, 200,458 data fills
- `__osTimerInterrupt` (src/libultra/os/timerintr.c): 34: 16%, 299,993 data fills; 55: 11%, 249,675 data fills; 49: 9%, 158,267 data fills; 51: 7%; 29: 7%
- `__osInsertTimer` (src/libultra/os/timerintr.c): 95: 29%, 191,147 data fills; 101: 21%, 153,890 data fills; 118: 14%; 108: 11%; 100: 11%

## Audio: 20.72%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| mixing (synthesis.c) | 8.52% | 41.1% | 8,034,183 | 3,838,265 |
| note playback | 3.91% | 18.9% | 4,821,680 | 2,009,265 |
| sequence player | 3.43% | 16.6% | 3,990,746 | 1,860,826 |
| note effects | 2.66% | 12.9% | 3,940,977 | 1,606,070 |
| game side (sound effects, music commands) | 0.96% | 4.7% | 774,537 | 1,503,683 |
| thread and tasks | 0.52% | 2.5% | 598,897 | 767,013 |
| loading (fonts, samples, sequences) | 0.39% | 1.9% | 430,455 | 636,472 |
| manager | 0.21% | 1.0% | 278,686 | 280,778 |
| heap and caches | 0.11% | 0.5% | 107,840 | 96,263 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `AudioSynth_ProcessNote` | 4.43% | 21.4% | 563,283 | 1,207 | 392 | 4,676,009 | 1,830,801 | src/audio/internal/synthesis.c |
| `AudioSeq_SequenceChannelProcessScript` | 2.09% | 10.1% | 466,386 | 688 | 261 | 2,467,666 | 605,307 | src/audio/internal/seqplayer.c |
| `Audio_ProcessNotes` | 2.00% | 9.7% | 35,338 | 8,705 | 1,549 | 3,121,364 | 1,125,760 | src/audio/internal/playback.c |
| `AudioSynth_DoOneAudioUpdate` | 1.85% | 8.9% | 35,335 | 8,028 | 2,353 | 1,328,586 | 710,560 | src/audio/internal/synthesis.c |
| `Audio_InitSampleState` | 1.73% | 8.3% | 563,409 | 471 | 145 | 1,506,757 | 641,630 | src/audio/internal/playback.c |
| `AudioSynth_ProcessEnvelope` | 0.90% | 4.3% | 563,279 | 244 | 111 | 634,764 | 238,310 | src/audio/internal/synthesis.c |
| `Audio_SequenceChannelProcessSound` | 0.76% | 3.7% | 813,118 | 144 | 31 | 1,123,627 | 507,914 | src/audio/internal/effects.c |
| `Audio_AdsrUpdate` | 0.64% | 3.1% | 563,372 | 173 | 44 | 1,075,006 | 289,428 | src/audio/internal/effects.c |
| `AudioSeq_SequencePlayerProcessSequence` | 0.55% | 2.6% | 70,251 | 1,192 | 149 | 852,518 | 498,059 | src/audio/internal/seqplayer.c |
| `Audio_NoteVibratoUpdate` | 0.53% | 2.6% | 563,380 | 145 | 17 | 1,114,979 | 92,462 | src/audio/internal/effects.c |
| `func_800DB03C` | 0.41% | 2.0% | 35,334 | 1,775 | 423 | 597,343 | 143,588 | src/audio/internal/synthesis.c |
| `AudioSynth_Update` | 0.38% | 1.8% | 11,779 | 4,915 | 695 | 221,546 | 417,914 | src/audio/internal/synthesis.c |
| `Audio_GetVibratoFreqScale` | 0.38% | 1.8% | 563,366 | 102 | 32 | 311,838 | 289,861 | src/audio/internal/effects.c |
| `AudioThread_UpdateImpl` | 0.36% | 1.7% | 11,842 | 4,698 | 287 | 420,808 | 540,609 | src/audio/internal/thread.c |
| `Audio_SequencePlayerProcessSound` | 0.33% | 1.6% | 70,254 | 716 | 211 | 270,712 | 397,057 | src/audio/internal/effects.c |
| `AudioSeq_HandleScriptFlowControl` | 0.26% | 1.3% | 1,295,354 | 31 | 18 | 83,618 | 161,904 | src/audio/internal/seqplayer.c |
| `Audio_ChooseActiveSfx` | 0.26% | 1.2% | 26,913 | 1,462 | 284 | 178,954 | 397,070 | src/audio/game/sfx.c |
| `AudioSynth_InitNextRingBuf` | 0.24% | 1.2% | 70,739 | 523 | 95 | 197,406 | 263,692 | src/audio/internal/synthesis.c |
| `AudioSeq_SeqLayerProcessScript` | 0.22% | 1.1% | 437,928 | 76 | 22 | 299,780 | 79,258 | src/audio/internal/seqplayer.c |
| `AudioMgr_HandleRetrace.part.0` | 0.16% | 0.8% | 11,785 | 2,034 | 126 | 242,213 | 175,723 | src/code/audio_thread_manager.c |
| `AudioSeq_ProcessSequences` | 0.13% | 0.6% | 35,337 | 584 | 87 | 129,986 | 219,778 | src/audio/internal/seqplayer.c |
| `Audio_SetSfxProperties` | 0.13% | 0.6% | 18,704 | 1,040 | 151 | 49,867 | 253,265 | src/audio/game/general.c |
| `AudioSynth_SaveReverbSamples` | 0.12% | 0.6% | 70,672 | 268 | 44 | 159,978 | 78,778 | src/audio/internal/synthesis.c |
| `AudioSynth_LoadRingBuffer1` | 0.11% | 0.5% | 70,673 | 228 | 35 | 128,159 | 71,927 | src/audio/internal/synthesis.c |
| `AudioLoad_ProcessAsyncLoads` | 0.10% | 0.5% | 11,784 | 1,320 | 117 | 201,719 | 85,424 | src/audio/internal/load.c |
| (181 more functions) | 1.66% | 8.0% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `AudioSynth_ProcessNote` (src/audio/internal/synthesis.c): 1073: 8%, 822,050 data fills; 974: 6%, 299,655 data fills; 1048: 5%, 391,571 data fills; 1349: 4%, 237,627 data fills; 1121: 4%, 411,353 data fills
- `AudioSeq_SequenceChannelProcessScript` (src/audio/internal/seqplayer.c): 1313: 15%, 540,182 data fills; 1297: 8%, 441,831 data fills; 1320: 7%, 230,130 data fills; 1286: 6%, 116,432 data fills; 1811: 5%, 297,923 data fills
- `Audio_ProcessNotes` (src/audio/internal/playback.c): 211: 10%, 453,689 data fills; 208: 9%, 573,597 data fills; 250: 8%, 366,994 data fills; 251: 7%, 277,393 data fills; 216: 6%, 303,829 data fills
- `AudioSynth_DoOneAudioUpdate` (src/audio/internal/synthesis.c): 929: 35%; 862: 17%, 481,918 data fills; 928: 7%, 227,702 data fills; 930: 6%, 118,085 data fills; 927: 4%, 93,803 data fills
- `Audio_InitSampleState` (src/audio/internal/playback.c): 54: 11%, 127,506 data fills; 51: 9%, 346,978 data fills; 38: 8%; 127: 7%; 48: 6%, 126,747 data fills
- `AudioSynth_ProcessEnvelope` (src/audio/internal/synthesis.c): 1534: 25%, 250,080 data fills; 1530: 8%, 63,044 data fills; 1490: 7%; 1485: 7%; 1456: 6%, 78,175 data fills
- `Audio_SequenceChannelProcessSound` (src/audio/internal/effects.c): 47: 23%, 373,622 data fills; 13: 23%, 401,838 data fills; 44: 8%, 83,379 data fills; 34: 8%; 58: 7%, 82,576 data fills
- `Audio_AdsrUpdate` (src/audio/internal/effects.c): 380: 27%, 555,630 data fills; 321: 13%, 184,162 data fills; 342: 12%, 150,670 data fills; 280: 9%; 352: 7%, 88,330 data fills

## Fishing mod: 12.20%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| ovl_En_Fishmod | 4.55% | 37.3% | 4,815,477 | 3,734,346 |
| props (scatter) | 3.39% | 27.8% | 3,310,619 | 1,508,828 |
| fish manager | 1.15% | 9.4% | 1,635,879 | 779,631 |
| fishmod.c | 1.07% | 8.7% | 549,418 | 1,119,543 |
| fish rigs | 1.05% | 8.6% | 838,256 | 732,492 |
| fishmod_trig.h | 0.52% | 4.2% | 67,056 | 142,035 |
| fishmod_tri.h | 0.36% | 2.9% | 24,886 | 408,138 |
| fishmod_items.c | 0.12% | 1.0% | 94,321 | 201,090 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `EnFishmod_ScatterDraw` | 1.82% | 14.9% | 3,855 | 72,469 | 17,882 | 1,894,826 | 943,415 | src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c |
| `Scatter_Build` | 1.40% | 11.4% | 803 | 266,751 | 79,855 | 1,273,155 | 414,559 | src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c |
| `EnFishmod_Update` | 0.80% | 6.6% | 191,034 | 643 | 149 | 842,367 | 668,558 | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_RigDraw` | 0.62% | 5.1% | 11,357 | 8,416 | 1,727 | 681,952 | 378,165 | src/overlays/actors/ovl_En_Fishmod/fishmod_rig.c |
| `EnFishmod_DrawBillboard` | 0.56% | 4.6% | 24,947 | 3,450 | 765 | 511,821 | 196,197 | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_FollowFar.constprop.0` | 0.53% | 4.4% | 122,369 | 670 | 124 | 694,942 | 213,101 | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `Fishmod_SinCosBinang` | 0.52% | 4.2% | 651,895 | 122 | 43 | 67,056 | 142,035 | fishmod_trig.h |
| `EnFishmod_ManagerDraw` | 0.51% | 4.2% | 3,849 | 20,388 | 2,089 | 817,887 | 393,540 | src/overlays/actors/ovl_En_Fishmod/fishmod_manager.c |
| `EnFishmod_FishAI` | 0.42% | 3.4% | 37,007 | 1,741 | 221 | 361,379 | 606,295 | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_ManagerUpdate` | 0.36% | 3.0% | 3,832 | 14,422 | 2,370 | 580,891 | 124,092 | src/overlays/actors/ovl_En_Fishmod/fishmod_manager.c |
| `Fishmod_WaterFogFactor` | 0.32% | 2.7% | 211,080 | 235 | 62 | 98,198 | 167,389 | src/code/fishmod.c |
| `Fishmod_ClaimDisplayLists` | 0.32% | 2.6% | 60,555 | 803 | 116 | 65,257 | 436,191 | src/code/fishmod.c |
| `Fishmod_PointDistSqToSeg2D` | 0.30% | 2.4% | 222,915 | 204 | 49 | 10,660 | 326,940 | fishmod_tri.h |
| `EnFishmod_ParticlesUpdate` | 0.25% | 2.0% | 3,831 | 9,867 | 1,405 | 408,522 | 102,176 | src/overlays/actors/ovl_En_Fishmod/fishmod_particles.c |
| `EnFishmod_SwimTowardImpl` | 0.24% | 2.0% | 36,978 | 1,001 | 170 | 179,952 | 312,618 | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `Particles_Matrix` | 0.21% | 1.7% | 45,140 | 724 | 234 | 209,794 | 35,226 | src/overlays/actors/ovl_En_Fishmod/fishmod_particles.c |
| `EnFishmod_ParticlesDraw` | 0.19% | 1.6% | 3,849 | 7,631 | 903 | 311,560 | 97,698 | src/overlays/actors/ovl_En_Fishmod/fishmod_particles.c |
| `EnFishmod_UpdateSwim` | 0.18% | 1.5% | 86,464 | 326 | 71 | 140,587 | 145,240 | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_FogAmount` | 0.18% | 1.4% | 35,683 | 754 | 98 | 195,684 | 173,965 | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_BodyClearance` | 0.15% | 1.2% | 129,382 | 179 | 26 | 93,705 | 235,540 | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `Fishmod_Relight` | 0.15% | 1.2% | 3,849 | 5,792 | 1,755 | 121,632 | 86,165 | src/code/fishmod.c |
| `Rig_RotateAxis` | 0.12% | 1.0% | 36,456 | 502 | 177 | 6,750 | 41,135 | src/overlays/actors/ovl_En_Fishmod/fishmod_rig.c |
| `EnFishmod_CheckSpookImpl` | 0.11% | 0.9% | 32,264 | 543 | 86 | 150,782 | 57,434 | src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c |
| `EnFishmod_WaterCell` | 0.11% | 0.9% | 36,979 | 467 | 94 | 59,308 | 148,694 | src/overlays/actors/ovl_En_Fishmod/fishmod_water.c |
| `EnFishmod_WaterFloor` | 0.11% | 0.9% | 10,442 | 1,619 | 217 | 54,088 | 196,031 | src/overlays/actors/ovl_En_Fishmod/fishmod_water.c |
| (99 more functions) | 1.71% | 14.1% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `EnFishmod_ScatterDraw` (src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c): 3003: 15%, 408,442 data fills; 3002: 12%, 314,147 data fills; 3091: 10%; 3168: 8%, 96,645 data fills; 3088: 7%, 276,219 data fills
- `Scatter_Build` (src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c): 2780: 10%; 2535: 5%, 162,403 data fills; 2577: 3%; 2592: 2%; 2576: 2%
- `EnFishmod_Update` (src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c): 2177: 83%, 737,691 data fills; 2181: 6%, 57,357 data fills; 2179: 5%, 38,247 data fills; 2174: 4%; 2180: 1%
- `EnFishmod_RigDraw` (src/overlays/actors/ovl_En_Fishmod/fishmod_rig.c): 1287: 27%, 198,193 data fills; 1351: 6%, 64,570 data fills; 1245: 3%, 14,399 data fills; 1221: 3%, 23,219 data fills; 1200: 3%, 28,437 data fills
- `EnFishmod_DrawBillboard` (src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c): 2928: 14%, 130,387 data fills; 2923: 7%, 32,385 data fills; 2927: 6%; 2924: 6%; 2925: 5%
- `EnFishmod_FollowFar.constprop.0` (src/overlays/actors/ovl_En_Fishmod/z_en_fishmod.c): 1238: 28%, 146,897 data fills; 1231: 10%, 122,063 data fills; 1260: 7%, 83,807 data fills; 1265: 7%, 79,839 data fills; 1267: 5%, 56,923 data fills
- `Fishmod_SinCosBinang` (fishmod_trig.h): 51: 28%; 49: 25%; 48: 22%; 50: 10%; 59: 6%
- `EnFishmod_ManagerDraw` (src/overlays/actors/ovl_En_Fishmod/fishmod_manager.c): 863: 45%, 527,725 data fills; 1035: 5%; 1032: 4%, 13,793 data fills; 1042: 3%, 22,746 data fills; 810: 3%, 23,259 data fills

## Collision: 9.34%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| floors, walls, ceilings (z_bgcheck.c) | 6.83% | 73.2% | 6,361,024 | 7,787,614 |
| colliders (z_collision_check.c) | 1.56% | 16.7% | 1,753,921 | 1,079,364 |
| geometry tests (sys_math3d.c) | 0.95% | 10.1% | 259,714 | 907,910 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `BgCheck_RaycastDownImpl` | 2.54% | 27.2% | 56,623 | 6,883 | 1,034 | 2,393,761 | 3,208,774 | src/code/z_bgcheck.c |
| `BgCheck_CheckLineAgainstSSList` | 0.75% | 8.0% | 26,636 | 4,326 | 1,348 | 791,030 | 287,611 | src/code/z_bgcheck.c |
| `BgCheck_SphVsStaticWall` | 0.55% | 5.9% | 21,852 | 3,850 | 666 | 448,954 | 693,571 | src/code/z_bgcheck.c |
| `BgCheck_GetWaterSurface` | 0.34% | 3.6% | 130,028 | 401 | 125 | 205,908 | 329,287 | src/code/z_bgcheck.c |
| `BgCheck_CheckWallImpl` | 0.27% | 2.9% | 22,317 | 1,876 | 167 | 232,637 | 450,078 | src/code/z_bgcheck.c |
| `CollisionPoly_LineVsPoly` | 0.27% | 2.9% | 118,566 | 345 | 80 | 169,256 | 216,336 | src/code/z_bgcheck.c |
| `CollisionCheck_ResetDamage` | 0.26% | 2.7% | 394,273 | 100 | 5 | 550,473 | 44,850 | src/code/z_collision_check.c |
| `DynaPoly_AddBgActorToLookup` | 0.25% | 2.7% | 15,391 | 2,519 | 328 | 531,591 | 97,948 | src/code/z_bgcheck.c |
| `BgCheck_CheckLineImpl` | 0.22% | 2.4% | 11,535 | 2,970 | 263 | 212,576 | 342,293 | src/code/z_bgcheck.c |
| `Math3D_SphVsSphOverlap` | 0.21% | 2.3% | 200,882 | 164 | 48 | 27,686 | 58,077 | src/code/sys_math3d.c |
| `DynaPoly_UpdateContext` | 0.19% | 2.0% | 3,849 | 7,386 | 709 | 327,325 | 92,626 | src/code/z_bgcheck.c |
| `CollisionCheck_OC` | 0.18% | 1.9% | 3,849 | 7,056 | 3,546 | 180,495 | 42,765 | src/code/z_collision_check.c |
| `CollisionCheck_OC_JntSphVsJntSph` | 0.17% | 1.9% | 241,959 | 110 | 66 | 50,961 | 43,316 | src/code/z_collision_check.c |
| `DynaPoly_UnsetAllInteractFlags` | 0.14% | 1.5% | 293,220 | 74 | 21 | 122,326 | 138,093 | src/code/z_bgcheck.c |
| `CollisionCheck_SetOC` | 0.13% | 1.4% | 58,358 | 348 | 43 | 98,807 | 248,780 | src/code/z_collision_check.c |
| `BgCheck_SphVsDynaWall` | 0.13% | 1.4% | 22,319 | 903 | 137 | 158,851 | 145,962 | src/code/z_bgcheck.c |
| `Math3D_SphVsCylOverlapCenterDist` | 0.13% | 1.4% | 140,534 | 140 | 31 | 81,262 | 64,555 | src/code/sys_math3d.c |
| `BgCheck_GetStaticLookupIndicesFromPos` | 0.11% | 1.2% | 59,174 | 283 | 43 | 66,768 | 149,196 | src/code/z_bgcheck.c |
| `CollisionCheck_OC_JntSphVsCyl.part.0` | 0.10% | 1.1% | 152,975 | 102 | 54 | 64,724 | 31,378 | src/code/z_collision_check.c |
| `Math3D_LineVsCube` | 0.10% | 1.1% | 19,885 | 774 | 140 | 41,291 | 171,351 | src/code/sys_math3d.c |
| `CollisionCheck_Incompatible` | 0.10% | 1.0% | 415,257 | 35 | 21 | 53,545 | 15,817 | src/code/z_collision_check.c |
| `CollisionCheck_AC` | 0.09% | 1.0% | 4,518 | 3,152 | 315 | 178,905 | 38,724 | src/code/z_collision_check.c |
| `Collider_ResetJntSphAC` | 0.09% | 1.0% | 44,955 | 314 | 27 | 107,838 | 113,058 | src/code/z_collision_check.c |
| `BgCheck_PosInStaticBoundingBox` | 0.08% | 0.9% | 36,494 | 354 | 44 | 49,165 | 155,516 | src/code/z_bgcheck.c |
| `CollisionCheck_SetAC` | 0.08% | 0.9% | 54,132 | 234 | 43 | 112,204 | 74,770 | src/code/z_collision_check.c |
| (159 more functions) | 1.85% | 19.8% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `BgCheck_RaycastDownImpl` (src/code/z_bgcheck.c): 3199: 93%, 2,168,233 data fills; 3198: 3%, 127,478 data fills; 3220: 2%, 67,741 data fills; 3202: 1%, 25,508 data fills; 1243: 0%
- `BgCheck_CheckLineAgainstSSList` (src/code/z_bgcheck.c): 2060: 33%; 2037: 30%, 522,339 data fills; 2041: 10%, 125,604 data fills; 2053: 5%; 2056: 4%
- `BgCheck_SphVsStaticWall` (src/code/z_bgcheck.c): 1859: 58%, 264,145 data fills; 1846: 8%, 63,867 data fills; 1817: 8%; 1888: 6%, 24,731 data fills; 1838: 3%
- `BgCheck_GetWaterSurface` (src/code/z_bgcheck.c): 5943: 28%; 5932: 13%, 73,909 data fills; 5944: 12%; 5928: 11%, 39,425 data fills; 5934: 10%, 27,076 data fills
- `BgCheck_CheckWallImpl` (src/code/z_bgcheck.c): 3555: 62%, 124,483 data fills; 3550: 14%, 36,713 data fills; 3583: 10%; 3552: 8%, 30,038 data fills; 3553: 3%, 21,127 data fills
- `CollisionPoly_LineVsPoly` (src/code/z_bgcheck.c): 666: 26%; 664: 21%, 118,145 data fills; 657: 15%, 26,705 data fills; 669: 13%; 693: 8%
- `CollisionCheck_ResetDamage` (src/code/z_collision_check.c): 3027: 58%, 354,739 data fills; 3023: 36%, 195,722 data fills; 3028: 6%
- `DynaPoly_AddBgActorToLookup` (src/code/z_bgcheck.c): 4489: 23%, 157,709 data fills; 4484: 21%, 115,401 data fills; 4500: 16%, 104,174 data fills; 4451: 12%, 69,362 data fills; 4449: 8%, 42,420 data fills

## Maths (matrices, vectors, trig, random): 8.61%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| sys_matrix.c | 4.29% | 49.8% | 3,203,018 | 3,701,149 |
| z_lib.c | 2.28% | 26.5% | 1,681,287 | 1,802,103 |
| sys_math_atan.c | 0.51% | 5.9% | 278,448 | 708,743 |
| lookathil.c | 0.26% | 3.0% | 146,033 | 414,025 |
| mtxutil.c | 0.26% | 3.0% | 252,229 | 148,701 |
| sinf.c | 0.21% | 2.4% | 121,453 | 322,792 |
| z_olib.c | 0.20% | 2.4% | 132,023 | 252,512 |
| cosf.c | 0.15% | 1.7% | 83,947 | 210,283 |
| lookat.c | 0.12% | 1.4% | 24,242 | 222,485 |
| perspective.c | 0.12% | 1.4% | 88,924 | 143,083 |
| qrand.c | 0.11% | 1.3% | 22,451 | 74,653 |
| ortho.c | 0.07% | 0.8% | 26,651 | 104,288 |
| sqrtf.c | 0.02% | 0.3% | 0 | 18,511 |
| sys_math.c | 0.00% | 0.0% | 548 | 1,918 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Matrix_MtxFToMtx` | 1.57% | 18.2% | 316,865 | 760 | 151 | 1,468,356 | 1,283,600 | src/code/sys_matrix.c |
| `Matrix_TranslateRotateZYX` | 1.13% | 13.1% | 275,277 | 629 | 181 | 284,736 | 895,475 | src/code/sys_matrix.c |
| `Math_SinS` | 0.70% | 8.2% | 810,249 | 133 | 39 | 294,593 | 576,161 | src/code/z_lib.c |
| `Math_Atan2S` | 0.51% | 5.9% | 327,828 | 239 | 39 | 278,448 | 708,743 | src/code/sys_math_atan.c |
| `Math_CosS` | 0.47% | 5.5% | 541,582 | 133 | 39 | 145,872 | 443,597 | src/code/z_lib.c |
| `Matrix_Push` | 0.40% | 4.6% | 264,992 | 231 | 41 | 599,323 | 97,968 | src/code/sys_matrix.c |
| `Math_Vec3f_Copy` | 0.38% | 4.4% | 441,296 | 132 | 7 | 833,075 | 29,009 | src/code/z_lib.c |
| `Matrix_MultVec3f` | 0.31% | 3.6% | 162,541 | 294 | 54 | 337,349 | 162,433 | src/code/sys_matrix.c |
| `guLookAtHiliteF` | 0.22% | 2.6% | 7,805 | 4,420 | 498 | 98,199 | 375,970 | src/libultra/gu/lookathil.c |
| `sinf` | 0.21% | 2.4% | 62,367 | 517 | 60 | 121,453 | 322,792 | src/libultra/gu/sinf.c |
| `Matrix_Scale` | 0.20% | 2.4% | 107,114 | 291 | 68 | 55,069 | 260,237 | src/code/sys_matrix.c |
| `guMtxF2L` | 0.18% | 2.1% | 38,617 | 712 | 174 | 170,898 | 107,051 | src/libultra/gu/mtxutil.c |
| `Math_Vec3f_DistXZ` | 0.17% | 2.0% | 291,996 | 92 | 13 | 116,697 | 82,006 | src/code/z_lib.c |
| `Math_Vec3f_DistXYZ` | 0.15% | 1.8% | 91,939 | 256 | 19 | 96,848 | 222,587 | src/code/z_lib.c |
| `cosf` | 0.15% | 1.7% | 41,764 | 536 | 80 | 83,947 | 210,283 | src/libultra/gu/cosf.c |
| `Math_SmoothStepToS` | 0.13% | 1.5% | 220,530 | 91 | 23 | 77,852 | 101,117 | src/code/z_lib.c |
| `Matrix_SetTranslateRotateYXZ` | 0.11% | 1.3% | 45,353 | 380 | 42 | 116,521 | 130,325 | src/code/sys_matrix.c |
| `guPerspectiveF` | 0.10% | 1.2% | 11,547 | 1,393 | 156 | 70,336 | 120,028 | src/libultra/gu/perspective.c |
| `guLookAtF` | 0.10% | 1.2% | 11,547 | 1,377 | 181 | 14,731 | 183,596 | src/libultra/gu/lookat.c |
| `Matrix_Translate` | 0.09% | 1.1% | 75,015 | 189 | 38 | 52,820 | 106,260 | src/code/sys_matrix.c |
| `OLib_Vec3fToVecSph` | 0.08% | 1.0% | 29,582 | 440 | 80 | 55,449 | 84,423 | src/code/z_olib.c |
| `guMtxIdentF` | 0.08% | 0.9% | 38,662 | 302 | 106 | 81,331 | 41,650 | src/libultra/gu/mtxutil.c |
| `Math_SmoothStepToF` | 0.07% | 0.9% | 218,277 | 52 | 24 | 17,646 | 61,988 | src/code/z_lib.c |
| `Matrix_RotateY` | 0.07% | 0.9% | 19,981 | 564 | 88 | 8,804 | 144,999 | src/code/sys_matrix.c |
| `Rand_ZeroOne` | 0.07% | 0.8% | 305,047 | 33 | 17 | 8,524 | 54,262 | src/libc64/qrand.c |
| (54 more functions) | 0.93% | 10.8% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Matrix_MtxFToMtx` (src/code/sys_matrix.c): 602: 23%, 652,569 data fills; 600: 21%, 576,252 data fills; 606: 13%, 217,777 data fills; 603: 9%; 605: 8%
- `Matrix_TranslateRotateZYX` (src/code/sys_matrix.c): 410: 8%, 91,119 data fills; 416: 8%; 417: 7%; 418: 7%; 400: 7%, 132,356 data fills
- `Math_SinS` (src/code/z_lib.c): 47: 88%; 106: 6%, 69,400 data fills; 48: 5%; 109: 1%, 10,086 data fills
- `Math_Atan2S` (src/code/sys_math_atan.c): 138: 16%, 57,139 data fills; 136: 11%, 40,689 data fills; 132: 8%, 36,548 data fills; 116: 7%; 124: 7%, 27,618 data fills
- `Math_CosS` (src/code/z_lib.c): 39: 89%; 106: 9%, 72,788 data fills; 40: 2%; 109: 1%, 4,671 data fills
- `Matrix_Push` (src/code/sys_matrix.c): 40: 77%, 427,477 data fills; 41: 11%, 97,756 data fills; 39: 10%, 73,232 data fills; 42: 2%
- `Math_Vec3f_Copy` (src/code/z_lib.c): 249: 51%, 424,992 data fills; 252: 44%, 397,008 data fills; 250: 4%; 251: 2%
- `Matrix_MultVec3f` (src/code/sys_matrix.c): 641: 38%, 186,477 data fills; 642: 25%, 44,976 data fills; 643: 21%; 644: 9%, 44,642 data fills; 639: 8%, 48,788 data fills

## Actor framework (z_actor.c): 5.20%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_actor.c | 5.19% | 99.9% | 6,405,072 | 4,952,891 |
| z_cheap_proc.inc.c | 0.01% | 0.1% | 3,338 | 8,746 |
| z_actor_dlftbls.c | 0.00% | 0.0% | 1 | 3 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Actor_UpdateAll` | 1.97% | 37.9% | 3,849 | 78,573 | 12,739 | 2,443,768 | 1,516,664 | src/code/z_actor.c |
| `Actor_DrawAll` | 0.81% | 15.6% | 3,849 | 32,278 | 3,144 | 1,367,928 | 515,708 | src/code/z_actor.c |
| `Actor_Draw.part.0` | 0.44% | 8.4% | 45,350 | 1,476 | 115 | 395,198 | 700,518 | src/code/z_actor.c |
| `Actor_CullingVolumeTest` | 0.33% | 6.4% | 394,758 | 130 | 23 | 477,986 | 80,971 | src/code/z_actor.c |
| `Attention_FindActorInCategory` | 0.22% | 4.2% | 43,109 | 778 | 105 | 404,906 | 50,546 | src/code/z_actor.c |
| `Actor_UpdateBgCheckInfo` | 0.20% | 3.8% | 22,652 | 1,347 | 134 | 196,582 | 321,518 | src/code/z_actor.c |
| `func_8002E830` | 0.16% | 3.1% | 15,511 | 1,582 | 190 | 168,009 | 200,622 | src/code/z_actor.c |
| `func_8002E2AC` | 0.12% | 2.4% | 22,700 | 841 | 62 | 73,661 | 246,450 | src/code/z_actor.c |
| `ActorShadow_DrawFeet` | 0.11% | 2.2% | 3,859 | 4,521 | 297 | 140,475 | 162,035 | src/code/z_actor.c |
| `ActorShadow_Draw` | 0.09% | 1.7% | 14,681 | 942 | 80 | 104,307 | 119,252 | src/code/z_actor.c |
| `func_800359B8` | 0.07% | 1.4% | 9,039 | 1,225 | 92 | 43,979 | 134,406 | src/code/z_actor.c |
| `ActorShadow_DrawFoot` | 0.06% | 1.2% | 7,026 | 1,385 | 106 | 53,411 | 105,226 | src/code/z_actor.c |
| `Actor_MoveXZGravity` | 0.05% | 1.0% | 30,876 | 270 | 42 | 60,356 | 28,158 | src/code/z_actor.c |
| `Actor_UpdateVelocityXZGravity` | 0.04% | 0.9% | 34,727 | 198 | 30 | 47,465 | 34,721 | src/code/z_actor.c |
| `Attention_WeightedDistToPlayerSq` | 0.04% | 0.7% | 88,131 | 62 | 17 | 43,588 | 23,217 | src/code/z_actor.c |
| `Actor_SetFocus` | 0.03% | 0.7% | 23,592 | 226 | 13 | 39,471 | 44,255 | src/code/z_actor.c |
| `BodyBreak_SetInfo` | 0.03% | 0.6% | 55,878 | 87 | 24 | 32,624 | 18,068 | src/code/z_actor.c |
| `Attention_FindActor` | 0.03% | 0.6% | 3,865 | 1,196 | 136 | 33,499 | 41,470 | src/code/z_actor.c |
| `Attention_Update` | 0.03% | 0.6% | 3,849 | 1,171 | 70 | 26,034 | 54,261 | src/code/z_actor.c |
| `Actor_Draw` | 0.03% | 0.5% | 45,349 | 94 | 21 | 20,445 | 37,557 | src/code/z_actor.c |
| `Actor_MoveXYZ` | 0.03% | 0.5% | 10,254 | 385 | 42 | 28,678 | 26,375 | src/code/z_actor.c |
| `Actor_GetWorldPosShapeRot` | 0.02% | 0.4% | 13,406 | 237 | 27 | 9,779 | 40,630 | src/code/z_actor.c |
| `Attention_SetNaviState` | 0.02% | 0.4% | 3,755 | 843 | 48 | 22,287 | 26,282 | src/code/z_actor.c |
| `Actor_UpdatePos` | 0.02% | 0.4% | 7,699 | 410 | 34 | 16,448 | 29,793 | src/code/z_actor.c |
| `Attention_Draw` | 0.02% | 0.4% | 3,849 | 738 | 47 | 14,121 | 35,041 | src/code/z_actor.c |
| (55 more functions) | 0.21% | 4.0% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Actor_UpdateAll` (src/code/z_actor.c): 2579: 11%, 394,588 data fills; 2553: 10%, 376,530 data fills; 2545: 9%, 351,129 data fills; 2552: 9%, 388,383 data fills; 2585: 8%, 209,666 data fills
- `Actor_DrawAll` (src/code/z_actor.c): 3087: 46%, 789,885 data fills; 3077: 23%, 395,221 data fills; 3066: 7%; 3090: 4%; 3106: 3%, 24,598 data fills
- `Actor_Draw.part.0` (src/code/z_actor.c): 2700: 10%; 2742: 9%, 48,621 data fills; 2741: 8%, 46,066 data fills; 2745: 8%; 2789: 7%, 44,888 data fills
- `Actor_CullingVolumeTest` (src/code/z_actor.c): 3003: 58%, 395,075 data fills; 3007: 17%, 37,609 data fills; 3009: 13%, 45,134 data fills; 3011: 5%; 3010: 2%
- `Attention_FindActorInCategory` (src/code/z_actor.c): 3652: 31%, 161,262 data fills; 3655: 25%, 111,059 data fills; 3667: 11%, 42,593 data fills; 3659: 10%, 45,768 data fills; 3688: 6%, 11,378 data fills

## Animation: 3.95%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_skelanime.c | 2.37% | 59.8% | 2,847,871 | 1,491,727 |
| z_skin_matrix.c | 1.59% | 40.2% | 1,856,836 | 589,981 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `SkinMatrix_Vec3fMtxFMultXYZW` | 1.26% | 31.9% | 465,875 | 415 | 69 | 1,689,531 | 235,986 | src/code/z_skin_matrix.c |
| `SkelAnime_GetFrameData` | 0.87% | 22.1% | 53,552 | 2,505 | 673 | 1,142,515 | 371,176 | src/code/z_skelanime.c |
| `SkelAnime_DrawFlexLimbLod` | 0.32% | 8.1% | 76,984 | 638 | 142 | 385,283 | 210,682 | src/code/z_skelanime.c |
| `SkelAnime_DrawFlexLimbOpa` | 0.30% | 7.5% | 80,038 | 566 | 139 | 388,275 | 87,861 | src/code/z_skelanime.c |
| `SkinMatrix_MtxFMtxFMult` | 0.21% | 5.4% | 25,807 | 1,270 | 273 | 70,262 | 217,796 | src/code/z_skin_matrix.c |
| `SkelAnime_DrawLimbOpa` | 0.17% | 4.3% | 62,019 | 422 | 113 | 177,838 | 81,019 | src/code/z_skelanime.c |
| `SkelAnime_InterpFrameTable` | 0.11% | 2.8% | 4,877 | 3,435 | 1,368 | 29,139 | 41,977 | src/code/z_skelanime.c |
| `SkelAnime_LoopFull` | 0.10% | 2.5% | 48,420 | 308 | 33 | 142,865 | 59,769 | src/code/z_skelanime.c |
| `SkelAnime_AnimateFrame` | 0.09% | 2.2% | 48,845 | 277 | 24 | 117,828 | 95,496 | src/code/z_skelanime.c |
| `SkelAnime_DrawFlexOpa` | 0.08% | 2.1% | 10,586 | 1,186 | 122 | 116,712 | 75,557 | src/code/z_skelanime.c |
| `SkelAnime_DrawLimb` | 0.08% | 2.0% | 16,928 | 731 | 120 | 98,356 | 61,995 | src/code/z_skelanime.c |
| `SkelAnime_DrawFlexLod` | 0.05% | 1.1% | 3,857 | 1,805 | 129 | 43,002 | 85,653 | src/code/z_skelanime.c |
| `SkinMatrix_MtxFToNewMtx` | 0.04% | 1.1% | 65,894 | 101 | 15 | 18,285 | 82,358 | src/code/z_skin_matrix.c |
| `SkinMatrix_SetTranslate` | 0.04% | 1.1% | 54,872 | 120 | 19 | 43,021 | 37,418 | src/code/z_skin_matrix.c |
| `SkelAnime_DrawOpa` | 0.03% | 0.9% | 4,335 | 1,234 | 97 | 41,012 | 45,360 | src/code/z_skelanime.c |
| `Animation_OnFrame` | 0.03% | 0.8% | 22,084 | 215 | 33 | 2,116 | 73,334 | src/code/z_skelanime.c |
| `SkelAnime_Update` | 0.03% | 0.8% | 48,902 | 94 | 3 | 48,892 | 35,544 | src/code/z_skelanime.c |
| `AnimTaskQueue_AddLoadPlayerFrame` | 0.03% | 0.7% | 4,802 | 867 | 71 | 31,173 | 43,231 | src/code/z_skelanime.c |
| `SkinMatrix_SetRotateZYX` | 0.02% | 0.5% | 6,008 | 464 | 50 | 21,762 | 13,615 | src/code/z_skin_matrix.c |
| `SkelAnime_Draw` | 0.02% | 0.4% | 1,302 | 1,777 | 101 | 19,598 | 19,780 | src/code/z_skelanime.c |
| `AnimTaskQueue_Update` | 0.01% | 0.4% | 3,850 | 592 | 47 | 12,674 | 23,083 | src/code/z_skelanime.c |
| `LinkAnimation_AnimateFrame` | 0.01% | 0.3% | 2,969 | 554 | 33 | 13,719 | 14,242 | src/code/z_skelanime.c |
| `SkinMatrix_SetScale` | 0.01% | 0.2% | 7,959 | 185 | 19 | 13,940 | 2,567 | src/code/z_skin_matrix.c |
| `LinkAnimation_Once` | 0.01% | 0.1% | 1,606 | 564 | 42 | 4,875 | 9,453 | src/code/z_skelanime.c |
| `LinkAnimation_OnFrame` | 0.00% | 0.1% | 1,488 | 483 | 43 | 3,178 | 8,243 | src/code/z_skelanime.c |
| (30 more functions) | 0.03% | 0.8% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `SkinMatrix_Vec3fMtxFMultXYZW` (src/code/z_skin_matrix.c): 35: 51%, 1,262,944 data fills; 36: 24%, 408,006 data fills; 38: 12%; 37: 12%; 39: 1%
- `SkelAnime_GetFrameData` (src/code/z_skelanime.c): 499: 22%, 314,751 data fills; 501: 21%, 231,923 data fills; 503: 16%, 170,748 data fills; 491: 13%, 138,626 data fills; 500: 7%, 83,500 data fills
- `SkelAnime_DrawFlexLimbLod` (src/code/z_skelanime.c): 158: 12%, 85,621 data fills; 170: 11%, 51,256 data fills; 195: 8%, 24,862 data fills; 156: 8%, 51,429 data fills; 171: 8%, 33,374 data fills
- `SkelAnime_DrawFlexLimbOpa` (src/code/z_skelanime.c): 373: 17%, 107,240 data fills; 411: 11%, 45,214 data fills; 381: 9%, 39,504 data fills; 385: 7%, 30,565 data fills; 360: 7%
- `SkinMatrix_MtxFMtxFMult` (src/code/z_skin_matrix.c): 88: 8%, 16,080 data fills; 94: 7%, 16,791 data fills; 106: 7%, 13,365 data fills; 100: 7%, 11,868 data fills; 148: 6%

## Effects: 2.20%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_effect_soft_sprite.c | 0.85% | 38.7% | 1,467,004 | 258,678 |
| ovl_Effect_Ss_Bubble | 0.66% | 29.8% | 939,372 | 288,767 |
| ovl_Effect_Ss_G_Ripple | 0.44% | 20.1% | 511,244 | 285,871 |
| z_effect.c | 0.15% | 6.8% | 248,712 | 129,118 |
| ovl_Effect_Ss_KiraKira | 0.06% | 2.7% | 51,433 | 85,918 |
| z_eff_blure.c | 0.02% | 0.7% | 17,421 | 26,956 |
| z_effect_soft_sprite_old_init.c | 0.01% | 0.7% | 7,274 | 28,311 |
| ovl_Effect_Ss_Dust | 0.01% | 0.4% | 12,373 | 4,385 |
| ovl_Effect_Ss_G_Splash | 0.00% | 0.1% | 1,720 | 4,793 |
| ovl_Effect_Ss_HitMark | 0.00% | 0.0% | 795 | 1,920 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `EffectSsBubble_Draw` | 0.53% | 23.9% | 84,214 | 958 | 133 | 801,694 | 246,021 | src/overlays/effects/ovl_Effect_Ss_Bubble/z_eff_ss_bubble.c |
| `EffectSs_DrawAll` | 0.34% | 15.4% | 3,849 | 13,531 | 2,046 | 608,021 | 59,631 | src/code/z_effect_soft_sprite.c |
| `EffectSsGRipple_DrawRipple` | 0.33% | 14.9% | 57,928 | 871 | 177 | 431,885 | 190,093 | src/overlays/effects/ovl_Effect_Ss_G_Ripple/z_eff_ss_g_ripple.c |
| `EffectSs_Update` | 0.24% | 11.0% | 150,884 | 247 | 31 | 452,088 | 42,111 | src/code/z_effect_soft_sprite.c |
| `EffectSs_UpdateAll` | 0.18% | 8.0% | 3,849 | 7,057 | 1,373 | 286,016 | 73,568 | src/code/z_effect_soft_sprite.c |
| `EffectSsBubble_Update` | 0.12% | 5.6% | 85,394 | 221 | 60 | 132,748 | 31,017 | src/overlays/effects/ovl_Effect_Ss_Bubble/z_eff_ss_bubble.c |
| `EffectSsGRipple_Update` | 0.09% | 4.2% | 57,538 | 245 | 70 | 73,690 | 51,356 | src/overlays/effects/ovl_Effect_Ss_G_Ripple/z_eff_ss_g_ripple.c |
| `Effect_UpdateAll` | 0.07% | 3.0% | 3,849 | 2,634 | 230 | 123,222 | 42,415 | src/code/z_effect.c |
| `Effect_DrawAll` | 0.06% | 2.9% | 3,849 | 2,546 | 218 | 123,857 | 30,815 | src/code/z_effect.c |
| `EffectSsKiraKira_Draw` | 0.05% | 2.2% | 6,010 | 1,257 | 131 | 42,276 | 75,906 | src/overlays/effects/ovl_Effect_Ss_KiraKira/z_eff_ss_kirakira.c |
| `EffectSs_FindSlot` | 0.04% | 1.9% | 6,359 | 1,013 | 308 | 59,992 | 14,152 | src/code/z_effect_soft_sprite.c |
| `EffectSs_Delete` | 0.03% | 1.3% | 9,691 | 456 | 36 | 41,672 | 29,231 | src/code/z_effect_soft_sprite.c |
| `EffectSs_Spawn` | 0.02% | 1.1% | 6,368 | 577 | 81 | 18,748 | 39,806 | src/code/z_effect_soft_sprite.c |
| `Effect_GetPlayState` | 0.02% | 0.9% | 55,875 | 54 | 3 | 1,602 | 55,875 | src/code/z_effect.c |
| `EffectSsGRipple_Init` | 0.01% | 0.5% | 0 | 0 | 0 | 5,525 | 22,399 | src/overlays/effects/ovl_Effect_Ss_G_Ripple/z_eff_ss_g_ripple.c |
| `EffectSsGRipple_Draw` | 0.01% | 0.5% | 58,150 | 26 | 7 | 144 | 22,023 | src/overlays/effects/ovl_Effect_Ss_G_Ripple/z_eff_ss_g_ripple.c |
| `EffectSsBubble_Init` | 0.01% | 0.4% | 0 | 0 | 0 | 4,930 | 11,729 | src/overlays/effects/ovl_Effect_Ss_Bubble/z_eff_ss_bubble.c |
| `EffectBlure_Update` | 0.01% | 0.4% | 3,858 | 334 | 17 | 7,681 | 15,397 | src/code/z_eff_blure.c |
| `EffectSsDust_Draw` | 0.01% | 0.4% | 1,300 | 940 | 182 | 12,055 | 3,611 | src/overlays/effects/ovl_Effect_Ss_Dust/z_eff_ss_dust.c |
| `func_809AABF0` | 0.01% | 0.4% | 6,011 | 198 | 40 | 8,679 | 6,254 | src/overlays/effects/ovl_Effect_Ss_KiraKira/z_eff_ss_kirakira.c |
| `EffectBlure_Draw` | 0.01% | 0.3% | 3,849 | 295 | 14 | 9,710 | 11,548 | src/code/z_eff_blure.c |
| `EffectSs_DrawGEffect` | 0.01% | 0.3% | 589 | 1,461 | 154 | 5,462 | 8,441 | src/code/z_effect_soft_sprite_old_init.c |
| `EffectSsGRipple_Spawn` | 0.00% | 0.2% | 2,934 | 211 | 25 | 183 | 10,411 | src/code/z_effect_soft_sprite_old_init.c |
| `EffectSsBubble_Spawn` | 0.00% | 0.1% | 2,823 | 127 | 27 | 631 | 4,467 | src/code/z_effect_soft_sprite_old_init.c |
| `EffectSsKiraKira_SpawnDispersed` | 0.00% | 0.1% | 390 | 852 | 86 | 912 | 4,503 | src/code/z_effect_soft_sprite_old_init.c |
| (19 more functions) | 0.01% | 0.3% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `EffectSsBubble_Draw` (src/overlays/effects/ovl_Effect_Ss_Bubble/z_eff_ss_bubble.c): 53: 14%, 100,844 data fills; 90: 12%, 127,101 data fills; 72: 7%, 84,212 data fills; 92: 7%, 49,488 data fills; 91: 6%, 39,974 data fills
- `EffectSs_DrawAll` (src/code/z_effect_soft_sprite.c): 338: 37%, 278,250 data fills; 339: 23%, 154,161 data fills; 358: 22%, 150,315 data fills; 337: 8%; 340: 3%
- `EffectSsGRipple_DrawRipple` (src/overlays/effects/ovl_Effect_Ss_G_Ripple/z_eff_ss_g_ripple.c): 98: 11%, 59,623 data fills; 101: 10%, 64,822 data fills; 136: 9%, 39,781 data fills; 134: 8%, 26,470 data fills; 138: 7%, 35,217 data fills
- `EffectSs_Update` (src/code/z_effect_soft_sprite.c): 287: 29%, 150,420 data fills; 286: 26%, 150,378 data fills; 285: 26%, 150,345 data fills; 283: 5%; 288: 4%

## Scheduler, threads and system: 2.13%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| sched.c | 0.67% | 31.3% | 543,573 | 1,262,030 |
| padmgr.c | 0.35% | 16.4% | 275,831 | 579,337 |
| math64.c | 0.25% | 11.5% | 130,221 | 393,962 |
| padsetup.c | 0.17% | 8.0% | 114,657 | 178,661 |
| fault_n64.c | 0.17% | 8.0% | 154,395 | 260,754 |
| graph.c | 0.17% | 7.8% | 182,878 | 267,942 |
| sys_rumble.c | 0.12% | 5.4% | 85,870 | 153,149 |
| irqmgr.c | 0.10% | 4.8% | 153,754 | 117,803 |
| pad.c | 0.07% | 3.2% | 12,961 | 155,187 |
| viconfig.c | 0.02% | 0.9% | 23,750 | 35,412 |
| z_rumble.c | 0.02% | 0.8% | 6,554 | 40,361 |
| z_debug.c | 0.01% | 0.6% | 9,271 | 23,120 |
| main.c | 0.01% | 0.6% | 11,301 | 23,684 |
| z_debug_display.c | 0.01% | 0.4% | 8,194 | 19,249 |
| fp.c | 0.00% | 0.1% | 0 | 3,849 |
| stackcheck.c | 0.00% | 0.0% | 1,170 | 36 |
| sprintf.c | 0.00% | 0.0% | 653 | 861 |
| cic6105.c | 0.00% | 0.0% | 9 | 15 |
| driverominit.c | 0.00% | 0.0% | 4 | 15 |
| boot_main.c | 0.00% | 0.0% | 6 | 10 |
| code_n64dd_800AD410.c | 0.00% | 0.0% | 3 | 6 |
| idle.c | 0.00% | 0.0% | 3 | 6 |
| z_prenmi_buff.c | 0.00% | 0.0% | 4 | 2 |
| runtime.c | 0.00% | 0.0% | 1 | 6 |
| z_n64dd.c | 0.00% | 0.0% | 1 | 1 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `PadSetup_Init` | 0.17% | 8.0% | 1 | 26,176,588 | 3,657,132 | 114,657 | 178,661 | src/libu64/padsetup.c |
| `Sched_HandleRSPDone` | 0.15% | 7.2% | 22,681 | 1,037 | 67 | 132,304 | 279,402 | src/code/sched.c |
| `Math_FAtanContFracF` | 0.13% | 6.0% | 107,634 | 182 | 30 | 54,754 | 227,368 | src/libc64/math64.c |
| `Math_FAtan2F` | 0.12% | 5.5% | 108,480 | 166 | 24 | 75,466 | 166,591 | src/libc64/math64.c |
| `PadMgr_UpdateInputs` | 0.12% | 5.5% | 11,785 | 1,514 | 259 | 96,467 | 174,141 | src/code/padmgr.c |
| `RumbleMgr_Update` | 0.12% | 5.4% | 11,706 | 1,507 | 427 | 85,870 | 153,144 | src/code/sys_rumble.c |
| `Sched_Schedule` | 0.11% | 5.1% | 46,896 | 353 | 26 | 81,785 | 215,403 | src/code/sched.c |
| `Sched_RunTask` | 0.11% | 5.0% | 22,757 | 717 | 47 | 108,857 | 196,458 | src/code/sched.c |
| `PadMgr_HandleRetrace` | 0.11% | 5.0% | 11,785 | 1,373 | 133 | 93,695 | 167,937 | src/code/padmgr.c |
| `Fault_RemoveClient` | 0.10% | 4.6% | 45,351 | 330 | 24 | 73,564 | 167,825 | src/code/fault_n64.c |
| `Sched_HandleNotification` | 0.08% | 3.9% | 27,423 | 469 | 48 | 67,317 | 153,323 | src/code/sched.c |
| `IrqMgr_HandleRetrace` | 0.08% | 3.8% | 11,788 | 1,056 | 61 | 141,581 | 70,631 | src/code/irqmgr.c |
| `Graph_Update` | 0.07% | 3.3% | 3,850 | 2,839 | 162 | 83,392 | 102,601 | src/code/graph.c |
| `Fault_AddClient` | 0.07% | 3.3% | 45,352 | 234 | 28 | 76,971 | 89,064 | src/code/fault_n64.c |
| `Sched_HandleRetrace` | 0.06% | 3.0% | 11,787 | 827 | 54 | 62,696 | 100,912 | src/code/sched.c |
| `Graph_TaskSet00` | 0.06% | 2.9% | 3,850 | 2,480 | 182 | 71,407 | 103,614 | src/code/graph.c |
| `PadMgr_UpdateRumble` | 0.06% | 2.7% | 11,705 | 746 | 113 | 14,698 | 131,755 | src/code/padmgr.c |
| `PadUtils_UpdateRelXY` | 0.05% | 2.5% | 50,857 | 162 | 26 | 12,961 | 123,339 | src/libu64/pad.c |
| `Sched_TaskComplete.part.0` | 0.04% | 2.1% | 15,630 | 438 | 32 | 50,472 | 64,393 | src/code/sched.c |
| `Sched_ThreadEntry` | 0.04% | 2.0% | 1 | 6,676,281 | 744,973 | 876 | 121,156 | src/code/sched.c |
| `PadMgr_RequestPadData` | 0.04% | 1.9% | 3,862 | 1,566 | 156 | 53,467 | 34,650 | src/code/padmgr.c |
| `Sched_QueueTask` | 0.04% | 1.7% | 15,631 | 354 | 23 | 15,835 | 80,994 | src/code/sched.c |
| `Graph_InitTHGA` | 0.03% | 1.3% | 3,850 | 1,096 | 87 | 24,226 | 50,060 | src/code/graph.c |
| `IrqMgr_ThreadEntry` | 0.02% | 1.0% | 1 | 3,160,377 | 188,759 | 12,159 | 47,149 | src/code/irqmgr.c |
| `ViConfig_UpdateBlack` | 0.02% | 0.9% | 11,792 | 245 | 13 | 23,747 | 35,401 | src/boot/viconfig.c |
| (53 more functions) | 0.14% | 6.4% | | | | | | |

## Sky, weather and lights: 1.70%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_lights.c | 0.91% | 53.6% | 751,627 | 1,333,267 |
| z_kankyo.c | 0.72% | 42.6% | 511,535 | 1,105,446 |
| z_vr_box_draw.c | 0.06% | 3.7% | 57,430 | 111,642 |
| z_vr_box.c | 0.00% | 0.1% | 2,035 | 165 |
| z_env_flags.c | 0.00% | 0.0% | 0 | 1 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Environment_DrawLensFlare` | 0.27% | 16.0% | 3,850 | 10,874 | 1,318 | 295,431 | 217,197 | src/code/z_kankyo.c |
| `Environment_Update` | 0.25% | 14.6% | 3,850 | 9,877 | 1,598 | 96,809 | 471,979 | src/code/z_kankyo.c |
| `Lights_BindAll` | 0.21% | 12.5% | 56,984 | 571 | 82 | 191,988 | 307,010 | src/code/z_lights.c |
| `Lights_Draw` | 0.20% | 12.0% | 53,062 | 590 | 50 | 159,418 | 365,988 | src/code/z_lights.c |
| `Lights_BindPoint` | 0.19% | 11.2% | 170,861 | 171 | 34 | 59,608 | 266,860 | src/code/z_lights.c |
| `Lights_BindDirectional` | 0.13% | 7.9% | 113,791 | 181 | 27 | 134,695 | 165,471 | src/code/z_lights.c |
| `LightContext_NewLights` | 0.08% | 4.8% | 56,912 | 221 | 14 | 99,789 | 110,006 | src/code/z_lights.c |
| `Environment_DrawSunAndMoon` | 0.08% | 4.5% | 3,849 | 3,083 | 268 | 24,864 | 169,672 | src/code/z_kankyo.c |
| `Skybox_Draw` | 0.06% | 3.7% | 3,170 | 3,011 | 219 | 57,430 | 107,788 | src/code/z_vr_box_draw.c |
| `Lights_GlowCheck` | 0.05% | 3.0% | 3,860 | 1,996 | 142 | 60,930 | 63,849 | src/code/z_lights.c |
| `Lights_DrawGlow` | 0.03% | 1.8% | 3,850 | 1,214 | 87 | 37,556 | 38,659 | src/code/z_lights.c |
| `Environment_UpdateSkybox` | 0.03% | 1.8% | 3,170 | 1,450 | 149 | 14,620 | 63,515 | src/code/z_kankyo.c |
| `Environment_DrawLightning` | 0.02% | 1.2% | 3,850 | 784 | 77 | 19,298 | 30,792 | src/code/z_kankyo.c |
| `Environment_DrawSunLensFlare` | 0.02% | 0.9% | 3,849 | 637 | 50 | 12,373 | 29,250 | src/code/z_kankyo.c |
| `Environment_PlayTimeBasedSequence` | 0.01% | 0.7% | 3,849 | 488 | 27 | 11,555 | 19,245 | src/code/z_kankyo.c |
| `Environment_DrawSkyboxFilters` | 0.01% | 0.6% | 3,866 | 394 | 32 | 2,466 | 25,537 | src/code/z_kankyo.c |
| `Environment_GraphCallback` | 0.01% | 0.5% | 3,854 | 329 | 17 | 11,467 | 11,549 | src/code/z_kankyo.c |
| `Environment_FillScreen` | 0.01% | 0.4% | 3,850 | 276 | 20 | 7 | 19,245 | src/code/z_kankyo.c |
| `Environment_ChangeLightSetting` | 0.01% | 0.4% | 3,689 | 280 | 13 | 11,067 | 11,068 | src/code/z_kankyo.c |
| `Environment_UpdateLightningStrike` | 0.01% | 0.4% | 3,849 | 265 | 22 | 3,854 | 15,396 | src/code/z_kankyo.c |
| `Lights_PointNoGlowSetInfo` | 0.01% | 0.4% | 3,851 | 244 | 13 | 3,885 | 11,551 | src/code/z_lights.c |
| `Environment_DrawCustomLensFlare` | 0.00% | 0.3% | 3,849 | 170 | 6 | 3,839 | 7,698 | src/code/z_kankyo.c |
| `Environment_UpdateStorm` | 0.00% | 0.2% | 3,170 | 152 | 5 | 96 | 9,510 | src/code/z_kankyo.c |
| `Environment_ZBufValToFixedPoint` | 0.00% | 0.2% | 3,721 | 121 | 12 | 3,734 | 3,734 | src/code/z_kankyo.c |
| `Lights_PointGlowSetInfo` | 0.00% | 0.2% | 3,850 | 116 | 14 | 3,744 | 3,854 | src/code/z_lights.c |
| (13 more functions) | 0.00% | 0.1% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Environment_DrawLensFlare` (src/code/z_kankyo.c): 1650: 9%, 26,093 data fills; 1669: 6%, 22,355 data fills; 1540: 6%, 24,201 data fills; 1657: 6%, 21,872 data fills; 1686: 5%, 18,262 data fills
- `Environment_Update` (src/code/z_kankyo.c): 1134: 6%; 1108: 5%; 1073: 5%, 7,701 data fills; 1078: 4%; 1156: 4%
- `Lights_BindAll` (src/code/z_lights.c): 192: 41%, 92,149 data fills; 187: 26%; 190: 12%, 48,558 data fills; 195: 10%; 193: 6%, 19,671 data fills
- `Lights_Draw` (src/code/z_lights.c): 79: 46%; 80: 31%, 56,958 data fills; 75: 20%, 58,941 data fills; 76: 1%; 77: 1%

## Link: 1.54%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| ovl_player_actor | 0.88% | 57.5% | 610,397 | 1,760,969 |
| z_player_lib.c | 0.64% | 41.9% | 400,294 | 1,127,355 |
| z_player_call.c | 0.01% | 0.6% | 11,632 | 15,419 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Player_OverrideLimbDrawGameplayDefault` | 0.15% | 9.8% | 80,848 | 285 | 35 | 61,181 | 329,554 | src/code/z_player_lib.c |
| `Player_UpdateCommon` | 0.15% | 9.7% | 3,849 | 5,934 | 397 | 112,835 | 294,609 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_PostLimbDrawGameplay` | 0.14% | 9.3% | 80,832 | 271 | 58 | 50,466 | 253,935 | src/code/z_player_lib.c |
| `Player_OverrideLimbDrawGameplayCommon` | 0.11% | 6.9% | 80,832 | 202 | 34 | 49,651 | 190,354 | src/code/z_player_lib.c |
| `Player_ProcessSceneCollision` | 0.07% | 4.8% | 3,849 | 2,935 | 215 | 59,149 | 143,623 | src/overlays/actors/ovl_player_actor/z_player.c |
| `func_80847298` | 0.07% | 4.5% | 3,849 | 2,772 | 235 | 13,655 | 160,065 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_DrawImpl` | 0.06% | 4.1% | 3,849 | 2,485 | 152 | 69,432 | 90,416 | src/code/z_player_lib.c |
| `Player_InCsMode` | 0.06% | 3.9% | 17,907 | 519 | 26 | 83,766 | 72,189 | src/code/z_player_lib.c |
| `func_8008F87C.part.0` | 0.06% | 3.6% | 7,698 | 1,105 | 132 | 44,530 | 78,234 | src/code/z_player_lib.c |
| `Player_UpdateInterface.part.0` | 0.05% | 3.2% | 3,849 | 1,975 | 128 | 27,231 | 108,079 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_Draw` | 0.04% | 2.4% | 3,850 | 1,455 | 92 | 48,572 | 55,142 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_GetEnvironmentalHazard` | 0.03% | 2.2% | 25,587 | 199 | 52 | 25,079 | 39,539 | src/code/z_player_lib.c |
| `Player_Update` | 0.03% | 2.0% | 3,849 | 1,236 | 63 | 36,812 | 42,377 | src/overlays/actors/ovl_player_actor/z_player.c |
| `func_8083D6EC` | 0.03% | 2.0% | 3,850 | 1,225 | 91 | 32,702 | 49,309 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_DrawGameplay` | 0.03% | 1.8% | 3,849 | 1,131 | 60 | 30,994 | 38,492 | src/overlays/actors/ovl_player_actor/z_player.c |
| `func_808382DC.part.0` | 0.03% | 1.7% | 3,829 | 1,067 | 70 | 22,195 | 49,468 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_UpdateCamAndSeqModes.part.0` | 0.03% | 1.6% | 3,855 | 996 | 77 | 9,508 | 59,415 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_ProcessItemButtons` | 0.02% | 1.5% | 3,402 | 1,057 | 161 | 15,308 | 44,409 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_HandleExitsAndVoids.part.0` | 0.02% | 1.5% | 3,849 | 925 | 71 | 15,227 | 46,056 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_UpdateItems` | 0.02% | 1.1% | 3,405 | 767 | 52 | 15,348 | 27,406 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_TryActionHandlerList.part.0` | 0.02% | 1.1% | 3,010 | 852 | 108 | 14,417 | 24,929 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_ProcessControlStick` | 0.02% | 1.1% | 3,849 | 656 | 50 | 15,798 | 30,793 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_DetectRumbleSecrets` | 0.02% | 1.1% | 3,829 | 659 | 29 | 19,223 | 26,905 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_CalcSpeedAndYawFromControlStick` | 0.02% | 1.0% | 2,909 | 796 | 65 | 7,905 | 31,703 | src/overlays/actors/ovl_player_actor/z_player.c |
| `Player_UpdateZTargeting` | 0.01% | 1.0% | 3,849 | 592 | 49 | 4,347 | 38,474 | src/overlays/actors/ovl_player_actor/z_player.c |
| (95 more functions) | 0.26% | 17.1% | | | | | | |

## Actors: 1.48%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| ovl_En_Tite | 0.37% | 24.9% | 479,503 | 424,271 |
| ovl_En_Crow | 0.30% | 20.2% | 269,203 | 402,472 |
| ovl_En_Elf | 0.19% | 12.7% | 144,897 | 351,704 |
| ovl_En_River_Sound | 0.12% | 8.3% | 77,852 | 250,596 |
| ovl_Bg_Spot06_Objects | 0.10% | 6.6% | 78,283 | 156,492 |
| ovl_En_Sw | 0.09% | 6.1% | 73,192 | 161,687 |
| ovl_En_Kanban | 0.08% | 5.3% | 79,186 | 97,157 |
| ovl_En_Kakasi2 | 0.06% | 3.7% | 82,454 | 59,310 |
| ovl_Obj_Mure2 | 0.03% | 2.2% | 49,948 | 24,850 |
| ovl_En_Door | 0.03% | 2.0% | 32,285 | 43,812 |
| ovl_En_Weather_Tag | 0.03% | 1.8% | 22,639 | 51,411 |
| z_en_a_keep.c | 0.03% | 1.7% | 18,008 | 51,497 |
| ovl_En_Gs | 0.02% | 1.4% | 15,841 | 36,737 |
| ovl_Shot_Sun | 0.02% | 1.1% | 20,201 | 20,420 |
| ovl_En_Kusa | 0.01% | 0.6% | 11,201 | 8,865 |
| ovl_Bg_Mjin | 0.01% | 0.5% | 4,702 | 18,087 |
| ovl_Bg_Haka | 0.00% | 0.3% | 5,074 | 8,177 |
| ovl_En_Kakasi3 | 0.00% | 0.3% | 5,551 | 6,930 |
| ovl_Door_Ana | 0.00% | 0.3% | 3,522 | 7,303 |
| ovl_En_Wonder_Talk2 | 0.00% | 0.1% | 1,397 | 1,864 |
| z_en_item00.c | 0.00% | 0.0% | 107 | 319 |
| ovl_Door_Warp1 | 0.00% | 0.0% | 15 | 26 |
| horses | 0.00% | 0.0% | 9 | 20 |
| ovl_En_Ishi | 0.00% | 0.0% | 4 | 19 |
| ovl_Obj_Bean | 0.00% | 0.0% | 2 | 8 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `EnTite_Update` | 0.22% | 15.0% | 30,641 | 1,113 | 101 | 316,623 | 259,047 | src/overlays/actors/ovl_En_Tite/z_en_tite.c |
| `EnCrow_Update` | 0.09% | 6.3% | 10,253 | 1,392 | 132 | 90,173 | 140,129 | src/overlays/actors/ovl_En_Crow/z_en_crow.c |
| `BgSpot06Objects_DrawLakeHyliaWater.part.0` | 0.09% | 5.7% | 3,848 | 3,399 | 375 | 72,768 | 127,080 | src/overlays/actors/ovl_Bg_Spot06_Objects/z_bg_spot06_objects.c |
| `EnRiverSound_GetSfxPos` | 0.07% | 4.7% | 3,855 | 2,766 | 246 | 27,435 | 160,574 | src/overlays/actors/ovl_En_River_Sound/z_en_river_sound.c |
| `EnCrow_FlyIdle` | 0.07% | 4.4% | 9,842 | 1,018 | 101 | 52,244 | 106,340 | src/overlays/actors/ovl_En_Crow/z_en_crow.c |
| `EnCrow_PostLimbDraw` | 0.06% | 4.2% | 82,174 | 117 | 24 | 45,521 | 69,180 | src/overlays/actors/ovl_En_Crow/z_en_crow.c |
| `EnTite_CheckDamage` | 0.06% | 3.9% | 30,665 | 289 | 17 | 100,454 | 33,916 | src/overlays/actors/ovl_En_Tite/z_en_tite.c |
| `EnTite_Idle` | 0.05% | 3.1% | 13,746 | 509 | 52 | 42,213 | 75,656 | src/overlays/actors/ovl_En_Tite/z_en_tite.c |
| `EnKanban_Update` | 0.04% | 2.5% | 3,847 | 1,498 | 105 | 35,257 | 65,473 | src/overlays/actors/ovl_En_Kanban/z_en_kanban.c |
| `EnCrow_OverrideLimbDraw` | 0.03% | 2.4% | 82,175 | 65 | 18 | 27,681 | 26,982 | src/overlays/actors/ovl_En_Crow/z_en_crow.c |
| `func_80A053F0` | 0.03% | 2.4% | 3,849 | 1,392 | 80 | 32,787 | 56,892 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `EnKakasi2_Update` | 0.03% | 2.3% | 11,541 | 448 | 27 | 58,373 | 27,011 | src/overlays/actors/ovl_En_Kakasi2/z_en_kakasi2.c |
| `func_80B0D590` | 0.03% | 2.2% | 3,847 | 1,310 | 77 | 29,269 | 56,916 | src/overlays/actors/ovl_En_Sw/z_en_sw.c |
| `EnCrow_Draw` | 0.03% | 2.2% | 10,271 | 480 | 25 | 41,417 | 41,473 | src/overlays/actors/ovl_En_Crow/z_en_crow.c |
| `EnKanban_Draw` | 0.03% | 2.2% | 507 | 9,720 | 2,881 | 39,439 | 12,352 | src/overlays/actors/ovl_En_Kanban/z_en_kanban.c |
| `EnRiverSound_Update` | 0.03% | 2.0% | 3,853 | 1,195 | 68 | 40,924 | 45,240 | src/overlays/actors/ovl_En_River_Sound/z_en_river_sound.c |
| `func_80A03CF8` | 0.03% | 1.9% | 3,849 | 1,117 | 91 | 14,356 | 61,793 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `EnElf_Draw` | 0.02% | 1.5% | 3,849 | 871 | 57 | 23,508 | 35,195 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `func_80A90264` | 0.02% | 1.4% | 11,542 | 284 | 11 | 24,060 | 32,287 | src/overlays/actors/ovl_En_Kakasi2/z_en_kakasi2.c |
| `func_80A0461C` | 0.02% | 1.4% | 3,858 | 839 | 38 | 25,753 | 34,552 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `EnTite_PostLimbDraw` | 0.02% | 1.4% | 55,876 | 57 | 33 | 1,634 | 11,972 | src/overlays/actors/ovl_En_Tite/z_en_tite.c |
| `func_80B0CCF4` | 0.02% | 1.3% | 2,011 | 1,438 | 118 | 14,463 | 32,104 | src/overlays/actors/ovl_En_Sw/z_en_sw.c |
| `func_80A02C98` | 0.02% | 1.2% | 2,638 | 1,014 | 87 | 10,513 | 35,382 | src/overlays/actors/ovl_En_Elf/z_en_elf.c |
| `WeatherTag_CheckEnableWeatherEffect` | 0.02% | 1.2% | 3,850 | 693 | 49 | 14,886 | 31,971 | src/overlays/actors/ovl_En_Weather_Tag/z_en_weather_tag.c |
| `EnAObj_Update` | 0.02% | 1.1% | 3,851 | 674 | 45 | 14,093 | 30,805 | src/code/z_en_a_keep.c |
| (116 more functions) | 0.33% | 22.2% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `EnTite_Update` (src/overlays/actors/ovl_En_Tite/z_en_tite.c): 931: 10%, 29,316 data fills; 926: 10%, 35,883 data fills; 993: 9%, 37,088 data fills; 941: 7%, 28,137 data fills; 930: 6%, 28,733 data fills

## HUD and map: 1.31%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_parameter.c | 0.89% | 67.9% | 820,669 | 1,428,007 |
| z_lifemeter.c | 0.23% | 17.5% | 200,026 | 284,515 |
| z_map_exp.c | 0.19% | 14.6% | 159,125 | 295,756 |
| z_construct.c | 0.00% | 0.0% | 98 | 94 |
| speed_meter.c | 0.00% | 0.0% | 2 | 3 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Interface_Draw` | 0.28% | 21.7% | 3,849 | 11,359 | 896 | 281,656 | 443,350 | src/code/z_parameter.c |
| `Health_DrawMeter` | 0.19% | 14.7% | 3,850 | 7,699 | 1,468 | 168,644 | 213,833 | src/code/z_lifemeter.c |
| `Interface_DrawItemButtons` | 0.13% | 9.7% | 3,857 | 5,068 | 441 | 110,264 | 222,211 | src/code/z_parameter.c |
| `Magic_DrawMeter` | 0.11% | 8.7% | 3,858 | 4,516 | 358 | 116,366 | 176,261 | src/code/z_parameter.c |
| `Minimap_Draw` | 0.10% | 7.8% | 3,849 | 4,061 | 309 | 91,490 | 161,707 | src/code/z_map_exp.c |
| `Minimap_DrawCompassIcons` | 0.08% | 6.3% | 3,849 | 3,307 | 228 | 67,580 | 114,744 | src/code/z_map_exp.c |
| `Interface_InitVertices` | 0.08% | 6.1% | 3,849 | 3,183 | 374 | 61,898 | 134,732 | src/code/z_parameter.c |
| `Interface_DrawItemIconTexture` | 0.07% | 5.2% | 15,407 | 677 | 100 | 90,414 | 57,805 | src/code/z_parameter.c |
| `Interface_Update` | 0.06% | 4.9% | 3,850 | 2,585 | 153 | 52,570 | 123,056 | src/code/z_parameter.c |
| `func_80083108` | 0.04% | 3.1% | 3,849 | 1,637 | 170 | 19,058 | 91,855 | src/code/z_parameter.c |
| `Interface_DrawActionButton` | 0.03% | 2.6% | 3,850 | 1,356 | 114 | 25,622 | 57,746 | src/code/z_parameter.c |
| `Health_UpdateMeter` | 0.03% | 2.3% | 3,849 | 1,182 | 99 | 27,105 | 54,276 | src/code/z_lifemeter.c |
| `func_8008A8B8` | 0.03% | 2.1% | 7,729 | 541 | 58 | 30,876 | 27,605 | src/code/z_parameter.c |
| `Interface_DrawActionLabel` | 0.02% | 1.2% | 3,853 | 650 | 54 | 15,398 | 26,960 | src/code/z_parameter.c |
| `Interface_DrawAmmoCount` | 0.01% | 1.1% | 11,547 | 191 | 35 | 5,872 | 24,376 | src/code/z_parameter.c |
| `Magic_Update` | 0.01% | 0.7% | 3,828 | 342 | 28 | 3,894 | 19,169 | src/code/z_parameter.c |
| `Health_UpdateBeatingHeart` | 0.01% | 0.6% | 3,849 | 289 | 21 | 4,251 | 16,358 | src/code/z_lifemeter.c |
| `Map_Update` | 0.01% | 0.5% | 3,849 | 259 | 16 | 23 | 19,278 | src/code/z_map_exp.c |
| `Interface_SetDoAction` | 0.01% | 0.4% | 3,849 | 201 | 12 | 5,983 | 7,814 | src/code/z_parameter.c |
| `Interface_SetNaviCall` | 0.00% | 0.3% | 6,250 | 110 | 15 | 121 | 11,550 | src/code/z_parameter.c |
| `Interface_RaiseButtonAlphas` | 0.00% | 0.1% | 273 | 534 | 43 | 275 | 2,423 | src/code/z_parameter.c |
| `Interface_LoadActionLabel` | 0.00% | 0.0% | 116 | 643 | 53 | 365 | 945 | src/code/z_parameter.c |
| `Regs_InitDataImpl` | 0.00% | 0.0% | 1 | 7,644 | 481 | 58 | 60 | src/code/z_construct.c |
| `Interface_UpdateHudAlphas` | 0.00% | 0.0% | 8 | 889 | 56 | 24 | 104 | src/code/z_parameter.c |
| `Health_IsCritical` | 0.00% | 0.0% | 10 | 372 | 19 | 19 | 42 | src/code/z_lifemeter.c |
| (12 more functions) | 0.00% | 0.0% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Interface_Draw` (src/code/z_parameter.c): 3347: 14%, 58,651 data fills; 3253: 5%, 19,247 data fills; 3314: 3%, 17,239 data fills; 3372: 3%, 8,980 data fills; 4047: 3%, 7,696 data fills

## Camera and view: 1.09%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_camera.c | 0.65% | 59.0% | 494,986 | 1,130,283 |
| z_view.c | 0.37% | 33.7% | 340,098 | 500,688 |
| z_quake.c | 0.07% | 6.2% | 45,620 | 119,626 |
| shrink_window.c | 0.01% | 1.1% | 4,822 | 30,780 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Camera_Update` | 0.14% | 12.9% | 3,850 | 5,621 | 345 | 145,325 | 212,529 | src/code/z_camera.c |
| `View_ApplyOrthoToOverlay` | 0.11% | 9.8% | 7,699 | 2,139 | 199 | 91,305 | 165,591 | src/code/z_view.c |
| `View_ApplyPerspectiveToOverlay` | 0.11% | 9.8% | 7,698 | 2,139 | 285 | 105,461 | 120,471 | src/code/z_view.c |
| `Camera_Normal1` | 0.08% | 7.5% | 1,634 | 7,704 | 450 | 75,517 | 128,102 | src/code/z_camera.c |
| `View_ApplyPerspective` | 0.08% | 6.9% | 3,849 | 3,012 | 254 | 80,363 | 92,386 | src/code/z_view.c |
| `Camera_Jump3` | 0.07% | 6.2% | 1,829 | 5,689 | 356 | 60,487 | 106,971 | src/code/z_camera.c |
| `Camera_BGCheckInfo` | 0.06% | 5.2% | 4,840 | 1,785 | 106 | 56,630 | 86,086 | src/code/z_camera.c |
| `Quake_Update` | 0.04% | 3.6% | 3,849 | 1,551 | 203 | 33,779 | 63,284 | src/code/z_quake.c |
| `Camera_CalcUpFromPitchYawRoll` | 0.03% | 3.0% | 3,849 | 1,294 | 144 | 3,450 | 73,138 | src/code/z_camera.c |
| `View_LookAt` | 0.03% | 2.3% | 11,547 | 334 | 30 | 21,952 | 34,888 | src/code/z_view.c |
| `Quake_UpdateShakeInfo` | 0.02% | 2.2% | 4,354 | 856 | 115 | 9,906 | 48,439 | src/code/z_quake.c |
| `View_ApplyLetterbox` | 0.02% | 2.0% | 3,849 | 851 | 81 | 11,652 | 41,644 | src/code/z_view.c |
| `Camera_CalcDefaultYaw` | 0.02% | 1.9% | 3,086 | 1,041 | 75 | 7,469 | 43,356 | src/code/z_camera.c |
| `Camera_CalcAtDefault` | 0.02% | 1.9% | 3,463 | 903 | 70 | 14,266 | 34,745 | src/code/z_camera.c |
| `Camera_UpdateWater` | 0.02% | 1.9% | 3,850 | 809 | 72 | 9,234 | 48,684 | src/code/z_camera.c |
| `func_80046E20` | 0.02% | 1.8% | 3,082 | 1,003 | 70 | 15,054 | 36,367 | src/code/z_camera.c |
| `Camera_GetPitchAdjFromFloorHeightDiffs` | 0.02% | 1.8% | 1,634 | 1,874 | 139 | 16,176 | 31,920 | src/code/z_camera.c |
| `Camera_CalcDefaultPitch` | 0.02% | 1.6% | 2,916 | 892 | 80 | 6,189 | 35,899 | src/code/z_camera.c |
| `func_80045508` | 0.02% | 1.4% | 3,082 | 747 | 36 | 16,725 | 19,388 | src/code/z_camera.c |
| `Camera_UpdateDistortion` | 0.01% | 1.3% | 3,849 | 561 | 51 | 3,173 | 32,625 | src/code/z_camera.c |
| `Camera_RequestModeImpl` | 0.01% | 1.2% | 3,900 | 524 | 34 | 8,031 | 27,905 | src/code/z_camera.c |
| `Camera_GetWaterSurface` | 0.01% | 1.2% | 3,850 | 512 | 37 | 8,717 | 22,549 | src/code/z_camera.c |
| `Camera_ClampDist` | 0.01% | 1.1% | 3,463 | 520 | 39 | 3,117 | 27,406 | src/code/z_camera.c |
| `Camera_LERPCeilVec3f` | 0.01% | 1.0% | 7,476 | 230 | 33 | 2,418 | 23,111 | src/code/z_camera.c |
| `Camera_GetWaterBoxBgCamIndex` | 0.01% | 1.0% | 3,626 | 463 | 42 | 5,349 | 21,740 | src/code/z_camera.c |
| (44 more functions) | 0.11% | 9.7% | | | | | | |

## C library (memory copies, printf, 64-bit maths): 0.91%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| bzero.c | 0.32% | 35.0% | 554,315 | 55 |
| memset.c | 0.25% | 27.2% | 370,271 | 92,565 |
| ll.c | 0.21% | 23.2% | 10,494 | 31,734 |
| bcopy.c | 0.11% | 12.2% | 196,486 | 138,461 |
| memmove.c | 0.01% | 1.4% | 22,083 | 7,461 |
| xprintf.c | 0.00% | 0.4% | 1,995 | 6,394 |
| xlitob.c | 0.00% | 0.4% | 2,082 | 5,508 |
| string.c | 0.00% | 0.2% | 1,181 | 1,012 |
| ldiv.c | 0.00% | 0.1% | 597 | 1,443 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `bzero` | 0.32% | 35.0% | 18 | 2,708,892 | 153,999 | 554,315 | 55 | src/libultra/libc/bzero.c |
| `memset` | 0.25% | 27.2% | 51,029 | 741 | 185 | 370,271 | 92,565 | src/libc/memset.c |
| `__ull_div` | 0.21% | 22.9% | 340,744 | 94 | 14 | 10,114 | 28,536 | src/libultra/libc/ll.c |
| `bcopy` | 0.11% | 12.2% | 25,245 | 675 | 73 | 196,486 | 138,461 | src/libultra/libc/bcopy.c |
| `memmove` | 0.01% | 1.4% | 841 | 2,271 | 188 | 22,083 | 7,461 | src/libc/memmove.c |
| `_Printf` | 0.00% | 0.4% | 212 | 2,593 | 384 | 1,995 | 6,394 | src/libultra/libc/xprintf.c |
| `_Litob` | 0.00% | 0.4% | 382 | 1,298 | 163 | 2,082 | 5,508 | src/libultra/libc/xlitob.c |
| `lldiv` | 0.00% | 0.1% | 889 | 194 | 45 | 597 | 1,443 | src/libultra/libc/ldiv.c |
| `__ll_div` | 0.00% | 0.1% | 889 | 190 | 18 | 190 | 1,496 | src/libultra/libc/ll.c |
| `memcpy` | 0.00% | 0.1% | 1,388 | 115 | 40 | 974 | 438 | src/libultra/libc/string.c |
| `__ull_rem` | 0.00% | 0.1% | 382 | 231 | 14 | 190 | 954 | src/libultra/libc/ll.c |
| `strchr` | 0.00% | 0.0% | 764 | 88 | 27 | 196 | 570 | src/libultra/libc/string.c |
| `__ll_mul` | 0.00% | 0.0% | 889 | 65 | 12 | 0 | 748 | src/libultra/libc/ll.c |
| `strlen` | 0.00% | 0.0% | 25 | 167 | 98 | 11 | 4 | src/libultra/libc/string.c |

## Game loop (game.c, z_play.c): 0.61%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_play.c | 0.53% | 86.3% | 509,916 | 873,988 |
| game.c | 0.08% | 12.6% | 75,530 | 129,978 |
| z_frame_advance.c | 0.01% | 1.2% | 4,547 | 15,397 |
| title_setup.c | 0.00% | 0.0% | 4 | 6 |
| z_game_over.c | 0.00% | 0.0% | 1 | 1 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Play_Draw` | 0.22% | 35.7% | 3,849 | 8,739 | 572 | 238,348 | 352,003 | src/code/z_play.c |
| `Play_Update` | 0.16% | 26.3% | 3,858 | 6,416 | 514 | 147,876 | 291,044 | src/code/z_play.c |
| `func_800BFCB8` | 0.05% | 8.2% | 7,019 | 1,105 | 77 | 31,128 | 87,431 | src/code/z_play.c |
| `Play_SetFog` | 0.02% | 3.9% | 19,647 | 189 | 25 | 25,033 | 25,098 | src/code/z_play.c |
| `GameState_SetFrameBuffer` | 0.02% | 3.8% | 3,851 | 924 | 54 | 27,034 | 29,830 | src/code/game.c |
| `Play_Main` | 0.02% | 3.2% | 3,850 | 771 | 56 | 19,367 | 34,689 | src/code/z_play.c |
| `GameState_Draw` | 0.02% | 3.1% | 3,849 | 767 | 53 | 16,345 | 34,645 | src/code/game.c |
| `GameState_Update` | 0.02% | 2.7% | 3,850 | 665 | 37 | 22,720 | 26,973 | src/code/game.c |
| `FrameAdvance_IsEnabled` | 0.02% | 2.5% | 120,838 | 19 | 3 | 20,533 | 9,192 | src/code/z_play.c |
| `FishmodPlay_ClearZ` | 0.01% | 2.4% | 11,548 | 196 | 14 | 10,092 | 28,550 | src/code/z_play.c |
| `func_800C49F4` | 0.01% | 1.8% | 3,849 | 447 | 33 | 9,377 | 19,246 | src/code/game.c |
| `Play_InCsMode` | 0.01% | 1.8% | 11,522 | 143 | 11 | 6,971 | 23,024 | src/code/z_play.c |
| `Play_GetScreenPos` | 0.01% | 1.7% | 1,896 | 830 | 57 | 7,875 | 15,129 | src/code/z_play.c |
| `FrameAdvance_Update` | 0.01% | 1.2% | 3,850 | 284 | 8 | 4,546 | 15,396 | src/code/z_frame_advance.c |
| `GameState_ReqPadData` | 0.01% | 0.9% | 3,850 | 219 | 19 | 39 | 15,403 | src/code/game.c |
| `Play_GetCamera` | 0.00% | 0.6% | 3,915 | 153 | 7 | 2,626 | 7,724 | src/code/z_play.c |
| `GameState_IsRunning` | 0.00% | 0.2% | 3,862 | 51 | 2 | 3 | 3,851 | src/code/game.c |
| `Play_Init` | 0.00% | 0.0% | 1 | 5,326 | 339 | 30 | 61 | src/code/z_play.c |
| `GameState_Init` | 0.00% | 0.0% | 2 | 937 | 68 | 12 | 19 | src/code/game.c |
| `Play_SpawnScene` | 0.00% | 0.0% | 1 | 1,544 | 63 | 11 | 13 | src/code/z_play.c |
| `Play_InitScene` | 0.00% | 0.0% | 1 | 1,442 | 56 | 11 | 12 | src/code/z_play.c |
| `Play_SetupRespawnPoint` | 0.00% | 0.0% | 1 | 1,011 | 35 | 11 | 6 | src/code/z_play.c |
| `Setup_Init` | 0.00% | 0.0% | 1 | 590 | 26 | 4 | 5 | src/code/title_setup.c |
| `Play_SetupTransition` | 0.00% | 0.0% | 1 | 542 | 60 | 1 | 8 | src/code/z_play.c |
| `Play_LoadFile` | 0.00% | 0.0% | 1 | 383 | 23 | 3 | 4 | src/code/z_play.c |
| (6 more functions) | 0.00% | 0.0% | | | | | | |

Costliest lines (the function's own code, callees not counted; share of the function):

- `Play_Draw` (src/code/z_play.c): 1528: 4%, 7,733 data fills; 1226: 3%, 11,501 data fills; 1471: 3%, 7,701 data fills; 1547: 3%, 15,397 data fills; 1468: 3%, 10,221 data fills

## Loading (DMA, decompression, overlays): 0.57%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| yaz0.c | 0.44% | 78.1% | 428,326 | 3,990 |
| z_std_dma.c | 0.11% | 19.9% | 134,637 | 134,770 |
| loadfragment2_n64.c | 0.01% | 2.0% | 13,070 | 986 |
| z_DLF.c | 0.00% | 0.0% | 5 | 5 |
| z_locale.c | 0.00% | 0.0% | 2 | 5 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Yaz0_DecompressImpl` | 0.44% | 77.6% | 63 | 1,071,444 | 500,835 | 424,520 | 2,851 | src/boot/yaz0.c |
| `DmaMgr_ProcessRequest` | 0.05% | 9.7% | 5,187 | 1,620 | 215 | 78,662 | 38,879 | src/boot/z_std_dma.c |
| `DmaMgr_DmaRomToRam` | 0.03% | 5.9% | 7,176 | 716 | 72 | 35,043 | 50,008 | src/boot/z_std_dma.c |
| `DmaMgr_RequestAsync` | 0.02% | 3.4% | 5,188 | 563 | 32 | 19,944 | 32,220 | src/boot/z_std_dma.c |
| `Overlay_Relocate` | 0.01% | 2.0% | 31 | 54,880 | 22,735 | 12,823 | 562 | src/libu64/loadfragment2_n64.c |
| `DmaMgr_ThreadEntry` | 0.00% | 0.8% | 1 | 662,747 | 77,864 | 339 | 11,532 | src/boot/z_std_dma.c |
| `Yaz0_NextDMA` | 0.00% | 0.5% | 1,987 | 220 | 54 | 3,516 | 865 | src/boot/yaz0.c |
| `DmaMgr_AudioDmaHandler` | 0.00% | 0.1% | 318 | 249 | 20 | 534 | 925 | src/boot/z_std_dma.c |
| `DmaMgr_RequestSync` | 0.00% | 0.1% | 291 | 261 | 29 | 113 | 1,197 | src/boot/z_std_dma.c |
| `Overlay_Load` | 0.00% | 0.0% | 31 | 1,174 | 66 | 247 | 424 | src/libu64/loadfragment2_n64.c |
| `Yaz0_FirstDMA` | 0.00% | 0.0% | 63 | 310 | 38 | 118 | 188 | src/boot/yaz0.c |
| `Yaz0_Decompress` | 0.00% | 0.0% | 63 | 250 | 19 | 172 | 86 | src/boot/yaz0.c |
| `DmaMgr_Init` | 0.00% | 0.0% | 1 | 612 | 55 | 2 | 9 | src/boot/z_std_dma.c |
| `Overlay_LoadGameState` | 0.00% | 0.0% | 2 | 275 | 14 | 4 | 4 | src/code/z_DLF.c |
| `Locale_Init` | 0.00% | 0.0% | 1 | 422 | 19 | 2 | 5 | src/boot/z_locale.c |
| `Overlay_FreeGameState` | 0.00% | 0.0% | 1 | 142 | 10 | 1 | 1 | src/code/z_DLF.c |

## Display list setup: 0.46%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_rcp.c | 0.34% | 73.4% | 293,570 | 497,503 |
| gfxprint.c | 0.10% | 22.6% | 99,309 | 159,284 |
| sys_cfb.c | 0.01% | 1.4% | 7,706 | 11,561 |
| gfxalloc.c | 0.00% | 1.1% | 1,890 | 9,507 |
| sys_ucode.c | 0.00% | 0.9% | 3,851 | 7,700 |
| TwoHeadGfxArena.c | 0.00% | 0.6% | 0 | 7,704 |
| z_vimode.c | 0.00% | 0.0% | 12 | 13 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `GfxPrint_Setup` | 0.09% | 19.2% | 3,849 | 3,525 | 340 | 87,483 | 130,883 | src/libu64/gfxprint.c |
| `Gfx_SetupFrame` | 0.09% | 19.0% | 3,850 | 3,488 | 285 | 76,745 | 142,589 | src/code/z_rcp.c |
| `Gfx_SetFog2` | 0.05% | 10.9% | 27,487 | 281 | 44 | 13,385 | 75,202 | src/code/z_rcp.c |
| `Gfx_SetupDL_25Opa` | 0.05% | 10.8% | 121,870 | 63 | 9 | 57,320 | 54,753 | src/code/z_rcp.c |
| `Gfx_SetupDL_60NoCDXlu` | 0.04% | 8.8% | 57,929 | 108 | 17 | 57,848 | 29,697 | src/code/z_rcp.c |
| `Gfx_TwoTexScrollEnvColor` | 0.03% | 5.7% | 7,703 | 523 | 81 | 23,767 | 43,040 | src/code/z_rcp.c |
| `func_800947AC` | 0.02% | 3.8% | 22,762 | 117 | 12 | 24,594 | 18,181 | src/code/z_rcp.c |
| `Gfx_SetupDL_39Overlay` | 0.01% | 2.9% | 23,078 | 89 | 9 | 11,631 | 23,607 | src/code/z_rcp.c |
| `Gfx_SetupDL` | 0.01% | 2.8% | 38,054 | 53 | 11 | 8,733 | 20,006 | src/code/z_rcp.c |
| `Gfx_SetupDL_42Overlay` | 0.01% | 2.3% | 11,529 | 138 | 9 | 7,694 | 21,485 | src/code/z_rcp.c |
| `Gfx_SetupDL_25Xlu` | 0.01% | 2.2% | 13,011 | 122 | 9 | 3,159 | 24,977 | src/code/z_rcp.c |
| `func_80093C80` | 0.01% | 2.1% | 7,707 | 194 | 16 | 5,155 | 23,096 | src/code/z_rcp.c |
| `GfxPrint_Init` | 0.01% | 1.9% | 3,858 | 347 | 21 | 7,702 | 15,422 | src/libu64/gfxprint.c |
| `GfxPrint_Open` | 0.01% | 1.1% | 3,849 | 201 | 14 | 4,018 | 8,377 | src/libu64/gfxprint.c |
| `SysCfb_GetFbPtr` | 0.00% | 0.8% | 3,851 | 156 | 12 | 3,850 | 7,704 | src/code/sys_cfb.c |
| `Gfx_Open` | 0.00% | 0.6% | 19,269 | 23 | 2 | 0 | 8,283 | src/code/gfxalloc.c |
| `SysCfb_GetFbCount` | 0.00% | 0.6% | 3,850 | 110 | 3 | 3,850 | 3,850 | src/code/sys_cfb.c |
| `Gfx_SetupDL_54Opa` | 0.00% | 0.6% | 3,857 | 109 | 9 | 5 | 7,699 | src/code/z_rcp.c |
| `Gfx_SetupDL_40Opa` | 0.00% | 0.5% | 3,170 | 109 | 9 | 18 | 6,340 | src/code/z_rcp.c |
| `Gfx_SetupDL_25Xlu2` | 0.00% | 0.4% | 6,008 | 53 | 9 | 2,641 | 1,726 | src/code/z_rcp.c |
| `Gfx_Close` | 0.00% | 0.4% | 19,248 | 16 | 5 | 1,890 | 1,224 | src/code/gfxalloc.c |
| `GfxPrint_Close` | 0.00% | 0.3% | 3,871 | 59 | 6 | 82 | 3,849 | src/libu64/gfxprint.c |
| `SysUcode_GetUCode` | 0.00% | 0.3% | 3,850 | 59 | 3 | 3,849 | 0 | src/code/sys_ucode.c |
| `THGA_Init` | 0.00% | 0.3% | 15,421 | 14 | 2 | 0 | 3,854 | src/code/TwoHeadGfxArena.c |
| `THGA_IsCrash` | 0.00% | 0.3% | 11,565 | 18 | 2 | 0 | 3,850 | src/code/TwoHeadGfxArena.c |
| (12 more functions) | 0.01% | 1.1% | | | | | | |

## Other (no source): 0.37%

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `_icon_item_jpn_staticSegmentDataEnd` | 0.31% | 84.6% | 0 | 0 | 0 | 65,536 | 333,217 |  |
| `_n64ddSegmentBssEnd` | 0.06% | 15.2% | 0 | 0 | 0 | 38,191 | 2,709 |  |
| `leoDiskStack` | 0.00% | 0.2% | 0 | 0 | 0 | 845 | 284 |  |
| `gN64DDDiskReadTemporaryBuffer` | 0.00% | 0.0% | 0 | 0 | 0 | 12 | 26 |  |
| `gPauseMapMarkDataTable` | 0.00% | 0.0% | 0 | 0 | 0 | 8 | 11 |  |
| `LEOC2_Syndrome` | 0.00% | 0.0% | 0 | 0 | 0 | 1 | 5 |  |
| `LEOinterruptThreadStack` | 0.00% | 0.0% | 0 | 0 | 0 | 2 | 4 |  |
| `LEOevent_que` | 0.00% | 0.0% | 0 | 0 | 0 | 2 | 2 |  |
| `LEOcommand_que` | 0.00% | 0.0% | 0 | 0 | 0 | 3 | 0 |  |
| `LEOcontrol_que` | 0.00% | 0.0% | 0 | 0 | 0 | 0 | 2 |  |
| `LEOdma_que` | 0.00% | 0.0% | 0 | 0 | 0 | 1 | 0 |  |

## Scenes, rooms and objects: 0.37%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_room.c | 0.19% | 51.0% | 165,599 | 162,148 |
| z_scene.c | 0.13% | 36.7% | 205,248 | 50,307 |
| z_scene_table.c | 0.05% | 12.3% | 42,491 | 77,065 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Room_DrawCullable` | 0.17% | 46.0% | 3,849 | 6,713 | 1,711 | 149,864 | 126,903 | src/code/z_room.c |
| `Object_IsLoaded` | 0.09% | 23.6% | 395,357 | 34 | 7 | 128,170 | 22,222 | src/code/z_scene.c |
| `Object_UpdateEntries` | 0.04% | 11.0% | 3,849 | 1,609 | 157 | 65,501 | 23,133 | src/code/z_scene.c |
| `Scene_DrawConfigLakeHylia` | 0.04% | 10.5% | 3,849 | 1,536 | 158 | 34,771 | 65,479 | src/code/z_scene_table.c |
| `Room_Draw` | 0.01% | 3.8% | 7,706 | 278 | 19 | 11,854 | 27,494 | src/code/z_room.c |
| `Object_GetSlot` | 0.01% | 2.1% | 2,320 | 504 | 56 | 11,388 | 4,788 | src/code/z_scene.c |
| `Scene_Draw` | 0.01% | 1.8% | 3,849 | 261 | 12 | 7,720 | 11,586 | src/code/z_scene_table.c |
| `Room_ProcessRoomRequest` | 0.00% | 0.9% | 3,850 | 124 | 5 | 3,855 | 3,875 | src/code/z_room.c |
| `func_80095AA0` | 0.00% | 0.4% | 7,710 | 26 | 2 | 0 | 3,849 | src/code/z_room.c |
| `Object_LoadPersistent` | 0.00% | 0.0% | 4 | 1,018 | 48 | 31 | 31 | src/code/z_scene.c |
| `Scene_CommandAlternateHeaderList` | 0.00% | 0.0% | 2 | 1,922 | 167 | 37 | 24 | src/code/z_scene.c |
| `Scene_CommandObjectList` | 0.00% | 0.0% | 1 | 2,893 | 405 | 38 | 13 | src/code/z_scene.c |
| `Object_InitContext` | 0.00% | 0.0% | 1 | 2,425 | 104 | 24 | 9 | src/code/z_scene.c |
| `Room_RequestNewRoom.part.0` | 0.00% | 0.0% | 1 | 1,608 | 79 | 13 | 12 | src/code/z_room.c |
| `Room_SetupFirstRoom` | 0.00% | 0.0% | 1 | 1,558 | 104 | 11 | 14 | src/code/z_room.c |
| `Scene_ExecuteCommands` | 0.00% | 0.0% | 2 | 636 | 35 | 8 | 13 | src/code/z_scene.c |
| `Scene_CommandTimeSettings` | 0.00% | 0.0% | 1 | 1,136 | 76 | 8 | 12 | src/code/z_scene.c |
| `Scene_CommandCollisionHeader` | 0.00% | 0.0% | 1 | 970 | 73 | 6 | 10 | src/code/z_scene.c |
| `Scene_CommandSpecialFiles` | 0.00% | 0.0% | 1 | 800 | 47 | 6 | 8 | src/code/z_scene.c |
| `Scene_CommandPlayerEntryList` | 0.00% | 0.0% | 1 | 693 | 41 | 8 | 6 | src/code/z_scene.c |
| `Scene_CommandMiscSettings` | 0.00% | 0.0% | 1 | 599 | 32 | 4 | 5 | src/code/z_scene.c |
| `Scene_CommandSkyboxSettings` | 0.00% | 0.0% | 1 | 398 | 10 | 3 | 3 | src/code/z_scene.c |
| `Scene_CommandRoomList` | 0.00% | 0.0% | 1 | 326 | 19 | 2 | 3 | src/code/z_scene.c |
| `Scene_CommandSpawnList` | 0.00% | 0.0% | 1 | 322 | 17 | 2 | 3 | src/code/z_scene.c |
| `Scene_CommandExitList` | 0.00% | 0.0% | 1 | 322 | 17 | 2 | 3 | src/code/z_scene.c |
| (11 more functions) | 0.00% | 0.0% | | | | | | |

## Text: 0.05%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_message.c | 0.03% | 62.3% | 28,364 | 65,836 |
| z_elf_message.c | 0.02% | 37.6% | 7,939 | 43,598 |
| z_kanfont.c | 0.00% | 0.1% | 39 | 12 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Message_DrawMain` | 0.01% | 26.8% | 3,849 | 585 | 47 | 11,149 | 26,960 | src/code/z_message.c |
| `Message_Draw` | 0.01% | 17.3% | 3,850 | 379 | 37 | 2,084 | 23,098 | src/code/z_message.c |
| `QuestHint_GetTextIdFromScript` | 0.01% | 17.2% | 2,401 | 604 | 69 | 2,549 | 21,742 | src/code/z_elf_message.c |
| `QuestHint_CheckCondition` | 0.01% | 15.1% | 7,203 | 176 | 36 | 2,989 | 17,054 | src/code/z_elf_message.c |
| `Message_GetState` | 0.01% | 11.2% | 11,418 | 83 | 7 | 11,280 | 8,077 | src/code/z_message.c |
| `Message_Update` | 0.00% | 6.9% | 3,849 | 151 | 7 | 3,849 | 7,698 | src/code/z_message.c |
| `QuestHint_GetNaviTextId` | 0.00% | 5.2% | 2,401 | 183 | 7 | 2,401 | 4,802 | src/code/z_elf_message.c |
| `Font_LoadOrderedFont` | 0.00% | 0.1% | 1 | 7,349 | 3,904 | 39 | 12 | src/code/z_kanfont.c |
| `Message_SetTables` | 0.00% | 0.0% | 1 | 323 | 17 | 2 | 3 | src/code/z_message.c |

## Memory: 0.05%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| __osMalloc_n64.c | 0.04% | 80.0% | 91,101 | 5,958 |
| TwoHeadArena.c | 0.01% | 19.0% | 13,830 | 11,709 |
| z_malloc.c | 0.00% | 0.7% | 0 | 1,062 |
| malloc.c | 0.00% | 0.3% | 6 | 390 |
| gamealloc.c | 0.00% | 0.0% | 4 | 15 |
| listalloc.c | 0.00% | 0.0% | 0 | 1 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `__osMalloc` | 0.03% | 51.7% | 576 | 6,871 | 589 | 62,172 | 3,118 | src/libc64/__osMalloc_n64.c |
| `ArenaImpl_GetSizes` | 0.01% | 23.6% | 244 | 7,420 | 862 | 25,625 | 501 | src/libc64/__osMalloc_n64.c |
| `THA_Init` | 0.01% | 14.1% | 15,404 | 70 | 6 | 12,455 | 7,808 | src/code/TwoHeadArena.c |
| `THA_IsCrash` | 0.00% | 4.9% | 11,547 | 32 | 5 | 1,356 | 3,882 | src/code/TwoHeadArena.c |
| `__osFree` | 0.00% | 3.9% | 863 | 348 | 31 | 2,412 | 2,263 | src/libc64/__osMalloc_n64.c |
| `__osMallocR` | 0.00% | 0.8% | 7 | 8,276 | 440 | 888 | 70 | src/libc64/__osMalloc_n64.c |
| `ZeldaArena_Malloc` | 0.00% | 0.3% | 564 | 47 | 4 | 0 | 508 | src/code/z_malloc.c |
| `SystemArena_GetSizes` | 0.00% | 0.3% | 122 | 164 | 14 | 1 | 366 | src/libc64/malloc.c |
| `ZeldaArena_Free` | 0.00% | 0.2% | 858 | 21 | 4 | 0 | 302 | src/code/z_malloc.c |
| `ZeldaArena_GetSizes` | 0.00% | 0.2% | 122 | 102 | 6 | 0 | 244 | src/code/z_malloc.c |
| `THA_AllocTailAlign16` | 0.00% | 0.0% | 25 | 78 | 7 | 16 | 14 | src/code/TwoHeadArena.c |
| `SystemArena_Malloc` | 0.00% | 0.0% | 13 | 118 | 18 | 3 | 20 | src/libc64/malloc.c |
| `GameAlloc_Malloc` | 0.00% | 0.0% | 3 | 220 | 24 | 2 | 8 | src/code/gamealloc.c |
| `__osMallocInit` | 0.00% | 0.0% | 2 | 328 | 18 | 4 | 6 | src/libc64/__osMalloc_n64.c |
| `SystemArena_Free` | 0.00% | 0.0% | 5 | 92 | 16 | 2 | 4 | src/libc64/malloc.c |
| `THA_AllocTailAlign` | 0.00% | 0.0% | 5 | 68 | 6 | 2 | 3 | src/code/TwoHeadArena.c |
| `ZeldaArena_MallocR` | 0.00% | 0.0% | 7 | 45 | 4 | 0 | 6 | src/code/z_malloc.c |
| `GameAlloc_Cleanup` | 0.00% | 0.0% | 1 | 254 | 20 | 1 | 3 | src/code/gamealloc.c |
| `GameAlloc_Init` | 0.00% | 0.0% | 2 | 118 | 4 | 1 | 3 | src/code/gamealloc.c |
| `THA_GetRemaining` | 0.00% | 0.0% | 1 | 181 | 4 | 1 | 2 | src/code/TwoHeadArena.c |
| `ZeldaArena_Init` | 0.00% | 0.0% | 1 | 101 | 5 | 0 | 2 | src/code/z_malloc.c |
| `GameAlloc_Free` | 0.00% | 0.0% | 1 | 66 | 11 | 0 | 1 | src/code/gamealloc.c |
| `ListAlloc_Init` | 0.00% | 0.0% | 1 | 54 | 4 | 0 | 1 | src/code/listalloc.c |
| `THA_Destroy` | 0.00% | 0.0% | 2 | 3 | 3 | 0 | 0 | src/code/TwoHeadArena.c |

## Pause menu and screen effects: 0.04%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_kaleido_setup.c | 0.02% | 49.8% | 15,838 | 42,353 |
| PreRender.c | 0.01% | 28.0% | 19,290 | 7,783 |
| z_kaleido_scope_call.c | 0.01% | 22.1% | 11,507 | 15,479 |
| ovl_kaleido_scope | 0.00% | 0.1% | 24 | 51 |
| z_kaleido_manager.c | 0.00% | 0.0% | 24 | 28 |
| z_vismono.c | 0.00% | 0.0% | 0 | 6 |
| z_viszbuffer.c | 0.00% | 0.0% | 2 | 4 |
| z_viscvg.c | 0.00% | 0.0% | 2 | 3 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `KaleidoSetup_Update` | 0.02% | 49.8% | 3,849 | 848 | 59 | 15,823 | 42,343 | src/code/z_kaleido_setup.c |
| `PreRender_SetValues` | 0.01% | 28.0% | 3,862 | 475 | 12 | 19,290 | 7,779 | src/code/PreRender.c |
| `KaleidoScopeCall_LoadPlayer` | 0.01% | 22.1% | 7,709 | 188 | 12 | 11,504 | 15,475 | src/code/z_kaleido_scope_call.c |
| `KaleidoScope_DrawWorldMap` | 0.00% | 0.0% | 0 | 0 | 0 | 12 | 17 | src/overlays/misc/ovl_kaleido_scope/z_kaleido_map.c |
| `KaleidoManager_GetRamAddr` | 0.00% | 0.0% | 6 | 224 | 29 | 8 | 16 | src/code/z_kaleido_manager.c |
| `KaleidoSetup_Init` | 0.00% | 0.0% | 1 | 1,180 | 70 | 15 | 10 | src/code/z_kaleido_setup.c |
| `KaleidoScope_DrawInventoryEditor` | 0.00% | 0.0% | 0 | 0 | 0 | 4 | 9 | src/overlays/misc/ovl_kaleido_scope/z_kaleido_debug.c |
| `KaleidoScope_Update` | 0.00% | 0.0% | 0 | 0 | 0 | 3 | 9 | src/overlays/misc/ovl_kaleido_scope/z_kaleido_scope.c |
| `KaleidoManager_ClearOvl` | 0.00% | 0.0% | 1 | 657 | 20 | 6 | 4 | src/code/z_kaleido_manager.c |
| `KaleidoScope_DrawDungeonMap` | 0.00% | 0.0% | 0 | 0 | 0 | 3 | 7 | src/overlays/misc/ovl_kaleido_scope/z_kaleido_map.c |
| `KaleidoManager_LoadOvl` | 0.00% | 0.0% | 1 | 544 | 23 | 6 | 4 | src/code/z_kaleido_manager.c |
| `KaleidoManager_Init` | 0.00% | 0.0% | 1 | 541 | 25 | 4 | 4 | src/code/z_kaleido_manager.c |
| `KaleidoScope_QuadTextureIA8` | 0.00% | 0.0% | 1 | 372 | 37 | 2 | 4 | src/overlays/misc/ovl_kaleido_scope/z_kaleido_scope.c |
| `VisMono_Init` | 0.00% | 0.0% | 3 | 114 | 13 | 0 | 6 | src/code/z_vismono.c |
| `KaleidoScopeCall_Init` | 0.00% | 0.0% | 1 | 334 | 19 | 3 | 4 | src/code/z_kaleido_scope_call.c |
| `VisZBuffer_Init` | 0.00% | 0.0% | 2 | 166 | 7 | 2 | 4 | src/code/z_viszbuffer.c |
| `VisCvg_Init` | 0.00% | 0.0% | 2 | 115 | 5 | 2 | 2 | src/code/z_viscvg.c |
| `KaleidoScope_QuadTextureIA4` | 0.00% | 0.0% | 0 | 0 | 0 | 0 | 3 | src/overlays/misc/ovl_kaleido_scope/z_kaleido_scope.c |
| `PreRender_Init` | 0.00% | 0.0% | 1 | 160 | 12 | 0 | 3 | src/code/PreRender.c |
| `KaleidoScope_DrawDigit` | 0.00% | 0.0% | 0 | 0 | 0 | 0 | 2 | src/overlays/misc/ovl_kaleido_scope/z_kaleido_debug.c |
| `PreRender_SetValuesSave` | 0.00% | 0.0% | 1 | 73 | 14 | 0 | 1 | src/code/PreRender.c |
| `VisCvg_Destroy` | 0.00% | 0.0% | 1 | 50 | 2 | 0 | 1 | src/code/z_viscvg.c |
| `VisMono_Destroy` | 0.00% | 0.0% | 1 | 3 | 2 | 0 | 0 | src/code/z_vismono.c |
| `VisZBuffer_Destroy` | 0.00% | 0.0% | 1 | 2 | 2 | 0 | 0 | src/code/z_viszbuffer.c |

## Screen transitions: 0.01%

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `TransitionFade_Update` | 0.01% | 62.1% | 3,869 | 351 | 24 | 4,095 | 19,471 | src/code/z_fbdemo_fade.c |
| `TransitionFade_Draw` | 0.01% | 37.8% | 3,870 | 214 | 5 | 1,240 | 15,436 | src/code/z_fbdemo_fade.c |
| `TransitionFade_IsDone` | 0.00% | 0.1% | 21 | 126 | 2 | 21 | 21 | src/code/z_fbdemo_fade.c |
| `TransitionFade_Init` | 0.00% | 0.0% | 2 | 210 | 14 | 1 | 6 | src/code/z_fbdemo_fade.c |
| `TransitionFade_Start` | 0.00% | 0.0% | 2 | 110 | 10 | 0 | 4 | src/code/z_fbdemo_fade.c |
| `TransitionFade_SetType` | 0.00% | 0.0% | 2 | 105 | 8 | 0 | 4 | src/code/z_fbdemo_fade.c |
| `TransitionFade_SetColor` | 0.00% | 0.0% | 2 | 27 | 2 | 0 | 1 | src/code/z_fbdemo_fade.c |
| `TransitionFade_Destroy` | 0.00% | 0.0% | 1 | 50 | 2 | 0 | 1 | src/code/z_fbdemo_fade.c |

## Cutscenes: 0.01%

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `Cutscene_UpdateManual` | 0.01% | 46.4% | 3,859 | 246 | 13 | 8,038 | 11,547 | src/code/z_demo.c |
| `Cutscene_UpdateScripted` | 0.01% | 44.0% | 3,849 | 234 | 11 | 3,850 | 11,548 | src/code/z_demo.c |
| `CutsceneHandler_DoNothing` | 0.00% | 9.4% | 3,859 | 50 | 2 | 0 | 3,849 | src/code/z_demo.c |
| `Cutscene_HandleEntranceTriggers` | 0.00% | 0.1% | 1 | 1,974 | 339 | 20 | 8 | src/code/z_demo.c |
| `Cutscene_HandleConditionalTriggers` | 0.00% | 0.0% | 1 | 958 | 61 | 5 | 10 | src/code/z_demo.c |
| `Cutscene_InitContext` | 0.00% | 0.0% | 1 | 211 | 3 | 2 | 1 | src/code/z_demo.c |

## Save data and inventory: 0.00%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| z_ss_sram.c | 0.00% | 94.0% | 562 | 1,226 |
| z_inventory.c | 0.00% | 3.7% | 39 | 39 |
| z_sram.c | 0.00% | 1.8% | 15 | 15 |
| z_common_data.c | 0.00% | 0.4% | 3 | 5 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `SsSram_ReadWrite` | 0.00% | 69.6% | 122 | 673 | 55 | 559 | 771 | src/code/z_ss_sram.c |
| `SsSram_Dma` | 0.00% | 24.4% | 122 | 236 | 42 | 3 | 455 | src/code/z_ss_sram.c |
| `Inventory_ChangeEquipment` | 0.00% | 3.7% | 18 | 241 | 17 | 39 | 39 | src/code/z_inventory.c |
| `Sram_InitDebugSave` | 0.00% | 1.8% | 1 | 2,125 | 191 | 15 | 14 | src/code/z_sram.c |
| `SaveContext_Init` | 0.00% | 0.4% | 1 | 518 | 30 | 3 | 5 | src/code/z_common_data.c |
| `Sram_Init` | 0.00% | 0.0% | 1 | 50 | 2 | 0 | 1 | src/code/z_sram.c |

## Other engine code: 0.00%

| part | share | of the system | data fills | code fills |
|---|---:|---:|---:|---:|
| entry.c | 0.00% | 82.0% | 1,196 | 2 |
| kanji_tex.c | 0.00% | 17.1% | 177 | 30 |
| driveexist.c | 0.00% | 0.9% | 2 | 8 |

| function | share | of the system | calls | cycles/call | instr/call | data fills | code fills | file |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `entrypoint` | 0.00% | 82.0% | 1 | 90,853 | 11,969 | 1,196 | 2 | src/makerom/entry.c |
| `KanjiTex_CodepointToOffset` | 0.00% | 17.1% | 236 | 80 | 22 | 177 | 30 | src/code/kanji_tex.c |
| `LeoDriveExist` | 0.00% | 0.9% | 1 | 959 | 43 | 2 | 8 | src/libleo/api/driveexist.c |

