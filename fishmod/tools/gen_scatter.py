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
        self.seed = seed
        self.scale = scale
        self.cache = {}

    def lattice(self, i, j):
        k = (i, j)
        if k not in self.cache:
            # from the seed and the cell only (not the object's address or Python's hashing), so every build places
            # the same props
            self.cache[k] = random.Random((self.seed * 73856093) ^ (i * 19349663) ^ (j * 83492791)).random()
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
    "lily_pad": ((18, 130), 0.012, {4: 0.95, 13: 0.5, 11: 0.12, 6: 0.1}, (3, 8), (0.45, 0.85), False),
    "float_leaves": ((12, 70), 0.01, {4: 0.15, 13: 0.15, 12: 0.06}, (1, 2), (0.9, 1.3), False),
    # (the bed plants: fewer clumps, more and bigger plants in each, so beds read as beds with sand between them)
    "grass_tuft": ((4, 260), 0.035, {3: 0.04, 12: 0.05, 6: 0.05, 11: 0.05}, (4, 8), (1.0, 1.8), False),
    "ribbon_weed": ((25, 220), 0.02, {6: 0.13, 12: 0.13, 11: 0.09, 3: 0.035, 1: 0.09}, (5, 9), (1.0, 1.6), False),
    "broadleaf": ((20, 160), 0.018, {4: 0.045, 12: 0.085, 13: 0.085}, (3, 6), (0.85, 1.25), False),
    "bush_weed": ((60, 290), 0.014, {12: 0.042, 6: 0.042, 7: 0.07}, (3, 5), (0.9, 1.35), False),
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
    "kelp_frond": ((150, 420), 0.0, {7: 0.1, 9: 0.05}, (3, 6), (1.0, 1.4), True),
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
HEIGHT = {"grass_tuft": 16, "ribbon_weed": 80, "tall_weed": 140, "broadleaf": 40, "bush_weed": 42, "kelp_frond": 180,
          "vine_strand": 150, "big_pondweed": 52, "bank_roots": 70, "fan_weed": 44,
          "old_posts": 60, "branch": 30, "sunken_log": 30, "rock": 20, "boulder": 20}
# How far apart a clump's plants are scattered (the rest: 14): tight, so a clump reads as one plant mass
CLUSTER_SPREAD = {"grass_tuft": 11.0, "ribbon_weed": 12.0, "broadleaf": 12.0, "bush_weed": 15.0, "big_pondweed": 13.0}
# The bed plants are lushest where the camera spends its time, the shallows by the shores, and thin out toward the
# deep middle, which is mostly seen far off through the surface: their chance, by depth below the surface
BED_KINDS = {"grass_tuft", "ribbon_weed", "broadleaf", "bush_weed", "big_pondweed", "vine_strand"}


def depth_lushness(depth):
    if depth <= 140:
        return 1.2
    if depth <= 320:
        return 1.2 - 0.5 * (depth - 140) / 180
    return 0.7


# how far a prop spreads from its middle (patches spread wider): the bed is checked all round it
FOOTPRINT = {"grass_tuft": 40, "ribbon_weed": 32, "bush_weed": 50, "tall_weed": 42, "broadleaf": 22,
             "big_pondweed": 30, "kelp_frond": 15, "vine_strand": 12, "fan_weed": 22, "bank_roots": 35}
# props that lie along the bed, tilted to its slope: half their length (along the model's x) and half their width
TILTED = {"sunken_log": (150, 15), "branch": (70, 6)}
# Rocks (all one vanilla model) get variety for free: stretched along each axis by a hash of where they stand (the
# game computes the same, fishmod_scatter.c Scatter_RockStretch: no per-prop data) and tilted a little at random
ROCK_KINDS = ("rock", "boulder")
ROCK_TILT = math.radians(12)
ROCK_RADIUS = 66  # the vanilla rock's reach from its middle at scale 1


def rock_stretch(x, z):
    """(x, y, z) stretch of the rock standing at integer (x, z), as Scatter_RockStretch."""
    m = 0xFFFFFFFF
    h = ((x & m) * 0x9E3779B1 & m) ^ ((z & m) * 0x85EBCA77 & m)
    h ^= h >> 15
    h = (h * 0x2C1B3C6D) & m
    h ^= h >> 12
    return (0.75 + (h & 0xFF) * (0.6 / 255.0), 0.7 + ((h >> 8) & 0xFF) * (0.45 / 255.0),
            0.75 + ((h >> 16) & 0xFF) * (0.6 / 255.0))


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


# the layout: one of many the rules can make (chosen by eye from several: the fullest)
SEED = 4


