# frames2: frames1 without the crash

frames1 (the mod as shipped against vanilla, every frame logged over the SummerCart64's USB) crashed or froze on the
console after its second packet, in both ROMs, every run, and never in ares. The bisection (notes-bisect1.md): the
build without the profile's per-frame tags ran; the ones with them crashed (bar one). So frames2 is frames1 without
them. Everything else is the same: the route, each leg walked once, eight laps, the frame log.

| patch | what |
|---|---|
| **`fish-oot-frames2-shipped.bps`** | the mod as shipped: `fishmod/build.sh --script pcprof_frames --hw` |
| **`fish-oot-frames2-vanilla.bps`** | the unmodified game with the route, the timing and the USB: `vanilla/build.sh` |

Lost: the CPU profile split into slow and full-speed frames (`--frames slow`). The frame log still says, frame by
frame, where the time goes (each section of the update, actor categories, the costliest actors, draw, RSP, RDP).

## Running each
As frames1: an empty folder per ROM, `sc64deployer debug` running in it, start the ROM (blank save for the mod's).
The mod's shows **`test: frame N, M packets`** at the bottom of the screen. Vanilla's shows **`vbench usb: N packets`**.
At the end it says DONE. About an hour each. Zip each folder and send it.

Checked in ares with an emulated SummerCart64 (fishmod/tools/ares-sc64.patch): both send and the readers read them.
