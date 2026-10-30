#!/usr/bin/env python3
"""Mockups of the reorganised item page (rod, lures, more bottles) drawn over real pause-menu captures.

Inputs are two ares captures of the item page (fishmod/scripts/pausegrid.inc.c and pausegrid_empty.inc.c): the
debug save's full page (cursor on Din's Fire) and the same page empty. The empty page is cleaned of its background
pipes (the bow-to-arrows bracket, the spell column, the bottle shelf); each mockup redraws pipes for its own groups
with pieces cut from that capture, then places the icons: the vanilla items cut out of the full capture (as the game
draws them: dimmed for the wrong age, ammo counts and all), new ones from the ROM's icon textures and placeholder
lure icons drawn here. Output is ROM-derived: it goes to fishmod/out/, never into git.

Usage: item_grid_mockups.py <full.png> <empty.png> <outdir>
"""
import math
import os
import re
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TEX = os.path.join(ROOT, "extracted/ntsc-1.2/assets/textures")
ICONS = os.path.join(TEX, "icon_item_static")

# vanilla grid in the 750x570 capture (measured from the full/empty difference)
COL_X = [175.5, 254.5, 332.0, 413.0, 493.5, 568.5]
ROW_Y = [183.5, 265.5, 341.0, 419.0]
PITCH = 78.6
CX = sum(COL_X) / 6
CY = sum(ROW_Y) / 4
INTERIOR = (134, 142, 614, 462)  # the page's dark window

VANILLA = [
    ["stick", "nut", "bomb", "bow", "arrow_fire", "dins"],
    ["slingshot", "ocarina", "bombchu", "hookshot", "arrow_ice", "farores"],
    ["boomerang", "lens", "beans", "hammer", "arrow_light", "nayrus"],
    [None, None, None, None, "trade_adult", "trade_child"],
]

BOTTLES = ["bottle_fish", "bottle_larva", "bottle_empty", "bottle_bug", "bottle_fairy", "bottle_red", "bottle_blue_fire",
           "bottle_milk"]
TEXTURE_ITEMS = {
    "rod": "gItemIconFishingPoleTex.rgba32.png",
    "dins": "gItemIconDinsFireTex.rgba32.png",
    "bottle_fish": "gItemIconBottleFishTex.rgba32.png",
    "bottle_empty": "gItemIconBottleEmptyTex.rgba32.png",
    "bottle_bug": "gItemIconBottleBugTex.rgba32.png",
    "bottle_fairy": "gItemIconBottleFairyTex.rgba32.png",
    "bottle_red": "gItemIconBottlePotionRedTex.rgba32.png",
    "bottle_blue_fire": "gItemIconBottleBlueFireTex.rgba32.png",
    "bottle_milk": "gItemIconBottleMilkFullTex.rgba32.png",
    "bottle_larva": os.path.join(ROOT, "src/fishmod_data/art/bottle_larva_3.png"),
}
NAMES = {"rod": "Fishing Rod", "lure_crank": "Crankbait", "lure_frog": "Frog Lure", "lure_sink": "Sinking Lure"}


def load(p):
    return np.asarray(Image.open(p).convert("RGB")).astype(np.float32)


def blur3(a):
    p = np.pad(a, [(1, 1), (1, 1)] + [(0, 0)] * (a.ndim - 2), mode="edge")
    h, w = a.shape[:2]
    return sum(p[dy:dy + h, dx:dx + w] for dy in range(3) for dx in range(3)) / 9.0


def dilate(m, n):
    for _ in range(n):
        p = np.pad(m, 1)
        m = p[1:-1, 1:-1] | p[:-2, 1:-1] | p[2:, 1:-1] | p[1:-1, :-2] | p[1:-1, 2:]
    return m


