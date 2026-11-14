#!/usr/bin/env python3
"""Designs for the north-beach dock side by side (fishmod/tools/mesh_preview.py): each from the beach, from the side
on the water and from under it, with what it would cost the RDP. The lake bed and water level are the scene's own
(gen_habitat.py's dock_bed_depth, the water 25 units under the deck).

Usage: dock_designs.py OUT.png [--only A,C]
"""
import math
import os
import sys

import numpy as np
from PIL import Image, ImageDraw

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_habitat as gh  # noqa: E402
import mesh_preview as mp  # noqa: E402

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
SPOT06 = os.path.join(ROOT, "extracted", "ntsc-1.2", "assets", "scenes", "overworld", "spot06")
TURIBORI = os.path.join(ROOT, "extracted", "ntsc-1.2", "assets", "scenes", "misc", "turibori")
WATER_Y = -25.0  # the water surface, from the deck (Lake Hylia full: -1313 against the deck's -1288)


def tex(folder, name):
    return np.asarray(Image.open(os.path.join(folder, name)).convert("RGBA"))


TEX = {
    "dock_planks": lambda: tex(TURIBORI, "turibori_scene_0000ACE0_Tex.rgba16.png"),
    "dock_wood": lambda: tex(TURIBORI, "turibori_scene_000081E0_Tex.rgba16.png"),
    "lake_planks": lambda: tex(SPOT06, "spot06_scene_00016CB8_Tex.rgba16.png"),   # the island bridge's boards
    "lake_post": lambda: tex(SPOT06, "spot06_scene_00016CB8_Tex.rgba16.png"),     # the same boards, along the post
    "lake_stone": lambda: tex(SPOT06, "spot06_scene_0001A4B8_Tex.i4.png"),     # the lake's grey brick
    "bed": lambda: tex(SPOT06, "spot06_scene_0000C2B8_Tex.rgba16.png"),
}


def bed_y(z):
    """the lake bed under the dock's middle (from the scene: 30 under the deck at 100 out, 100 at 400), the beach
    rising behind it"""
    if z < 0:
        return min(25.0, -4.0 - z * 0.25)
    return -(4.0 + z * 0.24)


def bed_mesh():
    m = gh.Mesh("bed", "bed")
    xs = np.linspace(-500, 500, 11)
    zs = np.linspace(-300, 900, 25)
    ids = {}
    for i, x in enumerate(xs):
        for j, z in enumerate(zs):
            ids[i, j] = m.vert((x, bed_y(z) - 0.0003 * x * x * 0.05, z), (x / 64.0, z / 64.0), (150, 140, 110))
    for i in range(len(xs) - 1):
        for j in range(len(zs) - 1):
            m.quad(ids[i, j], ids[i, j + 1], ids[i + 1, j + 1], ids[i + 1, j])
    return gh.orient_outward(m, (0, -10000, 400))


def tube(m, p0, p1, r, sides, col0, col1, v0, v1, twist=0.0, phase=0.0, rings=None, caps=(False, False)):
    """a closed post from p0 to p1 with the texture's v from v0 to v1; `twist` turns the far end by that share of a
    side (0.5: the antiprism, rounder for its vertex count); `rings` adds rings between (fractions 0-1, with their
    colours) for a colour change part way (the waterline)"""
    first = len(m.f)
    ax = np.subtract(p1, p0, dtype=float)
    length = np.linalg.norm(ax)
    ax /= length
    up = np.array((0, 1, 0)) if abs(ax[1]) < 0.9 else np.array((1, 0, 0))
    u = np.cross(ax, up)
    u /= np.linalg.norm(u)
    w = np.cross(ax, u)
    stops = [(0.0, col0)] + list(rings or []) + [(1.0, col1)]
    rings_ids = []
    for t, col in stops:
        ring = []
        for s in range(sides + 1):
            a = 2 * math.pi * (s + phase + twist * t) / sides
            d = u * math.cos(a) + w * math.sin(a)
            pos = np.add(p0, ax * length * t) + d * r
            shade = 0.8 + 0.2 * math.cos(a - 0.6)
            ring.append(m.vert(tuple(pos), (s / sides, v0 + (v1 - v0) * t), tuple(int(c * shade) for c in col)))
        rings_ids.append(ring)
    for r0, r1 in zip(rings_ids, rings_ids[1:]):
        for s in range(sides):
            m.quad(r0[s], r0[s + 1], r1[s + 1], r1[s])
    for end, ring in ((0, rings_ids[0]), (1, rings_ids[-1])):
        if caps[end]:
            c = m.vert(tuple(p1 if end else p0), (0.5, 0.5), stops[-1 if end else 0][1])
            for s in range(sides):
                m.tri(c, ring[s], ring[s + 1])
    return gh.orient_outward(m, tuple(np.add(p0, p1) / 2), first)


