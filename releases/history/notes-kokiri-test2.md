Kokiri Forest test build 2. Apply either patch to the **Ocarina of Time NTSC 1.2 (US)** `.z64` ROM
(MD5 `57a9719ad547c516342e1a15d5c28c3d`).

| Patch | Build command |
|---|---|
| `fish-oot-kokiri-test2.bps` | `fishmod/build.sh --opt 1` (build A's optimizations) |
| `fish-oot-kokiri-test2-testmode.bps` | `fishmod/build.sh --opt 1 --testmode` |

The test-mode patch is the same game, but a **new file** skips the intro and starts with three empty bottles and
one with bugs, the slingshot (30 seeds), the Goron's Bracelet, Kokiri Sword and Deku Shield, sticks, nuts and
200 rupees. Existing files are not changed.

New since build A:
- **Fish-on music:** a fast, repeating bass line with a tense (not combat) feel replaces the area music while a
  fish is hooked; the area music comes back when it's landed or lost.
- **Fights:** the rod bends toward the fish and harder the more the pull is across it, and shakes during runs;
  Link's arms lean toward the pull. Fish mix runs away, sweeps across the line, and charges back toward you that
  throw slack into the line, and they go deep between runs.
- **Landing a fish:** item fanfare, the camera swings round to the front of Link holding the fish up, and a
  textbox names it with its size (and any new record) and asks whether to keep it in a bottle.
- **Casting:** casts go farther, and each strength upgrade adds 20% to the longest cast.
- **B button:** going through doors with the rod out no longer turns B into a Deku Stick when you have no sword.
- Not yet fixed: a crash when loading a save on hardware (being investigated).

