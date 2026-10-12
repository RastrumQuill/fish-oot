#!/usr/bin/env python3
"""Generates the habitat props for Lake Hylia's sub-biomes: textures (PNG) and small meshes (OBJ with vertex colours
and UVs), in Ocarina of Time's style. fishmod/tools/assets.py turns them into C (fishmod/assets/assets.json), and
fishmod/tools/style_sheet.py renders them next to vanilla art for review.

Style rules (fishmod/docs/research/visual-references.md):
- low vertex counts: crossed cards for plants, few-sided cylinders for wood, a quad per decal;
- small textures (16x16 to 32x64), 4-bit palettes or greyscale cards tinted by vertex colour;
- colours from the vanilla textures they sit next to (sampled from the Fishing Pond and Lake Hylia: see PALETTES);
- vertex colours do the shading: dark at the base, light at the tips.

Deterministic (fixed seeds), so the output only changes when this file does.

Usage: fishmod/tools/gen_habitat.py     writes fishmod/art/habitat/*.png and fishmod/assets/habitat/*.obj
"""
import math
import os
import random

from PIL import Image

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
ART = os.path.join(ROOT, "fishmod", "art", "habitat")
MESH = os.path.join(ROOT, "fishmod", "assets", "habitat")


def hexc(h):
    return tuple(int(h[i:i + 2], 16) for i in (1, 3, 5))


# Sampled from vanilla textures (8-colour quantisations, most common first)
PALETTES = {
    "pond_reeds": ["#346230", "#264b23", "#498641", "#184217", "#133611", "#081f05", "#082b07", "#12290e"],
    "pond_pebbles": ["#abad7c", "#c6c998", "#524e29", "#949466", "#7f7e52", "#19170d", "#3b3719", "#69663e"],
    "lily_pad": ["#294905", "#295208", "#214404", "#346310", "#457d20", "#305a0b", "#142b01", "#315208"],
    "wood_post": ["#e1b8a1", "#f4d9c8", "#493831", "#352621", "#cca591", "#755c53", "#ae8a79", "#100a09"],
    "lake_sand": ["#a7a570", "#958c53", "#99945a", "#857d45", "#a59c64", "#b3b289", "#9c9c63", "#8c8c53"],
    "lake_cobble": ["#3a351e", "#676240", "#575233", "#4b462a", "#1a160b", "#8a9052", "#7f7d51", "#a2a95e"],
    "lake_grass": ["#6c7a3b", "#525d28", "#878746", "#b3b367", "#8c9a50", "#e1d496", "#a8a05a", "#82954c"],
    "pond_moss": ["#67673d", "#575630", "#707346", "#838752", "#423e20", "#4a4a29", "#2b2311", "#5a6339"],
}


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(len(a)))


def save(img, name):
    os.makedirs(ART, exist_ok=True)
    img.save(os.path.join(ART, name + ".png"))


# ---------------------------------------------------------------------------------------------------------------
# Textures


def tex_bark():
    """Waterlogged bark: dark wood with vertical grooves and a green-brown algae tint."""
    rnd = random.Random(61)
    dark = [hexc(c) for c in ("#100a09", "#352621", "#493831", "#755c53")]
    moss = [hexc(c) for c in PALETTES["pond_moss"][:4]]
    w, h = 16, 32
    img = Image.new("RGB", (w, h))
    grooves = [rnd.uniform(0, w) for _ in range(5)]
    for y in range(h):
        for x in range(w):
            g = min(min(abs(x - gx), w - abs(x - gx)) for gx in grooves)
            v = min(1.0, g / 2.0) * (0.8 + 0.2 * math.sin(y * 0.9 + x))
            base = lerp(dark[0], dark[3], v)
            img.putpixel((x, y), lerp(base, moss[int(v * 3.99)], 0.35))
    return img


def tex_rings():
    """A log's cut end: growth rings."""
    w = h = 16
    a, b = hexc("#755c53"), hexc("#493831")
    img = Image.new("RGB", (w, h))
    for y in range(h):
        for x in range(w):
            r = math.hypot(x - 7.5, y - 7.5)
            img.putpixel((x, y), a if int(r * 1.3) % 2 == 0 else b)
    return img


