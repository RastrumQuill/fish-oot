#!/usr/bin/env python3
"""fish-oot: a lossless texture pass over the extracted assets, for the RDP (build.sh --texpack). Game-wide: every
texture whose definition and loads it can rewrite, in every scene and object.

Each texture is made as small as it can be with every pixel drawn the same (and no extra work for the RDP):

- **colour-indexed**: a 16-bit RGBA texture with no more than 256 colours becomes 8-bit indexed (CI8) with a palette
  of exactly its colours, shared by the textures drawn together; with no more than 16 colours (each texture of the
  material), 4-bit (CI4): one 16-colour palette, or a palette slot of its own for each texture (the tile's palette
  number). The RDP looks each texel up before filtering, so every pixel drawn is the same;
- **fewer bits where they're all the same**: a 16-bit intensity-alpha texture whose alpha is its intensity becomes
  8-bit intensity (an I texel's alpha is its intensity); one whose values all fit 4 bits exactly becomes IA8; IA8
  with 1-bit alpha and 3-bit intensity exactly, IA4; I8 whose values fit 4 bits exactly, I4;
- **half the texels where they repeat**: a texture whose halves are the same along an axis is stored as one half,
  wrapped by the tile's mask one size down (again while it halves); one whose halves mirror each other, as one half
  with the tile's mirror. Exact, filtering included (the texel past the seam is the same one): only where the tile
  wraps over the whole texture (its mask is the texture's size) or clamps (then the tile's size stays the original's:
  clamping comes before masking), and never twice mirrored;
- **loads not repeated**: a load the same as the one just before it, with nothing that touches the texture memory or
  the tiles between, goes.

Every converted texture is checked before anything is written: decoded back through its palette, its format and its
tile's wrap and mirror, it must give exactly the original texels.

The display lists that load them are rewritten to match:

- the load's format becomes the new one (the 4-bit macros where it's 4-bit), its masks and mirror flags as above, and
  TMEM addresses halve for indexed textures (everything then fits in TMEM's lower half, where CI textures must be:
  the palette sits in the upper half);
- before the material's loads: the palette's load, and the texture-lookup mode set to RGBA16; after the material
  (when the next one isn't converted, or at the list's end), the mode back to none, so nothing after it is looked up;
- textures drawn together in one material share one palette (TMEM holds one 256-colour palette): together they must
  stay within 256 colours, else they stay as they are;
- only where it gains: the textures at 8 bits a texel plus the palette and its load must come to fewer bytes than the
  16-bit textures (a small texture with many colours stays);
- a texture converts only if every use of its name anywhere is a load this pass rewrites (not one used by code, an
  animated-material table or a load it doesn't know), and a list only if it calls no other list and nothing skips its
  end (a cull before its first load is fine);
- a pipe sync with no drawing or loading since the previous one in the same list goes (the pipeline is already empty).

The original tree isn't touched: the output is a copy (hard links for every file the pass doesn't change), which the
build then reads (EXTRACTED_DIR).

Usage: tools/texpack.py SRC_TREE OUT_TREE [--only SUBDIR] [--report]
"""
import collections
import os
import re
import shutil
import struct
import sys
import zlib

import numpy as np
from PIL import Image

DRAW = re.compile(r"gsSP(1Triangle|2Triangles|1Quadrangle|Tri|TextureRectangle|ScisTextureRectangle|Line3D)|"
                  r"gsDP(FillRectangle|LoadBlock|LoadTile|LoadTLUT)")
LOAD = ("gsDPLoadTextureBlock", "gsDPLoadMultiBlock", "gsDPLoadTextureBlock_4b", "gsDPLoadMultiBlock_4b")
CALLS = ("gsSPDisplayList", "gsSPBranchList", "gsSPBranchLessZ", "gsSPBranchLessZraw", "gsSPLoadUcode",
         "gsSPLoadUcodeL", "gsSPBranchLessZrg")
# a texture's definition: u64 NAME[TEX_LEN(u64, NAME_WIDTH, NAME_HEIGHT, BITS)] = { #include "STEM.FMT.inc.c" };
TEXDEF = re.compile(r"u64 (\w+)\[TEX_LEN\(u64, \1_WIDTH, \1_HEIGHT, (\d+)\)\] = \{\n#include \"([^\"]+)\."
                    r"(rgba16|ia16|ia8|ia4|i8|i4)\.inc\.c\"\n\};")