def inpaint(img, mask):
    """Fills `mask` from its edges inward, then smooths it (the page's backdrop is a blurred, dark game frame)."""
    out = img.copy()
    known = (~mask).astype(np.float32)
    out[mask] = 0
    while known.min() < 1:
        k = blur3(known)
        s = blur3(out * known[..., None])
        grow = (known == 0) & (k > 0)
        out[grow] = s[grow] / k[grow][:, None]
        known[grow] = 1
    for _ in range(60):
        b = blur3(out)
        out[mask] = b[mask]
    return out


def rect_mask(shape, rects):
    m = np.zeros(shape[:2], bool)
    for x0, y0, x1, y1 in rects:
        m[y0:y1, x0:x1] = True
    return m


# ---------------------------------------------------------------------------------------------------------- page
def clean_page(E, F):
    """The empty page without its pipes and without the info panel's text."""
    pipe_rects = [(388, 156, 520, 204), (466, 156, 520, 368), (544, 156, 596, 368), (140, 396, 446, 444)]
    bluish = (E[..., 2] - E[..., 0]) > 7
    pipes = dilate(bluish & rect_mask(E.shape, pipe_rects), 4)
    panel = (E[..., 0] > 110) & (E[..., 2] < 100) & rect_mask(E.shape, [(230, 476, 520, 530)])  # the yellow prompt
    panel = dilate(panel, 3)
    C = inpaint(E, pipes | panel)
    # the empty page's cursor sits on the R tab; the full capture's is on the grid, so its R tab is clean
    C[236:345, 620:722] = F[236:345, 620:722]
    return C, pipes


def sprite(E, C, box, circle=None):
    """A pipe piece cut from the empty page: alpha from how far it stands out from the cleaned page."""
    x0, y0, x1, y1 = box
    e = E[y0:y1, x0:x1]
    c = C[y0:y1, x0:x1]
    a = np.clip((np.abs(e - c).max(2) - 4) / 22.0, 0, 1)
    if circle:
        yy, xx = np.mgrid[y0:y1, x0:x1]
        a *= np.clip(circle[2] + 0.5 - np.hypot(xx - circle[0], yy - circle[1]), 0, 1)
    rgb = np.clip((e - (1 - a[..., None]) * c) / np.maximum(a[..., None], 0.05), 0, 255)
    return Image.fromarray(np.dstack([rgb, a * 255]).astype(np.uint8), "RGBA")


class Pipes:
    def __init__(self, E, C):
        self.knob = sprite(E, C, (548, 158, 592, 204), circle=(569.5, 180.5, 21))
        self.vbar = sprite(E, C, (552, 206, 588, 238))  # between two knobs of the spell column
        self.hbar = sprite(E, C, (436, 160, 470, 200))  # the bow-to-arrows bar
        self.shelf = sprite(E, C, (140, 396, 446, 444))

    def bar(self, img, a, b, k):
        (x0, y0), (x1, y1) = a, b
        if abs(y1 - y0) < 1:  # horizontal
            src = self.hbar
            L = int(abs(x1 - x0))
            h = int(src.height * k)
            piece = src.resize((max(L, 1), h), Image.BILINEAR)
            img.alpha_composite(piece, (int(min(x0, x1)), int(y0 - h / 2 + (src.height / 2 - 20) * k)))
        else:
            src = self.vbar
            L = int(abs(y1 - y0))
            w = int(src.width * k)
            piece = src.resize((w, max(L, 1)), Image.BILINEAR)
            img.alpha_composite(piece, (int(x0 - w / 2), int(min(y0, y1))))

    def knob_at(self, img, p, k):
        s = self.knob.resize((int(self.knob.width * k), int(self.knob.height * k)), Image.BILINEAR)
        img.alpha_composite(s, (int(p[0] - s.width / 2), int(p[1] - s.height / 2)))

    def chain(self, img, pts, k):
        for a, b in zip(pts, pts[1:]):
            if a[0] != b[0] and a[1] != b[1]:  # an elbow: across, then down
                mid = (b[0], a[1])
                self.bar(img, a, mid, k)
                self.bar(img, mid, b, k)
            else:
                self.bar(img, a, b, k)
        for p in pts:
            self.knob_at(img, p, k)

    def shelf_at(self, img, x0, x1, y, k):
        src = self.shelf
        h = int(src.height * k)
        cap = 22
        left = src.crop((0, 0, cap, src.height)).resize((int(cap * k), h), Image.BILINEAR)
        right = src.crop((src.width - cap, 0, src.width, src.height)).resize((int(cap * k), h), Image.BILINEAR)
        mid_w = int(x1 - x0) - 2 * left.width
        mid = src.crop((cap, 0, src.width - cap, src.height)).resize((max(mid_w, 1), h), Image.BILINEAR)
        top = int(y - h / 2)
        img.alpha_composite(left, (int(x0), top))
        img.alpha_composite(mid, (int(x0) + left.width, top))
        img.alpha_composite(right, (int(x0) + left.width + mid_w, top))


