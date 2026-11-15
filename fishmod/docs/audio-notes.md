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
