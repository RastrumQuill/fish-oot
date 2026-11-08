# qol1: quality-of-life test build

## The patches
- **`fish-oot-qol1-testmode.bps`**: emulators. **`fish-oot-qol1-hw-testmode.bps`**: the console (F3DEX3, with the
  occlusion plane and bed cards: the same features as the four-way bench, whose crash isn't explained yet).
- Test mode: a new file skips the intro and starts with testing items. Warps need L held (L + D-pad Up: Kokiri
  Forest, L + D-pad Down: Lake Hylia), so **the D-pad buttons don't work while L is held**.

## What's new
- **The D-pad: four more item buttons.** In the pause menu's item page, point at an item and press a D-pad
  direction to put it there (the same direction again takes it off). Child and adult Link each have their own D-pad,
  as with the C buttons. The HUD shows a small cross with the four items under the C buttons, dimmed while the C
  buttons are off (cutscenes, minigames, horseback...).
- **Equipment on buttons.** On the equipment page, point at a sword, shield, tunic or boots and press a C button or a
  D-pad direction. Pressing that button in play puts it on: boots and tunics switch back to the Kokiri ones if already
  worn (Iron Boots on and off without pausing), swords and shields switch (not while one is in Link's hand).
- **Faster text:** hold B and a text box fills at once.
- **Strength:** each strength upgrade pushes and pulls blocks faster (1.35x, 1.7x, 2x), and casts further; casting
  distance is also scaled per area (Lake Hylia full, Kokiri Forest 0.6x, others 0.8x). These are first numbers, to be
  tuned with the feel pass.

## Known gaps
- No ammo counts on the D-pad icons yet.
- A bottle used from the D-pad works like one on a C button; a fairy revival takes a fairy from a C button first.
- The save block grew by 8 bytes (version 2): older files load with an empty D-pad.

Checked in ares: items and equipment on the D-pad from a script, the icons on the HUD, a tunic toggled on and off,
a bomb taken out from D-Up; an item put on D-Up from the pause menu.
