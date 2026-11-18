# Tool catalogue

Every tool written for this project that is not itself part of the game, in one list. The files stay where the build
and the docs expect them; this page is the one place to read what exists.

The last column says how far each tool travels as it stands. It is a reading of what each file depends on; none has
been tried outside this repository except `n64prof/`, which was written to stand alone.

- **any project**: no dependency on the N64 or this game.
- **any N64 project**: needs an N64 ROM, ELF or the patched emulator, but nothing of this game.
- **this game**: reads this mod's data formats, save layout or source tree. The method may carry; the file would need work.

Not listed: the in-ROM test harnesses (`src/code/fishmod_debug*.c` and the 155 scripts in `fishmod/scripts/`). They
are compiled into debug builds of this game and most readers below depend on them.

## Emulator: patches and runners

Patches to ares v148 and the scripts that build and run it headless. Everything measured in the emulator depends on these.

| tool | what it does | travels to |
|---|---|---|
| `fishmod/tools/ares-prof.patch` | ares patch: exact CPU profile per instruction (`ARES_CPUPROF`, cut per test leg by a RAM marker), RSP profile by command (`ARES_RSPPROF`), bytes moved on the memory bus by requester (`ARES_BUSPROF`), instruction traces for layout (`ARES_ITRACE`), SP DMA log (`ARES_SPDMALOG`). | any N64 project |
| `fishmod/tools/ares-aiprof.patch` | ares patch: counts what the audio interface did (`ARES_AIPROF`): buffers taken, refused, times it ran dry, silent samples, per test leg. | any N64 project |
| `fishmod/tools/ares-autoshot.patch` | ares patch: saves every Nth frame raw (`ARES_AUTOSHOT`), for exact A/B picture comparisons. | any N64 project |
| `fishmod/tools/ares-nodraw.patch` | ares patch: `ARES_NORDP=1` reads RDP commands and raises its interrupts but draws nothing, for faster runs that only need CPU and sound results. | any N64 project |
| `fishmod/tools/ares-sc64.patch` | ares patch: a minimal SummerCart64 (`ARES_SC64_DIR`), so a ROM's USB logging can be tested without the cartridge. | any N64 project |
| `fishmod/tools/build_ares.sh` | Builds ares (N64 core only) from source for the capture tools (harness.py, ares_capture.sh, regress.sh), with ares-autoshot.patch: ARES_AUTOSHOT=<dir> saves a screenshot every ARES_AUTOSHOT_EVERY frames (default 15), and ... | any N64 project |
| `fishmod/tools/ares_run.sh` | Runs a ROM in ares headless for a while, several at once if wanted: each run gets its own GDB port and its own SummerCart64 USB folder, so runs don't collide (two ares sharing a port or folder log nothing). | any N64 project |
| `fishmod/tools/ares_capture.sh` | Runs a ROM in ares on a virtual X display and captures screenshots (fishmod/tools/harness.py does the work). | any N64 project |
| `fishmod/tools/harness.py` | Runs a ROM in ares on a virtual X display: the one launcher the capture tools share. | any N64 project |
| `fishmod/tools/ares_debug.bml` | ares settings file used by the headless runs. | any N64 project |
| `fishmod/tools/build_n64_gcc.sh` | Builds a bare-metal GCC for the N64 (target mips64-elf, the VR4300's instructions and timings by default) from the GNU sources: the compiler proper and binutils, no C library (the game has its own). | any N64 project |

## Profilers and cost readers

Where CPU, RSP, RDP and memory-bus time goes. Most read the files the ares patches write, plus an ELF.

| tool | what it does | travels to |
|---|---|---|
| `fishmod/tools/cpuprof.py` | Where the CPU's time goes, by function, exactly: reads the profile ares writes with ARES_CPUPROF=<file> (fishmod/tools/ares-prof.patch: the CPU on the interpreter, every instruction counted by PC with its clocks and the data cache lines it ... | any N64 project |
| `fishmod/tools/rspprof.py` | Where the audio microcode's RSP time goes, by command: reads the profile ares writes with ARES_RSPPROF=<file> (fishmod/tools/ares-prof.patch: every RSP instruction and its clocks counted by PC, on the interpreter, so the counts are exact ... | any N64 project |
| `fishmod/tools/busprof.py` | The bytes moved to and from RDRAM, by who asked, from the file ares writes with ARES_BUSPROF=<file> (fishmod/tools/ares-prof.patch): the CPU's cache fills and write-backs (the audio driver's own apart, by its code's addresses), the RSP's ... | any N64 project |
| `fishmod/tools/hotcode.py` | Finds code that may be fighting the instruction cache: hot paths bigger than the cache can comfortably keep. | any N64 project |
| `fishmod/tools/codescan.py` | A search through the built code itself (no profile, nothing run) for loops and routines whose code is large for the instruction cache. | any N64 project |
| `fishmod/tools/dlstat.py` | What a frame's display list asks of the RSP and the RDP: every command it runs, walked out of a RAM dump the way F3DEX3 walks it (segments resolved, called and branched lists followed), counted by kind; the RDP state commands that change ... | any N64 project |
| `fishmod/tools/rdp_cost.py` | Where a frame's RDP time goes, triangle by triangle: last frame's display list walked out of a RAM dump as F3DEX3 runs it (segments, calls, the matrix stack, the vertex buffer, CULLDL), each triangle transformed, clipped to the near plane ... | any N64 project |
| `n64prof/` | A PC-sampling profiler for any libultra or libdragon program: in-ROM sampler, host reader, output for compilers' sample profiles. Already written to stand alone. | any N64 project |
| `fishmod/tools/sysprof.py` | Where the CPU's time goes, by system: every part of the engine and the mod, each one's files, functions and lines. | this game |
| `fishmod/tools/inst_profile.py` | Where the CPU's time lands, by the kind of instruction it was on (a console profile streamed over USB: one bucket per instruction), to tell waiting on memory from computing. | this game |
| `fishmod/tools/pc_ram.py` | The PC profile between two RAM dumps of an emulator run (a FISHMOD_PCPROF build without a SummerCart64: the samples count into the table in RAM, fishmod_debug_pcprof.inc.c sPcTable), by function. | this game |
| `fishmod/tools/rdp_cost_props.py` | The props' share of the RDP over several frames (rdp_cost.py --json outputs): by kind, and by kind and tier (the model list's name: ...DL near (cut cards), ...DLFar whole cards, ...DLFarH half texture, ...DLImp single card). | this game |
| `fishmod/tools/rdpbench.py` | Reads the RDP calibration bench's run (fishmod_debug_rdpbench.inc.c, FISHMOD_RDP_BENCH): each test's RDP time and counters from the frame log, less the empty frame's, and what that comes to per pixel and per primitive. | this game |
| `fishmod/tools/framelog.py` | Reads fish-oot's frame log (FISHMOD_FRAMELOG: 'FFRM' packets over a SummerCart64's USB, saved by `sc64deployer debug` as binaryout-*.bin) and says where the slow frames' time goes. | this game |
| `fishmod/tools/framelog_vs.py` | Two frame logs (fishmod/tools/framelog.py's input) side by side on the same route: area by area (each leg, all laps together), each part of the frame, and overall. | this game |

## Same-answer checks

Prove a change gives the same answers, sound or picture as before.

| tool | what it does | travels to |
|---|---|---|
| `fishmod/tools/audlog_compare.py` | Compares two audio check logs (fishmod/tools/collog.sh --audio, the audlog script): output buffer by output buffer, the hash of its samples. | any N64 project |
| `fishmod/tools/collog.sh` | Runs a collog ROM (fishmod/scripts/collog.inc.c) in ares for a while, then saves its collision log (one record a frame: the hash of every collision question and answer, and the calls of each kind) from the emulator's memory through ares' GDB server. | this game |
| `fishmod/tools/collog_compare.py` | Compares two collision logs (fishmod/tools/collog.sh): frame by frame, the hash of every collision question and answer. | this game |
| `fishmod/tools/animcheck_compare.py` | Compares two animation check logs (fishmod/scripts/animcheck.inc.c, saved by fishmod/tools/collog.sh --anim): case by case, the pose (the joint table and the limb matrices: level 1) and the opaque and translucent draw streams (every triangle with its vertices and the state it's drawn with: level 2). | this game |
| `fishmod/tools/gen_animcheck_cases.py` | The animation check's case table (fishmod/docs/anim-verify.md): every one of Link's animations, read from the game's own list of them (assets/objects/gameplay_keep/player_anim_headers.c), so none is missed and a new one is picked up. | this game |
| `fishmod/tools/state_hash.py` | Compares the state fingerprints two script runs wrote to SRAM (fishmod_debug.c, StateHash_Update): every 200 frames, a hash of every actor's id, params, position and rotation. | this game |
| `fishmod/tools/settrace.py` | Two builds' runs of one scripted route, laid side by side: every set of props Scatter_Build made (the frame, the camera, what's in it) and Link's place every 8th frame, from memory dumps of gFishmodSetTrace and gFishmodLinkTrace taken ... | this game |
| `fishmod/tools/rigcheck.py` | Checks the fish rig's cleaned display lists against the lists as they were (fishmod_rig.c, Rig_Peephole; debug builds keep both: gFishmodRigCheck). | this game |
| `fishmod/tools/spdmalog_compare.py` | Compares two SP DMA logs (`ARES_SPDMALOG`) task by task. | any N64 project |
| `fishmod/tools/skin_check.c` | SkinMatrix's translate-rotate(-scale) matrices written directly (src/code/z_skin_matrix.c) against the original products, for random rotations, scales and positions: every entry must compare equal (== on floats: a zero's sign is the only difference the products can make). | this game |
| `fishmod/tools/tri_check.c` | Checks include/fishmod_tri.h (the cheaper Math3D_TriChkPointParaYImpl) against the original, copied verbatim below, on random triangles and points: anywhere near, on a corner, on an edge, just off an edge; every tolerance the game uses (chkDist 0-50, detMax 0 and 300). | this game |
| `fishmod/tools/trig_check.c` | Checks fish-oot's polynomial trig (include/fishmod_trig.h) against double precision on the PC, with the originals it replaces (libultra's sine table, Math_Atan2S's table, Math_FAtanContFracF's continued fraction) for comparison. | any project |
| `tools/bgcheck_check/` | Builds the collision code at two revisions and asks both the same questions on every scene; answers must match bit for bit. | this game |
| `tools/check_overlay_relocs.py` | Checks that an object's relocations come out right through the game's overlay relocation (Overlay_Relocate, src/libu64/relocation_gc.c; fado copies the relocations in the object's order): the loader pairs each R_MIPS_LO16 with the last ... | this game |
| `fishmod/tools/regress.sh` | Regression runs for refactoring: builds a fixed set of scripted scenarios, runs each in ares long enough for all its state fingerprints (fishmod_debug.c, StateHash_Update), and keeps their SRAM and a few screenshots. | this game |
| `fishmod/tools/regress_run.sh` | Runs ROMs already built by regress.sh again (no build): <outdir>/<from>/<script>/r.z64 -> <outdir>/<to>/<script>/. | this game |
| `fishmod/tools/recheck.sh` | Compiles just the given source files with the exact commands a build used (from its log), to check a change in seconds instead of a whole build (compile, link, compress, patch). | any project |
| `fishmod/tools/flip_metric.py` | FLIP (Andersson et al. 2020, "FLIP: A Difference Evaluator for Alternating Images"), the LDR model, in numpy: how visible the difference between two images is to someone watching one replace the other, per pixel, 0 (none) to 1. | any project |
| `fishmod/tools/popin_check.py` | Pop-in check: from a RAM dump of a debug build (framedump.sh), every prop that should be on screen by the last set's own view (inside the picture, within its kind's draw distance, not on a dry chunk, not right against the camera) against ... | this game |
| `fishmod/tools/inputrec.py` | The input recorder's PC side (src/code/fishmod_debug_inputrec.inc.c; fishmod/docs/input-replay.md). | this game |

