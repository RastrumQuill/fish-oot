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
    # the Fishing Pond's own reed (one tall see-through card, swaying), in stands along the reed banks
    "pond_reed": ((-6, 30), 0.02, {13: 0.55, 4: 0.45, 11: 0.12, 6: 0.12, 2: 0.1}, (4, 9), (0.75, 1.0), False),
    "cattails": ((-4, 22), 0.02, {13: 0.25, 4: 0.25, 11: 0.1}, (2, 5), (0.8, 1.2), False),
    "lily_pad": ((18, 95), 0.012, {4: 0.6, 13: 0.5, 11: 0.12, 6: 0.1}, (3, 8), (0.45, 0.85), False),
    "float_leaves": ((12, 70), 0.01, {4: 0.15, 13: 0.15, 12: 0.06}, (1, 2), (0.9, 1.3), False),
    "grass_tuft": ((4, 260), 0.10, {3: 0.1, 12: 0.12, 6: 0.12, 11: 0.12}, (2, 5), (0.8, 1.6), False),
    "ribbon_weed": ((25, 220), 0.05, {6: 0.3, 12: 0.3, 11: 0.2, 3: 0.08, 1: 0.2}, (3, 7), (0.8, 1.5), False),
    "broadleaf": ((20, 160), 0.03, {4: 0.25, 12: 0.2, 13: 0.2}, (2, 4), (0.8, 1.4), False),
    "bush_weed": ((60, 290), 0.03, {12: 0.1, 6: 0.1, 7: 0.15}, (2, 4), (0.8, 1.5), False),
    "tall_weed": ((110, 340), 0.0, {}, (3, 7), (0.8, 1.3), True),
    "rock": ((-10, 600), 0.006, {5: 0.35, 14: 0.12, 1: 0.25, 9: 0.1}, (1, 4), (0.15, 0.6), False),
    "pebble_patch": ((8, 60), 0.0, {1: 0.4, 3: 0.05, 15: 0.05}, (1, 3), (0.6, 1.2), False),
    # Wood: the lake's one tree is the dead tree on the main island, so wood is only what has fallen from it, in the
    # water round the island (zone 16); no drowned or fallen trees
    "sunken_log": ((6, 260), 0.0, {16: 0.3}, (1, 1), (0.4, 0.65), False),
    "branch": ((6, 260), 0.0, {16: 0.45}, (1, 2), (0.5, 0.9), False),
    "rush_tuft": ((-4, 20), 0.01, {13: 0.1, 4: 0.1, 6: 0.1, 11: 0.1}, (2, 5), (0.8, 1.3), False),
    "iris": ((-4, 18), 0.015, {4: 0.2, 13: 0.2, 6: 0.08, 11: 0.06}, (2, 4), (0.8, 1.2), False),
    "marigold": ((-6, 8), 0.01, {4: 0.12, 13: 0.12, 11: 0.08}, (1, 3), (0.8, 1.2), False),
    "big_pondweed": ((30, 180), 0.01, {12: 0.12, 6: 0.12, 4: 0.1}, (2, 4), (0.8, 1.3), False),
    "spatterdock": ((15, 80), 0.0, {4: 0.2, 13: 0.2, 6: 0.08}, (2, 5), (0.8, 1.3), False),
    "kelp_frond": ((150, 420), 0.0, {7: 0.2, 9: 0.1}, (3, 6), (0.8, 1.3), True),
    "vine_strand": ((60, 260), 0.01, {12: 0.06, 6: 0.06, 10: 0.06}, (2, 4), (0.8, 1.3), False),
    "vine_curtain": ((20, 200), 0.0, {14: 0.08, 15: 0.06, 2: 0.06}, (1, 3), (0.8, 1.2), False),
    "bank_roots": ((0, 60), 0.0, {10: 0.05, 11: 0.05, 6: 0.05, 13: 0.04}, (1, 2), (0.8, 1.3), True),
    "fan_weed": ((300, 1100), 0.0, {9: 0.3, 8: 0.01}, (1, 3), (0.8, 1.4), False),
    "boulder": ((-20, 700), 0.0, {5: 0.06, 14: 0.08, 7: 0.03, 1: 0.03, 9: 0.05}, (1, 2), (0.9, 2.2), False),
    "old_posts": ((5, 200), 0.0, {2: 0.08}, (1, 2), (0.8, 1.2), False),  # old pilings by the lab's shore
    "leaf_litter": ((10, 120), 0.005, {11: 0.05, 13: 0.04, 4: 0.04}, (1, 1), (0.8, 1.3), False),
}
# How tall each underwater kind is at scale 1 (gen_habitat.py): scaled down where the water is shallower, so nothing
# meant to be under water pokes out of it. Emergent kinds (reeds, rushes) are meant to stand above it.
HEIGHT = {"grass_tuft": 16, "ribbon_weed": 80, "tall_weed": 190, "broadleaf": 40, "bush_weed": 42, "kelp_frond": 260,
          "vine_strand": 150, "big_pondweed": 52, "bank_roots": 70, "fan_weed": 44,
          "old_posts": 60, "branch": 30, "sunken_log": 30, "rock": 20, "boulder": 20}
