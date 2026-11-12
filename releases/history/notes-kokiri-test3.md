Kokiri Forest test build 3. Apply either patch to the **Ocarina of Time NTSC 1.2 (US)** `.z64` ROM
(MD5 `57a9719ad547c516342e1a15d5c28c3d`).

| Patch | Build command |
|---|---|
| `fish-oot-kokiri-test3.bps` | `fishmod/build.sh --opt 1` (build A's optimizations) |
| `fish-oot-kokiri-test3-testmode.bps` | `fishmod/build.sh --opt 1 --testmode` |

**Test mode:** a new file skips the intro and starts with three empty bottles and one with bugs, the slingshot,
the Goron's Bracelet, Kokiri Sword and Deku Shield, sticks, nuts and 200 rupees. It shows an FPS counter at the
bottom of the screen, and **D-pad Up / Down warps to Kokiri Forest / Lake Hylia** (Lake Hylia has the Kokiri fish
for now, for comparing a big and a small body of water).

New since test build 2:

**Bait**
- **Z** while holding the rod (before casting) puts bait on the hook, and cycles through the bait you have. A
  line under the hints shows what's on the hook and how many are left.
- **Beehives** can be knocked down with the slingshot (or arrows) and give 2–4 bee larvae. Kokiri Forest has a
  hive on the house wall by the moat.
- A **bottle of bugs** turns into three bug baits when you pick bug bait.
- Bait is used up when a fish takes it.

**Fish**
- Less interested in the hook: they notice it from closer and less often, one at a time, and stay away longer
  after losing interest.
- More alive: bodies bend into turns, startled fish flick before darting off, and fish rest near home and make
  short cruises (sometimes down to the bottom) instead of roaming at random.
- They keep away from Link: getting close, wading in nearby or running along the bank scares them off. You can't
  step on one.
- Fish that get stuck against a wall now free themselves.
- A fish kept in a bottle remembers its species and size; poured back into the water it swims off as that fish.

**Look and sound**
- Fish-on music is louder.
- A landed fish hangs head up from the line beside Link.
- Casts start fully zoomed in on the bobber (Z still cycles the zoom).
- Control hints moved up under the top of the HUD, out from behind the map and rupees.
- The pause menu's background no longer turns to garbage with the rod out. (If the menu ever hangs for a
  moment when pausing with the rod out, please say so: it showed up in the emulator but may not on hardware.)

**Planning:** Lake Hylia planning has started (fishmod/docs/lake-hylia.md): sub-biomes, the performance benchmark
plan, species sized to the water, and ideas for other areas.