def box(m, lo, hi, col, uv_scale=(1 / 32.0, 1 / 32.0), faces="all"):
    first = len(m.f)
    (x0, y0, z0), (x1, y1, z1) = lo, hi
    su, sv = uv_scale
    quads = {
        "top": ((x0, y1, z0), (x1, y1, z0), (x1, y1, z1), (x0, y1, z1), (x1 - x0) * su, (z1 - z0) * sv),
        "bottom": ((x0, y0, z1), (x1, y0, z1), (x1, y0, z0), (x0, y0, z0), (x1 - x0) * su, (z1 - z0) * sv),
        "xp": ((x1, y0, z0), (x1, y0, z1), (x1, y1, z1), (x1, y1, z0), (z1 - z0) * su, (y1 - y0) * sv),
        "xn": ((x0, y0, z1), (x0, y0, z0), (x0, y1, z0), (x0, y1, z1), (z1 - z0) * su, (y1 - y0) * sv),
        "zp": ((x1, y0, z1), (x0, y0, z1), (x0, y1, z1), (x1, y1, z1), (x1 - x0) * su, (y1 - y0) * sv),
        "zn": ((x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0), (x1 - x0) * su, (y1 - y0) * sv),
    }
    for k, (a, b, c, d, uu, vv) in quads.items():
        if faces != "all" and k not in faces:
            continue
        gh.quad_uv(m, (a, b, c, d), col, (0, 0, uu, vv))
    return gh.orient_outward(m, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), first)


def depth_at(x, z):
    d = gh.dock_bed_depth(x, z)
    return (-bed_y(z)) if d is None else d


# ---------------------------------------------------------------------------------------------------------------
# The designs: each a list of (mesh, texture, cull)

def design_now():
    deck = gh.Mesh("deck", "dock_planks")
    hw, L, t = gh.DOCK_WIDTH / 2, gh.DOCK_LENGTH, gh.DOCK_THICK
    top, side, under = (235, 230, 220), (170, 165, 155), (110, 110, 105)
    rep = L / 30.0
    gh.quad_uv(deck, ((-hw, 0, 0), (hw, 0, 0), (hw, 0, L), (-hw, 0, L)), top, (0, 0, 1, rep))
    gh.quad_uv(deck, ((-hw, -t, L), (hw, -t, L), (hw, -t, 0), (-hw, -t, 0)), under, (0, 0, 1, rep))
    gh.quad_uv(deck, ((hw, -t, 0), (hw, -t, L), (hw, 0, L), (hw, 0, 0)), side, (0, 0.8, rep, 1))
    gh.quad_uv(deck, ((-hw, -t, L), (-hw, -t, 0), (-hw, 0, 0), (-hw, 0, L)), side, (0, 0.8, rep, 1))
    gh.quad_uv(deck, ((hw, -t, L), (-hw, -t, L), (-hw, 0, L), (hw, 0, L)), side, (0, 0.8, 1, 1))
    posts = gh.Mesh("posts", "dock_wood")
    dark, light = gh.hexc("#4a3a28"), gh.hexc("#a08a68")
    for z in (100, 200, 300, 400):  # (the old layout)
        for x in (-25, 25):
            gh.cylinder(posts, (x, -150, z), (x, -t, z), 5, 5, 6, dark, light, v_repeat=4.0)
        gh.cylinder(posts, (-33, -t - 4, z), (33, -t - 4, z), 4, 4, 4, dark, light, v_repeat=1.5)
    x, z, h = (-25, 412, 30)
    gh.cylinder(posts, (x, -150, z), (x, h, z), 6, 6, 6, dark, light, v_repeat=5.0)
    return [(deck, "dock_planks", False), (posts, "dock_wood", False)]


