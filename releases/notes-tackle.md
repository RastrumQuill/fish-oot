# The tackle build (22 November)

`fish-oot-play.bps`, `fish-oot-test-lake.bps`, `fish-oot-test-kokiri.bps`, `fish-oot-test-lakechild.bps`,
`fish-oot-test-lakedrained.bps` (`fishmod\pc\play.bat` loads them). Everything since the Kokiri test build.

## Fixes

- An item put on the D-pad comes off the C buttons, and one put on a C button comes off the D-pad (no more doubles).
- The bait bottles' icons on a C button no longer break after a room change (they were read from the game's icon
  file at their number).
- Title screen and credits: the clock runs at vanilla speed and the weather isn't rolled there (the white night fog).
- The cast no longer flashes one frame of sky (the fishing camera was drawn from the world's origin on its first
  frame).
- Z-targeting under water: the bars are black again (they took the water fog's colour).
- Tall weed and ribbon weed: no dark line along their tops (their texture wrapped).
- Drained Lake Hylia: rocks out of the water have no moss; fish no longer appear above the water (the roster is made
  again once the lake's water level is set).

## Fishing

- **Tackle** (the item page's lure slots; use one with the rod out and not cast, use it again for the bobber):
  - Pond lure: floats; reel to make it dive and wag, stop and it rises; a twitch ducks it.
  - Sinking lure: sinks to just over the bed; reel to hop it up, stop and it settles.
  - Sinker: the weight anchors on the bed, the baited hook floats 18 above it on its leader. Its item shows the bobber
    while the sinker is on (pressing it then puts the bobber back).
  - The camera follows the tackle under water once the bait is 45 deep (any rig, the bobber's bait too).
- **Worms**: a bottle of worms (three baits); catfish like them best.
- **The bait's action, per species** (as Sega Bass Fishing's): twitches draw the predators and put the bottom feeders
  off; a slow reel draws the chasers; stopping just after a twitch or the reel is when a chaser strikes; too many
  twitches and the wary lose interest. Pike and bass love it; carp, catfish and loach want the bait still.
- **New fish at Lake Hylia**: the Hylian Pike and the Hearty Bass.
- **Rumble** through the fishing: a tap per nibble, hard jolts for the bite and the hook, the fight felt through the
  line (none on a slack line), jolts on runs and leaps, a buzz while the drag pays out.
- Small fish reel in faster (a light fish can't hold against the reel), so they don't go slack without fast-reeling.
- Plants move less when pushed through.

## Test saves

Bait in all but one bottle: three of bee larvae, one of worms, two of bugs (one on C-Right), one empty for a catch;
the pond lure, the sinking lure and the sinker on the item page.

## Feel tuner (`fishmod\pc\feel.bat`, test builds)

New: `reel_light` (how much faster the lightest fish reel), `rumble_nibble`, `rumble_bite`, `rumble_fight`,
`plant_lean`, `plant_drift`. Each species' `action` and `chase` are in `fishmod/data/species.json`.

## Icons

The tackle's icons are rendered from the Fishing Pond's own models; the larva and worm bottles are made from the
game's own potion bottle (its glass and highlights), the larvae suspended in honey.

## Not checked on the console

The tackle was checked in ares (each rig's look, the camera going under); a fight on a lure or the sinker hasn't been
seen yet. Rumble can't be felt in the emulator.
