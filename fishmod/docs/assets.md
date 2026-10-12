# Assets

New art for the mod (textures, icons, small models) goes through one manifest, `fishmod/assets/assets.json`, and one
tool, `fishmod/tools/assets.py`, which turns it into C that the build compiles. `fishmod/build.sh` runs the tool
before every build, so editing a PNG or OBJ and rebuilding is enough.

## Adding a texture

1. Put the PNG somewhere under `fishmod/` (generated art goes in `fishmod/art/`, hand-made sources in `fishmod/assets/`).
2. Add an entry to `"textures"`:

   ```json
   { "name": "gFishmodSomethingTex", "src": "fishmod/art/something.png", "format": "ci4",
     "out": "src/overlays/actors/ovl_En_Fishmod/fishmod_assets.inc.c" }
   ```

3. `#include` the `out` file in one C file, and `extern u64 gFishmodSomethingTex[];` wherever else it's used.

Formats: `rgba32`, `rgba16`, `ia16`, `ia8`, `ia4`, `i8`, `i4`, `ci8`, `ci4`. For `ci8`/`ci4`, the palette is
written as `<name>TLUT` (RGBA16), and the image is quantized to 256 / 16 colours if it has more.

Pick the smallest format that looks right: it's less memory and less RDP time for loading the texture.

- Coloured detail: `ci4` (16 colours), then `ci8`, then `rgba16`.
- Greyscale shapes tinted by the combiner: `i4`/`ia4`.
- `rgba32` only where vanilla uses it (item icons).

The tool refuses textures bigger than TMEM (4 KB, or 2 KB for palette textures), and rows that aren't a multiple
of 64 bits. It warns about sizes that aren't a power of two.

## Adding a model

Export a Wavefront OBJ (Blender: *File → Export → Wavefront*, triangulated or not; quads and polygons are fanned
into triangles). Then add an entry to `"models"`:

```json
{ "name": "gFishmodRockDL", "src": "fishmod/assets/rock.obj", "out": "src/.../fishmod_assets.inc.c",
  "scale": 10.0, "texture": "gFishmodRockTex", "color": [200, 200, 200], "lighting": false }
```

- `scale`: OBJ units to world units. Positions must fit in 16 bits after scaling.
- `texture` (optional): a texture from the same manifest. The display list loads it and maps the OBJ's UVs onto it.
- Vertex colours come from `v x y z r g b` lines when the OBJ has them (Blender's colour attribute export), and
  from `color` otherwise. With `lighting: true` the OBJ's normals are written instead, for lit materials.

The output is `<name>Vtx[]` and the display list `<name>[]`: it holds vertex loads of at most 32 (the F3DEX2
cache, so it also runs under F3DEX3) and triangles. The material (combiner, render mode) is still set by the code
that draws it, as with vanilla actors.

For anything bigger than props (new scenes, rigged models), use Fast64 in Blender. This tool is for the small
things that don't need a full Blender pipeline.

## Checking

`fishmod/tools/assets.py --check` reports problems and generated files that are out of date, without writing
anything. Generated files are committed, so builds work without Pillow; `build.sh` skips the step (with a note) if
Pillow isn't installed.

## Current assets

| Asset | Source | Made by |
|---|---|---|
| Bait bottle icons (larvae 3/2/1), `rgba32` | `fishmod/art/bottle_larva_*.png` | `fishmod/tools/gen_bait_icons.py` (from the vanilla bottle icon) |
