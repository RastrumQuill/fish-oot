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

Second update (same name, rebuilt):
- fish: their level of detail and their AI follow the camera (and the bobber), not Link, so fish keep swimming
  wherever the camera is; full model to 300 units, the reduced model to 750, then a far shape that now carries the
  species' own scale pattern and colours (back, belly, head), drawn to 1300;
- nothing in front of the Zora's Domain tunnel (it opens at the bottom of the plaza, under the walkway's end): the
  way out through the plaza is clear, and fewer rocks round the ruins;
- the Gerudo river inflow: no props under overhangs or inside the bank, flat pebble patches only on flat ground and
  under water;
- plants sway (weed slowly with the water, reeds faster in the wind), neighbours together like a wave;
- underwater haze: a thick fog when the camera goes under, complete by about 900 units, so far props and fish come
  and go inside it (no growing or shrinking, no change to the scene's far plane or what of the level is drawn);
- props draw further (800 / 1200 / 1600 units by kind) and only what's on screen;
- now and then a bubble or two rises from the weed near the camera (vanilla's bubble) and pops at the surface with
  vanilla's ripple, so they show from above too; under water, faint specks drift round the camera;
- Lake Hylia's three gossip stones draw from much further (vanilla stops drawing them past about 1350 units);
- tall plants sway less (every tip moves about the same few units);
- the drifting specks are little soft dots now (vanilla's Kokiri Forest dust mote, much smaller, the water's colour),
  drifting smoothly and fading in and out;
- rocks are solid (walk the bed with the Iron Boots): chunky shared hulls, the rock garden merged coarser;
- a dock on the north beach (the Fishing Pond's planks), solid with wooden footsteps: walk out and fish from its end;
- fish rest for shorter spells, look alive while they do (fins working, a slight hover), and their rest runs at the
  same pace far from the camera as near it (before, a fish that had been far off was often still resting, frozen,
  for a few seconds when the camera arrived);
- the props cost much less: what's drawn is only worked out again when the camera moves (lake bench: 1.1 ms a
  frame, was 5.5), and more plants are patches;
- the first two stops are under the water over the west weed flat (from above you can't see through the surface).

Third update (same name, rebuilt):
- no plants sticking out of the water at the edges: an underwater plant's size is now checked against the shallowest
  bed all round it, not just under its middle, so on a slope (the island's shelf, the foot of the back walls) the
  shallow side no longer pokes out; rushes and other plants that stand out of the water grow only on gentle banks;
- the island's fallen wood lies along the bed, tilted to its slope (pieces that would sit too steeply aren't placed);
- the bass keeps its mouth shut (the model's own pose gaped); it opens only to bite and while it fights, and
  then draws both sides of its surfaces, so you don't see out through the mouth; a fish that gets away shuts it;
- far fish (the flat shape past 750 units) also face the camera from every side now: nose-on or tail-on they're a
  short oval, not a line seen edge-on.

Fourth update (same name, rebuilt): fish behaviour, first pass
- mood: each fish is more or less hungry and more or less frightened. Hungry fish rest less, pick at the bottom
  more, notice a bait from further and bite sooner; frightened ones (spooked, or near a fish being caught) stay by
  their home cover, rest longer, startle at less and won't come to a bait until they calm down (about a minute);
- senses: a fish sees a bait or Link ahead of it, not behind, and less far at night or in rain; a bobber plopping
  in nearby draws hungry fish to look;
- schools: greengill swim in schools of 4 to 8 behind a leader, and bolt together when one is scared.