# ---------------------------------------------------------------------------------------------------------- items
def vanilla_sprites(F, E):
    out = {}
    d = np.abs(F - E).max(2)
    for r, row in enumerate(VANILLA):
        for c, key in enumerate(row):
            if key is None:
                continue
            x, y = COL_X[c], ROW_Y[r]
            x0, y0 = int(x - 40), int(y - 40)
            f = F[y0:y0 + 80, x0:x0 + 80]
            e = E[y0:y0 + 80, x0:x0 + 80]
            a = np.clip((d[y0:y0 + 80, x0:x0 + 80] - 10) / 40.0, 0, 1)
            if key == "dins":  # under the capture's cursor: drawn from its texture instead
                continue
            rgb = np.clip((f - (1 - a[..., None]) * e) / np.maximum(a[..., None], 0.05), 0, 255)
            out[key] = Image.fromarray(np.dstack([rgb, a * 255]).astype(np.uint8), "RGBA")
    return out


def icon_size_px(F, E):
    """How big a 32x32 icon is drawn on the page: the bomb's cut-out width against its texture's."""
    tex = np.asarray(Image.open(os.path.join(ICONS, "gItemIconBombTex.rgba32.png")).convert("RGBA"))[..., 3]
    cols = np.where(tex.max(0) > 64)[0]
    d = np.abs(F - E).max(2)[150:217, 296:368] > 40
    dc = np.where(d.sum(0) > 2)[0]
    return 32 * (dc[-1] - dc[0] + 1) / (cols[-1] - cols[0] + 1)


