# Sample profiles

LLVM text sample profiles (`-fprofile-sample-use`) made with `n64prof/tools/n64prof.py llvm` from console runs.

- **`oot-console2.prof`**: fish-oot's pcprof2 run on the console (14.7 hours of the Lake Hylia bench route: walking,
  swimming, diving, fishing, every time of day and weather), read with the ELF of the build that ran (commit `7d0542b`,
  `fishmod/build.sh --script pcprof --hw`). 554 functions. 44% of its samples fell in code built without line tables
  (audio, libultra, boot), so those functions have no line counts here; the next profile build should give them `-g`.
  `llvm-profdata show --sample` reads it; `llvm-profdata merge --sample --binary` converts it.

Use: `fishmod/build.sh --pgo` (the mod's code on clang with this profile; `PGO_PROFILE` names another).

Head samples (a function's entry count to LLVM) are estimated from the samples on its first line, the prologue: PC
sampling sees no calls. A profile with 0 head samples makes every function look never called, so LLVM takes every
call in it as cold and stops inlining (in the fish bench, the first profiled build was slower than plain clang).