# how far a prop spreads from its middle (patches spread wider): the bed is checked all round it
FOOTPRINT = {"grass_tuft": 40, "ribbon_weed": 32, "bush_weed": 50, "tall_weed": 42, "broadleaf": 22,
             "big_pondweed": 30, "kelp_frond": 15, "vine_strand": 12, "fan_weed": 22, "bank_roots": 35}
# props that lie along the bed, tilted to its slope: half their length (along the model's x) and half their width
TILTED = {"sunken_log": (150, 15), "branch": (70, 6)}


def lie_on_bed(floor_at, x, z, surface, here, yaw, half_len, half_wid):
    """Where a long prop rests and how it tilts: the bed under both ends and both sides. Returns (y, (tilt about its
    width, tilt about its length)) in radians, as the game builds the matrix (fishmod_scatter.c)."""
    c, s = math.cos(yaw), math.sin(yaw)
    ax, az = c, -s  # the model's x axis, turned (its length)
    bx, bz = s, c   # the model's z axis, turned (its width)
    ends = [floor_at(x + ax * d, z + az * d, surface) for d in (-half_len, half_len)]
    sides = [floor_at(x + bx * d, z + bz * d, surface) for d in (-half_wid, half_wid)]
    ends = [here if e is None else e for e in ends]
    sides = [here if e is None else e for e in sides]
    tz = math.atan2(ends[1] - ends[0], 2 * half_len)
    tx = -math.atan2(sides[1] - sides[0], 2 * half_wid)
    clamp = math.radians(40)
    return (min(ends + [here]) + (sum(ends) / 2 - min(ends + [here])) * 0.8,
            (max(-clamp, min(clamp, tx)), max(-clamp, min(clamp, tz))))