DLDEF = re.compile(r"Gfx (\w+)\[(\d+)\] = \{\n#include \"([^\"]+\.inc\.c)\"\n\};")
WORD = re.compile(r"\b\w+\b")
PALETTE_COST = 64  # bytes: the palette load's commands (texture image, tiles, the load, the lookup mode on and off)
BITS = {"rgba16": 16, "ia16": 16, "ia8": 8, "ia4": 4, "i8": 8, "i4": 4, "ci8": 8, "ci4": 4}
FMT = {"rgba16": ("G_IM_FMT_RGBA", "G_IM_SIZ_16b"), "ia16": ("G_IM_FMT_IA", "G_IM_SIZ_16b"),
       "ia8": ("G_IM_FMT_IA", "G_IM_SIZ_8b"), "ia4": ("G_IM_FMT_IA", "G_IM_SIZ_4b"),
       "i8": ("G_IM_FMT_I", "G_IM_SIZ_8b"), "i4": ("G_IM_FMT_I", "G_IM_SIZ_4b"),
       "ci8": ("G_IM_FMT_CI", "G_IM_SIZ_8b"), "ci4": ("G_IM_FMT_CI", "G_IM_SIZ_4b")}
# textures a later pass rewrites itself (tools/meshpack.py grey_caustics): left as they are
KEEP = {"spot06_scene_000170B8_Tex"}


def parse_commands(text):
    """[(name, args)] for a display list body: gsXxx(args), ... (args with nested parentheses)."""
    out = []
    i = 0
    n = len(text)
    pat = re.compile(r"(gs\w+)\(")
    while i < n:
        m = pat.search(text, i)
        if not m:
            break
        depth = 0
        j = m.end() - 1
        while j < n:
            if text[j] == "(":
                depth += 1
            elif text[j] == ")":
                depth -= 1
                if depth == 0:
                    break
            j += 1
        out.append((m.group(1), " ".join(text[m.end():j].split())))
        i = j + 1
    return out


def split_args(args):
    out, depth, cur = [], 0, ""
    for ch in args:
        if ch == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
            continue
        depth += ch == "("
        depth -= ch == ")"
        cur += ch
    out.append(cur.strip())
    return out


def write_ci_png(path, w, h, indices, palette):
    """A paletted PNG (8-bit indices) with exactly len(palette) PLTE and tRNS entries (build_from_png requires them to
    match); the format (ci8 or ci4) is the file's name."""
    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    raw = b"".join(b"\0" + bytes(indices[y * w:(y + 1) * w]) for y in range(h))
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 3, 0, 0, 0))
    png += chunk(b"PLTE", b"".join(bytes(p[:3]) for p in palette))
    png += chunk(b"tRNS", bytes(p[3] for p in palette))
    png += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    tmp = path + ".tmp"
    open(tmp, "wb").write(png)
    os.replace(tmp, path)


def replace_file(path, text):
    """Write without touching the hard-linked original."""
    tmp = path + ".tmp"
    open(tmp, "w").write(text)
    os.replace(tmp, path)


# each load macro's arguments by name
ARGS = {
    "gsDPLoadTextureBlock": ("timg", "fmt", "siz", "w", "h", "pal", "cms", "cmt", "masks", "maskt", "shifts", "shiftt"),
    "gsDPLoadTextureBlock_4b": ("timg", "fmt", "w", "h", "pal", "cms", "cmt", "masks", "maskt", "shifts", "shiftt"),
    "gsDPLoadMultiBlock": ("timg", "tmem", "rtile", "fmt", "siz", "w", "h", "pal", "cms", "cmt", "masks", "maskt",
                           "shifts", "shiftt"),
    "gsDPLoadMultiBlock_4b": ("timg", "tmem", "rtile", "fmt", "w", "h", "pal", "cms", "cmt", "masks", "maskt", "shifts",
                              "shiftt"),
}


class Load:
    def __init__(self, dl, index, name, args):
        self.dl, self.index, self.kind = dl, index, name
        a = split_args(args)
        self.ok = len(a) == len(ARGS[name])
        self.f = dict(zip(ARGS[name], a)) if self.ok else {}
        if self.ok and name.endswith("_4b"):
            self.f["siz"] = "G_IM_SIZ_4b"
        self.f.setdefault("tmem", "0")
        self.f.setdefault("rtile", "G_TX_RENDERTILE")
        self.tex = a[0]
        self.fmt, self.siz, self.tmem = self.f.get("fmt"), self.f.get("siz"), self.f["tmem"]
        self.multi = name.startswith("gsDPLoadMultiBlock")

    def text(self, f):
        """the load (name, args) with fields f: the 4-bit macro where the size is 4-bit"""
        four = f["siz"] == "G_IM_SIZ_4b"
        name = ("gsDPLoadMultiBlock" if self.multi else "gsDPLoadTextureBlock") + ("_4b" if four else "")
        return name, ", ".join(f[k] for k in ARGS[name])


