# Test 5g: far tiers and the bank test

## `fish-oot-hwbench5.bps`: one overnight bench
As before: boot it, leave it, send the `.sav`. It needs a fresh save. It's a GCC build (now the default).

| Mode | What it tells us |
|---|---|
| base | as shipped |
| noimp | no far single cards: whole cards out to the cutoff (what the single cards save) |
| nofartex | far props on full-size textures (what the half-size textures save) |
| nodissolve | no dissolve past the cutoff (what the extra faded cards cost) |
| zsame | the depth buffer moved into the frame buffers' memory bank (how much the bank layout matters) |
| nonear / nofar / noprops / nosky | where the time goes, as before |

## `fish-oot-lake-tour-5g.bps` and `fish-oot-kokiri-test5g.bps`
The tour with the FPS counter, and the Kokiri / Lake Hylia test, with the far tiers, the stretched and tilted rocks
and exact rock collision. Worth looking at: plants in the distance (they should look the same), and plants at the edge
of their distance dissolving instead of popping.

## What changed in the far distance
- **Half-size textures** for far plants, per kind from where a texel of the full texture is under half a pixel.
- **One card per plant** (17 upright kinds) once it's at most 16 pixels tall and seen from near the side (within 20
  degrees): a picture of the whole plant, rendered at build time from its own mesh, on one triangle turned to face the
  camera.
- **A dissolve past each kind's distance** (15% further), with those single cards, instead of the plant popping.

Exact A/B against the old far drawing at all 22 stops (ares, raw frames): within the old distances the differences
are in the underwater weed beds seen through the surface, slightly smoother texture detail (`far-ab-worst.png`:
the three worst places, old left, new right, at 3x). The largest share comes from the half-size textures.

## Decided before the results
Under 2% counts as no change.
- **noimp:** keep the single cards if noimp is 2% or more slower than base.
- **nofartex:** keep the half-size textures only if nofartex is 2% or more slower (they change the picture a little).
- **nodissolve:** if the dissolve costs 2% or more, shorten its band or drop it.
- **zsame:** if moving depth into the frame buffers' bank costs 3% or more, the bank layout matters and the depth
  buffer gets a bank of its own (away from the command buffer and display lists); otherwise the layout stays.