FLAT_KINDS = {"pebble_patch", "leaf_litter"}  # flat on the bed
FLOATING = {"lily_pad", "float_leaves", "spatterdock", "vine_curtain"}  # sit on the surface, not the bed
# Leaves floating on the surface lie side by side, not on top of each other: each one's round footprint (radius,
# and its centre's offset in the model: vanilla's lily pad sits off its origin) at scale 1, and how far apart a
# cluster's leaves are scattered. Two may touch, overlapping a little at the edges, no more.
FLOAT_SPACE = {"lily_pad": (58.0, -32.0, -6.0), "spatterdock": (31.0, 0.0, 0.0), "float_leaves": (35.0, 0.0, 0.0)}
FLOAT_SPREAD = {"lily_pad": 45.0, "spatterdock": 32.0, "float_leaves": 32.0}
FLOAT_OVERLAP = 0.95


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

    # places kept clear of props (a zone's "exclude" shapes: rect x0 z0 x1 z1, or circle x z r)
    excludes = [sh for zn in zones for sh in zn.get("exclude", [])]

    def excluded(x, z):
        for sh in excludes:
            if sh[0] == "rect" and sh[1] <= x <= sh[3] and sh[2] <= z <= sh[4]:
                return True
            if sh[0] == "circle" and (x - sh[1]) ** 2 + (z - sh[2]) ** 2 <= sh[3] ** 2:
                return True
        return False

    def surface_at(x, z):
        return next((b["y"] for b in boxes if b["xMin"] <= x <= b["xMin"] + b["xLen"] and
                     b["zMin"] <= z <= b["zMin"] + b["zLen"]), None)

    def floor_at(x, z, surf):
        return finder.floor(int(x), int(z), surf + 60)

    out = []
    # the floating leaves placed so far, by 64-unit cell: (centre x, centre z, radius)
    floats = {}

    def leaf_circle(kind, x, z, yaw, scale):
        r, ox, oz = FLOAT_SPACE[kind]
        c, s_ = math.cos(yaw), math.sin(yaw)
        # as the game turns the model: x' = x cos + z sin, z' = -x sin + z cos
        return (x + (ox * c + oz * s_) * scale, z + (-ox * s_ + oz * c) * scale, r * scale)

    def leaf_free(circle):
        cx, cz, r = circle
        gx, gz = int(cx // 64), int(cz // 64)
        for ix in range(gx - 2, gx + 3):
            for iz in range(gz - 2, gz + 3):
                for ox, oz, orr in floats.get((ix, iz), ()):
                    if (cx - ox) ** 2 + (cz - oz) ** 2 < ((r + orr) * FLOAT_OVERLAP) ** 2:
                        return False
        return True

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
                if kind in ("reed_clump", "pond_reed", "cattails", "rush_tuft", "iris", "marigold"):
                    chance *= 1.0 - min(1.0, open_shore * 3.0)
                    # standing out of the water only on gentle banks: at the foot of a cliff or the island's steep
                    # shelf they read as grass stuck on the wall
                    chance *= max(0.0, min(1.0, (0.6 - slope) / 0.3))
                if steep:
                    chance += 0.35 * max(0.0, min(1.0, (slope - 0.6) / 0.8))
                # patchy: dense in some places, none in others
                chance *= max(0.0, patch[kind](x, z) * 2.2 - 0.6)
                if rnd.random() >= chance:
                    continue
                n = rnd.randint(*cluster)
                spread = FLOAT_SPREAD.get(kind, 14.0)
                if kind in FLOAT_SPACE:
                    n *= 6  # leaves that would overlap are skipped: more tries, so the bed still fills in
                for k in range(n):
                    px = x + (rnd.gauss(0, spread) if k else 0)
                    pz = z + (rnd.gauss(0, spread) if k else 0)
                    ps = surface_at(px, pz)
                    if ps is None or excluded(px, pz):
                        continue
                    pf = floor_at(px, pz, ps)
                    if pf is None:
                        continue
                    pd = ps - pf
                    if not (drange[0] - 4 <= pd <= drange[1] + 10):
                        continue
                    # open water above the bed: not under an overhang, inside a bank or beside a waterfall (the
                    # bridges are the exception)
                    top = finder.floor(int(px), int(pz), 100000)
                    if top is not None and top > pf + 4 and max(zin.get(5, 0.0), zin.get(10, 0.0)) == 0.0:
                        continue
                    # flat patches (pebbles, leaf litter) only on flat ground, not hanging over an edge
                    if kind in FLAT_KINDS:
                        r = 50 * scale[1]
                        corners = [floor_at(px + dx, pz + dz, ps) for dx, dz in ((r, 0), (-r, 0), (0, r), (0, -r))]
                        if any(c is None or abs(c - pf) > 6 for c in corners):
                            continue
                    y = (ps + 1) if kind in FLOATING else pf  # floating: a unit above the water, not z-fighting it
                    sc = rnd.uniform(*scale)
                    yaw = rnd.uniform(0, 2 * math.pi)
                    tilt = (0.0, 0.0)
                    if kind in HEIGHT:
                        # stay a little under the surface (rocks and logs may show in the shallows), all over the
                        # prop's footprint: on a slope a patch's shallow side is what pokes out
                        shallowest = pf
                        r = FOOTPRINT.get(kind, 20)
                        for fx, fz in ((r, 0), (-r, 0), (0, r), (0, -r)):
                            fy = floor_at(px + fx, pz + fz, ps)
                            if fy is not None:
                                shallowest = max(shallowest, fy)
                        room = (ps - shallowest) - (2 if kind in ("rock", "boulder", "sunken_log", "branch") else 8)
                        sc = min(sc, room / HEIGHT[kind])
                        if sc < 0.35:
                            continue
                    if kind in TILTED:
                        # lying along the bed: tilted to its slope along the length and across it
                        y, tilt = lie_on_bed(floor_at, px, pz, ps, pf, yaw, TILTED[kind][0] * sc, TILTED[kind][1] * sc)
                        if max(abs(tilt[0]), abs(tilt[1])) > math.radians(25):
                            continue  # too steep: it would have rolled further down
                    if kind in FLOAT_SPACE:
                        circle = leaf_circle(kind, px, pz, yaw, sc)
                        if not leaf_free(circle):
                            continue  # it would lie on top of another leaf
                        floats.setdefault((int(circle[0] // 64), int(circle[1] // 64)), []).append(circle)
                    out.append({"kind": kind, "x": px, "y": y, "z": pz, "yaw": yaw, "tilt": tilt,
                                "scale": sc, "tint": rnd.uniform(0.82, 1.08)})
                break  # one rule per candidate: keeps kinds from stacking on one spot
    return out


def preview(scene, instances, outdir):
    from PIL import Image, ImageDraw
    ss = load_tool("style_sheet")
    zm = load_tool("zone_map")
    os.makedirs(outdir, exist_ok=True)
    folder, (x0, x1, z0, z1), _ = zm.SCENES[scene]
    colours = {"pond_reed": (240, 240, 120), "reed_clump": (220, 220, 80), "cattails": (200, 160, 60), "lily_pad": (60, 220, 60),
               "float_leaves": (120, 255, 120), "grass_tuft": (90, 140, 60), "ribbon_weed": (40, 120, 40),
               "broadleaf": (150, 200, 60), "bush_weed": (30, 90, 30), "tall_weed": (10, 60, 20),
               "rock": (200, 200, 200), "pebble_patch": (230, 210, 160), "sunken_log": (120, 70, 30),
               "branch": (150, 100, 60), "stump": (100, 60, 30),
               "old_posts": (180, 140, 100), "leaf_litter": (170, 90, 40), "boulder": (240, 240, 240)}
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
    meshes = {k: ss.load_obj(k) for k in RULES if k not in ("lily_pad", "rock", "boulder", "pond_reed")}
    meshes["pond_reed"] = ss.load_vanilla("gFishingReedModel", "gFishingLilyPadTex.rgba16.png", 32, 32)
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
        # patch kinds are several clumps per mesh: keep one in that many
        if kind in gh.PATCH and rnd.random() >= 1.0 / gh.PATCH[kind]:
            continue
        props.append((it["x"], it["y"], it["z"], kind, it["yaw"], it["scale"], it["tint"], it.get("tilt", (0.0, 0.0))))
        if kind == "lily_pad" and rnd.random() < 0.15:
            # a flower on some of the pads, near the pad's middle (vanilla's pad sits off its origin)
            r, cx, cz = FLOAT_SPACE["lily_pad"]
            c, sn = math.cos(it["yaw"]), math.sin(it["yaw"])
            ox = (cx * c + cz * sn) * it["scale"] + rnd.uniform(-4, 4)
            oz = (-cx * sn + cz * c) * it["scale"] + rnd.uniform(-4, 4)
            for part in ("lily_flower", "lily_centre"):
                props.append((it["x"] + ox, it["y"] + 0.5, it["z"] + oz, part, it["yaw"], 1.0, 1.0, (0.0, 0.0)))
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
    for x, y, z, kind, yaw, scale, tint, tilt in props:
        tx, tz = (int(round(t / (2 * math.pi) * 256)) for t in tilt)
        lines.append(f"        {{ {int(x)}, {int(y)}, {int(z)}, {kinds.index(kind)}, {int(yaw / (2 * math.pi) * 256) & 255}, "
                     f"{max(1, min(255, int(scale * 64)))}, {max(1, min(255, int(tint * 128)))}, {tx}, {tz} }},")
    lines += ["    },", "};", ""]
    os.makedirs(os.path.join(ROOT, "src", "fishmod_data"), exist_ok=True)
    with open(os.path.join(ROOT, "src", "fishmod_data", f"fishmod_sc_{base}.c"), "w") as f:
        f.write("\n".join(lines))
    print(f"{scene}: {n} props in {w}x{h} chunks of {CHUNK}, {(o_props + 12 * n) / 1024:.1f} KB")


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
