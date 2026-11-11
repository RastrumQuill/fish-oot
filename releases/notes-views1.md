# views1: what each part of the mod costs the RSP, the RDP and the CPU

**`fish-oot-views1.bps`** (`fishmod/build.sh --script pcprof_views --hw --colpack --gcc-all`): frames5 with the view
bench in place of the play route. Apply it to the same ROM as the frames patches, and run it with the SummerCart64's USB
logging (`sc64deployer debug`), as the frames runs. It needs no input: about 25 minutes, then "VIEW BENCH DONE".

The camera goes round the Lake Hylia tour's 22 stops. At each one, once it has settled, the scene is frozen and seven
settings take turns for 60 frames each, on the same picture:

1. as shipped
2. no fish
3. no props
4. no bed cards (their clumps drawn plant by plant: what the bed cards save)
5. no water surface
6. no sky
7. neither fish nor props

Each setting's first 20 frames are left out. Read the run with `fishmod/tools/viewbench.py FOLDER`. It prints what each
part costs the RSP, the RDP and the CPU, on average and stop by stop. (The actors are frozen, so the CPU's update is not
the game's; the draw is.)