def design_fixed():
    return [(gh.mesh_dock_deck(), "dock_planks", True), (gh.mesh_dock_posts(), "dock_wood", True)]


def lake_deck(L, hw, t, rail=False):
    """the island bridge's boards across the deck, a rim board along each side"""
    m = gh.Mesh("deck", "lake_planks")
    top, side, under = (230, 225, 210), (175, 165, 150), (95, 100, 90)
    gh.quad_uv(m, ((-hw, 0, 0), (hw, 0, 0), (hw, 0, L), (-hw, 0, L)), top, (0, 0, L / 48.0, hw / 32.0))
    gh.orient_outward(m, (0, -100, L / 2), 0)  # (faces up)
    first = len(m.f)
    gh.quad_uv(m, ((-hw, -t, L), (hw, -t, L), (hw, -t, 0), (-hw, -t, 0)), under, (0, 0, L / 48.0, hw / 32.0))
    gh.orient_outward(m, (0, 100, L / 2), first)  # (faces down)
    box(m, (hw, -t - 2, 0), (hw + 3, 2, L), side, (1 / 16.0, 1 / 16.0), faces=("xp", "top", "bottom", "zp"))
    box(m, (-hw - 3, -t - 2, 0), (-hw, 2, L), side, (1 / 16.0, 1 / 16.0), faces=("xn", "top", "bottom", "zp"))
    box(m, (-hw, -t, L - 0.01), (hw, 0, L), side, (1 / 16.0, 1 / 16.0), faces=("zp",))
    return m


def design_lake():
    """C: the island bridge's own look. Square posts turned half a side at the top (an antiprism: as round as eight
    sides from most angles, from four vertices a ring), the rope-wrapped post texture above the deck, wet and dark
    under the waterline. Three pairs, a little thicker, standing above the deck as the bridge's do."""
    L, hw, t = gh.DOCK_LENGTH, gh.DOCK_WIDTH / 2, gh.DOCK_THICK
    deck = lake_deck(L, hw, t)
    posts = gh.Mesh("posts", "lake_post")
    wet, dry, top = (70, 80, 55), (200, 185, 160), (230, 215, 190)
    for z in (60, 200, 340, 412):
        for x in (-hw - 1, hw + 1):
            d = depth_at(x, z)
            above = 22.0
            span = d + 10 + above
            wl = (d + 10 + WATER_Y) / span  # the waterline's share up the post
            tube(posts, (x, -(d + 10), z), (x, above, z), 5.5, 4, wet, top, 3.0, 0.0, twist=0.5, phase=0.5,
                 rings=[(wl, wet), (min(wl + 0.04, 0.99), dry)], caps=(False, True))
    return [(deck, "lake_planks", True), (posts, "lake_post", True)]