# The bed-skin test (FISHMOD_BEDSKIN=1, fishmod/build.sh --bedskin): the bed itself is coloured (gen_bedskin.py), so
# no moss mats; where the density cap or the thinning would make one, the plant just goes. Its own thinning list.
BEDSKIN = os.environ.get("FISHMOD_BEDSKIN") == "1"


def place(scene, seed=SEED):
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
                if kind in BED_KINDS:
                    chance *= depth_lushness(depth)
                # patchy: dense in some places, none in others (thick beds, open sand between; softer than before so the shallows
                # aren't bare in places)
                chance *= max(0.0, patch[kind](x, z) * 2.2 - 0.6)
                if rnd.random() >= chance:
                    continue
                n = rnd.randint(*cluster)
                spread = FLOAT_SPREAD.get(kind, CLUSTER_SPREAD.get(kind, 14.0))
                # a clump's own colour (each plant varies a little round it): beds vary clump to clump, like real ones
                clump_tint = rnd.uniform(0.88, 1.1)
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
                        tol = 6
                        corners = [floor_at(px + dx, pz + dz, ps) for dx, dz in ((r, 0), (-r, 0), (0, r), (0, -r))]
                        if any(c is None or abs(c - pf) > tol for c in corners):
                            continue
                    y = (ps + 1) if kind in FLOATING else pf  # floating: a unit above the water, not z-fighting it
                    sc = rnd.uniform(*scale)
                    yaw = rnd.uniform(0, 2 * math.pi)
                    tilt = (0.0, 0.0)
                    lift = 0.0  # how far a tilted rock's raised edge comes up, per unit of scale
                    if kind in ROCK_KINDS:
                        st = rock_stretch(int(px), int(pz))
                        # (its own random numbers, by where it stands: the scatter's sequence, and so every other prop,
                        # stays as it was)
                        trnd = random.Random(int(px) * 100003 + int(pz))
                        tilt = (trnd.uniform(-ROCK_TILT, ROCK_TILT), trnd.uniform(-ROCK_TILT, ROCK_TILT))
                        lift = ROCK_RADIUS * max(st[0], st[2]) * math.sin(max(abs(tilt[0]), abs(tilt[1])))
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
                        if kind in ROCK_KINDS:
                            # stretched taller or tilted: smaller, to stay as far under the surface (never dropped, so
                            # which props are placed doesn't change)
                            sc = min(sc, room / (HEIGHT[kind] * st[1] + lift))
                    if kind in ROCK_KINDS:
                        y = pf - lift * sc * 0.6  # sunk so the raised edge doesn't float off the bed
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
                                "scale": sc, "tint": clump_tint * rnd.uniform(0.95, 1.04)})
                break  # one rule per candidate: keeps kinds from stacking on one spot
    if not BEDSKIN:
        out += place_moss(seed, rnd_state=None, surface_at=surface_at, floor_at=floor_at, zones=zones,
                          excluded=excluded, boxes=boxes)
    BED[:] = [surface_at, floor_at]
    return out


# Moss mats (gen_habitat.py mesh_moss_patch: a low dome, its rim sunk in the sand): their own pass with their own
# random numbers, so changing them never moves anything else. On the bed in the shallows, in clusters, tilted to the
# bed's slope; not where the bed bends too much for a flat mat (it would float at one edge).
MOSS = {"depth": (6, 260), "base": 0.1, "boosts": {4: 0.08, 12: 0.08, 13: 0.08, 6: 0.07, 11: 0.07, 1: 0.07, 3: 0.03},
        "cluster": (1, 3), "scale": (0.75, 1.15), "spread": 55.0, "radius": 60.0}


BED = []  # (surface_at, floor_at) of the last place(), for the thinning pass


def moss_fit(floor_at, px, pz, ps, pf, yaw, sc):
    """(y, tilt) of a moss mat lying on the bed here, or None where it can't: too steep, or the bed bends more than
    3 units under its rim (a mat can't bend, so not over bumps or hollows)."""
    r = MOSS["radius"] * sc * 0.8
    y, tilt = lie_on_bed(floor_at, px, pz, ps, pf, yaw, r, r)
    if max(abs(tilt[0]), abs(tilt[1])) > math.radians(25):
        return None
    c, s_ = math.cos(yaw), math.sin(yaw)
    for ang in range(0, 360, 45):
        a = math.radians(ang)
        lx, lz = math.cos(a) * r, math.sin(a) * r
        wx, wz = px + lx * c + lz * s_, pz - lx * s_ + lz * c
        fy = floor_at(wx, wz, ps)
        if fy is None or abs(fy - (y + lx * math.tan(tilt[1]) - lz * math.tan(tilt[0]))) > 3.0:
            return None
    return y, tilt


