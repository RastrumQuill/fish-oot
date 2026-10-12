# lake-tour1: Lake Hylia habitat tour (feedback build)

Not a playable release: a guided camera tour of Lake Hylia's new habitat props, above and below the water, so
the props can be tuned. Fish are in the lake as usual. Built from the test5 work in progress (`--opt 5`, debug build).

Apply `fish-oot-lake-tour1.bps` to the NTSC 1.2 (US) ROM. It boots straight into Lake Hylia and the tour starts.

| Button | |
|---|---|
| A | next stop |
| B | previous stop |
| Stick | turn the camera around what it's looking at (left/right), raise or lower it (up/down) |
| L | debug text on/off (frame times, positions) |

22 stops, each with a number and a caption (Navi's "Hey!" plays on arrival). For feedback, give the stop number:
"stop 11: too sparse", "stop 20: reeds too tall", and so on.

What's new to look at:
- ten more prop kinds edited from vanilla art: rushes, iris, big pondweed, spatterdock, kelp-like fronds, vines and
  vine curtains, bank roots, marigolds, fan weed;
- the Fishing Pond's own reeds (tall and see-through) in stands along the reed banks, beside the new reed clumps;
- no drowned or fallen trees any more: wood is driftwood from the Gerudo river, near the inflow, the lab shore and
  the west flat.

Updated (same name, rebuilt):
- props draw much further (up to 1500 units), nearest first whatever their kind, only what's in view, and grow in at
  the edge of their range instead of popping in;
- 300 fish in the lake (it had 10), most living by cover (weed beds, rocks, reeds, pads); on the tour they wake
  around the camera, a couple per second, so they gather at each stop;
- nothing grows on the ruins' walkway, plaza or pillars (by and near them is fine);
- the only wood is branches and a log under the island's dead tree; no drowned or fallen trees;
- a bobber's bait sinks at most about 5 m (180 units): the deep basin and the Water Temple gate are out of its reach.

Known: the underwater haze isn't in yet (the water looks like vanilla's under the surface); the pond reeds don't
sway yet.
