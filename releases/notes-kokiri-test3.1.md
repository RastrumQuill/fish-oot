Kokiri Forest test build 3.1: fixes for what turned up in test 3. Apply either patch to the
**Ocarina of Time NTSC 1.2 (US)** `.z64` ROM (MD5 `57a9719ad547c516342e1a15d5c28c3d`).

| Patch | Build command |
|---|---|
| `fish-oot-kokiri-test3.1.bps` | `fishmod/build.sh --opt 1` |
| `fish-oot-kokiri-test3.1-testmode.bps` | `fishmod/build.sh --opt 1 --testmode` |

**Bait is in bottles now**
- Knock down a beehive (slingshot, arrows or sword) and a chunk of comb falls out. Swing an **empty bottle** at it
  to scoop it up: a bottle of bee larvae (3).
- With the rod out (before casting), **use the bait bottle from its C button** to put one on the hook: larvae
  3 → 2 → 1 → empty bottle. A bottle of bugs works the same way (one use).
- The line under the hints shows what's on the hook. Z is no longer used for bait.
- The new bottles' icons are made from the vanilla bottle; in the pause menu they show the name "Bottle" for now.

**Fixes**
- A landed fish hangs by the head from Link's hand, for every species and size.
- The catch textbox no longer overflows: the "Keep it in a bottle?" choice is in its own box.
- A closer zoom level (half the distance of the old closest); casts start there. Z still cycles the zoom.
- A bobber dragged off the bank into the water floats again.
- Pause background garbage: nothing fishmod draws (hints, banners, the test-mode FPS counter) goes into the
  pause menu's picture any more. That was the slingshot / after-the-hive case.
- A bottled fish poured out on land is the vanilla flopping fish again (and if you bottle it, it's still the
  fish it was).

**Known:** in the emulator, pausing with the rod out can freeze for a few seconds and then carry on. If it ever
happens on hardware, please say so.