# ---------------------------------------------------------------------------------------------------------------
# Textures edited from vanilla ones (OoT's textures are edited photos; editing them keeps that look). Made from the
# extracted assets of the user's own ROM at build time, never committed: written to fishmod/art/habitat_rom/.

VANILLA = os.path.join(ROOT, "extracted", "ntsc-1.2", "assets")
ART_ROM = os.path.join(ROOT, "fishmod", "art", "habitat_rom")


def vanilla(path):
    return Image.open(os.path.join(VANILLA, path)).convert("RGBA")


def luma(c):
    return (c[0] * 299 + c[1] * 587 + c[2] * 114) / 1000.0


def ramp(palette, t):
    """A colour from a palette sorted dark to light, t in 0-1."""
    cols = sorted((hexc(c) for c in palette), key=luma)
    t = max(0.0, min(1.0, t)) * (len(cols) - 1)
    i = min(int(t), len(cols) - 2)
    return lerp(cols[i], cols[i + 1], t - i)


def rom_pebbles():
    """The Fishing Pond's river stones, recoloured toward Lake Hylia's cobble and sand: keeps the photo detail."""
    src = vanilla("scenes/misc/turibori/turibori_scene_0000FCE0_Tex.rgba16.png")
    out = Image.new("RGB", src.size)
    pal = PALETTES["lake_cobble"] + PALETTES["lake_sand"][:4]
    lo = min(luma(src.getpixel((x, y))) for y in range(src.height) for x in range(src.width))
    hi = max(luma(src.getpixel((x, y))) for y in range(src.height) for x in range(src.width))
    for y in range(src.height):
        for x in range(src.width):
            r, g, b, a = src.getpixel((x, y))
            t = (luma((r, g, b)) - lo) / max(hi - lo, 1)
            # keep a little of the original colour so stones differ from each other
            out.putpixel((x, y), lerp(ramp(pal, t), (r, g, b), 0.3))
    return out


