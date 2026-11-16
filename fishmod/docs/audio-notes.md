# Audio engine: what's been measured, tests to run, improvements to try

The audio is the third-largest cost in a slow frame after the RDP and the CPU, and the one least worked on. These
notes collect what the console runs have shown so far, the tests that would show more, and the changes worth trying.

## What the console has shown (morning-results branch)

| run (20261113-0722 unless noted) | visible hitches | RSP audio, slow frames | note |
|---|---|---|---|
| rsp-ab, laps with the audio's RSP work on (1, 3) | 40%, 45% of frames over 52 ms | 7.3 ms (whole run) | 4 laps, one power-on |
| rsp-ab, laps with it off (2, 4) | 28%, 29% of frames over 52 ms | 0 | the audio's RSP work costs ~14 points |
| frames13 (sound on, the baseline) | 33% | 12.9 ms | |
| frames13-27k (mixed at 26.8 kHz) | 25% | 10.9 ms | the only audio test that clearly helped |
| frames13-reverb2 (every reverb at half rate or less) | 39% | 13.9 ms | no gain (one lap: noisy) |
| frames13-audio2 (mixed every two refreshes) | 36% | 13.4 ms | no gain; half the tasks, the same work |
| frames24 (20261115-1043) | 28% | 12.1 ms | today's build |

- **The RSP's audio work is worth about 14 points of slow frames.** It shares the RSP with the graphics microcode, so
  every millisecond of it delays the RDP's work for the frame.
- **The mixing rate matters**: 26.8 kHz instead of 32 kHz cut the RSP audio time by 2 ms and the hitches by 8 points
  (one lap each, so it needs repeating).
- **Halving the reverbs or the task count didn't help**: the cost is in the per-note work (decoding, resampling,
  envelopes), not in how often the tasks start.
- In ares, 10-20 notes play at once at the lake, every one sending to a reverb, none silent.
- The audio driver's CPU work is already bit-exact and packed (frames12-13: note and channel fields together, slots
  claimed without a read); ares' RSP audio time is too unsteady (8.6-14.7 ms within one run) to judge RSP changes:
  only the console can.

## Tests to build

1. **Where the RSP audio time goes, by command** (ares, exact): dump each audio task's command list (the `aspMain`
   ABI list) at a few lake frames and count, per note, its ADPCM decodes, resamples, envelope mixes, filters and
   reverb sends; weight them by the commands' known RSP cost. Tells which of decode, resample, mix and reverb to cut.
2. **The notes playing, by source** (ares): each playing note's sequence or sound effect, its volume after the
   envelope and the distance attenuation, its reverb send. Shows how many notes are inaudible (near-zero volume)
   but still mixed, and which sources (rain, wind, water, river sound, music) cost the most.
3. **rsp-ab again on today's build**: the audio's RSP work on and off, 4 laps, one power-on, to get the current cost
   (the 14 points were frames13's).
4. **The rate sweep** (console, one power-on): 32, 29, 26.8 and 22 kHz in turn, a lap each, twice round. The cost
   against the rate, and a recording at each rate (capture.py) to listen to.
5. **A voice cap sweep**: the most notes mixed at once at 24, 16, 12 and 8 (the quietest dropped first), each lap,
   with a recording. How few notes sound the same.
6. **Listening checks for anything not bit-exact**: capture.py records the console's sound at the same bench points
   for both builds; compare by ear and by spectrum (the same places on the route, the same time of day).
7. **Bit-exact checks for everything else**: the audlog script and audlog_compare.py (every output buffer's hash
   alike over the route), as for frames12-13's driver changes.

## Improvements to try (cheapest first)

- **Skip inaudible notes**: a note whose volume after the envelope and distance is below what the 16-bit output can
  show is still decoded, resampled and mixed. Dropping them changes no audible sample (and few bits): test with 2 and 6.
- **Mix at a lower rate**: 26.8 kHz looked like the clearest gain; settle the rate by ear with test 4.
- **Cap the voices**: drop the quietest notes past a cap (test 5); the lake's ambient layers (rain, wind, water,
  the river sound) likely overlap.
