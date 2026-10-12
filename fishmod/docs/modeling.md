# How the fish are modeled

Every fish in the mod is built from the two models in Ocarina of Time's Fishing Pond, the **bass** and the
**loach**. No new 3D models are needed; each species reshapes, recolors and retextures one of them. All of
it is data in [`fishmod/data/species.json`](../data/species.json), so a species can be tweaked without
touching code.

## Layers

1. **Base model**: `bass` (deep-bodied, spiny fins) or `loach` (long, flat-headed bottom-dweller).
2. **Whole-fish shape**: `shape` = `[length, height, width]` scales the whole model, e.g. the Greengill is
   short and deep (`[0.8, 1.15, 0.85]`).
3. **Per-part look**: `look.parts` gives each body part its own color multiplier (`tint`) and scale.
   Parts: `head`, `bodyFront`, `bodyRear`, `tail`, `finDorsal`, `finPectoral`, `finBelly`.
   Scaling a part doesn't drag its children along, so a wider head doesn't inflate the tail.
4. **Texture patterns**: `look.textures` repaints the model's textures per role (`back`, `belly`, `head`,
   `side`, `fin`) with pattern operations: `recolor`, `bars`, `spots`, `blotches`, `spot`,
   `brightness`. The eye is always kept. See
   [`gen_textures.py`](../tools/gen_textures.py) for every option.
5. **Extras**: `look.whiskers` adds catfish barbels.

## Which limb is which

Found with [`model_preview.py`](../tools/model_preview.py) and confirmed in game.

| Part | Bass limbs | Loach limbs |
|---|---|---|
| head | 0 | 0 |
| bodyFront | 9 | 1, 2, 7 |
| bodyRear | 6 | 5, 6 |
| tail | 4, 5 | 4 |
| finDorsal | 8, 11, 12 | 3, 10 |
| finPectoral | 13, 14 | 8, 9 |
| finBelly | 7, 10 | – |

| Texture role | Bass texture | Loach texture |
|---|---|---|
| back | `gFishingFishTex_0016D0` (16×16) | `gFishingLoachTex_00ED28` (32×32) |
| belly | `gFishingFishTex_0018D0` (16×16) | `gFishingLoachTex_00F528` (32×32) |
| head | `gFishingFishTex_001AD0` (32×32, has the eye) | `gFishingLoachTex_010528` (32×32, has the eye) |
| side | – | `gFishingLoachTex_00FD28` (32×32) |
| fin | `gFishingFishTex_0022D0` (16×16) | `gFishingLoachTex_010DA8` (16×16) |

## How it works in the game

When the fishing object loads, the fish display lists in RAM are patched once:

- the pond's own tint commands are removed, so each fish sets its tint per limb while drawing;
- the role textures are pointed at display-list segments `0x08`–`0x0C`, so each fish picks its species'
  textures (or the pond originals) by setting those segments before drawing.

Generated textures are tiny (16×16 or 32×32, 0.5–2 KB each), so a whole species costs a few KB.

## Making the renders

```bash
fishmod/build.sh --script photo
fishmod/tools/photo_capture.py fishmod/out/fish-oot-debug-photo.z64 --ares /path/to/ares
python3 fishmod/tools/gen_species.py   # refresh the wiki pages
```

The photo build shows each species in turn on a plain backdrop high above Kokiri Forest, from the side and
from a three-quarter view. The capture script reads a colored marker in the corner to know which photo is on
screen and saves `fishmod/wiki/img/<species>.jpg` and `<species>_34.jpg`. Renders are from
[ares](https://ares-emu.net/), an accurate N64 emulator, so they look the way the game does on hardware.

[← Docs](README.md)