def design_pier():
    """D: two broad stone piers under the deck (Lake Hylia's own stone), the planks over them, two short mooring
    posts at the far end. A few big faces instead of many tall thin ones."""
    L, hw, t = gh.DOCK_LENGTH, gh.DOCK_WIDTH / 2, gh.DOCK_THICK
    deck = lake_deck(L, hw, t)
    stone = gh.Mesh("piers", "lake_stone")
    for z in (150, 330):
        d = depth_at(0, z)
        box(stone, (-hw + 4, -(d + 10), z - 14), (hw - 4, -t, z + 14), (190, 185, 175), (1 / 24.0, 1 / 24.0),
            faces=("xp", "xn", "zp", "zn"))
    posts = gh.Mesh("posts", "lake_post")
    wet, dry, top = (70, 80, 55), (200, 185, 160), (230, 215, 190)
    for x in (-hw + 4, hw - 4):
        d = depth_at(x, L - 6)
        span = d + 10 + 24
        wl = (d + 10 + WATER_Y) / span
        tube(posts, (x, -(d + 10), L - 6), (x, 24, L - 6), 5.0, 4, wet, top, 3.0, 0.0, twist=0.5, phase=0.5,
             rings=[(wl, wet), (min(wl + 0.04, 0.99), dry)], caps=(False, True))
    return [(deck, "lake_planks", True), (stone, "lake_stone", True), (posts, "lake_post", True)]


DESIGNS = {"A: now": design_now, "B: as built (twisted pentagon posts)": design_fixed, "C: Lake Hylia bridge style": design_lake,
           "D: stone piers": design_pier}

VIEWS = [
    ("from the beach", (40, 45, -120), (0, -10, 220), None),
    ("from the water, side on", (-230, -8, 150), (0, -20, 230), None),
    ("under the water", (-140, -55, 120), (0, -45, 260), ((60, 95, 90), 50.0, 520.0)),
]


def arrays(m):
    v = np.array([p[:3] for p in m.v])
    col = np.array([p[3:6] for p in m.v])
    uv = np.array(m.vt)
    f = [(a - 1, b - 1, c - 1) for a, b, c in m.f]
    return v, uv, col, f


def render(design, view):
    name, eye, at, fog = view
    under = fog is not None
    c = mp.Canvas(sky=(60, 95, 90) if under else (150, 175, 190))
    mvp = mp.look_at(eye, at)
    v, uv, col, f = arrays(bed_mesh())
    c.draw(mvp, v, uv, col, f, TEX["bed"](), cull=True, fog=fog or ((150, 175, 190), 600.0, 2500.0), count=False,
           water_y=WATER_Y, under_tint=None if under else (0.55, 0.75, 0.75))
    for m, t, cull in design():
        v, uv, col, f = arrays(m)
        c.draw(mvp, v, uv, col, f, TEX[t](), cull=cull, fog=fog or ((150, 175, 190), 600.0, 2500.0),
               water_y=WATER_Y, under_tint=None if under else (0.55, 0.75, 0.75))
    return c


def main():
    out = sys.argv[1]
    only = None
    if "--only" in sys.argv:
        only = sys.argv[sys.argv.index("--only") + 1].split(",")
    names = [n for n in DESIGNS if only is None or n[0] in only]
    scale = 2
    cw, ch = mp.W * scale, mp.H * scale
    sheet = Image.new("RGB", (cw * len(VIEWS) + 200, (ch + 4) * len(names) + 24), (30, 30, 30))
    d = ImageDraw.Draw(sheet)
    for j, view in enumerate(VIEWS):
        d.text((200 + j * cw + 6, 6), view[0], fill=(255, 255, 255))
    for i, n in enumerate(names):
        y = 24 + i * (ch + 4)
        d.text((6, y + 6), n, fill=(255, 255, 0))
        tris = 0
        for j, view in enumerate(VIEWS):
            c = render(DESIGNS[n], view)
            sheet.paste(c.image(scale), (200 + j * cw, y))
            d.text((200 + j * cw + 6, y + ch - 16), "dock: %.2f ms RDP, %d tris" % (c.cost_ns / 1e6, c.tris),
                   fill=(255, 255, 255))
            print("%-28s %-26s %.2f ms, %d triangles, %.0f px" % (n, view[0], c.cost_ns / 1e6, c.tris, c.px))
        total = sum(len(m.f) for m, _, _ in DESIGNS[n]())
        d.text((6, y + 22), "%d triangles" % total, fill=(200, 200, 200))
    sheet.save(out)


if __name__ == "__main__":
    main()
