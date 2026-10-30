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
       (FISHMOD_HW=1: the original-hardware meshes, into fishmod/assets/habitat_hw/, not committed)
"""
import math
import os
import random
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
ART = os.path.join(ROOT, "fishmod", "art", "habitat")
# the original-hardware version's meshes (FISHMOD_HW=1) go elsewhere, should they ever differ: made for that build only,
# and the committed ones stay the emulator version's
MESH = os.path.join(ROOT, "fishmod", "assets",
                    "habitat_hw" if os.environ.get("FISHMOD_HW") == "1" else "habitat")
os.makedirs(MESH, exist_ok=True)
# the meshes the game uses: the same, with each card cut down to its texture's outline (fishmod/tools/card_outline.py,
# made from the ROM's textures, so not committed)
MESH_TRIM = os.path.join(ROOT, "src", "fishmod_data", "habitat_trim")
# small plants whose cards are drawn as one triangle each (round everything that can show): a triangle's setup costs
# the RDP more than the few pixels a tight outline of several triangles saves on a plant this small
MESH_TRI = os.path.join(ROOT, "src", "fishmod_data", "habitat_tri")
# the meshes of kinds whose texture gets a see-through margin (so the triangle has room past the leaves' edges),
# their UVs moved onto the bigger texture
MESH_PAD = os.path.join(ROOT, "src", "fishmod_data", "habitat_pad")
# the far single cards (fishmod/tools/impostor.py) and the half-size far textures, made from the ROM's textures
MESH_IMP = os.path.join(ROOT, "src", "fishmod_data", "habitat_imp")
# The far tiers' distances come from how big things are on screen: the game's 60 degree view over 240 lines puts
# 207.8 pixels per unit of size at a distance of one unit.
PX_PER_UNIT = 207.8
# a plant becomes its single card once it's no taller on screen than this (at scale 1; the game scales it per prop).
# Checked with the exact A/B capture (edge_ab_capture.py, bench flag NOIMP)
IMP_PX = 16
TRI_KINDS = {"grass_tuft", "float_leaves", "leaf_litter", "spatterdock", "lily_flower", "lily_centre"}
# upright plants that get the far single card (fishmod/tools/impostor.py): turned to face the camera, so not the flat
# ones (floating leaves, litter, lily flowers), nor wood and the dock
IMPOSTOR_KINDS = {"grass_tuft", "ribbon_weed", "tall_weed", "broadleaf", "bush_weed", "reed_clump", "cattails",
                  "old_posts", "rush_tuft", "iris", "big_pondweed", "kelp_frond", "vine_strand", "vine_curtain",
                  "bank_roots", "marigold", "fan_weed"}


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


def rom_spatterdock():
    """A round floating leaf from a quarter of the magic bean's platform leaf, mirrored four ways, darkened toward a
    pond green."""
    src = vanilla("objects/object_mamenoki/gMagicBeanPlatformTex.rgba16.png")
    w, h = src.size
    out = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    q = src.resize((w // 2, h // 2), Image.BOX)
    for (fx, fy, ox, oy) in ((False, False, 0, 0), (True, False, w // 2, 0), (False, True, 0, h // 2),
                             (True, True, w // 2, h // 2)):
        t = q.transpose(Image.FLIP_LEFT_RIGHT) if fx else q
        t = t.transpose(Image.FLIP_TOP_BOTTOM) if fy else t
        out.paste(t, (ox, oy))
    res = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    for y in range(h):
        for x in range(w):
            r, g, b, a = out.getpixel((x, y))
            if a < 128 or math.hypot(x - w / 2 + 0.5, y - h / 2 + 0.5) > w / 2 - 0.5:
                continue
            v = luma((r, g, b)) / 255.0
            res.putpixel((x, y), lerp(hexc("#1e3c10"), hexc("#86a046"), v) + (255,))
    return res


def rom_moss():
    """Bed moss: the castle courtyard's hedge (fine, even leaf speckle, tiles) recoloured from its bright garden green to
    a dark olive moss, its contrast kept so the speckle reads at a distance."""
    src = vanilla("scenes/indoors/hairal_niwa_n/hairal_niwa_n_scene_000068F0_Tex.rgba16.png")
    out = Image.new("RGB", src.size)
    pal = ["#26361a", "#34482a", "#465c30", "#5c723a", "#788a48"]
    vals = [luma(src.getpixel((x, y))) for y in range(src.height) for x in range(src.width)]
    lo, hi = min(vals), max(vals)
    for y in range(src.height):
        for x in range(src.width):
            out.putpixel((x, y), ramp(pal, (luma(src.getpixel((x, y))) - lo) / max(hi - lo, 1)))
    return out


def rom_colour(path, size=None):
    """A vanilla texture kept in colour (cut-out kept)."""
    src = vanilla(path)
    if size:
        src = src.resize(size, Image.BOX)
    return src


ROM_TEXTURES = {
    "rush_card": (lambda: ia_card("scenes/misc/kakusiana/kakusiana_room_0_000032A0_Tex.rgba16.png"), "ia8"),
    "iris_card": (lambda: ia_card("objects/object_spot18_obj/gGoronCitySpearLeafTex.rgba16.png"), "ia8"),
    "bigleaf_card": (lambda: ia_card("scenes/misc/kakusiana/kakusiana_room_9_00004B40_Tex.rgba16.png"), "ia8"),
    "spatterdock": (rom_spatterdock, "ci4"),
    "frond_card": (lambda: ia_card("scenes/overworld/spot11/spot11_scene_000164E0_Tex.ia4.png"), "ia8"),
    "vine_card": (lambda: ia_card("scenes/dungeons/Bmori1/Bmori1_room_10_00002A60_Tex.rgba16.png"), "ia8"),
    "curtain_card": (lambda: ia_card("scenes/overworld/spot04/spot04_room_0_00010D08_Tex.rgba16.png"), "ia8"),
    "bankroot_card": (lambda: ia_card("scenes/dungeons/ddan/ddan_room_1_00006F00_Tex.rgba16.png"), "ia8"),
    "marigold": (lambda: rom_colour("scenes/overworld/spot15/spot15_scene_0000BA00_Tex.rgba16.png"), "ci8"),
    "fan_card": (lambda: ia_card("objects/object_jj/gJabuJabuFrillTex.rgba16.png"), "ia8"),
    "reed_card": (rom_reed_card, "ia8"),
    "grass_card": (rom_grass_card, "ia8"),
    "ribbon_card": (rom_ribbon_card, "ia8"),
    "broadleaf_card": (rom_broadleaf_card, "ia8"),
    "bush_card": (rom_bush_card, "ia8"),
    "roots_card": (rom_roots_card, "ia8"),
    "float_leaf": (rom_float_leaf, "ci4"),
    "leaf_litter": (rom_leaf_litter, "ci4"),
    "bark": (rom_bark, "ci4"),
    # the north-beach dock: the Fishing Pond's own weathered planks and plain wood
    "dock_planks": (lambda: rom_colour("scenes/misc/turibori/turibori_scene_0000ACE0_Tex.rgba16.png"), "ci8"),
    "dock_wood": (lambda: rom_colour("scenes/misc/turibori/turibori_scene_000081E0_Tex.rgba16.png"), "ci8"),
    "pebbles": (rom_pebbles, "ci4"),
    "moss": (rom_moss, "ci4"),
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
        self.quads = []  # the faces of each quad (a card: its texture across it), for cutting to the outline

    def vert(self, p, uv, col):
        self.v.append((p[0], p[1], p[2]) + tuple(c / 255.0 for c in col))
        self.vt.append(uv)
        return len(self.v)

    def tri(self, a, b, c):
        self.f.append((a, b, c))

    def quad(self, a, b, c, d):
        self.quads.append((len(self.f), (a, b, c, d)))
        self.f.append((a, b, c))
        self.f.append((a, c, d))

    def _write(self, folder, v, vt, faces):
        os.makedirs(folder, exist_ok=True)
        with open(os.path.join(folder, self.name + ".obj"), "w") as f:
            f.write(f"# generated by fishmod/tools/gen_habitat.py; texture: {self.texture}\n")
            for x in v:
                f.write("v %.2f %.2f %.2f %.4f %.4f %.4f\n" % x)
            for a, b in vt:
                f.write("vt %.4f %.4f\n" % (a, 1.0 - b))  # OBJ's v points up; ours is image rows (down)
            for face in faces:
                f.write("f " + " ".join(f"{i}/{i}" for i in face) + "\n")

    def write(self):
        self._write(MESH, self.v, self.vt, self.f)
        if self.name in TRI_KINDS:
            tri = self.trimmed(triangle=True)
            if tri is not None:
                self._write(MESH_TRI, *tri[:3])
        trimmed = self.trimmed()
        self.trim_stats = None
        if trimmed is not None:
            self._write(MESH_TRIM, *trimmed[:3])
            self.trim_stats = trimmed[3]
        elif os.path.exists(os.path.join(MESH_TRIM, self.name + ".obj")):
            os.remove(os.path.join(MESH_TRIM, self.name + ".obj"))
        return self

    def trimmed(self, triangle=False):
        """The mesh with each card (a quad with the whole texture across it, once or repeated up) cut down to its
        texture's outline. The outline holds every texel that can show, and positions, UVs and colours inside the
        quad are where they were, so the picture is the same. None if nothing here can be cut."""
        path = texture_path(self.texture)
        if path is None or not self.quads:
            return None
        import card_outline

        cut = {}  # first face index -> polygon (verts as (pos, uv, col))
        area_before = area_after = 0.0
        for fi, (a, b, c, d) in self.quads:
            uvs = [self.vt[i - 1] for i in (a, b, c, d)]
            us, vs = sorted({round(t[0], 4) for t in uvs}), sorted({round(t[1], 4) for t in uvs})
            if us != [0.0, 1.0] or len(vs) != 2 or vs[0] != 0.0 or abs(vs[1] - round(vs[1])) > 1e-4:
                continue
            rep = int(round(vs[1]))
            if triangle:
                if rep != 1:
                    continue
                poly = tri_poly(self.texture)
            else:
                poly = card_outline.outline(path, rep)
            if poly is None:
                continue
            # bilinear over the quad, in its own UV (exact for the flat parallelograms the cards are)
            corner = {(round(t[0]), round(t[1] / rep)): i for t, i in zip(uvs, (a, b, c, d))}

            def at(u, v):
                s_, t_ = u, v / rep
                w = {(0, 0): (1 - s_) * (1 - t_), (1, 0): s_ * (1 - t_), (0, 1): (1 - s_) * t_, (1, 1): s_ * t_}
                acc = [0.0] * 6
                for k, wt in w.items():
                    vx = self.v[corner[k] - 1]
                    for j in range(6):
                        acc[j] += vx[j] * wt
                return tuple(acc)

            verts = [(at(u, v), (u, v)) for u, v in poly]
            # the same facing as the quad (its winding in UV)
            def signed(pts):
                return sum(pts[i][0] * pts[(i + 1) % len(pts)][1] - pts[(i + 1) % len(pts)][0] * pts[i][1]
                           for i in range(len(pts)))
            if (signed(uvs) > 0) != (signed([uv for _, uv in verts]) > 0):
                verts.reverse()
            cut[fi] = verts
            area_before += 1.0
            area_after += abs(signed([uv for _, uv in verts])) / 2 / rep
        if not cut:
            return None
        v, vt, faces = [], [], []
        remap = {}
        for fi, face in enumerate(self.f):
            if fi in cut:
                verts = cut[fi]
                base = len(v)
                for pos, uv in verts:
                    v.append(pos)
                    vt.append(uv)
                for tri in ear_clip([uv for _, uv in verts]):
                    faces.append(tuple(base + 1 + j for j in tri))
                continue
            if fi - 1 in cut:
                continue  # the quad's second triangle
            out = []
            for i in face:
                if i not in remap:
                    v.append(self.v[i - 1])
                    vt.append(self.vt[i - 1])
                    remap[i] = len(v)
                out.append(remap[i])
            faces.append(tuple(out))
        return v, vt, faces, (len(cut), area_after / area_before)


def tex_padding(name):
    """For a one-triangle kind's texture: (path of the texture to use, (pad x, pad y, w, h, W, H)) where the texture
    gets a see-through margin if no triangle fits it clamped as it is; pad is None when it needs none."""
    import card_outline
    path = texture_path(name)
    if card_outline.triangle(path) is not None:
        return path, None
    img = Image.open(path).convert("RGBA")
    w, h = img.size
    W, H = w * 2, h * 2
    big = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    big.paste(img, ((W - w) // 2, (H - h) // 2))
    out = os.path.join(ART_ROM, name + "_pad.png")
    big.save(out)
    return out, ((W - w) // 2, (H - h) // 2, w, h, W, H)


def tri_poly(name):
    """The card's one triangle, in the original texture's UV (it may reach past 0..1)."""
    import card_outline
    path, pad = tex_padding(name)
    r = card_outline.triangle(path)
    if r is None:
        return None
    tri = r[0]
    if pad is None:
        return tri
    px, py, w, h, W, H = pad
    return [((u * W - px) / w, (v * H - py) / h) for u, v in tri]


def remap_obj(src, dst, pad):
    """An OBJ with its UVs moved onto the padded texture."""
    px, py, w, h, W, H = pad
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    with open(src) as f, open(dst, "w") as o:
        for line in f:
            if line.startswith("vt "):
                u, v = map(float, line.split()[1:3])
                v = 1.0 - v  # (OBJ's v is up)
                u, v = (u * w + px) / W, (v * h + py) / H
                line = "vt %.4f %.4f\n" % (u, 1.0 - v)
            o.write(line)
    return dst


def texture_path(name):
    """A cut-out texture's PNG (the ROM-made ones first, as the asset manifest picks them), or None."""
    for folder in (ART_ROM, ART):
        path = os.path.join(folder, name + ".png")
        if os.path.exists(path):
            return path
    return None