- **Reverb sends only for audible notes**, and the lake's two reverbs fed from fewer notes (reverb2's half rate
  didn't help, but the number of notes sent may).
- **Mono for ambient sounds** that have no stereo position, mixed once instead of twice.
- **The command list's RAM traffic** (task 21): the audio task reads its command list and the samples from RDRAM
  while the RDP is drawing; fewer, larger DMAs and no repeated loads of the same sample cut the contention with
  the RDP's own memory traffic.
- **A cheaper resampler for low notes and effects** (the microcode's own lower-quality path) where nobody would hear
  the difference: by listening test.

Each change that alters the sound gets a recording against the build before it; each that shouldn't gets the
audlog check.

## What the exact profiles show (16 November, ares; the console's laps in releases/notes-audio-ab.md)

Two profilers in ares, both exact and the same from run to run: `rspprof.py` (the RSP's instructions by audio
command) and `cpuprof.py` (the CPU's instructions, cycles, data cache fills and write-backs, and instruction cache
fills, by function). Measure the CPU on a bench build without the PC sampler (`--script hwbench_play`): the sampler's
interrupt, ten times an audio update, evicts the code being measured.

**On the console the audio is 32 points of slow frames at the lake** (45% with it, 13% without): 14 the RSP's
tasks, 18 the driver's CPU work (12 to 13 ms a game frame: about half the sequences and notes, a bit under half the
command lists).

**The audio thread's time is cache misses, not instructions.** An update (three ticks, about 15 notes) in ares:

| | per update | share of its 229,000 cycles |
|---|---|---|
| instructions | 71,100 | 31% |
| data cache fills | 1,550 (40 cycles each) | 27% |
| data cache write-backs | 830 | 14% |
| instruction cache fills | 770 (48 cycles each) | 16% |
| the rest (multiplies, divides, floating point) | | 12% |

- The data a tick touches doesn't fit the 8 KB data cache: about 820 lines (13 KB) take nine tenths of the fills,
  the notes most (350 fills an update: an active note is read across 13 of its 14 lines, three ticks an update, and
  again for the command lists), then the channels (233), the driver's globals (130), the layers (106), the notes'
  per-tick states (102) and the audio thread's own stack (122).
- The code an update runs is 14 KB of hot instructions inside 37 KB of functions; each update starts with it cold
  (the game ran in between).

**Tried, each with the same sound (the audio check), and what it gave:**

| change | audio thread's cycles an update |
|---|---|
| A lean copy of the note's command builder for the common case, the game's own for the rest (15% fewer instructions in that stage, checked command for command against the game's: 59,916 notes, none different) | no change: the same misses. Not kept |
| The driver on clang with a sample profile and hot/cold splitting | -1.4% (it calls 44 KB hot) |
| The driver built for size (`GCC_AUDIO_OPT=Os`) | +4%: more instructions, the same fills |
| The hot functions placed together in the order an update runs them (`fishmod/data/audio_text_order.txt`) | -0.9% (-4.5% under the PC sampler). Kept |
| An update every two refreshes (`FISHMOD_AUDIO_EVERY2`, six ticks) | -2.8% a tick: the code is filled less, the data as much |

So no rearrangement of the driver as it stands is worth more than a few percent: the cost is the amount of data
and code each tick walks, 180 ticks a second. What's left that can keep every sample:

- **Fewer, larger commands to the RSP** (a note's decode, resample, filter and envelope as one command, its state in
  one block): the command lists are 4.2 KB an update (524 commands) that the CPU writes line by line and the RSP
  reads back; the RSP's own share of dispatch, moves, codebook reloads and load waits is 15 to 20% of its audio work.
  Needs a second audio microcode (the game's fills all but 80 bytes of the RSP's instruction memory).
- **Less data a tick**: the script state and the I/O ports of a channel in one line, the notes' unused bytes out,
  the per-tick note states smaller. A few percent each.

## The measure for a change to the driver, and where it stands (16 November)

The bench builds don't run the same audio twice (the game's frames fall differently against the refreshes: 4% more or
fewer instructions from one run to the next), so a change of a percent can't be read from them. The audio check's
build does (lockstep: three updates a game frame, the same notes every run): **`cpuprof.py` on the audlog build, the
audio thread's cycles an update**, is exact to a few tenths of a percent, and the same build proves the sound.

    fishmod/build.sh --script audlog --hw --texpack --colpack --meshpack --gcc-all
    ARES_CPUPROF=cpu.bin fishmod/tools/ares_run.sh 0 ROM 150
    fishmod/tools/cpuprof.py cpu.bin --elf ELF --per-calls AudioThread_UpdateImpl \
        --match '^(Audio|func_800D|func_800E|Fishmod_Sample)' --top 0

