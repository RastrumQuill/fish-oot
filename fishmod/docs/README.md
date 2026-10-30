# fish-oot docs

Developer notes. The player-facing fish pages are in the [wiki](../wiki/README.md); patches and release notes are in
[releases/](../../releases/README.md). Building, debug scripts and tools are listed in the main
[README](../../README.md#development).

Plans
- [Roadmap](roadmap.md): what's planned, what's next, measured performance, long-term goals
- [Test5 plan](v5-plan.md): biomes, time and behaviour
- [Test5: the rest of the work](test5-remaining.md): what's left of build 5, item by item
- [Lake Hylia plan](lake-hylia.md): sub-biomes, the benchmark plan, other areas
- [Plant card optimizer](card-optimizer.md): design, not built yet
- [Refactor plan](refactor-plan.md): the refactor's stages, what each changed and how it was checked

How things work
- [Save format](save-format.md): the save block, its versions, and the SRAM map
- [How the fish are modeled](modeling.md): species looks from the pond models
- [Assets](assets.md): new textures and models through `fishmod/tools/assets.py`
- [Offline processing](offline.md): PC-side tools that make the game lighter
- [F3DEX3](f3dex3.md): the faster graphics microcode
- [Hardware rig](hw-rig.md): running and measuring on the console
- [Research](research/README.md): N64 performance, fish AI, what makes fishing feel good, TP and Sega Bass Fishing

[history/](history/) keeps the finished bench reports (hwbench-1 to 5i, the LOD tuning run) and the optimization plan
they led to. Their findings are summed up in the roadmap's "Engine and performance" section; code comments that name a bench
("hwbench-5h") point there.
