#!/usr/bin/env python3
"""Places habitat props across a scene by rules (fishmod/docs/v5-plan.md, phase 3), at build time.

Real lakes band from shore to deep (emergent reeds, floating-leaved lilies, submerged weed, a weed edge at the drop
-off, then bare bottom: fishmod/docs/research/visual-references.md); the zones (fishmod/data/<scene>_zones.json) add
their own props (rocks at the points and ruins, wood in the coves and under the bridges, pebbles at the inflow).
Density is patchy (value noise) and plants grow in clusters, so it never looks like a grid.

Output: a list of instances (prop, position, yaw, scale, tint), used by the game's scatter (baked into chunks of
pre-transformed geometry, like vanilla's static scenery) and by the preview renders.

Usage: fishmod/tools/gen_scatter.py --preview DIR     write top-down and oblique preview renders
"""
import argparse
import importlib.util
import json
import math
import os
import random

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))


def load_tool(name):
    spec = importlib.util.spec_from_file_location(name, os.path.join(ROOT, "fishmod", "tools", name + ".py"))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


gw = load_tool("gen_watermap")

SCENES = {"SCENE_LAKE_HYLIA": "overworld/spot06"}
STEP = 36  # candidate spacing (world units), jittered


class Noise:
    """Smooth value noise for patchy density."""

    def __init__(self, seed, scale):
        self.rnd = random.Random(seed)
        self.scale = scale
        self.cache = {}

    def lattice(self, i, j):
        k = (i, j)
        if k not in self.cache:
            self.cache[k] = random.Random(hash((i, j, id(self))) & 0xFFFFFFFF).random()
        return self.cache[k]

    def __call__(self, x, z):
        gx, gz = x / self.scale, z / self.scale
        i, j = math.floor(gx), math.floor(gz)
        fx, fz = gx - i, gz - j
        fx, fz = fx * fx * (3 - 2 * fx), fz * fz * (3 - 2 * fz)
        a = self.lattice(i, j) * (1 - fx) + self.lattice(i + 1, j) * fx
        b = self.lattice(i, j + 1) * (1 - fx) + self.lattice(i + 1, j + 1) * fx
        return a * (1 - fz) + b * fz


def in_zone(zone, x, z):
    """How far inside a zone the point is: 1 at the middle, 0 at the edge and outside (zones blend)."""
    sh = zone["shape"]
    if isinstance(sh, str):
        return 0.0
    if sh[0] == "circle":
        d = math.hypot(x - sh[1], z - sh[2]) / sh[3]
    else:
        ax, az, bx, bz, r = sh[1:]
        vx, vz = bx - ax, bz - az
        t = max(0.0, min(1.0, ((x - ax) * vx + (z - az) * vz) / (vx * vx + vz * vz)))
        d = math.hypot(x - (ax + vx * t), z - (az + vz * t)) / r
    return max(0.0, 1.0 - d) ** 0.7