| the driver | instructions | cycles an update | data fills | write-backs | code fills |
|---|---|---|---|---|---|
| before any of this (frames25's) | 68,515 | 217,288 | 1,496 | 828 | 636 |
| the hot functions together; the note's fields in a tick's order | 68,695 | 215,147 (-1.0%) | 1,452 | 826 | 623 |

(The function sections cost the command-line claim its inlining, 900 instructions an update: forced back.)

**What the exact changes left are worth, from the bytes each tick reads** (`cpuprof`'s `.bytes`): the channel's script
state and I/O ports beside its head, about 1%; the note's per-tick state reused when its inputs haven't changed, about
2%; the driver's scattered globals and its stack, 1 to 2%; one compact record a note to the RSP instead of 13 commands
(a second microcode that expands it), about 4% of the CPU's and 3% of the RSP's audio time. All of it together is
near a tenth of the driver's 12 ms a frame. The rest is the work itself: about 15 notes and 14 channels walked 180
times a second, through an 8 KB data cache that the walk doesn't fit in, with the code cold at every refresh.

## The memory bus (16 November, ares: `fishmod/tools/busprof.py`)

`ARES_BUSPROF` counts every byte moved to or from RDRAM by who asked. The play bench at the lake, per game frame
(50 ms). (The RDP's drawing isn't in it: ares draws on the GPU. The video interface's read-out isn't either.)

| KB a game frame | read | written |
|---|---|---|
| CPU: the audio driver's code filled | 56.5 | |
| CPU: the audio driver's data filled / written back | 67.7 | 37.2 |
| RSP: the audio tasks' DMAs | 79.2 | 31.2 |
| **the audio's, together: 272 KB** | | |
| CPU: the rest's code | 459 | |
| CPU: the rest's data | 259 | 157 |
| RSP: the graphics tasks' DMAs | 438 | 401 |
| RDP: its commands fetched | 248 | |

The audio is about an eighth of what's counted. The game's own code is the largest single reader (459 KB a frame of
instruction cache fills: the 16 KB cache, 20 times a second), then the graphics microcode's loads and its output to
the RDP.

**The audio's RSP DMAs, by what they carry** (1,543 a game frame, 112 KB):

| | DMAs | KB |
|---|---|---|
| the reverbs' ring buffers, in and out | 72 | 24.4 |
| the command list, 64 bytes (8 commands) at a time | 240 | 14.9 |
| the microcode itself, text and data, each task | 6 | 14.1 |
| the notes' sample data | ~190 | ~14 |
| the notes' states (32 bytes each, in and out) | 438 | 13.7 |
| the ADPCM codebooks | 111 | 8.8 |
| the finished sound out | 9 | 6.3 |
| the decoder's last frame in, and other small states | ~480 | ~16 |

What can come off it and keep every sample: the command list (a compact record a note: about 10 of its 15 KB, and
its 240 DMAs to about 20 with larger pieces); the microcode's own load and the driver's cold code, by waking the
driver less often (every second refresh: -11% of the audio's traffic, measured; releases/notes-audio-laps.md); the
codebooks, if DMEM had room to keep them (it doesn't, as laid out). The reverbs, the samples, the states and the
sound itself are the work.
