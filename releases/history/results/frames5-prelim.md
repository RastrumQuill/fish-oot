# frames5 on the console: preliminary (laps 1-2, legs 1-14)

The same laps and legs from each run (36 lap/leg pairs; frames that sent a USB packet left out):

| | vanilla | frames4-gcc | frames5 |
|---|---|---|---|
| fps | 16.6 | 17.2 | 17.5 |
| frames over 52 ms | 81% | 62% | 58% |
| frame | 60.2 | 58.3 | 57.3 |
| CPU update | 38.5 | 30.0 | 27.6 |
| CPU draw | 19.1 | 23.8 | 23.4 |
| RSP graphics | 19.7 | 31.1 | 31.2 |
| RDP | 38.7 | 43.4 | 43.5 |
| fish update | | 5.5 | 4.7 |
| fish draw | | 7.4 | 7.6 |
| props draw | | 4.5 | 4.7 |

Frame time by leg (ms):

| leg | vanilla | frames4-gcc | frames5 |
|---|---|---|---|
| 1 | 53.1 | 62.9 | 62.2 |
| 2 | 59.1 | 63.2 | 61.2 |
| 3 | 58.4 | 62.9 | 62.2 |
| 4 | 67.9 | 73.6 | 70.1 |
| 5 | 75.0 | 68.8 | 67.6 |
| 6 | 76.1 | 66.9 | 63.4 |
| 7 | 68.2 | 58.7 | 58.8 |
| 8 | 59.2 | 58.0 | 56.3 |
| 9 | 58.8 | 60.7 | 57.6 |
| 10 | 57.2 | 57.2 | 56.1 |
| 11 | 62.1 | 62.4 | 61.2 |
| 12 | 60.0 | 51.9 | 52.0 |
| 13 | 54.4 | 50.0 | 50.0 |
| 14 | 56.3 | 50.1 | 50.0 |

CPU update fell 2.4 ms a frame (fish update 0.8 of it), but the frame only 1.0 ms: in the slow legs the RDP (43.5 ms
on average, 54 in the ruins) and the CPU's draw are now close behind the update. Slow frames by the longer of CPU
and RDP: CPU 88%, RDP 12%.