def ear_clip(poly):
    """Triangles (index triples, the polygon's winding) for a simple polygon."""
    def signed(pts):
        return sum(pts[i][0] * pts[(i + 1) % len(pts)][1] - pts[(i + 1) % len(pts)][0] * pts[i][1]
                   for i in range(len(pts)))
    sign = 1 if signed(poly) > 0 else -1
    idx = list(range(len(poly)))
    tris = []

    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])

    def in_tri(p, a, b, c):
        d1, d2, d3 = cross(a, b, p) * sign, cross(b, c, p) * sign, cross(c, a, p) * sign
        return d1 >= 0 and d2 >= 0 and d3 >= 0

    guard = 0
    while len(idx) > 3 and guard < 1000:
        guard += 1
        for k in range(len(idx)):
            i0, i1, i2 = idx[k - 1], idx[k], idx[(k + 1) % len(idx)]
            a, b, c = poly[i0], poly[i1], poly[i2]
            if cross(a, b, c) * sign <= 0:
                continue  # reflex (or flat)
            if any(in_tri(poly[j], a, b, c) for j in idx if j not in (i0, i1, i2)):
                continue
            tris.append((i0, i1, i2))
            del idx[k]
            break
        else:
            raise ValueError("ear_clip: no ear (not a simple polygon?)")
    tris.append(tuple(idx))
    return tris


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


