# refactor1: the refactored build (for testing)

The game after the refactor: it should play like the last test builds. What to look out for is anything that
behaves differently.

## The patches
- **`fish-oot-refactor1.bps`**: for emulators (ares, Project64, ...).
- **`fish-oot-refactor1-hw.bps`**: for the console (flashcart), with the F3DEX3 graphics microcode (HLE emulators
  can't run it).
- **`-testmode`** versions of both: a new file skips the intro and starts with testing items. In test mode the warps
  and the frame-time readout need L held (L + D-pad Up: Kokiri Forest, L + D-pad Down: Lake Hylia, L + D-pad Left:
  the readout).

## What changed for a player
- **The rod works everywhere.** Take it out in any scene; casting needs only water. Water without fish never bites.
- **The rod is a C item only.** L no longer puts it on B; L is the minimap's again.
- **Saves:** the mod's save block moved to a fixed place with a version mark. A file from an older build is moved over
  the first time it loads (catch records, items and bottled fish kept).
- **Each day's fish are new.** The mod has its own random numbers now, so the fish in each spot on a given day differ
  from the older builds (still the same all day).
- Fixes: the heart-piece icon, giving the rod or a lure overwriting the Deku Stick count, the fight music not stopping
  in some exits, bait used from the wrong bottle.

## Checked
The scripted runs give the same game states as before the refactor (bait, the lake, the pause screen, new file, load
file, the journal); both release builds build. Per fish the game does the same work or less (ares).
