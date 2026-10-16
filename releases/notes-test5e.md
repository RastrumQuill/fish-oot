# Test 5e: everything for the next two console runs

Two overnight runs cover everything: `fish-oot-hwbench4.bps` (IDO) one night, and `fish-oot-hwbench4-gcc.bps`
(GCC) another. Each needs a fresh `.sav`. A lap is about 40 minutes; a night is enough.

## In these builds
- **Far plants (past 500 units) draw their whole cards,** uncut: the same picture in a third of the triangles.
- **Small plants draw one triangle per card:** grass tufts, floating leaves, leaf litter, spatterdock, lotus
  flowers and lotus centres, near and far.
  - The triangle holds everything that can show, and the texture is clamped at its edges.
  - Textures whose leaves reach their edges get a see-through margin. That adds up to 2 KB of texture memory each,
    all 4-bit.
  - Rushes, marigolds and fan weed would outgrow the texture memory with a margin, so they keep their cut outlines.
- **The nearest plants (within 500 units) are drawn strictly nearest first,** whatever their kind, so a plant
  hidden behind a nearer one costs only a depth read per pixel. This covers at most 128 plants.
- **Plants and the dock take the scene's light:** they dim and tint with the time of day instead of glowing at
  night. Midday looks as before.
- **Near plants' edges:** the colour-buffer-free mode within 500 units, as before.
- **Lake Hylia's intro cutscene** is marked as seen. **The tour** has narration captions and the FPS counter.

In the exact A/B capture of the triangles and strict order against the previous way: the bottom row of the grass
tufts differs slightly (the clamped texture), and edge pixels of overlapping near plants differ by order. Side by side
at 6x I couldn't spot anything else.

## The bench's modes (each against base, as shipped)
| Mode | What it measures |
|---|---|
| altedge | the old edge mode everywhere |
| trimfar | far plants cut to outlines (what the uncut far cards save) |
| notri | small plants with their cut outlines (what one triangle per card saves) |
| nostrict | the nearest plants by band and kind (what strict order saves) |
| nonear / nofar | only far / only near plants |
| noprops / nosky | no plants; no plants and no sky |

The CPU columns of the two nights compare IDO and GCC.

## Also
- `fish-oot-lake-tour-5e.bps`: the tour, with the FPS counter.
- `fish-oot-kokiri-test5e.bps`: the Kokiri / Lake Hylia test.
