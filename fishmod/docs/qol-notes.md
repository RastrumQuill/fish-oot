# Quality of life: done, open, ideas

The game itself made easier to play: controls, menus, text, the HUD, the save. Not the fishing mod's own features
(records, the fishing log, bottled fish, the bait's depth, the cast's reach, the water fog, plants making way) nor the
project's own needs (the bigger verified save, debug only in test builds): those are in the roadmap.

Gathered from the roadmap (fishmod/docs/roadmap.md, its quality-of-life lists), the requests in the sessions' chat
(dates given), and play feedback.

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
- **Blocks pushed and pulled faster with each strength upgrade**: 1.35, 1.7, 2 times as fast with the Goron's
  Bracelet, Silver and Golden Gauntlets (fishmod.c Fishmod_PushSpeedScale; first numbers, to be tuned by feel).

## Open (from the roadmap)

- **Faster text**: hold B to fill a box at once; skip text already read (item gets, repeated owl talk). In
  z_message.c: B held fills the box (as A does after its delay); a "read" bit per message id (a small bitfield in the
  mod's save block) to offer the skip.

## Ideas from play

- **The Lens of Truth under water**: the game turns the lens off when Link is under the surface (z_parameter.c:
  vanilla, so it can't be used to see through the water's own fog). Letting it stay on under water, and clearing the
  underwater haze while it's on, would make it the way to see the lake's deep bed.
