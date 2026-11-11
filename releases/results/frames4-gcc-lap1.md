# frames4-gcc on the console: the first lap (legs 1-19)

The whole game's C on GCC (`--gcc-all`), with everything up to frames4. Identified by its profile: only the GCC
build's ELF maps its samples sensibly. Lap 1, legs 1-19, against the same legs of the earlier runs (frames that sent a
USB packet left out):

| | vanilla | frames2 | frames3 + texpack | frames3b | frames4-gcc |
|---|---|---|---|---|---|
| fps | 16.9 | 13.1 | 14.6 | 14.5 | **17.4** |
| frames over 52 ms | 75% | 98% | 88% | 90% | **57%** |
| frame | 59.1 | 76.6 | 68.5 | 69.1 | **57.3** |
| CPU update | 38.0 | 48.7 | 38.9 | 41.2 | **29.6** |
| CPU draw | 18.4 | 24.8 | 26.9 | 25.0 | 23.0 |
| RSP graphics | 17.8 | 28.8 | 29.0 | 29.3 | 30.1 |
| RDP | 36.8 | 41.9 | 42.1 | 42.4 | 42.6 |
| fish update | | 7.9 | 7.6 | 8.5 | 5.4 |
| fish draw | | 7.9 | 9.0 | 7.8 | 7.2 |
| props draw | | 4.8 | 5.9 | 5.0 | 4.5 |

The mod, fish and props included, now runs faster than unmodified vanilla on the same route. Slow frames limited by
the RDP: 6% (frames2: 0%); the CPU sits idle 3.5% of the time (the first time it has).
