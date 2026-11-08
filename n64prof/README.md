# n64prof

A PC-sampling profiler for N64 programs, for compilers' sample profiles (LLVM's `-fprofile-sample-use`) and for
finding where a program's CPU time goes. It isn't tied to a game: fish-oot (an Ocarina of Time mod) is its first
user, and it fits any libultra or libdragon program with an ELF.

## What it measures

A timer interrupt every few thousand cycles (jittered, so work that repeats with the frame isn't always hit or
always missed) records the PC the CPU was interrupted at. Time the CPU spends waiting (instruction and data cache
misses, RDRAM) lands on the instruction that waited, so the profile shows that cost even though the VR4300 has no
counter for it. Samples are counted per 16 bytes of code; code in an overlay is counted at its link-time address,
so the counts match the ELF.

It doesn't see the RSP or the RDP (graphics and audio microcode, pixel fill), DMA waits as such, or code that runs
with interrupts off. It can't say *why* an instruction was slow: an emulator that models the caches (ares) can count
misses per instruction; run the same program and input on both and compare (`n64prof.py compare`).

## Wiring it into a program

1. Build `n64prof.c` into the program (plain C89, no floating point, no allocation).
2. Give it a table: `n64prof_init(table, 8192, 11719, (u32)_codeSegmentEnd)` (8192 entries of 8 bytes; a sample
   every 11719 count cycles is about 4000 a second; addresses above the last argument are expected to be in
   overlays).
3. Call `n64prof_sample(pc)` from a timer interrupt with the interrupted PC, and set the timer's next interval to
   `n64prof_next_interval()`.
   - **libultra:** the counter interrupt doesn't run the timers: it posts a message, and the VI manager's thread
     runs them later (`__osTimerInterrupt`). So keep the interrupted PC in the exception handler: at `counter:` in
     `exceptasm.s`, store `THREAD_PC(k0)` (the interrupted thread's saved PC) to a global. Then in
     `__osTimerInterrupt` (`timerintr.c`) call a hook for each timer as it fires, before it is set again; the hook
     passes that global to `n64prof_sample` and sets `timer->interval`. Start it with
     `osSetTimer(&timer, interval, interval, NULL, NULL)`. fish-oot's glue: `src/code/fishmod_debug_pcprof.inc.c`
     (reading `__osRunningThread->context.pc` in the hook instead gives the VI manager's own PC: always
     `osRecvMesg`).
   - **libdragon:** a timer callback runs in interrupt context; read the interrupted PC from the saved exception
     frame (`reg_block_t`'s `epc`).
4. If the program loads code at run time, once a frame fill `n64prof_ranges_begin()` with each loaded overlay's
   (RAM start, RAM end, link-time start) and call `n64prof_ranges_end(count)`.
5. Every so often, `n64prof_serialize(buf, size, user)` and store the bytes: save memory (fish-oot: 28 KB of SRAM,
   every 30 seconds, the hottest few thousand buckets), a flashcart's USB link, or anything else.
6. Drive the program with recorded or scripted input, so runs compare.

## Reading it

```
n64prof/tools/n64prof.py report  DUMP --elf program.elf        # where the time goes, by function
n64prof/tools/n64prof.py llvm    DUMP --elf program.elf -o program.prof
n64prof/tools/n64prof.py compare CONSOLE_DUMP ARES_DUMP --elf program.elf
```

`--offset` finds a dump inside a bigger file (a save). Needs `llvm-symbolizer`. Build with line tables (`-g`; GCC's
code doesn't change with it) for the LLVM profile; the function report needs only symbols. The profile is by source
line, so one taken from a GCC build serves an LLVM build of the same source.

## Dump format (big-endian)

A 0x40-byte header (`N64ProfHeader` in `n64prof.h`: magic `N64P`, version, samples taken, dropped, not in this
dump, entries, interval, unmapped, bucket size, dump count, six words for the program), then `entries` pairs of
(bucket address, count), in no order: the hottest buckets that fit.

Read a dump with the ELF of the same build. Line tables (`-g`) can change code slightly (GCC on MIPS: with the
assembler scheduling, it can't fill some delay slots across line markers), so a dump from a build without them
doesn't match an ELF with them.
