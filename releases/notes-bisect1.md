# bisect1: why the frames1 ROMs stop on the console

Both frames1 ROMs (the mod and vanilla) send exactly two frame-log packets, then crash or freeze before the third,
on every run. In ares neither does, even with the real USB code talking to an emulated SummerCart64
(fishmod/tools/ares-sc64.patch): the cause is something only the console or the cart does. These three ROMs each
remove one piece, to find which one:

| patch | what's removed |
|---|---|
| `fish-oot-bisect1-noring.bps` | the profile's per-frame tags (samples counted as in pcprof3, which ran for hours) |
| `fish-oot-bisect1-nosend.bps` | sending the frame log (the records are still made; the profile is still sent) |
| `fish-oot-bisect1-fake.bps` | the USB entirely (nothing talks to the SummerCart64) |

Each shows **`test: frame N, M packets`** at the bottom of the screen, updated every 30 frames. Run each (with
`sc64deployer debug` running, as before) for about a minute: the frames1 ROMs stopped near frame 80-120. For each,
report whether it gets past frame 300, or the frame number where it stops. Sending the packets is optional.

Built with `fishmod/build.sh --script pcprof_frames_{noring,nosend,fake} --hw`.
