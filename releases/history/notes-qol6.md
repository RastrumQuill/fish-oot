# qol6: equipment on a button works at any time

**`fish-oot-qol6-testmode.bps`** (emulators), **`fish-oot-qol6-hw-testmode.bps`** (console): qol5 (see
[its notes](history/notes-qol5.md)) plus:

- **Equipment on a C or D-pad button can be swapped at any time Link can be controlled,** even when the C buttons are
  greyed out (swimming, climbing, hanging, holding something). Not while paused, in a cutscene, with text up, or at a
  game over.
- **Pressing equipment that's already worn puts the default back:** the Kokiri tunic and boots; the Kokiri Sword and
  Deku Shield as a child, the Master Sword and Hylian Shield as an adult. If the default isn't owned (or is what's worn
  already), the error sound plays.
- Each age wears only what the pause menu lets it (the Kokiri Sword is a child's, the Iron Boots an adult's...).
- A sword or shield still can't be swapped while it's in Link's hand or raised (the held model would stay the old one).

Also in this build, since qol5: the audio, collision and props work of frames7 and frames8 (see the bench notes in
[history/](history/)), and the third frame buffer (run-ahead).
