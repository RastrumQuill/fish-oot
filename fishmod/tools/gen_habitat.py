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


def card_blades(w, h, blades, seed, width=(1.5, 3.0), bend=4.0, taper=True):
    """Greyscale + alpha card of grass-like blades (an IA texture: tinted by vertex colour)."""
    rnd = random.Random(seed)
    s = 4
    img = Image.new("LA", (w * s, h * s), (0, 0))
    px = img.load()
    for _ in range(blades):
        x0 = rnd.uniform(0.15, 0.85) * w
        top = rnd.uniform(0.0, 0.35) * h
        lean = rnd.uniform(-bend, bend)
        bw = rnd.uniform(*width)
        shade = rnd.uniform(0.6, 1.0)
        for yy in range(int(top * s), h * s):
            t = (yy / s - top) / (h - top)  # 0 at the tip, 1 at the base
            cx = x0 + lean * (1 - t) ** 2
            half = bw * (0.25 + 0.75 * t if taper else 1.0) * 0.5
            for xx in range(int((cx - half) * s), int((cx + half) * s) + 1):
                if 0 <= xx < w * s:
                    # lighter toward the tip, darker down the middle edge
                    edge = abs(xx / s - cx) / max(half, 0.01)
                    v = int(255 * shade * (0.75 + 0.25 * (1 - t)) * (1.0 - 0.25 * edge))
                    if v > px[xx, yy][0] or px[xx, yy][1] == 0:
                        px[xx, yy] = (v, 255)
    img = img.resize((w, h), Image.BOX)
    # 1-bit alpha (as the N64 alpha compare uses it): solid where the blade covers most of a texel
    out = Image.new("LA", (w, h))
    for y in range(h):
        for x in range(w):
            v, a = img.getpixel((x, y))
            out.putpixel((x, y), (min(255, int(v * 255 / max(a, 1))) if a else 0, 255 if a > 110 else 0))
    return out


def tex_reed():
    return card_blades(32, 64, 14, 11, width=(1.6, 2.6), bend=5.0)


def tex_ribbon():
    """Eelgrass ribbons: few, long, flat, bending one way (the current)."""
    rnd = random.Random(21)
    w, h, s = 16, 64, 4
    img = Image.new("LA", (w * s, h * s), (0, 0))
    px = img.load()
    for r in range(4):
        x0 = (r + 0.5) * w / 4 + rnd.uniform(-1, 1)
        top = rnd.uniform(0.0, 0.25) * h
        phase = rnd.uniform(0, math.pi)
        bw = rnd.uniform(2.2, 3.0)
        for yy in range(int(top * s), h * s):
            t = (yy / s - top) / (h - top)
            cx = x0 + 2.2 * math.sin(phase + t * 4.0) * (1 - t) + 2.0 * (1 - t)
            half = bw * 0.5
            for xx in range(int((cx - half) * s), int((cx + half) * s) + 1):
                if 0 <= xx < w * s:
                    v = int(255 * (0.6 + 0.4 * (1 - t)) * (0.85 + 0.15 * math.sin(yy * 0.3)))
                    px[xx, yy] = (v, 255)
    img = img.resize((w, h), Image.BOX)
    out = Image.new("LA", (w, h))
    for y in range(h):
        for x in range(w):
            v, a = img.getpixel((x, y))
            out.putpixel((x, y), (min(255, int(v * 255 / max(a, 1))) if a else 0, 255 if a > 110 else 0))
    return out