## Sweeps and shared work

Run hundreds of emulator runs across machines and judge each one.

| tool | what it does | travels to |
|---|---|---|
| `fishmod/sweep/worker.py` | The sweep worker: any Linux machine (your PC under WSL, a Colab session, a server) takes runs of a sweep from the queue in the repository, plays them in ares, and pushes the results. | any N64 project |
| `fishmod/sweep/worker.sh` | The sweep worker on a Linux machine (Ubuntu or Debian: a PC's WSL, a Colab session, a server). | any N64 project |
| `fishmod/sweep/colab.ipynb` | Notebook that runs the sweep worker in a Colab session. | any N64 project |
| `fishmod/tools/audio_sweep_report.py` | Reads one part of the audio sweep as fishmod/tools/audio_sweep_run.sh saved it (fishmod/docs/audio-sweep.md) and writes what the sweep's checks need: OUT/aud.bin the sound log (one record an output buffer: audlog_compare.py compares two ... | this game |
| `fishmod/tools/audio_sweep.sh` | The audio sweep's scenes leg for one build (fishmod/docs/audio-sweep.md): plays every variant of a sweep_scenes ROM in ares, four at a time, and writes each one's report. | this game |
| `fishmod/tools/audio_sweep_run.sh` | Plays one part of the audio sweep (fishmod/docs/audio-sweep.md; a sweep_* script's ROM) in ares to its end and saves what the sweep's checks read: OUT/ram.bin        the console's memory at the end (the sound log, the frame count, every ... | this game |
| `fishmod/tools/audio_sweep_check.sh` | The audio sweep's verdict between two builds (fishmod/tools/audio_sweep.sh's OUT folders): for every run both have, the same sound (audlog_compare.py), both still running at the end, and the cost leg by leg, the legs where the new build is slower (the audio, or the rest of the game) named. | this game |
| `fishmod/tools/audio_ref_tree.sh` | Turns the working tree into the audio sweep's reference (fishmod/docs/audio-sweep.md) and back: the mod's audio as it was before the audio work (commit REF: the driver, its headers, the scheduler, the link order and flags), with only what ... | this game |
| `fishmod/tools/anatomy_sweep_report.py` | The anatomy sweep's report (fishmod/scripts/anatomy_sweep.inc.c): per leg, what each part of the frame left out saves, against the shipped laps on either side of its lap: the RDP's frame time, its drawing (pipeline busy), its texture loading (TMEM busy), the hitches; then the whole route. | this game |
| `fishmod/tools/prop_sweep_report.py` | The prop sweep's report (fishmod/scripts/prop_sweep.inc.c): per test (a plant kind left out, drawn with its whole cards up close, or with its cut split low), what it saves against the shipped laps on either side of its laps: the RDP's ... | this game |
| `fishmod/tools/rdp_sweep_report.py` | The RDP sweep's report (fishmod/scripts/rdp_sweep.inc.c): per leg, what each thing left out saves, against the shipped laps on either side of its lap (the night's drift cancelled): the RDP's frame time, its drawing (pipeline busy), its texture loading (TMEM busy), the hitches; then the whole route. | this game |
| `fishmod/pc/` | Job packs for a Windows PC: run several emulator jobs at once from a zip and send results back. | this game |