def num(x):
    x = x.strip()
    if x in ("G_TX_NOMASK", "G_TX_NOLOD"):
        return 0
    try:
        return int(x, 0)
    except ValueError:
        return None


def axis_ok(ld, axis, dim):
    """whether this load's tile lets the texture halve along the axis ("s" or "t") and how it covers it: the texture's
    size a power of two, not mirrored, the mask its size (it wraps over the whole of it) or no mask and clamped"""
    cm = [t.strip() for t in ld.f["cm" + axis].split("|")]
    m = num(ld.f["mask" + axis])
    if m is None or dim & (dim - 1) or "G_TX_MIRROR" in cm:
        return None
    clamp = "G_TX_CLAMP" in cm
    if (1 << m) == dim or (m == 0 and clamp):
        return clamp
    return None


def texels(png, fmt):
    """the texels as the RDP sees them, RGBA 0-255 (H, W, 4)"""
    im = Image.open(png)
    if fmt in ("i8", "i4"):
        g = np.asarray(im.convert("L")).astype(np.int32)
        return np.stack([g, g, g, g], -1)
    if fmt in ("ia16", "ia8", "ia4"):
        a = np.asarray(im.convert("LA")).astype(np.int32)
        return np.stack([a[..., 0], a[..., 0], a[..., 0], a[..., 1]], -1)
    return np.asarray(im.convert("RGBA")).astype(np.int32)


def ex3(v):
    return (v << 5) | (v << 2) | (v >> 1)


def smaller_format(fmt, t):
    """the narrowest exact format for an intensity texture, or None"""
    i, a = t[..., 0], t[..., 3]
    if fmt == "ia16":
        if np.array_equal(i, a):
            return "i8" if not np.all(i % 17 == 0) else "i4"
        if np.all(i % 17 == 0) and np.all(a % 17 == 0):
            return "ia8"
    if fmt == "ia8":
        if np.all(ex3(i >> 5) == i) and np.all((a == 0) | (a == 255)):
            return "ia4"
    if fmt == "i8" and np.all(i % 17 == 0):
        return "i4"
    return None