# Patches: a few clumps baked into one mesh, so the commonest small plants are a third to a quarter as many props
# to place and draw (each prop costs a matrix and a draw call however small it is). gen_scatter.py thins these kinds
# by the same number.
PATCH = {"grass_tuft": 4, "ribbon_weed": 3, "bush_weed": 2, "tall_weed": 2}


def patch(name, count, radius, seed, make):
    """`count` copies of make(i) (a Mesh), each moved, turned and sized a little differently, as one mesh."""
    rnd = random.Random(seed)
    m = None
    for i in range(count):
        part = make(i)
        if m is None:
            m = Mesh(name, part.texture)
        ang = rnd.uniform(0, 2 * math.pi)
        r = radius * (0.0 if i == 0 else rnd.uniform(0.45, 1.0))
        ox, oz = math.cos(ang) * r, math.sin(ang) * r
        turn = rnd.uniform(0, 2 * math.pi)
        c, sn = math.cos(turn), math.sin(turn)
        sc = rnd.uniform(0.75, 1.15)
        base = len(m.v)
        for (x, y, z, cr, cg, cb), uv in zip(part.v, part.vt):
            m.v.append((ox + (x * c - z * sn) * sc, y * sc, oz + (x * sn + z * c) * sc, cr, cg, cb))
            m.vt.append(uv)
        for fi, quad in part.quads:
            m.quads.append((len(m.f) + fi, tuple(base + j for j in quad)))
        for face in part.f:
            m.f.append(tuple(base + j for j in face))
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