# Rules: prop -> (depth range (units below the surface), base chance, zone boosts {zone id: extra chance},
# cluster size range, scale range, needs steep slope)
RULES = {
    "reed_clump": ((-6, 28), 0.02, {13: 0.5, 4: 0.35, 6: 0.15, 11: 0.1}, (2, 6), (0.8, 1.3), False),
    "cattails": ((-4, 22), 0.02, {13: 0.25, 4: 0.25, 11: 0.1}, (2, 5), (0.8, 1.2), False),
    "lily_pad": ((18, 95), 0.012, {4: 0.6, 13: 0.5, 11: 0.12, 6: 0.1}, (3, 8), (0.45, 0.85), False),
    "float_leaves": ((12, 70), 0.01, {4: 0.15, 13: 0.15, 12: 0.06}, (1, 2), (0.9, 1.3), False),
    "grass_tuft": ((4, 260), 0.10, {3: 0.1, 12: 0.12, 6: 0.12, 11: 0.12}, (2, 5), (0.8, 1.6), False),
    "ribbon_weed": ((25, 220), 0.05, {6: 0.3, 12: 0.3, 11: 0.2, 3: 0.08, 1: 0.2}, (3, 7), (0.8, 1.5), False),
    "broadleaf": ((20, 160), 0.03, {4: 0.25, 12: 0.2, 13: 0.2}, (2, 4), (0.8, 1.4), False),
    "bush_weed": ((60, 290), 0.03, {12: 0.1, 6: 0.1, 7: 0.15}, (2, 4), (0.8, 1.5), False),
    "tall_weed": ((110, 340), 0.0, {}, (3, 7), (0.8, 1.3), True),
    "rock": ((-10, 600), 0.006, {5: 0.35, 14: 0.4, 1: 0.25, 9: 0.1}, (1, 4), (0.15, 0.6), False),
    "pebble_patch": ((-4, 60), 0.0, {1: 0.4, 3: 0.05, 15: 0.05}, (1, 3), (0.6, 1.2), False),
    "sunken_log": ((25, 260), 0.002, {6: 0.05, 10: 0.06, 2: 0.04, 11: 0.04}, (1, 1), (0.7, 1.1), False),
    "branch": ((20, 260), 0.004, {6: 0.06, 10: 0.08, 11: 0.06, 13: 0.04}, (1, 2), (0.7, 1.2), False),
    "boulder": ((-20, 700), 0.0, {5: 0.06, 14: 0.08, 7: 0.03, 1: 0.03, 9: 0.05}, (1, 2), (0.9, 2.2), False),
    "fallen_tree": ((60, 320), 0.0, {6: 0.012, 10: 0.015, 11: 0.012, 12: 0.006}, (1, 1), (0.8, 1.2), False),
    "drowned_tree": ((60, 300), 0.0, {10: 0.03, 11: 0.03, 14: 0.02}, (1, 1), (0.8, 1.3), False),
    "old_posts": ((5, 200), 0.0, {14: 0.12, 15: 0.15, 2: 0.08}, (1, 2), (0.8, 1.2), False),
    "leaf_litter": ((10, 120), 0.005, {11: 0.05, 13: 0.04, 4: 0.04}, (1, 1), (0.8, 1.3), False),
}
FLOATING = {"lily_pad", "float_leaves"}  # sit on the surface, not the bed


def place(scene, seed=1):
    folder = SCENES[scene]
    d = os.path.join(gw.SCENES, folder)
    vtx = gw.parse_vtx(gw.one(d, "*_VtxList.inc.c"))
    finder = gw.FloorFinder(gw.parse_floors(gw.one(d, "*_PolyList.inc.c"), vtx))
    boxes = gw.parse_waterboxes(gw.one(d, "*_WaterBoxes.inc.c"))
    zones = json.load(open(os.path.join(ROOT, "fishmod", "data", scene.lower().replace("scene_", "") + "_zones.json")))
    zones = zones["zones"]
    rnd = random.Random(seed)
    patch = {k: Noise(seed * 31 + i, 420.0) for i, k in enumerate(RULES)}

    def surface_at(x, z):
        return next((b["y"] for b in boxes if b["xMin"] <= x <= b["xMin"] + b["xLen"] and
                     b["zMin"] <= z <= b["zMin"] + b["zLen"]), None)

    def floor_at(x, z, surf):
        return finder.floor(int(x), int(z), surf + 60)

    out = []
    xs = [b["xMin"] for b in boxes] + [b["xMin"] + b["xLen"] for b in boxes]
    zs = [b["zMin"] for b in boxes] + [b["zMin"] + b["zLen"] for b in boxes]
    for gz in range(min(zs), max(zs), STEP):
        for gx in range(min(xs), max(xs), STEP):
            x = gx + rnd.uniform(0, STEP)
            z = gz + rnd.uniform(0, STEP)
            surf = surface_at(x, z)
            if surf is None:
                continue
            f = floor_at(x, z, surf)
            if f is None:
                continue
            depth = surf - f
            # slope: how fast the floor falls nearby
            fs = [floor_at(x + dx, z + dz, surf) for dx, dz in ((24, 0), (-24, 0), (0, 24), (0, -24))]
            fs = [v for v in fs if v is not None]
            slope = (max(fs) - min(fs)) / 48.0 if fs else 0.0
            zin = {zn["id"]: in_zone(zn, x, z) for zn in zones}
            # the beach and the dock stay open for fishing (sand, not reeds)
            open_shore = max(zin.get(3, 0.0), zin.get(15, 0.0))
            for kind, (drange, base, boosts, cluster, scale, steep) in RULES.items():
                if not (drange[0] <= depth <= drange[1]):
                    continue
                chance = base + sum(b * zin.get(zid, 0.0) for zid, b in boosts.items())
                if kind in ("reed_clump", "cattails"):
                    chance *= 1.0 - min(1.0, open_shore * 3.0)
                if steep:
                    chance += 0.35 * max(0.0, min(1.0, (slope - 0.6) / 0.8))
                # patchy: dense in some places, none in others
                chance *= max(0.0, patch[kind](x, z) * 2.2 - 0.6)
                if rnd.random() >= chance:
                    continue
                n = rnd.randint(*cluster)
                for k in range(n):
                    px = x + (rnd.gauss(0, 14) if k else 0)
                    pz = z + (rnd.gauss(0, 14) if k else 0)
                    ps = surface_at(px, pz)
                    if ps is None:
                        continue
                    pf = floor_at(px, pz, ps)
                    if pf is None:
                        continue
                    pd = ps - pf
                    if not (drange[0] - 4 <= pd <= drange[1] + 10):
                        continue
                    y = ps if kind in FLOATING else pf
                    out.append({"kind": kind, "x": px, "y": y, "z": pz, "yaw": rnd.uniform(0, 2 * math.pi),
                                "scale": rnd.uniform(*scale), "tint": rnd.uniform(0.82, 1.08)})
                break  # one rule per candidate: keeps kinds from stacking on one spot
    return out


