#!/usr/bin/env python3
"""The item page's new graphics, made at build time from the game's own (so nothing ROM-derived is committed).

- The page background: the vanilla "Select Item" tiles with their pipes (the bow-to-arrows bracket, the spell
  column and the bottle shelf) erased and drawn again, from pieces cut out of the same tiles, for the 6 x 5 grid
  (fishmod/docs/roadmap.md, mockup C): the same bracket and column on the shorter rows, a line from the rod through
  the three lures, and a shelf under six bottles.
- Placeholder icons for the three lures (drawn here; fishmod/tools/item_grid_mockups.py has the same drawings).
- Item names for the rod and the lures in the game's message font, in the item-name format (128 x 16 IA4).

Writes src/fishmod_data/fishmod_item_page.c (a ROM file the pause menu loads next to its own graphics) and, with
--preview DIR, PNGs of what it made.

The grid geometry here must match z_kaleido_scope.c's (fishmod: FISHMOD_ITEM_* in fishmod_item_page.h).
"""
import argparse
import os
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import item_grid_mockups as mock  # noqa: E402  (lure drawings and the font)

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TEX = os.path.join(ROOT, "extracted/ntsc-1.2/assets/textures")
OUT = os.path.join(ROOT, "src/fishmod_data/fishmod_item_page.c")

# page texture space (240 x 160) is the page's vertex space: x = 120 + vx, y = 80 - vy
COL_X = [40 + 32 * c for c in range(6)]
ROW_Y = [80 - (44 - 25 * r) for r in range(5)]  # FISHMOD_ITEM_ROW0_Y 44, FISHMOD_ITEM_ROW_PITCH 25
SCALE = 0.8  # pipes shrink with the icons
INTERIOR = (0, 187)  # the page window's colour (intensity, alpha)

LURES = ["lure_crank", "lure_frog", "lure_sink"]
NAMES = ["Fishing Rod", "Crankbait", "Frog Lure", "Sinking Lure"]
# the page tiles the new pipes change (the others are drawn from the game's own); fishmod_item_page.h has the same
CHANGED_TILES = ["03", "04", "11", "13", "14", "21", "22", "23", "24"]


def load_page():
    page = np.zeros((160, 240, 2), np.float32)
    for c in range(3):
        for r in range(5):
            n = f"{c}{r}"
            if r == 0:
                p = os.path.join(TEX, f"icon_item_nes_static/gPauseSelectItem{n}ENGTex.ia8.png")
            else:
                p = os.path.join(TEX, f"icon_item_static/gPauseSelectItem{n}Tex.ia8.png")
            page[r * 32:(r + 1) * 32, c * 80:(c + 1) * 80] = np.asarray(Image.open(p).convert("LA"), np.float32)
    return page


def piece(page, x0, y0, x1, y1, circle=None):
    """A bit of pipe: its pixels where it's opaque (the window behind it isn't)."""
    p = page[y0:y1, x0:x1].copy()
    a = (p[..., 1] > 250).astype(np.float32)
    if circle:
        yy, xx = np.mgrid[y0:y1, x0:x1]
        a *= (np.hypot(xx + 0.5 - circle[0], yy + 0.5 - circle[1]) <= circle[2])
    return p[..., 0], a


def resize(arr, w, h):
    return np.asarray(Image.fromarray(arr.astype(np.float32), "F").resize((max(1, w), max(1, h)), Image.BILINEAR))


def stamp(page, inten, alpha, x, y):
    """Puts a piece with its top-left at (x, y); opaque where its alpha is more than half."""
    h, w = inten.shape
    x, y = int(round(x)), int(round(y))
    m = alpha > 0.5
    reg = page[y:y + h, x:x + w]
    reg[..., 0][m] = inten[m]
    reg[..., 1][m] = 255


class Pipes:
    def __init__(self, page):
        self.knob = piece(page, 190, 27, 211, 48, circle=(200.5, 37.5, 8.0))
        self.vbar = piece(page, 194, 48, 207, 60)  # between the spell column's top two knobs
        self.hbar = piece(page, 146, 30, 158, 45)  # the bow-to-arrows bar
        self.shelf = piece(page, 28, 122, 150, 144)
        self.cap = 10

    def knob_at(self, page, x, y):
        i, a = self.knob
        w = int(round(i.shape[1] * SCALE))
        stamp(page, resize(i, w, w), resize(a, w, w), x - w / 2, y - w / 2)

    def bar(self, page, a, b):
        (x0, y0), (x1, y1) = a, b
        if y0 == y1:
            i, al = self.hbar
            h = int(round(i.shape[0] * SCALE))
            L = int(abs(x1 - x0))
            stamp(page, resize(i, L, h), resize(al, L, h), min(x0, x1), y0 - h / 2)
        else:
            i, al = self.vbar
            w = int(round(i.shape[1] * SCALE))
            L = int(abs(y1 - y0))
            stamp(page, resize(i, w, L), resize(al, w, L), x0 - w / 2, min(y0, y1))

    def chain(self, page, pts):
        for a, b in zip(pts, pts[1:]):
            if a[0] != b[0] and a[1] != b[1]:  # across, then down
                mid = (b[0], a[1])
                self.bar(page, a, mid)
                self.bar(page, mid, b)
            else:
                self.bar(page, a, b)
        for p in pts:
            self.knob_at(page, *p)

    def shelf_at(self, page, x0, x1, y):
        i, a = self.shelf
        h = int(round(i.shape[0] * SCALE))
        cap = self.cap
        cw = int(round(cap * SCALE))
        mid = int(x1 - x0) - 2 * cw
        top = y - h / 2
        stamp(page, resize(i[:, :cap], cw, h), resize(a[:, :cap], cw, h), x0, top)
        stamp(page, resize(i[:, cap:-cap], mid, h), resize(a[:, cap:-cap], mid, h), x0 + cw, top)
        stamp(page, resize(i[:, -cap:], cw, h), resize(a[:, -cap:], cw, h), x0 + cw + mid, top)


