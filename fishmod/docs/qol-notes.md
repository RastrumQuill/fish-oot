# Quality of life: done, open, ideas

Gathered from the roadmap (fishmod/docs/roadmap.md, its quality-of-life lists), the requests in the sessions' chat
(dates given), and play feedback. Each open item
says what it would take; none of them needs the RDP or RSP, so none should cost frame time.

## Done

- **The D-pad as four more C buttons** (bottles, boots, tunics, swords, any item): fishmod_items.c. In the pause
  menu, press a D-pad direction on an item to put it there, again to take it off; the same C button again takes an
  item off a C button too. The Lens of Truth on the D-pad now stays on (the magic meter turned it off unless a C
  button held it: frames27). (Asked 8 and 12 Nov.)
- **The D-pad on the HUD**: a cross under the C buttons shaded as they are (arrows pressed in, not embossed), its
  items with their ammo counts as the C buttons show them; shown only once something is on it, an item dimmed while
  it can't be used. (Asked 8 Nov.)
- **Equipment swaps at any time** (swords, shields, tunics, boots), from a C or D-pad button too; pressing what's
  already worn goes back to the default (Kokiri tunic and boots; Kokiri Sword and Deku Shield as a child, Master Sword
  and Hylian Shield as an adult). (Asked 12 Nov.)
- **The time of day**: a small arc under the overworld minimap, the sun or moon riding it, fading with the minimap,
  hidden where time is frozen (fishmod_items.c).
- **"New record!"** when a catch beats the best for its species (the catch's text box).
- **Strength scales Link's strength**: the cast and the line's length +35% an upgrade (about twice as far with the
  Golden Gauntlets: fishmod_rod.c STRENGTH_CAST_SCALE), blocks pushed and pulled 1.35, 1.7, 2 times as fast
  (fishmod.c Fishmod_PushSpeedScale). First numbers, to be tuned by feel.
- **Bottled fish keep their species and size** (the save's bottle slots).
- **The Lens of Truth sees through the water from above**: it turns the water fog off while it's on (frames27).

## Open (from the roadmap)

- **Faster text**: hold B to fill a box at once; skip text already read (item gets, repeated owl talk). In
  z_message.c: B held fills the box (as A does after its delay); a "read" bit per message id (a small bitfield in the
  mod's save block) to offer the skip.
- **The fishing log from the pause menu: A on the rod.** A does nothing on the item page in vanilla, so A on the rod
  opens the log (catches, records, species seen), as A on a song shows it. A on a bottled fish could show that fish
  (species, size, where it was caught). The records plaque in Link's house already shows a fishing journal: the same
  text, reachable anywhere.
- **Depth handled for you**: the float and sinker settle at a sane depth for where the bait lands (the water's depth
  there and what lives at that spot), and fish look further up and down for food. Now: the float depth is set on
  every cast (fishmod_rod.c); check it follows the water and the spot's fish, not a fixed default.
- **The buyer says what he's getting** when Link sells a bottled fish (its species and size, not "fish").
- **A bigger, verified save**: the mod's data in a larger save (flashcarts and emulators take it), each part with a
  version and a checksum, a failed part taken from the backup copy if that one is good, and otherwise loaded with a
  warning, never wiped; a small tool to recompute the checksums for anyone editing the file by hand.
- **Debug only in test builds, behind holding L** (partly done: the debug input already needs L): none in release
  builds.

## Ideas from play

- **The Lens of Truth under water**: the game turns the lens off when Link is under the surface (z_parameter.c:
  vanilla, so it can't be used to see through the water's own fog). Letting it stay on under water, and clearing the
  underwater haze while it's on, would make it the way to see the lake's deep bed.
- **Plants that make way**: done for the beds and floating plants; the spatterdock drifts like the lily pads. If it
  feels too little when swimming through, widen how far Link's body reaches the leaves.
- **Strength and casting**: already scaled (twice as far at the strongest); "Link throws stone pillars at his
  strongest" may want more than that, by feel.