def preview(scene, instances, outdir):
    from PIL import Image, ImageDraw
    ss = load_tool("style_sheet")
    zm = load_tool("zone_map")
    os.makedirs(outdir, exist_ok=True)
    folder, (x0, x1, z0, z1), _ = zm.SCENES[scene]
    colours = {"reed_clump": (220, 220, 80), "cattails": (200, 160, 60), "lily_pad": (60, 220, 60),
               "float_leaves": (120, 255, 120), "grass_tuft": (90, 140, 60), "ribbon_weed": (40, 120, 40),
               "broadleaf": (150, 200, 60), "bush_weed": (30, 90, 30), "tall_weed": (10, 60, 20),
               "rock": (200, 200, 200), "pebble_patch": (230, 210, 160), "sunken_log": (120, 70, 30),
               "branch": (150, 100, 60), "stump": (100, 60, 30), "drowned_tree": (80, 50, 20),
               "old_posts": (180, 140, 100), "leaf_litter": (170, 90, 40), "boulder": (240, 240, 240),
               "fallen_tree": (60, 40, 20)}
    s = 0.12
    img = Image.new("RGB", (int((x1 - x0) * s), int((z1 - z0) * s)), (30, 50, 70))
    d = ImageDraw.Draw(img)
    for it in instances:
        px, pz = (it["x"] - x0) * s, (it["z"] - z0) * s
        d.point((px, pz), fill=colours[it["kind"]])
    img.save(os.path.join(outdir, "scatter_top.png"))

    # oblique views of a few areas, rendered with the props' real meshes
    pad = ss.load_vanilla("gFishingLilyPadModel", "gFishingLilyPadTex.rgba16.png", 32, 32)
    rock = ss.load_vanilla("gFishingRockModel", "gFishingRockTex.rgba16.png", 32, 32)
    meshes = {k: ss.load_obj(k) for k in RULES if k not in ("lily_pad", "rock", "boulder")}
    meshes["lily_pad"] = pad
    meshes["rock"] = rock
    meshes["boulder"] = rock
    views = {"north_beach": (-650, 3500), "pond_cove": (1650, 4350), "west_flat": (-3900, 5600),
             "island": (-900, 7150), "dropoff": (-1500, 5200)}
    for name, (cx, cz) in views.items():
        near = [it for it in instances if abs(it["x"] - cx) < 420 and abs(it["z"] - cz) < 420]
        models = []
        for it in near:
            m = meshes[it["kind"]]
            models.append(m.moved((it["x"], it["y"], it["z"]), it["scale"], it["yaw"]))
        if not models:
            continue
        lo, hi = ss.bounds(models)
        cy = (lo[1] + hi[1]) / 2
        im = ss.render(models, 640, 400, (cx + 380, cy + 330, cz + 520), (cx, cy - 40, cz), 50)
        im.save(os.path.join(outdir, f"scatter_{name}.png"))


CHUNK = 384


