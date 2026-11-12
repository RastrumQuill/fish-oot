# qol2: the shaded D-pad and ammo counts

qol1 (see [its notes](notes-qol1.md)) with the HUD's D-pad redone:
- **A shaded pad** in the N64 controller's D-pad grey, bevelled and lit like the C buttons (our own texture,
  `fishmod/tools/gen_dpad_tex.py`), an arrow pressed into each arm under its item (seen when that button is empty).
  It fades with the C buttons; an item that can't be used now is dimmed as a disabled C button's is.
- **Ammo counts** on the D-pad's items in the C buttons' digits (a little smaller): green when full, grey at none.

Patches: `fish-oot-qol2-testmode.bps` (emulators), `fish-oot-qol2-hw-testmode.bps` (console).
