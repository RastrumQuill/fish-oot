# frames10: the Guays' and Tektites' collision checks, the fish manager's travel

**`fish-oot-frames10.bps`** (`fishmod/build.sh --script pcprof_frames --hw --colpack --gcc-all`): frames9 (see
[its notes](notes-frames9.md)) plus what its console run found:

- **Guays:** circling idle, more than 60 units over the ground, away from walls and more than 300 units from Link
  (they only dive within 300), a Guay checks its collision one frame in four. frames9 measured 2.1 ms a frame of
  Guay collision (0.69 ms a check, 3 of them over the lake). Diving, hit, near the ground or a wall: every frame.
- **Tektites:** the one-frame-in-four logic of an idle Tektite now also from 400 units while it's off screen (it only
  reacts within 300); before, only from 1,000. Its hurtbox stays up every frame.
- **The fish manager:** a fish away from Link travels by arithmetic only; the water is looked at once, when it's
  about to appear (frames9 looked every pass: 0.6 ms a frame).
- (The stress bench's pass sums no longer carry over from the fish bench to the plants.)

Expected: about 2-3 ms a frame less CPU update. What to compare with frames9: the CPU update, the misc and enemy
categories, the actor report (FISHMOD-BGCHECK En_Crow, En_Tite).

## Results (console, 12 Nov, run 20261112-1202: frames10 twice, frames9 between)

- **On screen: 18.38 fps (both frames10 runs) against frames9's 18.16; visible hitches 28-29% of frames against
  32-34%.** The CPU's update 1.8 ms a frame less (23.8 against 25.6 ms).
- Each hitch put down to the frame that made it (the record after it is the one that counts the late picture): in
  frames10 the RDP alone was over the frame in 49% of them, both in 33%, the CPU alone in 11%, neither in 7%
  (frames9: 34%, 40%, 17%, 9%). **The RDP is now what limits the route** (over the frame in 82% of the hitches).