def lure_icon(kind):
    """A placeholder 32x32 lure icon in the item icons' manner (drawn big, shaded, outlined, shrunk)."""
    S = 256
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    out = (20, 16, 12, 255)

    def hook(x, y, r=26):
        d.arc((x - r, y - r, x + r, y + r), 20, 200, fill=out, width=14)
        d.arc((x - r, y - r, x + r, y + r), 20, 200, fill=(200, 205, 215, 255), width=7)

    if kind == "lure_crank":  # a fat yellow-green crankbait with a red lip and a treble hook under its belly
        d.line((60, 150, 30, 190), fill=out, width=12)
        d.line((112, 170, 112, 205), fill=out, width=10)
        hook(112, 215, 22)
        d.ellipse((40, 70, 220, 175), fill=out)
        d.ellipse((48, 78, 212, 167), fill=(170, 200, 40, 255))
        d.ellipse((60, 84, 190, 128), fill=(235, 240, 120, 255))
        for i in range(5):
            d.arc((70 + i * 24, 100, 110 + i * 24, 150), 200, 340, fill=(60, 110, 30, 255), width=6)
        d.polygon([(46, 120), (6, 150), (22, 104)], fill=out)
        d.polygon([(42, 120), (14, 140), (26, 110)], fill=(220, 40, 30, 255))
        d.ellipse((168, 92, 200, 124), fill=out)
        d.ellipse((172, 96, 196, 120), fill=(255, 255, 255, 255))
        d.ellipse((179, 102, 192, 115), fill=(10, 10, 10, 255))
        d.polygon([(212, 110), (248, 96), (250, 120), (214, 132)], fill=out)  # tail ring
    elif kind == "lure_frog":  # a green popper frog with orange belly and two hooks
        for sx in (-1, 1):
            d.line((128, 150, 128 + sx * 70, 200), fill=out, width=26)
            d.line((128, 150, 128 + sx * 70, 200), fill=(70, 150, 40, 255), width=14)
            d.ellipse((128 + sx * 70 - 18, 188, 128 + sx * 70 + 18, 214), fill=out)
        d.ellipse((58, 60, 198, 196), fill=out)
        d.ellipse((66, 68, 190, 188), fill=(80, 170, 50, 255))
        d.ellipse((92, 120, 164, 184), fill=(240, 150, 50, 255))
        d.ellipse((80, 76, 140, 120), fill=(140, 220, 90, 255))
        for ex in (88, 168):
            d.ellipse((ex - 24, 40, ex + 24, 88), fill=out)
            d.ellipse((ex - 18, 46, ex + 18, 82), fill=(255, 230, 60, 255))
            d.ellipse((ex - 8, 56, ex + 8, 74), fill=(10, 10, 10, 255))
        hook(96, 222, 20)
        hook(160, 222, 20)
    else:  # sinking lure: a silver-pink spoon with a red bead and a hook
        d.line((128, 18, 128, 60), fill=out, width=10)
        d.ellipse((112, 50, 144, 82), fill=out)
        d.ellipse((118, 56, 138, 76), fill=(220, 40, 60, 255))
        d.polygon([(128, 70), (196, 140), (170, 206), (128, 222), (86, 206), (60, 140)], fill=out)
        d.polygon([(128, 82), (186, 142), (164, 198), (128, 212), (92, 198), (70, 142)], fill=(205, 210, 225, 255))
        d.polygon([(128, 92), (170, 142), (150, 186), (128, 196)], fill=(250, 170, 200, 255))
        d.polygon([(100, 120), (118, 104), (112, 170), (96, 160)], fill=(255, 255, 255, 230))
        hook(128, 228, 22)
    im = im.filter(ImageFilter.GaussianBlur(1.2))
    im = im.crop(im.getbbox())  # fill the icon as the game's icons do
    f = 30.0 / max(im.size)
    im = im.resize((max(1, int(im.width * f * 8)), max(1, int(im.height * f * 8))), Image.LANCZOS)
    big = Image.new("RGBA", (256, 256), (0, 0, 0, 0))
    big.alpha_composite(im, ((256 - im.width) // 2, (256 - im.height) // 2))
    return big.resize((32, 32), Image.LANCZOS)


def texture_icon(key):
    p = TEXTURE_ITEMS[key]
    if not os.path.isabs(p):
        p = os.path.join(ICONS, p)
    return Image.open(p).convert("RGBA")


# ---------------------------------------------------------------------------------------------------------- text
def font():
    widths = []
    src = open(os.path.join(ROOT, "src/code/z_message.c")).read()
    block = src[src.index("f32 sFontWidths[144]"):]
    widths = [float(v) for v in re.findall(r"([0-9.]+)f,", block[:block.index("};")])]
    glyphs = {}
    for f in os.listdir(os.path.join(TEX, "nes_font_static")):
        m = re.match(r"gMsgChar([0-9A-F]{2})", f)
        if m:
            glyphs[int(m.group(1), 16)] = np.asarray(Image.open(os.path.join(TEX, "nes_font_static", f)).convert("L"))
    return widths, glyphs


def draw_name(img, text, cx, cy, fnt, scale):
    widths, glyphs = fnt
    total = sum(widths[ord(ch) - 32] for ch in text)
    W, H = int(total * scale) + 40, int(16 * scale) + 20
    layer = np.zeros((H, W), np.float32)
    x = 10.0
    for ch in text:
        g = glyphs.get(ord(ch))
        if g is not None:
            gi = Image.fromarray(g).resize((int(16 * scale), int(16 * scale)), Image.BILINEAR)
            ga = np.asarray(gi).astype(np.float32) / 255.0
            xi = int(x)
            layer[10:10 + ga.shape[0], xi:xi + ga.shape[1]] = np.maximum(layer[10:10 + ga.shape[0], xi:xi + ga.shape[1]], ga)
        x += widths[ord(ch) - 32] * scale
    a = Image.fromarray((layer * 255).astype(np.uint8))
    shadow = Image.new("RGBA", (W, H), (0, 0, 0, 255))
    shadow.putalpha(a.point(lambda v: int(v * 0.8)))
    white = Image.new("RGBA", (W, H), (255, 255, 255, 255))
    white.putalpha(a)
    ox, oy = int(cx - (total * scale) / 2 - 10), int(cy - 8 * scale - 10)
    img.alpha_composite(shadow, (ox + 2, oy + 2))
    img.alpha_composite(white, (ox, oy))


# ---------------------------------------------------------------------------------------------------------- layout
def cursor(img, x, y, size, k):
    corners = ["TopLeft", "TopRight", "BottomLeft", "BottomRight"]
    s = int(16 * 2.3 * k)
    for i, n in enumerate(corners):
        t = Image.open(os.path.join(ICONS, f"gPauseMenuCursor{n}Tex.ia4.png")).convert("LA")
        a = np.asarray(t)[..., 1]
        lum = np.asarray(t)[..., 0].astype(np.float32) / 255.0
        rgba = np.dstack([255 * lum, 255 * lum, 0 * lum, a]).astype(np.uint8)
        piece = Image.fromarray(rgba, "RGBA").resize((s, s), Image.BILINEAR)
        px = x - size / 2 if i % 2 == 0 else x + size / 2 - s
        py = y - size / 2 if i < 2 else y + size / 2 - s
        img.alpha_composite(piece, (int(px), int(py)))


def c_frame(img, x, y, size, k):
    d = ImageDraw.Draw(img)
    w = max(2, int(round(3 * k)))
    h = size / 2
    d.rectangle((x - h, y - h, x + h, y + h), outline=(235, 235, 235, 255), width=w)


def render(base, layout, sprites, pipes, icon_px, fnt, path):
    rows = layout["grid"]
    nr, nc = len(rows), len(rows[0])
    px = PITCH * 6 / nc
    py = PITCH * 4 / nr
    pitch_x, pitch_y = px, py
    k = min(px, py) / PITCH
    xs = [CX + (c - (nc - 1) / 2) * pitch_x for c in range(nc)]
    ys = [CY + (r - (nr - 1) / 2) * pitch_y for r in range(nr)]
    img = base.copy()

    def at(key):
        for r, row in enumerate(rows):
            if key in row:
                return xs[row.index(key)], ys[r]
        raise KeyError(key)

    for chain in layout.get("chains", []):
        pipes.chain(img, [at(key) for key in chain], k)
    for shelf in layout.get("shelves", []):
        x0, y0 = at(shelf[0])
        x1, _ = at(shelf[-1])
        pipes.shelf_at(img, x0 - pitch_x / 2 + 4 * k, x1 + pitch_x / 2 - 4 * k, y0, k)

    frame = 70 * k
    for key in layout.get("c_items", []):
        c_frame(img, *at(key), frame, k)
    for r, row in enumerate(rows):
        for c, key in enumerate(row):
            if key is None:
                continue
            if key in sprites:
                s = sprites[key]
                n = int(round(80 * k))
                s = s.resize((n, n), Image.BILINEAR)
            else:
                t = lure_icon(key) if key.startswith("lure") else texture_icon(key)
                n = int(round(icon_px * k))
                s = t.resize((n, n), Image.BILINEAR)
            img.alpha_composite(s, (int(xs[c] - s.width / 2), int(ys[r] - s.height / 2)))
    hover = layout.get("cursor")
    if hover:
        cursor(img, *at(hover), 74 * k, k)
        draw_name(img, NAMES.get(hover, hover), 375, 499, fnt, 2.0)
    img.convert("RGB").save(path)


LAYOUTS = {
    # The roadmap's plan: one more column. The rod tops it and two lures hang under it like a line; one more bottle.
    "A_seven_columns": {
        "grid": [
            ["stick", "nut", "bomb", "bow", "arrow_fire", "dins", "rod"],
            ["slingshot", "ocarina", "bombchu", "hookshot", "arrow_ice", "farores", "lure_crank"],
            ["boomerang", "lens", "beans", "hammer", "arrow_light", "nayrus", "lure_sink"],
            ["bottle_fish", "bottle_larva", "bottle_empty", "bottle_bug", "bottle_fairy", "trade_adult", "trade_child"],
        ],
        "chains": [["bow", "arrow_fire", "arrow_ice", "arrow_light"], ["dins", "farores", "nayrus"],
                   ["rod", "lure_crank", "lure_sink"]],
        "shelves": [["bottle_fish", "bottle_fairy"]],
        "c_items": ["rod", "bottle_fish", "bottle_larva"],
        "cursor": "rod",
    },
    # Rod and lures copy the bow and arrows exactly (a bracket across, then down a column of three); the trade items
    # step under the rod and the whole bottom row is one long bottle shelf.
    "B_eight_columns": {
        "grid": [
            ["stick", "nut", "bomb", "bow", "arrow_fire", "dins", "rod", "lure_crank"],
            ["slingshot", "ocarina", "bombchu", "hookshot", "arrow_ice", "farores", "trade_adult", "lure_frog"],
            ["boomerang", "lens", "beans", "hammer", "arrow_light", "nayrus", "trade_child", "lure_sink"],
            BOTTLES,
        ],
        "chains": [["bow", "arrow_fire", "arrow_ice", "arrow_light"], ["dins", "farores", "nayrus"],
                   ["rod", "lure_crank", "lure_frog", "lure_sink"]],
        "shelves": [["bottle_fish", "bottle_milk"]],
        "c_items": ["rod", "bottle_fish", "bottle_larva"],
        "cursor": "rod",
    },
    # Six columns as vanilla, one more row: a fishing row (the rod with its lures strung along the line, then the
    # trade items), and a shelf of six bottles under it.
    "C_fishing_row": {
        "grid": [
            ["stick", "nut", "bomb", "bow", "arrow_fire", "dins"],
            ["slingshot", "ocarina", "bombchu", "hookshot", "arrow_ice", "farores"],
            ["boomerang", "lens", "beans", "hammer", "arrow_light", "nayrus"],
            ["rod", "lure_crank", "lure_frog", "lure_sink", "trade_adult", "trade_child"],
            BOTTLES[:6],
        ],
        "chains": [["bow", "arrow_fire", "arrow_ice", "arrow_light"], ["dins", "farores", "nayrus"],
                   ["rod", "lure_crank", "lure_frog", "lure_sink"]],
        "shelves": [["bottle_fish", "bottle_red"]],
        "c_items": ["rod", "bottle_fish", "bottle_larva"],
        "cursor": "rod",
    },
}


def main():
    full, empty, outdir = sys.argv[1:4]
    os.makedirs(outdir, exist_ok=True)
    F, E = load(full), load(empty)
    C, _ = clean_page(E, F)
    base = Image.fromarray(np.clip(C, 0, 255).astype(np.uint8)).convert("RGBA")
    pipes = Pipes(E, C)
    sprites = vanilla_sprites(F, E)
    icon_px = icon_size_px(F, E)
    print(f"icon size on the page: {icon_px:.1f} px")
    fnt = font()
    Image.fromarray(F.astype(np.uint8)).save(os.path.join(outdir, "0_vanilla.png"))
    base.convert("RGB").save(os.path.join(outdir, "clean_page.png"))
    for name, layout in LAYOUTS.items():
        render(base, layout, sprites, pipes, icon_px, fnt, os.path.join(outdir, f"{name}.png"))
        print("wrote", name)


if __name__ == "__main__":
    main()
