# frames28 set: the control, audio and collision, the water fog's cheaper variants, all

Five builds of the same tree (frames28's), `fishmod/build.sh --script pcprof_frames --hw --texpack --colpack
--meshpack --gcc-all`, differing only as below. Expansion Pak in.

| patch | what differs |
|---|---|
| `fish-oot-frames28.bps` | the control: frames28 as it is (the bed's water fog in 8 cut bands, 2,101 triangles in the cut pieces, about 220 fog changes) |
| `fish-oot-frames28-audiocol.bps` | the audio lead's work (claude/sleepy-faraday-ve9g2u at 20b126e, merged: the note's fields in the order a tick reads them, the driver's hot code placed together; its per-lap audio timers, a few clock reads an update, in every debug build) and the collision lead's `z_bgcheck.c` (claude/collision-opt, 05a25dc: ours was unchanged since their base, a8db7be) |
| `fish-oot-frames28-nocut.bps` | the bed's 8 bands without cutting: each whole triangle in the band of its middle's depth (`MESHPACK_FOG_NOCUT=1`): 577 triangles, as before the bands, 102 fog changes |
| `fish-oot-frames28-bands4.bps` | the bed cut at 4 depths, not 8 (`MESHPACK_FOG_BANDS=0,30,100,250`): 1,359 triangles, 138 fog changes |
| `fish-oot-frames28-all.bps` | audiocol and nocut together |

What the bands cost before (20261115-1716): frames28 54% hitches, RDP 49.3 ms in slow frames; without the bed's bands
30%, 45.9 ms. nocut has the bands' fog changes without the extra triangles; bands4 half the cuts.
nocut's fog changes at triangle edges (a whole triangle's fog by its middle), not along depth lines: check how it
looks from above the water as well as what it costs.
