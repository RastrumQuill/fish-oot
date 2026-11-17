# The audio sweep: what has to pass before an audio change goes to a console

The lake route and the 17-scene tour were samples of the game. A change that is exact and faster there can still be
wrong, slower, or stop the game somewhere else (the tour's own first run found three such cases, and a crash in a
scene it only passed through). The sweep replaces "some scenes" with a rule for when the checking is finished:

**every instruction of the audio driver and of the audio microcode that the game can reach has been run, in each of
the four sound settings, with the output compared against the reference build, the cost measured against it, and the
game still running at the end.** What hasn't been run is listed by name, and each line on that list is either given
a leg that reaches it or a written reason why the game can't.

Nothing is sent to the console rig until the whole sweep passes in the emulator. The console run is then the same
sweep, in real time.

## The builds compared

| | build | what it is |
|---|---|---|
| R | the mod before the audio work (`--gcc-all`, commit before the layout change) | the reference: its output is the right answer, its cost the one to beat |
| A | now, without `--expak` | layout, repacked notes, pins |
| B | now, with `--expak`, updates built ahead | (lockstep turns building ahead on) |

All three are built from the same tree (`fishmod/tools/audio_ref_tree.sh on`, build, `off` for R), so the game, the
mod and the sweep's own code are the same in each, and R is rebuilt whenever any of those change.

**Nothing outside the driver may move.** The CPU's caches are direct-mapped (16 KB of instructions, 8 KB of data):
code that has only moved collides with other code than before, and can cost more or less for that alone. The first
headphones run showed it: the sound identical in all 106 scenes, but the rest of the game 8% dearer in Zelda's
courtyard, because the scheduler had grown 240 bytes and the code after the driver started elsewhere, and an actor's
code now fell on `sinf`'s lines. So the link keeps everything outside the driver where R has it, modulo the cache
sizes: the scheduler's three functions that grew are linked with the driver and their old room left empty
(`fishmod/data/sched_text_slots.txt`), each section goes on after the driver at R's place
(`fishmod/data/audio_pin.txt`), and the heap starts on a 16 KB boundary. `fishmod/tools/layout/check_outside.py R.elf
NEW.elf` proves it for a build, symbol by symbol; a build that fails it is not swept. What the rest of the game then
costs more or less than in R is the driver's doing (what it leaves in the caches, the scheduler's changes), not luck.

