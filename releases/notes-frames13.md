# frames13

frames12 with three changes, for the CPU:

- **The wall clearance bound off** (frames11's, kept in frames12). The bisection of 12 November found frames11's CPU
  slowdown in it: with it alone left out (nowall), the CPU's update was 1.3 ms less on full-speed frames and the
  visible hitches 19% instead of 28-31%. The other two suspects did nothing to the CPU: noclaim the same as frames11;
  oldsky the same CPU and 1.2 ms more RDP (the sky changes stay). The collision answers are the same either way
  (the collision logs, frames11).
- **Audio: a playing note's layer and channel fields packed together.** What every audio update reads of each
  playing note's layer (its note, channel, frequency and velocity) in two cache lines instead of four, and of its
  channel (gain, filters, stereo, reverb) in three instead of six. Audio check: every sample the same over 21,559
  buffers.
- **Audio: each update's note state written without reading it first.** Its slot (written whole every update, and
  long out of the data cache by then) is claimed with the cache's Create Dirty Exclusive instead of read from RDRAM
  first: two misses a playing note an update were the costliest instructions of `Audio_InitSampleState`. Audio check:
  every sample the same. Untried on the console until this run: a crash, or a broken frame log, would be this.

**Rebuilt 13 November with the sound on.** Every bench script skips the title screen, and the title screen is what
turns on the audio's RSP tasks: every bench so far (frames7 to the first frames13) mixed its sound on the CPU but never
ran the mixing on the RSP. The output was silence, and the frame logs' RSP audio time 0 on every frame. The CPU times
were whole; the RSP and RDP times were missing the audio task, which pauses the graphics task while it runs. This
build turns the audio on at boot (`title_setup.c`): the first true baseline for the RSP and RDP, with sound.

First run (silent, 12 November `20261112-2106`): visible hitches 10% (frames12 15%), CPU update 19.7 ms (21.1), draw
21.2 ms (21.9); the cache instruction fine on the console.

Compare with frames12 (12 November, `20261112-2034/01-frames12`): the CPU's update and draw, the visible hitches.
