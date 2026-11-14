# Low-poly shapes for the RDP

What a shape costs the N64's RDP and how round it looks, measured (fishmod/tools/shape_study.py: each shape drawn from
24 directions with mesh_preview.py's rasterizer, back faces culled, costed with the console-calibrated model of
rdp_cost.py: about 1 us a triangle + 0.42 us a scanline + 26 ns a pixel; roundness from the outline's width over 180
directions, 1.0 a true circle; wobble, how much the outline's width changes up its height). Pictures:
[post-shapes.png](post-shapes.png).

Posts (80 tall, radius 5): triangles a model / drawn, RDP us at 150 and 600 units, roundness (narrowest outline / widest), wobble up the height
| shape | tris | drawn (near) | us near | us far | roundness | wobble |
|---|---|---|---|---|---|---|
| triangle prism | 6 | 2.8 | 155.3 | 27.3 | 0.866 | 0.000 |
| square prism | 8 | 3.7 | 195.4 | 32.9 | 0.707 | 0.000 |
| pentagon prism | 10 | 4.9 | 236.8 | 42.7 | 0.951 | 0.000 |
| hexagon prism | 12 | 5.5 | 268.7 | 48.0 | 0.866 | 0.000 |
| octagon prism | 16 | 7.7 | 340.1 | 57.7 | 0.927 | 0.000 |
| triangle, twisted 1/2 | 6 | 3.0 | 153.9 | 27.9 | 0.978 | 0.045 |
| square, twisted 1/2 | 8 | 3.8 | 195.7 | 31.5 | 0.828 | 0.064 |
| pentagon, twisted 1/2 | 10 | 4.9 | 234.0 | 40.1 | 0.991 | 0.015 |
| hexagon, twisted 1/2 | 12 | 5.5 | 264.1 | 47.9 | 0.921 | 0.027 |
| square, 4 rows offset | 32 | 15.7 | 210.1 | 41.3 | 0.883 | 0.062 |
| hexagon, 4 rows offset | 48 | 22.5 | 282.9 | 63.5 | 0.947 | 0.027 |
| camera-facing card | 2 | 2.0 | 130.1 | 26.4 | 1.000 (always faces you) | 0.000 |

Balls (radius 30, for rocks):
| shape | tris | drawn | us near | us far | roundness |
|---|---|---|---|---|---|
| octahedron | 8 | 3.8 | 97.1 | 18.1 | 0.718 |
| uv 6x4 | 36 | 15.8 | 205.0 | 46.9 | 0.866 |
| icosahedron | 20 | 8.9 | 159.4 | 32.9 | 0.871 |

What it says:
- **For a tall thin post the cost is its visible full-height triangles** (each pays every scanline it covers): about
  half the sides, two triangles each. Fewer sides is cheaper almost in proportion. Hidden length (buried in the bed,
  inside other geometry) costs as much as visible length: trim it first.
- **Odd side counts are much rounder than even ones.** An odd polygon's width hardly changes as it turns (a pentagon:
  95%, against an octagon's 93% and a hexagon's 87%). A triangle prism is as round as a hexagon at half the triangles.
- **Turning one end by half a side (the antiprism) is free roundness**: the same triangles, the outline of twice the
  sides. It works best on odd counts: a twisted triangle (6 triangles) 98%, rounder than an octagon (16); a twisted
  pentagon (10) 99%. The cost: a slight waist up the height (wobble 2-6%) and, close up, a diagonal crease in the
  shading (the twisted triangle shows it most).
- **Smooth shading hides the facets either way**: vertex colours baked from radial normals make every one of them read
  round in its shading; only the outline shows the side count, and only up close.
- **A camera-facing card is no cheaper than a 3-sided post** for a thin upright (two full-height triangles either way):
  cards pay off only for shapes with many triangles (plants: the impostors).
- **Balls (rocks):** an icosahedron (20 triangles) is as round as a 6x4 UV sphere (36) at three quarters of the cost;
  an octahedron (8) is visibly a diamond.

Sources on the techniques (practitioners' notes, not measurements):
[twisting cylinders to increase perceived segment count](https://blenderartists.org/t/twitsing-cylinders-to-increase-percived-segment-count/1381275),
[offset rows and odd counts](https://polycount.com/discussion/comment/1125584),
[custom normals for low-poly rounding](https://docs.blender.org/manual/en/2.79/modeling/meshes/editing/normals.html),
[the RDP's per-triangle setup](https://ultra64.ca/files/documentation/online-manuals/man-v5-1/kantan/step2/4/4_2.htm).