Inside the driver the order of the functions is worked out, not guessed (`fishmod/tools/layout`): ares records what
the audio thread executes (`ARES_ITRACE`), `icache_layout` plays the records through a model of the instruction
cache for a candidate order and searches for the one with the fewest fills, over the pak and no-pak builds together
(a function is padded to its largest size in any build, so one order puts it at the same address in all).
(`tools/mkspecrules` is not rebuilt by the game's Makefile: `make -C tools mkspecrules` after changing it.)

Each is built once per leg group with the same script, run in lockstep (the audio thread stepped by the game, three
updates a frame: the same notes on every run, so two builds can be compared sample for sample and cycle for cycle).

## What is checked on every leg

1. **The same sound**: every output buffer's samples hashed (`audlog`), R against A and B, with `audlog_compare.py`.
   One differing buffer fails the leg.
2. **The same picture**: the graphics log (each frame's display lists hashed), R against A and B. The audio work must
   not change what is drawn.
3. **Still running**: at the end of the leg every thread's state is read from the emulator's memory; a stopped
   thread, or the fault handler having run, fails the leg and names the function. The sweep also records the frame
   count reached, so a hang shows as a short count.
4. **The cost, of everything**: `cpuprof` per leg, in cycles per game frame, split into the audio thread and the
   rest of the game (graph thread, scheduler, interrupts, everything else), with data-cache fills and write-backs
   and instruction-cache fills for each. A leg where the *rest of the game* costs more than in R by more than the
   run-to-run noise (zero in lockstep: any difference is real) is reported even when audio got cheaper there:
   that is the "fixed here, slower there" case, including changes outside audio such as the forest crash fix.
   `busprof` gives the same split for bytes on the memory bus.
5. **The RSP**: `rspprof` per leg, cycles per audio task and DMEM/RDRAM traffic.
6. **Coverage**: `cpuprof` already counts every instruction by address and `rspprof` every microcode instruction. A
   new tool (`audcover.py`) adds the legs' counts together and lists, by function and source line, what in
   `src/audio/internal/*.c`, `src/audio/game/*.c`, the audio manager, the audio part of the scheduler and `aspMain`
   was never executed, per sound setting.

## The legs

Each leg is a script in `fishmod/scripts/sweep_*.inc.c`, run four times: stereo, mono, **headphones**, surround
(`Audio_SetSoundOutputMode` at the start of the leg; headphones is its own path through the note's pan and delay
code and through the microcode's mixer, and surround another). Legs run in parallel in the emulator (four at a
time); the lot is a few hours of machine time, which is free.

| leg | what it plays | what it is there to reach |
|---|---|---|
| **scenes** | every scene in the scene table by its first entrance (about 100), as child and as adult where both exist, day and night, 10 s each with Link running, rolling, swinging, shielding; cutscenes at entrances play out | all 18 audio session settings actually used (reverbs, downsampled reverb, filters, second taps, voice counts), every scene's music and ambience sequence, scene loads and audio heap resets between every pair |
| **sequences** | every sequence in the table (113) started on the music player and left for 20 s, then the fanfare player over it, then two at once with one fading out | every sequence script, every instrument and drum, the script commands and layer paths the scenes' music doesn't use, fades, tempo changes, loads of every font and sample bank (and the sample store filling and evicting) |
| **effects** | every sound effect in the seven banks (about 1,270), at positions sweeping round and away from the camera, in bursts dense enough to run out of voices | every effect's channel script, position to pan/volume/reverb/filter, voice stealing and release, the bank priority code, the highest voice count the driver can reach |
| **ocarina** | the ocarina taken out and played from the controller: all five notes, each bent with the stick both ways, vibrato, R and Z modifiers; every song played correctly (the game recognises it and plays it back); a wrong song; the scarecrow's song recorded and played back; the memory game's playback | the ocarina's own update inside the audio thread (it reads the controller itself, so the script's inputs are also fed to it), free playing, recognition, playback, recording; the music ducking under it |
| **states** | the game's other calls to the audio code, made directly as the game's code makes them (so the leg doesn't depend on reaching each place): battle music in and out, the indoor and underwater filters, pause, fanfares over music, Ganon's tower's music by floor, Saria's and Malon's songs by distance, the river's and waterfall's pitch, every effect-stopping call, every sequence command, every command to the audio thread, loads asked for outright, a reset into each of the 18 session settings with music and effects in each, and at the end the reset button | the base and extra low-pass filters, volume ducking and restore, a second sequence over the first, channels switched by the game, the command handlers, background loads, every session setting whether a scene uses it or not, the reset-time fade |
| **system** | boot through the logo and the title demo, file select, new file, load a file, soft reset, the credits' start | settings and sequences outside play, loads while nothing is in memory yet, reset while sound is playing |
| **loads** | scene changes back and forth across different session settings every 3 s; a fanfare during a scene load; sequences started the frame after a reset | the cases where building ahead must fall back to one update at a time (loads in flight, reset, filters changing) and the reset-time buffer handling |

The order inside a leg is fixed by the build (tables generated from the scene, sequence and effect tables, so a new
scene or effect joins the sweep without anyone remembering to add it).

## What the lockstep sweep can't show, and what covers it

Sending audio tasks to the RSP early (`gFishmodAudioLazy`) and the scheduler changes depend on real timing, which
lockstep removes. For those:

- the same legs run again in real time in the emulator with each of the four switch settings (as now / ahead / lazy /
  both), checked for: still running (3), no AI buffer underrun or overrun (the AI's length register sampled each
  retrace: a dropout is a count, and one is a failure), and every task's command list hashed and matched against the
  set of lists the lockstep run produced for that leg (the lists can arrive at different times; they may not be
  different lists);
- then the console run below.

## The console run (only after everything above passes)

One ROM, the sweep's legs in real time with frame times logged per leg (`pcprof_frames`, no sampler), four laps
(as now / ahead / lazy / both), headphones on two of the legs' laps and stereo on the others, results by leg. The
notes sent with it state: the commit, that the emulator sweep passed on that commit, the legs' coverage figure, and
any line of the driver still unreached and why. A ROM that has not passed is not offered.

## Order of work

1. `audcover.py` and the per-leg cost/threads report (tools; no game change).
2. Sound setting as a script option; the scenes leg from the scene table; run it: this alone re-tests the forest fix
   everywhere and gives the first whole-game cost comparison R / A / B.
3. Sequences and effects legs (tables at build time).
4. Ocarina leg (controller inputs fed to the ocarina's own read).
5. States, system and loads legs.
6. Read the coverage list; add legs or reasons until it is empty.
7. Real-time runs with the four switch settings.
8. The console ROM and its notes.