## Code layout for the instruction cache

Order functions so fewer cache lines are refilled. See `fishmod/tools/layout/README.md`.

| tool | what it does | travels to |
|---|---|---|
| `fishmod/tools/layout/icache_layout.c` | Chooses the order the audio driver's functions are linked in from records of what the audio thread executed (ares, ARES_ITRACE): the console's instruction cache is 16 KB, direct-mapped, 32-byte lines, so which functions share lines decides how often code is fetched from memory again. | any N64 project |
| `fishmod/tools/layout/layout_funcs.py` | Lists a build's functions for icache_layout: hex address, size, 1 if the linker can place it anywhere in the audio driver's block (the driver's own, src/audio/, each in its own section; and the scheduler's functions that are linked there, fishmod/data/sched_text_slots.txt), name. | any N64 project |
| `fishmod/tools/layout/check_outside.py` | Checks that nothing outside the audio driver has moved in the CPU's caches between two builds: every symbol of the code segment that both have, outside the driver (src/audio/ and the scheduler functions linked with it), must be at the same ... | any N64 project |
| `fishmod/tools/layout/audio_pin_from_ref.py` | Writes fishmod/data/audio_pin.txt's phases anew from a reference build (the tree without the audio work: fishmod/tools/audio_ref_tree.sh) and a build of the current tree with the file as it is: for each section, how far what follows the ... | this game |
| `fishmod/tools/audio_text_order.py` | Writes fishmod/data/audio_text_order.txt (the order tools/mkspecrules.c links the audio driver's functions in) from the audio sweep's CPU profiles: every function under src/audio/ that ran, the most-run first. | this game |
| `tools/mkspecrules.c` | The decomp's linker-rule generator, extended (126 lines) to link chosen files function by function in an order read from a list, with fixed cache phases. | this game |

## ROM, patch and build utilities

| tool | what it does | travels to |
|---|---|---|
| `fishmod/tools/bps.py` | BPS patches (the format Flips makes), with no outside tool: create one from a source and a target, or apply one. | any project |
| `fishmod/tools/to_z64.py` | Converts an N64 ROM in any byte order (.z64, .v64, .n64) to big-endian .z64. | any N64 project |
| `fishmod/tools/rspasm.py` | A small assembler and disassembler for the N64's RSP (its MIPS subset and the vector unit), for the audio microcode: `dis` turns a microcode's text into source this assembles back to the same bytes; `asm` assembles a source (the game's own, taken apart and changed) into text for the build. | any N64 project |
| `fishmod/tools/build_profile.py` | Build profiles (fishmod/data/profiles.json): the settings for the kind of machine a ROM is made for. | this game |
| `fishmod/tools/sram_map.py` | The SRAM map of fishmod's debug and bench builds, read from include/fishmod_sram.h (the one source). | this game |
| `tools/reloc_elf_pairing.py` | Marks an overlay's relocation table (fado's .reloc.s) for standard ELF HI16/LO16 pairing: puts the RELOC_ELF_PAIRING entry (include/libu64/overlay.h) first and counts it. | this game |
| `tools/fishmod_gcc_cc.sh` | GCC for the game's own code in an otherwise IDO build (GCC_CODE=1, see the Makefile). | this game |
| `tools/fishmod_clang_cc.sh` | Clang (LLVM) for the mod's own code (CLANG_MOD=1, see the Makefile; fishmod/build.sh --clang), the rest staying as it was (the game's code on GCC, boot, libultra, libc and audio on IDO). | this game |
| `fishmod/build.sh` | Builds the modded ROM and creates fish-oot.bps against the original NTSC 1.2 (US) ROM in baseroms/ntsc-1.2/. | this game |
| `vanilla/` | The unmodified game with only a timing bench added, to compare frame times with the mod on the same route. | this game |
| `fixle.sh` | Fixes line endings of a file (from the decomp's helper scripts). | any project |

## Build-time passes over the game's own data

Lossless rewrites of extracted assets: same picture, same surfaces, less work for the console. Tied to this game's formats, but the methods carry to other N64 games.

| tool | what it does | travels to |
|---|---|---|
| `tools/texpack.py` | A lossless texture pass over the extracted assets, for the RDP (build.sh --texpack). | this game |
| `tools/colpack.py` | A collision pass over the extracted scenes (build.sh --colpack): fewer triangles, the same surfaces. | this game |
| `tools/meshpack.py` | A mesh pass over the extracted scenes (build.sh --meshpack): the RSP's culling made finer, the same picture. | this game |

## Bench and budget readers

Read results the mod's bench builds write to a save or over USB, and estimate console frame time from them.

| tool | what it does | travels to |
|---|---|---|
| `fishmod/tools/hwbench_report.py` | Reads the hardware bench's results (fishmod/scripts/hwbench.inc.c, SRAM 0x0000 of the save file) and prints a report: per tour stop and mode, the frame rate, where the frame's time goes (CPU, RSP, RDP), the RDP's own counters (command ... | this game |
| `fishmod/tools/area_report.py` | Reads the area bench (fishmod/scripts/hwbench_area.inc.c) from a save: at each spot, the frame time and every step of the game's update, as shipped and with the mod's fish, props, water surface and sky off, in the iron boots and in his own; and the costliest kinds of actor. | this game |
| `fishmod/tools/playlog_report.py` | Reads the play log (fishmod/scripts/playlog.inc.c) from a save: frame times at Lake Hylia per situation, and the slowest seconds with where Link and the camera were and what was going on. | this game |
| `fishmod/tools/read_stats.py` | Reads the timing stats a debug build writes to the end of SRAM (see Fishmod_DebugTiming) from an emulator save file (.ram/.sra/.sav, 32 KB, big-endian) and prints whole-run averages and worst frames in milliseconds. | this game |
| `fishmod/tools/stress_prof.py` | Reads the profiled stress bench (fishmod/scripts/stress_prof.inc.c): for each bench (fish, then plants) and step, the count of fish or plants, the frame rate, where the frame's time went (CPU update and draw, RSP, RDP, the fish's own ... | this game |
| `fishmod/tools/stress_report.py` | Reads a stress bench's save (fishmod/scripts/stress_fish.inc.c, stress_plants.inc.c; fishmod_debug_stress.inc.c): per step, the count of fish or plants and the frame rate, CPU, RSP and RDP times, averaged over every pass, and the largest count that held 20 fps. | this game |
| `fishmod/tools/lod_calibrate.py` | Reads the LOD calibration's results (fishmod/scripts/lod_calibrate.inc.c, written to SRAM at 0x7A40) from an emulator save file and writes include/fishmod_lod.h: how far each habitat prop kind is drawn, above water and under it. | this game |
| `fishmod/tools/lod_tune.py` | Reads the LOD tuning's saves (fishmod/scripts/lod_tune.inc.c, one per part: SRAM 0x0000) and merges them: per plant kind, time of day and above or under water, the shortest distance that changed no view, for the cut (how far it's drawn), the single card and the half-size texture. | this game |
| `fishmod/tools/viewbench.py` | Reads the view bench's frame log (fishmod/scripts/pcprof_views.inc.c): at each tour stop, the frozen scene drawn with each setting in turn. | this game |
| `fishmod/tools/rdp_estimate.py` | Estimated console frame time per tour stop for a placement, scaled from hardware bench 4's measurements. | this game |
| `fishmod/tools/section_budget.py` | Estimated console frame time across each Lake Hylia section (the zones of fishmod/data/lake_hylia_zones.json), from cameras like the game's own, not picked ones: fish: Link on the shore or a dock near the section, the rod's camera (75 ... | this game |
| `fishmod/tools/shot_search.py` | Console frame-time estimate for any camera at Lake Hylia, and a search for under-water shots that fit a budget. | this game |
| `fishmod/tools/scatter_budget.py` | How many props each tour stop draws, from a placement (gen_scatter.py's), the way fishmod_scatter.c picks them: in the view cone and within each kind's distance (above water the set distances, under water the tuned ones). | this game |
| `fishmod/tools/overdraw.py` | How much the habitat props fill the screen from a camera pose: the N64's RDP pays for every pixel of every triangle drawn (see-through texels included), so the props' overdraw is the sum of their triangles' screen areas. | this game |
| `fishmod/tools/thin_beds.py` | Thin the plant beds only where the console needs it: every sampled spot (section_budget.py's game-like cameras, looked at in eight directions) must stay under the frame-time limit by shot_search.py's model. | this game |

## Content generators

Make game data at build time. Their output ships; the tools don't.

| tool | what it does | travels to |
|---|---|---|
| `fishmod/tools/assets.py` | Turns the mod's source art into C the build compiles, from one manifest: fishmod/assets/assets.json. | this game |
| `fishmod/tools/gen_species.py` | Generates game data and wiki pages from fishmod/data/species.json. | this game |
| `fishmod/tools/gen_textures.py` | Generates per-species fish textures from the Fishing Pond textures and the "textures" section of each species' "look" in fishmod/data/species.json. | this game |
| `fishmod/tools/gen_habitat.py` | Generates the habitat props for Lake Hylia's sub-biomes: textures (PNG) and small meshes (OBJ with vertex colours and UVs), in Ocarina of Time's style. | this game |
| `fishmod/tools/gen_scatter.py` | Places habitat props across a scene by rules (fishmod/docs/v5-plan.md, phase 3), at build time. | this game |
| `fishmod/tools/gen_collision.py` | Collision for the solid habitat props (rocks): baked at build time, added to the scene's own collision when it loads (fishmod.c), so it costs nothing per frame beyond the lookups BgCheck already does. | this game |
| `fishmod/tools/gen_watermap.py` | Bakes the water floor map of each fishing scene from its collision, at build time. | this game |
| `fishmod/tools/gen_occluders.py` | Occluder candidates for F3DEX3's occlusion plane, from each fishing scene's collision (at build time). | this game |
| `fishmod/tools/gen_bedshade.py` | Shading for Lake Hylia's bed under the full lake: darker under dense weed beds and lily pads (their shade) and deeper down, from the props' placement (gen_scatter.py) and the depth. | this game |
| `fishmod/tools/gen_lod_views.py` | Viewpoints for the LOD sweep (fishmod/scripts/lod_sweep.inc.c): where the camera can be around Lake Hylia, on a grid over the water and its shores, each looked from in several directions. | this game |
| `fishmod/tools/gen_fight_seq.py` | Writes assets/audio/sequences/seq_110.seq: the fish-on music (NA_BGM_FISHMOD_FIGHT). | this game |
| `fishmod/tools/gen_bait_icons.py` | Generates the bait bottle icons (32x32 RGBA32, like the vanilla bottle icons) from the vanilla empty bottle: bee larvae suspended in honey, as in Twilight Princess, with the honey at 3/3, 2/3 and 1/3 for the three uses left. | this game |
| `fishmod/tools/gen_item_page.py` | The item page's new graphics, made at build time from the game's own (so nothing ROM-derived is committed). | this game |
| `fishmod/tools/gen_clock_tex.py` | The HUD clock's textures (our own art): the day arc (midnight at its ends, noon at its top, the sky's colour along it: night blue, dawn rose, gold noon, dusk orange) and the sun and moon that ride it. | this game |
| `fishmod/tools/gen_dpad_tex.py` | The HUD's D-pad texture (our own art): a cross shaded as the C buttons are, a flat face with a bevelled rim lit from the upper left, a dished middle and small arrows pressed into its arms (under the items). | this game |
| `fishmod/tools/bedcard.py` | Bed cards: each scatter chunk's distant plants as one card (at build time). | this game |
| `fishmod/tools/card_outline.py` | Outlines for the plant cards: for each cut-out texture, a polygon of a few vertices that holds every texel that can show (the opaque ones and a texel round them, which the N64's filtering can blend into the edge), and as little else as it can. | this game |
| `fishmod/tools/impostor.py` | The far plants' single card: a picture of the whole plant from the side, rendered from its own mesh, texture and vertex colours (as the game shades it: texel times vertex colour), for one card the game turns to face the camera (fishmod_scatter.c, the billboard tier). | this game |
| `fishmod/tools/rock_mesh.py` | The lake's rocks, each made for where it lies (fishmod/tools/rock_designs.py's E design, "made for its spot"): the same placement as before (gen_scatter.py rocks_reworked), but each rock its own mesh, in the world, sized by what it is. | this game |
| `fishmod/tools/prop_kinds.py` | The habitat prop kinds (fishmod/data/prop_kinds.json): the one table the habitat tools and the game share. | this game |
| `fishmod/tools/gen_noise_tex.py` | Soft grey noise that tiles perfectly (made in frequency space, so it wraps by construction). | any project |
| `fishmod/tools/gen_canopy_tex.py` | Turn a photo of ground cover into a tiling I4 canopy texture (128x64, 16 greys, 4 KB). | any project |

## Previews, design studies and captures

Look at art and layouts before they go in, and capture the running game.

| tool | what it does | travels to |
|---|---|---|
| `fishmod/tools/mesh_preview.py` | A software preview of prop meshes at the N64's resolution (320x240): textured, vertex-coloured, depth-tested, back faces culled where asked, the scene's fog, a water surface and a lake bed for context, from any camera. | any N64 project |
| `fishmod/tools/model_preview.py` | Offline preview of an extracted skeleton model (bind pose, no animation). | this game |
| `fishmod/tools/style_sheet.py` | Renders the habitat props (fishmod/tools/gen_habitat.py) next to vanilla art, for review before they go in the game: a style sheet. | this game |
| `fishmod/tools/prop_designs.py` | The plant cards drawn three ways, for comparing by look and by the RDP's cost (fishmod/tools/mesh_preview.py's rasterizer and cost model): each kind as a small bed of nine under the water, near and far. | this game |
| `fishmod/tools/rock_designs.py` | The lake's rocks drawn four ways, on the lake bed itself (its collision floors), for comparing by look and by the RDP's cost (fishmod/tools/mesh_preview.py's rasterizer and cost model): A now: vanilla's Fishing Pond rock (16 triangles), ... | this game |
| `fishmod/tools/dock_designs.py` | Designs for the north-beach dock side by side (fishmod/tools/mesh_preview.py): each from the beach, from the side on the water and from under it, with what it would cost the RDP. | this game |
| `fishmod/tools/shape_study.py` | Which low-poly shapes give the most roundness for the RDP's time: a post (or a ball) built each way, drawn from many angles with fishmod/tools/mesh_preview.py's rasterizer, back faces culled, and costed with the console-calibrated model (a ... | this game |
| `fishmod/tools/item_grid_mockups.py` | Mockups of the reorganised item page (rod, lures, more bottles) drawn over real pause-menu captures. | this game |
| `fishmod/tools/scatter_map.py` | Draws the scene's placed habitat props (src/fishmod_data/fishmod_sc_<scene>.c, made by gen_scatter.py) from above over the depth map, one colour per kind, for review. | this game |
| `fishmod/tools/zone_map.py` | Draws a scene's sub-biome zones (fishmod/data/<scene>_zones.json) over its depth map, for review. | this game |
| `fishmod/tools/water_map.py` | Maps fishable water in a scene from the extracted collision data. | this game |
| `fishmod/tools/skel_survey.py` | Every skeleton in the game and how much of it ever moves: for each object's skeletons, its limbs (with geometry or without: a limb with no display list is a joint only, still computed every frame), and, over every animation of the same ... | this game |
| `fishmod/tools/anim_capture.py` | Records the animation studio (fishmod/build.sh --script anim) to video clips: one per species and view (from above, three-quarter), as MP4 and GIF, at the game's 20 frames per second. | this game |
| `fishmod/tools/photo_capture.py` | Captures the wiki renders from the photo booth build (fishmod/build.sh --script photo). | this game |
| `fishmod/tools/tour_capture.py` | Screenshots every stop of the camera tour (fishmod/build.sh --script tour_auto): runs the ROM in ares on a virtual display and saves one shot per stop once it has settled, read from the marker in the top-left corner (red = 8 * stop number, ... | this game |
| `fishmod/tools/edge_ab_capture.py` | Exact A/B screenshots of the props' two edge modes (fishmod/build.sh --script edge_ab): runs the ROM in ares on a virtual display, with ares saving its raw output frames itself (the console's VI output, before any scaling or colour ... | this game |
| `fishmod/tools/pop_ab_capture.py` | Pop-in, seen and scored (fishmod/build.sh --script pop_ab): runs the ROM in ares, and at every tour stop keeps one raw frame of each of the pop-in test's variants (fishmod_debug.c, FISHMOD_POP_AB): the frozen view as the game draws it, the ... | this game |