def rom_lily_flower():
    """Half of the Great Fairy fountain's flower (a photo-made white flower): a fan of petals, for billboards stood
    in a ring to make a cupped water lily. Pink at the base fading to white tips."""
    src = vanilla("objects/object_dy_obj/gGreatFairyFlowerTex.ia16.png")
    w, h = src.size
    half = src.crop((0, 0, w, h // 2))  # the top half: petals fanning out from the centre at the bottom edge
    out = Image.new("RGBA", (w, h // 2), (0, 0, 0, 0))
    pink, white = hexc("#d8708c"), hexc("#fbf3f3")
    cx, cy = (w - 1) / 2.0, h / 2.0
    for y in range(h // 2):
        for x in range(w):
            r, g, b, a = half.getpixel((x, y))
            if a < 128:
                continue
            v = luma((r, g, b)) / 255.0
            d = math.hypot(x - cx, y - cy) / (w / 2.0)
            col = lerp(pink, white, min(1.0, max(0.0, (d - 0.15) / 0.7)))
            out.putpixel((x, y), tuple(int(c * (0.6 + 0.4 * v)) for c in col) + (255,))
    return out


def rom_lily_centre():
    """The flower's centre: the middle of the fairy flower, tinted yellow."""
    src = vanilla("objects/object_dy_obj/gGreatFairyFlowerTex.ia16.png")
    w, h = src.size
    c = src.crop((w // 4, h // 4, w * 3 // 4, h * 3 // 4))
    out = Image.new("RGBA", c.size, (0, 0, 0, 0))
    gold, yellow = hexc("#b88018"), hexc("#f4d848")
    for y in range(c.height):
        for x in range(c.width):
            r, g, b, a = c.getpixel((x, y))
            if a < 128 or math.hypot(x - c.width / 2 + 0.5, y - c.height / 2 + 0.5) > c.width * 0.42:
                continue
            out.putpixel((x, y), lerp(gold, yellow, luma((r, g, b)) / 255.0) + (255,))
    return out


def rom_bark():
    """Waterlogged wood: the Fishing Pond's wood post, darkened and tinted with algae, its grain kept."""
    src = vanilla("objects/object_fish/gFishingWoodPostTex.rgba16.png")
    moss = [hexc(c) for c in PALETTES["pond_moss"]]
    out = Image.new("RGB", src.size)
    for y in range(src.height):
        for x in range(src.width):
            r, g, b, a = src.getpixel((x, y))
            v = luma((r, g, b)) / 255.0
            dark = (int(r * 0.35), int(g * 0.32), int(b * 0.28))
            out.putpixel((x, y), lerp(dark, moss[(x * 3 + y) % 4], 0.25 + 0.2 * (1 - v)))
    return out


def ia_card(path, crop=None, size=None, gamma=1.0):
    """A vanilla texture as a greyscale card with its cut-out (an IA texture tinted by vertex colour, so one card
    gives many plants), detail kept."""
    src = vanilla(path)
    if crop:
        src = src.crop(crop)
    if size:
        src = src.resize(size, Image.NEAREST if size[0] <= src.width else Image.BILINEAR)
    out = Image.new("LA", src.size)
    lo, hi = 255, 0
    for y in range(src.height):
        for x in range(src.width):
            r, g, b, a = src.getpixel((x, y))
            if a >= 128:
                lo, hi = min(lo, luma((r, g, b))), max(hi, luma((r, g, b)))
    for y in range(src.height):
        for x in range(src.width):
            r, g, b, a = src.getpixel((x, y))
            v = (luma((r, g, b)) - lo) / max(hi - lo, 1)
            out.putpixel((x, y), (int(255 * (0.25 + 0.75 * v ** gamma)), 255 if a >= 128 else 0))
    return out


def rom_reed_card():
    return ia_card("scenes/misc/turibori/turibori_scene_00003CE0_Tex.rgba16.png")


def rom_grass_card():
    return ia_card("objects/gameplay_keep/gUnusedGrassBladesTex.rgba16.png")


def rom_ribbon_card():
    # the pond's reeds squeezed narrow and stretched tall: long ribbons
    return ia_card("scenes/misc/turibori/turibori_scene_00003CE0_Tex.rgba16.png", None, (32, 64))


def rom_broadleaf_card():
    return ia_card("objects/gameplay_field_keep/gFieldBushTex.rgba16.png")


def rom_bush_card():
    return ia_card("objects/gameplay_keep/gCuttableShrubTex.rgba16.png", gamma=1.3)


def rom_roots_card():
    return ia_card("scenes/misc/turibori/turibori_scene_000059E0_Tex.ia8.png")


def rom_float_leaf():
    """Floating leaves (spatterdock-like): Hana's leaf, darkened to a pond green."""
    src = vanilla("objects/gameplay_field_keep/gHanaLeafTex.rgba16.png")
    out = Image.new("RGBA", src.size)
    for y in range(src.height):
        for x in range(src.width):
            r, g, b, a = src.getpixel((x, y))
            v = luma((r, g, b)) / 255.0
            out.putpixel((x, y), lerp(hexc("#1c3a10"), hexc("#7a9a40"), v) + (255 if a >= 128 else 0,))
    return out


def rom_leaf_litter():
    return vanilla("objects/gameplay_keep/gWitheredLeafTex.rgba16.png")


ROM_TEXTURES = {
    "reed_card": (rom_reed_card, "ia8"),
    "grass_card": (rom_grass_card, "ia8"),
    "ribbon_card": (rom_ribbon_card, "ia8"),
    "broadleaf_card": (rom_broadleaf_card, "ia8"),
    "bush_card": (rom_bush_card, "ia8"),
    "roots_card": (rom_roots_card, "ia8"),
    "float_leaf": (rom_float_leaf, "ci4"),
    "leaf_litter": (rom_leaf_litter, "ci4"),
    "bark": (rom_bark, "ci4"),
    "pebbles": (rom_pebbles, "ci4"),
    "lily_flower": (rom_lily_flower, "ci4"),
    "lily_centre": (rom_lily_centre, "ci4"),
}


TEXTURES = {
    "log_rings": (tex_rings, "ci4"),
}


# ---------------------------------------------------------------------------------------------------------------
# Meshes: lists of (pos, uv, colour) triangles, written as OBJ with vertex colours.


class Mesh:
    def __init__(self, name, texture):
        self.name = name
        self.texture = texture
        self.v = []   # (x, y, z, r, g, b)
        self.vt = []  # (u, v)
        self.f = []   # ((vi, ti), ...)

    def vert(self, p, uv, col):
        self.v.append((p[0], p[1], p[2]) + tuple(c / 255.0 for c in col))
        self.vt.append(uv)
        return len(self.v)

    def tri(self, a, b, c):
        self.f.append((a, b, c))

    def quad(self, a, b, c, d):
        self.f.append((a, b, c))
        self.f.append((a, c, d))

    def write(self):
        os.makedirs(MESH, exist_ok=True)
        with open(os.path.join(MESH, self.name + ".obj"), "w") as f:
            f.write(f"# generated by fishmod/tools/gen_habitat.py; texture: {self.texture}\n")
            for v in self.v:
                f.write("v %.2f %.2f %.2f %.4f %.4f %.4f\n" % v)
            for u, v in self.vt:
                f.write("vt %.4f %.4f\n" % (u, 1.0 - v))  # OBJ's v points up; ours is image rows (down)
            for face in self.f:
                f.write("f " + " ".join(f"{i}/{i}" for i in face) + "\n")
        return self


def crossed_cards(name, texture, width, height, planes, base_col, tip_col, seed, lean=0.0):
    """Planes crossing at the centre, like vanilla's grass: one texture, coloured dark at the base and light at
    the tips by the vertex colours."""
    rnd = random.Random(seed)
    m = Mesh(name, texture)
    for p in range(planes):
        ang = math.pi * p / planes + rnd.uniform(-0.15, 0.15)
        dx, dz = math.cos(ang) * width / 2, math.sin(ang) * width / 2
        lx = lean * rnd.uniform(0.6, 1.0)
        a = m.vert((-dx, 0, -dz), (0, 1), base_col)
        b = m.vert((dx, 0, dz), (1, 1), base_col)
        c = m.vert((dx + lx, height, dz), (1, 0), tip_col)
        d = m.vert((-dx + lx, height, -dz), (0, 0), tip_col)
        m.quad(a, b, c, d)
    return m


def cylinder(m, p0, p1, r0, r1, sides, col0, col1, v_repeat=2.0, cap=None, cap_col=None):
    """A few-sided tube from p0 to p1 (radius r0 to r1)."""
    ax = [p1[i] - p0[i] for i in range(3)]
    length = math.sqrt(sum(a * a for a in ax))
    ax = [a / length for a in ax]
    up = (0, 1, 0) if abs(ax[1]) < 0.9 else (1, 0, 0)
    u = [ax[1] * up[2] - ax[2] * up[1], ax[2] * up[0] - ax[0] * up[2], ax[0] * up[1] - ax[1] * up[0]]
    ul = math.sqrt(sum(a * a for a in u))
    u = [a / ul for a in u]
    w = [ax[1] * u[2] - ax[2] * u[1], ax[2] * u[0] - ax[0] * u[2], ax[0] * u[1] - ax[1] * u[0]]
    ring0, ring1 = [], []
    for s in range(sides + 1):
        t = 2 * math.pi * s / sides
        c, sn = math.cos(t), math.sin(t)
        # a little darker on the underside
        shade = 0.75 + 0.25 * (0.5 + 0.5 * sn)
        d = [u[i] * c + w[i] * sn for i in range(3)]
        ring0.append(m.vert([p0[i] + d[i] * r0 for i in range(3)], (s / sides, v_repeat),
                            tuple(int(x * shade) for x in col0)))
        ring1.append(m.vert([p1[i] + d[i] * r1 for i in range(3)], (s / sides, 0), tuple(int(x * shade) for x in col1)))
    for s in range(sides):
        m.quad(ring0[s], ring0[s + 1], ring1[s + 1], ring1[s])
    return m


def mesh_log():
    m = Mesh("sunken_log", "bark")
    dark, light = hexc("#493831"), hexc("#8a7a5a")
    cylinder(m, (-150, 12, 0), (150, 14, 4), 15, 12, 6, dark, light, v_repeat=6.0)
    # two branch stubs
    cylinder(m, (40, 20, 6), (70, 52, 22), 5, 2, 4, dark, light, v_repeat=1.5)
    cylinder(m, (-80, 18, -6), (-100, 40, -30), 4, 2, 4, dark, light, v_repeat=1.2)
    return m


def cap(m, center, radius, sides, col, y_up=True):
    """A flat few-sided disc (a cut end) mapped with the ring texture's UVs (0-1 across)."""
    c = m.vert(center, (0.5, 0.5), col)
    ring = []
    for s in range(sides):
        t = 2 * math.pi * s / sides
        ring.append(m.vert((center[0] + math.cos(t) * radius, center[1], center[2] + math.sin(t) * radius),
                           (0.5 + 0.5 * math.cos(t), 0.5 + 0.5 * math.sin(t)), col))
    for s in range(sides):
        a, b = ring[s], ring[(s + 1) % sides]
        m.tri(c, b, a) if y_up else m.tri(c, a, b)
    return m


def mesh_stump():
    m = Mesh("stump", "bark")
    dark, light = hexc("#352621"), hexc("#8a7a5a")
    cylinder(m, (0, -10, 0), (0, 24, 0), 15, 11, 6, dark, light, v_repeat=1.2)
    # roots
    for k in range(3):
        a = 2 * math.pi * k / 3 + 0.4
        cylinder(m, (math.cos(a) * 8, 4, math.sin(a) * 8), (math.cos(a) * 26, -6, math.sin(a) * 26), 5, 2, 4,
                 dark, dark, v_repeat=0.8)
    return m


def mesh_stump_top():
    """The stump's cut top, drawn with the ring texture."""
    return cap(Mesh("stump_top", "log_rings"), (0, 24, 0), 11, 6, hexc("#c8b8a0"))


def mesh_drowned_tree():
    m = Mesh("drowned_tree", "bark")
    dark, light = hexc("#2b2311"), hexc("#707346")
    cylinder(m, (0, -10, 0), (6, 160, 4), 14, 8, 5, dark, light, v_repeat=5.0)
    for (a, b, r) in (((5, 90, 3), (55, 150, 30), 5), ((6, 120, 4), (-45, 190, -20), 4), ((6, 155, 4), (20, 210, -35), 3)):
        cylinder(m, a, b, r, 1.5, 4, dark, light, v_repeat=2.0)
    return m


def mesh_decal(name, texture, size, col):
    m = Mesh(name, texture)
    h = size / 2
    a = m.vert((-h, 0, -h), (0, 0), col)
    b = m.vert((h, 0, -h), (1, 0), col)
    c = m.vert((h, 0, h), (1, 1), col)
    d = m.vert((-h, 0, h), (0, 1), col)
    m.quad(a, d, c, b)
    return m


def mesh_lily_flower():
    """A cupped water lily: half-flower billboards stood in a ring around the centre, tilted outward (an outer ring
    of five, an inner ring of three standing straighter), and a small flat centre."""
    m = Mesh("lily_flower", "lily_flower")
    white = (255, 255, 255)
    for ring, count, width, height, tilt, turn in ((0, 5, 12, 7, 0.75, 0.0), (1, 3, 8, 6, 0.35, 0.6)):
        for k in range(count):
            ang = 2 * math.pi * k / count + turn
            # the card's base sits on a small circle, facing outward, leaning out by `tilt`
            ox, oz = math.cos(ang), math.sin(ang)
            tx, tz = -oz, ox  # along the card
            base = 1.5 + ring * 0.5
            bx, bz = ox * base, oz * base
            lean = math.sin(tilt) * height
            up = math.cos(tilt) * height
            a = m.vert((bx - tx * width / 2, 0.5, bz - tz * width / 2), (0, 1), white)
            b = m.vert((bx + tx * width / 2, 0.5, bz + tz * width / 2), (1, 1), white)
            c = m.vert((bx + tx * width / 2 + ox * lean, 0.5 + up, bz + tz * width / 2 + oz * lean), (1, 0), white)
            d = m.vert((bx - tx * width / 2 + ox * lean, 0.5 + up, bz - tz * width / 2 + oz * lean), (0, 0), white)
            m.quad(a, b, c, d)
    return m


def mesh_lily_centre():
    m = Mesh("lily_centre", "lily_centre")
    s, y, white = 2.5, 2.5, (255, 255, 255)
    a = m.vert((-s, y, -s), (0, 0), white)
    b = m.vert((s, y, -s), (1, 0), white)
    c = m.vert((s, y, s), (1, 1), white)
    d = m.vert((-s, y, s), (0, 1), white)
    m.quad(a, d, c, b)
    return m


def crossed_cards_v(name, texture, width, height, planes, base_col, tip_col, seed, lean=0.0, v_repeat=1.0):
    """crossed_cards with the texture repeated up the card (tall weed columns)."""
    m = crossed_cards(name, texture, width, height, planes, base_col, tip_col, seed, lean)
    m.vt = [(u, v * v_repeat) for u, v in m.vt]
    return m


def mesh_flat_scatter(name, texture, count, spread, size, y, col, seed):
    """Small flat quads scattered around (floating leaves on the surface, leaf litter on the bed)."""
    rnd = random.Random(seed)
    m = Mesh(name, texture)
    for _ in range(count):
        cx, cz = rnd.uniform(-spread, spread), rnd.uniform(-spread, spread)
        ang = rnd.uniform(0, math.pi)
        sz = size * rnd.uniform(0.7, 1.2)
        c, sn = math.cos(ang) * sz, math.sin(ang) * sz
        pts = [(cx - c + sn, cz - sn - c), (cx + c + sn, cz + sn - c), (cx + c - sn, cz + sn + c), (cx - c - sn, cz - sn + c)]
        uv = [(0, 0), (1, 0), (1, 1), (0, 1)]
        ids = [m.vert((px, y + rnd.uniform(0, 0.4), pz), t, col) for (px, pz), t in zip(pts, uv)]
        m.quad(ids[0], ids[3], ids[2], ids[1])
    return m


def mesh_fallen_tree():
    """A big fallen tree on the bottom: a thick trunk, a root plate at one end and a crown of bare branches."""
    m = Mesh("fallen_tree", "bark")
    dark, light = hexc("#241c10"), hexc("#7a7a58")
    cylinder(m, (-260, 22, 0), (240, 30, 30), 26, 16, 6, dark, light, v_repeat=8.0)
    # the root plate: a flared, short cylinder at the base, turned up
    cylinder(m, (-270, 10, -4), (-285, 60, -10), 44, 30, 6, dark, dark, v_repeat=1.0)
    # crown branches fanning from the top end
    for (a, b, r) in (((150, 34, 20), (330, 90, 80), 7), ((180, 36, 26), (320, 40, -70), 6),
                      ((210, 40, 34), (360, 120, 10), 5), ((120, 30, 14), (230, 110, -40), 5),
                      ((60, 26, 8), (110, 80, 60), 4), ((260, 32, 30), (400, 30, 70), 4)):
        cylinder(m, a, b, r, 1.5, 4, dark, light, v_repeat=2.0)
    return m


def mesh_branch():
    m = Mesh("branch", "bark")
    dark, light = hexc("#2b2311"), hexc("#707346")
    cylinder(m, (-70, 4, 0), (70, 7, 10), 4, 2.5, 4, dark, light, v_repeat=3.0)
    cylinder(m, (10, 6, 4), (35, 30, 30), 2.2, 1, 3, dark, light, v_repeat=1.0)
    cylinder(m, (-30, 5, 2), (-45, 22, -20), 2, 1, 3, dark, light, v_repeat=1.0)
    return m


MESHES = [
    lambda: crossed_cards("grass_tuft", "grass_card", 34, 16, 2, hexc("#1c3a12"), hexc("#6c7a3b"), 104),
    lambda: crossed_cards_v("tall_weed", "ribbon_card", 26, 190, 3, hexc("#0d260b"), hexc("#5a6a30"), 105, lean=18,
                            v_repeat=2.0),
    lambda: crossed_cards("broadleaf", "broadleaf_card", 38, 40, 3, hexc("#1a3a12"), hexc("#7a8a40"), 106),
    lambda: crossed_cards("old_posts", "roots_card", 70, 60, 2, hexc("#2b2311"), hexc("#8a8a70"), 107),
    lambda: crossed_cards("cattails", "reed_card", 44, 110, 3, hexc("#3a4a18"), hexc("#b8b070"), 108, lean=3),
    lambda: mesh_flat_scatter("float_leaves", "float_leaf", 5, 22, 7, 0.5, (255, 255, 255), 109),
    lambda: mesh_flat_scatter("leaf_litter", "leaf_litter", 9, 40, 5, 0.6, (200, 200, 200), 110),
    mesh_branch,
    mesh_fallen_tree,

    lambda: crossed_cards("reed_clump", "reed_card", 40, 95, 3, hexc("#184217"), hexc("#8f9342"), 101, lean=4),
    lambda: crossed_cards("ribbon_weed", "ribbon_card", 18, 80, 3, hexc("#133611"), hexc("#6c7a3b"), 102, lean=10),
    lambda: crossed_cards("bush_weed", "bush_card", 46, 42, 3, hexc("#082b07"), hexc("#525d28"), 103),
    mesh_log,
    mesh_stump,
    mesh_stump_top,
    mesh_drowned_tree,
    lambda: mesh_decal("pebble_patch", "pebbles", 120, (255, 255, 255)),
    mesh_lily_flower,
    mesh_lily_centre,
]


# The props the game draws (kinds of the scatter, fishmod/tools/gen_scatter.py), in this order: the game's enum is
# generated from it. "vanilla" kinds draw a model from object_fish (loaded wherever fish are).
PROP_KINDS = [
    ("grass_tuft", "grass_card", "ia8"),
    ("ribbon_weed", "ribbon_card", "ia8"),
    ("tall_weed", "ribbon_card", "ia8"),
    ("broadleaf", "broadleaf_card", "ia8"),
    ("bush_weed", "bush_card", "ia8"),
    ("reed_clump", "reed_card", "ia8"),
    ("cattails", "reed_card", "ia8"),
    ("old_posts", "roots_card", "ia8"),
    ("float_leaves", "float_leaf", "ci4"),
    ("leaf_litter", "leaf_litter", "ci4"),
    ("pebble_patch", "pebbles", "ci4"),
    ("sunken_log", "bark", "ci4"),
    ("branch", "bark", "ci4"),
    ("drowned_tree", "bark", "ci4"),
    ("fallen_tree", "bark", "ci4"),
    ("lily_flower", "lily_flower", "ci4"),
    ("lily_centre", "lily_centre", "ci4"),
]
VANILLA_KINDS = ["lily_pad", "rock"]
DATA = os.path.join(ROOT, "src", "fishmod_data")


def write_props_manifest():
    """The textures and models as an assets.py manifest (into src/fishmod_data/, generated at build time)."""
    import json

    os.makedirs(DATA, exist_ok=True)
    out = "src/fishmod_data/fishmod_props.inc.c"
    textures, models, seen = [], [], set()
    for kind, tex, fmt in PROP_KINDS:
        if tex not in seen:
            seen.add(tex)
            src = os.path.join(ART_ROM, tex + ".png")
            if not os.path.exists(src):
                src = os.path.join(ART, tex + ".png")
            textures.append({"name": "gFishmodProp_" + tex + "Tex", "src": os.path.relpath(src, ROOT),
                             "format": fmt, "out": out})
        models.append({"name": "gFishmodProp_" + kind + "DL", "src": f"fishmod/assets/habitat/{kind}.obj",
                       "texture": "gFishmodProp_" + tex + "Tex", "out": out, "separateMaterial": True})
    with open(os.path.join(DATA, "props_manifest.json"), "w") as f:
        json.dump({"textures": textures, "models": models}, f, indent=1)
    kinds = [k for k, _, _ in PROP_KINDS] + VANILLA_KINDS
    with open(os.path.join(DATA, "fishmod_props_kinds.h"), "w") as f:
        f.write("// Generated by fishmod/tools/gen_habitat.py. Do not edit.\n")
        f.write("// All prop kinds, in the scatter's order; the generated ones have display lists in fishmod_props.inc.c\n")
        f.write("#define FISHMOD_PROP_KINDS(X) \\\n")
        for k in kinds:
            f.write(f"    X({k.upper()}, {k}) \\\n")
        f.write("\n#define FISHMOD_PROP_KINDS_GENERATED(X) \\\n")
        for k, _, _ in PROP_KINDS:
            f.write(f"    X({k.upper()}, {k}) \\\n")
        f.write("\n")
    return kinds


def main():
    for name, (fn, fmt) in TEXTURES.items():
        save(fn(), name)
    if os.path.isdir(VANILLA):
        os.makedirs(ART_ROM, exist_ok=True)
        for name, (fn, fmt) in ROM_TEXTURES.items():
            fn().save(os.path.join(ART_ROM, name + ".png"))
    else:
        print("note: no extracted assets, so the textures edited from vanilla ones aren't made")
    meshes = [f().write() for f in MESHES]
    write_props_manifest()
    for m in meshes:
        print(f"{m.name}: {len(m.v)} verts, {len(m.f)} tris, texture {m.texture}")


if __name__ == "__main__":
    main()