def write_c(scene, instances):
    """The scene's props for the game: src/fishmod_data/fishmod_sc_<scene folder>.c (generated at build time from
    the extracted scene, not committed)."""
    gh = load_tool("gen_habitat")
    kinds = [k for k, _, _ in gh.PROP_KINDS] + gh.VANILLA_KINDS
    folder = SCENES[scene]
    base = os.path.basename(folder)
    d = os.path.join(gw.SCENES, folder)
    boxes = gw.parse_waterboxes(gw.one(d, "*_WaterBoxes.inc.c"))
    rnd = random.Random(7)
    props = []
    for it in instances:
        kind = {"boulder": "rock"}.get(it["kind"], it["kind"])
        props.append((it["x"], it["y"], it["z"], kind, it["yaw"], it["scale"], it["tint"]))
        if kind == "lily_pad" and rnd.random() < 0.15:
            # a flower on some of the pads
            ox, oz = rnd.uniform(-4, 4), rnd.uniform(-4, 4)
            for part in ("lily_flower", "lily_centre"):
                props.append((it["x"] + ox, it["y"] + 0.5, it["z"] + oz, part, it["yaw"], 1.0, 1.0))
    x0 = int(min(p[0] for p in props)) - 1
    z0 = int(min(p[2] for p in props)) - 1
    w = int((max(p[0] for p in props) - x0) // CHUNK) + 1
    h = int((max(p[2] for p in props) - z0) // CHUNK) + 1

    def chunk_of(p):
        return int((p[2] - z0) // CHUNK) * w + int((p[0] - x0) // CHUNK)

    props.sort(key=lambda p: (chunk_of(p), kinds.index(p[3])))
    starts = []
    i = 0
    for c in range(w * h):
        starts.append(i)
        while i < len(props) and chunk_of(props[i]) == c:
            i += 1
    starts.append(len(props))
    surfaces = []
    for c in range(w * h):
        cx, cz = x0 + (c % w + 0.5) * CHUNK, z0 + (c // w + 0.5) * CHUNK
        surf = next((b["y"] for b in boxes if b["xMin"] <= cx <= b["xMin"] + b["xLen"] and
                     b["zMin"] <= cz <= b["zMin"] + b["zLen"]), -32768)
        surfaces.append(surf)
    n = len(props)
    hdr = 0x1C
    o_start = hdr
    o_surf = o_start + 2 * (w * h + 1)
    o_props = o_surf + 2 * (w * h)
    lines = ["// Generated by fishmod/tools/gen_scatter.py from the scene. Do not edit.",
             '#include "fishmod_scatter.h"', "",
             "typedef struct {", "    FishmodScatterHeader header;", f"    u16 chunkStart[{w * h + 1}];",
             f"    s16 chunkSurface[{w * h}];", f"    FishmodProp props[{n}];", f"}} FishmodScatter_{base};", "",
             f"FishmodScatter_{base} gFishmodScatter_{base} = {{",
             f"    {{ FISHMOD_SCATTER_MAGIC, {x0}, {z0}, {CHUNK}, {w}, {h}, {n}, {o_start}, {o_surf}, {o_props} }},",
             "    { " + ", ".join(str(v) for v in starts) + " },",
             "    { " + ", ".join(str(v) for v in surfaces) + " },", "    {"]
    for x, y, z, kind, yaw, scale, tint in props:
        lines.append(f"        {{ {int(x)}, {int(y)}, {int(z)}, {kinds.index(kind)}, {int(yaw / (2 * math.pi) * 256) & 255}, "
                     f"{max(1, min(255, int(scale * 64)))}, {max(1, min(255, int(tint * 128)))} }},")
    lines += ["    },", "};", ""]
    os.makedirs(os.path.join(ROOT, "src", "fishmod_data"), exist_ok=True)
    with open(os.path.join(ROOT, "src", "fishmod_data", f"fishmod_sc_{base}.c"), "w") as f:
        f.write("\n".join(lines))
    print(f"{scene}: {n} props in {w}x{h} chunks of {CHUNK}, {(o_props + 10 * n) / 1024:.1f} KB")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--scene", default="SCENE_LAKE_HYLIA")
    ap.add_argument("--preview", default=None)
    ap.add_argument("--c", action="store_true", help="write the game's data (src/fishmod_data/)")
    args = ap.parse_args()
    inst = place(args.scene)
    counts = {}
    for it in inst:
        counts[it["kind"]] = counts.get(it["kind"], 0) + 1
    print(f"{len(inst)} props:", ", ".join(f"{k} {v}" for k, v in sorted(counts.items(), key=lambda kv: -kv[1])))
    if args.preview:
        preview(args.scene, inst, args.preview)
    if args.c:
        write_c(args.scene, inst)


if __name__ == "__main__":
    main()