def mesh_moss_patch():
    """A low, lumpy mat of moss rising out of the bed: an 8-sided fan (the console pays per triangle and per prop, so
    fewer, bigger mats) with a ragged rim sunk under the sand, so the bed
    cuts the edge (an irregular outline from the terrain, no see-through texels), only a few units high in the
    middle, so from the low under-water camera it covers few scanlines (cheap) and reads as a carpet, not a mound.
    The texture is laid on from above and repeats, so the patch's size doesn't stretch it."""
    rnd = random.Random(121)
    m = Mesh("moss_patch", "moss")
    radius, height, sides, tile = 72.0, 3.0, 8, 26.0
    def uv(x, z):
        return (x / tile, z / tile)
    c = m.vert((rnd.uniform(-6, 6), height, rnd.uniform(-6, 6)), uv(0, 0), (240, 245, 225))
    ring = []
    for k in range(sides):
        t = 2 * math.pi * (k + rnd.uniform(-0.35, 0.35)) / sides
        # an uneven, lumpy edge: two slow waves round the rim and a little jitter (not alternating in and out,
        # which made star shapes)
        r = radius * (0.78 + 0.14 * math.sin(2 * t + 0.7) + 0.1 * math.sin(3 * t + 2.1) + rnd.uniform(-0.05, 0.05))
        x, z = math.cos(t) * r, math.sin(t) * r
        # the rim well under the sand: where the bed rises or dips a little, it still cuts the edge, never a gap
        ring.append(m.vert((x, -3.5, z), uv(x, z), (175, 185, 160)))  # (a little darker where it meets the sand)
    for k in range(sides):
        m.tri(c, ring[(k + 1) % sides], ring[k])
    return m


def mesh_branch():
    m = Mesh("branch", "bark")
    dark, light = hexc("#2b2311"), hexc("#707346")
    cylinder(m, (-70, 4, 0), (70, 7, 10), 4, 2.5, 4, dark, light, v_repeat=3.0)
    cylinder(m, (10, 6, 4), (35, 30, 30), 2.2, 1, 3, dark, light, v_repeat=1.0)
    cylinder(m, (-30, 5, 2), (-45, 22, -20), 2, 1, 3, dark, light, v_repeat=1.0)
    return m


# The dock on the north beach (zone 15): placed by hand, not scattered. Its own space has the shore end of the deck at
# the origin, the deck's top at y 0, and the dock running out along +z. gen_collision.py makes its collision from
# the same numbers, and the game draws it at DOCK.
DOCK = {"scene": "SCENE_LAKE_HYLIA", "x": -400, "y": -1288, "z": 3080}
DOCK_LENGTH = 420
DOCK_WIDTH = 60
DOCK_THICK = 6
DOCK_PILING_ROWS = (100, 200, 300, 400)  # pairs of pilings, this far out
DOCK_PILING_X = 25
DOCK_PILING_R = 5
DOCK_PILING_DEPTH = 150                   # down from the deck: well into the bed all along
DOCK_MOORING = (-25, 412, 30)              # x, z, height above the deck


def quad_uv(m, corners, col, uv):
    """A quad (corners in order round it) with UVs (u0, v0, u1, v1) across it."""
    u0, v0, u1, v1 = uv
    ids = [m.vert(c, t, col) for c, t in zip(corners, ((u0, v0), (u1, v0), (u1, v1), (u0, v1)))]
    m.quad(*ids)


def mesh_dock_deck():
    """The deck: weathered planks across it, top, sides, far end and underside (seen from under the water)."""
    m = Mesh("dock_deck", "dock_planks")
    hw, L, t = DOCK_WIDTH / 2, DOCK_LENGTH, DOCK_THICK
    top, side, under = (235, 230, 220), (170, 165, 155), (110, 110, 105)
    rep = L / 30.0  # one plank-texture tile per 30 units of deck
    quad_uv(m, ((-hw, 0, 0), (hw, 0, 0), (hw, 0, L), (-hw, 0, L)), top, (0, 0, 1, rep))
    quad_uv(m, ((-hw, -t, L), (hw, -t, L), (hw, -t, 0), (-hw, -t, 0)), under, (0, 0, 1, rep))
    quad_uv(m, ((hw, -t, 0), (hw, -t, L), (hw, 0, L), (hw, 0, 0)), side, (0, 0.8, rep, 1))
    quad_uv(m, ((-hw, -t, L), (-hw, -t, 0), (-hw, 0, 0), (-hw, 0, L)), side, (0, 0.8, rep, 1))
    quad_uv(m, ((hw, -t, L), (-hw, -t, L), (-hw, 0, L), (hw, 0, L)), side, (0, 0.8, 1, 1))
    return m


