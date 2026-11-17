# Code layout tools

These place functions so that fewer instruction-cache lines have to be refilled from RAM. They were written for the
audio driver. Placement tuning is on hold until 1.0, when it is meant to cover the whole game, not only audio.

## Why it matters

The console's instruction cache is 16 KB, direct-mapped, in 32-byte lines. Two pieces of code whose addresses are
equal modulo 16 KB evict each other. In the emulator's model a refill costs 48 cycles; refills were about 31% of the
audio thread's cost and about half of the rest of the game's (roughly 9,800 a frame outside audio).

## Tools

- `icache_layout.c`: searches for a function order with the fewest refills, given instruction traces
  (`ARES_ITRACE`, see `fishmod/tools/ares-prof.patch`). Takes several builds' traces at once and pads each function
  to its largest size across them, so one order serves every build.
- `layout_funcs.py ELF`: lists functions with size and whether they may move. Needs debug info; for a release ELF
  copy the movable flags over by name from a debug build.
- `check_outside.py REF_ELF NEW_ELF`: checks that nothing outside the movable set changed its address modulo 16 KB
  (code) or 8 KB (data).
- `audio_pin_from_ref.py REF_ELF NEW_ELF`: works out the phases for `fishmod/data/audio_pin.txt`.

The results are applied by `tools/mkspecrules.c` from `fishmod/data/audio_text_order.txt`, `audio_pin.txt` and
`sched_text_slots.txt`.

## State of the data files

The order and the pins were worked out before the merge of `claude/harkinian-tp-fishing-refactor-clean`. They still
link and run correctly, but they are no longer tuned: code and data have moved since and will keep moving. Don't
re-derive them before 1.0.

## Notes for the game-wide pass at 1.0

- The movable set is whatever is linked function by function (`-ffunction-sections` plus an order list). Today that
  is `src/audio` and the functions of `src/code/sched.c`. Widening it means adding files to both.
- Traces must come from many scenes and states. What is hot on one map says little about another.
- Actor overlay code is loaded into the heap, so its cache phase depends on where the heap starts and on load order.
  `main.c` rounds the heap start up to 16 KB to keep that phase fixed between builds. A game-wide layout has to
  treat overlay code as fixed neighbours (as now) or give overlays fixed addresses.
- Small moves outside the tuned code change costs elsewhere: a scheduler that grew by 0xF0 bytes moved later code
  onto `sinf` and friends and cost some scenes 3 to 8% outside audio. Compare builds with `check_outside.py` before
  reading cost differences as real.
- Trace every thread that matters, and weight the search by frame cost, not by raw refill count, if the aim is the
  worst frame.
