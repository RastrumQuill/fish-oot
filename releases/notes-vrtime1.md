# vrtime1: the VR4300 timing tests (console)

Timing tests for the N64's CPU, for LLVM's N64 cost model (`n64prof/vrtime/`). Built from the commit after
`bcfcabe` with `fishmod/build.sh --script vrtime --hw`.

## The patch
- **`fish-oot-vrtime1-hw.bps`**: console. **Blank save**: the results go over the save files.

## Running it
About a second after the game starts it runs 59 tests. The picture freezes for a few seconds while they run (each
test has interrupts off and waits for the graphics chips to go idle, so nothing else touches memory). Then
`vrtime: 59 tests done and saved` appears at the bottom. Turn it off and send the `.sav`. One run is enough;
two or three let us check they agree.

## Reading it
```
n64prof/tools/vrtime.py SAVE                     # cycles per operation, and cache-miss costs
n64prof/tools/vrtime.py SAVE --vs ARES_SAVE      # beside ares's (n64prof/vrtime/results-ares.md)
```