def mesh_dock_posts():
    """The pilings (pairs, down into the bed), a cross beam under the deck at each pair, and a mooring post."""
    m = Mesh("dock_posts", "dock_wood")
    dark, light = hexc("#4a3a28"), hexc("#a08a68")
    for z in DOCK_PILING_ROWS:
        for x in (-DOCK_PILING_X, DOCK_PILING_X):
            cylinder(m, (x, -DOCK_PILING_DEPTH, z), (x, -DOCK_THICK, z), DOCK_PILING_R, DOCK_PILING_R, 6, dark, light,
                     v_repeat=4.0)
        cylinder(m, (-DOCK_PILING_X - 8, -DOCK_THICK - 4, z), (DOCK_PILING_X + 8, -DOCK_THICK - 4, z), 4, 4, 4, dark,
                 light, v_repeat=1.5)
    x, z, h = DOCK_MOORING
    cylinder(m, (x, -DOCK_PILING_DEPTH, z), (x, h, z), 6, 6, 6, dark, light, v_repeat=5.0)
    return m


def mesh_spatterdock():
    """Floating leaves: three round pads on the surface."""
    rnd = random.Random(121)
    m = Mesh("spatterdock", "spatterdock")
    white = (255, 255, 255)
    for k in range(3):
        cx, cz = rnd.uniform(-14, 14), rnd.uniform(-14, 14)
        sz = rnd.uniform(9, 13)
        ids = [m.vert((cx + dx * sz, 0.5 + k * 0.2, cz + dz * sz), uv, white)
               for (dx, dz), uv in zip(((-1, -1), (1, -1), (1, 1), (-1, 1)), ((0, 0), (1, 0), (1, 1), (0, 1)))]
        m.quad(ids[0], ids[3], ids[2], ids[1])
    return m


def mesh_curtain():
    """Vines hanging down from the surface (under the ruins' blocks and the docks): cards whose top is at y 0."""
    m = crossed_cards("vine_curtain", "curtain_card", 50, 90, 2, hexc("#8a9a50"), hexc("#1c2c10"), 122)
    m.v = [(x, y - 90.0, z, r, g, b) for (x, y, z, r, g, b) in m.v]
    # the texture's top rows are the solid edge of the overhang it came from (a flat dark bar, a cross seen from
    # above): the cards start below it. In open water a spatterdock floats on top (gen_scatter.py accents), so the
    # vines read as its roots.
    m.vt = [(u, 0.14 + v * 0.86) for u, v in m.vt]
    return m


MESHES = [
    lambda: crossed_cards("rush_tuft", "rush_card", 40, 26, 2, hexc("#24401a"), hexc("#9aa050"), 111),
    lambda: crossed_cards("iris", "iris_card", 30, 85, 3, hexc("#2a4a1c"), hexc("#a0b060"), 112, lean=4),
    lambda: crossed_cards("big_pondweed", "bigleaf_card", 40, 52, 2, hexc("#1a3412"), hexc("#7a8a40"), 113),
    mesh_spatterdock,
    # one card, turned to face the camera in the game (fishmod_scatter.c faceCam): a thin frond, so the turn doesn't
    # show, at a third of the three crossed cards' cost (the console pays per scanline per card)
    lambda: crossed_cards("kelp_frond", "frond_card", 26, 180, 1, hexc("#0c200a"), hexc("#6a7a38"), 114,
                          lean=22),
    lambda: crossed_cards_v("vine_strand", "vine_card", 28, 150, 2, hexc("#122a0e"), hexc("#7a8a48"), 115,
                            lean=10, v_repeat=2.0),
    mesh_curtain,
    lambda: crossed_cards("bank_roots", "bankroot_card", 60, 70, 2, hexc("#2a2014"), hexc("#8a7a58"), 116),
    lambda: crossed_cards("marigold", "marigold", 28, 22, 2, (200, 200, 200), (255, 255, 255), 117),
    lambda: crossed_cards("fan_weed", "fan_card", 44, 44, 2, hexc("#2a3a1a"), hexc("#8a9a58"), 118),

    lambda: patch("grass_tuft", PATCH["grass_tuft"], 26, 204, lambda i: crossed_cards(
        "grass_tuft", "grass_card", 34, 16, 1, hexc("#1c3a12"), hexc("#6c7a3b"), 104 + i)),
    # one clump stretched up the whole height (the texture once, not twice: twice stacked a second clump on the
    # first's blade tips, pinched in the middle), leaning a little
    lambda: patch("tall_weed", PATCH["tall_weed"], 30, 205, lambda i: crossed_cards_v(
        "tall_weed", "ribbon_card", 26, 140, 1, hexc("#0d260b"), hexc("#5a6a30"), 105 + i, lean=6, v_repeat=1.0)),
    lambda: crossed_cards("broadleaf", "broadleaf_card", 38, 40, 2, hexc("#1a3a12"), hexc("#7a8a40"), 106),
    lambda: crossed_cards("old_posts", "roots_card", 70, 60, 2, hexc("#2b2311"), hexc("#8a8a70"), 107),
    lambda: crossed_cards("cattails", "reed_card", 44, 110, 3, hexc("#3a4a18"), hexc("#b8b070"), 108, lean=3),
    lambda: mesh_flat_scatter("float_leaves", "float_leaf", 5, 22, 7, 0.5, (255, 255, 255), 109),
    lambda: mesh_flat_scatter("leaf_litter", "leaf_litter", 9, 40, 5, 0.6, (200, 200, 200), 110),
    mesh_branch,
    mesh_dock_deck,
    mesh_dock_posts,

    lambda: crossed_cards("reed_clump", "reed_card", 40, 95, 3, hexc("#184217"), hexc("#8f9342"), 101, lean=4),
    lambda: patch("ribbon_weed", PATCH["ribbon_weed"], 22, 202, lambda i: crossed_cards(
        "ribbon_weed", "ribbon_card", 22, 80, 1, hexc("#133611"), hexc("#6c7a3b"), 102 + i, lean=10)),
    lambda: patch("bush_weed", PATCH["bush_weed"], 30, 203, lambda i: crossed_cards(
        "bush_weed", "bush_card", 46, 42, 2, hexc("#082b07"), hexc("#525d28"), 103 + i)),
    mesh_log,
    lambda: mesh_decal("pebble_patch", "pebbles", 120, (255, 255, 255)),
    mesh_moss_patch,
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
    ("rush_tuft", "rush_card", "ia8"),
    ("iris", "iris_card", "ia8"),
    ("big_pondweed", "bigleaf_card", "ia8"),
    ("spatterdock", "spatterdock", "ci4"),
    ("kelp_frond", "frond_card", "ia8"),
    ("vine_strand", "vine_card", "ia8"),
    ("vine_curtain", "curtain_card", "ia8"),
    ("bank_roots", "bankroot_card", "ia8"),
    ("marigold", "marigold", "ci8"),
    ("fan_weed", "fan_card", "ia8"),
    ("dock_deck", "dock_planks", "ci8"),
    ("dock_posts", "dock_wood", "ci8"),
    ("lily_flower", "lily_flower", "ci4"),
    ("lily_centre", "lily_centre", "ci4"),
    ("moss_patch", "moss", "ci4"),
]
VANILLA_KINDS = ["pond_reed", "lily_pad", "rock"]  # object_fish's own (the Fishing Pond's reed, pads, rocks)
DATA = os.path.join(ROOT, "src", "fishmod_data")


