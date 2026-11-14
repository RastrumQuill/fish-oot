# RDP sweep, console, night of 13-14 November (releases/fish-oot-rdp-sweep.bps)

fishmod/tools/rdp_sweep_report.py on the run's three packet files (140 laps, clear dawn, every other lap as shipped):

403200 frames, laps 0-139; shipped laps' RDP from 44.5 to 44.3 ms (first and last: the night's drift)

## Saved by leaving it out (ms a frame, mean over its laps; RDP / drawing / texture loading; hitch points)

| left out | laps | RDP | drawing | loading | frame | hitches |
|---|---|---|---|---|---|---|
| no props | 12 | 8.46 | 8.46 | 0.52 | 3.84 | +23.0 |
| no fish | 12 | 1.27 | 1.27 | 0.24 | 1.98 | +11.8 |
| no water surface | 12 | 0.02 | 0.02 | -0.00 | 0.02 | +0.1 |
| no sky | 12 | 3.05 | 3.06 | 0.04 | 1.64 | +9.8 |
| no HUD | 11 | -0.15 | -0.15 | -0.01 | -0.16 | -0.9 |
| none of them | 11 | 14.39 | 14.37 | 0.91 | 3.87 | +23.1 |

## By leg: RDP ms saved (spread across its laps)

| leg | no props | no fish | no water surface | no sky | no HUD | none of them |
|---|---|---|---|---|---|---|
| 1 | 12.5 ±1.0 | 2.6 ±0.8 | -0.0 ±0.8 | 2.7 ±0.7 | -0.0 ±1.1 | 20.0 ±0.7 |
| 2 | 7.4 ±0.8 | 1.1 ±0.6 | -0.0 ±0.6 | 3.9 ±0.2 | -0.0 ±0.4 | 13.6 ±0.7 |
| 3 | 16.6 ±1.0 | 1.3 ±0.8 | -0.1 ±0.9 | 2.4 ±0.6 | 0.2 ±0.6 | 21.3 ±0.6 |
| 4 | 12.6 ±0.8 | 1.8 ±0.6 | -0.1 ±0.4 | 3.2 ±0.4 | -0.1 ±0.4 | 20.4 ±0.7 |
| 5 | 14.2 ±2.2 | 1.8 ±0.5 | 0.1 ±0.3 | 2.5 ±0.2 | -0.0 ±0.3 | 23.7 ±0.4 |
| 6 | 6.2 ±2.5 | 0.6 ±3.2 | 0.1 ±3.0 | 3.1 ±3.7 | -0.7 ±3.4 | 13.5 ±1.4 |
| 7 | 6.1 ±0.6 | 0.8 ±0.9 | -0.3 ±1.0 | 4.8 ±0.5 | 0.0 ±1.1 | 11.7 ±0.6 |
| 8 | 5.0 ±0.5 | 0.8 ±1.1 | -0.2 ±0.8 | 4.3 ±0.9 | -0.0 ±1.0 | 11.0 ±1.0 |
| 9 | 5.5 ±1.7 | 1.0 ±2.4 | -0.3 ±0.9 | -0.6 ±1.2 | -0.3 ±1.2 | 6.6 ±1.6 |
| 10 | 7.4 ±0.7 | 0.8 ±0.7 | -0.1 ±0.7 | 4.5 ±0.8 | -0.6 ±1.3 | 12.3 ±0.5 |
| 11 | 2.4 ±0.9 | 1.4 ±0.4 | 0.1 ±0.7 | 4.4 ±1.0 | -0.2 ±0.7 | 8.9 ±0.5 |
| 21 | 5.8 ±2.6 | 1.3 ±2.8 | 1.1 ±2.4 | 1.1 ±2.7 | -0.1 ±2.7 | 9.5 ±1.7 |
