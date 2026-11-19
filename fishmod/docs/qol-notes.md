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

- **Faster text**: text printing at twice the speed, as in Majora's Mask and Ocarina of Time 3D; hold B to fill a box
  at once; skip text already read (item gets, repeated owl talk). In z_message.c: the print speed doubled; B held
  fills the box (as A does after its delay); a "read" bit per message id (a small bitfield in the mod's save block) to
  offer the skip.

- **Movement that feels more responsive**: mostly how Link answers the stick, partly the delay before the picture
  shows it.
  - *Turning and answering the stick* (the main part): his speed builds slowly from a standstill, he turns in a wide
    arc at a run, he skids and turns round instead of reversing, and he slides on a little after the stick lets go
    (z_player.c: the acceleration, turn rate and deceleration by state). Tighter starts, a sharper turn at speed, a
    quicker turn-round and a shorter stop, on the ground and in the water, by feel; without changing what he can
    reach (jump distances, ledges, what the game's puzzles expect stay as they are).
  - *The delay from a press to the picture*: the frame log measures it (the TV's own delay not included): about
    120 ms on average now (the frames28 set: mean 118-127 ms), about six refreshes at 20 fps. Shorter by reading the
    controller later in the frame (as late as the update can still use it), by fewer hitches (a late frame holds the
    last picture a refresh longer), and run-ahead's buffering (made for consistency, not speed) looked at again for a
    frame it could give back.

## Ideas from play

- **The Lens of Truth under water**: the game turns the lens off when Link is under the surface (z_parameter.c:
  vanilla, so it can't be used to see through the water's own fog). Letting it stay on under water, and clearing the
  underwater haze while it's on, would make it the way to see the lake's deep bed.

## From another romhack's list (19 Nov: these fit)

- **The pause menu's delay**: the subscreen opens and closes without its wait (with the responsiveness work).
- **Gold Skulltula tokens don't freeze Link**: he keeps moving as the token is collected, as in Majora's Mask's
  spider houses.
- **The rupee counter coloured by wallet**: its colour shows which wallet Link has.
- **The Stone of Agony without a Rumble Pak**: an icon by the rupee counter when a hidden grotto is near.
- **"Navi" centred on the C-Up button.**
- **The Burning Kakariko cutscene from any entrance**, not only the main one.
- **The lab's diving heart piece at any time** (it can't be missed).
- **Graves as in the later releases** (their behaviour fixed).
- **No Song of Storms cooldown** (where it can be played stays as it is).
- **The Bunny Hood as in Majora's Mask**: Link runs faster while he wears it.
- **The Goron and Zora tunics for young Link** (with the equipment swaps). The dialogue that says he can't wear them
  changes with the quests' text (roadmap.md, Quests).

Not taken: guaranteed fish bites (the fishing is this mod's design), extended draw distance (a performance cost),
new Link models, the bridge needing every medallion, warps in the Gerudo Training Ground and Ganon's Castle,
Farore's Wind across time, Biggoron in two days, the text revamp, Bombchu Bowling's prizes in a fixed order, Z
targeting on Hold by default.