def half_texture(src, out):
    """The texture at half size: colour averaged over the texels that show, see-through where under half of them do.
    None if it's already small (8 texels on a side)."""
    img = Image.open(src).convert("RGBA")
    w, h = img.size
    if min(w, h) <= 8:
        return None
    a = np.asarray(img).astype(float).reshape(h // 2, 2, w // 2, 2, 4)
    alpha = a[..., 3]
    weight = (alpha >= 128).astype(float)
    n = weight.sum((1, 3))
    rgb = (a[..., :3] * weight[..., None]).sum((1, 3)) / np.maximum(n, 1)[..., None]
    rgb_all = a[..., :3].mean((1, 3))
    rgb = np.where(n[..., None] > 0, rgb, rgb_all)
    o = np.zeros((h // 2, w // 2, 4), np.uint8)
    o[..., :3] = np.clip(rgb, 0, 255)
    o[..., 3] = np.where(n >= 2, 255, 0)
    Image.fromarray(o).save(out)
    return out


def half_distance(mesh_path, tex_path):
    """How far away (at scale 1) the half-size texture can't be told apart: a texel of the full texture is then
    smaller than half a pixel on screen. From the mesh's UVs: the most texels per unit along any edge, either way
    across the texture (upright cards and flat leaves alike)."""
    import impostor
    pos, uv, _, tris = impostor.load_obj(mesh_path)
    tw, th = Image.open(tex_path).size
    best = 0.0
    for (a, ta), (b, tb), (c, tc) in tris:
        for (i, ti), (j, tj) in (((a, ta), (b, tb)), ((b, tb), (c, tc)), ((c, tc), (a, ta))):
            d = math.dist(pos[i], pos[j])
            if d > 1e-3 and ti >= 0 and tj >= 0:
                best = max(best, math.hypot((uv[ti][0] - uv[tj][0]) * tw, (uv[ti][1] - uv[tj][1]) * th) / d)
    return PX_PER_UNIT * 2 / best if best > 0 else 0.0


def rock_vc():
    """Vanilla's Fishing Pond rock (its geometry, UVs and texture, from the extracted assets) with vertex colours in
    place of its lighting normals, drawn unlit like the plants (lit by the scene's light colour, by time of day): moss
    on the faces that look up, darker toward the base where it sits in the silt, a little of the sun's side baked in.
    Returns (obj path, png path), or None without the extracted assets."""
    obj = os.path.join(VANILLA, "objects", "object_fish")
    vtx_src = os.path.join(obj, "gFishingRockModelVtx.inc.c")
    dl_src = os.path.join(obj, "gFishingRockModelDL.inc.c")
    tex_src = os.path.join(obj, "gFishingRockTex.rgba16.png")
    if not all(os.path.exists(p) for p in (vtx_src, dl_src, tex_src)):
        return None
    import re as _re
    verts = [tuple(int(v, 0) for v in m) for m in
             _re.findall(r"VTX\(\s*(-?\w+),\s*(-?\w+),\s*(-?\w+),\s*(-?\w+),\s*(-?\w+),\s*(\w+),\s*(\w+),\s*(\w+),",
                         open(vtx_src).read())]
    tris = []
    for m in _re.finditer(r"gsSP2Triangles\((\d+), (\d+), (\d+), 0, (\d+), (\d+), (\d+), 0\)", open(dl_src).read()):
        v = list(map(int, m.groups()))
        tris += [v[:3], v[3:]]
    tw, th = Image.open(tex_src).size
    sun = (0.3, 1.0, 0.2)
    sl = math.sqrt(sum(c * c for c in sun))
    sun = tuple(c / sl for c in sun)
    os.makedirs(MESH_IMP, exist_ok=True)
    out = os.path.join(MESH_IMP, "rock_vc.obj")
    with open(out, "w") as f:
        f.write("# vanilla's rock with vertex colours (gen_habitat.py rock_vc)\n")
        for x, y, z, s_, t_, nx, ny, nz in verts:
            n = [(c - 256 if c > 127 else c) / 127.0 for c in (nx, ny, nz)]
            ln = math.sqrt(sum(c * c for c in n)) or 1.0
            n = [c / ln for c in n]
            up = max(0.0, n[1])
            moss = max(0.0, min(1.0, (up - 0.35) / 0.45)) * 0.85
            ao = 0.72 + 0.28 * max(0.0, min(1.0, (y + 5) / 55.0))
            lit = 0.8 + 0.2 * max(0.0, sum(a * b for a, b in zip(n, sun)))
            col = [(1 - moss) * 1.0 + moss * m for m in (0.70, 0.90, 0.52)]
            col = [min(1.0, c * ao * lit) for c in col]
            f.write(f"v {x} {y} {z} {col[0]:.3f} {col[1]:.3f} {col[2]:.3f}\n")
        for x, y, z, s_, t_, *_ in verts:
            f.write(f"vt {s_ / 32.0 / tw:.5f} {1.0 - t_ / 32.0 / th:.5f}\n")
        for a, b, c in tris:
            f.write(f"f {a + 1}/{a + 1} {b + 1}/{b + 1} {c + 1}/{c + 1}\n")
    png = os.path.join(ART_ROM, "rock_vc.png")
    os.makedirs(ART_ROM, exist_ok=True)
    Image.open(tex_src).convert("RGBA").save(png)
    return out, png


def write_props_manifest():
    """The textures and models as an assets.py manifest (into src/fishmod_data/, generated at build time)."""
    import json

    os.makedirs(DATA, exist_ok=True)
    out = "src/fishmod_data/fishmod_props.inc.c"
    textures, models, materials, seen = [], [], [], set()
    tri_kinds_built = []
    seen_half, have_half, half_kinds, imp_kinds = set(), set(), [], []
    for kind, tex, fmt in PROP_KINDS:
        pad = None
        if kind in TRI_KINDS:
            _, pad = tex_padding(tex)
        if tex not in seen:
            seen.add(tex)
            src = os.path.join(ART_ROM, tex + ".png")
            if not os.path.exists(src):
                src = os.path.join(ART, tex + ".png")
            if pad is not None:
                src = os.path.join(ART_ROM, tex + "_pad.png")
            textures.append({"name": "gFishmodProp_" + tex + "Tex", "src": os.path.relpath(src, ROOT),
                             "format": fmt, "out": out})
        mesh = os.path.join(MESH_TRIM, kind + ".obj")
        if not os.path.exists(mesh):
            mesh = os.path.join(MESH, kind + ".obj")
        far = os.path.join(MESH, kind + ".obj")
        tri = os.path.join(MESH_TRI, kind + ".obj")
        is_tri = kind in TRI_KINDS and os.path.exists(tri)
        if pad is not None:
            mesh = remap_obj(mesh, os.path.join(MESH_PAD, kind + ".obj"), pad)
            far = remap_obj(far, os.path.join(MESH_PAD, kind + "_far.obj"), pad)
            if is_tri:
                tri = remap_obj(tri, os.path.join(MESH_PAD, kind + "_tri.obj"), pad)
        texname = "gFishmodProp_" + tex + "Tex"
        # far away (past the near tier): the texture at half size, from where a texel is under half a pixel
        tex_src = os.path.join(ART_ROM, tex + ".png")
        if not os.path.exists(tex_src):
            tex_src = os.path.join(ART, tex + ".png")
        if pad is not None:
            tex_src = os.path.join(ART_ROM, tex + "_pad.png")
        halfname = None
        half_png = os.path.join(MESH_IMP, tex + "_half.png")
        os.makedirs(MESH_IMP, exist_ok=True)
        if tex not in seen_half:
            seen_half.add(tex)
            if half_texture(tex_src, half_png):
                textures.append({"name": "gFishmodProp_" + tex + "HalfTex", "src": os.path.relpath(half_png, ROOT),
                                 "format": fmt, "out": out})
                have_half.add(tex)
        if tex in have_half:
            halfname = "gFishmodProp_" + tex + "HalfTex"
            half_kinds.append((kind, tex, round(half_distance(tri if is_tri else far, tex_src))))
        if kind in IMPOSTOR_KINDS:
            import impostor
            r = impostor.impostor(kind, os.path.join(MESH, kind + ".obj"), texture_path(tex), MESH_IMP)
            if r is not None:
                textures.append({"name": "gFishmodProp_" + kind + "ImpTex", "src": os.path.relpath(r["png"], ROOT),
                                 "format": "ci4", "out": out})
                models.append({"name": "gFishmodProp_" + kind + "DLImp", "src": os.path.relpath(r["obj"], ROOT),
                               "texture": "gFishmodProp_" + kind + "ImpTex", "out": out, "separateMaterial": True,
                               "clamp": True})
                imp_kinds.append((kind, round(r["height"] * PX_PER_UNIT / IMP_PX)))
        if is_tri:
            # one triangle per card, near and far (clamped texture); the cut outline kept for the bench's comparison
            models.append({"name": "gFishmodProp_" + kind + "DL", "src": os.path.relpath(tri, ROOT),
                           "texture": texname, "out": out, "separateMaterial": True, "clamp": True})
            models.append({"name": "gFishmodProp_" + kind + "DLFar", "src": os.path.relpath(tri, ROOT),
                           "texture": texname, "out": out, "separateMaterial": True, "clamp": True})
            if halfname:
                models.append({"name": "gFishmodProp_" + kind + "DLFarH", "src": os.path.relpath(tri, ROOT),
                               "texture": halfname, "out": out, "separateMaterial": True, "clamp": True})
            models.append({"name": "gFishmodProp_" + kind + "DLAlt", "src": os.path.relpath(mesh, ROOT),
                           "texture": texname, "out": out, "separateMaterial": True, "clamp": True})
            tri_kinds_built.append(kind)
            continue
        models.append({"name": "gFishmodProp_" + kind + "DL", "src": os.path.relpath(mesh, ROOT),
                       "texture": texname, "out": out, "separateMaterial": True})
        # far away: the whole cards, uncut. The same picture (the cut only drops texels that never show) in a third
        # of the triangles, and far props are few pixels each, so their cost is per triangle (hwbench-2)
        models.append({"name": "gFishmodProp_" + kind + "DLFar", "src": os.path.relpath(far, ROOT),
                       "texture": texname, "out": out, "separateMaterial": True})
        if halfname:
            # and from where a texel is under half a pixel, with the texture at half size (its own UVs)
            models.append({"name": "gFishmodProp_" + kind + "DLFarH", "src": os.path.relpath(far, ROOT),
                           "texture": halfname, "out": out, "separateMaterial": True})
    # the rocks with vertex colours (they stay the vanilla kind ROCK; fishmod_scatter.c draws them with these)
    rv = rock_vc()
    if rv is not None:
        textures.append({"name": "gFishmodProp_rockvcTex", "src": os.path.relpath(rv[1], ROOT), "format": "ci8",
                         "out": out})
        models.append({"name": "gFishmodProp_rockvcDL", "src": os.path.relpath(rv[0], ROOT),
                       "texture": "gFishmodProp_rockvcTex", "out": out, "separateMaterial": True})
    with open(os.path.join(DATA, "props_manifest.json"), "w") as f:
        json.dump({"textures": textures, "materials": materials, "models": models}, f, indent=1)
    kinds = [k for k, _, _ in PROP_KINDS] + VANILLA_KINDS
    with open(os.path.join(DATA, "fishmod_props_kinds.h"), "w") as f:
        f.write("// Generated by fishmod/tools/gen_habitat.py. Do not edit.\n")
        f.write("// All prop kinds, in the scatter's order; the generated ones have display lists in fishmod_props.inc.c\n")
        f.write("#define FISHMOD_PROP_KINDS(X) \\\n")
        for k in kinds:
            f.write(f"    X({k.upper()}, {k}) \\\n")
        f.write("\n// the north-beach dock (placed by hand): its scene and where its shore end is\n")
        f.write(f"#define FISHMOD_DOCK_SCENE {DOCK['scene']}\n")
        f.write(f"#define FISHMOD_DOCK_X {DOCK['x']}\n#define FISHMOD_DOCK_Y {DOCK['y']}\n#define FISHMOD_DOCK_Z {DOCK['z']}\n")
        f.write("\n#define FISHMOD_PROP_KINDS_GENERATED(X) \\\n")
        for k, _, _ in PROP_KINDS:
            f.write(f"    X({k.upper()}, {k}) \\\n")
        f.write("\n// the kinds drawn one triangle per card (with the cut outline kept as ...DLAlt for the bench)\n")
        f.write("#define FISHMOD_PROP_KINDS_TRI(X) \\\n")
        for k in tri_kinds_built:
            f.write(f"    X({k.upper()}, {k}) \\\n")
        f.write("\n// the kinds with a far model on the half-size texture (...DLFarH): kind, name, texture, from how far (scale 1)\n")
        f.write("#define FISHMOD_PROP_KINDS_HALF(X) \\\n")
        for k, tex, d in half_kinds:
            f.write(f"    X({k.upper()}, {k}, {tex}, {d}) \\\n")
        f.write("\n// the kinds with a far single card (...DLImp, fishmod/tools/impostor.py): kind, from how far (scale 1)\n")
        f.write("#define FISHMOD_PROP_KINDS_IMP(X) \\\n")
        for k, d in imp_kinds:
            f.write(f"    X({k.upper()}, {k}, {d}) \\\n")
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
        trim = f"; {m.trim_stats[0]} cards cut to {m.trim_stats[1]:.0%} of their area" if m.trim_stats else ""
        print(f"{m.name}: {len(m.v)} verts, {len(m.f)} tris, texture {m.texture}{trim}")


if __name__ == "__main__":
    main()
