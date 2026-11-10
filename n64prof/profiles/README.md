# Sample profiles

LLVM text sample profiles (`-fprofile-sample-use`) made with `n64prof/tools/n64prof.py llvm` from console runs.

- **`oot-console2.prof`**: fish-oot's pcprof2 run on the console (14.7 hours of the Lake Hylia bench route: walking,
  swimming, diving, fishing, every time of day and weather), read with the ELF of the build that ran (commit `7d0542b`,
  `fishmod/build.sh --script pcprof --hw`). 554 functions. 44% of its samples fell in code built without line tables
  (audio, libultra, boot), so those functions have no line counts here; the next profile build should give them `-g`.
  `llvm-profdata show --sample` reads it; `llvm-profdata merge --sample --binary` converts it.

- **`oot-console3.prof`**: fish-oot's pcprof3 run on the console (the same Lake Hylia route, eight laps: 393,577 frames,
  96.3 million samples, 6,176 lost), streamed over a SummerCart64's USB one bucket per instruction, from a build with
  the whole game on clang -Os and line tables everywhere clang builds (commit `4d6ebd5`,
  `fishmod/build.sh --script pcprof --hw --clang-game --clang-opt Os`; ROM MD5 `a7b66762ad323d5ce4d5112e8936bdbd`).
  1,173 functions; 17% of the samples in code with no line (assembly, and instructions clang gives none). About 7% of
  the play went unsampled: libultra's timer race (fixed since: see `src/libultra/os/timerintr.c`) stopped the
  sampler for up to 91 s at a time, at moments that don't follow the route, so the profile is a smaller sample, not a
  skewed one.

Use: `fishmod/build.sh --pgo` (the mod's code on clang with this profile; `PGO_PROFILE` names another; the default is still `oot-console2.prof` until a console comparison picks).

Head samples (a function's entry count to LLVM) are estimated from the samples on its first line, the prologue: PC
sampling sees no calls. A profile with 0 head samples makes every function look never called, so LLVM takes every
call in it as cold and stops inlining (in the fish bench, the first profiled build was slower than plain clang).