def halvings(t, axis, bits, allow_mirror):
    """how the texture halves along the axis (1: s, 0: t): (times halved by repeats, then mirrored), keeping at least
    2 texels and, along s, whole 64-bit TMEM lines (the loads' rows)"""
    n = 0
    mirror = False

    def fits(d):
        return d >= 2 and (axis == 0 or (d * bits) % 64 == 0)
    while True:
        d = t.shape[axis]
        if d % 2 or not fits(d // 2):
            break
        lo, hi = np.split(t, 2, axis=axis)
        if np.array_equal(lo, hi):
            t = lo
            n += 1
            continue
        if allow_mirror and np.array_equal(lo, np.flip(hi, axis=axis)):
            t = lo
            mirror = True
        break
    return n, mirror, t


def unfold(t, w0, h0, ms, mt):
    """the texture as the tile reads it over the original's size: wrapped by the mask (its size now), mirrored where
    the mirror flag is (the check that the halving gives the original texels back)"""
    h, w = t.shape[:2]
    s = np.arange(w0)
    tt = np.arange(h0)
    if ms:
        s = np.where((s // w) % 2 == 1, w - 1 - s % w, s % w)
    else:
        s = s % w
    if mt:
        tt = np.where((tt // h) % 2 == 1, h - 1 - tt % h, tt % h)
    else:
        tt = tt % h
    return t[tt][:, s]


def main():
    args = sys.argv[1:]
    report = "--report" in args
    only = None
    if "--only" in args:
        only = args[args.index("--only") + 1]
        args = [a for a in args if a not in ("--only", only)]
    args = [a for a in args if a != "--report"]
    if len(args) != 2:
        sys.exit(__doc__)
    src, out = args
    assets = os.path.join(src, "assets")

    # --- every text file of the tree: names used, texture and list definitions ---
    texts = {}
    for root, _, files in os.walk(assets):
        for f in files:
            if f.endswith((".c", ".h")):
                p = os.path.join(root, f)
                texts[p] = open(p, errors="replace").read()
    uses = collections.Counter()
    for t in texts.values():
        uses.update(WORD.findall(t))
    for extra in ("src", "include", "assets", "fishmod"):  # the repository's own code and assets
        for root, _, files in os.walk(extra):
            for f in files:
                if f.endswith((".c", ".h")):
                    uses.update(WORD.findall(open(os.path.join(root, f), errors="replace").read()))
    texdefs = {}  # name -> (defining .c, include stem, format)
    dldefs = {}   # include path of a list body -> (list name, defining .c)
    for p, t in texts.items():
        if not p.endswith(".c"):
            continue
        for m in TEXDEF.finditer(t):
            if int(m.group(2)) == BITS[m.group(4)]:
                texdefs[m.group(1)] = (p, m.group(3), m.group(4))
        for m in DLDEF.finditer(t):
            dldefs[m.group(3)] = (m.group(1), p)

    # --- the display lists and their loads ---
    lists = {}  # include path -> commands
    for inc in dldefs:
        p = os.path.join(src, inc)
        if p in texts and (only is None or any(inc.startswith(os.path.join("assets", o)) for o in only.split(","))):
            lists[inc] = parse_commands(texts[p])
    loads = []
    for inc, cmds in lists.items():
        for k, (name, a) in enumerate(cmds):
            if name in LOAD:
                loads.append(Load(inc, k, name, a))
    by_tex = collections.defaultdict(list)
    for ld in loads:
        by_tex[ld.tex].append(ld)

    # --- what each texture can become ---
    # a texture can change when every use of its name is its definition, its data's #include, its extern and the
    # loads this pass reads (all in its own format), and its size is defined in its header beside the extern
    tex = {}   # name -> texels (H, W, 4)
    hdr = {}   # name -> header path
    for name, (cfile, stem, fmt) in texdefs.items():
        lds = by_tex.get(name)
        if name in KEEP or not lds or not all(ld.ok and FMT[fmt] == (ld.fmt, ld.siz) for ld in lds):
            continue
        if uses[name] != 3 + len(lds):
            continue
        hfile = cfile[:-2] + ".h"
        if hfile not in texts or not re.search(rf"#define {name}_WIDTH \d+\n#define {name}_HEIGHT \d+",
                                                texts[hfile]):
            continue
        png = os.path.join(src, stem + "." + fmt + ".png")
        if not os.path.exists(png):
            continue
        tex[name] = texels(png, fmt)
        hdr[name] = hfile

    # colour-indexed: rgba16 with <= 256 colours, in material groups (textures loaded together share a palette)
    colours = {}
    eligible = set()
    for name, t in tex.items():
        if texdefs[name][2] != "rgba16" or t.shape[1] % 8:
            continue
        cs = set(map(tuple, t.reshape(-1, 4)))
        if len(cs) <= 256:
            colours[name] = cs
            eligible.add(name)

    # material groups: the loads between two draws, per list; a list qualifies if it calls nothing, and nothing
    # after its first load could end it early
    def groups(inc):
        cmds = lists[inc]
        out, cur = [], []
        for k, (name, a) in enumerate(cmds):
            if name in LOAD:
                cur.append(k)
            elif DRAW.match(name) and cur:
                out.append(cur)
                cur = []
        if cur:
            out.append(cur)
        return out

    def list_ok(inc):
        cmds = lists[inc]
        first = next((k for k, (n, _) in enumerate(cmds) if n in LOAD), None)
        for k, (name, _) in enumerate(cmds):
            if name in CALLS or name.startswith("gsSPBranch") or name in ("gsDPSetTile", "gsDPSetTextureImage"):
                return False
            if name == "gsSPCullDisplayList" and first is not None and k > first:
                return False
        return True

    ok_lists = {inc for inc in lists if list_ok(inc)}
    group_of = {}
    for inc in lists:
        for g in groups(inc):
            for k in g:
                group_of[(inc, k)] = g

    def find_root(parent, t):
        while parent[t] != t:
            parent[t] = parent[parent[t]]
            t = parent[t]
        return t

    def components(names):
        parent = {t: t for t in names}
        for name in names:
            for ld in by_tex[name]:
                for k in group_of[(ld.dl, ld.index)]:
                    other = Load(ld.dl, k, *lists[ld.dl][k]).tex
                    if other in parent:
                        parent[find_root(parent, other)] = find_root(parent, name)
        comps = collections.defaultdict(list)
        for t in sorted(names):
            comps[find_root(parent, t)].append(t)
        return list(comps.values())

    ci_fmt = {}  # texture -> "ci8" or "ci4"
    # settle: a texture stays only if every group it's loaded in converts whole (all its textures convert, the list
    # qualifies, its TMEM addresses halve exactly), and its group's palette (or palettes) fit
    while True:
        changed = False
        for name in sorted(eligible):
            for ld in by_tex[name]:
                g = group_of[(ld.dl, ld.index)]
                g_loads = [Load(ld.dl, k, *lists[ld.dl][k]) for k in g]
                bad = ld.dl not in ok_lists or any(x.tex not in eligible for x in g_loads)
                for x in g_loads:
                    try:
                        bad |= int(x.tmem, 0) % 2 != 0
                    except ValueError:
                        bad = True
                if bad:
                    eligible.discard(name)
                    changed = True
                    break
        ci_fmt = {}
        for members in components(eligible):
            n_colours = len(set().union(*(colours[t] for t in members)))
            nt = sum(tex[t].shape[0] * tex[t].shape[1] for t in members)
            best = None
            # 8-bit, one palette of them all; only where it gains on the 16-bit textures
            if n_colours <= 256:
                best = ("ci8", nt + 2 * n_colours + PALETTE_COST)
            # 4-bit: each texture no more than 16 colours (all of them together: one palette; else a slot each)
            if all(len(colours[t]) <= 16 and tex[t].shape[1] % 16 == 0 for t in members) and len(members) <= 16:
                slots = 1 if n_colours <= 16 else len(members)
                b4 = nt // 2 + 32 * slots + PALETTE_COST
                if best is None or b4 < best[1]:
                    best = ("ci4", b4)
            if best is None or best[1] >= 2 * nt:
                for t in members:
                    eligible.discard(t)
                changed = True
            else:
                for t in members:
                    ci_fmt[t] = best[0]
        if not changed:
            break

    if os.environ.get("TEXPACK_CI") == "0":  # (for bisecting: the display-list pass alone)
        eligible = set()
        ci_fmt = {}
    for skip in filter(None, os.environ.get("TEXPACK_SKIP", "").split(",")):  # (for bisecting: textures kept)
        eligible.discard(skip)
        ci_fmt.pop(skip, None)
        tex.pop(skip, None)

    # each texture's plan: its format (indexed, a narrower intensity one, or its own), then its halvings
    plan = {}  # name -> dict(fmt, t (the stored texels), s/t: (halvings, mirror, clamp flags per load))
    for name, t in sorted(tex.items()):
        fmt0 = texdefs[name][2]
        fmt = ci_fmt.get(name) if name in eligible else None
        if fmt is None and fmt0 != "rgba16":
            fmt = smaller_format(fmt0, t)
        fmt = fmt or fmt0
        lds = by_tex[name]
        h0, w0 = t.shape[:2]
        geo = {}
        st = t
        for axis, ax, dim in (("s", 1, w0), ("t", 0, h0)):
            clamps = [axis_ok(ld, axis, dim) for ld in lds]
            if any(c is None for c in clamps) or os.environ.get("TEXPACK_GEO") == "0":
                geo[axis] = (0, False)
                continue
            n, mirror, st2 = halvings(st, ax, BITS[fmt], True)
            if n or mirror:
                st = st2
            geo[axis] = (n, mirror)
        if fmt == fmt0 and not any(n or m for n, m in geo.values()):
            continue
        # the check: the stored texels, read as the tile reads them, are the original
        if not np.array_equal(unfold(st, w0, h0, geo["s"][1], geo["t"][1]), t):
            sys.exit(f"texpack: {name}: the halved texture doesn't give the original back")
        plan[name] = {"fmt": fmt, "t": st, "w0": w0, "h0": h0, "geo": geo}

    # palettes: one per 8-bit group (all its colours) or 4-bit group of <= 16 colours; else a 16-colour slot each
    palette_of = {}  # texture -> (palette array name, palette, slot, [the textures whose palettes make the array])
    for members in components([t for t in plan if plan[t]["fmt"] in ("ci8", "ci4")]):
        fmt = plan[members[0]]["fmt"]
        allc = sorted(set().union(*(set(map(tuple, plan[t]["t"].reshape(-1, 4))) for t in members)))
        shared = fmt == "ci8" or len(allc) <= 16
        by_file = collections.defaultdict(list)
        for t in members:
            by_file[texdefs[t][0]].append(t)
        for in_file in by_file.values():  # (a copy in each file: a static texture's palette can't be shared)
            pal_name = in_file[0] + "_TPal"
            for t in in_file:
                if shared:
                    palette_of[t] = (pal_name, allc, 0, [in_file[0]])
                else:
                    own = sorted(set(map(tuple, plan[t]["t"].reshape(-1, 4))))
                    own += [own[-1]] * (16 - len(own))
                    palette_of[t] = (pal_name, own, members.index(t), members)
    # the check: each indexed texture's texels through its palette are its texels
    for t, (_, pal, _, _) in palette_of.items():
        index = {c: i for i, c in enumerate(pal)}
        ids = np.array([index[tuple(c)] for c in plan[t]["t"].reshape(-1, 4)])
        if not np.array_equal(np.array(pal)[ids].reshape(plan[t]["t"].shape), plan[t]["t"]):
            sys.exit(f"texpack: {t}: the palette doesn't give its texels back")

    counts = collections.Counter()
    saved = 0
    for name, p in plan.items():
        fmt0 = texdefs[name][2]
        if p["fmt"] != fmt0:
            counts[f"{fmt0} -> {p['fmt']}"] += 1
        for axis in "st":
            n, m = p["geo"][axis]
            if n:
                counts[f"halved by repeats along {axis}"] += 1
            if m:
                counts[f"halved by mirror along {axis}"] += 1
        saved += p["w0"] * p["h0"] * BITS[fmt0] // 8 - p["t"].shape[0] * p["t"].shape[1] * BITS[p["fmt"]] // 8
    print(f"texpack: {len(plan)} of {len(texdefs)} textures smaller ({len(tex)} it can rewrite), "
          f"{saved / 1024:.0f} KB less texture data to load")
    for k, v in sorted(counts.items()):
        print(f"texpack:   {k}: {v}")
    if report:
        return

    # --- the output tree: hard links, then the changed files replaced ---
    if os.path.exists(out):
        shutil.rmtree(out)
    shutil.copytree(src, out, copy_function=os.link)

    changed_text = {}  # source path -> new text

    def text_of(p):
        return changed_text.get(p, texts[p])

    pal_defined = set()
    for name in sorted(plan):
        p = plan[name]
        cfile, stem, fmt0 = texdefs[name]
        fmt = p["fmt"]
        st = p["t"]
        h, w = st.shape[:2]
        os.unlink(os.path.join(out, stem + "." + fmt0 + ".png"))
        dst = os.path.join(out, stem + "." + fmt + ".png")
        if fmt in ("ci8", "ci4"):
            pal_name, pal, _, _ = palette_of[name]
            index = {c: i for i, c in enumerate(pal)}
            write_ci_png(dst, w, h, [index[tuple(c)] for c in st.reshape(-1, 4)], pal)
        elif fmt in ("i8", "i4"):
            Image.fromarray(st[..., 0].astype(np.uint8), "L").save(dst)
        elif fmt in ("ia16", "ia8", "ia4"):
            Image.fromarray(np.stack([st[..., 0], st[..., 3]], -1).astype(np.uint8), "LA").save(dst)
        else:
            Image.fromarray(st.astype(np.uint8), "RGBA").save(dst)
        old = (f"u64 {name}[TEX_LEN(u64, {name}_WIDTH, {name}_HEIGHT, {BITS[fmt0]})] = {{\n"
               f"#include \"{stem}.{fmt0}.inc.c\"\n}};")
        new = (f"u64 {name}[TEX_LEN(u64, {name}_WIDTH, {name}_HEIGHT, {BITS[fmt]})] = {{\n"
               f"#include \"{stem}.{fmt}.inc.c\"\n}};")
        t = text_of(cfile)
        assert old in t, (name, cfile)
        storage = "static " if re.search(rf"static u64 {name}\[", t) else ""
        if fmt in ("ci8", "ci4"):
            pal_name, pal, _, srcs = palette_of[name]
            pal_words = 4 * len(srcs) if len(srcs) > 1 else (len(pal) + 3) // 4  # (u64s: 4 colours each)
            if pal_name not in pal_defined:
                pal_defined.add(pal_name)
                incs = "".join(f"#include \"{texdefs[s][1]}.tlut.rgba16.inc.c\"\n" for s in srcs)
                # (the match starts after any "static ": the palette takes it, and the texture gets its own back)
                new = f"u64 {pal_name}[{pal_words}] = {{\n{incs}}};\n\n{storage}" + new
        changed_text[cfile] = t.replace(old, new, 1)
        hfile = hdr[name]
        ht = text_of(hfile)
        ht = re.sub(rf"#define {name}_WIDTH \d+\n#define {name}_HEIGHT \d+",
                    f"#define {name}_WIDTH {w}\n#define {name}_HEIGHT {h}", ht, count=1)
        decl = re.search(rf"(static |extern )?u64 {name}\[TEX_LEN\(u64, {name}_WIDTH, {name}_HEIGHT, {BITS[fmt0]}\)\];",
                         ht)
        if decl is not None:
            prefix = decl.group(1) or ""
            new_h = f"{prefix}u64 {name}[TEX_LEN(u64, {name}_WIDTH, {name}_HEIGHT, {BITS[fmt]})];"
            if fmt in ("ci8", "ci4"):
                pal_name, pal, _, srcs = palette_of[name]
                pal_words = 4 * len(srcs) if len(srcs) > 1 else (len(pal) + 3) // 4
                pal_decl = f"{prefix}u64 {pal_name}[{pal_words}];"
                if pal_decl not in ht:
                    new_h += "\n" + pal_decl
            ht = ht.replace(decl.group(0), new_h, 1)
        changed_text[hfile] = ht

    def rewrite_load(ld, halve_tmem):
        """the load as the plan has it: format, masks, mirrors, palette slot; and the tile size kept where it clamps"""
        p = plan[ld.tex]
        f = dict(ld.f)
        f["fmt"], f["siz"] = FMT[p["fmt"]]
        h, w = p["t"].shape[:2]
        f["w"], f["h"] = str(w), str(h)
        if halve_tmem:
            f["tmem"] = hex(int(ld.tmem, 0) // 2)
        if ld.tex in palette_of:
            f["pal"] = str(palette_of[ld.tex][2])
        keep_size = False
        for axis, d0, d in (("s", p["w0"], w), ("t", p["h0"], h)):
            n, mirror = p["geo"][axis]
            if not (n or mirror):
                continue
            f["mask" + axis] = str(d.bit_length() - 1)
            if mirror:
                cm = [x.strip() for x in f["cm" + axis].split("|") if x.strip() != "G_TX_NOMIRROR"]
                f["cm" + axis] = " | ".join(["G_TX_MIRROR"] + cm)
            if "G_TX_CLAMP" in f["cm" + axis]:
                keep_size = True
        out = [ld.text(f)]
        if keep_size:
            out.append(("gsDPSetTileSize", f"{f['rtile']}, 0, 0, {(p['w0'] - 1) << 2}, {(p['h0'] - 1) << 2}"))
        return out

    # display lists
    syncs_removed = 0
    reloads_removed = 0
    lists_changed = 0
    conv_ci = {t for t in plan if plan[t]["fmt"] in ("ci8", "ci4")}
    for inc, cmds in lists.items():
        conv_groups = []
        for g in groups(inc):
            texs = [Load(inc, k, *cmds[k]).tex for k in g]
            if all(t in conv_ci for t in texs):
                conv_groups.append(g)
            else:
                assert not any(t in conv_ci for t in texs), (inc, texs)
        all_groups = groups(inc)
        conv = {g[0] for g in conv_groups}
        # each command belongs to a material: the next group's, if no draw comes between them, else the last group's
        owner = [None] * len(cmds)
        nxt = None
        for k in range(len(cmds) - 1, -1, -1):
            name = cmds[k][0]
            starts = [g for g in all_groups if g[0] == k]
            if starts:
                nxt = starts[0][0]
            if DRAW.match(name):
                nxt = None
            owner[k] = nxt
        last = None
        for k in range(len(cmds)):
            if owner[k] is None:
                owner[k] = last
            else:
                last = owner[k]
        own_lut_before_load = set()  # groups whose own setup sets the lookup mode before their first load
        for k, (name, _) in enumerate(cmds):
            if name == "gsDPSetTextureLUT" and owner[k] is not None and k < owner[k]:
                own_lut_before_load.add(owner[k])
        new = []
        drawn = True
        ours = False  # a converted material set RGBA16 here and it isn't put back yet

        def emit(name, a):
            nonlocal drawn
            if name == "gsDPPipeSync":
                drawn = False
            elif DRAW.match(name) or name in LOAD:
                drawn = True
            new.append((name, a))

        def restore():
            nonlocal ours
            if drawn:
                emit("gsDPPipeSync", "")
            emit("gsDPSetTextureLUT", "G_TT_NONE")
            ours = False

        for k, (name, a) in enumerate(cmds):
            g0 = owner[k]
            if name == "gsSPEndDisplayList" and ours:
                restore()
            if k == g0:  # a material's first load
                if g0 in conv:
                    if g0 not in own_lut_before_load:
                        if drawn:
                            emit("gsDPPipeSync", "")
                        emit("gsDPSetTextureLUT", "G_TT_RGBA16")
                    pal_name, pal, _, srcs = palette_of[Load(inc, k, name, a).tex]
                    emit("gsDPLoadTLUT", f"{16 * len(srcs) if len(srcs) > 1 else len(pal)}, 256, {pal_name}")
                    ours = True
                elif ours and g0 not in own_lut_before_load:
                    restore()
            if name == "gsDPSetTextureLUT" and g0 in conv:
                emit(name, "G_TT_RGBA16")
                ours = True
                continue
            if name == "gsDPSetTextureLUT":
                ours = False
            if name in LOAD:
                ld = Load(inc, k, name, a)
                if ld.tex in plan:
                    for c in rewrite_load(ld, ld.multi and g0 in conv):
                        emit(*c)
                    continue
            emit(name, a)
        # the pipe syncs with nothing drawn or loaded since the last one (not the first: drawing came before); a load
        # the same as the one before it, nothing touching the texture memory or the tiles between
        final = []
        busy = True
        last_load = None  # the last texture load and palette load, while TMEM and the tiles still hold them
        last_tlut = None
        for name, a in new:
            if name in LOAD:
                if (name, a) == last_load and os.environ.get("TEXPACK_RELOAD") != "0":
                    reloads_removed += 1
                    continue
                last_load = (name, a)
                if Load(inc, 0, name, a).fmt != "G_IM_FMT_CI":
                    last_tlut = None  # (a texture that isn't indexed may sit in TMEM's upper half, the palette's)
            elif name == "gsDPLoadTLUT":
                if (name, a) == last_tlut and os.environ.get("TEXPACK_RELOAD") != "0":
                    reloads_removed += 1
                    continue
                last_tlut = (name, a)
                if last_load is not None and Load(inc, 0, *last_load).fmt != "G_IM_FMT_CI":
                    last_load = None
            elif name in CALLS or name.startswith("gsSPBranch") or name in (
                    "gsDPSetTile", "gsDPSetTileSize", "gsDPSetTextureImage", "gsDPLoadBlock", "gsDPLoadTile",
                    "gsDPLoadTLUT_pal16", "gsDPLoadTLUT_pal256", "gsDPLoadTLUTCmd", "gsSPEndDisplayList"):
                last_load = last_tlut = None  # (anything else that loads, sets a tile, or leaves the list)
            if name == "gsDPPipeSync":
                if not busy and os.environ.get("TEXPACK_SYNC") != "0":
                    syncs_removed += 1
                    continue
                busy = False
            elif DRAW.match(name) or name in LOAD:
                busy = True
            final.append((name, a))
        if final == [tuple(c) for c in cmds]:
            continue
        lists_changed += 1
        body = "".join(f"    {n}({a}),\n" for n, a in final)
        replace_file(os.path.join(out, inc), body)
        dl_name, cfile = dldefs[inc]
        t = text_of(cfile)
        t2 = re.sub(rf"Gfx {dl_name}\[\d+\] = \{{", f"Gfx {dl_name}[] = {{", t, count=1)
        changed_text[cfile] = t2
        hfile = cfile[:-2] + ".h"
        if hfile in texts:
            changed_text[hfile] = re.sub(rf"extern Gfx {dl_name}\[\d+\];", f"extern Gfx {dl_name}[];",
                                         text_of(hfile), count=1)
    for p, t in changed_text.items():
        replace_file(os.path.join(out, os.path.relpath(p, src)), t)
    print(f"texpack: {lists_changed} display lists rewritten, {syncs_removed} empty pipe syncs removed, "
          f"{reloads_removed} repeated loads removed, {len(changed_text)} C files changed")


if __name__ == "__main__":
    main()