def tex_bush():
    """Coontail / milfoil: soft bushy stems with irregular whorls of fine leaves."""
    rnd = random.Random(31)
    w, h, s = 32, 32, 4
    img = Image.new("LA", (w * s, h * s), (0, 0))
    px = img.load()
    for _ in range(6):
        x0, lean = rnd.uniform(9, 23), rnd.uniform(-6, 6)
        top = rnd.uniform(1, 8)
        y = top
        while y < h - 1:
            t = (y - top) / (h - top)
            cx = x0 + lean * (1 - t) ** 1.5
            # a whorl: short leaves fanning out and up at random angles
            for _ in range(rnd.randint(3, 6)):
                ang = rnd.uniform(-2.6, -0.5) if rnd.random() < 0.5 else rnd.uniform(-2.6, -0.5) + math.pi * 0.0
                ln = rnd.uniform(2.0, 4.5) * (0.6 + 0.4 * (1 - t))
                side = rnd.choice((-1, 1))
                for k in range(int(ln * s)):
                    xx = int((cx + side * math.cos(ang) * k / s) * s)
                    yy = int((y + math.sin(ang) * k / s * 0.6) * s)
                    if 0 <= xx < w * s and 0 <= yy < h * s:
                        v = int(255 * (0.55 + 0.45 * rnd.random()) * (0.7 + 0.3 * (1 - t)))
                        for d in (0, 1):
                            if xx + d < w * s:
                                px[xx + d, yy] = (v, 255)
            y += rnd.uniform(1.3, 2.4)
        for yy in range(int(top * s), h * s):
            t = (yy / s - top) / (h - top)
            xx = int((x0 + lean * (1 - t) ** 1.5) * s)
            for d in (-1, 0, 1):
                if 0 <= xx + d < w * s:
                    px[xx + d, yy] = (110, 255)
    img = img.resize((w, h), Image.BOX)
    out = Image.new("LA", (w, h))
    for y in range(h):
        for x in range(w):
            v, a = img.getpixel((x, y))
            out.putpixel((x, y), (min(255, int(v * 255 / max(a, 1))) if a else 0, 255 if a > 70 else 0))
    return out


def tex_pebbles():
    """Rounded river stones packed together (the inflow bed), colours from the pond's pebbles."""
    rnd = random.Random(41)
    pal = [hexc(c) for c in PALETTES["pond_pebbles"]] + [hexc(c) for c in PALETTES["lake_cobble"][:4]]
    w = h = 32
    img = Image.new("RGB", (w, h), hexc("#19170d"))
    px = img.load()
    stones = []
    # packed like a river bed: stones of a few sizes, touching but not overlapping (tiling across the edges)
    for _ in range(600):
        x, y, r = rnd.uniform(0, w), rnd.uniform(0, h), rnd.uniform(2.6, 5.0)
        ok = True
        for sx, sy, sr, _, _ in stones:
            dx = min(abs(x - sx), w - abs(x - sx))
            dy = min(abs(y - sy), h - abs(y - sy))
            if math.hypot(dx, dy) < (r + sr) * 0.92:
                ok = False
                break
        if ok:
            stones.append((x, y, r, rnd.uniform(0.75, 1.0), rnd.choice([pal[0], pal[3], pal[4], pal[7], pal[9], pal[10], pal[11]])))
    for y in range(h):
        for x in range(w):
            best = None
            for sx, sy, r, sq, col in stones:
                # tile seamlessly: nearest copy
                dx = min(abs(x - sx), w - abs(x - sx))
                dy = min(abs(y - sy), h - abs(y - sy))
                d = math.hypot(dx, dy / sq) / (r * 1.32)  # grown so neighbours meet
                if d < 1.0 and (best is None or d < best[0]):
                    best = (d, col, (x - sx) / r, (y - sy) / r)
            if best:
                d, col, ux, uy = best
                light = min(1.08, 1.02 - 0.55 * d ** 3 - 0.22 * uy - 0.12 * ux)  # lit from the top left, a dark rim
                grain = 0.9 + 0.2 * ((x * 7 + y * 13) * 2654435761 % 97) / 97.0  # a little grit
                px[x, y] = tuple(max(0, min(255, int(c * light * grain))) for c in col)
    return img


