Kokiri Forest test build 4: the performance work. Apply a patch to the **Ocarina of Time NTSC 1.2 (US)** `.z64` ROM
(MD5 `57a9719ad547c516342e1a15d5c28c3d`).

| Patch | What it is | Build command |
|---|---|---|
| `fish-oot-kokiri-test4.bps` | The game | `fishmod/build.sh --opt 4` |
| `fish-oot-kokiri-test4-testmode.bps` | New files skip the intro and start with testing items | `fishmod/build.sh --opt 4 --testmode` |
| `fish-oot-kokiri-test4-animstudio.bps` | Animation studio, not the game: every species swimming in place on a plain backdrop | `fishmod/build.sh --script anim --opt 4` |

**Faster fish**
- Fish are drawn with a lighter skeleton: only the parts that move get a matrix each frame, and fins that never move on
  their own are baked into the body. They look the same. In a tank of 36 fish around Link, drawing them went from about
  13 ms to under 2 ms a frame (emulator measurements).
- Fish a bit further away (and small fish) use a simpler bend (head, middle and tail); far away they're the flat shape.
- Fish far from Link stop thinking, and fish far from their home water aren't in the scene at all until you come back.
- With the Expansion Pak, the game uses the extra memory (more room for the scene and the water floor cache).

**Fish stay put for the day**
- Each fishing spot has its own fish for the in-game day: the same species, sizes and homes every time you visit. A fish
  you keep in a bottle is gone until the next day; one you release stays. The water no longer restocks itself every few
  seconds.

**Changed**
- Catfish whiskers are off for now (too thin to see); they'll come back redone.
- Fish spread out by depth in deep water instead of keeping to a few levels.

**Animation studio** (the third patch): boots straight into the studio and loops through all six species, about 20
seconds from above and 20 from three-quarter each. Left to right: full skeleton, the simpler one, and a small fish on
the simpler one with bigger bends. In ares, Settings > Nintendo 64 > Quality: HD or UHD renders it sharper.

**Known:** the larger fish pinch just behind the head when they bend, and turning swings the body around the head like
a pendulum (real fish pivot about a third of the way back). Both are next, together with the larva bottle icons.
