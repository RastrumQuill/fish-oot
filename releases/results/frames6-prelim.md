# frames6 on the console: preliminary

The same laps and legs from each run (39 lap/leg pairs; frames that sent a USB packet left out):

| | vanilla | frames5 | frames6 |
|---|---|---|---|
| fps | 16.7 | 17.6 | 18.1 |
| frames over 52 ms | 80% | 55% | 46% |
| frame | 60.0 | 56.9 | 55.2 |
| CPU update | 38.2 | 27.0 | 25.8 |
| CPU draw | 19.2 | 23.2 | 21.5 |
| RSP | 19.1 | 30.2 | 29.0 |
| RDP | 38.6 | 42.8 | 40.9 |
| fish update | | 4.5 | 4.7 |
| fish draw | | 7.5 | 6.8 |
| props draw | | 4.7 | 3.8 |

Leg 1 (across the west flat, under water), F3DEX3's counters: 5,169 vertices, 3,050 triangles in, 1,329 drawn; the
RSP stalled 19 ms a frame on a full RDP FIFO (the RDP's the limit there: pipeline busy 81%, TMEM loads 5%).
