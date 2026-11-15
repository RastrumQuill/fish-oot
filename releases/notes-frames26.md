# frames26

**`fish-oot-frames26.bps`** (`fishmod/build.sh --script pcprof_frames --hw --texpack --colpack --meshpack --gcc-all`):
frames25 with:

- **The water fog from above is the underwater fog**: anything under the surface is fogged by the length of the
  view's ray inside the water, at the camera-under-water fog's own strength and colour (its start and end, the
  water's light setting's colour), then the scene's haze by distance over it. Looking straight down from just over
  the surface hides the bed as much as from just under it. The lake bed's vertices and, now, the props' vertices
  (plants and rocks near the camera each drawn from their own copied vertices, so a tall plant's foot is foggier than
  its top; far ones by their middle). Before, the props weren't fogged at all from above (the darkening was all they
  had) and the bed's fog was a weaker one of its own.
- **Nothing meant to be under water pokes out**: placement uses each model's real height (the grass tuft's model is
  23 tall, its listed height 16: 21 tufts stood out of the shallows) and its tilt; the build stops if any would.
- **The Fishing Pond's reed bends** when touched (its model read where object_fish is loaded; touching one was the
  frames24 crash).

Compare with frames25 (the props' copied vertices cost the CPU while the camera moves).