def tex_silt():
    """A soft patch of silt with a ragged edge (a decal on the lake bed): alpha cut-out."""
    rnd = random.Random(51)
    pal = [hexc(c) for c in PALETTES["pond_moss"]]
    w = h = 32
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    # value noise
    grid = [[rnd.random() for _ in range(9)] for _ in range(9)]

    def noise(x, y):
        gx, gy = x / w * 8, y / h * 8
        ix, iy = int(gx), int(gy)
        fx, fy = gx - ix, gy - iy
        a = grid[iy][ix] * (1 - fx) + grid[iy][ix + 1] * fx
        b = grid[iy + 1][ix] * (1 - fx) + grid[iy + 1][ix + 1] * fx
        return a * (1 - fy) + b * fy

    for y in range(h):
        for x in range(w):
            r = math.hypot(x - 15.5, y - 15.5) / 15.5
            n = noise(x, y)
            if r + (n - 0.5) * 0.5 < 0.9:
                col = pal[int(n * 5.99) % len(pal)]
                img.putpixel((x, y), col + (255,))
    return img


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


def tex_lily_flower():
    """A water lily flower seen from the side: a cup of broad pink-white petals and a yellow centre (alpha cut-out),
    chunky like OoT's flowers."""
    w = h = 16
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    pink, light, yellow = hexc("#e890a8"), hexc("#f4f0e8"), hexc("#e8c030")
    # petals: five broad pointed lobes fanning up from the base
    for p in range(5):
        ang = math.pi * (0.18 + 0.64 * p / 4)
        for y in range(h):
            for x in range(w):
                dx, dy = x + 0.5 - 8, 13.5 - (y + 0.5)
                r = math.hypot(dx, dy)
                a = math.atan2(dy, dx)
                # a lobe: wide near its middle, pointed at the tip
                da = abs(a - ang)
                if r < 7.5 and da < 0.42 * (1 - (r / 7.5) ** 2) + 0.05 and dy > -1:
                    img.putpixel((x, y), lerp(pink, light, min(1.0, r / 7.5 + 0.2)) + (255,))
    for x in range(6, 10):
        for y in range(11, 14):
            img.putpixel((x, y), yellow + (255,))
    return img


TEXTURES = {
    "reed_card": (tex_reed, "ia8"),
    "ribbon_card": (tex_ribbon, "ia8"),
    "bush_card": (tex_bush, "ia8"),
    "pebbles": (tex_pebbles, "ci4"),
    "silt": (tex_silt, "ci4"),
    "bark": (tex_bark, "ci4"),
    "log_rings": (tex_rings, "ci4"),
    "lily_flower": (tex_lily_flower, "ci4"),
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
            for t in self.vt:
                f.write("vt %.4f %.4f\n" % t)
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
    m = Mesh("lily_flower", "lily_flower")
    white = (255, 255, 255)
    for p in range(2):
        ang = math.pi * p / 2
        dx, dz = math.cos(ang) * 6, math.sin(ang) * 6
        a = m.vert((-dx, 0, -dz), (0, 1), white)
        b = m.vert((dx, 0, dz), (1, 1), white)
        c = m.vert((dx, 10, dz), (1, 0), white)
        d = m.vert((-dx, 10, -dz), (0, 0), white)
        m.quad(a, b, c, d)
    return m


MESHES = [
    lambda: crossed_cards("reed_clump", "reed_card", 40, 95, 3, hexc("#184217"), hexc("#8f9342"), 101, lean=4),
    lambda: crossed_cards("ribbon_weed", "ribbon_card", 18, 80, 3, hexc("#133611"), hexc("#6c7a3b"), 102, lean=10),
    lambda: crossed_cards("bush_weed", "bush_card", 46, 42, 3, hexc("#082b07"), hexc("#525d28"), 103),
    mesh_log,
    mesh_stump,
    mesh_stump_top,
    mesh_drowned_tree,
    lambda: mesh_decal("pebble_patch", "pebbles", 120, (255, 255, 255)),
    lambda: mesh_decal("silt_patch", "silt", 140, (255, 255, 255)),
    mesh_lily_flower,
]


def main():
    for name, (fn, fmt) in TEXTURES.items():
        save(fn(), name)
    meshes = [f().write() for f in MESHES]
    for m in meshes:
        print(f"{m.name}: {len(m.v)} verts, {len(m.f)} tris, texture {m.texture}")


if __name__ == "__main__":
    main()
