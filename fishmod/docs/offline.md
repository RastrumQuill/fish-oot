# Offline processing: work done on the PC so the N64 does less

Tools that spend PC time (minutes to hours) to make the game look the same or better and run lighter. All of them
read the user's own extracted ROM data, so what they make goes to `src/fishmod_data/` (not committed); the build
runs them, and the heavy searches cache their results.

## In the build

| What | Tool | Effect |
|---|---|---|
| Plant cards cut to their textures' outlines | `card_outline.py` (from `gen_habitat.py`) | Same picture; plant fill down 16-21% at the heaviest tour stops (`overdraw.py`, console settings) |
| Draw distances measured on the game's own frames | `lod_calibrate.py` + `scripts/lod_calibrate.inc.c` -> `include/fishmod_lod.h` | Under water each kind stops where it can no longer change a pixel (fog); above water nothing is cut (it still shows at 3000) |
| Per-zone distances over ~1100 views | `gen_lod_views.py` + `scripts/lod_sweep_p*.inc.c` -> `include/fishmod_lod_zones.h` | The same, per zone of the lake, from the shores, the surface and the bed |
| Lake Hylia's bed shaded under the full lake | `gen_bedshade.py` | Darker under dense weed and in deep water (brightness only: the ground is lit, see below) |

### Card outlines

`card_outline.py` searches (simulated annealing) for a polygon of up to 10 vertices that holds every texel that can
show (opaque, plus the one texel the N64's filter reaches) with the least area, a vertex costing 1.2% of the card.
The cache keeps the best found; run it with more effort to improve it:

    fishmod/tools/card_outline.py --iters 200000 --restarts 16

The best possible with any number of vertices is 52-91% of the card (the exact outline, grown a texel); the search
gets 60-96% with 5-9 vertices (the roots card, nearly solid, is left whole).

### LOD calibration and the sweep

Everything that moves is held still while measuring (both frame counters, the clock, the sky's turn, the
underwater wobble; no fish or particles), each frame is copied off the screen (again if the screen swapped
mid-copy), and every test is compared with a reference drawn just before it. A pixel differs if any 5-bit channel
moves by more than 2; a kind may change at most 2 pixels.

The sweep tests each view with every kind cut at once, and only splits the kinds (halves, down to the ones that
show) when that changes the frame. Run the three parts side by side, then:

    fishmod/tools/lod_calibrate.py --sweep sw0.ram sw1.ram sw2.ram --write

Distances only ever shorten a kind's own reach (`fishmod_scatter.c`), so the game is never asked to draw more than
before. Where the measure says a kind needs more than its reach, it shows (pops) today: those are listed, not raised.

### Bed shading

Lake Hylia's ground is drawn lit, so its vertices hold normals, not colours: a colour tint per vertex isn't
possible without losing the day, night and underwater lighting. F3DEX2 doesn't normalise normals, so a shorter
normal takes less of the sun: that's the shading. Only ~340 ground vertices lie under the full lake, so it is broad.
The drained lake is left as it was.

## Considered and not done

- **Pre-built meshes for still props** (rocks, pads, logs): only 14% of the props; their matrices are already only
  made when the camera moves; batching them would lose the nearest-first order and the per-prop distance cut.
- **Vertex-cache ordering**: `assets.py` already loads each card's vertices together, 32 at a time.

## For a bigger machine (later, local)

- **Cards fitted to detailed plants**: search the cards' angles and textures that best imitate a detailed plant
  from every angle the game shows it. Needs a score that matches the eye: an aesthetic model, or a small
  "plant / not plant" discriminator trained on renders.
- **Placement for lushness per cost**: anneal where plants go, scored on many rendered views (coverage and
  variety, by the same kind of model) against overdraw.
- **Textures through the N64's filter**: palettes and texels chosen by simulating the 3-point filter, fog and
  dither at real viewing distances.
- **Fish tuned against simulated anglers** (fishing-feel phase).