def make_page():
    page = load_page()
    pipes = Pipes(page)
    # erase the vanilla pipes: inside the window, everything opaque goes back to the window's colour
    win = page[23:149, 23:217]
    opaque = win[..., 1] > 250
    win[..., 0][opaque] = INTERIOR[0]
    win[..., 1][opaque] = INTERIOR[1]

    at = lambda c, r: (COL_X[c], ROW_Y[r])  # noqa: E731
    pipes.chain(page, [at(3, 0), at(4, 0), at(4, 1), at(4, 2)])  # bow -> fire, ice, light arrows
    pipes.chain(page, [at(5, 0), at(5, 1), at(5, 2)])  # Din's, Farore's, Nayru's
    pipes.chain(page, [at(0, 3), at(1, 3), at(2, 3), at(3, 3)])  # rod -> the three lures
    pipes.shelf_at(page, COL_X[0] - 12, COL_X[5] + 12, ROW_Y[4])  # six bottles
    return page


def to_ia8(la):
    i = np.clip(np.round(la[..., 0] / 17.0), 0, 15).astype(np.uint8)
    a = np.clip(np.round(la[..., 1] / 17.0), 0, 15).astype(np.uint8)
    return ((i << 4) | a).astype(np.uint8)


def make_name(text, fnt):
    """128 x 16 IA4, centred like the game's item names (white text; the game draws its shadow)."""
    widths, glyphs = fnt
    canvas = np.zeros((16, 128), np.float32)
    total = sum(widths[ord(ch) - 32] for ch in text)
    x = int(round((128 - total) / 2))
    for ch in text:
        g = glyphs.get(ord(ch))
        if g is not None:
            gx = max(0, x)
            reg = canvas[0:16, gx:gx + 16]
            reg[...] = np.maximum(reg, g[:, :reg.shape[1]].astype(np.float32))
        x += int(round(widths[ord(ch) - 32]))
    i = np.clip(np.round(canvas / 255.0 * 7), 0, 7).astype(np.uint8)
    a = (canvas > 40).astype(np.uint8)
    nib = (i << 1) | a
    return ((nib[:, 0::2] << 4) | nib[:, 1::2]).astype(np.uint8), canvas


def c_bytes(name, data):
    words = np.frombuffer(np.ascontiguousarray(data).tobytes(), dtype=">u8")
    lines = [f"    {{ // {name}"]
    for k in range(0, len(words), 4):
        lines.append("        " + ", ".join(f"0x{w:016X}" for w in words[k:k + 4]) + ",")
    lines.append("    },")
    return lines


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--preview", help="write PNGs of the results here")
    args = ap.parse_args()

    page = make_page()
    ia8 = to_ia8(page)
    old = to_ia8(load_page())
    tiles = []
    for c in range(3):
        for r in range(1, 5):  # the title row (00, 10, 20) is per language and left as it is
            t = ia8[r * 32:(r + 1) * 32, c * 80:(c + 1) * 80]
            if (t != old[r * 32:(r + 1) * 32, c * 80:(c + 1) * 80]).any():  # only the tiles that change
                tiles.append((f"{c}{r}", t))
    names_kept = [n for n, _ in tiles]
    if names_kept != CHANGED_TILES:
        sys.exit(f"gen_item_page: the changed tiles are {names_kept}, not {CHANGED_TILES}: update CHANGED_TILES "
                 "and FISHMOD_ITEM_PAGE_TILE_INDEX (fishmod_item_page.h) to match")
    lures = [np.asarray(mock.lure_icon(k).convert("RGBA"), np.uint8) for k in LURES]
    fnt = mock.font()
    names = [make_name(t, fnt) for t in NAMES]

    out = [
        "// Generated by fishmod/tools/gen_item_page.py from the game's own graphics. Do not edit.",
        '#include "fishmod_item_page.h"',
        "",
        "FishmodItemPage gFishmodItemPage = {",
        "    {",
    ]
    for n, t in tiles:
        out += ["    " + l for l in c_bytes(f"gPauseSelectItem{n}Tex, pipes redrawn", t)]
    out += ["    },", "    {"]
    for k, t in zip(LURES, lures):
        out += ["    " + l for l in c_bytes(k, t)]
    out += ["    },", "    {"]
    for n, (t, _) in zip(NAMES, names):
        out += ["    " + l for l in c_bytes(n, t)]
    out += ["    },", "};", ""]
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w") as f:
        f.write("\n".join(out))
    print(f"item page: {len(tiles)} tiles, {len(lures)} lure icons, {len(names)} names")

    if args.preview:
        os.makedirs(args.preview, exist_ok=True)
        i4 = (ia8 >> 4) * 17
        a4 = (ia8 & 15) * 17
        img = Image.fromarray(np.dstack([i4, i4, i4, a4]).astype(np.uint8), "RGBA")
        bg = Image.new("RGBA", img.size, (90, 30, 30, 255))
        bg.alpha_composite(img)
        bg.resize((720, 480), Image.NEAREST).save(os.path.join(args.preview, "item_page_bg.png"))
        for k, t in zip(LURES, lures):
            Image.fromarray(t, "RGBA").resize((128, 128), Image.NEAREST).save(os.path.join(args.preview, f"{k}.png"))
        for n, (_, canvas) in zip(NAMES, names):
            Image.fromarray(canvas.astype(np.uint8)).resize((512, 64), Image.NEAREST).save(
                os.path.join(args.preview, f"name_{n.replace(' ', '_')}.png"))


if __name__ == "__main__":
    main()
