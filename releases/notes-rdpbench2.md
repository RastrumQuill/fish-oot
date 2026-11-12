# rdpbench2: the RDP calibration bench, with the RDP's counters

**`fish-oot-rdpbench2.bps`**: rdpbench1 (see [its notes](history/notes-rdpbench1.md)) with the RDP's own counters
recorded (clock, command buffer, pipeline, TMEM). rdpbench1 never sampled them (the play bench does that for itself,
and this isn't the play bench), so its run had RDP times but zero counters.

rdpbench1's console times (the frame setup alone 2.5 ms; a full-screen fill about 0.9 ms; a full-screen texture 1.45 ms
in 1-cycle, 4.2 ms with Z; about 2.4 us a texture rectangle; tiny triangles bound by the RSP, not the RDP) are in
[history/](history/).
