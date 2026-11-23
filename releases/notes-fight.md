# The fight build (23 November)

`fish-oot-play.bps`, `fish-oot-test-lake.bps`, `fish-oot-test-kokiri.bps`, `fish-oot-test-lakechild.bps`,
`fish-oot-test-lakedrained.bps` (`fishmod\pc\play.bat` loads them). Everything since the tackle build.

## The fight (as Twilight Princess's and Sega Bass Fishing's)

- Fish fight longer and harder: runs change direction often while the fish is fresh, less as it tires; it fights at
  its own species' depth; stronger head shakes; the rumble beats with its tail.
- A run shows which way to pull ("! Pull left !" and so on).
- Leaps: hold the stick down while the fish is in the air and the line's tension halves; don't, and it may throw the
  hook (more likely on a tight line).
- Only big fish can snap the line (`snap_lbs`); smaller ones max the tension out without breaking it.
- Landing: once the fish is close, "A: LAND!" opens a short window (shorter for big fish). Press A in it; pressing
  outside it delays the next. Too long and the fish slips away. Then the hero shot: letterboxed, the camera drawing
  back, a fanfare (a bigger one for a record).
- The fight music moves with it: the bass and the drums while it runs, a strained trill as the tension rises, a surge
  as it tires and while it's being landed.
- Fish are held by the mouth (the tip of the snout) instead of the middle of the head, so big fish hang right.
- The hook and the bait are larger and easier to see: the hook hangs upright, the bait drawn round its bend (a
  wriggling worm, a larva, a bug).

## The sinker

- A weight and a float on a string, not a rigid stick: reel and the weight comes up off the bed and along, the hook
  trailing behind it on the leader; stop and the weight sinks back and the hook floats up over it. The leader sags when
  it's slack.

## Weather

- The day's weather (mostly clear, sometimes overcast, sometimes rain) is the same everywhere under the open sky
  (it was per fishing spot: walking from Hyrule Field into Lake Hylia could turn the sky at the border).
- The drained Lake Hylia's rain (vanilla's) is over the whole lake from the moment you arrive, not a zone by the
  water you walk into.
- The test saves with the lake full have the Water Temple beaten (they had the lake full but the temple not beaten, so
  vanilla's rain came in as you walked toward the water). A test file from an earlier build gets this when loaded.
- The morning the lake is restored stays clear all day.
- Vanilla's other weather zones (Lon Lon Ranch's clouds until you have Epona, the market's, Death Mountain's) blend
  from and back to the day's weather instead of jumping through a clear sky.

## Items

- The D-pad works as four more C buttons: an item put on a button while it's on another one trades places with what
  that button had (C to D-pad, D-pad to C, D-pad to D-pad), on the item page and the equipment page alike.
- Items on the D-pad have the equipped box round them on the item page, as C button items do.
- The bee larvae and worm bottles are named on the item page (they said "Empty Bottle").

## Test saves

- The Lens of Truth on D-pad Up and the double magic meter. At Lake Hylia, from above the water, the Lens takes the
  depth fog away so you can see down into it.

## Feel tuner (`fishmod\pc\feel.bat`, test builds)

New: `fight_length`, `snap_lbs`, `leap_throw`, `land_window`.

## Not checked

- No fight has been seen through in the emulator yet: an automated run at the lake (worms, 25 casts) got no bites at
  all, which is being looked into. If fish won't bite for you either, that's why.
- The sinker's new string and the fish held by the mouth haven't been looked at in the emulator.