# An even cap on how thick a bed gets: per 120-unit cell, the plants' cards times their height (what the console pays
# for, per scanline per card) no more than DENSITY_CAP: trims only the thickest tenth of the West weed flat (its
# 90th percentile ~916; 600 thinned beds that already held). Over it, plants turn to moss mats (by a hash, so
# the same ones every time), cheapest look first: the ones with most cards and height go.
DENSITY_CAP = 900.0
DENSITY_CELL = 120
# (the bed plants; the tall accents, already a third, are left alone: where they grow the views hold easily)
CAP_CARDS = {"grass_tuft": 4, "broadleaf": 2, "bush_weed": 4, "big_pondweed": 2, "ribbon_weed": 3}


def density_capped(instances):
    cells = {}
    for p in instances:
        if p["kind"] in CAP_CARDS:
            cells.setdefault((int(p["x"] // DENSITY_CELL), int(p["z"] // DENSITY_CELL)), []).append(p)
    turn = set()
    for key, ps in cells.items():
        load = sum(CAP_CARDS[p["kind"]] * HEIGHT.get(p["kind"], 40) * p["scale"] for p in ps)
        for p in sorted(ps, key=lambda p: spot_hash(p["x"], p["z"], 3)):
            if load <= DENSITY_CAP:
                break
            load -= CAP_CARDS[p["kind"]] * HEIGHT.get(p["kind"], 40) * p["scale"]
            turn.add(id(p))
    if not turn or not BED:
        return instances
    surface_at, floor_at = BED
    out = []
    for p in instances:
        if id(p) not in turn:
            out.append(p)
            continue
        if BEDSKIN:
            continue
        ps = surface_at(p["x"], p["z"])
        pf = floor_at(p["x"], p["z"], ps) if ps is not None else None
        sc = min(max(p["scale"], 0.7), 1.1)
        fit = moss_fit(floor_at, p["x"], p["z"], ps, pf, p["yaw"], sc) if pf is not None else None
        if fit is not None:
            out.append(dict(p, kind="moss_patch", y=fit[0], tilt=fit[1], scale=sc))
    print(f"density cap: {len(turn)} plants " + ("gone" if BEDSKIN else "to moss (or gone where a mat can't lie)"))
    return out


THIN = os.path.join(ROOT, "fishmod", "data", "lake_hylia_thin_bedskin.json" if BEDSKIN else "lake_hylia_thin.json")


def thinned(instances):
    """The plants thin_beds.py picked (where the console needs it) turned into moss mats, or dropped where a mat
    can't lie."""
    if not os.path.exists(THIN) or not BED:
        return instances
    listed = json.load(open(THIN))["moss"]
    picked = {(o["kind"], o["x"], o["z"]) for o in listed}
    drop = {(o["kind"], o["x"], o["z"]) for o in listed if o.get("drop")}
    surface_at, floor_at = BED
    out = []
    for p in instances:
        if (p["kind"], int(p["x"]), int(p["z"])) not in picked:
            out.append(p)
            continue
        if BEDSKIN or (p["kind"], int(p["x"]), int(p["z"])) in drop:
            continue
        ps = surface_at(p["x"], p["z"])
        pf = floor_at(p["x"], p["z"], ps) if ps is not None else None
        sc = min(max(p["scale"], 0.7), 1.1)
        fit = moss_fit(floor_at, p["x"], p["z"], ps, pf, p["yaw"], sc) if pf is not None else None
        if fit is not None:
            out.append(dict(p, kind="moss_patch", y=fit[0], tilt=fit[1], scale=sc))
    return out


def place_moss(seed, rnd_state, surface_at, floor_at, zones, excluded, boxes):
    rnd = random.Random(seed * 1000 + 77)
    noise = Noise(seed * 31 + 999, 420.0)
    out = []
    xs = [b["xMin"] for b in boxes] + [b["xMin"] + b["xLen"] for b in boxes]
    zs = [b["zMin"] for b in boxes] + [b["zMin"] + b["zLen"] for b in boxes]
    for gz in range(min(zs), max(zs), STEP):
        for gx in range(min(xs), max(xs), STEP):
            x, z = gx + rnd.uniform(0, STEP), gz + rnd.uniform(0, STEP)
            chance_roll = rnd.random()
            surf = surface_at(x, z)
            if surf is None:
                continue
            f = floor_at(x, z, surf)
            if f is None or not (MOSS["depth"][0] <= surf - f <= MOSS["depth"][1]):
                continue
            chance = MOSS["base"] + sum(b * in_zone(zn, x, z) for zn in zones for zid, b in MOSS["boosts"].items()
                                        if zn["id"] == zid)
            chance *= max(0.0, noise(x, z) * 2.2 - 0.6)
            if chance_roll >= chance:
                continue
            for k in range(rnd.randint(*MOSS["cluster"])):
                px = x + (rnd.gauss(0, MOSS["spread"]) if k else 0)
                pz = z + (rnd.gauss(0, MOSS["spread"]) if k else 0)
                sc = rnd.uniform(*MOSS["scale"])
                yaw = rnd.uniform(0, 2 * math.pi)
                ps = surface_at(px, pz)
                if ps is None or excluded(px, pz):
                    continue
                pf = floor_at(px, pz, ps)
                if pf is None or not (MOSS["depth"][0] - 4 <= ps - pf <= MOSS["depth"][1] + 10):
                    continue
                fit = moss_fit(floor_at, px, pz, ps, pf, yaw, sc)
                if fit is None:
                    continue
                y, tilt = fit
                out.append({"kind": "moss_patch", "x": px, "y": y, "z": pz, "yaw": yaw, "tilt": tilt, "scale": sc,
                            "tint": rnd.uniform(0.9, 1.08)})
    return out


# Tall plants as accents (step 3 of the frame-rate plan, fishmod/docs/opt-plan-hw.md): the console pays per scanline
# per card, so the tall kinds are kept to a third, and the eelgrass keeps its beds' edges while the middles turn to
# low plants (grass, broadleaf), with the moss between. Decided after placement, by a hash of where each prop stands,
# so the rest of the layout stays exactly as placed.
ACCENT_KEEP = {"kelp_frond": 1 / 3, "tall_weed": 1 / 3, "vine_strand": 1 / 3}
RIBBON_KEEP_EDGE, RIBBON_KEEP_INSIDE = 0.5, 0.17


def spot_hash(x, z, salt=0):
    h = ((int(x) * 73856093) ^ (int(z) * 19349663) ^ (salt * 83492791)) & 0xFFFFFFFF
    h ^= h >> 13
    h = (h * 0x5BD1E995) & 0xFFFFFFFF
    h ^= h >> 15
    return (h & 0xFFFF) / 65536.0


def accents(instances):
    grid = {}
    for p in instances:
        if p["kind"] in BED_KINDS:
            grid.setdefault((int(p["x"] // 100), int(p["z"] // 100)), []).append(p)

    def neighbours(p):
        gx, gz = int(p["x"] // 100), int(p["z"] // 100)
        return sum(1 for ix in (gx - 1, gx, gx + 1) for iz in (gz - 1, gz, gz + 1) for q in grid.get((ix, iz), ())
                   if q is not p and (q["x"] - p["x"]) ** 2 + (q["z"] - p["z"]) ** 2 < 100 * 100)

    ribbon = [p for p in instances if p["kind"] == "ribbon_weed"]
    counts = {id(p): neighbours(p) for p in ribbon}
    median = sorted(counts.values())[len(counts) // 2] if counts else 0
    out = []
    for p in instances:
        k = p["kind"]
        h = spot_hash(p["x"], p["z"], 1)
        if k in ACCENT_KEEP:
            if h < ACCENT_KEEP[k]:
                out.append(p)
            continue
        if k == "vine_curtain":
            # capped: a spatterdock's pads float on top (its roots, from below), so from above it's leaves, not a
            # cross of card tops
            out.append(p)
            out.append(dict(p, kind="spatterdock", scale=1.0, tilt=(0.0, 0.0)))
            continue
        if k == "ribbon_weed":
            edge = counts[id(p)] < median
            if h < (RIBBON_KEEP_EDGE if edge else RIBBON_KEEP_INSIDE):
                out.append(p)
            elif not edge:
                h2 = spot_hash(p["x"], p["z"], 2)
                if h2 < 0.5:
                    out.append(dict(p, kind="grass_tuft", scale=min(max(p["scale"], 1.0), 1.5)))
                elif h2 < 0.75:
                    out.append(dict(p, kind="broadleaf", scale=min(max(p["scale"] * 0.8, 0.85), 1.15)))
            continue
        out.append(p)
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
    ap.add_argument("--unthinned", action="store_true", help="without the density cap and thin_beds.py's list (A/B)")
    args = ap.parse_args()
    inst = accents(place(args.scene))
    if not args.unthinned:
        inst = thinned(density_capped(inst))
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
